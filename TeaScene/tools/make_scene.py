#!/usr/bin/env python3
"""TeaScene — the pixel art beneath the watchface chrome.

A gongfu tea table in a Jiangnan tea studio: pale plaster in dark hardwood
framing, a slatted bamboo cha pan, a gaiwan on its saucer, two cups, a leaf
dish, a folded towel, a kettle on a brazier, and a round moon window. Authored
as a 200x172 index map (Emery's screen minus the 56px info band at the top);
four palettes recolor it through the day.

    python3 tools/make_scene.py          # writes resource + header + previews

Two rules the 64-colour screen imposes, learned the hard way:
  * flat colour, hard edges. A dither spread over an area is noise, not a
    gradient — dithering is only ever a one- or two-pixel seam between bands.
  * value carries the picture. Dark frame, pale wall, mid tray, light
    porcelain, so everything still reads when the colours all go amber.
"""
import os
import sys
import math

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from scene_lib import Art, W, H, encode, argb8, snap        # noqa: E402
from palettes import *                                       # noqa: E402,F403
import palettes as P                                         # noqa: E402

ROOT = os.path.dirname(HERE)

# --- the composition, in one place -----------------------------------------
WALL_BOT = 96               # plaster ends, the table surface begins
RAIL_Y = 90                 # the panelling's bottom rail
POSTS = (0, 56, 118, 197)   # dark stiles; the panels fall between them

WIN_CX, WIN_CY, WIN_R = 158, 44, 30
SCROLL_X0, SCROLL_X1 = 12, 44

TRAY_Y0, TRAY_Y1 = 106, 164     # top face
TRAY_EDGE_Y1 = 170              # front face bottom
TRAY_BX0, TRAY_BX1 = 22, 178    # back edge span
TRAY_FX0, TRAY_FX1 = 4, 196     # front edge span

GAIWAN_CX = 68
CUPA_CX, CUPA_BASE, CUPA_RW = 116, 150, 7.5
CUPB_CX, CUPB_BASE, CUPB_RW = 138, 159, 8.5
CHAHE_CX, CHAHE_CY = 26, 157

# One camera for the whole scene: every opening is this much taller than a
# line. Mixing ratios is what made the first pass look like a bowl seen
# edge-on standing on a tray seen from above.
TILT = 0.33


def tray_span(y):
    t = (y - TRAY_Y0) / float(TRAY_Y1 - TRAY_Y0)
    return (TRAY_BX0 + (TRAY_FX0 - TRAY_BX0) * t,
            TRAY_BX1 + (TRAY_FX1 - TRAY_BX1) * t)


# ---------------------------------------------------------------------------
# soft edges: a solid core and one sparse ring. Never a 50% field.
# ---------------------------------------------------------------------------
def shadow_blob(a, cx, cy, rx, ry, idx, only=None, core=0.94, t_out=0.0):
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
            if not (0 <= x < a.w and 0 <= y < a.h):
                continue
            cur = a.px[y][x]
            if only is not None and cur not in only:
                continue
            d = math.hypot((x - cx) / float(rx), (y - cy) / float(ry))
            if d > 1:
                continue
            if d <= core:
                a.put(x, y, idx)
            else:
                a.dith(x, y, idx, cur, t_out)


def quad(a, pts, idx, only=None):
    """Fill a convex quad — used for the hard-edged shaft of window light."""
    ys = [p[1] for p in pts]
    for y in range(int(min(ys)), int(max(ys)) + 1):
        xs = []
        for k in range(len(pts)):
            x0, y0 = pts[k]
            x1, y1 = pts[(k + 1) % len(pts)]
            if y0 == y1:
                continue
            if min(y0, y1) <= y <= max(y0, y1):
                xs.append(x0 + (x1 - x0) * (y - y0) / float(y1 - y0))
        if len(xs) < 2:
            continue
        for x in range(int(round(min(xs))), int(round(max(xs))) + 1):
            if 0 <= x < a.w and 0 <= y < a.h:
                if only is None or a.px[y][x] in only:
                    a.put(x, y, idx)


# ---------------------------------------------------------------------------
# the wall: pale plaster in dark hardwood framing
# ---------------------------------------------------------------------------
def draw_wall(a):
    a.rect(0, 0, W - 1, WALL_BOT - 1, WALL_M)
    a.rect(0, 0, W - 1, 3, WALL_D)                    # head rail
    a.hline(0, W - 1, 4, WALL_G)                      # its shadow on the wall
    a.rect(0, RAIL_Y, W - 1, WALL_BOT - 1, WALL_D)    # bottom rail
    a.hline(0, W - 1, RAIL_Y, WALL_L)
    for x in POSTS:                                   # stiles
        a.rect(x, 4, x + 2, RAIL_Y - 1, WALL_D)
        if x + 3 < W:
            a.vline(x + 3, 4, RAIL_Y - 1, WALL_L)     # lit on the window side
        if x - 1 >= 0:
            a.vline(x - 1, 4, RAIL_Y - 1, WALL_G)     # shadow cast away from it

    # a bead moulding inset in each panel — joinery, and it keeps the pale
    # fields from reading as blank paper
    for k in range(1, len(POSTS) - 1):
        px0, px1 = POSTS[k] + 3, POSTS[k + 1] - 1
        a.rect(px0 + 3, 8, px1 - 3, 8, WALL_G)
        a.rect(px0 + 3, RAIL_Y - 4, px1 - 3, RAIL_Y - 4, WALL_L)
        a.vline(px0 + 3, 8, RAIL_Y - 4, WALL_L)
        a.vline(px1 - 3, 8, RAIL_Y - 4, WALL_G)

    # the table surface between the wall and the tray
    a.rect(0, WALL_BOT, W - 1, TRAY_Y0 - 1, FLOOR_D)
    a.hline(0, W - 1, WALL_BOT, WALL_D)


