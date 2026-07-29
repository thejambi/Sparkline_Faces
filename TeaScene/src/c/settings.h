#pragma once
#include <pebble.h>

enum { DATE_DAYNUM, DATE_MONTHDAY, DATE_OFF };  // "FRI 24" / "JUL 24"
enum { DIST_AUTO, DIST_KM, DIST_MI };
enum { SCENE_AUTO, SCENE_MORNING, SCENE_AFTERNOON,      // which colouring
       SCENE_EVENING, SCENE_NIGHT };

// Persisted whole. APPEND-ONLY: new fields go at the end; older saves stop
// short and keep defaults. Bump SETTINGS_VERSION only on reorder.
#define SETTINGS_VERSION 1
typedef struct {
  uint8_t version;
  uint8_t show_health;           // health corner + sparkline
  uint8_t date_format;
  uint8_t scene_time;            // auto, or a colouring pinned by hand
  bool leading_zero, show_battery, show_bt;
  bool bt_vibe, tap_info;
  bool weather_on;
  uint8_t dist_unit;             // km, miles, or whatever the watch says
} Settings;

extern Settings g_cfg;

void settings_init(void (*cb)(void));
