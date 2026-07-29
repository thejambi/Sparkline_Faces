#include "ui.h"
#include "settings.h"
#include "scene.h"

// TeaScene: Solfarer's info band, unchanged, over a tea room.
//
// The top 56 rows are the "main watchface info" — clock, the health corner
// grid, date and temperature, and the past-hour activity sparkline. Below
// that the screen is entirely art: a gongfu tea tray by a round window,
// recoloured four times a day.

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

// Health mode's shake: no card — the stats line's bpm row swaps to hours
// slept for a few seconds (sleep is the only figure the face doesn't already
// show).
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
// Health — same machinery as Solfarer, including the one deliberate quirk:
// the minute history is fetched once at boot and then exactly once more, at
// the first quarter-hour mark, because that is when the watch has filled in
// the past hour it was not running for. After that the sparkline is kept
// live from the step delta and never refetches.
// ---------------------------------------------------------------------------
static void fmt_thousands(char *buf, size_t cap, int v) {
  if (v >= 1000) snprintf(buf, cap, "%d,%03d", v / 1000, v % 1000);
  else snprintf(buf, cap, "%d", v);
}

// The Pebble app already carries a units preference, so Automatic follows it
// and the explicit choices are there for when it is unset or simply wrong.
static bool use_miles(void) {
  if (g_cfg.dist_unit == DIST_MI) return true;
  if (g_cfg.dist_unit == DIST_KM) return false;
#if defined(PBL_HEALTH)
  return health_service_get_measurement_system_for_display(
             HealthMetricWalkedDistanceMeters) == MeasurementSystemImperial;
#else
  return false;
#endif
}

