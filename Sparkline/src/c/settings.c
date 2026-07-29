#include "settings.h"
#include "ui.h"

Settings g_cfg;
static void (*s_cb)(void);
static Palette s_pal;

#define KEY_SETTINGS 20

static void defaults(void) {
  memset(&g_cfg, 0, sizeof g_cfg);
  g_cfg.version = SETTINGS_VERSION;
  g_cfg.show_health = true;
  g_cfg.date_format = DATE_DAYNUM;
  g_cfg.dist_unit = DIST_AUTO;
  g_cfg.show_battery = true;
  g_cfg.show_bt = true;
  g_cfg.show_sleep = true;
  g_cfg.wake_threshold = 500;
  g_cfg.weather_on = true;
  g_cfg.clock_font = CF_LECO;
  g_cfg.bold_font = true;
  g_cfg.bold_steps = true;
  g_cfg.theme = TH_CLASSIC;
  g_cfg.c_bg = 0x000000;
  g_cfg.c_time = 0xFFFFFF;
  g_cfg.c_health = 0x00FF00;
  g_cfg.c_date = 0xFFFFFF;
  g_cfg.c_muted = 0xAAAAAA;
  g_cfg.c_lines = 0x555555;
  g_cfg.c_spark = 0xFFAA00;
}

// ---------------------------------------------------------------------------
// Themes. Every colour here is already on the Pebble 64 — each channel is
// 0x00/0x55/0xAA/0xFF — so nothing gets quantised out from under the design.
// ---------------------------------------------------------------------------
typedef struct { uint32_t bg, time, health, date, muted, lines, spark; } Theme;

static const Theme THEMES[] = {
  // classic: the face as Solfarer left it
  { 0x000000, 0xFFFFFF, 0x00FF00, 0xFFFFFF, 0xAAAAAA, 0x555555, 0xFFAA00 },
  // mono: one ink, so weight and size carry the hierarchy instead of hue
  { 0x000000, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xAAAAAA, 0x555555, 0xFFFFFF },
  // amber: a phosphor terminal
  { 0x000000, 0xFFAA00, 0xFFAA00, 0xFFFFFF, 0xAA5500, 0x550000, 0xFF5500 },
  // ice
  { 0x000000, 0xFFFFFF, 0x00FFFF, 0xFFFFFF, 0x55AAAA, 0x005555, 0x00AAFF },
  // paper: the one light theme, so nothing may assume a black background
  { 0xFFFFFF, 0x000000, 0x005500, 0x000000, 0x555555, 0xAAAAAA, 0xFF5500 },
};

static void resolve(void) {
  Theme custom = { g_cfg.c_bg, g_cfg.c_time, g_cfg.c_health, g_cfg.c_date,
                   g_cfg.c_muted, g_cfg.c_lines, g_cfg.c_spark };
  const Theme *t = (g_cfg.theme == TH_CUSTOM) ? &custom
                 : &THEMES[g_cfg.theme < ARRAY_LENGTH(THEMES) ? g_cfg.theme : 0];
  s_pal.bg     = GColorFromHEX(t->bg);
  s_pal.time   = GColorFromHEX(t->time);
  s_pal.health = GColorFromHEX(t->health);
  s_pal.date   = GColorFromHEX(t->date);
  s_pal.muted  = GColorFromHEX(t->muted);
  s_pal.lines  = GColorFromHEX(t->lines);
  s_pal.spark  = GColorFromHEX(t->spark);
}

const Palette *palette(void) { return &s_pal; }

// Clay sends selects as strings ("0","1",..) and toggles as small ints —
// accept either so the config page and the C side can't drift apart.
static int tup_int(DictionaryIterator *it, uint32_t key, int fallback) {
  Tuple *t = dict_find(it, key);
  if (!t) return fallback;
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  return (int)t->value->int32;
}

// Clay's colour picker sends a packed 0xRRGGBB integer.
static uint32_t tup_col(DictionaryIterator *it, uint32_t key, uint32_t fallback) {
  Tuple *t = dict_find(it, key);
  if (!t) return fallback;
  if (t->type == TUPLE_CSTRING)
    return (uint32_t)strtol(t->value->cstring, NULL, 0) & 0xFFFFFF;
  return (uint32_t)t->value->int32 & 0xFFFFFF;
}

