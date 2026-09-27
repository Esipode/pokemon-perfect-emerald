#ifndef GUARD_CONSTANTS_INFINITY_CAVE_H
#define GUARD_CONSTANTS_INFINITY_CAVE_H

// Infinity Cave: end-game roguelite descent. One run is a sequence of
// procedurally generated rooms; before each room the player picks one of
// several nodes, each showing the room's type and its active modifiers.

// What a node's room contains. Stored in struct InfinityCaveRun.roomType.
enum InfCaveRoomType
{
    INFCAVE_ROOM_BATTLE,   // 2-3 rolled trainers
    INFCAVE_ROOM_GAUNTLET, // 4-6 trainers, weaker pools, more shards
    INFCAVE_ROOM_ELITE,    // 1 trainer, full party, one tier above depth
    INFCAVE_ROOM_BOSS,     // gym leader / rival / Elite Four identity
    INFCAVE_ROOM_REST,     // healing shrine, no trainers
    INFCAVE_ROOM_TREASURE, // item balls, no trainers
    INFCAVE_ROOM_SHOP,     // merchant spending shards, no trainers
    INFCAVE_ROOM_COUNT,
};

// Overlaid on a room type. Up to INFCAVE_MAX_MODIFIERS per room; each slot
// carries a modifier id plus one argument byte (type, weather, terrain...).
enum InfCaveModifier
{
    INFCAVE_MOD_NONE,
    INFCAVE_MOD_MONOTYPE,  // arg: enum Type every trainer pool is pruned to
    INFCAVE_MOD_WEATHER,   // arg: startingStatus weather, also the field weather
    INFCAVE_MOD_TERRAIN,   // arg: startingStatus terrain
    INFCAVE_MOD_DOUBLES,
    INFCAVE_MOD_SWARM,     // +2 trainers over the room type's base count
    INFCAVE_MOD_SURGE,     // +3 opponent levels, +50% shards
    INFCAVE_MOD_GIMMICK,   // aces carry a Z-Crystal / Mega Stone / Tera type
    INFCAVE_MOD_NO_ITEMS,  // bag locked in battle, +50% shards
    INFCAVE_MOD_BOUNTY,    // double shards
    INFCAVE_MOD_DARK,      // flash-darkened room, maze-biased layout
    INFCAVE_MOD_CRAMPED,   // corridor-heavy layout, fewer open rooms
    INFCAVE_MOD_TREASURED, // adds 2 item balls to a non-treasure room
    INFCAVE_MOD_COUNT,
};

// Why a run closed. Passed to InfCave_EndRun.
enum InfCaveEndReason
{
    INFCAVE_END_QUIT,   // player left through the lobby
    INFCAVE_END_DEFEAT, // lost a cave battle
};

#define INFCAVE_MAX_MODIFIERS 2

// Node options offered per descent. Grows with depth.
#define INFCAVE_MIN_OPTIONS         3
#define INFCAVE_MAX_OPTIONS         5
#define INFCAVE_OPTIONS_4_DEPTH     11 // 4 options from this depth
#define INFCAVE_OPTIONS_5_DEPTH     26 // 5 options from this depth

// Node cadence.
#define INFCAVE_BOSS_INTERVAL       5 // every depth divisible by this is a boss
#define INFCAVE_REST_MAX_GAP        4 // a rest node is offered at least this often
#define INFCAVE_SHOP_MIN_GAP        6 // a shop node appears at most this often

// Generated room canvas. Must satisfy the sBackupMapData budget:
// (width + MAP_OFFSET_W) * (height + MAP_OFFSET_H) <= MAX_MAP_DATA_SIZE.
#define INFCAVE_MAP_WIDTH           40
#define INFCAVE_MAP_HEIGHT          40

// Mask cell kinds. Generation works on a per-tile mask first; the autotile pass
// turns the mask into metatiles, so no pass before it needs to know tile art.
enum InfCaveCell
{
    INFCAVE_CELL_VOID,     // solid rock outside the cave
    INFCAVE_CELL_WALL,     // wall shell hugging the carved floor
    INFCAVE_CELL_FLOOR,    // walkable
    INFCAVE_CELL_RESERVED, // owned by a set piece; later passes must not touch it
};

// Mask generation limits. Rooms are axis-aligned rectangles placed with a gap
// between them, then chained together by corridors.
#define INFCAVE_MIN_ROOMS           5
#define INFCAVE_MAX_ROOMS           9
#define INFCAVE_ROOM_MIN_W          5
#define INFCAVE_ROOM_MAX_W          11
#define INFCAVE_ROOM_MIN_H          5
#define INFCAVE_ROOM_MAX_H          9
#define INFCAVE_ROOM_GAP            2 // free tiles required between two rooms
#define INFCAVE_CORRIDOR_WIDTH      2 // width 1 leaves walls too thin to tile
#define INFCAVE_MARGIN              3 // solid tiles kept at every canvas edge
#define INFCAVE_SMOOTH_PASSES       2

