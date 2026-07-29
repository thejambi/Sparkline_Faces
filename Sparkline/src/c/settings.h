#pragma once
#include <pebble.h>

enum { DATE_DAYNUM, DATE_MONTHDAY, DATE_OFF };          // "FRI 24" / "JUL 24"

// Clock typeface. The first four match ActiveHour's picker; the rest are the
// remaining system faces big enough to carry a clock. Sizes differ a lot
// between families, so each one carries its own vertical nudge in face.c.
enum { CF_LECO, CF_MONT, CF_ROBOTO, CF_BITHAM,
       CF_ROBOTO_SYS, CF_DROID, CF_GOTHIC, CF_COUNT };

enum { TH_CLASSIC, TH_MONO, TH_AMBER, TH_ICE, TH_PAPER, TH_CUSTOM };

// Persisted whole. APPEND-ONLY: new fields go at the end; older saves stop
// short and keep defaults, which is why SETTINGS_VERSION does not move when
// options are added. Bump it only on a reorder.
#define SETTINGS_VERSION 1
typedef struct {
  uint8_t version;
  uint8_t show_health;           // the health column and the sparkline
  uint8_t date_format;
  bool leading_zero, show_battery, show_bt;
  bool bt_vibe, tap_info;
  bool weather_on;
  uint8_t clock_font;
  uint8_t theme;
  bool bold_font;
  // Custom theme, packed 0xRRGGBB. Only read when theme == TH_CUSTOM.
  uint32_t c_bg, c_time, c_health, c_date, c_muted, c_lines, c_spark;
} Settings;

extern Settings g_cfg;

// The colours the face actually draws with, resolved from theme + custom.
typedef struct {
  GColor bg, time, health, date, muted, lines, spark;
} Palette;

const Palette *palette(void);

void settings_init(void (*cb)(void));
