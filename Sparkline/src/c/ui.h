#pragma once
#include <pebble.h>

// Emery only, for now. Every layout number here is chosen for a 200x228
// colour screen; other platforms get their own pass when the design settles.
//
// The face is Solfarer's info band with the walls taken away — same
// grammar, same palette, given the whole screen:
//
//   0    the clock, as large as LECO goes
//   66   a hairline
//   70   health right-aligned against a vertical rule, the day left of it
//   160  a hairline
//   164  the sparkline, standing on the bottom edge
#define TIME_TOP     (-6)     // LECO's ink is 43px inside a 60px box
#define TIME_BOX_H   72
#define RULE1_Y      66
#define GRID_Y0      70
#define GRID_ROW     30
#define SEP_X        116      // the vertical hairline the numbers hang on
#define GRID_BOT     158
#define RULE2_Y      161
#define SPARK_TOP    166
#define SPARK_BOT    227      // bars stand on the last row of the screen
#define MARGIN       4

#define COL_GOLD  GColorChromeYellow
#define COL_DIM   GColorLightGray
#define COL_FAINT GColorDarkGray
#define COL_GOOD  GColorGreen
#define COL_BAD   GColorRed

// Pebble's snprintf has no %f.
void fmt1(char *buf, size_t cap, double v);            // "12.3"

void face_init(void);
void face_deinit(void);
void face_poke(void);            // redraw
void face_set_temp(int temp);    // phone delivered a fresh Open-Meteo reading
