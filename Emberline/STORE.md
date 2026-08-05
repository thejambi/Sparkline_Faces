# Store listing

How to publish Emberline, and the things about publishing it that are easy to
get wrong.

**The listing body is not here.** It lives in `store/description.txt`, which is
what the publish command actually pipes in. This file used to carry a second
copy for reference, and the two drifted until the copy here was advertising
Roboto, LECO, a bold toggle and a 500-step default that the build had not had
for months. One home for the words, and it is the one the tool reads.

**Title** — Emberline

**Tagline** (one line, shown under the title)

> Sky over ground, and the last hour of your movement along the bottom edge.

## Notes for the listing

- **emery, basalt, diorite, flint** — Time 2, Time, Time Steel, Pebble 2,
  Pebble 2 Duo. Not aplite, which has no Health API. Round screens (chalk,
  gabbro) are not done yet.
- Weather comes from Open-Meteo — no account, no API key. It uses the phone's
  location, or a city or postal code you type in, in which case it never
  touches phone location at all.
- Bundled fonts are Montserrat, Inter and DSEG, all under the SIL Open Font
  License. License texts and the full table ship in `resources/fonts/`.
- Custom colors expose seven roles in Simple and fourteen in Advanced. Every
  preset value is already one of the watch's 64 colors, so nothing is
  quantized out from under the design.

## Publishing

Assets live in `store/`, matching Solfarer and Lighthaul. `tools/make_icon.py`
draws the icons, `tools/make_banner.py` the banner; the screenshots are single
frames from the emulator sweep. Screenshot filenames must start with the
platform name — the tool keys off that to know where a shot belongs.

The sheets in `preview/` are working documents for arguing about a design, not
upload candidates. Only `store/screenshots/` goes to the store.

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
| `screenshots/emery_6_hidden.png` | the panels parked, the clock grown into the space |
| `screenshots/basalt_{1,2}_*.png` | the same two on a 144x168 color screen |
| `screenshots/diorite_{1,2}_*.png`, `flint_{1,2}_*.png` | and in black and white |

**The shots carry demo data, and it is patched in rather than real.** The
emulator has no health history and its weather is live, so a straight capture
is an empty face at whatever temperature the sky happens to be. The sweep
patches steps, pulse, distance and terrain into `health.c`, pins the
temperature and the clock, and restores every source on exit.

Two things that cost a whole pass: `emu-set-time` takes its time as a
*positional* `HH:MM:SS`, not `--time` with an ISO stamp, and fails silently
otherwise — which is how the first sweep came back with twelve different clock
times. And the walked distance has to be derived from the step count, or the
sleep shot advertises seven and a half hours of sleep beside four miles walked.

**`--screenshots` appends, it does not replace.** Every publish that passes
them adds another full set to the listing, and the old ones have to be deleted
by hand in the dashboard. So pass them only when a shot has actually changed —
a release that only touches code or copy should leave the flag off entirely.

Bump `version` in `package.json` first, and write real release notes: the store
shows them, and "First release." has been sitting in this file as the example
long enough to be a trap.

Run from this directory, with the `.pbw` freshly built and the repo pushed
(the source URL is part of the listing):

```
pebble publish --non-interactive --no-gif-all-platforms --is-published \
  --source "https://github.com/thejambi/Sparkline_Faces" \
  --description "$(cat store/description.txt)" \
  --icon-small store/icon_small_48.png \
  --icon-large store/icon_large_144.png \
  --release-notes "What changed in this version."
```

Add `--screenshots store/screenshots/emery_1_dusk.png ...` only when the shots
themselves have changed — see the warning above.

`--category` is deliberately absent. Solfarer_App passed `tools`, but that is a
watchapp category and this declares `watchapp.watchface = true`, so the store
files it as a watchface on its own. If the API turns out to want one, it will
say so plainly rather than mis-file the listing.

`--no-gif-all-platforms` skips the rollover-GIF capture, which boots an
emulator per platform — four of them now. The static shots already show every
state worth showing, and the sweep is slow enough to be worth not repeating.
