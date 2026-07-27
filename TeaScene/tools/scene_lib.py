#!/usr/bin/env python3
"""Drawing primitives for the TeaScene indexed-pixel canvas.

The art is authored once as an index map; four palettes recolor it for the
times of day. Every palette entry must be a legal Pebble 64-color value —
each channel one of 0/85/170/255 — so the generator snaps and complains
rather than letting a colour drift off-gamut silently.
"""

W, H = 200, 172

BAYER4 = [[0, 8, 2, 10],
          [12, 4, 14, 6],
          [3, 11, 1, 9],
          [15, 7, 13, 5]]


def snap(c):
    """Nearest legal Pebble channel triple."""
    lv = (0, 85, 170, 255)
    return tuple(min(lv, key=lambda l: abs(l - v)) for v in c)


def argb8(c):
    """Pebble GColor8 byte: alpha 3, then two bits per channel."""
    r, g, b = snap(c)
    return 0xC0 | ((r // 85) << 4) | ((g // 85) << 2) | (b // 85)


class Art:
    def __init__(self, w=W, h=H, bg=0):
        self.w, self.h = w, h
        self.px = [[bg] * w for _ in range(h)]

    # --- primitives ------------------------------------------------------
    def put(self, x, y, i):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = i

    def get(self, x, y, default=0):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.px[y][x]
        return default

    def hline(self, x0, x1, y, i):
        if x1 < x0:
            x0, x1 = x1, x0
        for x in range(x0, x1 + 1):
            self.put(x, y, i)

    def vline(self, x, y0, y1, i):
        if y1 < y0:
            y0, y1 = y1, y0
        for y in range(y0, y1 + 1):
            self.put(x, y, i)

    def rect(self, x0, y0, x1, y1, i):
        for y in range(y0, y1 + 1):
            self.hline(x0, x1, y, i)

    def line(self, x0, y0, x1, y1, i):
        dx, dy = abs(x1 - x0), abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        while True:
            self.put(x0, y0, i)
            if x0 == x1 and y0 == y1:
                break
            e2 = err * 2
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    # --- dithering -------------------------------------------------------
    def dith(self, x, y, a, b, t):
        """Place `a` with probability t (ordered 4x4), else `b`."""
        self.put(x, y, a if BAYER4[y & 3][x & 3] < t * 16 else b)

    def band(self, x, y, lv, ramp, seam=0.20):
        """Hard-banded shading with a thin dithered seam between bands —
        flat colour is the point; the dither is only there to soften the
        step, never to carry the gradient itself."""
        lv = max(0.0, min(len(ramp) - 1.001, lv))
        i = int(round(lv))
        f = lv - i
        if f > 0.5 - seam and i + 1 < len(ramp):
            self.dith(x, y, ramp[i + 1], ramp[i], (f - 0.5 + seam) / (2 * seam))
        elif f < -0.5 + seam and i - 1 >= 0:
            self.dith(x, y, ramp[i - 1], ramp[i], (-0.5 + seam - f) / (2 * seam))
        else:
            self.put(x, y, ramp[i])

    def dith_rect(self, x0, y0, x1, y1, a, b, t):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.dith(x, y, a, b, t)

    def vgrad(self, x0, y0, x1, y1, stops):
        """Vertical gradient. stops = [(y, idx), ...] top to bottom; bands
        blend into each other with an ordered dither."""
        for y in range(y0, y1 + 1):
            lo = stops[0]
            hi = stops[-1]
            for k in range(len(stops) - 1):
                if stops[k][0] <= y <= stops[k + 1][0]:
                    lo, hi = stops[k], stops[k + 1]
                    break
            span = max(1, hi[0] - lo[0])
            t = (y - lo[0]) / span
            t = max(0.0, min(1.0, t))
            for x in range(x0, x1 + 1):
                self.dith(x, y, hi[1], lo[1], t)

    # --- shapes ----------------------------------------------------------
    def ellipse(self, cx, cy, rx, ry, i, clip=None):
        rx = max(rx, 0.5)
        ry = max(ry, 0.5)
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            dy = (y - cy) / ry
            if abs(dy) > 1:
                continue
            hw = rx * (1 - dy * dy) ** 0.5
            for x in range(int(cx - hw + 0.5), int(cx + hw + 0.5) + 1):
                if clip and not clip(x, y):
                    continue
                self.put(x, y, i)

    def ellipse_ring(self, cx, cy, rx, ry, i, th=1):
        inner = set()
        for y in range(int(cy - ry - 2), int(cy + ry + 3)):
            dy = (y - cy) / max(ry - th, 0.4)
            if abs(dy) <= 1:
                hw = max(rx - th, 0.4) * (1 - dy * dy) ** 0.5
                for x in range(int(cx - hw + 0.5), int(cx + hw + 0.5) + 1):
                    inner.add((x, y))
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            dy = (y - cy) / ry
            if abs(dy) > 1:
                continue
            hw = rx * (1 - dy * dy) ** 0.5
            for x in range(int(cx - hw + 0.5), int(cx + hw + 0.5) + 1):
                if (x, y) not in inner:
                    self.put(x, y, i)

    def span(self, cx, hw, y, i):
        for x in range(int(round(cx - hw)), int(round(cx + hw)) + 1):
            self.put(x, y, i)


# ---------------------------------------------------------------------------
# hybrid RLE: repeat runs and literal runs, one stream per row
# ---------------------------------------------------------------------------
def encode_row(row):
    out = bytearray()
    i, n = 0, len(row)
    lit = []

    def flush_lit():
        while lit:
            take = lit[:128]
            del lit[:128]
            out.append(0x80 | (len(take) - 1))
            out.extend(take)

    while i < n:
        j = i
        while j + 1 < n and row[j + 1] == row[i] and j - i < 127:
            j += 1
        run = j - i + 1
        if run >= 3:
            flush_lit()
            out.append(run - 1)
            out.append(row[i])
        else:
            lit.extend(row[i:i + run])
        i += run
    flush_lit()
    return bytes(out)


def encode(art):
    """Resource layout: u16 w, u16 h, u16 rows[h] offsets (from body start),
    u16 lens[h], then the row streams."""
    rows = [encode_row(r) for r in art.px]
    head = bytearray()
    head += art.w.to_bytes(2, 'little')
    head += art.h.to_bytes(2, 'little')
    off = 0
    offs, lens = bytearray(), bytearray()
    for r in rows:
        offs += off.to_bytes(2, 'little')
        lens += len(r).to_bytes(2, 'little')
        off += len(r)
    body = b''.join(rows)
    # the decoder keeps the row table as uint16, and reads a row into a fixed
    # 320-byte buffer — fail here rather than render garbage on the watch
    assert off <= 0xFFFF, 'art too large for uint16 row offsets: %d' % off
    assert max(len(r) for r in rows) <= 320, 'a row exceeds the read buffer'
    return bytes(head + offs + lens + body)
