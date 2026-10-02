"""World map placement data (GS Stage 28.10). Data only; read by build_world_map2.py.

Coordinates are in MAPSEC cells (one cell = one 8x8 tile). A cluster is a rect of a source
region grid (28x15, panel-local) whose occupied cells move together to a world destination.
Every occupied cell of every source grid must lie in exactly one cluster rect.
"""

WORLD_W, WORLD_H = 56, 36

# Region order is the L/R cycle order and the region id order (id 0 is open sea).
REGIONS = ["JOHTO", "KANTO", "HOENN", "SEVII"]

# Source grid -> (layout header in src/data/region_map, region).
SOURCES = {
    "JOHTO": ("region_map_layout_johto.h", "JOHTO"),
    "KANTO": ("region_map_layout_kanto.h", "KANTO"),
    "HOENN": ("region_map_layout.h", "HOENN"),
    "SEVII123": ("region_map_layout_sevii123.h", "SEVII"),
    "SEVII45": ("region_map_layout_sevii45.h", "SEVII"),
    "SEVII67": ("region_map_layout_sevii67.h", "SEVII"),
}

# Source art per grid: (path stem relative to the repo, or to the HnS clone when True).
# Each is a 128-px-wide affine sheet (.png) + 64x64 byte tilemap (.bin); the playable grid
# starts at tile (1, 2).
SOURCE_ART = {
    "JOHTO": ("graphics/pokenav/region_map/map_johto", True),
    "KANTO": ("graphics/region_map/map_kanto", False),
    "HOENN": ("graphics/region_map/map", False),
    "SEVII123": ("graphics/region_map/map_sevii_123", False),
    "SEVII45": ("graphics/region_map/map_sevii_45", False),
    "SEVII67": ("graphics/region_map/map_sevii_67", False),
}
ART_GRID_X, ART_GRID_Y = 1, 2

# Sevii islands and their world origin (top-left cell of the archipelago box, 22x15).
SEVII_X, SEVII_Y = 32, 19

# name, source grid, source rect (x, y, w, h), world destination (x, y), lock unit.
# Lock units: a locked unit's cells cannot be entered (Sevii 1-3 and 4-7 unlock separately).
CLUSTERS = [
    ("johto", "JOHTO", (1, 0, 24, 14), (7, 2), "JOHTO"),
    # Kanto's Indigo Plateau sits next to Johto's League: the continent join.
    ("kanto", "KANTO", (2, 1, 17, 14), (31, 3), "KANTO"),
    ("hoenn", "HOENN", (0, 0, 28, 15), (2, 19), "HOENN"),
    ("one_island", "SEVII123", (1, 3, 2, 8), (SEVII_X + 0, SEVII_Y + 0), "SEVII123"),
    ("two_island", "SEVII123", (9, 7, 1, 3), (SEVII_X + 4, SEVII_Y + 1), "SEVII123"),
    ("three_island", "SEVII123", (14, 12, 6, 2), (SEVII_X + 4, SEVII_Y + 5), "SEVII123"),
    ("four_island", "SEVII45", (3, 4, 1, 1), (SEVII_X + 12, SEVII_Y + 1), "SEVII4567"),
    ("navel_rock", "SEVII45", (10, 8, 1, 1), (SEVII_X + 9, SEVII_Y + 12), "SEVII4567"),
    ("five_island", "SEVII45", (14, 9, 5, 6), (SEVII_X + 11, SEVII_Y + 4), "SEVII4567"),
    ("six_island", "SEVII67", (15, 0, 4, 9), (SEVII_X + 18, SEVII_Y + 0), "SEVII4567"),
    ("seven_island", "SEVII67", (3, 6, 7, 7), (SEVII_X + 0, SEVII_Y + 8), "SEVII4567"),
    ("birth_island", "SEVII67", (18, 13, 1, 1), (SEVII_X + 20, SEVII_Y + 12), "SEVII4567"),
]

# Cluster pairs whose cells may touch (shared land border). All other clusters keep a sea gap.
TOUCHING = {("johto", "kanto")}

# Sea cells within this many cells (Chebyshev) of a region's MAPSEC cells belong to that region;
# the rest is open sea, which the cursor can always cross.
TERRITORY_MARGIN = 1

# Source cells beyond a cluster rect that still seed land pixels (coast around the rect).
# Margin cells inside another cluster's world rect are skipped.
MASK_MARGIN = 1


# --- R3 renderer data -------------------------------------------------------------------------

# Palette bank per lock unit (None = open sea). Bank 6 is the grey locked ramp, bank 15 text.
BANKS = {None: 0, "JOHTO": 1, "KANTO": 2, "HOENN": 3, "SEVII123": 4, "SEVII4567": 5}
LOCK_BANK = 6

# Colour roles (palette indices). Same pixel data in every bank; only the colours differ.
(C_SEA, C_SHALLOW, C_LAND, C_COAST, C_RELIEF, C_ROUTE, C_NODE, C_RIM, C_LABEL, C_BORDER) = range(10)

SEA_RGB = (20, 32, 64)
SHALLOW_RGB = (36, 58, 108)
ROUTE_RGB = (236, 238, 226)
NODE_RGB = (252, 252, 252)
RIM_RGB = (22, 26, 42)
LABEL_RGB = (112, 142, 196)
BORDER_RGB = (244, 244, 224)
LAND_RGB = {
    0: (104, 192, 84),            # sea bank: unused (no land lies in open-sea cells)
    1: (176, 180, 108),           # Johto: warm sage
    2: (88, 176, 140),            # Kanto: cool green
    3: (104, 192, 84),            # Hoenn: fresh green
    4: (220, 200, 142),           # Sevii 1-3: sand
    5: (208, 182, 132),           # Sevii 4-7: darker sand
}
GREY_LAND_RGB = (108, 110, 120)

# MAPSECs that read as routes (no node) besides ROUTE_*.
ROUTE_LIKE = {"KINDLE_ROAD", "CAPE_BRINK", "BOND_BRIDGE", "GREEN_PATH", "WATER_PATH",
              "CANYON_ENTRANCE", "SEVAULT_CANYON"}
# Node kind: *_CITY -> big node, the names below and *_TOWN -> small node, other non-routes -> diamond.
SMALL_NODES = {"INDIGO_PLATEAU", "CINNABAR_ISLAND", "ONE_ISLAND", "TWO_ISLAND", "THREE_ISLAND",
               "FOUR_ISLAND", "FIVE_ISLAND", "SIX_ISLAND", "SEVEN_ISLAND", "THREE_ISLE_PORT",
               "ROUTE_4_POKECENTER", "ROUTE_10_POKECENTER"}

# Adjacent MAPSECs are joined by a route line. Cells of different lock units never join unless
# listed in ROUTE_JOINS; ROUTE_BREAKS removes a join (both are MAPSEC name pairs, no prefix).
ROUTE_BREAKS = set()
ROUTE_JOINS = {("JOHTO_LEAGUE", "INDIGO_PLATEAU")}

# Mountains that get a darker relief patch.
RELIEF = ["MT_SILVER", "MT_CHIMNEY", "MT_MORTAR", "ICE_PATH"]

# Region label, anchor (px) the placement search prefers, region owning the sea it sits in.
LABELS = [
    ("JOHTO", (24, 24), "JOHTO"),
    ("KANTO", (340, 4), "KANTO"),
    ("HOENN", (120, 276), "HOENN"),
    ("SEVII ISLANDS", (330, 276), "SEVII"),
]
