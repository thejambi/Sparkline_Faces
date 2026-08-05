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
// Clock faces. Proportional ones are drawn into fixed slots to make them
// tabular; DSEG and the drawn face already are. Either way digits cannot
// shuffle.
//
// Append-only, like every other persisted enum here: a saved 2 has to keep
// meaning Roboto.
// CF_ROBOTO, CF_GROTESK, CF_SOURCE, CF_PLEX, CF_MONO and now CF_LECO were
// dropped after wrist testing. Their slots stay: a watch on 1.1.0 has Roboto
// persisted as 2, and renumbering would hand it somebody else's font. Their
// grid rows are zeroed and fall back to Montserrat.
// CF_GRID is "Blocky Digits" on the config page. It is not a font at all: it
// is a 10x13 grid scaled by whole numbers and drawn as rectangles, which is
// why it has one weight and ignores the bold toggle. See src/c/digits.h, and
// tools/digitgrid.py for the drawing itself.
enum { CF_MONT, CF_LECO, CF_ROBOTO, CF_GROTESK, CF_INTER, CF_SOURCE, CF_PLEX,
       CF_DSEG, CF_MONO, CF_KODE, CF_MRTN, CF_JRSY, CF_GRID, CF_COUNT };
// Three ways to set the same information. Stacked buys a much larger numeral;
// the single line buys back the colon and a calmer header. Cards moves the
// pulse into the right column with its caption underneath, which empties the
// header down to the step count and lets the two survivors be drawn as two
// separate cards rather than one continuous L.
//
// Append-only with the rest: LAY_CARDS goes last.
enum { LAY_STACK, LAY_LINE, LAY_CARDS, LAY_COUNT };
// The face used for everything that is not the clock. Append-only.
enum { TF_MONT, TF_INTER, TF_SOURCE, TF_SYSTEM, TF_JRSY, TF_DSEG,
       TF_COUNT };

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
  // Dead since the bold toggle was removed: the clock is always the bold cut.
  // The field stays because this struct is persisted whole and removing a byte
  // from the middle would reinterpret everything after it on every saved watch.
  bool bold_clock;
  uint8_t layout;
  // Roles that used to be shared. COL_INHERIT means "still shared", which is
  // what a save from before these existed reads as — so updating cannot move a
  // colour under anyone. Clay's picker is 24-bit and can never send it.
  uint32_t c_terrain, c_now, c_sleep;
  uint8_t text_font;
  uint32_t c_unlit;              // DSEG's dark segments; COL_INHERIT = the sky
  bool show_sep;                 // the rule around the clock's field
  uint32_t c_label, c_sep, c_info_bg;
} Settings;

#define COL_INHERIT 0xFF000000u

extern Settings g_cfg;

// What the face actually draws with.
//
// In the presets `terrain` follows the accent and `now` follows the ink,
// because movement is one idea and time is another and the palette should not
// let them drift apart. Custom is allowed to break that — it is the one place
// where breaking it is the point — so those two, and the sleep tint, can be
// set on their own there.
typedef struct {
  GColor sky, ground, horizon, ink, accent, muted, scale;
  GColor terrain;                // the bars; accent unless Custom says otherwise
  GColor now;                    // the newest column; ink unless overridden
  GColor sleep;                  // the whole sleep state; muted unless overridden
  // The segments a seven-segment display is *not* lighting. Only DSEG draws
  // them, and only where the palette has somewhere to put them: it has to sit
  // between the sky and the ink, and on a lit sky the Pebble 64 has nothing
  // there. Where there is nothing, this is the sky and the ghosts vanish.
  GColor unlit;
  // The labels in the sky used to share the chart's tone. They are different
  // jobs — one names a value you are reading, the other is the ruling behind a
  // plot — so they are separate roles even though the presets still tie them.
  GColor label;
  GColor sep;                    // the rule between the clock and the rest
  GColor info_bg;                // behind everything that is not the clock
} Palette;

const Palette *palette(void);

void settings_init(void (*cb)(void));
