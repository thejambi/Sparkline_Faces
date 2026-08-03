#include "ui.h"
#include "settings.h"
#include "health.h"

// Emberline. See ui.h for the layout and why the clock is stacked.
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
// The clock face is loaded on demand rather than living in this set: only
// one of the four bundled clock faces is ever on screen, and keeping all of
// them resident would cost RAM for nothing.
typedef enum { F_VAL, F_UNIT, F_CAPS, F_CAPS_S, F_COUNT } FontId;

static GFont s_font[F_COUNT];
static int s_ascent[F_COUNT];

// Each platform ships only its own sizes — an 88px numeral is meaningless on
// a 144px screen, and carrying both sets would double the resource budget for
// nothing. The names differ, so the choice has to be made at compile time.
#if defined(PBL_PLATFORM_EMERY)
#define T_MONT   RESOURCE_ID_FONT_VALUE_22, RESOURCE_ID_FONT_UNIT_14, \
                 RESOURCE_ID_FONT_CAPS_15,  RESOURCE_ID_FONT_CAPS_11
#define T_INTER  RESOURCE_ID_FONT_IVAL_22,  RESOURCE_ID_FONT_IUNI_14, \
                 RESOURCE_ID_FONT_ICAP_15,  RESOURCE_ID_FONT_ICPS_11
#define T_SOURCE RESOURCE_ID_FONT_SVAL_22,  RESOURCE_ID_FONT_SUNI_14, \
                 RESOURCE_ID_FONT_SCAP_15,  RESOURCE_ID_FONT_SCPS_11
#else
#define T_MONT   RESOURCE_ID_FONT_VALUE_16, RESOURCE_ID_FONT_UNIT_10, \
                 RESOURCE_ID_FONT_DATE_11,  RESOURCE_ID_FONT_CAPS_9
#define T_INTER  RESOURCE_ID_FONT_IVAL_16,  RESOURCE_ID_FONT_IUNI_10, \
                 RESOURCE_ID_FONT_ICAP_11,  RESOURCE_ID_FONT_ICPS_9
#define T_SOURCE RESOURCE_ID_FONT_SVAL_16,  RESOURCE_ID_FONT_SUNI_10, \
                 RESOURCE_ID_FONT_SCAP_11,  RESOURCE_ID_FONT_SCPS_9
#endif

// The face used for everything that is not the clock. Same four roles in the
// same order whichever family is chosen, so only the resource ids change.
static const uint32_t TEXT_RES[TF_COUNT][F_COUNT] = {
  [TF_MONT]   = { T_MONT },
  [TF_INTER]  = { T_INTER },
  [TF_SOURCE] = { T_SOURCE },
  [TF_SYSTEM] = { 0, 0, 0, 0 },
};

// Gothic is the only system family that reaches these sizes at all. Bitham has
// nothing full below 30 and the largest role here is 22; its 18 and 34 cuts are
// reduced-charset subsets. LECO and the Bitham numerals are digits-only, which
// rules them out for the caps.
//
// The grid is coarser than the bundled faces — 09/14/18/24 against 11/14/15/22
// — so the proportions are near, not equal. It costs no resource bytes at all,
// which is the trade.
static const char *TEXT_SYS[TF_COUNT][F_COUNT] = {
#if defined(PBL_PLATFORM_EMERY)
  [TF_SYSTEM] = { FONT_KEY_GOTHIC_24_BOLD, FONT_KEY_GOTHIC_14,
                  FONT_KEY_GOTHIC_18_BOLD, FONT_KEY_GOTHIC_14_BOLD },
#else
  [TF_SYSTEM] = { FONT_KEY_GOTHIC_18_BOLD, FONT_KEY_GOTHIC_09,
                  FONT_KEY_GOTHIC_14_BOLD, FONT_KEY_GOTHIC_09 },
#endif
};

// A representative glyph per font: its box height, minus nothing, is the
// distance from box top to baseline for a face with no descenders — which is
// true of every charset here (digits, caps, and h/m).
static const char *FONT_PROBE[F_COUNT] = { "8", "h", "8", "B" };