def draw_shelf(a):
    """A small wall shelf with a tea jar, in the middle panel. Without it
    that panel is 60x80 of blank plaster right where the eye lands."""
    x0, x1, y = 66, 110, 50
    a.rect(x0, y, x1, y + 2, WALL_D)                  # the board
    a.hline(x0, x1, y, WALL_L)
    a.rect(x0 + 4, y + 3, x0 + 6, y + 7, WALL_D)      # two brackets
    a.rect(x1 - 6, y + 3, x1 - 4, y + 7, WALL_D)
    a.hline(x0, x1, y + 3, WALL_G)                    # shadow under the board

    # a lidded tea jar standing on it. At 18px across, five ramp levels is
    # stripes — three, plus an outline the whole way round so the body cannot
    # dissolve into plaster of its own colour.
    jx = x0 + 14
    for yy in range(y - 15, y):
        hw = 8.0 - 2.0 * abs((yy - (y - 9)) / 7.0) ** 1.8
        xl, xr = int(round(jx - hw)), int(round(jx + hw))
        for x in range(xl, xr + 1):
            u = (x - jx) / hw
            a.put(x, yy, POR_L if 0.3 < u < 0.75 else
                         (POR_M if u > -0.4 else POR_S))
        a.put(xl, yy, POR_O)
        a.put(xr, yy, POR_O)
    flat_ellipse(a, jx, y - 16, 6.0, 2.2, 2.1, 3.0)
    ring(a, jx, y - 16, 6.0, 2.2, POR_O)
    a.put(jx, y - 18, POR_L)
    a.put(jx, y - 17, POR_M)
    for x in range(jx - 6, jx + 7):                   # a band that curves
        du = (x - jx) / 6.5
        a.put(x, y - 12 + int(round(1.4 * math.sqrt(max(0.0, 1 - du * du)))),
              POR_LN)

    # two cups stacked, clear of the jar and divided so they read as two
    cx2 = x1 - 11
    for yy in range(y - 9, y):
        hw = 6.5 - (yy - (y - 9)) * 0.12
        xl, xr = int(round(cx2 - hw)), int(round(cx2 + hw))
        for x in range(xl, xr + 1):
            u = (x - cx2) / hw
            a.put(x, yy, POR_L if 0.3 < u < 0.75 else
                         (POR_M if u > -0.4 else POR_S))
        a.put(xl, yy, POR_O)
        a.put(xr, yy, POR_O)
    a.hline(cx2 - 6, cx2 + 6, y - 5, POR_O)
    flat_ellipse(a, cx2, y - 10, 6.5, 2.3, 2.1, 3.0)
    ring(a, cx2, y - 10, 6.5, 2.3, POR_O)


# ---------------------------------------------------------------------------
# the round window
# ---------------------------------------------------------------------------
def draw_window(a):
    cx, cy, r = WIN_CX, WIN_CY, WIN_R
    inner = r - 4

    def in_sky(x, y):
        return math.hypot(x - cx, y - cy) <= inner + 0.3

    # sky: three flat bands
    for y in range(cy - inner - 1, cy + inner + 2):
        t = (y - (cy - inner)) / float(2 * inner)
        for x in range(cx - inner - 1, cx + inner + 2):
            if in_sky(x, y):
                a.band(x, y, t * 2.0, [SKY_T, SKY_M, SKY_B], seam=0.05)

    # sun / moon: a solid disc with one solid halo step
    dx, dy = cx + 11, cy - 14
    for y in range(dy - 11, dy + 12):
        for x in range(dx - 11, dx + 12):
            if not in_sky(x, y):
                continue
            d = math.hypot(x - dx, y - dy)
            if d <= 10.5:
                a.put(x, y, DISC_H)
    a.ellipse(dx, dy, 6.2, 6.2, DISC, clip=in_sky)

    # two ridges near the horizon
    for x in range(cx - inner - 1, cx + inner + 2):
        u = x - cx
        far = cy + 15 - 8 * math.exp(-((u + 11) / 12.0) ** 2) \
                      - 5 * math.exp(-((u - 12) / 9.0) ** 2)
        near = cy + 20 - 5 * math.exp(-((u - 1) / 15.0) ** 2)
        for y in range(int(far), cy + inner + 2):
            if in_sky(x, y):
                a.put(x, y, HILL_F)
        for y in range(int(near), cy + inner + 2):
            if in_sky(x, y):
                a.put(x, y, HILL_N)

    # a bamboo stand outside
    for st_x, top in ((cx - 20, cy - 12), (cx - 15, cy - 4)):
        for y in range(top, cy + inner + 2):
            if in_sky(st_x, y):
                a.put(st_x, y, BAMB_D)
            if in_sky(st_x + 1, y):
                a.put(st_x + 1, y, BAMB_L)
    for st_x, top in ((cx - 20, cy - 12), (cx - 15, cy - 4)):
        for ny in (cy - 4, cy + 6):                # culm nodes
            if in_sky(st_x, ny):
                a.put(st_x, ny, BAMB_D)
                a.put(st_x + 1, ny, BAMB_D)
    for (lx, ly, ex, ey) in ((cx - 20, cy - 11, cx - 27, cy - 17),
                             (cx - 19, cy - 8, cx - 10, cy - 14),
                             (cx - 15, cy - 3, cx - 23, cy - 8),
                             (cx - 14, cy - 1, cx - 6, cy - 6)):
        leaf_stroke(a, lx, ly, ex, ey, BAMB_L, 1.8, clip=in_sky)

    # Lattice: asymmetric muntins with a couple of cracked-ice chords. A
    # centred cross with a ring around it is a rifle scope, not a window.
    for x in (cx - 11, cx + 8):
        for y in range(cy - inner, cy + inner + 1):
            if in_sky(x, y):
                a.put(x, y, LATT)
    for y in (cy + 9,):
        for x in range(cx - inner, cx + inner + 1):
            if in_sky(x, y):
                a.put(x, y, LATT)
    for (x0, y0, x1, y1) in ((cx - 11, cy - 14, cx + 8, cy - 21),
                             (cx - 11, cy - 3, cx - 24, cy - 12),
                             (cx + 8, cy - 8, cx + 23, cy - 3)):
        a.line(x0, y0, x1, y1, LATT)

    # frame: dark hardwood, lit on the upper right
    for y in range(cy - r - 1, cy + r + 2):
        for x in range(cx - r - 1, cx + r + 2):
            d = math.hypot(x - cx, y - cy)
            if d > r + 0.4 or d < inner + 0.3:
                continue
            ang = math.atan2(-(y - cy), x - cx)
            lit = math.cos(ang - math.radians(55))
            a.put(x, y, FRM_L if lit > 0.5 else
                        (FRM_M if lit > -0.4 else FRM_D))
    for y in range(cy - r, cy + r + 1):                # inner lip, all round
        for x in range(cx - r, cx + r + 1):
            if inner + 0.3 <= math.hypot(x - cx, y - cy) < inner + 1.3:
                a.put(x, y, FRM_D)
    # its shadow on the plaster: down-left, hugging the frame
    for y in range(cy - r - 4, cy + r + 5):
        for x in range(cx - r - 5, cx + r + 5):
            d = math.hypot(x - (cx - 2), y - (cy + 2))
            if x - cx > (y - cy) + 4:            # lower-left arc only
                continue
            if r < d <= r + 2.2 and a.get(x, y) in (WALL_M, WALL_L, WALL_G):
                a.put(x, y, WALL_G)


