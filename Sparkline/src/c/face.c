#include "ui.h"
#include "settings.h"

// Sparkline: Solfarer's info band with nothing else on the screen.
//
// The band was designed to fit in 56 of Emery's 228 rows so that something
// else could have the rest. This face asks the opposite question — what does
// that information look like when it is allowed all 228? Same grammar, same
// palette: LECO clock, health values right-aligned against a vertical
// hairline with the day column facing them across it, gold bars for the last
// hour of movement. Everything simply given the room to be read at arm's
// length, and the sparkline promoted from a 10px strip to the floor of the
// screen.

static Window *s_win;
static Layer *s_layer;
static int s_hour, s_min, s_mday, s_mon, s_wday;
static bool s_bt_ok = true;
static uint8_t s_batt = 100;

static const char *WD[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
static const char *MO[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                              "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };

static void fmt_time(char *buf, size_t cap) {
  int h = s_hour;
  if (!clock_is_24h_style()) { h %= 12; if (h == 0) h = 12; }
  if (clock_is_24h_style() || g_cfg.leading_zero)
    snprintf(buf, cap, "%02d:%02d", h, s_min);
  else
    snprintf(buf, cap, "%d:%02d", h, s_min);
}

static bool health_on(void) { return g_cfg.show_health; }

// ---------------------------------------------------------------------------
// Right-alignment is done by hand throughout: measure, then draw left-aligned
// at (right edge - width) in a roomy box. The renderer's own right-aligned
// layout misplaces interior glyphs when the box runs near its width — digits
// come out fused on top of each other.
// ---------------------------------------------------------------------------
static GSize text_size(const char *s, GFont f) {
  // The measuring box has to clear the tallest font on the face. At 44 it
  // was shorter than LECO 60, which measured wrong and silently demoted the
  // clock two sizes.
  return graphics_text_layout_get_content_size(s, f, GRect(0, 0, 220, 100),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

static void draw_right(GContext *ctx, const char *s, GFont f, int right, int y,
                       int min_x) {
  GSize sz = text_size(s, f);
  int x = right - sz.w;
  if (x < min_x) x = min_x;
  graphics_draw_text(ctx, s, f, GRect(x, y, sz.w + 8, sz.h + 6),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void draw_left(GContext *ctx, const char *s, GFont f, int x, int y) {
  GSize sz = text_size(s, f);
  graphics_draw_text(ctx, s, f, GRect(x, y, sz.w + 8, sz.h + 6),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

// Health mode's shake: no card — the bpm row swaps to hours slept for a few
// seconds (sleep is the only figure the face doesn't already show).
#define SLEEP_PEEK_MS 6000
static time_t s_sleep_peek_at;
static AppTimer *s_peek_timer;

static bool sleep_peek(void) {
  return s_sleep_peek_at != 0 &&
         time(NULL) - s_sleep_peek_at < SLEEP_PEEK_MS / 1000;
}

static void peek_expire(void *ctx) {
  s_peek_timer = NULL;
  s_sleep_peek_at = 0;
  face_poke();
}

// ---------------------------------------------------------------------------
// Health — the same machinery as Solfarer, including the one deliberate
// quirk: the minute history is fetched once at boot and then exactly once
// more, at the first quarter-hour mark, because that is when the watch has
// filled in the past hour it was not running for. After that the sparkline is
// kept live from the step delta and never refetches.
// ---------------------------------------------------------------------------
static void fmt_thousands(char *buf, size_t cap, int v, bool comma) {
  if (comma && v >= 1000) snprintf(buf, cap, "%d,%03d", v / 1000, v % 1000);
  else snprintf(buf, cap, "%d", v);
}

//#define DEV_FAKE_HEALTH

#if defined(PBL_HEALTH)
static int steps_today(void) {
#ifdef DEV_FAKE_HEALTH
  return 8842;
#endif
  return (int)health_service_sum_today(HealthMetricStepCount);
}
static int walked_m_today(void) {
#ifdef DEV_FAKE_HEALTH
  return 6300;
#endif
  return (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
}
static int sleep_secs(void) {
#ifdef DEV_FAKE_HEALTH
  return 7 * 3600 + 42 * 60;
#endif
  time_t start = time_start_of_today(), end = time(NULL);
  HealthServiceAccessibilityMask m =
      health_service_metric_accessible(HealthMetricSleepSeconds, start, end);
  if (!(m & HealthServiceAccessibilityMaskAvailable)) return 0;
  return (int)health_service_sum_today(HealthMetricSleepSeconds);
}
static int hr_bpm(void) {
#ifdef DEV_FAKE_HEALTH
  return 68;
#endif
#if PBL_API_EXISTS(health_service_peek_current_value)
  return (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
#else
  return 0;
#endif
}

static uint8_t s_minsteps[60];
static int s_step_snap = -1;
static bool s_refetch_pending;

static void fetch_minute_history(void) {
#ifdef DEV_FAKE_HEALTH
  for (int i = 0; i < 60; i++) {
    int ago = 59 - i, st = 0;
    if (ago >= 40 && ago < 46) st = 70 + (i * 7) % 40;
    else if (ago >= 12 && ago < 22) st = 15 + (i * 5) % 30;
    else if (ago < 4) st = 55 + (i * 11) % 45;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    s_minsteps[((t->tm_min - ago) % 60 + 60) % 60] = st;
  }
  return;
#endif
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int cur = t->tm_min;
  HealthMinuteData *md = malloc(60 * sizeof *md);
  if (!md) return;
  time_t end = now, start = now - SECONDS_PER_HOUR;
  uint32_t nrec = health_service_get_minute_history(md, 60, &start, &end);
  for (uint32_t i = 0; i < nrec; i++) {
    int idx = ((int)i + 1 + cur) % 60;
    if (!md[i].is_invalid) s_minsteps[idx] = md[i].steps;
  }
  free(md);
}

static void minute_track(struct tm *t) {
  s_minsteps[t->tm_min] = 0;
  s_step_snap = steps_today();
  if (s_refetch_pending && t->tm_min % 15 == 1) {
    fetch_minute_history();
    s_refetch_pending = false;
  }
}

static int minute_steps_live(void) {
  if (s_step_snap < 0) return 0;
  int d = steps_today() - s_step_snap;
  return d > 0 ? d : 0;
}

static void health_evt(HealthEventType e, void *ctx) {
  if (e == HealthEventMovementUpdate) face_poke();
}

static void health_init(void) {
  fetch_minute_history();
  s_refetch_pending = true;
  s_step_snap = steps_today();
  health_service_events_subscribe(health_evt, NULL);
}
static void health_deinit(void) { health_service_events_unsubscribe(); }
#else
static int steps_today(void)    { return 0; }
static int walked_m_today(void) { return 0; }
static int sleep_secs(void)     { return 0; }
static int hr_bpm(void)         { return 0; }
static uint8_t s_minsteps[60];
static void minute_track(struct tm *t) { (void)t; }
static int minute_steps_live(void) { return 0; }
static void health_init(void) {}
static void health_deinit(void) {}
#endif

// ---------------------------------------------------------------------------
// Weather — phone fetches (Open-Meteo, pkjs), watch shows the latest number.
// ---------------------------------------------------------------------------
#define KEY_WEATHER 30
typedef struct { int32_t at; int16_t temp; } WeatherSave;
static int16_t s_temp;
static int32_t s_temp_at;

void face_set_temp(int temp) {
  s_temp = temp;
  s_temp_at = (int32_t)time(NULL);
  WeatherSave w = { s_temp_at, s_temp };
  persist_write_data(KEY_WEATHER, &w, sizeof w);
  face_poke();
}

static bool temp_fresh(void) {
  return g_cfg.weather_on && s_temp_at != 0 &&
         time(NULL) - s_temp_at < 3 * SECONDS_PER_HOUR;
}

static void weather_load(void) {
  if (persist_exists(KEY_WEATHER) &&
      persist_get_size(KEY_WEATHER) == (int)sizeof(WeatherSave)) {
    WeatherSave w;
    persist_read_data(KEY_WEATHER, &w, sizeof w);
    s_temp_at = w.at;
    s_temp = w.temp;
  }
}

void face_poke(void) {
  if (s_layer) layer_mark_dirty(s_layer);
}

// ---------------------------------------------------------------------------
// Typefaces. Each family is described by the face it uses for the clock and
// the one it uses for the step count, plus the y it wants to be drawn at —
// families disagree wildly about how much of their box is ink, so a single
// shared offset cannot centre them all. Two custom faces are bundled
// (Montserrat and Roboto) subset to digits, colon and comma.
//
// `comma` records whether the step face can render a thousands separator at
// all: the numeric-only system faces cannot, and would drop a blank.
// ---------------------------------------------------------------------------
typedef struct {
  const char *clock_sys[2];       // [bold, light]; NULL when custom-loaded
  uint32_t clock_res[2];
  const char *steps_sys[2];
  uint32_t steps_res[2];
  int clock_y, steps_dy;
  bool comma;
} FontSpec;

static const FontSpec FONTS[CF_COUNT] = {
  [CF_LECO] = {
    .clock_sys = { FONT_KEY_LECO_60_BOLD_NUMBERS_AM_PM,
                   FONT_KEY_LECO_60_NUMBERS_AM_PM },
    .steps_sys = { FONT_KEY_LECO_32_BOLD_NUMBERS, FONT_KEY_LECO_28_LIGHT_NUMBERS },
    .clock_y = -6, .steps_dy = -6, .comma = true },
  [CF_MONT] = {
    .clock_res = { RESOURCE_ID_FONT_MONT_B_58, RESOURCE_ID_FONT_MONT_L_58 },
    .steps_res = { RESOURCE_ID_FONT_MONT_B_34, RESOURCE_ID_FONT_MONT_L_34 },
    .clock_y = -1, .steps_dy = -4, .comma = true },
  [CF_ROBOTO] = {
    .clock_res = { RESOURCE_ID_FONT_ROBO_B_58, RESOURCE_ID_FONT_ROBO_L_58 },
    .steps_res = { RESOURCE_ID_FONT_ROBO_B_34, RESOURCE_ID_FONT_ROBO_L_34 },
    .clock_y = 1, .steps_dy = -4, .comma = true },
  [CF_BITHAM] = {
    .clock_sys = { FONT_KEY_BITHAM_42_BOLD, FONT_KEY_BITHAM_42_LIGHT },
    .steps_sys = { FONT_KEY_BITHAM_34_MEDIUM_NUMBERS,
                   FONT_KEY_BITHAM_34_MEDIUM_NUMBERS },
    .clock_y = 8, .steps_dy = -4, .comma = false },
  [CF_ROBOTO_SYS] = {                      // digits and a colon, nothing else
    .clock_sys = { FONT_KEY_ROBOTO_BOLD_SUBSET_49,
                   FONT_KEY_ROBOTO_BOLD_SUBSET_49 },
    .steps_sys = { FONT_KEY_GOTHIC_28_BOLD, FONT_KEY_GOTHIC_28 },
    .clock_y = 2, .steps_dy = 0, .comma = true },
  [CF_DROID] = {
    .clock_sys = { FONT_KEY_DROID_SERIF_28_BOLD, FONT_KEY_DROID_SERIF_28_BOLD },
    .steps_sys = { FONT_KEY_DROID_SERIF_28_BOLD, FONT_KEY_DROID_SERIF_28_BOLD },
    .clock_y = 15, .steps_dy = 0, .comma = true },
  [CF_GOTHIC] = {
    .clock_sys = { FONT_KEY_GOTHIC_28_BOLD, FONT_KEY_GOTHIC_28 },
    .steps_sys = { FONT_KEY_GOTHIC_28_BOLD, FONT_KEY_GOTHIC_28 },
    .clock_y = 18, .steps_dy = 0, .comma = true },
};

static const FontSpec *spec(void) { return &FONTS[g_cfg.clock_font]; }
static int weight(void) { return g_cfg.bold_font ? 0 : 1; }

// Custom faces are loaded on demand and held until the choice changes; two
// subset faces cost a few hundred bytes, and reloading on every redraw would
// hit flash once a minute for nothing.
static GFont s_cf, s_sf;
static uint32_t s_cf_res, s_sf_res;

static GFont custom(GFont *slot, uint32_t *cur, uint32_t res) {
  if (*cur != res) {
    if (*slot) fonts_unload_custom_font(*slot);
    *slot = fonts_load_custom_font(resource_get_handle(res));
    *cur = res;
  }
  return *slot;
}

static void fonts_release(void) {
  if (s_cf) { fonts_unload_custom_font(s_cf); s_cf = NULL; s_cf_res = 0; }
  if (s_sf) { fonts_unload_custom_font(s_sf); s_sf = NULL; s_sf_res = 0; }
}

static GFont clock_face(void) {
  const FontSpec *f = spec();
  if (f->clock_sys[weight()])
    return fonts_get_system_font(f->clock_sys[weight()]);
  return custom(&s_cf, &s_cf_res, f->clock_res[weight()]);
}

static GFont steps_face(void) {
  const FontSpec *f = spec();
  if (f->steps_sys[weight()])
    return fonts_get_system_font(f->steps_sys[weight()]);
  return custom(&s_sf, &s_sf_res, f->steps_res[weight()]);
}

static void draw_clock(GContext *ctx, GRect b) {
  char t[16];
  fmt_time(t, sizeof t);
  GFont f = clock_face();
  GSize sz = text_size(t, f);
  graphics_context_set_text_color(ctx, palette()->time);
  graphics_draw_text(ctx, t, f, GRect(MARGIN, spec()->clock_y, sz.w + 10,
                                      TIME_BOX_H),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

// Comms and power live in the corner the clock never reaches.
static void draw_status(GContext *ctx, GRect b) {
  int x = b.size.w - 22;
  if (g_cfg.show_battery) {
    int y = 8;
    graphics_context_set_stroke_color(ctx, palette()->lines);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, GRect(x, y, 16, 9));
    graphics_context_set_fill_color(ctx, palette()->lines);
    graphics_fill_rect(ctx, GRect(x + 16, y + 3, 2, 3), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, s_batt <= 20 ? COL_BAD : palette()->muted);
    int w = (12 * s_batt) / 100;
    graphics_fill_rect(ctx, GRect(x + 2, y + 2, w, 5), 0, GCornerNone);
  }
  if (g_cfg.show_bt && !s_bt_ok) {
    graphics_context_set_text_color(ctx, COL_BAD);
    draw_left(ctx, "BT", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
              x, 22);
  }
}

// ---------------------------------------------------------------------------
// The grid: health right-aligned against the rule, the day left of it. Three
// rows facing three rows, exactly as the band did — 8,842 | SAT / 6.3 km | 26
// / 68 bpm | 74°. Health stays green, the day column keeps its own colours,
// and the bpm row shows the night's sleep while the shake peek holds.
// ---------------------------------------------------------------------------
static void draw_grid(GContext *ctx, GRect b) {
  char buf[24], v[20];
  GFont f28b = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  GFont f28 = fonts_get_system_font(FONT_KEY_GOTHIC_28);

  // With health off there is no sparkline and no column to face, so the day
  // block drops into the middle of the space instead of hanging off the rule
  // with 70 empty rows beneath it.
  int y0 = health_on() ? GRID_Y0 : GRID_Y0 + 50;
  int r0 = y0, r1 = y0 + GRID_ROW, r2 = y0 + 2 * GRID_ROW;
  int day_x = SEP_X + 9;
  int num_r = SEP_X - 9;

  if (health_on()) {
    graphics_context_set_stroke_color(ctx, palette()->lines);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(SEP_X, r0 + 4), GPoint(SEP_X, GRID_BOT));

    graphics_context_set_text_color(ctx, palette()->health);
    fmt_thousands(v, sizeof v, steps_today(), spec()->comma);
    draw_right(ctx, v, steps_face(), num_r, r0 + spec()->steps_dy, MARGIN);

    char km[12];
    fmt1(km, sizeof km, walked_m_today() / 1000.0);
    snprintf(v, sizeof v, "%s km", km);
    draw_right(ctx, v, f28, num_r, r1, MARGIN);

    int ss = sleep_peek() ? sleep_secs() : 0;
    if (ss > 0) {                    // the shake peek rides bpm's row
      unsigned hh = ((unsigned)ss / 3600u) % 100u, mm = ((unsigned)ss / 60u) % 60u;
      snprintf(v, sizeof v, "%u:%02u slp", hh, mm);
      draw_right(ctx, v, f28, num_r, r2, MARGIN);
    } else {
      int hr = hr_bpm();
      if (hr > 0) {
        snprintf(v, sizeof v, "%d bpm", hr);
        draw_right(ctx, v, f28, num_r, r2, MARGIN);
      }
    }
  }

  // The day column sits where the rule leaves it when health is on, and at
  // the left margin when it isn't — nothing to face across.
  int dx = health_on() ? day_x : MARGIN;
  if (g_cfg.date_format != DATE_OFF) {
    const char *l1 = g_cfg.date_format == DATE_DAYNUM ? WD[s_wday] : MO[s_mon];
    graphics_context_set_text_color(ctx, palette()->muted);
    draw_left(ctx, l1, f28, dx, r0 + 2);
    snprintf(buf, sizeof buf, "%d", s_mday);
    graphics_context_set_text_color(ctx, palette()->date);
    draw_left(ctx, buf, f28b, dx, r1);
  }
  if (temp_fresh()) {
    snprintf(buf, sizeof buf, "%d\xC2\xB0", (int)s_temp);
    graphics_context_set_text_color(ctx, palette()->muted);
    draw_left(ctx, buf, f28, dx, r2);
  }
}

// ---------------------------------------------------------------------------
// The sparkline. A column a minute for the past hour, oldest at the left,
// standing on the bottom edge of the screen. At 61 rows it has room for a
// scale the 10px strip never could: five-minute ticks along the floor, a
// full-height rule at the top of the hour, and a dotted line at the pace
// that counts as walking. The newest minute is white so "now" is findable.
// ---------------------------------------------------------------------------
#define SPARK_CAP 90             // steps/minute that fills the panel
#define SPARK_WALK 60            // the reference pace

static void draw_spark(GContext *ctx, GRect b) {
  int gx0 = MARGIN, gw = b.size.w - 2 * MARGIN;
  int base = SPARK_BOT, gh = SPARK_BOT - SPARK_TOP;

  int live = minute_steps_live();
  s_minsteps[s_min] = live > 255 ? 255 : live;

  // scale first, so the bars stand on top of it
  graphics_context_set_fill_color(ctx, palette()->lines);
  for (int i = 0; i < 60; i++) {
    int wall = (s_min + 1 + i) % 60;
    if (wall % 5) continue;
    int x = gx0 + gw * i / 60;
    graphics_fill_rect(ctx, GRect(x, wall == 0 ? SPARK_TOP : base - 4, 1,
                                  wall == 0 ? gh + 1 : 5), 0, GCornerNone);
  }
  int wy = base - SPARK_WALK * gh / SPARK_CAP;
  for (int x = gx0; x < gx0 + gw; x += 3)
    graphics_fill_rect(ctx, GRect(x, wy, 1, 1), 0, GCornerNone);

  for (int i = 0; i < 60; i++) {
    int x = gx0 + gw * i / 60, xe = gx0 + gw * (i + 1) / 60;
    int wall = (s_min + 1 + i) % 60;
    int st = s_minsteps[wall];
    if (st > SPARK_CAP) st = SPARK_CAP;
    int h = 1 + st * (gh - 1) / SPARK_CAP;
    graphics_context_set_fill_color(ctx, wall == s_min ? palette()->time
                                                      : palette()->spark);
    graphics_fill_rect(ctx, GRect(x, base - h + 1, xe - x, h), 0, GCornerNone);
  }
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, palette()->bg);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  draw_clock(ctx, b);
  draw_status(ctx, b);

  graphics_context_set_fill_color(ctx, palette()->lines);
  graphics_fill_rect(ctx, GRect(MARGIN, RULE1_Y, b.size.w - 2 * MARGIN, 1),
                     0, GCornerNone);
  draw_grid(ctx, b);

  if (health_on()) {
    graphics_context_set_fill_color(ctx, palette()->lines);
    graphics_fill_rect(ctx, GRect(MARGIN, RULE2_Y, b.size.w - 2 * MARGIN, 1),
                       0, GCornerNone);
    draw_spark(ctx, b);
  }
}

static void set_clock(struct tm *t) {
  s_hour = t->tm_hour;
  s_min = t->tm_min;
  s_mday = t->tm_mday;
  s_mon = t->tm_mon;
  s_wday = t->tm_wday;
}

static void tick_handler(struct tm *t, TimeUnits changed) {
  set_clock(t);
  minute_track(t);
  face_poke();
}

static void tap_handler(AccelAxisType axis, int32_t dir) {
  if (!g_cfg.tap_info || !health_on()) return;
  s_sleep_peek_at = time(NULL);
  if (s_peek_timer) app_timer_cancel(s_peek_timer);
  s_peek_timer = app_timer_register(SLEEP_PEEK_MS + 300, peek_expire, NULL);
  face_poke();
}

static void bt_handler(bool connected) {
  if (!connected && s_bt_ok && g_cfg.bt_vibe && !quiet_time_is_active())
    vibes_short_pulse();
  s_bt_ok = connected;
  face_poke();
}

static void batt_handler(BatteryChargeState st) {
  s_batt = st.charge_percent;
  face_poke();
}

static void win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, draw);
  layer_add_child(root, s_layer);
}

static void win_unload(Window *w) {
  layer_destroy(s_layer);
  s_layer = NULL;
}

void face_init(void) {
  time_t now = time(NULL);
  set_clock(localtime(&now));
  s_bt_ok = connection_service_peek_pebble_app_connection();
  s_batt = battery_state_service_peek().charge_percent;
  weather_load();

  s_win = window_create();
  window_set_background_color(s_win, GColorBlack);
  window_set_window_handlers(s_win, (WindowHandlers){
    .load = win_load, .unload = win_unload });
  window_stack_push(s_win, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(tap_handler);
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = bt_handler });
  battery_state_service_subscribe(batt_handler);
  health_init();
}

void face_deinit(void) {
  if (s_peek_timer) { app_timer_cancel(s_peek_timer); s_peek_timer = NULL; }
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_deinit();
  fonts_release();
  window_destroy(s_win);
}
