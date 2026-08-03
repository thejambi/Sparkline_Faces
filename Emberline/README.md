# Emberline

Sky over ground, with one line where they meet.

The terrain along the bottom is the last hour of your movement — a column a
minute, sixty of them, standing on the bottom edge of the screen. The clock is
the sky above it. The one warm line between the two is the horizon.

**Pebble Time 2, Time, Time Steel, Pebble 2 and Pebble 2 Duo** — emery,
basalt, diorite and flint. Not aplite: the Classic and the original Steel have
no Health API at all, and a face whose ground is your step count has nothing
to draw there.

## The layout

Two of them, and which one is a setting.

**Stacked** — the default. Hours over minutes, and the biggest numeral the
screen allows.

```
  0   ▔▔▔▔▔▔▔▔▔▔▔▔                      battery, 2px, top edge
 24   8,842               BPM 68        the health header
 77                          THU        the day column, whole, and
 98                           30        floating: 45 rows of clear sky
102   9                                 above it and 45 below
113                          JUL
174   41                     76°
180   ────────────────────────────      the horizon
227   ground, then the terrain
```

**One line** — the colon comes back, pinned to the center of the screen. The
two groups swap sides: the day's values on the left, the body's on the right.

```
 24   WED 29                 8,842
 52   76°                   BPM 68
130            09:41               the colon sits dead center
156   ────────────────────────────
227   ground, then the terrain
```

Both layouts keep the day's values together and the body's values together.
They differ in which margin each group takes, and in how much of the screen
that leaves for the clock: stacked buys 94px numerals and pays for them with
a shorter terrain, while one line buys back the colon and a calmer header.
Every element is the same in both.

**The stacked layout's day column is the thing that pays for its clock.** The
obvious arrangement — date and temperature along the top, health down the
right margin — puts `8,842` beside the numerals and caps them at 76px. Swap
the two and the widest thing beside the clock becomes the weekday, so long as
the date is set as a column rather than a line: `WED 29` set as one line is
73px wide, which is wider than the step count it replaced and would have made
the clock smaller, not larger. Stacked into `THU` / `30` / `JUL` it is 36px,
and the clock gains twelve.

Setting it as a column also buys back a whole part of the date. On one line
the weekday and the month compete for the same row and one of them has to go,
which is what the date setting chooses between; a column has room for all
three, so stacked it shows the lot and ignores the setting.

The block does not tie to the hour's baseline. As three lines it is tall
enough that its own rhythm down the right margin matters more than an
alignment nothing else in that column shares, so its foot is placed where the
gap up to the header equals the gap down to the temperature — 45 rows either
side, against 34 and 56 when it hung off the hour. Ink heights are measured
off the device to do that arithmetic, because the box height Pebble reports
carries leading above the cap and is not the ink.

The step count carries no label there. Two labels will not fit on one row
with two values, and an accent-colored number with a comma in it needs no
telling — whereas a bare `68` does, so the pulse keeps `BPM` inboard of it in
both layouts.

Two alignment axes and nothing centered: everything begins at x=10 or ends at
x=189. Only the horizon and the ground run to the bezel.

**The clock is stacked, and that is the whole design.** Set on one line it
tops out at 44px of cap height on a 200px screen, and the face stops being
about the time. Stacked, the numerals are 64px, and the clock owns the field
the way it does on the faces people actually love — TimeStyle gives 82% of
its vertical to the time and nothing else competes.

The cost is the colon, which stacking removes. That is the trade.

### Type

One family, three ranks, and no centering anywhere.

| Rank | Element | Face |
| --- | --- | --- |
| Hero | clock | Montserrat Bold 94 |
| Secondary | steps, sleep, pulse, day number, temperature | Montserrat Bold 22 |
| Label | date on one line | Montserrat Bold 15, caps, tracked |
| Label | weekday, month, BPM, sleep units | Montserrat Bold 11/14 |

Every value at the secondary rank is bold, including the pulse. It was Light
once, to stop two 22px numerals sharing the header row from reading as two
headlines — but the hierarchy is already carried by color there, accent
against muted, and on a screen with no light of its own the hairline strokes
are the first thing to go. Color survives the panel; stroke weight does not.

Only the caps are tracked — tracked figures look broken. Every face is
subsetted by `characterRegex` to the handful of glyphs it actually draws, so
twelve faces cost 43KB.

**The clock is drawn one digit at a time, into fixed-width slots**, and the
hours and minutes are right-aligned to the same edge. Two consequences worth
having:

- The digits cannot shuffle. Montserrat is proportional — its `1` is barely
  half the width of its `0` — so a right-aligned proportional clock would jog
  sideways every time the minute changed. Slotting makes any face tabular.
- A single-digit hour sits in the *right* slot, above the minutes' second
  digit, rather than hanging off the left. The block stays where it is; only
  the alignment inside it changes.

The clock face is selectable, each with a bold option — bold by default,
because it is what survives brighter sun:

