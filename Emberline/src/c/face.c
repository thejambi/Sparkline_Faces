#include "ui.h"
#include "settings.h"
#include "digits.h"
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
#define T_DSEG   RESOURCE_ID_FONT_DVAL_17,  RESOURCE_ID_FONT_DUNI_11, \
                 RESOURCE_ID_FONT_DCAP_11,  RESOURCE_ID_FONT_DCPS_11
#else
#define T_MONT   RESOURCE_ID_FONT_VALUE_16, RESOURCE_ID_FONT_UNIT_10, \
                 RESOURCE_ID_FONT_DATE_11,  RESOURCE_ID_FONT_CAPS_9
#define T_DSEG   RESOURCE_ID_FONT_DVAL_13,  RESOURCE_ID_FONT_DUNI_6, \
                 RESOURCE_ID_FONT_DCAP_8,   RESOURCE_ID_FONT_DCPS_8
#endif

// The face used for everything that is not the clock. Same four roles in the
// same order whichever family is chosen, so only the resource ids change.
static const uint32_t TEXT_RES[TF_COUNT][F_COUNT] = {
  [TF_MONT]   = { T_MONT },
  [TF_SYSTEM] = { 0, 0, 0, 0 },
  [TF_DSEG]   = { T_DSEG },
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

// Ink heights and tracking, per text face.
//
// These place the day column and the degree ring, and they were global
// constants taken from Montserrat — which quietly made every other family a
// couple of pixels wrong. They are per-face now, which is also what lets
// DSEG14 carry a taller caption: fourteen segments need more than eight rows
// before the diagonals stop breaking up, and its own tracking is zero because
// segment forms already hold their own air.
//
// Montserrat's row is the pair already in ui.h, which was measured off the
// device by scanning ink rows in a screenshot. The others are derived from a
// local rasteriser, which reports one row taller than the device does — so
// they carry that correction. Montserrat is left exactly as it was rather than
// re-derived, because the default layout must not move.
//
// Gothic is a system face and cannot be measured here, so it borrows
// Montserrat's — near enough, and the same approximation it had before.
typedef struct { uint8_t val_ink, caps_ink, track; } TextMetrics;

static const TextMetrics TEXT_INK[TF_COUNT] = {
#if defined(PBL_PLATFORM_EMERY)
  [TF_MONT] = {INK_VAL, INK_CAPS_S, TRACK_CAPS},
  [TF_DSEG] = {16, 11, 0}, [TF_SYSTEM] = {INK_VAL, INK_CAPS_S, TRACK_CAPS},
#else
  [TF_MONT] = {INK_VAL, INK_CAPS_S, TRACK_CAPS},
  [TF_DSEG] = {12, 7, 0}, [TF_SYSTEM] = {INK_VAL, INK_CAPS_S, TRACK_CAPS},
#endif
};
static uint8_t s_ink_val = INK_VAL, s_ink_caps = INK_CAPS_S, s_track = TRACK_CAPS;

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
  // Source Sans was a text face once; its row is zeroed now, and asking for
  // resource 0 is not a thing to do.
  if (!TEXT_RES[want][0] && !TEXT_SYS[want][0]) want = TF_MONT;
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
  s_ink_val = TEXT_INK[want].val_ink;
  s_ink_caps = TEXT_INK[want].caps_ink;
  s_track = TEXT_INK[want].track;
  s_text_loaded = want;
}

static void fonts_load(void) {
  text_load();
  clock_resolve();
}

// The clock is resolved separately from the bundled text set, because it is
// the one place a face can be geometry rather than a font. `slot` is the
// width of the widest digit:
// Montserrat is proportional, and drawing into a fixed slot is what stops the
// minutes shuffling sideways when the digits change. DSEG and the drawn face
// are already tabular and simply agree.
static GFont s_clock, s_clock_custom;
static uint32_t s_clock_res;
static int s_clock_asc, s_clock_slot;
// Nonzero only for CF_GRID, and then it is the whole face: there is no
// GFont behind it and every draw below branches on this.
static int s_clock_scale, s_clock_air;

