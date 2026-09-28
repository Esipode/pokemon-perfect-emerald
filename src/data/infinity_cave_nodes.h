#ifndef GUARD_DATA_INFINITY_CAVE_NODES_H
#define GUARD_DATA_INFINITY_CAVE_NODES_H

// Node roll data: what the options offered before a descent may be. Included
// from src/infinity_cave.c only; the node screen reads a row through
// InfCave_GetRoomInfo.

static const u8 sInfCaveRoomName_Battle[] = _("BATTLE");
static const u8 sInfCaveRoomName_Gauntlet[] = _("GAUNTLET");
static const u8 sInfCaveRoomName_Elite[] = _("ELITE");
static const u8 sInfCaveRoomName_Boss[] = _("BOSS");
static const u8 sInfCaveRoomName_Rest[] = _("REST");
static const u8 sInfCaveRoomName_Treasure[] = _("TREASURE");
static const u8 sInfCaveRoomName_Shop[] = _("MERCHANT");

static const u8 sInfCaveRoomDesc_Battle[] = _("Two or three trainers.");
static const u8 sInfCaveRoomDesc_Gauntlet[] = _("A long line of trainers.");
static const u8 sInfCaveRoomDesc_Elite[] = _("One trainer, full party.");
static const u8 sInfCaveRoomDesc_Boss[] = _("A champion bars the way.");
static const u8 sInfCaveRoomDesc_Rest[] = _("A nurse heals your team.");
static const u8 sInfCaveRoomDesc_Treasure[] = _("Item balls lie unopened.");
static const u8 sInfCaveRoomDesc_Shop[] = _("A merchant takes shards.");

// Modifier groups a room type may carry. A battle modifier on a room that stands
// no trainer would advertise an effect the room never applies, and a hazard is
// paid for by the shards and the fight it complicates, so a trainerless room
// type carries the boons alone.
#define INFCAVE_MODS_HAZARD (INFCAVE_MOD_BIT(INFCAVE_MOD_DARK)          \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_CRAMPED))
#define INFCAVE_MODS_BOON   (INFCAVE_MOD_BIT(INFCAVE_MOD_TREASURED))
#define INFCAVE_MODS_FIELD  (INFCAVE_MODS_HAZARD | INFCAVE_MODS_BOON)
#define INFCAVE_MODS_BATTLE (INFCAVE_MOD_BIT(INFCAVE_MOD_MONOTYPE)      \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_WEATHER)       \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_TERRAIN)       \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_DOUBLES)       \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_SURGE)         \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_GIMMICK)       \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_NO_ITEMS)      \
                           | INFCAVE_MOD_BIT(INFCAVE_MOD_BOUNTY))

// One row per enum InfCaveRoomType. weight 0 is never rolled as an option:
// INFCAVE_ROOM_BOSS is offered by the cadence alone.
static const struct InfCaveRoomInfo sInfCaveRooms[INFCAVE_ROOM_COUNT] =
{
    [INFCAVE_ROOM_BATTLE] =
    {
        .name = sInfCaveRoomName_Battle,
        .description = sInfCaveRoomDesc_Battle,
        .icon = 0,
        .weight = 24,
        .minDepth = 0,
        .modifierMask = INFCAVE_MODS_FIELD | INFCAVE_MODS_BATTLE | INFCAVE_MOD_BIT(INFCAVE_MOD_SWARM),
    },
    [INFCAVE_ROOM_GAUNTLET] =
    {
        .name = sInfCaveRoomName_Gauntlet,
        .description = sInfCaveRoomDesc_Gauntlet,
        .icon = 1,
        .weight = 18,
        .minDepth = 3,
        .modifierMask = INFCAVE_MODS_FIELD | INFCAVE_MODS_BATTLE | INFCAVE_MOD_BIT(INFCAVE_MOD_SWARM),
    },
    [INFCAVE_ROOM_ELITE] =
    {
        .name = sInfCaveRoomName_Elite,
        .description = sInfCaveRoomDesc_Elite,
        .icon = 2,
        .weight = 14,
        .minDepth = 5,
        // No INFCAVE_MOD_SWARM: the room type is one trainer by definition, and
        // the extra two would arrive at the elite's own tier.
        .modifierMask = INFCAVE_MODS_FIELD | INFCAVE_MODS_BATTLE,
    },
    [INFCAVE_ROOM_BOSS] =
    {
        .name = sInfCaveRoomName_Boss,
        .description = sInfCaveRoomDesc_Boss,
        .icon = 3,
        .weight = 0,
        .minDepth = 0,
        .modifierMask = INFCAVE_MODS_FIELD | INFCAVE_MODS_BATTLE,
    },
    [INFCAVE_ROOM_REST] =
    {
        .name = sInfCaveRoomName_Rest,
        .description = sInfCaveRoomDesc_Rest,
        .icon = 4,
        .weight = 8,
        .minDepth = 5,
        .modifierMask = INFCAVE_MODS_BOON,
    },
    [INFCAVE_ROOM_TREASURE] =
    {
        .name = sInfCaveRoomName_Treasure,
        .description = sInfCaveRoomDesc_Treasure,
        .icon = 5,
        .weight = 10,
        .minDepth = 0,
        // The only boon is INFCAVE_MOD_TREASURED, which adds balls to a room that
        // has none; here it would only raise a count the room type already rolls.
        // Nothing is left, so the room type never carries a modifier.
        .modifierMask = 0,
    },
    [INFCAVE_ROOM_SHOP] =
    {
        .name = sInfCaveRoomName_Shop,
        .description = sInfCaveRoomDesc_Shop,
        .icon = 6,
        .weight = 8,
        .minDepth = 6,
        .modifierMask = INFCAVE_MODS_BOON,
    },
};

// How many modifier slots one option fills, by depth. The deepest row the depth
// reaches wins, so the rows must stay in ascending minDepth order. Index into
// weight is the modifier count itself.
struct InfCaveModCountTier
{
    u8 minDepth;
    u8 weight[INFCAVE_MAX_MODIFIERS + 1];
};

static const struct InfCaveModCountTier sInfCaveModCounts[] =
{
    { .minDepth = 0,  .weight = { 100,  0,  0 } },
    { .minDepth = 2,  .weight = {  60, 40,  0 } },
    { .minDepth = 6,  .weight = {  30, 55, 15 } },
    { .minDepth = 15, .weight = {  20, 50, 30 } },
    { .minDepth = 30, .weight = {  10, 45, 45 } },
};

#endif // GUARD_DATA_INFINITY_CAVE_NODES_H