static GSize tsz(const char *s, FontId f) {
  return graphics_text_layout_get_content_size(s, s_font[f],
      GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
}

static void clock_resolve(void);

static uint8_t s_text_loaded = 0xFF;
// A system font must never be handed to fonts_unload_custom_font, so which of
// the four are ours has to be remembered rather than assumed.
static bool s_font_custom[F_COUNT];

static void text_unload(void) {
  for (int i = 0; i < F_COUNT; i++) {
    if (s_font[i] && s_font_custom[i]) fonts_unload_custom_font(s_font[i]);
    s_font[i] = NULL;
    s_font_custom[i] = false;
  }
  s_text_loaded = 0xFF;
}

static void text_load(void) {
  uint8_t want = g_cfg.text_font < TF_COUNT ? g_cfg.text_font : TF_MONT;
  if (want == s_text_loaded) return;
  text_unload();
  for (int i = 0; i < F_COUNT; i++) {
    const char *key = TEXT_SYS[want][i];
    if (key) {
      s_font[i] = fonts_get_system_font(key);
    } else {
      s_font[i] = fonts_load_custom_font(resource_get_handle(TEXT_RES[want][i]));
      s_font_custom[i] = true;
    }
    s_ascent[i] = tsz(FONT_PROBE[i], (FontId)i).h;
  }
  s_text_loaded = want;
}

static void fonts_load(void) {
  text_load();
  clock_resolve();
}

// The clock is the one place a system face can appear, so it is resolved
// separately from the bundled set. `slot` is the width of the widest digit:
// Montserrat is proportional, and drawing into a fixed slot is what stops the
// minutes shuffling sideways when the digits change. LECO is already tabular
// and simply agrees.
static GFont s_clock, s_clock_custom;
static uint32_t s_clock_res;
static int s_clock_asc, s_clock_slot;

// A 60px LECO and an 88px Roboto cannot share a grid, so every (layout, face)
// pair carries its own. A `light` of 0 means the system LECO. In the stacked
// layout the clock is as large as each face can go before it runs out of
// height, which is why the horizon — and so the terrain — sits lower there.
// That is the trade, made explicit.
typedef struct {
  uint32_t light, bold;
  int b_hour, b_min;          // stacked
  int b_clock;                // one line
  int horizon;
} ClockGrid;

#if defined(PBL_PLATFORM_EMERY)
// Stacked sits at cap 68 with the horizon at 188: the clock is height-bound,
// not width-bound — every face leaves 16 to 54px unused beside the day column
// — so the only way to grow it was to spend terrain, 42 rows down to 34.
//
// Sizes come from tools/make_fonts.py, which solves each face for that cap and
// for the widest one-line set that clears the margins. They are not round
// numbers because the faces are not the same width.
static const ClockGrid GRID[LAY_COUNT][CF_COUNT] = {
  [LAY_STACK] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_94, RESOURCE_ID_FONT_CLOCK_B_94, 106, 182, 0, 188 },
    [CF_ROBOTO]  = { RESOURCE_ID_FONT_ROBO_93,  RESOURCE_ID_FONT_ROBO_B_93,  106, 182, 0, 188 },
    [CF_GROTESK] = { RESOURCE_ID_FONT_GROT_77,  RESOURCE_ID_FONT_GROT_B_77,  118, 182, 0, 188 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_91,  RESOURCE_ID_FONT_INTR_B_91,  106, 182, 0, 188 },
    [CF_SOURCE]  = { RESOURCE_ID_FONT_SRCE_103, RESOURCE_ID_FONT_SRCE_B_103, 106, 182, 0, 188 },
    [CF_PLEX]    = { RESOURCE_ID_FONT_PLEX_95,  RESOURCE_ID_FONT_PLEX_B_95,  106, 182, 0, 188 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_68,  RESOURCE_ID_FONT_DSEG_B_68,  106, 182, 0, 188 },
    // LECO is a system face and cannot grow, so it keeps its taller terrain
    [CF_LECO]    = { 0, 0, 92, 150, 0, 156 },
  },
  [LAY_LINE] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_57, RESOURCE_ID_FONT_CLOCK_B_57, 0, 0, 130, 156 },
    [CF_ROBOTO]  = { RESOURCE_ID_FONT_ROBO_67,  RESOURCE_ID_FONT_ROBO_B_67,  0, 0, 132, 158 },
    [CF_GROTESK] = { RESOURCE_ID_FONT_GROT_59,  RESOURCE_ID_FONT_GROT_B_59,  0, 0, 130, 156 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_56,  RESOURCE_ID_FONT_INTR_B_56,  0, 0, 130, 156 },
    [CF_SOURCE]  = { RESOURCE_ID_FONT_SRCE_71,  RESOURCE_ID_FONT_SRCE_B_71,  0, 0, 132, 158 },
    [CF_PLEX]    = { RESOURCE_ID_FONT_PLEX_57,  RESOURCE_ID_FONT_PLEX_B_57,  0, 0, 130, 156 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_49,  RESOURCE_ID_FONT_DSEG_B_49,  0, 0, 130, 156 },
    [CF_LECO]    = { 0, 0, 0, 0, 130, 156 },
  },
};
#else   // 144x168: width binds here, not height
static const ClockGrid GRID[LAY_COUNT][CF_COUNT] = {
  [LAY_STACK] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_58, RESOURCE_ID_FONT_CLOCK_B_58, 75, 122, 0, 128 },
    [CF_ROBOTO]  = { RESOURCE_ID_FONT_ROBO_58,  RESOURCE_ID_FONT_ROBO_B_58,  74, 122, 0, 128 },
    [CF_GROTESK] = { RESOURCE_ID_FONT_GROT_54,  RESOURCE_ID_FONT_GROT_B_54,  77, 122, 0, 128 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_56,  RESOURCE_ID_FONT_INTR_B_56,  74, 122, 0, 128 },
    [CF_SOURCE]  = { RESOURCE_ID_FONT_SRCE_65,  RESOURCE_ID_FONT_SRCE_B_65,  74, 122, 0, 128 },
    [CF_PLEX]    = { RESOURCE_ID_FONT_PLEX_59,  RESOURCE_ID_FONT_PLEX_B_59,  74, 122, 0, 128 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_43,  RESOURCE_ID_FONT_DSEG_B_43,  74, 122, 0, 128 },
    [CF_LECO]    = { 0, 0, 62, 98, 0, 104 },
  },
  [LAY_LINE] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_38, RESOURCE_ID_FONT_CLOCK_B_38, 0, 0, 94, 114 },
    [CF_ROBOTO]  = { RESOURCE_ID_FONT_ROBO_44,  RESOURCE_ID_FONT_ROBO_B_44,  0, 0, 96, 116 },
    [CF_GROTESK] = { RESOURCE_ID_FONT_GROT_40,  RESOURCE_ID_FONT_GROT_B_40,  0, 0, 94, 114 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_37,  RESOURCE_ID_FONT_INTR_B_37,  0, 0, 94, 114 },
    [CF_SOURCE]  = { RESOURCE_ID_FONT_SRCE_48,  RESOURCE_ID_FONT_SRCE_B_48,  0, 0, 96, 116 },
    [CF_PLEX]    = { RESOURCE_ID_FONT_PLEX_39,  RESOURCE_ID_FONT_PLEX_B_39,  0, 0, 94, 114 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_33,  RESOURCE_ID_FONT_DSEG_B_33,  0, 0, 94, 114 },
    [CF_LECO]    = { 0, 0, 0, 0, 94, 114 },
  },
};
#endif