// A 68px DSEG and a 94px Montserrat cannot share a grid, so every (layout,
// face) pair carries its own. In the stacked
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
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_91,  RESOURCE_ID_FONT_INTR_B_91,  106, 182, 0, 188 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_68,  RESOURCE_ID_FONT_DSEG_B_68,  106, 182, 0, 188 },
    [CF_LECO]    = { 0, 0, 0, 0, 0, 0 },
    // The drawn face carries a scale, not a resource: 13 rows x5 is a cap of
    // 65 against the 68 the layout allows, so it shares Montserrat's grid.
    [CF_GRID]    = { 5, 5, 106, 182, 0, 188 },
  },
  [LAY_LINE] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_57, RESOURCE_ID_FONT_CLOCK_B_57, 0, 0, 130, 156 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_56,  RESOURCE_ID_FONT_INTR_B_56,  0, 0, 130, 156 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_49,  RESOURCE_ID_FONT_DSEG_B_49,  0, 0, 130, 156 },
    [CF_LECO]    = { 0, 0, 0, 0, 0, 0 },
    // x4 spans 191 of the 200, which is what caps the gap here at 3 rather
    // than the 5 the stacked layout gets. x5 would not fit at all.
    [CF_GRID]    = { 4, 4, 0, 0, 130, 156 },
  },
  // Cards keeps the stacked numerals and horizon exactly. It rearranges what
  // is around the clock, not the clock.
  [LAY_CARDS] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_94, RESOURCE_ID_FONT_CLOCK_B_94, 106, 182, 0, 188 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_91,  RESOURCE_ID_FONT_INTR_B_91,  106, 182, 0, 188 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_68,  RESOURCE_ID_FONT_DSEG_B_68,  106, 182, 0, 188 },
    [CF_GRID]    = { 5, 5, 106, 182, 0, 188 },
  },
};
#else   // 144x168: width binds here, not height
static const ClockGrid GRID[LAY_COUNT][CF_COUNT] = {
  [LAY_STACK] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_58, RESOURCE_ID_FONT_CLOCK_B_58, 75, 122, 0, 128 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_56,  RESOURCE_ID_FONT_INTR_B_56,  74, 122, 0, 128 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_43,  RESOURCE_ID_FONT_DSEG_B_43,  74, 122, 0, 128 },
    [CF_LECO]    = { 0, 0, 0, 0, 0, 0 },
    // x4 would clear the day column but a cap of 52 puts the hour's
    // top 5 rows under the header, so x3 it is.
    [CF_GRID]    = { 3, 3, 75, 122, 0, 128 },
  },
  [LAY_LINE] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_38, RESOURCE_ID_FONT_CLOCK_B_38, 0, 0, 94, 114 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_37,  RESOURCE_ID_FONT_INTR_B_37,  0, 0, 94, 114 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_33,  RESOURCE_ID_FONT_DSEG_B_33,  0, 0, 94, 114 },
    [CF_LECO]    = { 0, 0, 0, 0, 0, 0 },
    [CF_GRID]    = { 2, 2, 0, 0, 94, 114 },
  },
  [LAY_CARDS] = {
    [CF_MONT]    = { RESOURCE_ID_FONT_CLOCK_58, RESOURCE_ID_FONT_CLOCK_B_58, 75, 122, 0, 128 },
    [CF_INTER]   = { RESOURCE_ID_FONT_INTR_56,  RESOURCE_ID_FONT_INTR_B_56,  74, 122, 0, 128 },
    [CF_DSEG]    = { RESOURCE_ID_FONT_DSEG_43,  RESOURCE_ID_FONT_DSEG_B_43,  74, 122, 0, 128 },
    [CF_GRID]    = { 3, 3, 75, 122, 0, 128 },
  },
};
#endif

// The enum keeps the slots of faces that have been dropped, because a watch
// running an older release has one of them persisted and renumbering would
// hand it somebody else's font. Their rows are left zeroed, and a zeroed
// horizon is the tell — nothing legitimate puts the horizon at the top of the
// screen. Those fall back to Montserrat.
static const ClockGrid *grid(void) {
  int lay = g_cfg.layout < LAY_COUNT ? g_cfg.layout : LAY_STACK;
  int face = g_cfg.clock_font < CF_COUNT ? g_cfg.clock_font : CF_MONT;
  if (GRID[lay][face].horizon == 0) face = CF_MONT;
  return &GRID[lay][face];
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
  int block = LABEL_RISE + LABEL_DROP + s_ink_caps;
  return (grid()->b_min - s_ink_val + block + BASE_ROW1) / 2;
}

static int horizon_y(void) { return grid()->horizon; }
static int ground_y(void) { return horizon_y() + HORIZON_H; }
static int bar_max(void) { return PLOT_BOT - ground_y() - 3; }

