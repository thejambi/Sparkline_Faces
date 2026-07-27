# TeaScene

Solfarer's info band over a Chinese tea room.

The top 56 rows are the "main watchface info" carried over unchanged from
[Solfarer](../../Solfarer): the clock in LECO 36, the health corner grid
(steps / distance / heart rate) facing the day column (weekday / date /
temperature) across a hairline, and the past-hour activity sparkline. A shake
swaps the heart-rate row for hours slept for six seconds.

Everything below that is art: a gongfu tea table in a Jiangnan studio, drawn
once and recoloured four times a day.

**Emery only.** Every layout number here is chosen for a 200x228 colour
screen. Other platforms get their own pass if an idea is worth publishing.

## The art

`tools/make_scene.py` builds a 200x172 index map and four palettes, then
writes:

- `resources/data/scene.bin` — the index map, hybrid-RLE per row
- `src/c/scene_data.h` — the four palettes as Pebble `GColor8` bytes
- `preview/*.png` — 3x renders, whole-watch mocks, and a 1x sheet

Run it after any change to the art:

```bash
python3 tools/make_scene.py
```

The scene lives in flash and is decoded a row at a time straight into the
framebuffer (`src/c/scene.c`), so the whole watchface costs about 9KB of RAM.
Holding it as a `GBitmap` would have cost 34KB of the 128KB heap for nothing.

### Working within 64 colours

The Emery panel has two bits per channel — every colour is some combination of
0, 85, 170 and 255. There is no such thing as a subtle gradient. Two rules the
art follows, both learned by getting them wrong first:

- **Flat colour, hard edges.** Dither is only ever a one- or two-pixel seam
  between two flat bands. A falloff spread over an area comes out as a field
  of shimmering noise, not as light. The shaft of sun on the tray is a hard-
  edged quad for exactly this reason.
- **Value carries the picture.** Dark framing, pale plaster, mid-tone tray,
  light porcelain. The four palettes shift hue a long way but keep that
  ladder, so the tea set reads at a glance in every one of them — and at
  night, when the lamp is over the tray, the ladder inverts on purpose.

### Times of day

`SCENE_AUTO` follows the clock: morning from 05:00, afternoon from 11:00,
evening from 17:00, night from 21:00. Any one of them can be pinned in
settings.

## Health

The sparkline uses the same fetch discipline as Solfarer: the minute history
is read once at boot, then exactly once more at the first quarter-hour mark
(minute % 15 == 1), which is when the watch has filled in the hour it was not
running for. After that the bar is kept live from the step delta and never
refetches.

## Build

```bash
pebble build && pebble install --emulator emery
```
