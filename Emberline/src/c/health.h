#pragma once
#include <pebble.h>
#include "ui.h"

// The terrain: 60 columns of movement, either the past hour in steps per
// minute or last night's sleep in movement counts.
typedef struct {
  uint8_t col[COLS];
  uint16_t cap;          // the value that fills the plot
  bool live;             // is the last column the minute happening now
} Terrain;

void health_init(void);
void health_deinit(void);
void health_minute(struct tm *t);

int hl_steps(void);
int hl_walked_m(void);
int hl_sleep_secs(void);
int hl_bpm(void);

// Last night's sleep holds the value slot until you have actually got up and
// moved past the wake threshold.
bool hl_sleeping(void);

// Builds the terrain for whichever state the face is in. Cached: the sleep
// window costs several flash reads to assemble and does not change while you
// are looking at it.
const Terrain *hl_terrain(void);