| Face | Emery stacked | Character |
| --- | --- | --- |
| **Montserrat** | 94 | geometric, round counters |
| **Inter** | 91 | tall x-height, very clean |
| **Roboto Mono** | 93 | tabular by design; a slashed zero |
| **DSEG7 Classic** | 68 | a real seven-segment display |
| **LECO** | 60 | squared LCD; system face |

The sizes are not round because the faces are not the same width, and they are
not chosen by eye — `tools/make_fonts.py` solves each one against cap height,
block width, and a ceiling that is easy to miss.

**Pebble caps a single glyph's packed bitmap**: `(w*h+7)/8` must fit
`MAX_FONT_GLYPH_SIZE`, which is 512 bytes on Emery and 256 on the 144x168
watches. The glyph that hits it is almost never a digit — it is `U+25AF`, the
wildcard box, which `fontgen` embeds in every font whether the charset asks
for it or not. Azeret Mono was rejected on exactly that: the only mono of
seven with an unmarked zero, and capped at cap 47 against everyone else's 68
by a glyph the face would never draw. Space Grotesk hits that before it runs out of screen, which is why it
sits at cap 56 where everything else reaches 68 — visibly smaller, and not a
mistake. Those ceilings were bisected with the SDK's own `font.fontgen` rather
than modelled: two attempts to predict them from a rasteriser here produced
sizes that still failed to build. `tools/probe_glyph_ceiling.py` re-derives
them.

Whichever face is chosen, the minutes' baseline and the horizon do not move —
only the hour rises — so the terrain and the day column are identical across
all eight.

**DSEG7 draws its unlit segments**, which is most of what makes a display read
as a display rather than as a typeface: an `8` goes down in a darker tone under
every digit, in both slots, so the hour does not go dark when it is one digit.

It needs somewhere to put that tone — between the sky and the ink — and above
a *lit* sky the Pebble 64 has nothing there. On Dusk every candidate reads as
`88` rather than `9`. So `unlit` is an eighth theme colour, and the themes with
no room for it set it to their own sky, where it quietly draws nothing:

| | unlit | against |
| --- | --- | --- |
| **Phosphor** | `550000` | `FF5500` on black — the real thing |
| **Noir** | `555555` | white on black |
| **Paper** | `AAAAAA` | black on white |
| **Moss** / **Tide** | `00AA00` / `00AAAA` | white on a dark ground |
| **Dusk** | *its own sky* | nothing to spend; the ghosts vanish |

Black and white gets nothing either: two colours have no third tone to ghost
with. Custom exposes it as an eleventh role, defaulting to the sky.

### The rest of the type

The face used for everything that is not the clock is selectable too, across
all four roles at once: Montserrat, Inter, or Gothic. Inter has
the taller x-height of the two bundled ones, so it reads largest at the same
pixel size. Only the chosen family is resident, and it is unloaded and
reloaded when the setting changes rather than kept alongside — which means
remembering which of the four are *ours*, since handing a system font to
`fonts_unload_custom_font` is not a thing to do twice.

**Gothic is the only system family that reaches these sizes**, and it costs no
resource bytes at all. Bitham cannot: its smallest full cut is 30 and the
largest role here is 22, while its 18 and 34 cuts are reduced-charset subsets.
LECO and the Bitham numerals are digits-only, which rules them out for the
caps. Gothic only comes in 09/14/18/24 against the 11/14/15/22 the layout
wants, so the proportions are near rather than exact.

One approximation worth knowing: `INK_VAL` and `INK_CAPS_S` are Montserrat's
ink heights, and they place the day column and the degree ring. The other
three families differ by a pixel or two, so those sit marginally off where
they would if measured per face. Deriving them on the device would mean
measuring ink rather than the box height Pebble reports, which it does not
offer.

Two things worth knowing:

- **Everything is positioned from a baseline, never a box top.** Pebble draws
  text from the box, so each font measures the distance down to its own
  baseline once, on the device, from a probe glyph. Guessing that offset is
  how rows end up three pixels out of alignment.
- **The degree mark is drawn, not set.** It keeps the glyph off the bundled
  charset and lets the ring hang past the right margin, so the numerals stay
  optically aligned with the row beneath.

### The info card

The header and the day column are one object: an L, a strip along the top and
a column down the right, joined at an elbow. It can take a background of its
own, and a rule bounds it.

Three of its corners are rounded and **they do not all round the same way**,
which is the whole character of the thing:

- the **elbow**, where the strip meets the column, turns convex toward the
  clock — the rule bends through it
- the strip's **free end curves up** into the left bezel, so the card lifts
  away from the screen edge once it is past the step count
- the column's **free end curves right** into the horizon, so it lifts away
  again once it is past the temperature

An L with a concave elbow is not something Pebble's rounded-rect primitives can
express — `graphics_fill_rect` rounds convex corners only. So it is built by
hand: fill the region, then cut each corner with a square of the opposite tone
and put a disc back. Six fills and three arcs, exact at any radius.

On one line there is no column to divide, so the card is the strip alone.

Nothing about this has to be visible. With the info background left at the sky
tone it is a rule and nothing more, and with the rule off it is not there at
all — which is the point: it is a separation some will want and others will not.

### Color

