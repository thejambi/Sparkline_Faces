#!/usr/bin/env python3
"""Draw the clock digits by hand, and see them in the real layout.

Edit DIGITS below. Every glyph is GRID_H rows of GRID_W characters — '#' is
ink, anything else is background. Then:

    python3 tools/digitgrid.py

which writes two files next to itself:

    digits_sheet.png   every glyph, magnified, with the grid drawn on
    digits_face.png    the four roles they would actually be used in

Nothing here touches the watchface. It exists so the shapes can be argued with
before any of it becomes C.

--- why 13 rows -------------------------------------------------------------
A pixel design only stays crisp if it scales by whole numbers, so the design
height has to divide usefully into every role's budget. 13 does:

    emery stacked    x5 -> cap 65   (the layout allows 68)
    emery one line   x4 -> cap 52   (the current font manages ~44)
    144   stacked    x3 -> cap 39   (the layout allows 43)
    144   one line   x2 -> cap 26   (the current font manages ~28)

Width has slack — at x5 an 8-wide glyph sits in a 42px slot where 71 is
free — so GRID_W can go to 10 or 12 for a blockier, more terminal figure.
Past 8 the row masks need 16 bits rather than 8, which doubles the data from
about 145 bytes to 290. Both are noise next to a font's 8KB.

--- what this cannot tell you -----------------------------------------------
PIL antialiases; the watch does not. That has misled us twice already, on
DSEG's unlit segments and on Jersey's stems. Here it matters less, because
every pixel is a filled rectangle at an integer scale and there is nothing to
resample — but the wrist is still the judge.
"""
import pathlib
import sys
from PIL import Image, ImageDraw, ImageFont

# ---------------------------------------------------------------------------
# EDIT BELOW. '#' is ink. Keep every glyph the same size as the others.
# ---------------------------------------------------------------------------
DIGITS = {
    '0': ['..#######.',
          '.#########',
          '.#########',
          '.###...###',
          '.###...###',
          '.###...###',
          '.###...###',
          '.###...###',
          '.###...###',
          '.###...###',
          '.#########',
          '.#########',
          '..#######.'],

    '1': ['.######...',
          '.######...',
          '.######...',
          '....###...',
          '....###...',
          '....###...',
          '....###...',
          '....###...',
          '....###...',
          '....###...',
          '.#########',
          '.#########',
          '.#########'],

    '2': ['..#######.',
          '.#########',
          '.#########',
          '.###...###',
          '.......###',
          '..########',
          '.#########',
          '.########.',
          '.###......',
          '.###......',
          '.#########',
          '.#########',
          '.#########'],

    '3': ['.########.',
          '.#########',
          '.#########',
          '.......###',
          '.......###',
          '..########',
          '..#######.',
          '..########',
          '.......###',
          '.......###',
          '.#########',
          '.#########',
          '.########.'],

    '4': ['.###...###',
          '.###...###',
          '.###...###',
          '.###...###',
          '.###...###',
          '.#########',
          '.#########',
          '..########',
          '.......###',
          '.......###',
          '.......###',
          '.......###',
          '.......###'],

    '5': ['.#########',
          '.#########',
          '.#########',
          '.###......',
          '.###......',
          '.########.',
          '.#########',
          '.#########',
          '.......###',
          '.###...###',
          '.#########',
          '.#########',
          '..#######.'],

    '6': ['..########',
          '.#########',
          '.#########',
          '.###......',
          '.###......',
          '.########.',
          '.#########',
          '.#########',
          '.###...###',
          '.###...###',
          '.#########',
          '.#########',
          '..#######.'],

    '7': ['.#########',
          '.#########',
          '.#########',
          '.###...###',
          '.......###',
          '.......###',
          '.......###',
          '.......###',
          '.......###',
          '.......###',
          '.......###',
          '.......###',
          '.......###'],

    '8': ['..#######.',
          '.#########',
          '.#########',
          '.###...###',
          '.###...###',
          '.#########',
          '..#######.',
          '.#########',
          '.###...###',
          '.###...###',
          '.#########',
          '.#########',
          '..#######.'],

    '9': ['..#######.',
          '.#########',
          '.#########',
          '.###...###',
          '.###...###',
          '.#########',
          '.#########',
          '..########',
          '.......###',
          '.......###',
          '.......###',
          '.......###',
          '.......###'],

    ':': ['..........',
          '..........',
          '..........',
          '....###...',
          '....###...',
          '..........',
          '..........',
          '..........',
          '....###...',
          '....###...',
          '..........',
          '..........',
          '..........'],
}