# ---------------------------------------------------------------------------
# ink strokes — a tapered brush line
# ---------------------------------------------------------------------------
def leaf_stroke(a, x0, y0, x1, y1, idx, wmax=2.2, clip=None):
    n = int(max(abs(x1 - x0), abs(y1 - y0)) * 3) + 6
    for k in range(n + 1):
        t = k / float(n)
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        w = wmax * min(1.0, t * 4.5) * (1.0 - t) ** 0.45
        for oy in range(int(-w), int(w) + 1):
            for ox in range(int(-w), int(w) + 1):
                if ox * ox + oy * oy <= w * w:
                    px, py = int(round(x)) + ox, int(round(y)) + oy
                    if clip and not clip(px, py):
                        continue
                    a.put(px, py, idx)


# ---------------------------------------------------------------------------
# the hanging scroll — mounting silk, rods with knobs, ink bamboo, a seal
# ---------------------------------------------------------------------------
def draw_scroll(a):
    x0, x1 = SCROLL_X0, SCROLL_X1
    # shadow down-left, the same direction as everything else in the room
    a.rect(x0 - 6, 12, x0 - 4, 84, WALL_G)
    a.rect(x0 - 6, 82, x1 + 2, 84, WALL_G)

    a.vline((x0 + x1) // 2, 4, 7, INK)                # hanging cord

    a.rect(x0 - 3, 7, x1 + 3, 12, ROD)                # top rod
    a.hline(x0 - 3, x1 + 3, 8, ROD_L)
    a.rect(x0, 12, x1, 76, PAPER_S)                   # mounting silk
    a.rect(x0 + 4, 19, x1 - 4, 69, PAPER)             # the paper itself
    a.rect(x0 - 5, 76, x1 + 5, 81, ROD)               # bottom rod, overhanging
    a.hline(x0 - 5, x1 + 5, 77, ROD_L)
    for kx in (x0 - 7, x1 + 5):                       # its knobs
        a.rect(kx, 76, kx + 2, 81, ROD_L)
        a.put(kx + 1, 78, ROD)

    # bamboo: a culm with nodes, leaves in threes off short side branches
    st = x0 + 10
    a.vline(st, 23, 66, INK)
    a.vline(st + 1, 23, 66, INK_S)
    for ny in (32, 44, 56):
        a.hline(st - 2, st + 3, ny, INK)
    def on_paper(px, py):
        return x0 + 4 <= px <= x1 - 4 and 19 <= py <= 69

    # Wet strokes in front, the thinner wash set well behind them — paired
    # one under each dark leaf, they read as motion blur rather than ink.
    leaf_stroke(a, st - 2, 30, st - 13, 22, INK_S, 1.7, clip=on_paper)
    leaf_stroke(a, st + 3, 41, st + 15, 46, INK_S, 1.6, clip=on_paper)
    leaf_stroke(a, st - 2, 53, st - 12, 60, INK_S, 1.6, clip=on_paper)
    leaf_stroke(a, st + 2, 33, st + 14, 24, INK, 2.0, clip=on_paper)
    leaf_stroke(a, st + 2, 34, st + 16, 35, INK, 1.9, clip=on_paper)
    leaf_stroke(a, st - 1, 46, st - 12, 38, INK, 1.9, clip=on_paper)
    leaf_stroke(a, st - 1, 47, st - 11, 52, INK, 1.7, clip=on_paper)
    leaf_stroke(a, st + 2, 58, st + 14, 52, INK, 1.9, clip=on_paper)

    for k in range(8):                                # a column of characters
        yy = 25 + k * 5
        a.hline(x1 - 8, x1 - 6, yy, INK)
        a.put(x1 - 7, yy + 1, INK)
        if k % 3 == 1:
            a.hline(x1 - 8, x1 - 6, yy + 2, INK)
    a.rect(x1 - 9, 61, x1 - 5, 66, SEAL)              # the one pure red
    a.put(x1 - 8, 62, PAPER)                          # carved, not a symbol
    a.put(x1 - 7, 63, PAPER)
    a.put(x1 - 6, 64, PAPER)
    a.put(x1 - 8, 65, PAPER)


# ---------------------------------------------------------------------------
# the cha pan — a bamboo tray with a raised rim, open slat gaps and a drain
# ---------------------------------------------------------------------------
def draw_tray(a):
    a.rect(0, TRAY_Y0, W - 1, H - 1, FLOOR_D)     # the table it stands on

    for y in range(TRAY_Y0, TRAY_Y1 + 1):
        xl, xr = tray_span(y)
        a.hline(int(math.ceil(xl)), int(math.floor(xr)), y, TRAY_T)

    # Slats. Contrast opens up toward the viewer: at the back the gap is only
    # one step below the face, at the front it is a real black opening. Six
    # equally hard stripes across half the frame is agitation, not texture.
    n = 7
    for k in range(1, n):
        t = (k / float(n)) ** 1.4
        y = TRAY_Y0 + 4 + int((TRAY_Y1 - TRAY_Y0 - 6) * t)
        near = k >= 4
        for d, idx in ((0, TRAY_G if near else TRAY_S),
                       (1, TRAY_S if near else TRAY_T),
                       (2, TRAY_L)):
            xl, xr = tray_span(y + d)
            a.hline(int(math.ceil(xl)) + 4, int(math.floor(xr)) - 4, y + d, idx)

    # raised rim all the way round, drawn over the slats so they end on it
    for y in range(TRAY_Y0, TRAY_Y1 + 1):
        xl, xr = tray_span(y)
        li, ri = int(math.ceil(xl)), int(math.floor(xr))
        a.put(li, y, TRAY_S)
        a.put(li + 1, y, TRAY_T)
        a.put(li + 2, y, TRAY_L)
        a.put(ri, y, TRAY_S)
        a.put(ri - 1, y, TRAY_L)
        a.put(ri - 2, y, TRAY_T)
    for d in range(3):
        xl, xr = tray_span(TRAY_Y0 + d)
        a.hline(int(math.ceil(xl)), int(math.floor(xr)), TRAY_Y0 + d,
                (TRAY_S, TRAY_L, TRAY_T)[d])

    # The shaft of sun, applied last and only to unlit slat faces, so the
    # slat crowns keep their own value and the shaft stays one clean shape.
    # It has to reach the gaiwan: a key light that misses the hero is a
    # contradiction you feel without being able to name it.
    LIT = [(112, TRAY_Y0), (190, TRAY_Y0), (150, TRAY_Y1), (66, TRAY_Y1)]
    quad(a, LIT, TRAY_LIT, only={TRAY_T})
    for sx in (0, -20):                                # muntins, projected
        quad(a, [(152 + sx, TRAY_Y0), (154 + sx, TRAY_Y0),
                 (130 + sx, TRAY_Y1), (128 + sx, TRAY_Y1)],
             TRAY_T, only={TRAY_LIT})
    quad(a, [(196, 132), (196, 134), (66, 143), (66, 141)],
         TRAY_T, only={TRAY_LIT})

    # the drain, front left — the whole point of a cha pan, and it balances
    # the kettle in the opposite corner
    a.ellipse(30, 137, 5.0, 2.4, TRAY_S)
    a.ellipse(30, 137.4, 3.6, 1.6, TRAY_G)

    a.rect(0, TRAY_Y1 + 1, W - 1, TRAY_EDGE_Y1, TRAY_E)
    a.hline(0, W - 1, TRAY_Y1 + 1, TRAY_EL)
    a.rect(0, TRAY_EDGE_Y1 + 1, W - 1, H - 1, UNDER)


TRAY_BG = {TRAY_T, TRAY_L, TRAY_S, TRAY_G, TRAY_LIT}


# ---------------------------------------------------------------------------
# porcelain — bodies of revolution, lit from the window on the right
# ---------------------------------------------------------------------------
POR_RAMP = [POR_D, POR_S, POR_M, POR_L, POR_H]
SQ = math.sqrt(0.5)
SPEC_LO, SPEC_HI = 0.50, 0.84


def por_level(u, occ=0.0):
    """Diffuse on a cylinder lit from the upper right at 45 degrees. The
    diffuse term only reaches POR_L; the specular is a separate narrow band,
    because a highlight driven off cos(theta) alone spreads over half the
    vessel and the glaze stops reading as glaze. The constant is bounce off
    the tray — without it the shadow side eats a third of every pot."""
    u = max(-1.0, min(1.0, u))
    diff = max(0.0, u * SQ + math.sqrt(max(0.0, 1 - u * u)) * SQ)
    return 0.45 + diff * 3.0 - occ


def por_row(a, cx, hw, y, occ=0.0, spec=1.0, outline=True, bounce=False,
            x_from=None, x_to=None, seam=0.2):
    xl = int(round(cx - hw)) if x_from is None else x_from
    xr = int(round(cx + hw)) if x_to is None else x_to
    if xr < xl:
        return
    for x in range(xl, xr + 1):
        u = (x - cx) / max(hw, 0.6)
        if spec > 0.5 and SPEC_LO <= u <= SPEC_HI and occ < 0.9:
            a.put(x, y, POR_H)
        else:
            a.band(x, y, min(3.42, por_level(u, occ)), POR_RAMP, seam=seam)
    if xr > xl and outline:
        a.put(xl, y, POR_O)
        # the cool bounce lives just inside the shadow edge, where a rim
        # light on the lit edge would have been invisible against POR_H
        if bounce and xr > xl + 1:
            a.put(xl + 1, y, POR_R)


def revolve(a, cx, y0, y1, hwf, occf=None, specf=None, bouncef=None,
            outline=True):
    for y in range(y0, y1 + 1):
        hw = hwf(y)
        if hw is None or hw <= 0:
            continue
        por_row(a, cx, hw, y,
                occ=occf(y) if occf else 0.0,
                spec=specf(y) if specf else 1.0,
                bounce=bouncef(y) if bouncef else False,
                outline=outline)


def por_ellipse(a, cx, cy, rx, ry, y0=None, y1=None, occ=0.0, spec=1.0,
                outline=True, seam=0.2):
    lo = int(math.ceil(cy - ry)) if y0 is None else y0
    hi = int(math.floor(cy + ry)) if y1 is None else y1
    for y in range(lo, hi + 1):
        dy = (y - cy) / float(ry)
        if abs(dy) > 1:
            continue
        hw = rx * math.sqrt(max(0.0, 1 - dy * dy))
        if hw < 0.6:
            continue
        por_row(a, cx, hw, y, occ=occ, spec=spec, outline=outline, seam=seam)


def flat_ellipse(a, cx, cy, rx, ry, lo_lv=2.5, hi_lv=3.5, front=None):
    """A horizontal surface — a saucer, a lid disc, a tea surface. Its normal
    points at the ceiling, so it is nearly uniform: a gentle back-to-front
    step, and no specular lobe pretending it is a sphere."""
    for y in range(int(math.ceil(cy - ry)), int(math.floor(cy + ry)) + 1):
        dy = (y - cy) / float(ry)
        if abs(dy) > 1:
            continue
        hw = rx * math.sqrt(max(0.0, 1 - dy * dy))
        lv = lo_lv + (hi_lv - lo_lv) * (0.5 + 0.5 * dy)
        for x in range(int(round(cx - hw)), int(round(cx + hw)) + 1):
            a.band(x, y, lv, POR_RAMP, seam=0.0)
    if front is not None:
        for y in range(int(math.ceil(cy - ry)), int(math.floor(cy + ry)) + 1):
            dy = (y - cy) / float(ry)
            if abs(dy) > 1 or dy < -0.2:
                continue
            hw = rx * math.sqrt(max(0.0, 1 - dy * dy))
            a.put(int(round(cx - hw)), y, front)
            a.put(int(round(cx + hw)), y, front)


def ring(a, cx, cy, rx, ry, idx):
    """A closed outline on a flat ellipse. Walking rows alone leaves the top
    and bottom arcs as dashes, which is what made the saucer look stitched."""
    for y in range(int(math.ceil(cy - ry)), int(math.floor(cy + ry)) + 1):
        dy = (y - cy) / float(ry)
        if abs(dy) > 1:
            continue
        hw = rx * math.sqrt(max(0.0, 1 - dy * dy))
        a.put(int(round(cx - hw)), y, idx)
        a.put(int(round(cx + hw)), y, idx)
    for x in range(int(math.ceil(cx - rx)), int(math.floor(cx + rx)) + 1):
        dx = (x - cx) / float(rx)
        if abs(dx) > 1:
            continue
        hh = ry * math.sqrt(max(0.0, 1 - dx * dx))
        a.put(x, int(round(cy - hh)), idx)
        a.put(x, int(round(cy + hh)), idx)


def profile(pts):
    def f(t):
        t = max(0.0, min(1.0, t))
        for k in range(len(pts) - 1):
            t0, v0 = pts[k]
            t1, v1 = pts[k + 1]
            if t0 <= t <= t1:
                u = (t - t0) / (t1 - t0)
                u = u * u * (3 - 2 * u)
                return v0 + (v1 - v0) * u
        return pts[-1][1]
    return f


# A gaiwan bowl is squat: the mouth is the widest thing on it, the wall falls
# away in a shallow curve, and it stands on a small foot ring.
BOWL = profile([(0.00, 1.000), (0.14, 0.940), (0.34, 0.845), (0.56, 0.727),
                (0.78, 0.605), (1.00, 0.470)])


def draw_gaiwan(a):
    cx = GAIWAN_CX
    shadow_blob(a, cx - 6, 145, 25, 6.5, TRAY_SH, only=TRAY_BG)

    # --- saucer: a lifted rim around a shallow well, both facing the light.
    # Its centre sits under the foot's contact line at y=138 — set further
    # back the bowl stands on the saucer's far rim instead of in the middle.
    flat_ellipse(a, cx, 139, 23, 23 * TILT, 2.9, 3.6)
    ring(a, cx, 139, 23, 23 * TILT, POR_O)
    flat_ellipse(a, cx, 140, 16, 16 * TILT, 2.6, 3.3)
    ring(a, cx, 140, 16, 16 * TILT, POR_S)

    # --- bowl: the mouth's back arc down to y=114, then the wall falling away
    RX, RY = 22.0, 22.0 * TILT

    def bowl_hw(y):
        if y < 114:
            dy = (y - 114) / RY
            if dy < -1:
                return None
            return RX * math.sqrt(max(0.0, 1 - dy * dy))
        return RX * BOWL((y - 114) / 20.0)

    revolve(a, cx, 108, 134, bowl_hw,
            occf=lambda y: max(0.0, (y - 128) / 7.0) * 1.2,
            specf=lambda y: 1.0 if 117 <= y <= 130 else 0.0,
            bouncef=lambda y: 118 <= y <= 130)
    revolve(a, cx, 134, 138, lambda y: 10.4 - (y - 134) * 0.3,
            occf=lambda y: 1.5, specf=lambda y: 0.0)
    for x in range(cx - 9, cx + 10):
        a.put(x, 138, POR_O)

    # an underglaze line under the rim: an arc on a body of revolution, not a
    # decal ruled straight across
    for x in range(cx - 21, cx + 22):
        du = (x - cx) / 21.0
        if abs(du) > 1:
            continue
        y = 120 + int(round(3.0 * math.sqrt(max(0.0, 1 - du * du))))
        if abs(x - cx) <= RX * BOWL((y - 114) / 20.0):
            a.put(x, y, POR_LN)

    # --- the lid. It fills the mouth: leave only a crescent of dark at the
    # front, which is the gap the steam comes out of. An open dark ring all
    # the way round reads as a bowl with something in it.
    a.ellipse(cx, 114.4, 19.0, 6.3, POR_D)
    flat_ellipse(a, cx, 113.2, 18.2, 6.0, 2.9, 3.6, front=POR_O)
    revolve(a, cx, 103, 113,
            lambda y: 13.5 * math.sqrt(max(0.0, 1 - ((y - 115.0) / 12.0) ** 2)),
            specf=lambda y: 1.0, outline=False)
    a.hline(cx - 13, cx + 13, 113, POR_S)

    # finial: a low button on a two-row stem, sitting on the dome — any
    # taller and it reads as a stopper
    a.rect(cx - 2, 102, cx + 2, 103, POR_M)
    a.put(cx + 2, 102, POR_L)
    a.put(cx - 2, 102, POR_S)
    flat_ellipse(a, cx, 100.6, 4.6, 2.4, 3.0, 3.7, front=POR_O)


def draw_cup(a, cx, base, rw):
    """Rim ellipse first, then tea strictly inside it — the first pass
    painted the liquor above and outside the cup, onto the tray."""
    ry = rw * TILT
    top = base - int(round(rw * 1.35))
    shadow_blob(a, cx - 4, base + 2, rw + 5, 4.5, TRAY_SH, only=TRAY_BG)

    def hw(y):
        if y < top:
            dy = (y - top) / ry
            if dy < -1:
                return None
            return rw * math.sqrt(max(0.0, 1 - dy * dy))
        t = (y - top) / float(base - top)
        return rw - (rw * 0.42) * (t ** 1.15)

    revolve(a, cx, top - int(ry) - 1, base, hw,
            occf=lambda y: max(0.0, (y - (base - 4)) / 4.0) * 1.5,
            specf=lambda y: 1.0 if top + 1 <= y <= base - 3 else 0.0,
            bouncef=lambda y: top + 2 <= y <= base - 3)
    for x in range(int(cx - rw * 0.58), int(cx + rw * 0.58) + 1):
        a.put(x, base, POR_O)

    a.ellipse(cx, top, rw - 1.0, ry - 0.4, POR_D)      # the inner wall
    a.ellipse(cx, top + 0.6, rw - 2.6, ry - 1.3, TEA_D)
    a.ellipse(cx, top + 0.4, rw - 3.4, ry - 1.8, TEA_M)
    a.ellipse(cx + 1.5, top + 0.1, rw - 6.0, ry - 2.4, TEA_H)


def draw_chahe(a):
    """The leaf dish: dry leaf lives here, not scattered over the tray."""
    cx, cy = CHAHE_CX, CHAHE_CY
    shadow_blob(a, cx - 3, cy + 4, 17, 4.5, TRAY_SH, only=TRAY_BG)
    flat_ellipse(a, cx, cy, 15, 15 * TILT, 2.6, 3.5)
    ring(a, cx, cy, 15, 15 * TILT, POR_O)
    a.ellipse(cx, cy + 0.6, 11.0, 11.0 * TILT, LEAF)
    for (lx, ly) in ((cx - 5, cy - 1), (cx + 2, cy), (cx - 1, cy + 1)):
        a.ellipse(lx, ly, 2.4, 1.0, TRAY_S)
    a.put(cx + 14, cy - 1, POR_L)                      # the pouring lip
    a.put(cx + 15, cy, POR_M)


def draw_towel(a):
    """A folded cha jin. Two dozen pixels; every tea table has one."""
    x0, y0, x1, y1 = 84, 155, 104, 163
    a.rect(x0, y0, x1, y1, CLOTH_D)
    for (cx_, cy_) in ((x0, y0), (x1, y0), (x0, y1), (x1, y1)):
        a.put(cx_, cy_, TRAY_SH)
    a.hline(x0 + 1, x1 - 1, y0, CLOTH_L)
    a.hline(x0 + 3, x1 - 2, y0 + 4, CLOTH_L)       # the fold
    a.vline(x1, y0 + 1, y1 - 1, CLOTH_L)
    a.hline(x0, x1 + 1, y1 + 1, TRAY_SH)


def draw_kettle(a):
    """A kettle on a charcoal brazier at the back right. Without it there is
    no way for tea to be happening in this room — and at night it is the
    light that explains why the tray is lit at all."""
    bx, by = 166, 133                                  # brazier centre, base

    # the glow it throws on the tray, then the pot's own contact shadow on
    # top of it — the other way round and the shadow has nothing left to
    # paint on, and the brazier floats
    shadow_blob(a, bx, by + 3, 13, 5, GLOW_T,
                only=TRAY_BG | {TRAY_LIT}, core=0.9)
    for gy, idx in ((133, TRAY_S), (135, TRAY_L), (142, TRAY_G), (144, TRAY_L)):
        for gx in range(bx - 14, bx + 15):          # re-cut the slats it covered
            if a.get(gx, gy) == GLOW_T:
                a.put(gx, gy, idx)
    shadow_blob(a, bx - 2, by + 5, 15, 4.5, TRAY_SH,
                only=TRAY_BG | {GLOW_T, TRAY_LIT})

    # brazier: a squat drum with the fire door facing us
    for y in range(118, by + 1):
        t = (y - 118) / float(by - 118)
        hw = 12.5 - 1.6 * t
        for x in range(int(bx - hw), int(bx + hw) + 1):
            u = (x - bx) / hw
            a.put(x, y, IRON_L if u > 0.45 else (IRON_M if u > -0.5 else IRON_D))
    a.ellipse(bx, 118, 12.5, 4.0, IRON_M)
    ring(a, bx, 118, 12.5, 4.0, IRON_D)
    for x in range(bx - 12, bx + 13):                  # a rolled top rim
        a.put(x, 121, IRON_D)
    for x in range(bx - 12, bx + 13):
        a.put(x, by, IRON_D)

    # the fire door, and the coals in it
    a.ellipse(bx - 1, 128, 6.0, 3.6, IRON_D)
    a.ellipse(bx - 1, 128, 4.6, 2.6, EMBER)
    a.ellipse(bx - 1, 128.2, 2.6, 1.4, EMBER_H)
    a.put(bx + 3, 130, EMBER)
    a.put(bx - 5, 126, EMBER)

    # kettle: a round belly, a spout to the left, a bail over the top
    def kh(y):
        if y < 100:
            return None
        t = (y - 100) / 18.0
        return 11.0 * math.sin(math.pi * (0.18 + 0.70 * t))
    for y in range(100, 118):
        hw = kh(y)
        if hw is None or hw < 1:
            continue
        for x in range(int(round(bx - hw)), int(round(bx + hw)) + 1):
            u = (x - bx) / hw
            a.put(x, y, IRON_L if 0.35 < u < 0.8 else
                        (IRON_M if u > -0.55 else IRON_D))
    a.ellipse(bx, 100, 6.5, 2.2, IRON_M)
    ring(a, bx, 100, 6.5, 2.2, IRON_D)
    a.rect(bx - 1, 96, bx + 1, 99, IRON_M)             # the knob
    a.put(bx + 1, 97, IRON_L)

    for k in range(8):                                 # spout
        y = 104 + k
        w = int(round(7.0 * math.exp(-((k - 1.0) / 3.0) ** 2)))
        for x in range(bx - 10 - w, bx - 9):
            a.put(x, y, IRON_M if x > bx - 10 - w + 1 else IRON_D)
    for k in range(30):                                # bail handle
        ang = math.pi * k / 29.0
        hx = bx - 8.0 * math.cos(ang)
        hy = 103 - 8.0 * math.sin(ang)
        a.put(int(round(hx)), int(round(hy)), IRON_D)
        if 4 <= k <= 25:
            a.put(int(round(hx)), int(round(hy)) - 1, IRON_L)


# ---------------------------------------------------------------------------
# steam — solid one-pixel curls. Dithered steam on this panel reads as dead
# pixels, so the plume is a stroke that simply stops.
# ---------------------------------------------------------------------------
def steam(a, x0, y0, length, amp, phase, freq=1.1, wob=0.0):
    for k in range(length + 1):
        t = k / float(length)
        y = y0 - k
        x = x0 + amp * math.sin(freq * t * 2 * math.pi + phase) * (0.3 + t) \
            + wob * t * t
        wide = 0.25 < t < 0.8
        for ox in ((-1, 0) if wide else (0,)):
            px, py = int(round(x)) + ox, int(y)
            if not (0 <= px < a.w and 0 <= py < a.h):
                continue
            cur = a.px[py][px]
            tone = P.STEAM_OVER.get(cur)
            if tone is None:
                continue
            if t > 0.84:                      # thin out over the last sixth
                a.dith(px, py, tone, cur, (1.0 - t) / 0.16)
            else:
                a.put(px, py, tone)


def draw_steam(a):
    steam(a, GAIWAN_CX + 14, 116, 30, 4.6, 2.2, 1.05, wob=6)
    steam(a, GAIWAN_CX - 14, 117, 19, 3.4, 0.5, 1.0, wob=-6)
    steam(a, CUPA_CX + 1, 138, 18, 3.2, 3.1, 1.0, wob=3)
    steam(a, 166 - 14, 104, 17, 3.0, 4.4, 1.0, wob=-4)


# ---------------------------------------------------------------------------
def build_art():
    a = Art(W, H, WALL_M)
    draw_wall(a)
    draw_shelf(a)
    draw_scroll(a)
    draw_window(a)
    draw_tray(a)
    draw_chahe(a)
    draw_gaiwan(a)
    draw_cup(a, CUPA_CX, CUPA_BASE, CUPA_RW)
    draw_kettle(a)
    draw_cup(a, CUPB_CX, CUPB_BASE, CUPB_RW)
    draw_towel(a)
    draw_steam(a)
    return a


# ---------------------------------------------------------------------------
# outputs
# ---------------------------------------------------------------------------
def write_resource(a):
    blob = encode(a)
    path = os.path.join(ROOT, 'resources', 'data', 'scene.bin')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(blob)
    return len(blob)


def write_header():
    lines = ['// Generated by tools/make_scene.py — do not edit by hand.',
             '#pragma once', '#include <pebble.h>', '',
             '#define SCENE_W %d' % W,
             '#define SCENE_H %d' % H,
             '#define SCENE_NCOL %d' % P.NCOL, '',
             'static const uint8_t SCENE_PAL[4][SCENE_NCOL] = {']
    for name, pal in P.PALETTES:
        lines.append('  { // %s' % name)
        row = ['0x%02X' % argb8(pal[i]) for i in range(P.NCOL)]
        for k in range(0, len(row), 8):
            lines.append('    ' + ', '.join(row[k:k + 8]) + ',')
        lines.append('  },')
    lines += ['};', '']
    with open(os.path.join(ROOT, 'src', 'c', 'scene_data.h'), 'w') as f:
        f.write('\n'.join(lines))


def render_png(a, pal, scale=3):
    from PIL import Image
    img = Image.new('RGB', (W, H))
    px = img.load()
    for y in range(H):
        for x in range(W):
            px[x, y] = snap(pal[a.px[y][x]])
    return img.resize((W * scale, H * scale), Image.NEAREST)


def render_face(a, pal, scale=3):
    """Whole-watch mock: Emery's real chrome band above, the scene below."""
    from PIL import Image, ImageDraw, ImageFont
    FW, FH, TOP = 200, 228, 56
    img = Image.new('RGB', (FW, FH), (0, 0, 0))
    px = img.load()
    for y in range(H):
        for x in range(W):
            px[x, TOP + y] = snap(pal[a.px[y][x]])
    img = img.resize((FW * scale, FH * scale), Image.NEAREST)
    d = ImageDraw.Draw(img)

    def font(sz, bold=False):
        for p in ('/System/Library/Fonts/Supplemental/Arial Bold.ttf'
                  if bold else '/System/Library/Fonts/Supplemental/Arial.ttf',
                  '/System/Library/Fonts/Helvetica.ttc'):
            try:
                return ImageFont.truetype(p, sz)
            except OSError:
                pass
        return ImageFont.load_default()

    S = scale
    d.text((3 * S, 1 * S), '9:41', fill=(255, 255, 255), font=font(34 * S, True))
    d.line([(163 * S, 3 * S), (163 * S, 44 * S)], fill=(85, 85, 85))
    d.text((120 * S, 0), '8,842', fill=(0, 255, 0), font=font(15 * S, True))
    d.text((133 * S, 14 * S), '6.3 km', fill=(0, 255, 0), font=font(12 * S))
    d.text((131 * S, 28 * S), '68 bpm', fill=(0, 255, 0), font=font(12 * S))
    d.text((168 * S, 0), 'SAT', fill=(170, 170, 170), font=font(12 * S, True))
    d.text((168 * S, 13 * S), '26', fill=(255, 255, 255), font=font(12 * S))
    d.text((168 * S, 27 * S), '74°', fill=(170, 170, 170), font=font(12 * S))
    import random
    random.seed(7)
    for i in range(60):
        x = 2 + (196 * i) // 60
        xe = 2 + (196 * (i + 1)) // 60
        h = random.choice([1, 1, 1, 2, 4, 7, 9, 3, 1, 1])
        d.rectangle([x * S, (46 + 10 - h) * S, xe * S - 1, 56 * S - 1],
                    fill=(255, 170, 0))
    return img


def main():
    a = build_art()
    used = sorted({a.px[y][x] for y in range(H) for x in range(W)})
    for name, pal in P.PALETTES:
        for i in used:
            if pal[i] == (255, 0, 255):
                print('  !! %s has no colour for %s' % (name, P.NAMES[i]))
    unused = [P.NAMES[i] for i in range(P.NCOL) if i not in used]
    if unused:
        print('  .. unused indices: %s' % ', '.join(unused))
    n = write_resource(a)
    write_header()
    out = os.path.join(ROOT, 'preview')
    os.makedirs(out, exist_ok=True)
    from PIL import Image
    for name, pal in P.PALETTES:
        render_png(a, pal).save(os.path.join(out, 'scene_%s.png' % name.lower()))
        render_face(a, pal).save(os.path.join(out, 'face_%s.png' % name.lower()))
    sheet = Image.new('RGB', (200 * 3 * 4 + 5 * 30, 228 * 3 + 20), (24, 24, 24))
    small = Image.new('RGB', (200 * 4 + 5 * 5, 238), (24, 24, 24))
    for k, (name, pal) in enumerate(P.PALETTES):
        f = render_face(a, pal)
        sheet.paste(f, (30 + k * (200 * 3 + 30), 10))
        small.paste(render_face(a, pal, scale=1), (5 + k * 205, 5))
    sheet.save(os.path.join(out, 'sheet.png'))
    small.save(os.path.join(out, 'sheet_1x.png'))
    print('scene.bin %d bytes, %d indices used of %d' % (n, len(used), P.NCOL))


if __name__ == '__main__':
    main()
