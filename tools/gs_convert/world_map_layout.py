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