// Mask legality. The post-pass edits the mask until it satisfies the rules the
// autotiler and the NPC placer rely on; a mask that still fails is regenerated
// from a shifted stream, and after INFCAVE_MASK_ATTEMPTS tries the generator
// falls back to a plain rectangular room.
#define INFCAVE_LEGALITY_PASSES     8   // repair sweeps before the mask is judged
#define INFCAVE_MASK_ATTEMPTS       8
#define INFCAVE_MIN_FLOOR_TILES     120 // a room with less walkable area is rejected
#define INFCAVE_EMERGENCY_W         24  // fallback room, centred on the canvas
#define INFCAVE_EMERGENCY_H         20

// Tile-role key map. LAYOUT_INFINITY_CAVE_TILEKEY stores one authored block per
// role at a fixed cell, so retheming the cave is a Porymap edit with no code
// change. Role ids are cell indices into that layout: row * width + column.
#define INFCAVE_TILEKEY_WIDTH       16
#define INFCAVE_TILEKEY_HEIGHT      8
#define INFCAVE_TILEKEY_CELL(row, col) ((row) * INFCAVE_TILEKEY_WIDTH + (col))
#define INFCAVE_ROLE_COUNT          (INFCAVE_TILEKEY_WIDTH * INFCAVE_TILEKEY_HEIGHT)

// Floor variants rolled per tile by the autotiler. Variant 0 dominates.
#define INFCAVE_FLOOR_VARIANT_COUNT 8

// Wall role names state which side the floor is on, not which corner of the art
// they draw: INFCAVE_ROLE_WALL_N is a wall with floor to its north. The INNER
// roles are the concave corners, where floor touches only that diagonal.
enum InfCaveTileRole
{
    // Row 0 - floor variants.
    INFCAVE_ROLE_FLOOR_0 = INFCAVE_TILEKEY_CELL(0, 0),
    INFCAVE_ROLE_FLOOR_1,
    INFCAVE_ROLE_FLOOR_2,
    INFCAVE_ROLE_FLOOR_3,
    INFCAVE_ROLE_FLOOR_4,
    INFCAVE_ROLE_FLOOR_5,
    INFCAVE_ROLE_FLOOR_6,
    INFCAVE_ROLE_FLOOR_7,

    // Row 1 - wall shapes.
    INFCAVE_ROLE_WALL_NW = INFCAVE_TILEKEY_CELL(1, 0),
    INFCAVE_ROLE_WALL_N,
    INFCAVE_ROLE_WALL_NE,
    INFCAVE_ROLE_WALL_W,
    INFCAVE_ROLE_WALL_FILL,
    INFCAVE_ROLE_WALL_E,
    INFCAVE_ROLE_WALL_SW,
    INFCAVE_ROLE_WALL_S,
    INFCAVE_ROLE_WALL_SE,
    INFCAVE_ROLE_WALL_INNER_NW,
    INFCAVE_ROLE_WALL_INNER_NE,
    INFCAVE_ROLE_WALL_INNER_SW,
    INFCAVE_ROLE_WALL_INNER_SE,

    // Row 2 - south-face pieces. gTileset_Cave draws the face a single metatile
    // tall, so these currently duplicate the row 1 south edge; they exist so a
    // retheme with a two-tall face has somewhere to put the second row.
    INFCAVE_ROLE_FACE_L = INFCAVE_TILEKEY_CELL(2, 0),
    INFCAVE_ROLE_FACE_M,
    INFCAVE_ROLE_FACE_R,
    INFCAVE_ROLE_FACE_INNER_L,
    INFCAVE_ROLE_FACE_INNER_R,

    // Row 3 - decorations, placed by the Stage 8 pass.
    INFCAVE_ROLE_DECOR_ROCK_SMALL = INFCAVE_TILEKEY_CELL(3, 0),
    INFCAVE_ROLE_DECOR_ROCK_LARGE,
    INFCAVE_ROLE_DECOR_STALAGMITE,
    INFCAVE_ROLE_DECOR_RUBBLE,
    INFCAVE_ROLE_DECOR_CRYSTAL,
    INFCAVE_ROLE_DECOR_BONES,
    INFCAVE_ROLE_DECOR_PUDDLE,

    // Row 4 - set-piece anchors.
    INFCAVE_ROLE_PAD_ENTRANCE = INFCAVE_TILEKEY_CELL(4, 0),
    INFCAVE_ROLE_PAD_EXIT,
    INFCAVE_ROLE_PAD_SHRINE,
    INFCAVE_ROLE_PAD_ITEM_BALL,
    INFCAVE_ROLE_PAD_SHOP,

    // Rows 5-7 are spare.
};

// Set to TRUE to log an ASCII picture of each generated mask over the debug
// print handler. Off in shipped builds; generation cost is otherwise unchanged.
#define INFCAVE_TRACE               FALSE

// Object event templates the generator may write per room. Leaves room for the
// player and a follower inside OBJECT_EVENTS_COUNT.
#define INFCAVE_MAX_OBJECTS         12

#endif // GUARD_CONSTANTS_INFINITY_CAVE_H