static void inbox(DictionaryIterator *it, void *ctx) {
  g_cfg.show_health  = tup_int(it, MESSAGE_KEY_ShowHealth, g_cfg.show_health);
  g_cfg.date_format  = tup_int(it, MESSAGE_KEY_DateFormat, g_cfg.date_format);
  g_cfg.dist_unit    = tup_int(it, MESSAGE_KEY_DistUnit, g_cfg.dist_unit);
  if (g_cfg.dist_unit > DIST_MI) g_cfg.dist_unit = DIST_AUTO;
  g_cfg.leading_zero = tup_int(it, MESSAGE_KEY_LeadingZero, g_cfg.leading_zero);
  g_cfg.show_battery = tup_int(it, MESSAGE_KEY_ShowBattery, g_cfg.show_battery);
  g_cfg.show_bt      = tup_int(it, MESSAGE_KEY_ShowBT, g_cfg.show_bt);
  g_cfg.bt_vibe      = tup_int(it, MESSAGE_KEY_BTVibe, g_cfg.bt_vibe);
  g_cfg.show_sleep   = tup_int(it, MESSAGE_KEY_ShowSleep, g_cfg.show_sleep);
  g_cfg.weather_on   = tup_int(it, MESSAGE_KEY_WeatherOn, g_cfg.weather_on);
  g_cfg.clock_font   = tup_int(it, MESSAGE_KEY_ClockFont, g_cfg.clock_font);
  if (g_cfg.clock_font >= CF_COUNT) g_cfg.clock_font = CF_LECO;
  g_cfg.bold_font    = tup_int(it, MESSAGE_KEY_BoldFont, g_cfg.bold_font);
  g_cfg.bold_steps   = tup_int(it, MESSAGE_KEY_BoldSteps, g_cfg.bold_steps);
  int wake = tup_int(it, MESSAGE_KEY_WakeThreshold, g_cfg.wake_threshold);
  if (wake < 0) wake = 0;
  if (wake > 20000) wake = 20000;
  g_cfg.wake_threshold = (uint16_t)wake;
  g_cfg.theme        = tup_int(it, MESSAGE_KEY_Theme, g_cfg.theme);
  if (g_cfg.theme > TH_CUSTOM) g_cfg.theme = TH_CLASSIC;
  g_cfg.c_bg     = tup_col(it, MESSAGE_KEY_ColBg, g_cfg.c_bg);
  g_cfg.c_time   = tup_col(it, MESSAGE_KEY_ColTime, g_cfg.c_time);
  g_cfg.c_health = tup_col(it, MESSAGE_KEY_ColHealth, g_cfg.c_health);
  g_cfg.c_date   = tup_col(it, MESSAGE_KEY_ColDate, g_cfg.c_date);
  g_cfg.c_muted  = tup_col(it, MESSAGE_KEY_ColMuted, g_cfg.c_muted);
  g_cfg.c_lines  = tup_col(it, MESSAGE_KEY_ColLines, g_cfg.c_lines);
  g_cfg.c_spark  = tup_col(it, MESSAGE_KEY_ColSpark, g_cfg.c_spark);
  Tuple *wt = dict_find(it, MESSAGE_KEY_WeatherTemp);   // phone-side fetch
  if (wt) face_set_temp((int)wt->value->int32);
  g_cfg.version = SETTINGS_VERSION;
  persist_write_data(KEY_SETTINGS, &g_cfg, sizeof g_cfg);
  resolve();
  if (s_cb) s_cb();
}

void settings_init(void (*cb)(void)) {
  s_cb = cb;
  defaults();
  // Older (shorter) blobs read over the defaults and stop where they end, so
  // a save written before fonts and themes existed keeps its choices and
  // picks the new options up at their defaults.
  int n = persist_exists(KEY_SETTINGS) ? persist_get_size(KEY_SETTINGS) : 0;
  if (n > 0 && n <= (int)sizeof g_cfg) {
    Settings tmp = g_cfg;
    persist_read_data(KEY_SETTINGS, &tmp, n);
    if (tmp.version == SETTINGS_VERSION) g_cfg = tmp;
  }
  resolve();
  app_message_register_inbox_received(inbox);
  app_message_open(1024, 64);
}
