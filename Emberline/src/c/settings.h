#pragma once
#include <pebble.h>

enum { DATE_DAYNUM, DATE_MONTHDAY, DATE_OFF };  // "WED 29" / "JUL 29"
enum { DIST_AUTO, DIST_KM, DIST_MI };
// Theme values are persisted and are sent by the config page as string ints,
// so they are append-only too: TH_PHOSPHOR goes after TH_CUSTOM rather than
// beside its siblings, because inserting it anywhere earlier would silently
// turn a saved Custom into a preset. The config page lists it first regardless.
enum { TH_DUSK, TH_NOIR, TH_PAPER, TH_MOSS, TH_TIDE, TH_CUSTOM, TH_PHOSPHOR,
       TH_COUNT };
// Montserrat is proportional, so the clock is drawn into fixed slots to make
// it tabular; LECO already is. Either way the digits cannot shuffle.
enum { CF_MONT, CF_LECO, CF_ROBOTO, CF_COUNT };
// Two ways to set the same information. Stacked buys a much larger numeral;
// the single line buys back the colon and a calmer header.
enum { LAY_STACK, LAY_LINE, LAY_COUNT };

// Persisted whole. APPEND-ONLY: new fields go at the end; older saves stop
// short and keep defaults. Bump SETTINGS_VERSION on a reorder, or to force a
// saved blob to be discarded so that changed defaults are actually seen —
// which is the only way a new default reaches a watch that has ever saved.
// 2: the Phosphor default, and the stacked/Montserrat/bold clock it goes with.
#define SETTINGS_VERSION 2
typedef struct {
  uint8_t version;
  uint8_t theme;
  uint8_t date_format;
  uint8_t dist_unit;
  bool leading_zero, show_bpm, show_battery, bt_vibe;
  bool show_sleep;               // sleep holds the value slot before you rise
  bool sleep_terrain;            // ...and the terrain shows last night
  bool weather_on;
  uint16_t wake_threshold;
  // Custom theme, packed 0xRRGGBB. Only read when theme == TH_CUSTOM.
  uint32_t c_sky, c_ground, c_horizon, c_ink, c_accent, c_muted, c_scale;
  uint8_t clock_font;
  bool bold_clock;
  uint8_t layout;
} Settings;

extern Settings g_cfg;

// What the face actually draws with. `bar` follows the accent and `now`
// follows the ink, because movement is one idea and time is another — the
// palette should not let them drift apart.
typedef struct {
  GColor sky, ground, horizon, ink, accent, muted, scale;
} Palette;

const Palette *palette(void);

void settings_init(void (*cb)(void));
