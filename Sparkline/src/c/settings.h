#pragma once
#include <pebble.h>

enum { DATE_DAYNUM, DATE_MONTHDAY, DATE_OFF };          // "FRI 24" / "JUL 24"

// Persisted whole. APPEND-ONLY: new fields go at the end; older saves stop
// short and keep defaults. Bump SETTINGS_VERSION only on reorder.
#define SETTINGS_VERSION 1
typedef struct {
  uint8_t version;
  uint8_t show_health;           // the health column and the sparkline
  uint8_t date_format;
  bool leading_zero, show_battery, show_bt;
  bool bt_vibe, tap_info;
  bool weather_on;
} Settings;

extern Settings g_cfg;

void settings_init(void (*cb)(void));
