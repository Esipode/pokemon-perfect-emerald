#ifndef GUARD_DATA_INFINITY_CAVE_H
#define GUARD_DATA_INFINITY_CAVE_H

// Props the Stage 8 decoration pass scatters over a generated room's floor.
// Whether a prop blocks movement is not stated here: the placer reads the
// collision bits of the prop's block in the key layout, so retheming a prop from
// walkable rubble to a solid boulder in Porymap needs no code change.
struct InfCaveDecor
{
    u8 role;        // enum InfCaveTileRole to stamp
    u8 weight;      // relative share of the props rolled for a room
    u8 minDistance; // Chebyshev tiles kept clear of every other prop
    u8 minDepth;    // not offered before this depth
    bool8 tall;     // taller than one tile: kept off tiles touching a wall
};

// Weights are shares of one roll, not percentages; the placer normalises them.
static const struct InfCaveDecor sInfCaveDecor[] =
{
    { INFCAVE_ROLE_DECOR_RUBBLE,      .weight = 22, .minDistance = 2, .minDepth = 0,  .tall = FALSE },
    { INFCAVE_ROLE_DECOR_ROCK_SMALL,  .weight = 24, .minDistance = 2, .minDepth = 0,  .tall = FALSE },
    { INFCAVE_ROLE_DECOR_BONES,       .weight = 8,  .minDistance = 3, .minDepth = 3,  .tall = FALSE },
    { INFCAVE_ROLE_DECOR_ROCK_LARGE,  .weight = 14, .minDistance = 3, .minDepth = 0,  .tall = FALSE },
    { INFCAVE_ROLE_DECOR_STALAGMITE,  .weight = 12, .minDistance = 3, .minDepth = 2,  .tall = TRUE  },
    { INFCAVE_ROLE_DECOR_CRYSTAL,     .weight = 6,  .minDistance = 4, .minDepth = 5,  .tall = TRUE  },
};

// Ground materials the patch pass lays over the floor as rectangles. Each entry
// names the first of nine roles ordered by enum InfCavePatchShape. As with
// props, whether a patch blocks movement is read from the collision bits of its
// authored blocks, not stated here: sand is walkable ground, a water pool is
// not, and both go through the same placer.
struct InfCavePatch
{
    u8 baseRole;  // INFCAVE_PATCH_NW role; the other eight follow it
    u8 weight;    // relative share of the patches rolled for a room
    u8 minDepth;  // not offered before this depth
};

static const struct InfCavePatch sInfCavePatch[] =
{
    { INFCAVE_ROLE_SAND_NW,  .weight = 70, .minDepth = 0 },
    { INFCAVE_ROLE_WATER_NW, .weight = 30, .minDepth = 2 },
};

// Set piece stamped into a generated room, by enum InfCaveRoomType.
// INFCAVE_PIECE_NONE leaves the room fully procedural. Every piece must share the room layout's
// tilesets, since stamping copies raw block words, and its outer ring must be
// walkable, since that ring is what carries paths past the piece; the stamp pass
// asserts both.
static const u16 sInfCavePieceLayout[INFCAVE_ROOM_COUNT] =
{
    [INFCAVE_ROOM_BATTLE]   = INFCAVE_PIECE_NONE,
    [INFCAVE_ROOM_GAUNTLET] = INFCAVE_PIECE_NONE,
    [INFCAVE_ROOM_ELITE]    = INFCAVE_PIECE_NONE,
    [INFCAVE_ROOM_BOSS]     = LAYOUT_INFINITY_CAVE_PIECE_BOSS,
    [INFCAVE_ROOM_REST]     = LAYOUT_INFINITY_CAVE_PIECE_REST,
    [INFCAVE_ROOM_TREASURE] = LAYOUT_INFINITY_CAVE_PIECE_TREASURE,
    [INFCAVE_ROOM_SHOP]     = LAYOUT_INFINITY_CAVE_PIECE_SHOP,
};

#endif // GUARD_DATA_INFINITY_CAVE_H
