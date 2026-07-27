#!/usr/bin/env python3
"""Palette indices and the four time-of-day colourings.

One index map, four palettes. Every colour is snapped to the Pebble 64 —
each channel 0/85/170/255 — so anything written here that isn't already on
the grid gets rounded to its nearest legal neighbour.
"""

NAMES = []


def _idx(name):
    NAMES.append(name)
    return len(NAMES) - 1


# --- wall ------------------------------------------------------------------
WALL_D = _idx('WALL_D')          # board seam / shadowed board
WALL_M = _idx('WALL_M')          # the wall itself
WALL_L = _idx('WALL_L')          # grain highlight / light pool
WALL_G = _idx('WALL_G')          # glow spill around the window
FLOOR_D = _idx('FLOOR_D')        # dark base of the wall behind the tray

# --- round window ----------------------------------------------------------
FRM_D = _idx('FRM_D')            # lacquer frame, shadow side
FRM_M = _idx('FRM_M')
FRM_L = _idx('FRM_L')            # lacquer frame, lit side
LATT = _idx('LATT')              # mullions
SKY_T = _idx('SKY_T')            # sky, high
SKY_M = _idx('SKY_M')
SKY_B = _idx('SKY_B')            # sky, at the horizon
DISC = _idx('DISC')              # sun / moon
DISC_H = _idx('DISC_H')          # its halo
HILL_F = _idx('HILL_F')          # far ridge
HILL_N = _idx('HILL_N')          # near ridge
BAMB_D = _idx('BAMB_D')          # bamboo outside the window
BAMB_L = _idx('BAMB_L')

# --- hanging scroll --------------------------------------------------------
ROD = _idx('ROD')                # dowels top and bottom
ROD_L = _idx('ROD_L')
PAPER = _idx('PAPER')
PAPER_S = _idx('PAPER_S')        # paper in shadow / mounting border
INK = _idx('INK')
INK_S = _idx('INK_S')            # thinner wash
SEAL = _idx('SEAL')

# --- tea tray (cha pan) ----------------------------------------------------
TRAY_T = _idx('TRAY_T')          # top face
TRAY_L = _idx('TRAY_L')          # lit slat crown
TRAY_S = _idx('TRAY_S')          # groove between slats
TRAY_E = _idx('TRAY_E')          # front edge face
TRAY_EL = _idx('TRAY_EL')        # front edge, lit top lip
TRAY_G = _idx('TRAY_G')          # the open gaps between slats
TRAY_SH = _idx('TRAY_SH')        # cast shadow on the tray
TRAY_LIT = _idx('TRAY_LIT')      # the shaft of window light on the tray
UNDER = _idx('UNDER')            # shadow beneath the tray
LEAF = _idx('LEAF')              # dry leaf in the cha he
CLOTH_D = _idx('CLOTH_D')        # folded tea towel
CLOTH_L = _idx('CLOTH_L')

# --- the kettle and its brazier: the reason there is tea at all, and the
# light source the night palette was missing
IRON_D = _idx('IRON_D')
IRON_M = _idx('IRON_M')
IRON_L = _idx('IRON_L')
EMBER = _idx('EMBER')            # the coals
EMBER_H = _idx('EMBER_H')        # their core
GLOW_T = _idx('GLOW_T')          # what they throw on the tray

# --- porcelain -------------------------------------------------------------
POR_H = _idx('POR_H')            # specular
POR_L = _idx('POR_L')
POR_M = _idx('POR_M')
POR_S = _idx('POR_S')            # shadow side
POR_D = _idx('POR_D')            # terminator / underside
POR_O = _idx('POR_O')            # outline against the tray
POR_R = _idx('POR_R')            # rim light from the window
POR_LN = _idx('POR_LN')          # underglaze band

# --- tea liquor ------------------------------------------------------------
TEA_H = _idx('TEA_H')
TEA_M = _idx('TEA_M')
TEA_D = _idx('TEA_D')

# --- steam, one tone per surface it drifts across --------------------------
ST_WALL = _idx('ST_WALL')
ST_SKY = _idx('ST_SKY')
ST_PAPER = _idx('ST_PAPER')
ST_FRAME = _idx('ST_FRAME')
ST_TRAY = _idx('ST_TRAY')
ST_POR = _idx('ST_POR')
ST_GLOW = _idx('ST_GLOW')

