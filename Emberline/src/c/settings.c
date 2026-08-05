#include "settings.h"
#include "ui.h"

Settings g_cfg;
static void (*s_cb)(void);
static Palette s_pal;

#define KEY_SETTINGS 20

static void defaults(void) {
  memset(&g_cfg, 0, sizeof g_cfg);
  g_cfg.version = SETTINGS_VERSION;
  g_cfg.theme = TH_DUSK;
  g_cfg.date_format = DATE_DAYNUM;
  g_cfg.dist_unit = DIST_AUTO;
  g_cfg.show_bpm = true;
  g_cfg.show_battery = true;
  g_cfg.show_sleep = true;
  g_cfg.sleep_terrain = true;
  g_cfg.weather_on = true;
  g_cfg.wake_threshold = 350;
  g_cfg.clock_font = CF_GRID;
  g_cfg.bold_clock = true;
  g_cfg.layout = LAY_STACK;
  g_cfg.auto_hide = true;
  g_cfg.grow_clock = true;
  g_cfg.c_sky = 0x000055;
  g_cfg.c_ground = 0x000000;
  g_cfg.c_horizon = 0xFFAA00;
  g_cfg.c_ink = 0xFFFFFF;
  g_cfg.c_accent = 0xFFAA00;
  g_cfg.c_muted = 0xAAAAFF;
  g_cfg.c_scale = 0x5555AA;
  g_cfg.c_terrain = g_cfg.c_now = g_cfg.c_sleep = COL_INHERIT;
  g_cfg.c_unlit = COL_INHERIT;
  g_cfg.text_font = TF_MONT;
  g_cfg.show_sep = true;
  g_cfg.c_label = g_cfg.c_sep = g_cfg.c_info_bg = COL_INHERIT;
}

// ---------------------------------------------------------------------------
// Themes. Every value is already on the Pebble 64 — each channel is one of
// 0x00/0x55/0xAA/0xFF — so nothing is quantized out from under the design.
//
// Most are built the same way: a lit sky over ground in shadow, and exactly
// one warm or bright thing, spent only on the horizon, the step count and the
// terrain. Those three are the same idea seen three ways.
//
// Phosphor is the exception. It spends two hues on an unlit screen rather than
// one on a lit one: the time in orange, everything the body reports in green,
// and no sky at all.
//
// Its unlit tone is its own sky, so DSEG's ghost segments are off here. 0x550000
// against black looked right in the emulator and vanished on the wrist, and the
// arithmetic says why: red carries 21% of perceived luminance, so one step of
// red above black is 7% apart where one step of green is 24%. It was the
// dimmest pairing the Pebble 64 can make. Custom can still set one. The horizon follows the clock there instead of the
// terrain, which is the opposite of the others — it reads as the line the
// numerals stand on rather than the edge of the plot. Dusk is the default
// because it is the only theme that depicts what the face is about.
// ---------------------------------------------------------------------------
typedef struct {
  uint32_t sky, ground, horizon, ink, accent, muted, scale, unlit, sep;
  uint32_t info_bg;   // behind the panels; equal to the sky leaves them untinted
} Theme;

static const Theme THEMES[TH_COUNT] = {
  // dusk — navy sky, black land, one amber line where they meet
  [TH_DUSK]  = { 0x000055, 0x000000, 0xFFAA00, 0xFFFFFF, 0xFFAA00, 0xAAAAFF, 0x5555AA , 0x000055 , 0x5555FF , 0x0000AA },
  // noir — no hue at all; the horizon is the only pure white shape
  [TH_NOIR]  = { 0x000000, 0x000000, 0xFFFFFF, 0xFFFFFF, 0xAAAAAA, 0xAAAAAA, 0x555555 , 0x555555 , 0x555555 , 0x000000 },
  // paper — the light one, and the most readable in direct sun
  [TH_PAPER] = { 0xFFFFFF, 0xFFFFAA, 0xFF5500, 0x000000, 0xFF5500, 0x555555, 0xAAAAAA , 0xAAAAAA , 0xAAAAAA , 0xFFFFFF },
  // moss
  [TH_MOSS]  = { 0x005500, 0x000000, 0xFFFF55, 0xFFFFFF, 0xAAFF00, 0xAAFFAA, 0x55AA55 , 0x00AA00 , 0x55AA55 , 0x005500 },
  // tide
  [TH_TIDE]  = { 0x005555, 0x000000, 0x00FFFF, 0xFFFFFF, 0x00FFFF, 0xAAFFFF, 0x55AAAA , 0x00AAAA , 0x55AAAA , 0x005555 },
  // TH_CUSTOM is a hole in this table: it comes from g_cfg instead
  // phosphor — two phosphors on a dead screen, orange for time, green for body
  [TH_PHOSPHOR] = { 0x000000, 0x000000, 0xFF5500, 0xFF5500, 0x00AA55, 0x00AA55, 0x005555 , 0x000000 , 0x005555 , 0x000000 },
};

