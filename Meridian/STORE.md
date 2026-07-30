# Store listing

Copy for the appstore entry. Kept here so it is versioned with the face it
describes — a listing that drifts from the build is worse than no listing.

**Title** — Meridian

**Tagline** (one line, shown under the title)

> Sky over ground, and the last hour of your movement along the bottom edge.

## Description

Meridian draws your hour as landscape.

The bottom of the screen is the last sixty minutes — one column a minute,
standing on the very bottom edge, with a quarter-hour graticule behind it and
a dotted line at walking pace. The newest minute is drawn in the clock's own
color, so *now* is always findable. Above it is the clock, as large as a
200-pixel screen will allow. Between them is one warm line: the horizon.

**Before you get up, it shows you the night instead.** Last night's sleep
holds the step slot until you have actually moved — 500 steps by default, and
adjustable. While it does, the chart's window becomes the night itself: sixty
columns from the moment you fell asleep to the moment you woke, each one how
much you moved, scaled so the most restless stretch fills the plot. The whole
face runs cool until you cross the threshold, then warms the moment steps take
the slot back.

Also on screen: the date, the temperature, your heart rate — or the day's
distance when there is no reading — and a two-pixel battery bar along the top
edge.

### Two layouts

- **Stacked** puts the hours over the minutes at 88px, the largest numeral
  that fits, with the health values along the top and the date set as a
  narrow column down the right margin.
- **One line** brings back the colon, pinned to the dead center of the
  screen, and groups the day's values against your body's.

### Six themes, plus your own

Dusk, Phosphor, Noir, Paper, Moss and Tide. Each is built the same way — a lit
sky over land in shadow, and exactly one bright color, spent only on the
horizon, the step count and the terrain. Phosphor is the exception: no sky at
all, the time in orange and everything your body reports in green.

Custom exposes all seven color roles. Every preset value is already one of
the watch's 64 colors, so nothing is quantized out from under the design.

### Type

Montserrat, Roboto or LECO for the clock, each with a bold option. All three
are drawn one digit at a time into fixed-width slots, so the numerals never
shuffle sideways as the minutes change.

## Notes for the listing

- **Emery only** (Pebble Time 2). The layout is built around 200x228.
- Weather comes from Open-Meteo — no account, no API key. It uses the phone's
  location, or a city or postal code you type in, in which case it never
  touches phone location at all.
- Bundled fonts: Montserrat (SIL Open Font License) and Roboto (Apache 2.0).
  License texts ship in `resources/fonts/`.

## Screenshots to upload

From `preview/`, in this order — all 200x228, native:

1. `face.png` — Dusk, stacked, Montserrat bold
2. `themes.png` — the six themes
3. `layouts.png` — stacked against one line
4. `states.png` — steps, sleep, 24-hour
5. `clockfonts.png` — Montserrat, Roboto, LECO

The sheets are multi-panel; upload the individual frames if the store wants
one watch per shot.

## Publishing

Assets live in `store/`, matching Solfarer and Lighthaul. `tools/make_icon.py`
draws the icons, `tools/make_banner.py` the banner; the screenshots are single
frames from the emulator sweep. Screenshot filenames must start with the
platform name — the tool keys off that to know where a shot belongs.

| File | What |
| --- | --- |
| `description.txt` | the listing body, piped in by the command below |
| `icon_small_48.png`, `icon_large_144.png` | Dusk sky, drawn at size |
| `banner_720x320.png` | uploaded through the web UI; `publish` has no flag for it |
| `screenshots/emery_1_dusk.png` | the default, stacked |
| `screenshots/emery_2_sleep.png` | last night's sleep, and the night as terrain |
| `screenshots/emery_3_one_line.png` | the other layout |
| `screenshots/emery_4_phosphor.png` | theming |
| `screenshots/emery_5_paper.png` | the light theme |

Run from this directory, with the `.pbw` freshly built and the repo pushed
(the source URL is part of the listing):

```
pebble publish --non-interactive --no-gif-all-platforms --is-published \
  --source "https://github.com/thejambi/Sparkline_Faces" \
  --description "$(cat store/description.txt)" \
  --icon-small store/icon_small_48.png \
  --icon-large store/icon_large_144.png \
  --screenshots store/screenshots/emery_1_dusk.png \
               store/screenshots/emery_2_sleep.png \
               store/screenshots/emery_3_one_line.png \
               store/screenshots/emery_4_phosphor.png \
               store/screenshots/emery_5_paper.png \
  --release-notes "First release."
```

`--category` is deliberately absent. Solfarer_App passed `tools`, but that is a
watchapp category and this declares `watchapp.watchface = true`, so the store
files it as a watchface on its own. If the API turns out to want one, it will
say so plainly rather than mis-file the listing.

`--no-gif-all-platforms` skips the rollover-GIF capture, which boots an
emulator per platform. There is only one platform here and the static shots
already show every state worth showing.
