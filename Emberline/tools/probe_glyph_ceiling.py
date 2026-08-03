import sys, os, json
sys.path.insert(0, os.path.expanduser('~/Library/Application Support/Pebble SDK/SDKs/4.17/sdk-core/pebble/common/tools'))
from font.fontgen import Font
R = '/Users/zach/Programming/Pebble/Sparkline_Faces/Emberline/resources/fonts/'
CPS = [ord(c) for c in '0123456789:'] + [0x25AF]

def ok(ttf, size, budget):
    try:
        f = Font(R+ttf, size, 256, budget, False)
        for cp in CPS:
            f.glyph_bits(cp, f.face.get_char_index(cp))
        return True
    except Exception:
        return False

def maxsize(l, b, budget):
    lo, hi = 20, 140
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if ok(l, mid, budget) and ok(b, mid, budget): lo = mid
        else: hi = mid - 1
    return lo

FAM = [('CLOCK','Montserrat-Light.ttf','Montserrat-Bold.ttf'),
       ('ROBO','Roboto-Light.ttf','Roboto-Bold.ttf'),
       ('GROT','SpaceGrotesk-Light.ttf','SpaceGrotesk-Bold.ttf'),
       ('INTR','Inter-Light.ttf','Inter-Bold.ttf'),
       ('SRCE','SourceSans3-Light.ttf','SourceSans3-Bold.ttf'),
       ('PLEX','IBMPlexMono-Light.ttf','IBMPlexMono-Bold.ttf'),
       ('DSEG','DSEG7Classic-Light.ttf','DSEG7Classic-Bold.ttf')]
res = {}
print('  %-7s %11s %9s' % ('family','emery(512)','144(256)'))
for stem,l,b in FAM:
    e, s = maxsize(l,b,512), maxsize(l,b,256)
    res[stem] = [e, s]
    print('  %-7s %11d %9d' % (stem, e, s), flush=True)
json.dump(res, open('/tmp/ceilings.json','w'))
