#pragma once
#include <pebble.h>

// Meridian — Emery, 200x228.
//
// Sky over ground, one warm line where they meet. Three meanings rather than
// five colours: the ink is time, the accent is movement, the muted tone is
// context.
//
// Stacked (the default):                One line:
//     22  WED 29            76°           24  WED 29        8,842
//     94  9              8,842            52  76°          BPM 68
//    156  41            BPM 68           130       9:41
//    162  ──────────────────────         156  ──────────────────────
//         ground, then terrain                ground, then terrain
//
// Stacked buys a much larger numeral — Roboto reaches 88 there against 68 on
// one line — and pays for it with a shorter terrain. The single line buys
// back the colon, which sits dead centre, and groups the day's values on the
// left against the body's on the right.
//
// Two alignment axes and nothing centred: everything starts at x=10 or ends
// at x=189. Only the horizon and the ground run to the bezel. The clock is
// stacked because that is the only way to get a 55px numeral onto a 200px
// screen — set on one line it has to drop to 44px, and the face stops being
// about the time.

#define SCREEN_W      200
#define SCREEN_H      228
#define MARGIN_L      10
#define MARGIN_R      189           // last pixel of the optical right edge

// Baselines, never box tops. The clock rows and the horizon move with the
// chosen face and layout — a 60px LECO and an 88px Roboto cannot share a
// grid — so those live in the metrics table in face.c. These two do not move.
#define BASE_DATE     22            // stacked layout: the single header row
#define BASE_ROW1     24            // one-line layout: the two header rows
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

void fmt1(char *buf, size_t cap, double v);            // "12.3"

void face_init(void);
void face_deinit(void);
void face_poke(void);
void face_set_temp(int temp);
void face_fonts_changed(void);   // the clock face or weight was reconfigured
