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

// Object event templates the generator may write per room. Leaves room for the
// player and a follower inside OBJECT_EVENTS_COUNT.
#define INFCAVE_MAX_OBJECTS         12

#endif // GUARD_CONSTANTS_INFINITY_CAVE_H
