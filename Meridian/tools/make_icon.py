#!/usr/bin/env python3
"""The icons: the face's whole idea, at 25, 48 and 144 pixels.

A horizon with terrain standing on the bottom edge. No clock — at any of these
sizes a numeral is a few pixels of stem and reads as noise — so the icons carry
the part of the design that survives being shrunk.

Two kinds, because they are shown in different places:

  launcher  25px, amber on transparent. The launcher draws its own background,
            and a black square would sit in it as a tile rather than a mark.
  store     48 and 144px, on Dusk's navy sky. A store icon *is* a tile, so it
            gets the default theme's ground rather than a hole.

Everything is drawn at its target size rather than scaled from one master:
upscaling a 25px bitmap gives soft edges, and the whole point of the mark is
that it is made of hard pixel rows.

    python3 tools/make_icon.py            # writes all three
"""
from PIL import Image

# Dusk, the default theme — see THEMES[] in src/c/settings.c
SKY = (0, 0, 85, 255)               # 0x000055
AMBER = (255, 170, 0, 255)          # 0xFFAA00
CLEAR = (0, 0, 0, 0)

# A plausible hour: two bursts and a quiet stretch, as fractions of the plot.
# Read left to right, this is the shape the terrain draws.
PROFILE = [0.15, 0.40, 0.95, 0.55, 0.08, 0.25, 0.65, 1.00]


def draw(size, ground, cols=len(PROFILE), horizon_frac=0.38, gap_frac=0.08):
    """One icon. `ground` is the sky colour, or None for transparent."""
    im = Image.new('RGBA', (size, size), ground or CLEAR)
    px = im.load()

    rule = max(1, round(size / 12))              # horizon thickness
    hy = round(size * horizon_frac)
    for y in range(hy, hy + rule):
        for x in range(size):
            px[x, y] = AMBER

    top = hy + rule + round(size * gap_frac)     # a gap under the horizon
    bot = size - 1
    room = bot - top + 1

    pitch = size / cols
    bar_w = max(1, round(pitch * 0.68))
    for i, frac in enumerate(PROFILE[:cols]):
        x0 = round(i * pitch + (pitch - bar_w) / 2)
        h = max(1, round(frac * room))
        for x in range(x0, min(x0 + bar_w, size)):
            for y in range(bot - h + 1, bot + 1):
                px[x, y] = AMBER
    return im


def main():
    for path, size, ground in (
            ('resources/images/menu_icon.png', 25, None),
            ('publish/icon_small.png', 48, SKY),
            ('publish/icon_large.png', 144, SKY)):
        im = draw(size, ground)
        im.save(path)
        print('%-34s %dx%d  %d colors' % (path, size, size, len(im.getcolors())))


if __name__ == '__main__':
    main()
