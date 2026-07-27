#pragma once
#include <pebble.h>

// The tea scene: one 200x172 index map in flash, four palettes in the
// binary. Decoded straight into the framebuffer a row at a time, so the art
// costs a few hundred bytes of RAM rather than 34KB.
void scene_init(void);
void scene_draw(GContext *ctx, int top, int variant);
int scene_variant_for_hour(int hour);      // 0 morning .. 3 night
