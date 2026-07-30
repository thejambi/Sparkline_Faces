#pragma once
#include <pebble.h>

// Meridian — Emery, 200x228.
//
// Sky over ground, one warm line where they meet. Three meanings rather than
// five colours: the ink is time, the accent is movement, the muted tone is
// context.
//
//     22   WED 29                    76°    tracked caps
//     94   9                      8,842     hours share a baseline with steps
//    156   41                    BPM 68     minutes share one with the pulse
//    162   ────────────────────────────     the horizon, 2px
//    164   ground
//    227   the terrain stands on the last row of the screen
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

#define BASE_DATE     22            // baselines, never box tops
#define BASE_HOUR     94
#define BASE_MIN      156

#define HORIZON_Y     162
#define HORIZON_H     2
#define GROUND_Y      (HORIZON_Y + HORIZON_H)

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
