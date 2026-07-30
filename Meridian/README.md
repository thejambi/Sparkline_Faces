# Meridian

Sky over ground, with one line where they meet.

The terrain along the bottom is the last hour of your movement — a column a
minute, sixty of them, standing on the bottom edge of the screen. The clock is
the sky above it. The one warm line between the two is the horizon.

**Emery only.** Every number here is chosen for a 200x228 colour screen.

## The layout

```
  0   ▔▔▔▔▔▔▔▔▔▔▔▔                      battery, 2px, top edge
 22   WED 29                    76°     tracked caps, both margins
 94   9                      8,842      hours share a baseline with the steps
156   41                    BPM 68      minutes share one with the pulse
162   ────────────────────────────      the horizon
164   ground
227   the terrain stands on the last row
```

Two alignment axes and nothing centred: everything begins at x=10 or ends at
x=189. Only the horizon and the ground run to the bezel.

**The clock is stacked, and that is the whole design.** Set on one line it
tops out at 44px on a 200px screen, and the face stops being about the time.
Stacked, the numerals are 55px, and the clock owns the field the way it does
on the faces people actually love — TimeStyle gives 82% of its vertical to the
time and nothing else competes.

The cost is the colon, which stacking removes. That is the trade.

### Type

One family, three ranks, and no centring anywhere.

| Rank | Element | Face |
| --- | --- | --- |
| Hero | clock | Montserrat Light 76 |
| Secondary | steps / sleep | Montserrat Bold 22 |
| Secondary | pulse | Montserrat Light 22 |
| Label | date, temperature | Montserrat Bold 15, caps, tracked |
| Label | BPM, sleep units | Montserrat Bold 11/14 |

Only the caps are tracked — tracked figures look broken. Every face is
subsetted by `characterRegex` to the handful of glyphs it actually draws, so
nine faces cost 27KB.

**The clock is drawn one digit at a time, into fixed-width slots**, and the
hours and minutes are right-aligned to the same edge. Two consequences worth
having:

- The digits cannot shuffle. Montserrat is proportional — its `1` is barely
  half the width of its `0` — so a right-aligned proportional clock would jog
  sideways every time the minute changed. Slotting makes any face tabular.
- A single-digit hour sits in the *right* slot, above the minutes' second
  digit, rather than hanging off the left. The block stays where it is; only
  the alignment inside it changes.

The clock face is selectable, each with a bold option:

| Face | Size | Character |
| --- | --- | --- |
| **Montserrat** | 76, bundled | geometric, round counters |
| **Roboto** | 76, bundled | a little narrower, more neutral |
| **LECO** | 60, system | squared LCD; already tabular |

LECO is a system face and cannot go past 60, so it sits noticeably smaller and
quieter than the two bundled ones — more instrument than poster. Only the
chosen clock face is resident: the other three would cost RAM for nothing, so
it loads on demand and the previous one is unloaded.

Two things worth knowing:

- **Everything is positioned from a baseline, never a box top.** Pebble draws
  text from the box, so each font measures the distance down to its own
  baseline once, on the device, from a probe glyph. Guessing that offset is
  how rows end up three pixels out of alignment.
- **The degree mark is drawn, not set.** It keeps the glyph off the bundled
  charset and lets the ring hang past the right margin, so the numerals stay
  optically aligned with the row beneath.

### Colour

Five themes plus Custom. Each is built the same way: a lit sky over ground in
shadow, and exactly one bright colour spent only on the horizon, the step
count and the terrain — because those three are one idea seen three ways.

| | sky | ground | accent |
| --- | --- | --- | --- |
| **Dusk** | navy | black | amber |
| **Noir** | black | black | white horizon, grey terrain |
| **Paper** | white | cream | orange |
| **Moss** | dark green | black | chartreuse |
| **Tide** | teal | black | cyan |

Three meanings rather than seven colours: **the ink is time, the accent is
movement, the muted tone is context.** Custom exposes all seven roles. Every
preset value is already on the Pebble 64 — each channel 00/55/AA/FF — so
nothing is quantised out from under the design.

Red is not a theme colour. It appears only on a flat battery and a lost phone.

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
- the newest minute in the clock's colour, so *now* is findable

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

Emulator screenshots also run through an LCD simulation, so colours read
washed out compared to the hex in `settings.c`. That is roughly what the real
transflective panel does; judge colour on the wrist, not in the simulator.