NCOL = len(NAMES)

# Which steam tone sits over which background.
STEAM_OVER = {
    WALL_D: ST_WALL, WALL_M: ST_WALL, WALL_L: ST_WALL, WALL_G: ST_GLOW,
    FLOOR_D: ST_WALL,
    SKY_T: ST_SKY, SKY_M: ST_SKY, SKY_B: ST_SKY,
    DISC: ST_SKY, DISC_H: ST_SKY, HILL_F: ST_SKY, HILL_N: ST_SKY,
    BAMB_D: ST_SKY, BAMB_L: ST_SKY,
    FRM_D: ST_FRAME, FRM_M: ST_FRAME, FRM_L: ST_FRAME, LATT: ST_FRAME,
    PAPER: ST_PAPER, PAPER_S: ST_PAPER, INK: ST_PAPER, INK_S: ST_PAPER,
    SEAL: ST_PAPER, ROD: ST_PAPER, ROD_L: ST_PAPER,
    TRAY_T: ST_TRAY, TRAY_L: ST_TRAY, TRAY_S: ST_TRAY, TRAY_SH: ST_TRAY,
    TRAY_E: ST_TRAY, TRAY_EL: ST_TRAY, LEAF: ST_TRAY, TRAY_G: ST_TRAY,
    TRAY_LIT: ST_TRAY, CLOTH_D: ST_TRAY, CLOTH_L: ST_TRAY,
    GLOW_T: ST_TRAY, FLOOR_D: ST_FRAME,
}
# porcelain and existing steam keep their own tone
for _i in (POR_H, POR_L, POR_M, POR_S, POR_D, POR_O, POR_R, POR_LN,
           TEA_H, TEA_M, TEA_D):
    STEAM_OVER[_i] = ST_POR
for _i in (ST_WALL, ST_SKY, ST_PAPER, ST_FRAME, ST_TRAY, ST_POR, ST_GLOW):
    STEAM_OVER[_i] = _i


def _pal(d):
    out = [(255, 0, 255)] * NCOL          # magenta = an index nobody coloured
    for k, v in d.items():
        out[k] = v
    return out


# ---------------------------------------------------------------------------
# The four colourings. The room is a Jiangnan tea studio: pale plaster in
# dark hardwood framing, a bamboo cha pan, qingbai porcelain. Value first —
# dark frame, pale wall, mid tray, light vessels — so the porcelain reads at
# 1x even before colour does any work. Red is spent only on the seal.
# ---------------------------------------------------------------------------
MORNING = _pal({          # low, raking, warm: the first light of the day
    WALL_D: (170, 85, 0),   WALL_M: (255, 255, 170), WALL_L: (255, 255, 255),
    WALL_G: (170, 170, 170), FLOOR_D: (85, 0, 0),

    FRM_D: (85, 0, 0),      FRM_M: (170, 85, 0),    FRM_L: (255, 170, 85),
    LATT: (85, 0, 0),
    SKY_T: (170, 170, 255), SKY_M: (170, 255, 255), SKY_B: (255, 255, 170),
    DISC: (255, 255, 255),  DISC_H: (255, 255, 170),
    HILL_F: (170, 170, 255), HILL_N: (85, 85, 170),
    BAMB_D: (85, 170, 85),  BAMB_L: (170, 255, 170),

    ROD: (85, 0, 0),        ROD_L: (170, 85, 0),
    PAPER: (255, 255, 255), PAPER_S: (170, 170, 85),
    INK: (0, 0, 0),         INK_S: (85, 85, 85),    SEAL: (255, 0, 0),

    TRAY_T: (170, 85, 0),   TRAY_L: (255, 170, 85),  TRAY_S: (85, 85, 0),
    TRAY_E: (85, 0, 0),     TRAY_EL: (255, 170, 85), TRAY_G: (0, 0, 0),
    TRAY_SH: (85, 0, 0),    TRAY_LIT: (255, 255, 170),
    UNDER: (0, 0, 0),       LEAF: (85, 0, 0),
    CLOTH_D: (0, 0, 85),    CLOTH_L: (85, 85, 170),

    POR_H: (255, 255, 255), POR_L: (255, 255, 170), POR_M: (170, 255, 255),
    POR_S: (170, 170, 170), POR_D: (85, 85, 85),    POR_O: (0, 0, 0),
    POR_R: (170, 170, 255), POR_LN: (0, 0, 170),

    TEA_H: (255, 255, 170), TEA_M: (255, 170, 0),   TEA_D: (170, 85, 0),

    ST_WALL: (255, 255, 255), ST_SKY: (255, 255, 255),
    ST_PAPER: (255, 255, 255), ST_FRAME: (170, 85, 0),
    ST_TRAY: (255, 170, 85), ST_POR: (255, 255, 255),
    ST_GLOW: (255, 255, 255),
    IRON_D: (0, 0, 0),      IRON_M: (85, 85, 85),   IRON_L: (170, 170, 170),
    EMBER: (255, 85, 0),    EMBER_H: (255, 255, 85), GLOW_T: (255, 170, 85),
})

