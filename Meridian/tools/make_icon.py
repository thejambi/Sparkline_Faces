#!/usr/bin/env python3
"""The launcher icon: the face's whole idea at 25 pixels.

A horizon with terrain standing on the bottom edge. No clock — at this size a
numeral is four pixels of stem and reads as noise — so the icon carries the
part of the design that survives being shrunk.

The sky is transparent rather than black: the launcher draws its own
background, and a black square would sit in it as a tile rather than a mark.
Every color is on the Pebble 64, same as the themes.
"""
from PIL import Image

W = H = 25
AMBER = (255, 170, 0, 255)          # 0xFFAA00, Dusk's accent
CLEAR = (0, 0, 0, 0)

HORIZON_Y = 9                       # 2 rows, then ground below
BAR_TOP = 13                        # a gap under the horizon, as on the face
BOT = 24
PITCH, BAR_W = 3, 2

# A plausible hour: two bursts and a quiet stretch. Read left to right, these
# are the same shape the terrain draws.
HEIGHTS = [2, 5, 11, 7, 1, 3, 8, 12]


def main(path):
    im = Image.new('RGBA', (W, H), CLEAR)
    px = im.load()

    for x in range(W):
        px[x, HORIZON_Y] = AMBER
        px[x, HORIZON_Y + 1] = AMBER

    room = BOT - BAR_TOP + 1
    for i, h in enumerate(HEIGHTS):
        x0 = 1 + i * PITCH
        if x0 + BAR_W > W:
            break
        h = min(h, room)
        for x in range(x0, x0 + BAR_W):
            for y in range(BOT - h + 1, BOT + 1):
                px[x, y] = AMBER

    im.save(path)
    print('%s  %dx%d  %d colors' % (path, W, H, len(im.getcolors())))


if __name__ == '__main__':
    import sys
    main(sys.argv[1] if len(sys.argv) > 1 else 'resources/images/menu_icon.png')
