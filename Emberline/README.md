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

**Stacked** — the default. Hours over minutes, the biggest numeral the screen
allows, and everything else in two panels.

```
  0   ▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔      battery, 2px, top edge
 24   8,842              MI 4.2       the header: what you have moved
 33   ╰────────────────────────╯      ...curving off both bezels
                       ╭──────────
 67                        62         the column: the pulse, with
 82                       BPM         its caption underneath
108                       WED
129   09                    5         the date, whole
144                       AUG
178                       74°
182   41
188   ────────────────────────────     the horizon
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
that leaves for the clock: stacked buys 94px numerals and pays for them with a
shorter terrain, while one line buys back the colon and a calmer header.

**The two panels are two objects, not one shape that got cut.** The header
carries the two figures that are about movement — the step count and the day's
distance. The column carries everything else: the pulse, the date, the
temperature. Each panel is anchored to exactly one bezel and rounded on every
edge that faces inward, so the only rule crossing the whole screen is the
horizon, which is the one line here that should.

That shape is what the pulse's caption pays for. Set inboard of its number it
needs a row to itself and the header has to span the width; set underneath, the
pulse joins the column and the header drops to a single row — and a row that
short can curve away at both ends instead of running bezel to bezel.

**The column's day block is what pays for the clock.** The obvious arrangement
— date and temperature along the top, health down the right margin — puts
`8,842` beside the numerals and caps them at 76px. Swap the two and the widest
thing beside the clock becomes the weekday, so long as the date is set as a
column rather than a line: `WED 29` on one line is 73px, wider than the step
count it replaced, and would have made the clock smaller rather than larger.
Stacked into `THU` / `30` / `JUL` it is 36px, and the clock gains twelve.

Setting it as a column also buys back a whole part of the date. On one line the
weekday and the month compete for the same row and one has to go, which is what
the date setting chooses between; a column has room for all three, so stacked
shows the lot and ignores the setting.

**The column's openings are divided, not chosen.** Whatever height its groups
do not use is split between the gaps and the margins, so losing the pulse
re-spaces the column rather than leaving a hole where it was. The margins are
half-spaces: a gap between two groups has to carry the eye across and read as a
division, while a margin only has to hold a group off the panel edge, and
giving both the same room crowds the middle. On Emery that is 10/18/18/10 with
a pulse and 22/43/22 without.

**The panels right-align to `CARD_R`, not to `MARGIN_R`.** The screen's optical
margin is too generous inside a filled panel: between the column's rule and
`MARGIN_R` there are 37px, and a three-digit pulse at 22pt is 39, so `188` used
to cross the rule while 10px of panel sat unused on the other side. A panel's
own fill does the containing that open sky needs a margin for, so the stacked
layout pulls its right axis in — and the header's distance comes with it, since
the two sit on the same line.

**The clock is centered in what the panels leave it**, not pinned to the left
margin. A two-slot block runs from 110px to 145px depending on the face, so a
fixed anchor leaves the narrow ones stranded a long way from the column while
the wide ones nearly touch it. Centering is what makes the font setting feel
like a choice rather than a different layout.

The step count carries no label. Two labels will not fit on one row with two
values, and an accent-colored number with a comma in it needs no telling —
whereas a bare `62` does, so the pulse keeps `BPM` with it in both layouts.

**The clock is stacked, and that is the whole design.** Set on one line it tops
out at 44px of cap height on a 200px screen, and the face stops being about the
time. Stacked, the numerals reach 68px, and the clock owns the field the way it
does on the faces people actually love — TimeStyle gives 82% of its vertical to
the time and nothing else competes.

The cost is the colon, which stacking removes. That is the trade.

### Hiding the panels

Stacked can park both panels off-screen and bring them back on a shake, for
seven seconds. It is off by default and it is the reason the panels are panels:
with them gone the clock has the whole sky rather than the band between the
header and the horizon.

Everything that moves is drawn from one number — `s_reveal`, 0 parked and 100
in — so there is a single thing to reason about rather than a position per
panel. The header leaves upward and the column rightward, each exactly its own
size, so neither is left peeking at a bezel. The frame timer runs only while
something is travelling, and the tap service is subscribed only while a layout
is using it.

What the clock does with the freed space is a second setting, because only one
face can take it as *size*:

- **Grow** — Blocky Digits steps up a whole scale, ×5 to ×6, cap 65 to 78. Its
  baselines move with it: the hour rises to clear the top edge while the
  minutes stay on the horizon. Whole-number scaling is the only way to grow
  without resampling, so the bundled faces cannot follow — they each ship one
  size per role.
- **Keep the size** — the block rises into the sky instead, until the gap above
  it matches the gap down to the horizon. That is 18px for Blocky Digits and 16
  for Montserrat, derived from each face's own cap rather than fixed, since a
  constant would center exactly one of them.

Position glides either way: the clock's field has its right edge travel out to
the bezel as the column leaves and its left margin close at the same rate, so
it stays centered in the space it actually has at that instant. Scale cannot be
interpolated, so growing *snaps* — at the halfway point, while the eye is
following the panels, which is the least conspicuous moment on offer.

Auto-hide does not reach a watch that already had settings saved. Its blob
stops short of the field, so it would otherwise inherit the new default and
hide the face somebody had been wearing until they happened to shake it.

### Type

One family, three ranks, and no centering anywhere.

| Rank | Element | Face |
| --- | --- | --- |
| Hero | clock | Blocky Digits, cap 65 *(default)* |
| Secondary | steps, sleep, pulse, day number, temperature | Montserrat Bold 22 |
| Label | date on one line | Montserrat Bold 15, caps, tracked |
| Label | weekday, month, BPM, sleep units | Montserrat Bold 11/14 |

Every value at the secondary rank is bold, including the pulse. It was Light
once, to stop two 22px numerals sharing the header row from reading as two
headlines — but the hierarchy is already carried by color there, accent
against muted, and on a screen with no light of its own the hairline strokes
are the first thing to go. Color survives the panel; stroke weight does not.

Only the caps are tracked — tracked figures look broken. Every bundled face is
subsetted by `characterRegex` to the handful of glyphs it actually draws; three
families across two weights, two layouts and two screens come to 43KB, and the
default clock face costs none of it.

**The clock is drawn one digit at a time, into fixed-width slots**, and the
hours and minutes are right-aligned to the same edge. Two consequences worth
having:

- The digits cannot shuffle. Montserrat is proportional — its `1` is barely
  half the width of its `0` — so a right-aligned proportional clock would jog
  sideways every time the minute changed. Slotting makes any face tabular.
- A single-digit hour sits in the *right* slot, above the minutes' second
  digit, rather than hanging off the left. The block stays where it is; only
  the alignment inside it changes.

The clock face is selectable. Always the bold cut — it is what survives
brighter sun, and the toggle that used to offer the light one was a row on the
config page nobody ever moved:

| Face | Emery stacked | Character |
| --- | --- | --- |
| **Blocky Digits** *(default)* | cap 65, or 78 grown | not a font: see below |
| **Montserrat** | 94 | geometric, round counters |
| **Inter** | 91 | tall x-height, very clean |
| **DSEG7 Classic** | 68 | a real seven-segment display |

**Blocky Digits is drawn, not set.** It is a 10x13 grid of cells scaled by a
whole number — 5 on the Emery stacked layout, 6 with the panels hidden, down to
2 on a 144x168 one-liner — and every row is emitted as runs of
`graphics_fill_rect`, so it is crisp at every size instead of being hinted down
to one. The whole set is 286 bytes
against roughly 8KB for one size of one bundled font. Draw it in
`tools/digitgrid.py`, preview it in the real layouts, then `--emit-c` writes
`src/c/digits.c`; nothing is ever hand-copied into C. Running `--emit-c` is a
deliberate step, not a build hook — the previews are the review, and a drawing
is not finished until it has been looked at.

The gap between digits is one grid column rather than a pixel count, because
two pixels beside a 50px digit is not the same gap as two beside a 20px one.
Glyphs are centered on their **ink**, not their box: the drawing carries a
blank column down one side, which is where that gap comes from, and centering
the box would count the blank as glyph and shove the whole clock half a column
across. `DIGIT_INK_L/R` are emitted from the drawing so redrawing cannot
reintroduce it.

The set follows one rule: **chamfer where the letterform curves, leave square
where a stroke is cut off.** That is why `0` and `8` are eased on all four
corners, `5` has a square top bar but a chamfered bottom bowl, and `1`, `4` and
`7` — the digits made only of straight strokes — are square everywhere. The
closest pair is `5` against `6` at three rows and seven cells; every other pair
has at least nine cells between them.

The sizes are not round because the faces are not the same width, and they are
not chosen by eye — `tools/make_fonts.py` solves each one against cap height,
block width, and a ceiling that is easy to miss.

**Pebble caps a single glyph's packed bitmap**: `(w*h+7)/8` must fit
`MAX_FONT_GLYPH_SIZE`, which is 512 bytes on Emery and 256 on the 144x168
watches. The glyph that hits it is almost never a digit — it is `U+25AF`, the
wildcard box, which `fontgen` embeds in every font whether the charset asks
for it or not. Azeret Mono was rejected on exactly that: the only mono of seven with an
unmarked zero, and capped at cap 47 against everyone else's 68 by a glyph the
face would never draw.

**Monospace is beside the point here.** The clock is drawn one digit per
fixed-width slot, which makes any face tabular — which is why proportional
Montserrat works at all. So the faces that were tried got chosen for their
shapes rather than their spacing, and the search for a blocky terminal one
ended by drawing it instead.

Those ceilings were bisected with the SDK's own `font.fontgen` rather than
modelled: two attempts to predict them from a rasteriser here produced sizes
that still failed to build. `tools/probe_glyph_ceiling.py` re-derives them.

Whichever face is chosen, the minutes' baseline and the horizon do not move —
only the hour rises — so the terrain and the column are identical across all
four. Growing Blocky Digits is the one exception, and it only happens with the
panels already gone.

**DSEG7 draws its unlit segments**, which is most of what makes a display read
as a display rather than as a typeface: an `8` goes down in a darker tone under
every digit, in both slots, so the hour does not go dark when it is one digit.

It needs somewhere to put that tone — between the sky and the ink — and above
a *lit* sky the Pebble 64 has nothing there. On Dusk every candidate reads as
`88` rather than `9`. So `unlit` is an eighth theme colour, and the themes with
no room for it set it to their own sky, where it quietly draws nothing:

| | unlit | against |
| --- | --- | --- |
| **Phosphor** | *its own sky* | off by default — see below |
| **Noir** | `555555` | white on black |
| **Paper** | `AAAAAA` | black on white |
| **Moss** / **Tide** | `00AA00` / `00AAAA` | white on a dark ground |
| **Dusk** | *its own sky* | nothing to spend; the ghosts vanish |

Black and white gets nothing either: two colours have no third tone to ghost
with. Custom exposes it as its own role, defaulting to the sky.

**Blocky Digits spends the same role on an outline instead.** It is laid out on
the same seven segments but does not separate them — its bars run together at
the corners — so an `8` behind it is not a row of dark segments, it is a lit
rectangle with a few notches. What it can do is wear the second ink as a
border, which decouples the clock's colour from whatever is behind it: white
numerals survive a white sky, which without an outline they do not.

The two can never collide despite sharing a role, because only one clock face
is ever active. `unlit` means "the second ink" and each face spends it its own
way. Like the ghosts it is opt-in, and off by default for the same reason —
a theme whose unlit is its own sky draws neither.

It grows inward as well as out: the border pass is the glyph dilated, and the
ink pass is the same glyph *eroded* by the same amount, so the band is twice
the nominal width — 4px at x5 — while the silhouette and the spacing stay
exactly where they were. Thickening it costs the stroke rather than the gap,
which is the only budget with room in it.

Dilation is exact on runs, since the union of inflated rectangles is a
dilation. Erosion is not, so that pass walks cells instead of runs and pulls in
each edge only where no neighbouring cell is ink. It is a cell-accurate
approximation rather than a true erosion — at a concave junction it leaves a
square of `in` by `in` that a real one would remove, which is 2px at x5 and has
never been visible. The clock measures 0ms either way.

**Stacked only.** The outline grows the glyph outward, so the air between slots
has to grow with it — one grid column of gap is exactly what a 2px outline on
each of two neighbours eats at x5, and at x3 it would merge them into one
shape. Widening the air fits every stacked role. One line has no width to give:
Emery at x4 would need 212px of a 200px screen before the colon is counted, and
even a 1px outline needs 202. It would have to drop a scale step, cap 52 to 39,
which is a worse trade than going without.

**Phosphor was the theme this was built for, and it is the one where it
failed.** `550000` against black read correctly in the emulator and vanished on
the wrist. The arithmetic says why: red carries 21% of perceived luminance, so
one step of red above black is 7% apart, where the same step in green — Moss —
is 24%. It was the dimmest pairing the Pebble 64 can make, and the emulator's
LCD simulation flattered it. Phosphor's unlit tone is now its own sky, and the
capability stays available through Custom.

### The rest of the type

The face used for everything that is not the clock is selectable too, across
all four roles at once: Montserrat, DSEG14, or Gothic. Only the chosen family
is resident, and it is unloaded and
reloaded when the setting changes rather than kept alongside — which means
remembering which of the four are *ours*, since handing a system font to
`fonts_unload_custom_font` is not a thing to do twice.

**DSEG7 is a clock face only, and DSEG14 is why.** Seven segments cannot form
most of the alphabet: DSEG7 has glyphs for A-Z, so a charset check passes, but
`WED MAR AUG` comes out as `ЬEd ΠAr AuG` — the W a broken H, the M a Π, and D,
R and U silently dropping to lowercase because the capitals are impossible. It
would fail at any size. Fourteen segments exist precisely to spell, so DSEG14
takes the text roles and the two together make one instrument rather than a
mixed metaphor.

Its captions are the weak point, and knowing why matters: at an eight-row cap
the diagonal segments are one pixel and break up, so `A` leans toward `R` and
`O` reads as a box. That is survivable only because these captions are a closed
set — weekday, month, `BPM` — recognised rather than read. It would not do for
arbitrary text.

**Gothic is the only system family that reaches these sizes**, and it costs no
resource bytes at all. Bitham cannot: its smallest full cut is 30 and the
largest role here is 22, while its 18 and 34 cuts are reduced-charset subsets.
LECO and the Bitham numerals are digits-only, which rules them out for the
caps. Gothic only comes in 09/14/18/24 against the 11/14/15/22 the layout
wants, so the proportions are near rather than exact.

Inter and Jersey were tried here and dropped after wrist testing — Inter
remains a clock face, Jersey is gone from both. Their slots stay in the enum
and their rows are zeroed, so a setting still pointing at one falls back to
Montserrat, which is the same discipline the clock faces use for LECO and the
five monos that were dropped before it.

**Ink heights and tracking are per text face**, not global. They place the day
column and the degree ring, and taking them from Montserrat quietly made every
other family a pixel or two wrong. It is also what lets DSEG14 carry a taller
caption than the rest: fourteen segments need more than eight rows before the
diagonals break up, and its tracking is zero because segment forms already hold
their own air — which makes its captions *narrower* than Montserrat's despite
being half again as tall.

Montserrat's row is the pair measured off the device by scanning ink rows in a
screenshot. The others come from a local rasteriser, which reports one row
taller than the device does, so they carry that correction — and Montserrat is
referenced rather than re-derived, because the default layout must not move.

Two things worth knowing:

- **Everything is positioned from a baseline, never a box top.** Pebble draws
  text from the box, so each font measures the distance down to its own
  baseline once, on the device, from a probe glyph. Guessing that offset is
  how rows end up three pixels out of alignment.
- **The degree mark is drawn, not set.** It keeps the glyph off the bundled
  charset and lets the ring hang past the numerals' own right edge, so they
  stay optically aligned with the rows above and below it. In the panels it
  takes the smaller ring: at 22pt a three-digit temperature plus a full-size
  one does not fit the column at all.

### The panels

The header and the column are two panels. Each can take a background of its own
and a rule bounds it, and each is anchored to exactly one bezel — the header to
the top, the column to the right — with every inward-facing edge rounded.

That anchoring is the whole discipline. A panel that runs off a bezel needs no
corner there, so what gets rounded is exactly what faces the clock, and no rule
ever reaches an edge. Only the horizon crosses the screen.

They were one object once: an L, joined at a concave elbow, which is not
something Pebble's primitives can express — `graphics_fill_rect` rounds convex
corners only. It had to be built by hand, cutting each corner with a square of
the opposite tone and putting a disc back. Two convex panels need none of that
for the fill; only `fill_corner` survives, for the two places the header lifts
off its bezels.

The column's free end still curves right into the horizon and stops, because
the horizon closes it — two lines meeting is one edge, and drawing both would
thicken it. Painted the same colour as the sky the horizon closes nothing, so
in that case the rule runs on to the bezel and encloses the panel itself. It
has to be drawn after the ground, which fills the horizon band across the full
width and would otherwise paint over it in exactly the case it exists for.

On one line there is no column, so the card is the header alone.

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

**Dusk spends two more steps of its own blue on the panels**: `000055` sky,
`0000AA` behind the panels, `5555FF` for the rule. The rule is deliberately not
`0000FF`, which is what one more step of the same pure blue would be — blue
carries seven percent of perceived luminance, so `0000FF` sits about two points
above the panel it is drawn on and would disappear on the wrist the same way
Phosphor's unlit segments did. `5555FF` is the next step the eye actually
resolves. Every other preset leaves its panels at its own sky, where they are a
rule and nothing more.

Red is not a theme color. It appears only on a flat battery and a lost phone.

## Sleep, and the morning

Last night's sleep holds the value slot until you have actually got up and
moved past the wake threshold — 350 steps by default. That much is ActiveHour's
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