static void resolve(void) {
  Theme custom = { g_cfg.c_sky, g_cfg.c_ground, g_cfg.c_horizon, g_cfg.c_ink,
                   g_cfg.c_accent, g_cfg.c_muted, g_cfg.c_scale,
                   g_cfg.c_unlit == COL_INHERIT ? g_cfg.c_sky : g_cfg.c_unlit,
                   g_cfg.c_sep == COL_INHERIT ? g_cfg.c_scale : g_cfg.c_sep,
                   // Custom's own inherit check runs below; this is only the
                   // fallback for a save from before the panels existed.
                   g_cfg.c_sky };
  const Theme *t = (g_cfg.theme == TH_CUSTOM) ? &custom
                 : &THEMES[g_cfg.theme < TH_COUNT ? g_cfg.theme : TH_DUSK];
  s_pal.sky     = GColorFromHEX(t->sky);
  s_pal.ground  = GColorFromHEX(t->ground);
  s_pal.horizon = GColorFromHEX(t->horizon);
  s_pal.ink     = GColorFromHEX(t->ink);
  s_pal.accent  = GColorFromHEX(t->accent);
  s_pal.muted   = GColorFromHEX(t->muted);
  s_pal.scale   = GColorFromHEX(t->scale);

#if defined(PBL_BW)
  // Diorite and Flint have two colors, not sixty-four, and no grey to fall
  // back on. Every theme would quantize by luminance, which puts the scale
  // tone (0x5555AA) on the black side and makes the graticule, the labels and
  // the battery bar vanish into the ground. So the palette is not resolved at
  // all here: everything is white on black, and the hierarchy that color was
  // carrying moves into size, weight and position — which the layout already
  // has. The theme picker still works, it just has nothing to change.
  s_pal.sky = s_pal.ground = GColorBlack;
  s_pal.horizon = s_pal.ink = s_pal.accent = GColorWhite;
  s_pal.muted = s_pal.scale = GColorWhite;
  s_pal.terrain = s_pal.now = s_pal.sleep = GColorWhite;
  s_pal.unlit = GColorBlack;   // no third tone to ghost with
  s_pal.label = s_pal.sep = GColorWhite;
  s_pal.info_bg = GColorBlack;
  return;
#endif

  // The three that were once shared. A preset keeps them tied on purpose; only
  // Custom can cut them loose, and only where it has actually been asked to.
  bool is_custom = g_cfg.theme == TH_CUSTOM;
  s_pal.terrain = GColorFromHEX(
      is_custom && g_cfg.c_terrain != COL_INHERIT ? g_cfg.c_terrain : t->accent);
  s_pal.now     = GColorFromHEX(
      is_custom && g_cfg.c_now != COL_INHERIT ? g_cfg.c_now : t->ink);
  s_pal.sleep   = GColorFromHEX(
      is_custom && g_cfg.c_sleep != COL_INHERIT ? g_cfg.c_sleep : t->muted);
  s_pal.unlit   = GColorFromHEX(t->unlit);
  s_pal.sep     = GColorFromHEX(t->sep);
  s_pal.label   = GColorFromHEX(
      is_custom && g_cfg.c_label != COL_INHERIT ? g_cfg.c_label : t->scale);
  s_pal.info_bg = GColorFromHEX(
      is_custom && g_cfg.c_info_bg != COL_INHERIT ? g_cfg.c_info_bg : t->info_bg);
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

// Clay's color picker sends a packed 0xRRGGBB integer.
static uint32_t tup_col(DictionaryIterator *it, uint32_t key, uint32_t fb) {
  Tuple *t = dict_find(it, key);
  if (!t) return fb;
  if (t->type == TUPLE_CSTRING)
    return (uint32_t)strtol(t->value->cstring, NULL, 0) & 0xFFFFFF;
  return (uint32_t)t->value->int32 & 0xFFFFFF;
}

static void inbox(DictionaryIterator *it, void *ctx) {
  g_cfg.theme        = tup_int(it, MESSAGE_KEY_Theme, g_cfg.theme);
  if (g_cfg.theme >= TH_COUNT) g_cfg.theme = TH_DUSK;
  g_cfg.date_format  = tup_int(it, MESSAGE_KEY_DateFormat, g_cfg.date_format);
  g_cfg.dist_unit    = tup_int(it, MESSAGE_KEY_DistUnit, g_cfg.dist_unit);
  if (g_cfg.dist_unit > DIST_MI) g_cfg.dist_unit = DIST_AUTO;
  g_cfg.leading_zero = tup_int(it, MESSAGE_KEY_LeadingZero, g_cfg.leading_zero);
  g_cfg.show_bpm     = tup_int(it, MESSAGE_KEY_ShowBpm, g_cfg.show_bpm);
  g_cfg.show_battery = tup_int(it, MESSAGE_KEY_ShowBattery, g_cfg.show_battery);
  g_cfg.bt_vibe      = tup_int(it, MESSAGE_KEY_BTVibe, g_cfg.bt_vibe);
  g_cfg.show_sleep   = tup_int(it, MESSAGE_KEY_ShowSleep, g_cfg.show_sleep);
  g_cfg.sleep_terrain = tup_int(it, MESSAGE_KEY_SleepTerrain, g_cfg.sleep_terrain);
  g_cfg.weather_on   = tup_int(it, MESSAGE_KEY_WeatherOn, g_cfg.weather_on);
  int wake = tup_int(it, MESSAGE_KEY_WakeThreshold, g_cfg.wake_threshold);
  if (wake < 0) wake = 0;
  if (wake > 20000) wake = 20000;
  g_cfg.wake_threshold = (uint16_t)wake;
  g_cfg.clock_font   = tup_int(it, MESSAGE_KEY_ClockFont, g_cfg.clock_font);
  if (g_cfg.clock_font >= CF_COUNT) g_cfg.clock_font = CF_MONT;
  g_cfg.layout       = tup_int(it, MESSAGE_KEY_Layout, g_cfg.layout);
  g_cfg.auto_hide    = tup_int(it, MESSAGE_KEY_AutoHide, g_cfg.auto_hide);
  g_cfg.grow_clock   = tup_int(it, MESSAGE_KEY_GrowClock, g_cfg.grow_clock);
  if (g_cfg.layout >= LAY_COUNT) g_cfg.layout = LAY_STACK;
  g_cfg.c_sky     = tup_col(it, MESSAGE_KEY_ColSky, g_cfg.c_sky);
  g_cfg.c_ground  = tup_col(it, MESSAGE_KEY_ColGround, g_cfg.c_ground);
  g_cfg.c_horizon = tup_col(it, MESSAGE_KEY_ColHorizon, g_cfg.c_horizon);
  g_cfg.c_ink     = tup_col(it, MESSAGE_KEY_ColInk, g_cfg.c_ink);
  g_cfg.c_accent  = tup_col(it, MESSAGE_KEY_ColAccent, g_cfg.c_accent);
  g_cfg.c_muted   = tup_col(it, MESSAGE_KEY_ColMuted, g_cfg.c_muted);
  g_cfg.c_scale   = tup_col(it, MESSAGE_KEY_ColScale, g_cfg.c_scale);
  g_cfg.c_terrain = tup_col(it, MESSAGE_KEY_ColTerrain, g_cfg.c_terrain);
  g_cfg.c_now     = tup_col(it, MESSAGE_KEY_ColNow, g_cfg.c_now);
  g_cfg.c_sleep   = tup_col(it, MESSAGE_KEY_ColSleep, g_cfg.c_sleep);
  g_cfg.c_unlit   = tup_col(it, MESSAGE_KEY_ColUnlit, g_cfg.c_unlit);
  g_cfg.c_label   = tup_col(it, MESSAGE_KEY_ColLabel, g_cfg.c_label);
  g_cfg.c_sep     = tup_col(it, MESSAGE_KEY_ColSep, g_cfg.c_sep);
  g_cfg.c_info_bg = tup_col(it, MESSAGE_KEY_ColInfoBg, g_cfg.c_info_bg);
  g_cfg.show_sep  = tup_int(it, MESSAGE_KEY_ShowSep, g_cfg.show_sep);
  g_cfg.text_font = tup_int(it, MESSAGE_KEY_TextFont, g_cfg.text_font);
  if (g_cfg.text_font >= TF_COUNT) g_cfg.text_font = TF_MONT;
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
  int n = persist_exists(KEY_SETTINGS) ? persist_get_size(KEY_SETTINGS) : 0;
  if (n > 0 && n <= (int)sizeof g_cfg) {
    Settings tmp = g_cfg;
    persist_read_data(KEY_SETTINGS, &tmp, n);
    if (tmp.version == SETTINGS_VERSION) {
      g_cfg = tmp;
      // A save from before the panels existed stops short of auto_hide, so
      // that field keeps the default set above — which is now on. Switching it
      // on for somebody who never asked would hide the face they have been
      // wearing until they happened to shake it, so a short save means off.
      // New installs, which have no save at all, still get it on.
      if (n < (int)(offsetof(Settings, auto_hide) + sizeof g_cfg.auto_hide))
        g_cfg.auto_hide = false;
    }
  }
  resolve();
  app_message_register_inbox_received(inbox);
  app_message_open(1024, 64);
}
