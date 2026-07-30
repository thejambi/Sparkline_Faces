#!/usr/bin/env python3
"""The store banner, 720x320 — the face's own composition at listing size.

Sky over ground, one warm line where they meet, terrain standing on the bottom
edge. The same idea the watch draws, which means the banner does not have to
explain the face: it is the face, with room for the name.

The screenshot stands *on* the horizon rather than floating over it, so the
banner reads as one scene rather than a mockup pasted onto a background.

    python3 tools/make_banner.py
"""
from PIL import Image, ImageDraw, ImageFont

W, H = 720, 320
FD = 'resources/fonts/'

# Dusk, the default theme — see THEMES[] in src/c/settings.c
SKY = (0, 0, 85)
GROUND = (0, 0, 0)
AMBER = (255, 170, 0)
INK = (255, 255, 255)
MUTED = (170, 170, 255)
SCALE = (85, 85, 170)

HORIZON_Y = 268
RULE = 3
SHOT = 'store/screenshots/emery_1_dusk.png'
SHOT_X = 486                        # leaves a 34px right margin
NAME = 'EMBERLINE'
NAME_MAX = 68                       # shrunk to fit if the name is a long one
TEXT_L = 44

# The same profile the icon uses, stretched over a banner's width.
PROFILE = [0.00, 0.12, 0.34, 0.55, 0.42, 0.18, 0.05, 0.00, 0.00, 0.08,
           0.22, 0.30, 0.24, 0.10, 0.00, 0.00, 0.15, 0.48, 0.72, 0.90,
           0.66, 0.38, 0.14, 0.00, 0.05, 0.20, 0.44, 0.58, 0.35, 0.12]


def main():
    im = Image.new('RGB', (W, H), SKY)
    d = ImageDraw.Draw(im)

    d.rectangle([0, HORIZON_Y, W - 1, HORIZON_Y + RULE - 1], fill=AMBER)
    d.rectangle([0, HORIZON_Y + RULE, W - 1, H - 1], fill=GROUND)

    # terrain, standing on the bottom edge as it does on the watch
    top = HORIZON_Y + RULE + 6
    bot = H - 1
    room = bot - top
    pitch = W / len(PROFILE)
    bar_w = int(pitch * 0.66)
    for i, frac in enumerate(PROFILE):
        h = int(frac * room)
        if h <= 0:
            continue
        x0 = int(i * pitch + (pitch - bar_w) / 2)
        d.rectangle([x0, bot - h, x0 + bar_w - 1, bot], fill=AMBER)

    def put(s, font, fill, x, baseline, track=0):
        if not track:
            b = font.getbbox(s)
            d.text((x - b[0], baseline - b[3]), s, font=font, fill=fill)
            return
        for c in s:
            b = font.getbbox(c)
            d.text((x - b[0], baseline - b[3]), c, font=font, fill=fill)
            x += (b[2] - b[0]) + track

    # The wordmark takes the largest size that still clears the watch. Fitted
    # rather than fixed, because renaming the face is exactly what pushes a
    # name off the end of a size chosen for the old one: EMBERLINE is 447px at
    # the 68 that MERIDIAN sat in comfortably at 391.
    room = SHOT_X - 24 - TEXT_L
    for size in range(NAME_MAX, 23, -2):
        name = ImageFont.truetype(FD + 'Montserrat-Bold.ttf', size)
        w = sum(name.getbbox(c)[2] - name.getbbox(c)[0] + 3 for c in NAME) - 3
        if w <= room:
            break
    print('  wordmark %s at %dpx (%d of %dpx)' % (NAME, size, w, room))

    tag = ImageFont.truetype(FD + 'Montserrat-Bold.ttf', 21)
    sub = ImageFont.truetype(FD + 'Montserrat-Light.ttf', 20)

    put(NAME, name, AMBER, TEXT_L, 150, track=3)
    put('SKY OVER GROUND', tag, MUTED, TEXT_L + 2, 190, track=4)
    put('the last hour of your movement,', sub, INK, TEXT_L + 2, 232)
    put('a column a minute', sub, INK, TEXT_L + 2, 258)

    # the watch, standing on the horizon
    shot = Image.open(SHOT).convert('RGB')
    sx, sy = SHOT_X, HORIZON_Y - shot.height + RULE
    d.rectangle([sx - 2, sy - 2, sx + shot.width + 1, sy + shot.height + 1],
                outline=SCALE, width=2)
    im.paste(shot, (sx, sy))

    im.save('store/banner_720x320.png')
    print('store/banner_720x320.png  %dx%d' % (W, H))


if __name__ == '__main__':
    main()
