#!/usr/bin/env python3
"""Generate the clock font table: package.json resources and the C metrics.

Seven bundled families x two weights x two layouts x two screens is 56 font
resources and 28 grid rows. Hand-maintaining that is how a size ends up one
pixel off on one platform and nobody notices for a month, so it is derived
here from four constraints and written out.

  emery stacked   cap 68, and the block must clear the day column at x=160
  emery one line  4 slots + a colon inside 188px
  144 stacked     cap 43, and the block must fit 85px
  144 one line    4 slots + a colon inside 132px

Run from the project root:  python3 tools/make_fonts.py
"""
import json
import pathlib
from PIL import ImageFont

R = 'resources/fonts/'

# (settings enum, resource stem, light file, bold file, label)
FAMILIES = [
    ('CF_MONT',    'CLOCK', 'Montserrat-Light.ttf',   'Montserrat-Bold.ttf'),
    ('CF_INTER',   'INTR',  'Inter-Light.ttf',        'Inter-Bold.ttf'),
    ('CF_DSEG',    'DSEG',  'DSEG7Classic-Light.ttf', 'DSEG7Classic-Bold.ttf'),
    ('CF_KODE',    'KODE',  'KodeMono-Regular.ttf',   'KodeMono-Bold.ttf'),
    ('CF_MRTN',    'MRTN',  'MartianMono-Light.ttf',  'MartianMono-Bold.ttf'),
    # Jersey 25 ships one weight only, so bold and light are the same file and
    # the bold toggle has nothing to say about it — as with LECO.
    ('CF_JRSY',    'JRSY',  'Jersey25-Regular.ttf',   'Jersey25-Regular.ttf'),
]

SMALL = ['basalt', 'diorite', 'flint']

_c = {}
def F(p, s):
    if (p, s) not in _c:
        _c[(p, s)] = ImageFont.truetype(R + p, s)
    return _c[(p, s)]

def ink(f, s):
    b = f.getbbox(s); return b[2] - b[0], b[3] - b[1]

def slot(f):
    return max(ink(f, str(d))[0] for d in range(10)) + 2


# Pebble caps a single glyph's packed bitmap: (w*h+7)//8 must fit
# MAX_FONT_GLYPH_SIZE, which is 512 bytes on Emery and 256 on the 144x168
# watches. That ceiling bites before the screen does for the fatter faces.
#
# These are not modelled. PIL's raster is fatter than the SDK's by an amount
# that is not constant — two attempts to calibrate it produced sizes that still
# failed to build — so they were bisected using the SDK's own font.fontgen,
# which is ground truth. Re-derive with tools/probe_glyph_ceiling.py if a face
# is added or the SDK changes.
CEILING = {                     # family stem -> (emery 512B, 144x168 256B)
    'CLOCK': (94, 66), 'INTR': (96, 67), 'DSEG': (81, 57),
    'KODE': (89, 63), 'MRTN': (87, 62), 'JRSY': (130, 91),
}


def for_cap(light, bold, cap, block_max, budget):
    """Largest size whose *shorter* weight still reaches `cap`, that fits."""
    best = None
    for sz in range(16, 200):
        h = min(ink(F(light, sz), '8')[1], ink(F(bold, sz), '8')[1])
        w = 2 * max(slot(F(light, sz)), slot(F(bold, sz)))
        if h <= cap and w <= block_max and sz <= budget:
            best = sz
        if h > cap + 6:
            break
    return best


def for_line(light, bold, room, budget):
    best = None
    for sz in range(14, 140):
        s = max(slot(F(light, sz)), slot(F(bold, sz)))
        c = max(ink(F(light, sz), ':')[0], ink(F(bold, sz), ':')[0])
        if 4 * s + c + 8 <= room and sz <= budget:
            best = sz
    return best


def main():
    rows = []
    for enum, stem, l, b in FAMILIES:
        e_stack = for_cap(l, b, 68, 142, CEILING[stem][0])  # day col at x=160
        e_line = for_line(l, b, 188, CEILING[stem][0])
        s_stack = for_cap(l, b, 43, 85, CEILING[stem][1])
        s_line = for_line(l, b, 132, CEILING[stem][1])
        rows.append((enum, stem, l, b, e_stack, e_line, s_stack, s_line))
        print('  %-11s emery %3d/%-3d   144 %3d/%-3d' %
              (enum, e_stack, e_line, s_stack, s_line))

    # --- package.json --------------------------------------------------------
    p = pathlib.Path('package.json')
    d = json.loads(p.read_text())
    media = d['pebble']['resources']['media']
    media[:] = [m for m in media
                if m['type'] != 'font' or not any(
                    m['name'].startswith('FONT_' + s + '_') or
                    m['name'].startswith('FONT_' + s + '_B_')
                    for _, s, *_ in FAMILIES)]
    for enum, stem, l, b, es, el, ss, sl in rows:
        for size, plats in ((es, ['emery']), (el, ['emery']),
                            (ss, SMALL), (sl, SMALL)):
            for weight, f in (('', l), ('B_', b)):
                media.append({
                    'type': 'font',
                    'name': 'FONT_%s_%s%d' % (stem, weight, size),
                    'file': 'fonts/' + f,
                    'characterRegex': '[0-9:]',
                    'targetPlatforms': plats,
                })
    # de-duplicate: a family may land on the same size for two roles
    seen, uniq = set(), []
    for m in media:
        k = (m.get('name'), tuple(m.get('targetPlatforms') or []))
        if k in seen:
            continue
        seen.add(k); uniq.append(m)
    media[:] = uniq
    p.write_text(json.dumps(d, indent=2) + '\n')
    n = sum(1 for m in media if m['type'] == 'font')
    print('\npackage.json: %d font resources' % n)

    print('\nThe metrics grid in face.c carries these sizes by hand: the'
          '\nbaselines differ per layout and are a design choice, not a'
          '\nderivation. Only the sizes above are computed.')


if __name__ == '__main__':
    main()
