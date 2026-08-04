# Bundled fonts

Every family here is redistributable, and each is embedded in the shipped
`.pbw` subset to the handful of glyphs the face actually draws. Attribution and
license terms therefore travel with the watchface, not just with this repo.

| File | Family | License |
| --- | --- | --- |
| `Montserrat-Bold.ttf`, `Montserrat-Light.ttf` | Montserrat | SIL Open Font License 1.1 (`OFL-Montserrat.txt`) |
| `Inter-Bold.ttf`, `Inter-Light.ttf` | Inter | SIL Open Font License 1.1 (`OFL-Inter.txt`) |
| `DSEG7Classic-Bold.ttf`, `DSEG7Classic-Light.ttf`, `DSEG14Classic-Bold.ttf` | DSEG | SIL Open Font License 1.1 (`OFL-DSEG.txt`) |

Blocky Digits needs no entry here: it is not a typeface. It is drawn in
`tools/digitgrid.py` and compiled to a table of row bitmasks in
`src/c/digits.c`.

Roboto was bundled through 1.1.0 and dropped after wrist testing.