// Health always reports metres, whichever way this comes out.
static void fmt_distance(char *buf, size_t cap, int metres) {
  char n[12];
  if (use_miles()) {
    fmt1(n, sizeof n, metres / 1609.344);
    snprintf(buf, cap, "%s mi", n);
  } else {
    fmt1(n, sizeof n, metres / 1000.0);
    snprintf(buf, cap, "%s km", n);
  }
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
// The info band. Numbers here are Solfarer's tall-rect layout verbatim —
// they were tuned against these fonts on this screen, so they stay put.
// ---------------------------------------------------------------------------
static void draw_band(GContext *ctx, GRect b) {
  char buf[96], t1[20];
  fmt_time(t1, sizeof t1);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, t1,
                     fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS),
                     GRect(2, -2, b.size.w - 46, 38),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // The corner grid: three health rows facing three day rows across a
  // hairline — 8,842 | SAT / 6.3 km | 26 / 68 bpm | 74°. Health stays green,
  // the day column keeps its own colours. The bpm row shows the night's
  // sleep while the shake peek holds.
  GFont f14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  GFont f14b = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  int sep_x = b.size.w - 37;
  int day_x = sep_x + 5, day_w = b.size.w - day_x - 2;
  int r0 = 0, r1 = 14, r2 = 28;

  if (g_cfg.date_format != DATE_OFF) {
    const char *l1 = g_cfg.date_format == DATE_DAYNUM ? WD[s_wday] : MO[s_mon];
    graphics_context_set_text_color(ctx, COL_DIM);
    graphics_draw_text(ctx, l1, f14b, GRect(day_x, r0, day_w, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    snprintf(buf, sizeof buf, "%d", s_mday);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, buf, f14, GRect(day_x, r1, day_w, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
  if (temp_fresh()) {
    snprintf(buf, sizeof buf, "%d\xC2\xB0", (int)s_temp);
    graphics_context_set_text_color(ctx, COL_DIM);
    graphics_draw_text(ctx, buf, f14, GRect(day_x, r2, day_w, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

  if (health_on()) {
    graphics_context_set_stroke_color(ctx, COL_FAINT);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(sep_x, 3), GPoint(sep_x, 44));
    GSize tmsz = graphics_text_layout_get_content_size(t1,
        fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS),
        GRect(0, 0, b.size.w, 40), GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    int num_x = 2 + tmsz.w + 4;
    int num_w = sep_x - 4 - num_x;
    if (num_w < 12) num_w = 12;
    int num_r = sep_x - 4;
    graphics_context_set_text_color(ctx, COL_GOOD);
    char v[16];
    // Right-alignment is done by hand: measure, then draw left-aligned at
    // (right edge - width) in a roomy box. The renderer's own right-aligned
    // layout misplaced interior glyphs when the box ran near its width —
    // digits drawn fused on top of each other.
    #define GRID_ROW(str, font, row) do { \
      GSize _s = graphics_text_layout_get_content_size((str), (font), \
          GRect(0, 0, 200, 18), GTextOverflowModeTrailingEllipsis, \
          GTextAlignmentLeft); \
      int _x = num_r - _s.w; \
      if (_x < num_x) _x = num_x; \
      graphics_draw_text(ctx, (str), (font), GRect(_x, (row), _s.w + 6, 18), \
          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL); \
    } while (0)
    // Steps draw in GOTHIC_18_BOLD, not 14 — partly for presence, and partly
    // because the 14-bold digit '4' has a broken advance that fuses it into
    // its left neighbour ("8,842" mashed). 18-bold's digits are clean.
    GFont f18b = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    fmt_thousands(v, sizeof v, steps_today());
    GSize ssz = graphics_text_layout_get_content_size(v, f18b,
        GRect(0, 0, 200, 22), GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    if (ssz.w > num_w - 4)           // tight gap: the comma goes first
      snprintf(v, sizeof v, "%d", steps_today());
    GRID_ROW(v, f18b, r0 - 3);
    fmt_distance(v, sizeof v, walked_m_today());
    GRID_ROW(v, f14, r1);
    int ss = sleep_peek() ? sleep_secs() : 0;
    if (ss > 0) {                    // the shake peek rides bpm's row
      unsigned hh = ((unsigned)ss / 3600u) % 100u, mm = ((unsigned)ss / 60u) % 60u;
      snprintf(v, sizeof v, "%u:%02u slp", hh, mm);
      GRID_ROW(v, f14, r2);
    } else {
      int hr = hr_bpm();
      if (hr > 0) {
        snprintf(v, sizeof v, "%d bpm", hr);
        GRID_ROW(v, f14, r2);
      }
    }
    #undef GRID_ROW

    // --- the sparkline: the last hour, a column a minute, oldest on the left
    int gh = 10, gy0 = TOP_H - gh;
    int gx0 = 2, gw = b.size.w - 2 * gx0;
    int live = minute_steps_live();
    s_minsteps[s_min] = live > 255 ? 255 : live;
    graphics_context_set_fill_color(ctx, COL_FAINT);
    graphics_fill_rect(ctx, GRect(gx0, gy0, gw, 1), 0, GCornerNone);
    for (int i = 0; i < 60; i++) {
      int wall = (s_min + 1 + i) % 60;
      if (wall % 5) continue;
      graphics_fill_rect(ctx, GRect(gx0 + gw * i / 60, gy0, 1, wall == 0 ? gh : 3),
                         0, GCornerNone);
    }
    graphics_context_set_fill_color(ctx, COL_GOLD);
    const int cap = 90;
    for (int i = 0; i < 60; i++) {
      int x = gx0 + gw * i / 60, xe = gx0 + gw * (i + 1) / 60;
      int st = s_minsteps[(s_min + 1 + i) % 60];
      if (st > cap) st = cap;
      int hcol = 1 + st * (gh - 1) / cap;
      graphics_fill_rect(ctx, GRect(x, gy0 + gh - hcol, xe - x, hcol), 0, GCornerNone);
    }
  }
}

// Comms and power ride the tray's front edge, the one band of the art dark
// enough to take small pale glyphs without a plate behind them.
static void draw_status(GContext *ctx, GRect b) {
  int y = TOP_H + 162;
  if (g_cfg.show_bt && !s_bt_ok) {
    graphics_context_set_text_color(ctx, COL_BAD);
    graphics_draw_text(ctx, "BT", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(4, y - 3, 24, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
  if (g_cfg.show_battery) {
    int bx = b.size.w - 21, by = y + 1;
    graphics_context_set_stroke_color(ctx, COL_DIM);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, GRect(bx, by, 14, 7));
    graphics_context_set_fill_color(ctx, COL_DIM);
    graphics_fill_rect(ctx, GRect(bx + 14, by + 2, 2, 3), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, s_batt <= 20 ? COL_BAD : COL_DIM);
    graphics_fill_rect(ctx, GRect(bx + 2, by + 2, s_batt / 10, 3), 0, GCornerNone);
  }
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, b.size.w, TOP_H), 0, GCornerNone);

  int v = g_cfg.scene_time == SCENE_AUTO ? scene_variant_for_hour(s_hour)
                                         : g_cfg.scene_time - 1;
  scene_draw(ctx, TOP_H, v);

  draw_band(ctx, b);
  draw_status(ctx, b);
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
  scene_init();

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
  window_destroy(s_win);
}