# ---------------------------------------------------------------------------
HERE = pathlib.Path(__file__).resolve().parent
RES = HERE.parent / 'resources' / 'fonts'
MB = str(RES / 'Montserrat-Bold.ttf')

GRID_H = len(DIGITS['0'])
GRID_W = len(DIGITS['0'][0])


def check():
    bad = [k for k, g in DIGITS.items()
           if len(g) != GRID_H or any(len(r) != GRID_W for r in g)]
    if bad:
        raise SystemExit('These glyphs are not %dx%d: %s\n'
                         'Every row must be exactly %d characters and every '
                         'glyph exactly %d rows.'
                         % (GRID_W, GRID_H, ' '.join(sorted(bad)),
                            GRID_W, GRID_H))


def snap(c):
    lv = (0, 85, 170, 255)
    return tuple(min(lv, key=lambda l: abs(l - v)) for v in c)
def hx(s):
    s = s.lstrip('#'); return snap(tuple(int(s[i:i+2], 16) for i in (0, 2, 4)))

P = dict(sky=hx('000055'), ground=hx('000000'), horizon=hx('FFAA00'),
         ink=hx('FFFFFF'), accent=hx('FFAA00'), muted=hx('AAAAFF'),
         label=hx('5555AA'), sep=hx('5555AA'))

DAY = ([0]*6 + [12,40,66,78,74,81,70,55,20,0,0,0] + [0]*9 + [8,14,9,0,0,0,0] +
       [0]*8 + [22,48,61,58,66,44,18,0,0,0] + [0,0,30,62,88,90,71,34])
DAY = (DAY + [0]*60)[:60]


def glyph(d, ch, x, top, scale, col):
    """One glyph, top-left at (x, top), every set pixel a scale x scale box.
    Rows are drawn as runs, which is how the C would do it too."""
    for r, row in enumerate(DIGITS[ch]):
        c = 0
        while c < GRID_W:
            if row[c] == '#':
                run = 0
                while c + run < GRID_W and row[c + run] == '#':
                    run += 1
                d.rectangle([x + c*scale, top + r*scale,
                             x + (c+run)*scale - 1, top + (r+1)*scale - 1],
                            fill=col)
                c += run
            else:
                c += 1


# name, W, H, margin_l, margin_r, scale, stacked?, b_hour/b_clock, b_min, hz
ROLES = [
    ('emery stacked',  200, 228, 10, 189, 5, True,  106, 182, 188),
    ('emery one line', 200, 228, 10, 189, 4, False, 130, 0,   156),
    ('144 stacked',    144, 168, 12, 131, 3, True,   74, 122, 128),
    ('144 one line',   144, 168, 12, 131, 2, False,  94, 0,   114),
]


