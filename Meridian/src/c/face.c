#include "ui.h"
#include "settings.h"
#include "health.h"

// Meridian. See ui.h for the layout and why the clock is stacked.
//
// Everything here is positioned from a baseline, never from a box top. Pebble
// draws text from the box, so each font carries the distance from its box top
// down to its baseline — measured once on the device rather than guessed,
// because it differs per face and per size and eyeballing it is how rows end
// up three pixels out of alignment.

static Window *s_win;
static Layer *s_layer;
static int s_hour, s_min, s_mday, s_mon, s_wday;
static bool s_bt_ok = true;
static uint8_t s_batt = 100;

static const char *WD[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
static const char *MO[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                              "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };

// ---------------------------------------------------------------------------
// Type
// ---------------------------------------------------------------------------
typedef enum { F_CLOCK, F_CLOCK_B, F_VAL, F_VAL_L, F_UNIT, F_CAPS, F_CAPS_S,
               F_COUNT } FontId;

static GFont s_font[F_COUNT];
static int s_ascent[F_COUNT];

static const uint32_t FONT_RES[F_COUNT] = {
  RESOURCE_ID_FONT_CLOCK_76, RESOURCE_ID_FONT_CLOCK_B_76,
  RESOURCE_ID_FONT_VALUE_22, RESOURCE_ID_FONT_VALUE_L_22,
  RESOURCE_ID_FONT_UNIT_14, RESOURCE_ID_FONT_CAPS_15,
  RESOURCE_ID_FONT_CAPS_11,
};
// A representative glyph per font: its box height, minus nothing, is the
// distance from box top to baseline for a face with no descenders — which is
// true of every charset here (digits, caps, and h/m).
static const char *FONT_PROBE[F_COUNT] = { "8", "8", "8", "8", "h", "8", "B" };

static GSize tsz(const char *s, FontId f) {
  return graphics_text_layout_get_content_size(s, s_font[f],
      GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
}

static void clock_resolve(void);

static void fonts_load(void) {
  for (int i = 0; i < F_COUNT; i++) {
    s_font[i] = fonts_load_custom_font(resource_get_handle(FONT_RES[i]));
    s_ascent[i] = tsz(FONT_PROBE[i], (FontId)i).h;
  }
  clock_resolve();
}

// The clock is the one place a system face can appear, so it is resolved
// separately from the bundled set. `slot` is the width of the widest digit:
// Montserrat is proportional, and drawing into a fixed slot is what stops the
// minutes shuffling sideways when the digits change. LECO is already tabular
// and simply agrees.
static GFont s_clock;
static int s_clock_asc, s_clock_slot;

static void clock_resolve(void) {
  if (g_cfg.clock_font == CF_LECO) {
    s_clock = fonts_get_system_font(g_cfg.bold_clock
        ? FONT_KEY_LECO_60_BOLD_NUMBERS_AM_PM : FONT_KEY_LECO_60_NUMBERS_AM_PM);
  } else {
    s_clock = s_font[g_cfg.bold_clock ? F_CLOCK_B : F_CLOCK];
  }
  GSize probe = graphics_text_layout_get_content_size("8", s_clock,
      GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
  s_clock_asc = probe.h;
  s_clock_slot = 0;
  for (char c = '0'; c <= '9'; c++) {
    char one[2] = { c, 0 };
    int w = graphics_text_layout_get_content_size(one, s_clock,
        GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft).w;
    if (w > s_clock_slot) s_clock_slot = w;
  }
  s_clock_slot += 2;                 // a hair of air between the slots
}

void face_fonts_changed(void) { clock_resolve(); }

// Right-aligned within the block, one digit per slot. A single-digit hour
// therefore sits above the minutes' second digit rather than above the first.
static void draw_clock_num(GContext *ctx, const char *s, int baseline) {
  int n = strlen(s);
  int right = MARGIN_L + 2 * s_clock_slot;
  int x = right - n * s_clock_slot;
  for (int i = 0; i < n; i++) {
    char one[2] = { s[i], 0 };
    GSize sz = graphics_text_layout_get_content_size(one, s_clock,
        GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    graphics_draw_text(ctx, one, s_clock,
        GRect(x + (s_clock_slot - sz.w) / 2, baseline - s_clock_asc,
              sz.w + 12, sz.h + 8),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    x += s_clock_slot;
  }
}

static void fonts_unload(void) {
  for (int i = 0; i < F_COUNT; i++)
    if (s_font[i]) { fonts_unload_custom_font(s_font[i]); s_font[i] = NULL; }
}

// Draw with the ink sitting on `baseline`, returning the width used.
static int draw_base(GContext *ctx, const char *s, FontId f, int x,
                     int baseline) {
  GSize sz = tsz(s, f);
  graphics_draw_text(ctx, s, s_font[f],
                     GRect(x, baseline - s_ascent[f], sz.w + 12, sz.h + 8),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  return sz.w;
}

static int draw_right(GContext *ctx, const char *s, FontId f, int right,
                      int baseline) {
  int w = tsz(s, f).w;
  draw_base(ctx, s, f, right - w, baseline);
  return w;
}

// Only the caps are tracked. Tracked figures look broken.
static int tracked_w(const char *s, FontId f) {
  int w = 0;
  for (const char *c = s; *c; c++) {
    char one[2] = { *c, 0 };
    w += tsz(one, f).w + TRACK_CAPS;
  }
  return w > 0 ? w - TRACK_CAPS : 0;
}

static void draw_tracked(GContext *ctx, const char *s, FontId f, int x,
                         int baseline) {
  for (const char *c = s; *c; c++) {
    char one[2] = { *c, 0 };
    x += draw_base(ctx, one, f, x, baseline) + TRACK_CAPS;
  }
}

// ---------------------------------------------------------------------------
// Weather
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

void face_poke(void) { if (s_layer) layer_mark_dirty(s_layer); }

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------
static void fmt_thousands(char *buf, size_t cap, int v) {
  if (v >= 1000) snprintf(buf, cap, "%d,%03d", v / 1000, v % 1000);
  else snprintf(buf, cap, "%d", v);
}

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

// The degree mark is drawn rather than set: it keeps the glyph off the
// bundled charset, and it lets the ring hang past the right margin so the
// numerals themselves stay optically aligned with the row below.
static void draw_degree(GContext *ctx, int x, int top, GColor col) {
  graphics_context_set_stroke_color(ctx, col);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(x, top, 4, 4));
}

static void draw_sky(GContext *ctx) {
  const Palette *p = palette();
  char buf[24];

  // date, tracked caps, flush left
  if (g_cfg.date_format != DATE_OFF) {
    const char *l = g_cfg.date_format == DATE_DAYNUM ? WD[s_wday] : MO[s_mon];
    snprintf(buf, sizeof buf, "%s %d", l, s_mday);
    graphics_context_set_text_color(ctx, p->muted);
    draw_tracked(ctx, buf, F_CAPS, MARGIN_L, BASE_DATE);
  }

  // temperature, numerals optically right-aligned, the ring hanging past
  if (temp_fresh()) {
    snprintf(buf, sizeof buf, "%d", (int)s_temp);
    graphics_context_set_text_color(ctx, p->muted);
    int w = tracked_w(buf, F_CAPS);
    draw_tracked(ctx, buf, F_CAPS, MARGIN_R - w, BASE_DATE);
    draw_degree(ctx, MARGIN_R + 3, BASE_DATE - 11, p->muted);
  }

  // the clock: hours over minutes, right-aligned to each other in place
  int h = s_hour;
  if (!clock_is_24h_style()) { h %= 12; if (h == 0) h = 12; }
  graphics_context_set_text_color(ctx, p->ink);
  if (clock_is_24h_style() || g_cfg.leading_zero)
    snprintf(buf, sizeof buf, "%02d", h);
  else
    snprintf(buf, sizeof buf, "%d", h);
  draw_clock_num(ctx, buf, BASE_HOUR);
  snprintf(buf, sizeof buf, "%02d", s_min);
  draw_clock_num(ctx, buf, BASE_MIN);

  // the value slot: steps, or last night's sleep until you are up
  if (hl_sleeping()) {
    int ss = hl_sleep_secs();
    unsigned hh = ((unsigned)ss / 3600u) % 100u;
    unsigned mm = ((unsigned)ss / 60u) % 60u;
    char a[6], b[6];
    snprintf(a, sizeof a, "%u", hh);
    snprintf(b, sizeof b, "%u", mm);
    int wa = tsz(a, F_VAL).w, wb = tsz(b, F_VAL).w;
    int wh = tsz("h", F_UNIT).w, wm = tsz("m", F_UNIT).w;
    int total = wa + 2 + wh + 7 + wb + 2 + wm;
    int x = MARGIN_R - total;
    graphics_context_set_text_color(ctx, p->muted);
    x += draw_base(ctx, a, F_VAL, x, BASE_HOUR) + 2;
    graphics_context_set_text_color(ctx, p->scale);
    x += draw_base(ctx, "h", F_UNIT, x, BASE_HOUR) + 7;
    graphics_context_set_text_color(ctx, p->muted);
    x += draw_base(ctx, b, F_VAL, x, BASE_HOUR) + 2;
    graphics_context_set_text_color(ctx, p->scale);
    draw_base(ctx, "m", F_UNIT, x, BASE_HOUR);
  } else {
    fmt_thousands(buf, sizeof buf, hl_steps());
    graphics_context_set_text_color(ctx, p->accent);
    draw_right(ctx, buf, F_VAL, MARGIN_R, BASE_HOUR);
  }

  // pulse, sharing the minutes' baseline
  int bpm = g_cfg.show_bpm ? hl_bpm() : 0;
  if (bpm > 0) {
    snprintf(buf, sizeof buf, "%d", bpm);
    graphics_context_set_text_color(ctx, p->muted);
    int vw = draw_right(ctx, buf, F_VAL_L, MARGIN_R, BASE_MIN);
    graphics_context_set_text_color(ctx, p->scale);
    int lw = tracked_w("BPM", F_CAPS_S);
    draw_tracked(ctx, "BPM", F_CAPS_S, MARGIN_R - vw - 8 - lw, BASE_MIN);
  } else if (!hl_sleeping()) {
    // no sensor: the distance takes the slot rather than leaving a hole
    char km[12];
    fmt1(km, sizeof km, use_miles() ? hl_walked_m() / 1609.344
                                    : hl_walked_m() / 1000.0);
    graphics_context_set_text_color(ctx, p->muted);
    int vw = draw_right(ctx, km, F_VAL_L, MARGIN_R, BASE_MIN);
    graphics_context_set_text_color(ctx, p->scale);
    const char *u = use_miles() ? "MI" : "KM";
    int lw = tracked_w(u, F_CAPS_S);
    draw_tracked(ctx, u, F_CAPS_S, MARGIN_R - vw - 8 - lw, BASE_MIN);
  }
}

static void draw_ground(GContext *ctx) {
  const Palette *p = palette();
  const Terrain *t = hl_terrain();
  bool sleeping = hl_sleeping();

  GColor bar = sleeping ? p->muted : p->accent;
  GColor now = sleeping ? p->scale : p->ink;

  graphics_context_set_fill_color(ctx, sleeping ? p->muted : p->horizon);
  graphics_fill_rect(ctx, GRect(0, HORIZON_Y, SCREEN_W, HORIZON_H), 0,
                     GCornerNone);
  graphics_context_set_fill_color(ctx, p->ground);
  graphics_fill_rect(ctx, GRect(0, GROUND_Y, SCREEN_W, SCREEN_H - GROUND_Y), 0,
                     GCornerNone);

  // A quarter-hour architecture behind the terrain. Where a column covers it
  // the rule vanishes; where it does not, it reads as structure.
  graphics_context_set_fill_color(ctx, p->scale);
  for (int i = 0; i < COLS; i++) {
    int wall = (s_min + 1 + i) % 60;
    if (t->live && wall % 15) continue;
    if (!t->live && i % 15) continue;
    graphics_fill_rect(ctx, GRect(PLOT_X + COL_W * i, GROUND_Y, 1,
                                  PLOT_BOT - GROUND_Y + 1), 0, GCornerNone);
  }
  graphics_fill_rect(ctx, GRect(PLOT_X, PLOT_BOT, COL_W * COLS, 1), 0,
                     GCornerNone);

  // the pace that counts as walking — only meaningful on the live hour
  if (t->live) {
    int wy = PLOT_BOT - WALK_RATE * BAR_MAX / STEP_CAP;
    for (int x = PLOT_X; x < PLOT_X + COL_W * COLS; x += 4)
      graphics_fill_rect(ctx, GRect(x, wy, 1, 1), 0, GCornerNone);
  }

  for (int i = 0; i < COLS; i++) {
    int v = t->col[i];
    if (v > t->cap) v = t->cap;
    int h = (v * BAR_MAX + t->cap / 2) / t->cap;
    bool is_now = t->live && i == COLS - 1;
    if (is_now && h < 4) h = 4;
    if (h <= 0) continue;
    graphics_context_set_fill_color(ctx, is_now ? now : bar);
    graphics_fill_rect(ctx, GRect(PLOT_X + COL_W * i, PLOT_BOT - h + 1,
                                  COL_W, h), 0, GCornerNone);
  }
}

// Power and comms ride the very top edge — a 2px bar and, if the phone is
// gone, a dot in the corner. Neither gets a band of its own.
static void draw_status(GContext *ctx) {
  const Palette *p = palette();
  if (g_cfg.show_battery) {
    int w = (SCREEN_W * s_batt) / 100;
    graphics_context_set_fill_color(ctx, s_batt <= 20 ? GColorRed : p->scale);
    graphics_fill_rect(ctx, GRect(0, 0, w, 2), 0, GCornerNone);
  }
  if (!s_bt_ok) {
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, GRect(SCREEN_W - 5, 0, 5, 5), 0, GCornerNone);
  }
}

static void draw(Layer *layer, GContext *ctx) {
  const Palette *p = palette();
  graphics_context_set_fill_color(ctx, p->sky);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  draw_sky(ctx);
  draw_ground(ctx);
  draw_status(ctx);
}

// ---------------------------------------------------------------------------
static void set_clock(struct tm *t) {
  s_hour = t->tm_hour;
  s_min = t->tm_min;
  s_mday = t->tm_mday;
  s_mon = t->tm_mon;
  s_wday = t->tm_wday;
}

static void tick_handler(struct tm *t, TimeUnits changed) {
  set_clock(t);
  health_minute(t);
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
  fonts_load();

  s_win = window_create();
  window_set_background_color(s_win, GColorBlack);
  window_set_window_handlers(s_win, (WindowHandlers){
    .load = win_load, .unload = win_unload });
  window_stack_push(s_win, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = bt_handler });
  battery_state_service_subscribe(batt_handler);
  health_init();
}

void face_deinit(void) {
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_deinit();
  fonts_unload();
  window_destroy(s_win);
}
