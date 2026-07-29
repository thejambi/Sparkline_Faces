# Sparkline

Solfarer's info band, and nothing else.

The band was designed to fit into 56 of Emery's 228 rows so that something
else could have the rest — [TeaScene](../TeaScene) is what that something else
looks like. This face asks the opposite question: what does the same
information look like when it is allowed all 228 rows?

Same grammar, same palette. A LECO clock, health values right-aligned against
a vertical hairline with the day column facing them across it, gold bars for
the last hour of movement. Every element simply given the room to be read at
arm's length — and the sparkline promoted from a 10px strip to the floor of
the screen.

**Emery only.** Every layout number here is chosen for a 200x228 colour
screen.

## The layout

```
  0   clock            LECO 60 bold — as large as LECO goes
 66   hairline
 70   8,842  |  WED    steps in LECO 32, so the two numbers that matter share
100   6.3 km |   29    a typeface; everything else is Gothic 28
130   68 bpm |  76°
161   hairline
166   sparkline        60 columns, standing on the bottom row of the screen
```

Two things are worth knowing about the type:

- **The clock picks its own size.** `time_font()` measures the string and
  steps down from LECO 60 only if it will not fit. 24-hour time is a whole
  digit wider than `9:41`, and 60-bold measures 162px of the 166 available —
  so it fits, but only just.
- **Right-alignment is done by hand.** Measure, then draw left-aligned at
  (right edge − width) in a roomy box. The renderer's own right-aligned
  layout misplaces interior glyphs when the box runs near its width; digits
  come out fused on top of each other. Inherited from Solfarer, still true.

One trap worth recording: the measuring box has to clear the tallest font on
the face. At 44px tall it was shorter than LECO 60, which measured wrong.

## The sparkline

A column a minute for the past hour, oldest at the left, standing on the
bottom edge. At 61 rows it has room for a scale the 10px strip never could:

- five-minute ticks along the floor, and a full-height rule at the top of the
  hour
- a dotted line at 60 steps/minute — the pace that counts as walking
- the newest minute drawn white, so *now* is findable

Bars are contiguous. A one-pixel gap between columns turns an activity trace
into a picket fence.

The fetch discipline is Solfarer's, unchanged: the minute history is read
once at boot, then exactly once more at the first quarter-hour mark
(`minute % 15 == 1`), which is when the watch has filled in the hour it was
not running for. After that the bar is kept live from the step delta and
never refetches.

## Build

```bash
pebble build && pebble install --emulator emery
```

If the emulator comes up showing a different watchface, `pebble wipe` and
install again.