AFTERNOON = _pal({        # high, pale, neutral
    WALL_D: (85, 0, 0),     WALL_M: (255, 255, 255), WALL_L: (255, 255, 255),
    WALL_G: (170, 170, 85), FLOOR_D: (85, 0, 0),

    FRM_D: (85, 0, 0),      FRM_M: (170, 85, 0),    FRM_L: (255, 170, 85),
    LATT: (85, 0, 0),
    SKY_T: (0, 170, 255),   SKY_M: (170, 255, 255), SKY_B: (255, 255, 170),
    DISC: (255, 255, 255),  DISC_H: (255, 255, 170),
    HILL_F: (170, 170, 255), HILL_N: (85, 170, 170),

    BAMB_D: (0, 170, 85),   BAMB_L: (170, 255, 85),
    ROD: (85, 0, 0),        ROD_L: (170, 85, 0),
    PAPER: (255, 255, 170), PAPER_S: (170, 170, 85),
    INK: (0, 0, 0),         INK_S: (85, 85, 85),    SEAL: (255, 0, 0),

    TRAY_T: (170, 85, 0),   TRAY_L: (255, 170, 85),  TRAY_S: (85, 85, 0),
    TRAY_E: (85, 0, 0),     TRAY_EL: (255, 170, 85), TRAY_G: (0, 0, 0),
    TRAY_SH: (85, 0, 0),    TRAY_LIT: (255, 255, 170),
    UNDER: (0, 0, 0),       LEAF: (85, 0, 0),
    CLOTH_D: (0, 0, 85),    CLOTH_L: (85, 85, 170),

    POR_H: (255, 255, 255), POR_L: (255, 255, 170), POR_M: (170, 255, 255),
    POR_S: (170, 170, 170), POR_D: (85, 85, 85),    POR_O: (0, 0, 0),
    POR_R: (170, 170, 255), POR_LN: (0, 0, 170),

    TEA_H: (255, 255, 85),  TEA_M: (255, 170, 0),   TEA_D: (170, 85, 0),

    ST_WALL: (255, 255, 170), ST_SKY: (255, 255, 255),
    ST_PAPER: (255, 255, 255), ST_FRAME: (170, 85, 0),
    ST_TRAY: (255, 170, 85), ST_POR: (255, 255, 255),
    ST_GLOW: (255, 255, 255),
    IRON_D: (0, 0, 0),      IRON_M: (85, 85, 85),   IRON_L: (170, 170, 170),
    EMBER: (255, 85, 0),    EMBER_H: (255, 255, 85), GLOW_T: (255, 170, 85),
})

