#pragma once
#include <pebble.h>

// Emery only, for now. Every layout number in this face is chosen for a
// 200x228 colour screen; other platforms get their own pass when an idea is
// ready to publish.
#define TOP_H 56              // the info band: clock, health, date, sparkline

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
