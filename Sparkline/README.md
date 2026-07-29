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
  0   clock            the configured face, LECO 60 bold by default
 66   hairline
 70   8,842  |  WED    the step count follows the clock's family, so the two
100   6.3 km |   29    numbers that matter share a typeface; the body is
130   68 bpm |  76°    Gothic 28 on a shared baseline
161   hairline
166   sparkline        60 columns, standing on the bottom row of the screen
```

**Right-alignment is done by hand.** Measure, then draw left-aligned at
(right edge − width) in a roomy box. The renderer's own right-aligned layout
misplaces interior glyphs when the box runs near its width; digits come out
fused on top of each other. Inherited from Solfarer, still true.

One trap worth recording: the measuring box has to clear the tallest font on
the face. At 44px tall it was shorter than LECO 60, which measured wrong.

## Fonts

Seven clock faces, each of which also sets the step count. The first four are
the families ActiveHour offers; the rest are the remaining system faces big
enough to carry a clock.

| Option | Face | Notes |
| --- | --- | --- |
| LECO (60) | system | the default; tall numerals, the widest of the set |
| Montserrat (58) | bundled | Bold / Light |
| Roboto (58) | bundled | Bold / Light |
| Bitham (42) | system | Bold / Light |
| Roboto, system (49) | system | bold only; the subset face has digits and a colon and nothing else, so the step count falls back to Gothic |
| Droid Serif (28) | system | bold only; a quiet, bookish clock |
| Gothic (28) | system | Bold / Regular |

The two bundled faces are subset to `[0-9:,]` — the clock and the step count
are the only things they ever draw — which keeps all eight generated sizes
inside 19KB. Licences travel with them in `resources/fonts/`.

**The clock and the step count carry their own weights.** At a third the
size, the step count often wants bold where the clock reads better light —
LECO in particular. Two toggles, not one.

Three things the families disagree about, all handled per-family in `FONTS[]`:

- **How much of the box is ink.** LECO's digits are 43px inside a 60px box;
  Gothic's nearly fill theirs. A single shared offset cannot centre them all
  against the hairline, so each family carries its own `clock_y`, measured
  against that rule rather than guessed.
- **Whether the light face is even the same size.** LECO's light numerals are
  28 where its bold are 32, so the step count jumped when the weight changed.
  `steps_dy` is therefore per weight, not per family.
- **Whether a comma exists.** The numeric-only system faces have no thousands
  separator, and asking for one drops a blank. `FONTS[].comma` records it, and
  Bitham renders `8842` rather than a gap.

## Colours

Five presets plus Custom: Classic (green on black, the face as Solfarer left
it), Mono, Amber, Ice, and Paper — the one light theme, which exists mostly to
keep the drawing code honest about never assuming a black background.

Custom exposes seven colours: background, clock (which also draws the newest
bar), health values, day of month, weekday and temperature, rules and chart
scale, and the sparkline bars. Every preset colour is already on the Pebble 64
— each channel 00/55/AA/FF — so nothing is quantised out from under the
design.

A warning stays a warning: the low-battery and disconnected indicators are red
in every theme.

## Distance

Kilometres, miles, or Automatic — which follows the units already set in the
Pebble app, via `health_service_get_measurement_system_for_display()`. Health
reports metres either way; only the divisor and the tail change.

## Sleep

ActiveHour's rule, by way of Solfarer: last night's sleep holds the step slot
until you have actually got up and moved. Past the wake threshold — 500 steps
by default — the step count takes it back for the rest of the day. With no
sleep recorded there is nothing to hold the slot with, so steps keep it.

This replaces the shake gesture the face used to carry. A shake is a thing you
have to remember to do; a threshold just knows.

`6h 32m` needs letters, which most of these numeric faces do not have. The two
bundled ones are subset to include `h` and `m` so they can spell it in their
own type; LECO and Bitham fall back to Gothic rather than drawing the string
with two holes in it.

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

Two emulator traps, both of which cost an hour here:

- **Settings persist across installs.** The bundled JS sends a weather message
  on launch, which makes the watch write the whole settings blob. After that,
  changing a default in `defaults()` does nothing — the saved blob wins. Wipe
  between runs when testing defaults.
- **Screenshots can race the install.** `pebble install` returns before the
  face has relaunched, so a screenshot taken too soon shows the previous
  build. Kill first, and give it several seconds.
