#pragma once
#include <pebble.h>

// Emberline.
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
// the numerals is "THU" rather than "8,842". The single line buys back the
// colon, which sits dead center.
//
// Two alignment axes and nothing centered: everything starts at MARGIN_L or
// ends at MARGIN_R. Only the horizon and the ground run to the bezel.
//
// ---------------------------------------------------------------------------
// Two screens, and the grid is derived for each rather than scaled from the
// other. The binding constraint differs: on Emery the clock runs out of
// *height*, on the 144x168 watches it runs out of *width*, because the day
// column and the terrain cannot shrink past legibility.
//
// The terrain is what fixes the margins. Sixty columns — one a minute, and
// that is the whole idea — must span margin to margin exactly. Emery takes 3px
// each for 180; 144 takes 2px for 120, which is the only other pitch that
// divides cleanly. Anything else and the columns beat against each other:
// every third bar comes out a pixel fatter and the ridge visibly ripples.
// ---------------------------------------------------------------------------
#if defined(PBL_PLATFORM_EMERY)

#define SCREEN_W      200
#define SCREEN_H      228
#define MARGIN_L      10
#define MARGIN_R      189           // last pixel of the optical right edge
#define BASE_ROW1     24
#define BASE_ROW2     52
#define PLOT_X        10
#define COL_W         3
#define PLOT_BOT      227
#define TRACK_CAPS    2             // only the caps are tracked
#define LABEL_DROP    15            // number baseline down to the caption under it
#define LABEL_RISE    21            // caption baseline up to the number under it
#define INK_VAL       16            // ink height of a numeral at F_VAL
#define INK_CAPS_S    8             // ...and of a cap at F_CAPS_S
#define DEG_SIZE      5             // the drawn degree ring
#define DEG_SIZE_S    4
// The rule around the clock's field: along under the header, then down between
// the clock and the day column, turning through a rounded corner. On one line
// there is no column to divide, so only the horizontal run is drawn.
#define SEP_Y         33
#define SEP_X         152
#define SEP_R         8
#define SEP_Y_LINE    62
// Cards with the panels parked: the clock has the whole screen, so Blocky
// Digits steps up a whole scale. x6 is cap 78 against the 76 the shown
// baselines allow, which is why the baselines move too — the hour comes up to
// clear the top edge and the minutes stay on the horizon. x7 would be cap 91,
// and two of those leave no gap between the rows at all.
#define CARDS_BIG_SCALE 6
#define CARDS_BIG_HOUR  88
#define CARDS_BIG_MIN   182

#else   // basalt, diorite, flint — 144x168

#define SCREEN_W      144
#define SCREEN_H      168
#define MARGIN_L      12
#define MARGIN_R      131
#define BASE_ROW1     18
#define BASE_ROW2     38
#define PLOT_X        12
#define COL_W         2
#define PLOT_BOT      167
#define TRACK_CAPS    1
#define LABEL_DROP    11
#define LABEL_RISE    16
#define INK_VAL       13
#define INK_CAPS_S    7
#define DEG_SIZE      4
#define DEG_SIZE_S    3
#define SEP_Y         25
#define SEP_X         99
#define SEP_R         6
#define SEP_Y_LINE    46
#define CARDS_BIG_SCALE 4
#define CARDS_BIG_HOUR  60
#define CARDS_BIG_MIN   122

#endif

#define COLS          60           // one a minute, on every screen
#define HORIZON_H     2
#define STEP_CAP      90           // steps/minute that fills the terrain
#define WALK_RATE     60           // the pace that counts as walking

// The day column reads THU / 30 / JUL. Two gaps rather than one because a cap
// is fewer rows of ink than a numeral: measured from the baselines, the same
// optical gap is a different number above and below.
//
// Ink heights are measured off the device rather than derived: Pebble reports
// a font's box height, which carries leading above the cap and is not the ink.
// The block runs from the weekday's ink top to the month's ink bottom.
#define DATE_BLOCK_H  (LABEL_RISE + LABEL_DROP + INK_CAPS_S)

void fmt1(char *buf, size_t cap, double v);            // "12.3"

void face_init(void);
void face_deinit(void);
void face_poke(void);
void face_set_temp(int temp);
void face_fonts_changed(void);   // the clock face or weight was reconfigured