// Blocky Digits' metrics are pure arithmetic — no resource to load, nothing to
// measure — so they can be recomputed every frame. That is what lets the clock
// change size as the panels come and go.
//
// The gap between slots is one grid column, so the digits breathe by the
// design's own unit. A fixed pixel count cannot: two pixels beside a 50px
// digit is not the same gap as two beside a 20px one, and the drawn face spans
// both. The one line has to hold four of these plus the colon inside the
// screen, so there it takes whatever is left over instead.
static void grid_metrics(int scale) {
  s_clock_scale = scale;
  s_clock_asc = DIGIT_H * scale;
  int ink = DIGIT_W * scale;
  int cw = (DIGIT_COLON_R - DIGIT_COLON_L + 1) * scale;
  s_clock_air = scale;
  if (g_cfg.layout == LAY_LINE) {
    int fit = (SCREEN_W - 8 - 4 * ink - cw) / 5;
    if (fit < 1) fit = 1;
    if (fit < s_clock_air) s_clock_air = fit;
  }
  s_clock_slot = ink + s_clock_air;
}

// Cards' auto-hide. `s_reveal` is 0 with the panels parked off-screen and 100
// with them fully in; every moving thing is drawn from it, so there is one
// number to reason about rather than a position per panel.
//
// A shake brings them in, they hold, then they leave. The frame timer only
// runs while something is actually travelling — at rest there is no timer at
// all, which is the difference between this and a battery complaint.
#define REVEAL_STEP_MS 33
#define REVEAL_SPAN_MS 260
#define REVEAL_HOLD_MS 7000
static int s_reveal = 100;
static bool s_reveal_in;
static AppTimer *s_reveal_timer, *s_hold_timer;

static bool cards_hiding(void) {
  return g_cfg.layout == LAY_CARDS && g_cfg.auto_hide;
}
// Defined down with the timers it owns, but face_fonts_changed needs it here.
static void reveal_sync(void);
static int reveal(void) { return cards_hiding() ? s_reveal : 100; }

// How far each panel has left to travel. The step panel is at the top so it
// leaves upward; the column is at the right so it leaves rightward. Each one
// parks exactly its own size away, so nothing is left peeking at the bezel.
static int side_dx(void) {
  return (SCREEN_W - SEP_X) * (100 - reveal()) / 100;
}
// One past its own height: parking at exactly SEP_Y leaves the panel's bottom
// edge sitting on row 0 as a hairline along the top of the screen.
static int step_dy(void) { return (SEP_Y + 1) * (100 - reveal()) / 100; }

// The header carries two values now, so it spans bezel to bezel and the
// column drops clear of it. Touching, they would read as one inverted T; the
// gap is what makes them two panels.
static int side_top(void) { return SEP_Y + SEP_R; }

// The clock only grows once the panels are more than half gone, because its
// scale cannot be interpolated: whole-number scaling is what keeps the drawn
// digits crisp, so the size has to snap. Snapping it mid-glide, while the eye
// is following the panels, is the least conspicuous moment available.
static bool clock_big(void) {
  return cards_hiding() && s_reveal < 50 && g_cfg.clock_font == CF_GRID;
}
static int clock_b_hour(void) {
  return clock_big() ? CARDS_BIG_HOUR : grid()->b_hour;
}
static int clock_b_min(void) {
  return clock_big() ? CARDS_BIG_MIN : grid()->b_min;
}