def face(role):
    name, W, H, ml, mr, S, stacked, b1, b2, hz = role
    im = Image.new('RGB', (W, H), P['sky']); d = ImageDraw.Draw(im)
    gw, gh = GRID_W*S, GRID_H*S
    cols = [c for c in range(GRID_W) if any(r[c] == '#' for r in DIGITS[':'])]
    c0, c1 = (cols[0], cols[-1]) if cols else (0, 0)
    cw = (c1 - c0 + 1)*S
    # The gap between slots is one grid column, so the digits breathe by the
    # design's own unit rather than by a pixel count that means one thing at
    # x5 and something else entirely at x2. The one line has to hold four of
    # those plus the colon inside the screen, so there it takes what is left.
    air = S
    if not stacked:
        air = min(air, max(1, (W - 8 - 4*gw - cw)//5))
    slot = gw + air

    def txt(s, size, x, base, col, track=None):
        f = ImageFont.truetype(MB, size)
        if track is None:
            b = f.getbbox(s); d.text((x-b[0], base-b[3]), s, font=f, fill=col)
            return b[2]-b[0]
        for c in s:
            b = f.getbbox(c); d.text((x-b[0], base-b[3]), c, font=f, fill=col)
            x += (b[2]-b[0]) + track
        return x

    big, cap, lab = (22, 15, 11) if W == 200 else (16, 11, 9)
    row1 = 24 if W == 200 else 18
    txt('8,842', big, ml, row1, P['accent'])
    w = ImageFont.truetype(MB, big).getbbox('68')
    txt('68', big, mr-(w[2]-w[0]), row1, P['muted'])

    if stacked:
        right = ml + 2*slot
        glyph(d, '9', right - slot, b1 - gh, S, P['ink'])
        glyph(d, '4', right - 2*slot, b2 - gh, S, P['ink'])
        glyph(d, '1', right - slot, b2 - gh, S, P['ink'])
        foot = (b2 - 16 + 44 + row1) // 2
        for s_, base, col, tr in (('30', foot-15, P['muted'], None),
                                  ('THU', foot-36, P['label'], 2),
                                  ('JUL', foot, P['label'], 2)):
            f = ImageFont.truetype(MB, big if s_ == '30' else lab)
            if tr is None:
                txt(s_, big, mr - (f.getbbox(s_)[2]-f.getbbox(s_)[0]), base, col)
            else:
                wd = sum(f.getbbox(c)[2]-f.getbbox(c)[0]+tr for c in s_)-tr
                txt(s_, lab, mr-wd, base, col, tr)
    else:
        # The colon is pinned to the centre and gets a slot of its own, sized
        # to its ink rather than to the grid — it only uses a couple of the
        # eight columns, and padding it to full width would open a gap the
        # real draw_clock_line does not have.
        cslot = cw + air
        cx0 = W//2 - cslot//2
        glyph(d, '9', cx0 - slot, b1 - gh, S, P['ink'])
        glyph(d, ':', cx0 + (cslot - cw)//2 - c0*S, b1 - gh, S,
              P['ink'])
        for i, ch in enumerate('41'):
            glyph(d, ch, cx0 + cslot + i*slot, b1 - gh, S, P['ink'])

    d.rectangle([0, hz, W-1, hz+1], fill=P['horizon'])
    d.rectangle([0, hz+2, W-1, H-1], fill=P['ground'])
    cw_, px = (3, 10) if W == 200 else (2, 12)
    bot, maxh = H-1, (H-1) - (hz+2) - 3
    for i in range(60):
        h = round(min(DAY[i], 90)*maxh/90)
        if h:
            d.rectangle([px+cw_*i, bot-h+1, px+cw_*i+cw_-1, bot],
                        fill=P['ink'] if i == 59 else P['accent'])
    return im, name, GRID_H*S, slot, air


ORDER = '0123456789:'          # ':' lands at DIGIT_COLON, one past the digits


def emit_c():
    """Write src/c/digits.{h,c} from the same dict the previews are drawn from.

    Hand-copying eleven glyphs into C is how one row ends up mirrored and
    nobody notices until it is on a wrist, so the art has exactly one home and
    this is the only thing allowed to translate it.
    """
    cols = [i for i in range(GRID_W)
            if any(DIGITS[':'][r][i] == '#' for r in range(GRID_H))]
    head = '\n'.join([
        '// Generated by tools/digitgrid.py --emit-c. Do not edit: redraw the',
        '// glyphs in that file and run it again.',
        '#pragma once',
        '#include <pebble.h>',
        '',
        '#define DIGIT_W %d' % GRID_W,
        '#define DIGIT_H %d' % GRID_H,
        '#define DIGIT_COLON %d' % ORDER.index(':'),
        '// The colon inks only these columns, so the one-line layout can give it',
        '// a slot of its own rather than a full digit width.',
        '#define DIGIT_COLON_L %d' % cols[0],
        '#define DIGIT_COLON_R %d' % cols[-1],
        '',
        '// One bitmask per row, bit (DIGIT_W-1) is the leftmost column.',
        'extern const uint16_t DIGIT_ROWS[%d][DIGIT_H];' % len(ORDER),
        '',
        '// Every set pixel as a `scale` x `scale` box, top-left at (x, top),',
        '// in the context fill color. Runs are one rectangle, not one per pixel.',
        'void digit_draw(GContext *ctx, int idx, int x, int top, int scale);',
        ''])
    body = ['// Generated by tools/digitgrid.py --emit-c. Do not edit.',
            '#include "digits.h"', '',
            'const uint16_t DIGIT_ROWS[%d][DIGIT_H] = {' % len(ORDER)]
    for ch in ORDER:
        masks = []
        for row in DIGITS[ch]:
            m = 0
            for i, c in enumerate(row):
                if c == '#':
                    m |= 1 << (GRID_W - 1 - i)
            masks.append('0x%03X' % m)
        body.append('  { %s },   // %s' % (', '.join(masks), ch))
    body += ['};', '',
             'void digit_draw(GContext *ctx, int idx, int x, int top, int scale) {',
             '  for (int r = 0; r < DIGIT_H; r++) {',
             '    uint16_t row = DIGIT_ROWS[idx][r];',
             '    int c = 0;',
             '    while (c < DIGIT_W) {',
             '      if (row & (1 << (DIGIT_W - 1 - c))) {',
             '        int run = 0;',
             '        while (c + run < DIGIT_W &&',
             '               (row & (1 << (DIGIT_W - 1 - c - run)))) run++;',
             '        graphics_fill_rect(ctx, GRect(x + c * scale, top + r * scale,',
             '                                      run * scale, scale), 0,',
             '                           GCornerNone);',
             '        c += run;',
             '      } else {',
             '        c++;',
             '      }',
             '    }',
             '  }',
             '}', '']
    src = HERE.parent / 'src' / 'c'
    (src / 'digits.h').write_text(head)
    (src / 'digits.c').write_text('\n'.join(body))
    print('  wrote src/c/digits.h and src/c/digits.c')


def main():
    check()
    if '--emit-c' in sys.argv:
        emit_c()
        return
    Z = 6
    cols = len(DIGITS)
    sheet = Image.new('RGB', (cols*(GRID_W*Z+8)+8, GRID_H*Z+40), (24, 24, 24))
    ds = ImageDraw.Draw(sheet)
    for i, ch in enumerate(sorted(DIGITS)):
        x = 8 + i*(GRID_W*Z+8)
        ds.rectangle([x-1, 7, x+GRID_W*Z, 8+GRID_H*Z], fill=P['sky'])
        glyph(ds, ch, x, 8, Z, P['ink'])
        for g in range(GRID_W+1):
            ds.line([x+g*Z, 8, x+g*Z, 8+GRID_H*Z], fill=(60, 60, 90))
        for g in range(GRID_H+1):
            ds.line([x, 8+g*Z, x+GRID_W*Z, 8+g*Z], fill=(60, 60, 90))
        ds.text((x, GRID_H*Z+18), ch, fill=(180, 180, 200))
    sheet.save(HERE / 'digits_sheet.png')

    ims, pad = [], 8
    for role in ROLES:
        im, name, cap, slot, air = face(role)
        ims.append(im)
        print('  %-15s x%d  cap %2d  slot %2d  air %d'
              % (name, role[5], cap, slot, air))
    wsum = sum(i.width for i in ims) + pad*(len(ims)+1)
    hmax = max(i.height for i in ims) + pad*2
    s = Image.new('RGB', (wsum, hmax), (24, 24, 24))
    x = pad
    for i in ims:
        s.paste(i, (x, pad)); x += i.width + pad
    s.resize((int(s.width*1.7), int(s.height*1.7)), Image.NEAREST).save(
        HERE / 'digits_face.png')
    print('\n  grid %dx%d, %d bytes of glyph data'
          % (GRID_W, GRID_H, len(DIGITS) * GRID_H * (1 if GRID_W <= 8 else 2)))
    print('  wrote tools/digits_sheet.png and tools/digits_face.png')


if __name__ == '__main__':
    main()
