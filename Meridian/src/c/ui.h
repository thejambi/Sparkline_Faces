#pragma once
#include <pebble.h>

// Meridian — Emery, 200x228.
//
// Sky over ground, one warm line where they meet. Three meanings rather than
// five colors: the ink is time, the accent is movement, the muted tone is
// context.
//
// Stacked (the default):                One line:
//     24  8,842        BPM 68            24  WED 29        8,842
//     77                     THU          52  76°          BPM 68
//     98                      30         130       9:41
//    102  9                             156  ──────────────────────
//    113                     JUL              ground, then terrain
//    174  41                76°
//    180  ──────────────────────
//         ground, then terrain
//
// Both layouts keep the day's values together and the body's values together;
// they differ in which side each group takes. Stacked runs the health values
// along the top as a header and gives the right margin to a narrow day column,
// which is what lets the clock be as large as it is: the widest thing beside
// the numerals is now "WED" rather than "8,842". The single line buys back
// the colon, which sits dead center.
//
// Two alignment axes and nothing centered: everything starts at x=10 or ends
// at x=189. Only the horizon and the ground run to the bezel. The clock is
// stacked because that is the only way to get a 64px numeral onto a 200px
// screen — set on one line it has to drop to 44px, and the face stops being
// about the time.

#define SCREEN_W      200
#define SCREEN_H      228
#define MARGIN_L      10
#define MARGIN_R      189           // last pixel of the optical right edge

// Baselines, never box tops. The clock rows and the horizon move with the
// chosen face and layout — a 60px LECO and an 88px Roboto cannot share a
// grid — so those live in the metrics table in face.c. These two do not move:
// the header takes one row stacked and two on a line.
#define BASE_ROW1     24
#define BASE_ROW2     52

#define HORIZON_H     2

// 60 columns of exactly 3px. Any other pitch and the columns beat against
// each other — every third bar comes out a pixel fatter and the terrain
// visibly ripples.
#define PLOT_X        10
#define COL_W         3
#define COLS          60
#define PLOT_BOT      227
#define BAR_MAX       60
#define STEP_CAP      90            // steps/minute that fills the terrain
#define WALK_RATE     60            // the pace that counts as walking

#define TRACK_CAPS    2             // only the caps are tracked

// The day column reads THU / 30 / JUL. Two gaps rather than one because a cap
// is 8 rows of ink and a numeral is 16: measured from the baselines, the same
// optical gap is a different number above and below.
#define LABEL_DROP    15            // number baseline down to the caption under it
#define LABEL_RISE    21            // caption baseline up to the number under it

// Ink heights, measured off the device rather than derived: Pebble reports a
// font's box height, which carries leading above the cap and is not the ink.
// The block runs from the weekday's ink top to the month's ink bottom.
#define INK_VAL       16            // a numeral at F_VAL
#define DATE_BLOCK_H  (LABEL_RISE + LABEL_DROP + 8)

void fmt1(char *buf, size_t cap, double v);            // "12.3"

void face_init(void);
void face_deinit(void);
void face_poke(void);
void face_set_temp(int temp);
void face_fonts_changed(void);   // the clock face or weight was reconfigured