EVENING = _pal({          # the room going amber, one warm band at the horizon
    WALL_D: (85, 0, 0),     WALL_M: (255, 170, 85), WALL_L: (255, 255, 170),
    WALL_G: (170, 85, 0), FLOOR_D: (85, 0, 0),

    FRM_D: (85, 0, 0),      FRM_M: (170, 85, 0),    FRM_L: (255, 170, 0),
    LATT: (85, 0, 0),
    SKY_T: (85, 85, 170),   SKY_M: (170, 85, 170),  SKY_B: (255, 170, 0),
    DISC: (255, 255, 170),  DISC_H: (255, 170, 85),
    HILL_F: (85, 85, 170),  HILL_N: (85, 0, 85),
    BAMB_D: (85, 0, 0),     BAMB_L: (170, 85, 0),

    ROD: (85, 0, 0),        ROD_L: (170, 85, 0),
    PAPER: (255, 255, 170), PAPER_S: (170, 170, 85),
    INK: (85, 0, 0),        INK_S: (170, 85, 0),    SEAL: (255, 0, 0),

    TRAY_T: (170, 85, 0),   TRAY_L: (255, 170, 0),   TRAY_S: (85, 85, 0),
    TRAY_E: (85, 0, 0),     TRAY_EL: (255, 170, 0),  TRAY_G: (0, 0, 0),
    TRAY_SH: (85, 0, 0),    TRAY_LIT: (255, 255, 85),
    UNDER: (0, 0, 0),       LEAF: (85, 0, 0),
    CLOTH_D: (0, 0, 85),    CLOTH_L: (85, 0, 85),

    POR_H: (255, 255, 255), POR_L: (255, 255, 170), POR_M: (255, 170, 85),
    POR_S: (170, 85, 85),   POR_D: (85, 85, 85),    POR_O: (0, 0, 0),
    POR_R: (170, 85, 170),  POR_LN: (0, 0, 170),

    TEA_H: (255, 255, 85),  TEA_M: (255, 170, 0),   TEA_D: (170, 85, 0),

    ST_WALL: (255, 255, 170), ST_SKY: (255, 255, 255),
    ST_PAPER: (255, 255, 255), ST_FRAME: (170, 85, 0),
    ST_TRAY: (255, 170, 0), ST_POR: (255, 255, 255),
    ST_GLOW: (255, 255, 255),
    IRON_D: (0, 0, 0),      IRON_M: (85, 85, 85),   IRON_L: (170, 170, 170),
    EMBER: (255, 85, 0),    EMBER_H: (255, 255, 85), GLOW_T: (255, 170, 0),
})

NIGHT = _pal({            # the lamp over the tray is the only light
    WALL_D: (0, 0, 0),      WALL_M: (85, 0, 0),     WALL_L: (170, 85, 0),
    WALL_G: (0, 0, 0),      FLOOR_D: (0, 0, 0),

    FRM_D: (0, 0, 0),       FRM_M: (170, 85, 0),    FRM_L: (170, 85, 0),
    LATT: (0, 0, 0),
    SKY_T: (0, 0, 85),      SKY_M: (0, 0, 170),     SKY_B: (85, 85, 170),
    DISC: (255, 255, 255),  DISC_H: (170, 170, 255),
    HILL_F: (0, 0, 85),     HILL_N: (0, 0, 85),
    BAMB_D: (0, 0, 0),      BAMB_L: (0, 85, 85),

    ROD: (0, 0, 0),         ROD_L: (85, 0, 0),
    PAPER: (255, 170, 85),  PAPER_S: (170, 85, 0),
    INK: (0, 0, 0),         INK_S: (85, 0, 0),      SEAL: (255, 0, 0),

    TRAY_T: (85, 0, 0),     TRAY_L: (170, 85, 0),   TRAY_S: (85, 85, 0),
    TRAY_E: (0, 0, 0),      TRAY_EL: (170, 85, 0),  TRAY_G: (0, 0, 0),
    TRAY_SH: (0, 0, 0),     TRAY_LIT: (255, 170, 0),
    UNDER: (0, 0, 0),       LEAF: (0, 0, 0),
    CLOTH_D: (0, 0, 85),    CLOTH_L: (85, 85, 85),

    POR_H: (255, 255, 255), POR_L: (255, 255, 170), POR_M: (255, 170, 85),
    POR_S: (85, 85, 170),   POR_D: (0, 0, 85),      POR_O: (0, 0, 0),
    POR_R: (85, 85, 170),   POR_LN: (0, 0, 170),

    TEA_H: (255, 255, 85),  TEA_M: (255, 170, 0),   TEA_D: (170, 85, 0),

    ST_WALL: (170, 85, 0),  ST_SKY: (170, 170, 255),
    ST_PAPER: (255, 255, 170), ST_FRAME: (85, 0, 0),
    ST_TRAY: (255, 170, 0), ST_POR: (255, 255, 255),
    ST_GLOW: (255, 255, 170),
    IRON_D: (0, 0, 0),      IRON_M: (85, 85, 85),   IRON_L: (170, 170, 170),
    EMBER: (255, 85, 0),    EMBER_H: (255, 255, 85), GLOW_T: (255, 255, 85),
})

PALETTES = [('MORNING', MORNING), ('AFTERNOON', AFTERNOON),
            ('EVENING', EVENING), ('NIGHT', NIGHT)]