static void clock_resolve(void) {
  const ClockGrid *g = grid();
  // The drawn face takes this before the resource machinery gets a look in:
  // its row holds a scale, and handing that to resource_get_handle would be a
  // very confusing crash. It has one weight, so the bold toggle says nothing.
  s_clock_scale = 0;
  if (g_cfg.clock_font == CF_GRID && g->horizon) {
    if (s_clock_custom) {
      fonts_unload_custom_font(s_clock_custom);
      s_clock_custom = NULL;
      s_clock_res = 0;
    }
    s_clock = NULL;
    grid_metrics((int)g->light);
    return;
  }
  // Always the bold cut. It is what survives bright sun, it is what every
  // build has shipped set to, and a toggle nobody moves is a row on the
  // config page paying no rent. `light` stays in the struct because
  // make_fonts.py still emits both weights and a later face may want the
  // choice back.
  uint32_t want = g->bold;
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

void face_fonts_changed(void) { text_load(); clock_resolve(); reveal_sync(); }

// A seven-segment display shows the segments it is *not* lighting, and that is
// most of what makes one read as a display rather than as a typeface. DSEG
// draws them by setting an 8 underneath every digit in a darker tone.
//
// It only happens where the palette has somewhere to put that tone: it must
// sit between the sky and the ink, and above a lit sky the Pebble 64 has
// nothing there — on Dusk every candidate reads as 88 rather than 9. Those
// themes set unlit to their own sky, and this quietly draws nothing.
// One character into its slot, whichever face is in play. The text faces are
// centred on their own ink, because Montserrat is proportional and a fixed
// slot is the only thing stopping the minutes shuffling sideways. The drawn
// face is tabular already and simply lands on the grid.
static void clock_glyph(GContext *ctx, char c, int x, int slot_w, int baseline,
                        GColor col) {
  if (s_clock_scale) {
    bool colon = c == ':';
    int idx = colon ? DIGIT_COLON : c - '0';
    // Center the ink in the slot, not the box. The drawing carries a blank
    // column down one side — which is where the gap between digits comes
    // from — and centering the box counts that blank as glyph, shoving the
    // whole clock half a column across. Taken from the drawing rather than
    // hard-coded, so redrawing the glyphs cannot put it back.
    int l = colon ? DIGIT_COLON_L : DIGIT_INK_L;
    int r = colon ? DIGIT_COLON_R : DIGIT_INK_R;
    graphics_context_set_fill_color(ctx, col);
    digit_draw(ctx, idx,
               x + (slot_w - (r - l + 1) * s_clock_scale) / 2
                 - l * s_clock_scale,
               baseline - s_clock_asc, s_clock_scale);
    return;
  }
  char one[2] = { c, 0 };
  GSize sz = graphics_text_layout_get_content_size(one, s_clock,
      GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
  graphics_context_set_text_color(ctx, col);
  graphics_draw_text(ctx, one, s_clock,
      GRect(x + (slot_w - sz.w) / 2, baseline - s_clock_asc, sz.w + 12,
            sz.h + 8),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

// What the colon occupies on the one line. It is far narrower than a digit in
// either face, and lending it a digit's slot would open a gap on both sides.
static int colon_slot(void) {
  if (s_clock_scale)
    return (DIGIT_COLON_R - DIGIT_COLON_L + 1) * s_clock_scale + s_clock_air;
  return graphics_text_layout_get_content_size(":", s_clock,
      GRect(0, 0, 240, 120), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft).w + 8;
}

// A seven-segment display shows the segments it is *not* lighting, and that is
// most of what makes one read as a display rather than as a typeface. DSEG
// draws them by setting an 8 underneath every digit in a darker tone.
//
// The drawn face is laid out on the same seven segments but does not separate
// them — its bars run into each other at the corners — so an 8 behind it is
// not a row of dark segments, it is a lit rectangle with a few notches. It is
// deliberately left out of this.
//
// It only happens where the palette has somewhere to put that tone: it must
// sit between the sky and the ink, and above a lit sky the Pebble 64 has
// nothing there — on Dusk every candidate reads as 88 rather than 9. Those
// themes set unlit to their own sky, and this quietly draws nothing.
static void draw_ghost(GContext *ctx, int x, int slot_w, int baseline) {
  const Palette *p = palette();
  if (g_cfg.clock_font != CF_DSEG || gcolor_equal(p->unlit, p->sky)) return;
  clock_glyph(ctx, '8', x, slot_w, baseline, p->unlit);
}

// Right-aligned within the block, one digit per slot. A single-digit hour
// therefore sits above the minutes' second digit rather than above the first.
//
// The block itself is centered in the clock's field rather than pinned to the
// left margin. The faces are nowhere near the same width — a two-slot block
// runs from 110px to 145px on Emery — so a fixed left anchor leaves the narrow
// ones stranded a long way from the separator while the wide ones nearly touch
// it. Centering makes every face sit the same way in the same space, which is
// the only thing that makes the choice feel like a choice rather than a
// different layout. Clamped, so a face wider than the field still starts at
// the margin instead of walking off the left bezel.
static void draw_clock_num(GContext *ctx, const char *s, int baseline) {
  GColor ink = palette()->ink;
  int n = strlen(s);
  int block = 2 * s_clock_slot;
  // In Cards the field's right edge glides out to the bezel as the column
  // leaves, and the left margin closes to nothing at the same rate, so the
  // clock stays centered in whatever space it actually has at that instant.
  int fr = SEP_X, fl = MARGIN_L;
  if (g_cfg.layout == LAY_CARDS) {
    int t = reveal();
    fr = SEP_X + (SCREEN_W - SEP_X) * (100 - t) / 100;
    fl = MARGIN_L * t / 100;
  }
  int left = fl + (fr - fl - block) / 2;
  if (left < fl) left = fl;
  int right = left + block;
  // Both slots get their unlit 8, even the one no digit lands in — a real
  // display does not go dark where the hour happens to be one digit.
  for (int i = 0; i < 2; i++)
    draw_ghost(ctx, left + i * s_clock_slot, s_clock_slot, baseline);
  int x = right - n * s_clock_slot;
  for (int i = 0; i < n; i++) {
    clock_glyph(ctx, s[i], x, s_clock_slot, baseline, ink);
    x += s_clock_slot;
  }
}

// One line, with the colon pinned to the center of the screen: the hours
// right-align into it and the minutes hang off it, so the colon never moves
// however the digits change.
static void draw_clock_line(GContext *ctx, const char *hh, const char *mm,
                            int baseline) {
  GColor ink = palette()->ink;
  int cslot = colon_slot();
  int cx0 = SCREEN_W / 2 - cslot / 2;

  for (int i = 0; i < 2; i++) {
    draw_ghost(ctx, cx0 - (2 - i) * s_clock_slot, s_clock_slot, baseline);
    draw_ghost(ctx, cx0 + cslot + i * s_clock_slot, s_clock_slot, baseline);
  }
  int x = cx0 - (int)strlen(hh) * s_clock_slot;
  for (const char *c = hh; *c; c++) {
    clock_glyph(ctx, *c, x, s_clock_slot, baseline, ink);
    x += s_clock_slot;
  }
  clock_glyph(ctx, ':', cx0, cslot, baseline, ink);
  x = cx0 + cslot;
  for (const char *c = mm; *c; c++) {
    clock_glyph(ctx, *c, x, s_clock_slot, baseline, ink);
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
    w += (*c == ' ' ? space_w(f) : tsz((char[2]){ *c, 0 }, f).w) + s_track;
  }
  return w > 0 ? w - s_track : 0;
}

static void draw_tracked(GContext *ctx, const char *s, FontId f, int x,
                         int baseline) {
  for (const char *c = s; *c; c++) {
    if (*c == ' ') { x += space_w(f) + s_track; continue; }
    char one[2] = { *c, 0 };
    x += draw_base(ctx, one, f, x, baseline) + s_track;
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

static void reveal_step(void *unused);

static void reveal_go(bool in) {
  s_reveal_in = in;
  if (!s_reveal_timer)
    s_reveal_timer = app_timer_register(REVEAL_STEP_MS, reveal_step, NULL);
}

static void reveal_leave(void *unused) { s_hold_timer = NULL; reveal_go(false); }

static void reveal_step(void *unused) {
  s_reveal_timer = NULL;
  int d = 100 * REVEAL_STEP_MS / REVEAL_SPAN_MS;
  s_reveal += s_reveal_in ? (d < 1 ? 1 : d) : -(d < 1 ? 1 : d);
  if (s_reveal > 100) s_reveal = 100;
  if (s_reveal < 0) s_reveal = 0;
  if (s_reveal_in ? s_reveal < 100 : s_reveal > 0)
    s_reveal_timer = app_timer_register(REVEAL_STEP_MS, reveal_step, NULL);
  else if (s_reveal_in)
    s_hold_timer = app_timer_register(REVEAL_HOLD_MS, reveal_leave, NULL);
  face_poke();
}

// A shake while they are already up re-arms the hold rather than starting a
// second animation, so shaking twice does not make them leave early.
static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (!cards_hiding()) return;
  if (s_hold_timer) { app_timer_cancel(s_hold_timer); s_hold_timer = NULL; }
  reveal_go(true);
}

// The tap service costs power, so it is only subscribed while a layout is
// actually using it. Settings changes run through here, which is also where
// the panels get put back on screen when auto-hide is switched off — otherwise
// turning it off would leave them parked with nothing left to summon them.
static void reveal_sync(void) {
  static bool on;
  bool want = cards_hiding();
  if (want == on) return;
  on = want;
  if (want) { accel_tap_service_subscribe(tap_handler); s_reveal = 0; }
  else {
    accel_tap_service_unsubscribe();
    s_reveal = 100;
    if (s_reveal_timer) { app_timer_cancel(s_reveal_timer); s_reveal_timer = NULL; }
    if (s_hold_timer) { app_timer_cancel(s_hold_timer); s_hold_timer = NULL; }
  }
}

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

// The right column in the Cards layout. Everything that is not the clock or
// the step count lives here, and the pulse brings its caption underneath the
// number rather than inboard of it — which is the whole move: with the pulse
// out of the header, the header is one value wide, and the two survivors can
// be drawn as two panels instead of one continuous L.
//
// The gaps are divided, not chosen. Whatever height the groups do not use is
// split evenly between them, so turning the date or the weather off re-spaces
// the column rather than leaving a hole where they used to be.
static void draw_side(GContext *ctx) {
  const Palette *p = palette();
  char buf[16], val[16];
  int dx = side_dx();   // the column carries its contents off with it

  // The pulse, and only the pulse — the distance that used to stand in for it
  // has its own slot in the header now. With no reading this group is simply
  // absent, and the gaps below divide the space it is not using, so the column
  // re-spaces instead of showing a hole where a heart rate would have been.
  int bpm = g_cfg.show_bpm ? hl_bpm() : 0;
  const char *lbl = NULL;
  if (bpm > 0) {
    snprintf(val, sizeof val, "%d", bpm);
    lbl = "BPM";
  }
  bool has_date = g_cfg.date_format != DATE_OFF;
  bool has_temp = temp_fresh();
  int n = (lbl ? 1 : 0) + (has_date ? 1 : 0) + (has_temp ? 1 : 0);
  if (!n) return;

  int used = (lbl ? s_ink_val + LABEL_DROP : 0)
           + (has_date ? s_ink_caps + LABEL_RISE + LABEL_DROP : 0)
           + (has_temp ? s_ink_val : 0);
  // Every opening in the column is the same size — the margin above the first
  // group, the margin below the last, and the gaps in between. The slack is
  // divided among all of them, so losing the pulse widens the margins instead
  // of opening one cavern in the middle of the panel.
  //
  // This also handles a lone group without a special case: two openings, one
  // above and one below, which is what centering is.
  int top = side_top(), bot = horizon_y();
  int gap = (bot - top - used) / (n + 1);
  if (gap < 4) gap = 4;
  int y = top + gap;

  if (lbl) {
    int b = y + s_ink_val;
    graphics_context_set_text_color(ctx, p->muted);
    draw_right(ctx, val, F_VAL, MARGIN_R + dx, b);
    graphics_context_set_text_color(ctx, p->label);
    draw_tracked(ctx, lbl, F_CAPS_S, MARGIN_R + dx - tracked_w(lbl, F_CAPS_S),
                 b + LABEL_DROP);
    y = b + LABEL_DROP + gap;
  }
  if (has_date) {
    const char *wd = WD[s_wday], *mo = MO[s_mon];
    int b_wd = y + s_ink_caps;
    int b_day = b_wd + LABEL_RISE;
    int b_mo = b_day + LABEL_DROP;
    graphics_context_set_text_color(ctx, p->label);
    draw_tracked(ctx, wd, F_CAPS_S, MARGIN_R + dx - tracked_w(wd, F_CAPS_S), b_wd);
    graphics_context_set_text_color(ctx, p->muted);
    snprintf(buf, sizeof buf, "%d", s_mday);
    draw_right(ctx, buf, F_VAL, MARGIN_R + dx, b_day);
    graphics_context_set_text_color(ctx, p->label);
    draw_tracked(ctx, mo, F_CAPS_S, MARGIN_R + dx - tracked_w(mo, F_CAPS_S), b_mo);
    y = b_mo + gap;
  }
  if (has_temp) {
    int b = y + s_ink_val;
    snprintf(buf, sizeof buf, "%d", (int)s_temp);
    graphics_context_set_text_color(ctx, p->muted);
    draw_right(ctx, buf, F_VAL, MARGIN_R + dx, b);
    draw_degree(ctx, MARGIN_R + dx + 3, b - s_ink_val, DEG_SIZE, p->muted);
  }
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
  bool cards = g_cfg.layout == LAY_CARDS;

  // Cards puts the date, the temperature and the pulse in one column, spaced
  // against each other rather than against the rows they used to share.
  if (cards) draw_side(ctx);

  if (!cards && g_cfg.date_format != DATE_OFF) {
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

  if (!cards && temp_fresh()) {
    snprintf(buf, sizeof buf, "%d", (int)s_temp);
    graphics_context_set_text_color(ctx, p->muted);
    if (line) {
      int w = tracked_w(buf, F_CAPS);
      draw_tracked(ctx, buf, F_CAPS, MARGIN_L, BASE_ROW2);
      draw_degree(ctx, MARGIN_L + w + 3, BASE_ROW2 - s_ink_caps - 3,
                  DEG_SIZE_S, p->muted);
    } else {
      draw_right(ctx, buf, F_VAL, MARGIN_R, grid()->b_min);
      draw_degree(ctx, MARGIN_R + 3, grid()->b_min - s_ink_val, DEG_SIZE,
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
    draw_clock_num(ctx, buf, clock_b_hour());
    draw_clock_num(ctx, mmbuf, clock_b_min());
  }

  // the value slot: steps, or last night's sleep until you are up. Stacked it
  // opens the header row at the left margin and the pulse closes it at the
  // right; on one line the two stack at the right margin instead. Either way
  // the pulse carries its label inboard of the value and the step count
  // carries none — an accent-colored number with a comma in it needs no
  // telling, and the two labels will not fit on one row with the two values.
  int b_val = BASE_ROW1 - (cards ? step_dy() : 0);
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

  // Cards spends the top right on the day's distance. It used to stand in for
  // the pulse when there was no reading, which meant it appeared and vanished
  // according to a sensor it has nothing to do with; here it is simply always
  // there, and the pulse keeps its own place down the column.
  if (cards) {
    fmt1(buf, sizeof buf, use_miles() ? hl_walked_m() / 1609.344
                                      : hl_walked_m() / 1000.0);
    const char *u = use_miles() ? "MI" : "KM";
    graphics_context_set_text_color(ctx, p->muted);
    int vw = draw_right(ctx, buf, F_VAL, MARGIN_R, b_val);
    graphics_context_set_text_color(ctx, p->label);
    draw_tracked(ctx, u, F_CAPS_S,
                 MARGIN_R - vw - 8 - tracked_w(u, F_CAPS_S), b_val);
    return;
  }
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

// The info as a card, and the rule that bounds it.
//
// The card is an L: a strip along the top and a column down the right, joined
// at an elbow. Three of its corners are rounded and they do not all round the
// same way, which is the whole character of it —
//
//   - the elbow, where the strip meets the column, is convex toward the clock:
//     the rule turns through it as it did when this was a plain bracket
//   - the strip's free end curves *up* into the left bezel, so the card lifts
//     away from the screen edge once it is past the step count
//   - the column's free end curves *right* into the horizon, so it lifts away
//     again once it is past the temperature
//
// An L with a concave elbow is not something Pebble's rounded-rect primitives
// can express, so it is built by hand: fill the region, then cut each corner
// with a square of the opposite tone and put a disc back. Six fills and three
// arcs, which is cheaper than it sounds and exact at any radius.
//
// On one line there is no column to divide, so the card is the strip alone.
static void fill_corner(GContext *ctx, int cx, int cy, int qx, int qy,
                        GColor cut, GColor keep) {
  // Round one corner: paint the quadrant in `cut`, then a disc of `keep`.
  graphics_context_set_fill_color(ctx, cut);
  graphics_fill_rect(ctx, GRect(qx, qy, SEP_R, SEP_R), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, keep);
  graphics_fill_circle(ctx, GPoint(cx, cy), SEP_R);
}

// Cards draws two panels instead of one L, which is a much easier shape: both
// are convex, both run off a bezel, and only the corners that face the clock
// need rounding. The header keeps the top, the column keeps the right, and the
// clock has the corner between them to itself.
//
static void draw_field_cards(GContext *ctx) {
  const Palette *p = palette();
  int hz = horizon_y();
  int dx = side_dx(), dy = step_dy();
  int top = side_top();
  int hy = SEP_Y - dy;          // the header's bottom edge, wherever it is now

  if (!gcolor_equal(p->info_bg, p->sky)) {
    graphics_context_set_fill_color(ctx, p->info_bg);
    graphics_fill_rect(ctx, GRect(0, -dy, SCREEN_W, SEP_Y), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(SEP_X + dx, top, SCREEN_W - SEP_X, hz - top),
                       SEP_R, GCornerTopLeft | GCornerBottomLeft);
    // both ends of the header lift off their bezel
    fill_corner(ctx, SEP_R, hy - SEP_R, 0, hy - SEP_R, p->sky, p->info_bg);
    fill_corner(ctx, SCREEN_W - 1 - SEP_R, hy - SEP_R, SCREEN_W - SEP_R,
                hy - SEP_R, p->sky, p->info_bg);
  }

  if (!g_cfg.show_sep) return;
  graphics_context_set_stroke_color(ctx, p->sep);
  graphics_context_set_stroke_width(ctx, 1);

  // The header curves away from both bezels rather than running into them, so
  // the only rule that crosses the whole screen is the horizon — which is the
  // one line on this face that is supposed to.
  graphics_draw_line(ctx, GPoint(SEP_R, hy), GPoint(SCREEN_W - 1 - SEP_R, hy));
  graphics_draw_arc(ctx, GRect(0, hy - 2 * SEP_R, 2 * SEP_R, 2 * SEP_R),
                    GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2,
                    TRIG_MAX_ANGLE * 3 / 4);
  graphics_draw_arc(ctx, GRect(SCREEN_W - 1 - 2 * SEP_R, hy - 2 * SEP_R,
                               2 * SEP_R, 2 * SEP_R),
                    GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 4,
                    TRIG_MAX_ANGLE / 2);

  // the column: in from the right bezel, round down, and along to the horizon
  if (SEP_X + dx + SEP_R < SCREEN_W - 1)
    graphics_draw_line(ctx, GPoint(SEP_X + dx + SEP_R, top),
                       GPoint(SCREEN_W - 1, top));
  graphics_draw_arc(ctx, GRect(SEP_X + dx, top, 2 * SEP_R, 2 * SEP_R),
                    GOvalScaleModeFitCircle, TRIG_MAX_ANGLE * 3 / 4,
                    TRIG_MAX_ANGLE);
  graphics_draw_line(ctx, GPoint(SEP_X + dx, top + SEP_R),
                     GPoint(SEP_X + dx, hz - SEP_R));
  graphics_draw_arc(ctx, GRect(SEP_X + dx, hz - 2 * SEP_R, 2 * SEP_R, 2 * SEP_R),
                    GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2,
                    TRIG_MAX_ANGLE * 3 / 4);
}

static void draw_field(GContext *ctx) {
  const Palette *p = palette();
  if (g_cfg.layout == LAY_CARDS) { draw_field_cards(ctx); return; }
  bool line = g_cfg.layout == LAY_LINE;
  int top = line ? SEP_Y_LINE : SEP_Y;
  int hz = horizon_y();
  bool tinted = !gcolor_equal(p->info_bg, p->sky);

  if (tinted) {
    graphics_context_set_fill_color(ctx, p->info_bg);
    graphics_fill_rect(ctx, GRect(0, 0, SCREEN_W, top), 0, GCornerNone);
    if (!line)
      graphics_fill_rect(ctx, GRect(SEP_X, top, SCREEN_W - SEP_X, hz - top), 0,
                         GCornerNone);
    // the strip lifts off the left bezel
    fill_corner(ctx, SEP_R, top - SEP_R, 0, top - SEP_R, p->sky, p->info_bg);
    if (!line) {
      // the elbow turns toward the clock
      fill_corner(ctx, SEP_X - SEP_R, top + SEP_R, SEP_X - SEP_R, top,
                  p->info_bg, p->sky);
      // and the column lifts off the horizon
      fill_corner(ctx, SEP_X + SEP_R, hz - SEP_R, SEP_X, hz - SEP_R,
                  p->sky, p->info_bg);
    }
  }

  if (!g_cfg.show_sep) return;
  graphics_context_set_stroke_color(ctx, p->sep);
  graphics_context_set_stroke_width(ctx, 1);
  int right = line ? SCREEN_W : SEP_X;

  graphics_draw_line(ctx, GPoint(SEP_R, top), GPoint(right - SEP_R, top));
  graphics_draw_arc(ctx, GRect(0, top - 2 * SEP_R, 2 * SEP_R, 2 * SEP_R),
                    GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2,
                    TRIG_MAX_ANGLE * 3 / 4);
  if (line) return;

  graphics_draw_arc(ctx, GRect(SEP_X - 2 * SEP_R, top, 2 * SEP_R, 2 * SEP_R),
                    GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE / 4);
  graphics_draw_line(ctx, GPoint(SEP_X, top + SEP_R),
                    GPoint(SEP_X, hz - SEP_R));
  graphics_draw_arc(ctx, GRect(SEP_X, hz - 2 * SEP_R, 2 * SEP_R, 2 * SEP_R),
                    GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2,
                    TRIG_MAX_ANGLE * 3 / 4);
}

// The card's free end lands on the horizon and stops, because the horizon
// closes it: two lines meeting is one edge, and drawing both would thicken it.
// Painted the same color as the sky the horizon closes nothing, and the card
// hangs open under the temperature — so the rule runs on to the bezel itself.
//
// This has to happen after the ground. draw_ground fills the horizon band
// across the full width, so anything drawn at that row beforehand is painted
// over in the exact case this exists to handle.
static void draw_field_close(GContext *ctx) {
  const Palette *p = palette();
  if (!g_cfg.show_sep || g_cfg.layout == LAY_LINE) return;
  if (!gcolor_equal(p->horizon, p->sky)) return;
  int x = SEP_X + SEP_R + (g_cfg.layout == LAY_CARDS ? side_dx() : 0);
  if (x >= SCREEN_W - 1) return;
  graphics_context_set_stroke_color(ctx, p->sep);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(x, horizon_y()),
                     GPoint(SCREEN_W - 1, horizon_y()));
}

static void draw(Layer *layer, GContext *ctx) {
  const Palette *p = palette();
  // Blocky Digits is the only face that can take the freed width crisply, and
  // its metrics are arithmetic, so they are settled here each frame rather
  // than in clock_resolve.
  if (s_clock_scale)
    grid_metrics(clock_big() ? CARDS_BIG_SCALE : (int)grid()->light);
  graphics_context_set_fill_color(ctx, p->sky);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  draw_field(ctx);
  draw_sky(ctx);
  draw_ground(ctx);
  draw_field_close(ctx);
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
  reveal_sync();
  health_init();
}

void face_deinit(void) {
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  if (cards_hiding()) accel_tap_service_unsubscribe();
  health_deinit();
  fonts_unload();
  window_destroy(s_win);
}