static const ClockGrid *grid(void) {
  return &GRID[g_cfg.layout < LAY_COUNT ? g_cfg.layout : 0]
              [g_cfg.clock_font < CF_COUNT ? g_cfg.clock_font : 0];
}

// Where the day column's last line sits. It floats rather than tying to the
// hour's baseline: as a three-line block, its own rhythm down the right margin
// matters more than an alignment nothing else in that column shares. Placed so
// the gap up to the header equals the gap down to the temperature —
//
//   (foot - DATE_BLOCK_H) - BASE_ROW1 == (b_min - INK_VAL) - foot
//
// which is 113 against Montserrat's 174 minutes — eleven rows below where the
// hour's baseline would have put it, and 45 of clear sky on either side of the
// block instead of 34 above and 56 below.
static int date_foot(void) {
  return (grid()->b_min - INK_VAL + DATE_BLOCK_H + BASE_ROW1) / 2;
}

static int horizon_y(void) { return grid()->horizon; }
static int ground_y(void) { return horizon_y() + HORIZON_H; }
static int bar_max(void) { return PLOT_BOT - ground_y() - 3; }

static void clock_resolve(void) {
  const ClockGrid *g = grid();
  uint32_t want = g_cfg.bold_clock ? g->bold : g->light;
  if (!want)
#if defined(PBL_PLATFORM_EMERY)
    s_clock = fonts_get_system_font(g_cfg.bold_clock
        ? FONT_KEY_LECO_60_BOLD_NUMBERS_AM_PM
        : FONT_KEY_LECO_60_NUMBERS_AM_PM);
#else
    // 42 is as far as LECO goes on these, and there is no bold cut at that
    // size, so the bold toggle simply has nothing to say about LECO here.
    s_clock = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
#endif
  if (want != s_clock_res) {
    if (s_clock_custom) {
      fonts_unload_custom_font(s_clock_custom);
      s_clock_custom = NULL;
    }
    s_clock_res = want;
    if (want) s_clock_custom = fonts_load_custom_font(resource_get_handle(want));
  }
  if (want) s_clock = s_clock_custom;
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

void face_fonts_changed(void) { text_load(); clock_resolve(); }

// A seven-segment display shows the segments it is *not* lighting, and that is
// most of what makes one read as a display rather than as a typeface. DSEG
// draws them by setting an 8 underneath every digit in a darker tone.
//
// It only happens where the palette has somewhere to put that tone: it must
// sit between the sky and the ink, and above a lit sky the Pebble 64 has
// nothing there — on Dusk every candidate reads as 88 rather than 9. Those
// themes set unlit to their own sky, and this quietly draws nothing.
static void draw_ghost(GContext *ctx, int x, int slot_w, int baseline) {
  const Palette *p = palette();
  if (g_cfg.clock_font != CF_DSEG || gcolor_equal(p->unlit, p->sky)) return;
  GSize sz = graphics_text_layout_get_content_size("8", s_clock,
      GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
  graphics_context_set_text_color(ctx, p->unlit);
  graphics_draw_text(ctx, "8", s_clock,
      GRect(x + (slot_w - sz.w) / 2, baseline - s_clock_asc, sz.w + 12,
            sz.h + 8),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, p->ink);
}

// Right-aligned within the block, one digit per slot. A single-digit hour
// therefore sits above the minutes' second digit rather than above the first.
static void draw_clock_num(GContext *ctx, const char *s, int baseline) {
  int n = strlen(s);
  int right = MARGIN_L + 2 * s_clock_slot;
  // Both slots get their unlit 8, even the one no digit lands in — a real
  // display does not go dark where the hour happens to be one digit.
  for (int i = 0; i < 2; i++)
    draw_ghost(ctx, MARGIN_L + i * s_clock_slot, s_clock_slot, baseline);
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

// One line, with the colon pinned to the center of the screen: the hours
// right-align into it and the minutes hang off it, so the colon never moves
// however the digits change.
static void draw_clock_line(GContext *ctx, const char *hh, const char *mm,
                            int baseline) {
  GSize cs = graphics_text_layout_get_content_size(":", s_clock,
      GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
  int cslot = cs.w + 8;
  int cx0 = SCREEN_W / 2 - cslot / 2;

  for (int i = 0; i < 2; i++) {
    draw_ghost(ctx, cx0 - (2 - i) * s_clock_slot, s_clock_slot, baseline);
    draw_ghost(ctx, cx0 + cslot + i * s_clock_slot, s_clock_slot, baseline);
  }
  int x = cx0 - (int)strlen(hh) * s_clock_slot;
  for (const char *c = hh; *c; c++) {
    char one[2] = { *c, 0 };
    GSize sz = graphics_text_layout_get_content_size(one, s_clock,
        GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    graphics_draw_text(ctx, one, s_clock,
        GRect(x + (s_clock_slot - sz.w) / 2, baseline - s_clock_asc,
              sz.w + 12, sz.h + 8),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    x += s_clock_slot;
  }
  graphics_draw_text(ctx, ":", s_clock,
      GRect(cx0 + (cslot - cs.w) / 2, baseline - s_clock_asc, cs.w + 12,
            cs.h + 8),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  x = cx0 + cslot;
  for (const char *c = mm; *c; c++) {
    char one[2] = { *c, 0 };
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
  text_unload();
  if (s_clock_custom) {
    fonts_unload_custom_font(s_clock_custom);
    s_clock_custom = NULL;
    s_clock_res = 0;
  }
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
//
// A space is advance, never a glyph. Every face here is subsetted by
// characterRegex to the characters that carry ink, so asking one for a space
// is asking for something that was never generated — which draws as nothing
// at 15pt on Emery and as the wildcard box at 11pt on Basalt. Stepping over it
// is the same width either way, and does not depend on what the subsetter
// happened to keep.
static int space_w(FontId f) { return tsz("0", f).w / 2; }

static int tracked_w(const char *s, FontId f) {
  int w = 0;
  for (const char *c = s; *c; c++) {
    w += (*c == ' ' ? space_w(f) : tsz((char[2]){ *c, 0 }, f).w) + TRACK_CAPS;
  }
  return w > 0 ? w - TRACK_CAPS : 0;
}

static void draw_tracked(GContext *ctx, const char *s, FontId f, int x,
                         int baseline) {
  for (const char *c = s; *c; c++) {
    if (*c == ' ') { x += space_w(f) + TRACK_CAPS; continue; }
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
static void draw_degree(GContext *ctx, int x, int top, int size, GColor col) {
  graphics_context_set_stroke_color(ctx, col);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(x, top, size, size));
}

static void draw_sky(GContext *ctx) {
  const Palette *p = palette();
  char buf[24];

  // The day's values and the body's values, each group kept with its own kind.
  // On one line they are two rows at the top left, set small. Stacked they are
  // a column down the right margin — and being a column is the point: the
  // widest thing beside the clock becomes "THU" rather than "8,842", which is
  // what pays for the numerals being as large as they are.
  //
  // A column has the room for the whole date, so it takes all three parts and
  // ignores the weekday-or-month setting entirely; on one line the two would
  // crowd the row, which is what the setting is there to choose between.
  bool line = g_cfg.layout == LAY_LINE;

  if (g_cfg.date_format != DATE_OFF) {
    if (line) {
      const char *l = g_cfg.date_format == DATE_DAYNUM ? WD[s_wday] : MO[s_mon];
      snprintf(buf, sizeof buf, "%s %d", l, s_mday);
      graphics_context_set_text_color(ctx, p->muted);
      draw_tracked(ctx, buf, F_CAPS, MARGIN_L, BASE_ROW1);
    } else {
      int b_mon = date_foot();
      int b_day = b_mon - LABEL_DROP;
      snprintf(buf, sizeof buf, "%d", s_mday);
      graphics_context_set_text_color(ctx, p->muted);
      draw_right(ctx, buf, F_VAL, MARGIN_R, b_day);
      graphics_context_set_text_color(ctx, p->label);
      const char *wd = WD[s_wday], *mo = MO[s_mon];
      draw_tracked(ctx, wd, F_CAPS_S, MARGIN_R - tracked_w(wd, F_CAPS_S),
                   b_day - LABEL_RISE);
      draw_tracked(ctx, mo, F_CAPS_S, MARGIN_R - tracked_w(mo, F_CAPS_S),
                   b_mon);
    }
  }

  if (temp_fresh()) {
    snprintf(buf, sizeof buf, "%d", (int)s_temp);
    graphics_context_set_text_color(ctx, p->muted);
    if (line) {
      int w = tracked_w(buf, F_CAPS);
      draw_tracked(ctx, buf, F_CAPS, MARGIN_L, BASE_ROW2);
      draw_degree(ctx, MARGIN_L + w + 3, BASE_ROW2 - INK_CAPS_S - 3,
                  DEG_SIZE_S, p->muted);
    } else {
      draw_right(ctx, buf, F_VAL, MARGIN_R, grid()->b_min);
      draw_degree(ctx, MARGIN_R + 3, grid()->b_min - INK_VAL, DEG_SIZE,
                  p->muted);
    }
  }

  int h = s_hour;
  if (!clock_is_24h_style()) { h %= 12; if (h == 0) h = 12; }
  graphics_context_set_text_color(ctx, p->ink);
  char mmbuf[4];
  if (clock_is_24h_style() || g_cfg.leading_zero)
    snprintf(buf, sizeof buf, "%02d", h);
  else
    snprintf(buf, sizeof buf, "%d", h);
  snprintf(mmbuf, sizeof mmbuf, "%02d", s_min);
  if (line) {
    draw_clock_line(ctx, buf, mmbuf, grid()->b_clock);
  } else {
    draw_clock_num(ctx, buf, grid()->b_hour);
    draw_clock_num(ctx, mmbuf, grid()->b_min);
  }

  // the value slot: steps, or last night's sleep until you are up. Stacked it
  // opens the header row at the left margin and the pulse closes it at the
  // right; on one line the two stack at the right margin instead. Either way
  // the pulse carries its label inboard of the value and the step count
  // carries none — an accent-colored number with a comma in it needs no
  // telling, and the two labels will not fit on one row with the two values.
  int b_val = BASE_ROW1;
  int b_bpm = line ? BASE_ROW2 : BASE_ROW1;
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
    int x = line ? MARGIN_R - total : MARGIN_L;
    graphics_context_set_text_color(ctx, p->sleep);
    x += draw_base(ctx, a, F_VAL, x, b_val) + 2;
    graphics_context_set_text_color(ctx, p->label);
    x += draw_base(ctx, "h", F_UNIT, x, b_val) + 7;
    graphics_context_set_text_color(ctx, p->sleep);
    x += draw_base(ctx, b, F_VAL, x, b_val) + 2;
    graphics_context_set_text_color(ctx, p->label);
    draw_base(ctx, "m", F_UNIT, x, b_val);
  } else {
    fmt_thousands(buf, sizeof buf, hl_steps());
    graphics_context_set_text_color(ctx, p->accent);
    if (line) draw_right(ctx, buf, F_VAL, MARGIN_R, b_val);
    else      draw_base(ctx, buf, F_VAL, MARGIN_L, b_val);
  }

  // the pulse, always right-aligned with its label inboard of it
  int bpm = g_cfg.show_bpm ? hl_bpm() : 0;
  const char *lbl = NULL;
  if (bpm > 0) {
    snprintf(buf, sizeof buf, "%d", bpm);
    lbl = "BPM";
  } else if (!hl_sleeping()) {
    // no sensor: the distance takes the slot rather than leaving a hole
    fmt1(buf, sizeof buf, use_miles() ? hl_walked_m() / 1609.344
                                      : hl_walked_m() / 1000.0);
    lbl = use_miles() ? "MI" : "KM";
  }
  if (lbl) {
    graphics_context_set_text_color(ctx, p->muted);
    int vw = draw_right(ctx, buf, F_VAL, MARGIN_R, b_bpm);
    graphics_context_set_text_color(ctx, p->label);
    draw_tracked(ctx, lbl, F_CAPS_S,
                 MARGIN_R - vw - 8 - tracked_w(lbl, F_CAPS_S), b_bpm);
  }
}

static void draw_ground(GContext *ctx) {
  const Palette *p = palette();
  const Terrain *t = hl_terrain();
  bool sleeping = hl_sleeping();

  GColor bar = sleeping ? p->sleep : p->terrain;
  GColor now = sleeping ? p->scale : p->now;

  graphics_context_set_fill_color(ctx, sleeping ? p->sleep : p->horizon);
  graphics_fill_rect(ctx, GRect(0, horizon_y(), SCREEN_W, HORIZON_H), 0,
                     GCornerNone);
  graphics_context_set_fill_color(ctx, p->ground);
  graphics_fill_rect(ctx, GRect(0, ground_y(), SCREEN_W,
                                SCREEN_H - ground_y()), 0, GCornerNone);

  // A quarter-hour architecture behind the terrain. Where a column covers it
  // the rule vanishes; where it does not, it reads as structure.
  graphics_context_set_fill_color(ctx, p->scale);
  for (int i = 0; i < COLS; i++) {
    int wall = (s_min + 1 + i) % 60;
    if (t->live && wall % 15) continue;
    if (!t->live && i % 15) continue;
    int x = PLOT_X + COL_W * i;
#if defined(PBL_BW)
    // With no grey, a solid white rule is as loud as the bars it sits behind
    // and the terrain stops reading as a silhouette. Drawn every other row it
    // recedes — the same trick the walking-pace line already uses.
    for (int y = ground_y(); y <= PLOT_BOT; y += 2)
      graphics_fill_rect(ctx, GRect(x, y, 1, 1), 0, GCornerNone);
#else
    graphics_fill_rect(ctx, GRect(x, ground_y(), 1,
                                  PLOT_BOT - ground_y() + 1), 0, GCornerNone);
#endif
  }
  graphics_fill_rect(ctx, GRect(PLOT_X, PLOT_BOT, COL_W * COLS, 1), 0,
                     GCornerNone);

  // the pace that counts as walking — only meaningful on the live hour
  if (t->live) {
    int wy = PLOT_BOT - WALK_RATE * bar_max() / STEP_CAP;
    for (int x = PLOT_X; x < PLOT_X + COL_W * COLS; x += 4)
      graphics_fill_rect(ctx, GRect(x, wy, 1, 1), 0, GCornerNone);
  }

  for (int i = 0; i < COLS; i++) {
    int v = t->col[i];
    if (v > t->cap) v = t->cap;
    int h = (v * bar_max() + t->cap / 2) / t->cap;
    bool is_now = t->live && i == COLS - 1;
    if (is_now && h < 4) h = 4;
    if (h <= 0) continue;
#if defined(PBL_BW)
    // `now` is the clock's color and the bars are the accent, which on two
    // colors is the same white. Cut it away from its neighbour with a column
    // of ground instead, so the newest minute is still findable.
    if (is_now) {
      graphics_context_set_fill_color(ctx, p->ground);
      graphics_fill_rect(ctx, GRect(PLOT_X + COL_W * i - 1, PLOT_BOT - h + 1,
                                    1, h), 0, GCornerNone);
    }
#endif
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
    graphics_context_set_fill_color(ctx, s_batt <= 20 ? GColorRed : p->label);
    graphics_fill_rect(ctx, GRect(0, 0, w, 2), 0, GCornerNone);
  }
  if (!s_bt_ok) {
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, GRect(SCREEN_W - 5, 0, 5, 5), 0, GCornerNone);
  }
}

// The clock's field, and the rule around it.
//
// The sky is filled with the info tone and then the clock's own rectangle is
// laid back over it in the sky tone — cheaper and simpler than working out the
// L-shaped complement, and it means one rounded corner does the whole job.
//
// The rectangle runs off the left edge and past the horizon on purpose: only
// its top edge, its right edge and the corner between them are ever on screen,
// so the other three corners can be rounded without anyone seeing them. On one
// line there is no day column to divide, so the field is full width and the
// rule is a plain horizontal run.
static GRect clock_field(void) {
  bool line = g_cfg.layout == LAY_LINE;
  int top = line ? SEP_Y_LINE : SEP_Y;
  int right = line ? SCREEN_W + SEP_R : SEP_X;
  return GRect(-SEP_R, top, right + SEP_R, horizon_y() - top + SEP_R);
}

static void draw_field(GContext *ctx) {
  const Palette *p = palette();
  GRect f = clock_field();
  if (!gcolor_equal(p->info_bg, p->sky)) {
    graphics_context_set_fill_color(ctx, p->info_bg);
    graphics_fill_rect(ctx, GRect(0, 0, SCREEN_W, horizon_y()), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, p->sky);
    graphics_fill_rect(ctx, f, SEP_R, GCornersAll);
  }
  if (g_cfg.show_sep) {
    graphics_context_set_stroke_color(ctx, p->sep);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_round_rect(ctx, f, SEP_R);
  }
}

static void draw(Layer *layer, GContext *ctx) {
  const Palette *p = palette();
  graphics_context_set_fill_color(ctx, p->sky);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  draw_field(ctx);
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