Six themes plus Custom. Five of them are built the same way: a lit sky over
ground in shadow, and exactly one bright color spent only on the horizon, the
step count and the terrain — because those three are one idea seen three ways.

| | sky | ground | accent |
| --- | --- | --- | --- |
| **Dusk** *(default)* | navy | black | amber |
| **Phosphor** | black | black | green data, orange time |
| **Noir** | black | black | white horizon, gray terrain |
| **Paper** | white | cream | orange |
| **Moss** | dark green | black | chartreuse |
| **Tide** | teal | black | cyan |

Three meanings rather than fourteen colors: **the ink is time, the accent is
movement, the muted tone is context.** Every preset value is already on the
Pebble 64 — each channel 00/55/AA/FF — so nothing is quantized out from under
the design.

A preset ties three roles together on purpose. The newest bar takes the
clock's color because the current minute *is* the time; the terrain takes the
step count's because both *are* movement; and the whole sleep state borrows
the muted tone, because before you are up everything on the face is context.
Custom can cut them loose — it is the one place where breaking the rule is the
point — so it exposes fourteen roles rather than seven, in two levels. Simple
shows the seven and keeps the rest following their parent; Advanced separates
every one. The labels in the sky were the last to be split off the chart's
tone: one names a value you are reading, the other is ruling behind a plot,
and they were only ever the same colour by accident of both being quiet.

Hiding the advanced pickers is not enough on its own — a hidden Clay item
still submits its value, so a parent changing while its child sat at a stale
default would silently break the tie that Simple is promising. The config
page copies a parent's value down to its children while Simple is selected.

The three latecomers default to a sentinel meaning **still shared**, which is
also what a settings blob saved before they existed reads as. So adding them
could not move a color under anyone who had already set one; the split only
happens once it is asked for.

Dusk is the default because it is the only theme that *depicts* the face: a
lit sky, land in shadow, one warm line between them. It also happens to be
the widest luminance separation of any dark theme here, which matters on a
reflective panel with no light of its own. Paper is strictly the most legible
in direct sun — a white ground uses the panel's brightest state — but at
thumbnail size it reads as a system face, and a default is the argument a
watchface makes for itself.

Phosphor is the exception, and it breaks the recipe on purpose. There is no
lit sky: both bands are black, so the horizon is the only
thing dividing them. It spends two hues rather than one — `FF5500` on the time
and the horizon, `00AA55` on everything the body reports, `005555` on the
graticule — which makes the split by *what a number is about* rather than by
rank. The horizon siding with the clock instead of the terrain is what stops
the two halves from reading as unrelated: it becomes the line the numerals
stand on. A useful accident falls out of it — asleep, the horizon and the
value drop to the muted green, so the face is entirely green until you get up
and the time turns orange.

Red is not a theme color. It appears only on a flat battery and a lost phone.

## Sleep, and the morning

Last night's sleep holds the value slot until you have actually got up and
moved past the wake threshold — 500 steps by default. That much is ActiveHour's
rule, by way of Solfarer.

The problem with only doing that much: for the first stretch of every morning
the face would show a sleep duration above sixty empty columns, because the
last hour was spent asleep. The signature element goes blank exactly when the
alternate state is up.

So in sleep state the terrain's window becomes **the night itself** — sixty
columns from the moment you fell asleep to the moment you woke, each one how
much you moved, auto-scaled so the most restless stretch fills the plot. The
watch's own `vmc` reading carries it; steps during sleep are all zero.

And the whole face is cool until you cross the threshold. The horizon, the
terrain and the value all sit in the muted tone, then go warm the moment steps
take the slot back. Your watch turns the light on when you get up.

If no sleep session is recorded, or the minute history is empty, the terrain
falls back to the past hour.

## The terrain

Sixty columns of exactly 3px, x=10 to 189. Any other pitch and the columns
beat against each other — every third bar comes out a pixel fatter and the
ridge visibly ripples.

- a quarter-hour graticule, full height, *behind* the columns: where a column
  covers it the rule vanishes, where it does not it reads as structure
- a baseline on the last row, so a still minute is ground rather than a stub
- a dotted line at 60 steps/minute, the pace that counts as walking
- the newest minute in the clock's color, so *now* is findable

The fetch discipline is Solfarer's, unchanged: the minute history is read once
at boot, then exactly once more at the first quarter-hour mark, which is when
the watch has filled in the hour it was not running for. After that the newest
column is kept live from the step delta and never refetches.

## Build

```bash
pebble build && pebble install --emulator emery
```

If the emulator comes up showing a different watchface, `pebble wipe` and
install again. Two traps, both of which have cost an hour here before:

- **Settings persist across installs.** The bundled JS sends a weather message
  on launch, which makes the watch save the whole settings blob — after which
  changing a default in `defaults()` does nothing. Wipe between runs when
  testing defaults.
- **Screenshots race the install.** `pebble install` returns before the face
  has relaunched. Kill first, and give it several seconds.

Emulator screenshots also run through an LCD simulation, so colors read
washed out compared to the hex in `settings.c`. That is roughly what the real
transflective panel does; judge color on the wrist, not in the simulator.
