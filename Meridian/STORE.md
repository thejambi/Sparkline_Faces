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

Assets live in `publish/`, built by `tools/make_icon.py` and copied from the
emulator sweep. Screenshot filenames must start with the platform name — the
tool keys off that to know which platform a shot belongs to.

| File | What |
| --- | --- |
| `icon_small.png` | 48x48, Dusk sky |
| `icon_large.png` | 144x144, Dusk sky |
| `emery_01_dusk.png` | the default, stacked |
| `emery_02_sleep.png` | last night's sleep, and the night as terrain |
| `emery_03_one_line.png` | the other layout |
| `emery_04_phosphor.png` | theming |
| `emery_05_paper.png` | the light theme |

Run from this directory, with the `.pbw` freshly built:

```
pebble publish --release-notes "First release."
```

It uploads to `appstore-api.repebble.com` and prompts for the listing fields
on a first publish — paste them from the top of this file.

**`--is-published` is deliberately not passed.** Without it the release is
created but not made visible, so the listing can be read once before anyone
else sees it. Flip it live from the web UI, or re-run with the flag.

If the emulator wedges during rollover-GIF capture, add
`--no-gif-all-platforms` and upload the static shots instead.
