#include "global.h"
#include "infinity_cave.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "event_data.h"
#include "field_weather.h"
#include "item.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "overworld.h"
#include "overworld_overlay.h"
#include "palette.h"
#include "random.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "shop.h"
#include "string_util.h"
#include "task.h"
#include "config/battle.h"
#include "constants/battle.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/opponents.h"
#include "constants/overworld_overlay.h"
#include "constants/pokemon.h"
#include "constants/vars.h"
#include "constants/weather.h"
#include "data/infinity_cave_modifiers.h"
#include "data/infinity_cave_nodes.h"
#include "data/infinity_cave_rooms.h"

static struct InfinityCaveRun *Run(void)
{
    return &gSaveBlock1Ptr->infinityCaveRun;
}

bool32 InfCave_IsInRun(void)
{
    return Run()->active != 0;
}

u32 InfCave_GetDepth(void)
{
    return Run()->depth;
}

u32 InfCave_GetShards(void)
{
    return Run()->shards;
}

void InfCave_StartRun(void)
{
    struct InfinityCaveRun *run = Run();

    memset(run, 0, sizeof(*run));
    InfCave_ClearTrainers();
    run->runSeed = Random32();
    run->active = TRUE;
    run->depth = 0;
    run->roomSeed = run->runSeed;
}

void InfCave_EndRun(enum InfCaveEndReason reason)
{
    // reason is what the caller already acted on (payout, warp, messaging); the
    // run struct keeps no history of it.
    (void)reason;
    InfCave_ClearTrainers();
    InfCave_ClearModifiers();
#if B_FLAG_NO_WHITEOUT != 0
    FlagClear(B_FLAG_NO_WHITEOUT);
#endif
    memset(Run(), 0, sizeof(struct InfinityCaveRun));
}

// Facility-style loss handling, armed by the room generator so it also covers the
// continue-from-save path: a cave defeat ends the descent and returns the player
// to the lobby instead of whiting out. Skipped in Nuzlocke mode, where a loss
// keeps the normal white-out consequence (src/battle_setup.c branches on the same
// condition). Cleared by InfCave_EndRun.
void InfCave_ArmNoWhiteout(void)
{
#if B_FLAG_NO_WHITEOUT != 0
    if (!gSaveBlock1Ptr->nuzlockeModeEnabled)
        FlagSet(B_FLAG_NO_WHITEOUT);
#endif
}

void InfCave_EndRunQuit(void)
{
    // Returning to the lobby restores the party, the same as a facility does on
    // the way out. Gated on an active run so the lobby is not a free heal spot
    // for a player who walks in without descending.
    if (InfCave_IsInRun())
        HealPlayerParty();

    InfCave_EndRun(INFCAVE_END_QUIT);
}

// A save made inside a room only replays if the run struct still describes that
// room. Reject anything the generator and the room scripts cannot act on: no
// active run, the lobby depth, a zeroed room seed, or an out-of-range room type
// or modifier.
bool32 InfCave_IsRunConsistent(void)
{
    struct InfinityCaveRun *run = Run();
    u32 i;

    if (!run->active || run->depth == 0 || run->roomSeed == 0)
        return FALSE;

    if (run->roomType >= INFCAVE_ROOM_COUNT)
        return FALSE;

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        if (run->modifier[i] >= INFCAVE_MOD_COUNT)
            return FALSE;
    }

    return TRUE;
}

// Room ON_LOAD special. ON_TRANSITION does not run on the continue-from-save
// path, so the check lives in ON_LOAD, which both the warp and the reload paths
// reach through InfCave_GenerateRoom. VAR_TEMP_1 hands the eject to the room's
// ON_FRAME script, since a warp cannot run from ON_LOAD.
void InfCave_ValidateRoom(void)
{
    VarSet(VAR_TEMP_1, InfCave_IsRunConsistent() ? 0 : 1);
}

// Seed of the room at a depth. Derived from runSeed and the depth alone, so the
// node roll can read the next depth's copy before the descent and see the same
// number the room itself will generate from.
static u32 RoomSeedForDepth(u32 depth)
{
    return ISO_RANDOMIZE2(Run()->runSeed + depth * 2654435761u);
}

void InfCave_AdvanceDepth(void)
{
    struct InfinityCaveRun *run = Run();

    // The next room rolls its own opponents into the same slots, so the stub ids'
    // defeat flags have to go with the depth the player just left. Clearing them
    // here rather than in the generator keeps a reload inside a room from
    // resurrecting the trainers already beaten in it.
    InfCave_ClearTrainers();

    // What the player took out of the room they are leaving goes with the depth
    // too: the next room rebuilds its own item balls and nurse from its seed.
    run->roomFlags = 0;

    run->depth++;
    // The room is identical no matter which nodes were taken to reach this depth.
    run->roomSeed = RoomSeedForDepth(run->depth);

    // Default room type for the new depth: a boss on the cadence, a battle
    // otherwise. The node roll overwrites this with the option the player picked,
    // and the debug room forcer overwrites it too, since both call InfCave_SetRoom
    // after this.
    InfCave_SetRoom((run->depth % INFCAVE_BOSS_INTERVAL) == 0 ? INFCAVE_ROOM_BOSS
                                                             : INFCAVE_ROOM_BATTLE,
                    NULL, NULL);
}

// Depth tint. Each floor rotates the cave's hue another
// INFCAVE_HUE_PERCENT_PER_DEPTH of the circle, so a deep run reads as a
// different place than a shallow one. The overlay is OVERLAY_LAYER_WORLD, so it
// moves the map tiles only; NPCs, the player and the UI keep their own colours.
// Called from the room generator, which both the warp and the reload path reach
// after Overlay_ResetAll has cleared the previous floor's overlay.
void InfCave_ApplyDepthHue(void)
{
    struct OverlayConfig config = {0};
    u32 hue = (InfCave_GetDepth() * OVERLAY_HUE_FULL_TURN * INFCAVE_HUE_PERCENT_PER_DEPTH) / 100;

    // The angle is a full u8 turn, so the shift wraps rather than saturating.
    hue &= OVERLAY_HUE_FULL_TURN - 1;
    if (hue == 0)
        return; // depth 0, and every wrap back to it, needs no overlay

    config.color = hue;
    config.opacity = OVERLAY_OPACITY_MAX;
    config.layer = OVERLAY_LAYER_WORLD;
    config.scope = OVERLAY_SCOPE_MAP_LOCAL;
    config.effect = OVERLAY_EFFECT_HUE_SHIFT;
    Overlay_Create(&config);
}

void InfCave_SetRoom(u32 roomType, const u8 *modifiers, const u8 *modifierArgs)
{
    struct InfinityCaveRun *run = Run();
    u32 i;

    run->roomType = roomType;
    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        run->modifier[i] = (modifiers != NULL) ? modifiers[i] : (u8)INFCAVE_MOD_NONE;
        run->modifierArg[i] = (modifierArgs != NULL) ? modifierArgs[i] : 0;
    }
}

// Descent ladder gate, read with specialvar. A boss room's ladder stays sealed
// until the boss is beaten, so the arena fight cannot be walked past. The stub
// id's defeat flag is cleared on every depth change, so the next room's ladder
// is free again.
u16 InfCave_IsExitLocked(void)
{
    if (Run()->roomType != INFCAVE_ROOM_BOSS)
        return FALSE;

    // A room that stood no trainer at all has no boss to beat; leaving the gate
    // armed there would strand the player.
    if (InfCave_GetRoomTrainerCount() == 0)
        return FALSE;

    return !HasTrainerBeenFought(TRAINER_INFCAVE_BOSS);
}

u32 InfCave_GetRoomType(void)
{
    return Run()->roomType;
}

u32 InfCave_GetModifier(u32 slot)
{
    if (slot >= INFCAVE_MAX_MODIFIERS)
        return INFCAVE_MOD_NONE;
    return Run()->modifier[slot];
}

u32 InfCave_GetModifierArg(u32 slot)
{
    if (slot >= INFCAVE_MAX_MODIFIERS)
        return 0;
    return Run()->modifierArg[slot];
}

bool32 InfCave_HasModifier(u32 modifier)
{
    u32 i;

    if (modifier == INFCAVE_MOD_NONE)
        return FALSE;

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        if (Run()->modifier[i] == modifier)
            return TRUE;
    }
    return FALSE;
}

const struct InfCaveModifierInfo *InfCave_GetModifierInfo(u32 modifier)
{
    if (modifier == INFCAVE_MOD_NONE || modifier >= INFCAVE_MOD_COUNT)
        return NULL;

    return &sInfCaveModifiers[modifier];
}

// Display name for a modifier slot's argument byte: the type, weather or terrain
// the slot names. NULL for a modifier that takes no argument, so the node screen
// prints its name on its own.
const u8 *InfCave_GetModifierArgName(u32 modifier, u32 arg)
{
    const struct InfCaveModifierInfo *info = InfCave_GetModifierInfo(modifier);
    u32 i;

    if (info == NULL)
        return NULL;

    switch (info->argKind)
    {
    case INFCAVE_MOD_ARG_TYPE:
        return (arg < NUMBER_OF_MON_TYPES) ? gTypesInfo[arg].name : NULL;
    case INFCAVE_MOD_ARG_WEATHER:
        for (i = 0; i < ARRAY_COUNT(sInfCaveWeathers); i++)
        {
            if (sInfCaveWeathers[i].startingStatus == arg)
                return sInfCaveWeathers[i].name;
        }
        return NULL;
    case INFCAVE_MOD_ARG_TERRAIN:
        for (i = 0; i < ARRAY_COUNT(sInfCaveTerrains); i++)
        {
            if (sInfCaveTerrains[i].startingStatus == arg)
                return sInfCaveTerrains[i].name;
        }
        return NULL;
    }

    return NULL;
}

// Read from both rows, so the table only has to name a bad pair once. One
// modifier twice in a room is refused here too: a second slot holding it would
// advertise an effect the room does not apply twice.
bool32 InfCave_ModifiersCompatible(u32 a, u32 b)
{
    const struct InfCaveModifierInfo *rowA = InfCave_GetModifierInfo(a);
    const struct InfCaveModifierInfo *rowB = InfCave_GetModifierInfo(b);

    if (rowA == NULL || rowB == NULL)
        return TRUE;
    if (a == b)
        return FALSE;
    if (rowA->incompatible & INFCAVE_MOD_BIT(b))
        return FALSE;

    return (rowB->incompatible & INFCAVE_MOD_BIT(a)) == 0;
}

// Weighted pick over the rows the current depth has unlocked, one roller per
// option table. Row 0 of each table is unlocked at depth 0, so the fallbacks
// below are only reached if a table is ever authored without one.
static u32 RollWeatherStatus(rng_value_t *rng, u32 depth)
{
    u32 total = 0, roll, i;

    for (i = 0; i < ARRAY_COUNT(sInfCaveWeathers); i++)
    {
        if (sInfCaveWeathers[i].minDepth <= depth)
            total += sInfCaveWeathers[i].weight;
    }

    if (total == 0)
        return sInfCaveWeathers[0].startingStatus;

    roll = InfCave_RandRange(rng, 0, total - 1);
    for (i = 0; i < ARRAY_COUNT(sInfCaveWeathers); i++)
    {
        if (sInfCaveWeathers[i].minDepth > depth)
            continue;
        if (roll < sInfCaveWeathers[i].weight)
            return sInfCaveWeathers[i].startingStatus;
        roll -= sInfCaveWeathers[i].weight;
    }
    return sInfCaveWeathers[0].startingStatus;
}

static u32 RollTerrainStatus(rng_value_t *rng, u32 depth)
{
    u32 total = 0, roll, i;

    for (i = 0; i < ARRAY_COUNT(sInfCaveTerrains); i++)
    {
        if (sInfCaveTerrains[i].minDepth <= depth)
            total += sInfCaveTerrains[i].weight;
    }

    if (total == 0)
        return sInfCaveTerrains[0].startingStatus;

    roll = InfCave_RandRange(rng, 0, total - 1);
    for (i = 0; i < ARRAY_COUNT(sInfCaveTerrains); i++)
    {
        if (sInfCaveTerrains[i].minDepth > depth)
            continue;
        if (roll < sInfCaveTerrains[i].weight)
            return sInfCaveTerrains[i].startingStatus;
        roll -= sInfCaveTerrains[i].weight;
    }
    return sInfCaveTerrains[0].startingStatus;
}

// depth is the depth the argument's option tables are read at, which is the
// target depth for a node roll rather than the depth the player stands on.
static u32 RollModifierArgAtDepth(u32 modifier, rng_value_t *rng, u32 depth)
{
    const struct InfCaveModifierInfo *info = InfCave_GetModifierInfo(modifier);

    if (info == NULL)
        return 0;

    switch (info->argKind)
    {
    case INFCAVE_MOD_ARG_TYPE:
        return sInfCaveMonotypes[InfCave_RandRange(rng, 0, ARRAY_COUNT(sInfCaveMonotypes) - 1)];
    case INFCAVE_MOD_ARG_WEATHER:
        return RollWeatherStatus(rng, depth);
    case INFCAVE_MOD_ARG_TERRAIN:
        return RollTerrainStatus(rng, depth);
    }
    return 0;
}

u32 InfCave_RollModifierArg(u32 modifier, rng_value_t *rng)
{
    return RollModifierArgAtDepth(modifier, rng, InfCave_GetDepth());
}

u32 InfCave_GetModifierShardPercent(void)
{
    u32 percent = 0, i;

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        const struct InfCaveModifierInfo *info = InfCave_GetModifierInfo(InfCave_GetModifier(i));

        if (info != NULL)
            percent += info->shardPercent;
    }
    return percent;
}

// Field weather a rolled INFCAVE_MOD_WEATHER argument runs the room at, or
// WEATHER_NONE for an argument no row claims.
static u32 FieldWeatherForStatus(u32 startingStatus)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sInfCaveWeathers); i++)
    {
        if (sInfCaveWeathers[i].startingStatus == startingStatus)
            return sInfCaveWeathers[i].fieldWeather;
    }
    return WEATHER_NONE;
}

// Field side of the modifiers, run from the generator: both the warp and the
// reload path reach it, and both reach it before the map load starts the weather
// and the flash scanline effect, so what is set here is what the room opens with.
// The room states its flash level and its weather outright rather than only when
// a modifier asks for one, since both live in the save block and would otherwise
// carry over from the floor before.
void InfCave_ApplyModifiers(void)
{
    u32 i;

    SetFlashLevel(InfCave_HasModifier(INFCAVE_MOD_DARK) ? INFCAVE_DARK_FLASH_LEVEL : 0);

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        if (InfCave_GetModifier(i) != INFCAVE_MOD_WEATHER)
            continue;

        SetSavedWeather(FieldWeatherForStatus(InfCave_GetModifierArg(i)));
        return;
    }

    SetSavedWeatherFromCurrMapHeader();
}

void InfCave_ClearModifiers(void)
{
    InfCave_SetRoom(InfCave_GetRoomType(), NULL, NULL);
    SetFlashLevel(0);
    SetSavedWeatherFromCurrMapHeader();
}

// The Bag is refused for a cave battle in a room carrying the modifier. Keyed on
// the trainer redirect rather than on the map, so nothing outside a cave battle
// loses its items.
bool32 InfCave_IsBagLocked(void)
{
    return gInfCaveBattleActive && InfCave_HasModifier(INFCAVE_MOD_NO_ITEMS);
}

void InfCave_AddShards(u32 amount)
{
    struct InfinityCaveRun *run = Run();
    u32 total = run->shards + amount;

    run->shards = total > 0xFFFF ? 0xFFFF : total;
}

bool32 InfCave_SpendShards(u32 amount)
{
    struct InfinityCaveRun *run = Run();

    if (run->shards < amount)
        return FALSE;

    run->shards -= amount;
    return TRUE;
}

// salt keeps unrelated consumers of one room (mask, decoration, trainers) on
// separate streams; pass a distinct constant from each.
rng_value_t InfCave_SeedRoomRng(u32 salt)
{
    return LocalRandomSeed(Run()->roomSeed ^ salt);
}

// Rolls that must be stable for a whole run rather than rebuilt per room draw
// from runSeed instead, so advancing the depth does not reshuffle them.
rng_value_t InfCave_SeedRunRng(u32 salt)
{
    return LocalRandomSeed(Run()->runSeed ^ salt);
}

u32 InfCave_Rand(rng_value_t *rng)
{
    return LocalRandom32(rng);
}

// Inclusive on both ends. lo > hi returns lo.
u32 InfCave_RandRange(rng_value_t *rng, u32 lo, u32 hi)
{
    if (hi <= lo)
        return lo;
    return lo + LocalRandom32(rng) % (hi - lo + 1);
}

// --- Node option roll -------------------------------------------------------

// The options are rolled off the next depth's seed before the descent, on a
// stream of their own, so nothing the room itself generates is shifted by them.
#define INFCAVE_SALT_NODES 0x4E4F4445u // 'NODE'

const struct InfCaveRoomInfo *InfCave_GetRoomInfo(u32 roomType)
{
    if (roomType >= INFCAVE_ROOM_COUNT)
        return NULL;

    return &sInfCaveRooms[roomType];
}

u32 InfCave_GetNodeDepth(void)
{
    return InfCave_GetDepth() + 1;
}

static u32 NodeOptionCount(u32 depth)
{
    if (depth >= INFCAVE_OPTIONS_5_DEPTH)
        return INFCAVE_MAX_OPTIONS;
    if (depth >= INFCAVE_OPTIONS_4_DEPTH)
        return INFCAVE_MIN_OPTIONS + 1;

    return INFCAVE_MIN_OPTIONS;
}

// The cadence gates below are depth arithmetic rather than counters in the run
// struct: the roll then writes nothing, so reopening the screen or reloading a
// save inside the lobby re-offers the same set.
static bool32 IsBossDepth(u32 depth)
{
    return depth != 0 && (depth % INFCAVE_BOSS_INTERVAL) == 0;
}

static bool32 RoomTypeOffered(u32 roomType, u32 depth)
{
    const struct InfCaveRoomInfo *info = &sInfCaveRooms[roomType];

    if (info->weight == 0 || depth < info->minDepth)
        return FALSE;

    // The merchant's rate limit. A shop every floor would turn shards into a
    // vending machine rather than a decision about when to spend them.
    if (roomType == INFCAVE_ROOM_SHOP && (depth % INFCAVE_SHOP_MIN_GAP) != 0)
        return FALSE;

    return TRUE;
}

// Weighted pick over the room types the depth offers. usedMask holds a bit per
// room type already in the set and those are skipped on the first pass, so a set
// reads as distinct choices; the second pass allows a repeat, which is what a
// shallow depth with only two offered types needs to fill three cards.
static u32 RollRoomType(u32 depth, u32 usedMask, rng_value_t *rng)
{
    u32 pass, total, roll, i;

    for (pass = 0; pass < 2; pass++)
    {
        total = 0;
        for (i = 0; i < INFCAVE_ROOM_COUNT; i++)
        {
            if (!RoomTypeOffered(i, depth) || (pass == 0 && (usedMask & (1 << i))))
                continue;
            total += sInfCaveRooms[i].weight;
        }

        if (total == 0)
            continue;

        roll = InfCave_RandRange(rng, 0, total - 1);
        for (i = 0; i < INFCAVE_ROOM_COUNT; i++)
        {
            if (!RoomTypeOffered(i, depth) || (pass == 0 && (usedMask & (1 << i))))
                continue;
            if (roll < sInfCaveRooms[i].weight)
                return i;
            roll -= sInfCaveRooms[i].weight;
        }
    }

    // INFCAVE_ROOM_BATTLE is offered at every depth, so no card is ever left
    // without a room type.
    return INFCAVE_ROOM_BATTLE;
}

// Modifier slots one option asks for. The deepest tier the depth reaches states
// the weights, so a shallow node is usually plain and a deep one rarely is.
static u32 RollModifierCount(u32 depth, rng_value_t *rng)
{
    const struct InfCaveModCountTier *tier = &sInfCaveModCounts[0];
    u32 total = 0, roll, i;

    for (i = 0; i < ARRAY_COUNT(sInfCaveModCounts); i++)
    {
        if (sInfCaveModCounts[i].minDepth <= depth)
            tier = &sInfCaveModCounts[i];
    }

    for (i = 0; i <= INFCAVE_MAX_MODIFIERS; i++)
        total += tier->weight[i];

    if (total == 0)
        return 0;

    roll = InfCave_RandRange(rng, 0, total - 1);
    for (i = 0; i <= INFCAVE_MAX_MODIFIERS; i++)
    {
        if (roll < tier->weight[i])
            return i;
        roll -= tier->weight[i];
    }
    return 0;
}

// TRUE while modifier may still join the option: the room type's mask allows it,
// the depth has unlocked it, and it shares a room with every slot already filled.
static bool32 ModifierFitsOption(const struct InfCaveNodeOption *option, u32 modifier, u32 depth, u32 filled)
{
    const struct InfCaveModifierInfo *info = InfCave_GetModifierInfo(modifier);
    u32 i;

    if (info == NULL || info->weight == 0 || depth < info->minDepth)
        return FALSE;
    if ((sInfCaveRooms[option->roomType].modifierMask & INFCAVE_MOD_BIT(modifier)) == 0)
        return FALSE;

    for (i = 0; i < filled; i++)
    {
        if (!InfCave_ModifiersCompatible(option->modifier[i], modifier))
            return FALSE;
    }
    return TRUE;
}

// Fills an option's modifier slots, stopping early when the rolled count asks for
// more than the room type and the depth can legally supply.
static void RollOptionModifiers(struct InfCaveNodeOption *option, u32 depth, rng_value_t *rng)
{
    u32 want = RollModifierCount(depth, rng);
    u32 filled, total, roll, i;

    for (filled = 0; filled < want; filled++)
    {
        total = 0;
        for (i = 0; i < INFCAVE_MOD_COUNT; i++)
        {
            if (ModifierFitsOption(option, i, depth, filled))
                total += sInfCaveModifiers[i].weight;
        }

        if (total == 0)
            return;

        roll = InfCave_RandRange(rng, 0, total - 1);
        for (i = 0; i < INFCAVE_MOD_COUNT; i++)
        {
            if (!ModifierFitsOption(option, i, depth, filled))
                continue;
            if (roll < sInfCaveModifiers[i].weight)
                break;
            roll -= sInfCaveModifiers[i].weight;
        }

        if (i >= INFCAVE_MOD_COUNT)
            return;

        option->modifier[filled] = i;
        option->modifierArg[filled] = RollModifierArgAtDepth(i, rng, depth);
    }
}

u32 InfCave_RollNodeOptions(struct InfCaveNodeOption *options)
{
    u32 depth = InfCave_GetNodeDepth();
    rng_value_t rng = LocalRandomSeed(RoomSeedForDepth(depth) ^ INFCAVE_SALT_NODES);
    u32 count, usedMask = 0, i;

    memset(options, 0, sizeof(struct InfCaveNodeOption) * INFCAVE_MAX_OPTIONS);

    // A boss depth offers no choice: the one node is the arena. Its ladder still
    // lets the player leave the cave, so the fight is not forced.
    if (IsBossDepth(depth))
    {
        options[0].roomType = INFCAVE_ROOM_BOSS;
        RollOptionModifiers(&options[0], depth, &rng);
        return 1;
    }

    count = NodeOptionCount(depth);
    i = 0;

    // The rest guarantee takes the first card, so healing stays reachable however
    // the weighted rolls fall.
    if ((depth % INFCAVE_REST_MAX_GAP) == 0 && RoomTypeOffered(INFCAVE_ROOM_REST, depth))
    {
        options[i].roomType = INFCAVE_ROOM_REST;
        usedMask |= 1 << INFCAVE_ROOM_REST;
        i++;
    }

    for (; i < count; i++)
    {
        options[i].roomType = RollRoomType(depth, usedMask, &rng);
        usedMask |= 1 << options[i].roomType;
    }

    // Modifiers are rolled once every room type is settled, so the forced card
    // and the rolled ones draw from the stream in one fixed order and a modifier's
    // legality is judged against the room type it will carry.
    for (i = 0; i < count; i++)
        RollOptionModifiers(&options[i], depth, &rng);

    return count;
}

// Rolls the set at a spread of depths and logs it: one line per option, one more
// per filled modifier slot. Reads the live run's seed, so the sets printed are the
// ones that run will be offered.
void InfCave_DebugDumpNodeOptions(void)
{
#ifndef NDEBUG
    static const u8 sSampleDepths[] = { 0, 1, 2, 3, 4, 5, 7, 9, 11, 14, 19, 29, 39, 49 };
    struct InfinityCaveRun *run = Run();
    struct InfCaveNodeOption options[INFCAVE_MAX_OPTIONS];
    u32 savedDepth = run->depth;
    u32 sample, count, i, slot;

    for (sample = 0; sample < ARRAY_COUNT(sSampleDepths); sample++)
    {
        run->depth = sSampleDepths[sample];
        count = InfCave_RollNodeOptions(options);
        DebugPrintf("InfCave depth %d: %d options", InfCave_GetNodeDepth(), count);

        for (i = 0; i < count; i++)
        {
            const struct InfCaveRoomInfo *room = InfCave_GetRoomInfo(options[i].roomType);

            DebugPrintf("  %d: %S", i, room->name);
            for (slot = 0; slot < INFCAVE_MAX_MODIFIERS; slot++)
            {
                const struct InfCaveModifierInfo *mod = InfCave_GetModifierInfo(options[i].modifier[slot]);

                if (mod != NULL)
                    DebugPrintf("     %S arg %d", mod->name, options[i].modifierArg[slot]);
            }
        }
    }

    run->depth = savedDepth;
#endif
}

// --- Descent ----------------------------------------------------------------

// Hands the field over to the node screen. The script that called this waits on
// waitstate; CB2_ReturnToFieldContinueScriptPlayMapMusic resumes it once a card
// is confirmed, with the pick left in InfCave_GetChosenNode.
static void Task_OpenNodeScreen(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    CleanupOverworldWindowsAndTilemaps();
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    SetMainCallback2(CB2_InitInfCaveNodeScreen);
    DestroyTask(taskId);
}

void InfCave_OpenNodeScreen(void)
{
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_OpenNodeScreen, 0);
}

// Second half of the descent step: advances the depth, then overwrites the
// default room type with the node the screen returned. The screen has to run
// before this, since the roll reads the depth the player still stands on. A
// screen that returned nothing leaves InfCave_AdvanceDepth's default standing,
// so the descent never stalls.
void InfCave_TakeChosenNode(void)
{
    const struct InfCaveNodeOption *option = InfCave_GetChosenNode();

    InfCave_AdvanceDepth();
    if (option != NULL)
        InfCave_SetRoom(option->roomType, option->modifier, option->modifierArg);
}

// --- Rest, treasure and shop rooms ------------------------------------------

// Separate RNG streams inside one room, following the generator's convention: a
// new consumer must not shift the numbers an existing one draws.
#define INFCAVE_SALT_BALLS 0x42414C4Cu // 'BALL', the ball count
#define INFCAVE_SALT_DROP  0x44524F50u // 'DROP', plus the ball slot

static bool32 RoomFlagGet(u32 mask)
{
    return (Run()->roomFlags & mask) != 0;
}

static void RoomFlagSet(u32 mask)
{
    Run()->roomFlags |= mask;
}

// Item tier the current depth rolls on, saturating at the deepest tier.
static u32 ItemTier(void)
{
    u32 tier = InfCave_GetDepth() / INFCAVE_ITEM_TIER_DEPTH;

    return tier >= INFCAVE_ITEM_TIER_COUNT ? INFCAVE_ITEM_TIER_COUNT - 1 : tier;
}

// Picks one row of table by weight, from the rows the depth's tier has unlocked.
// NULL only when the tier unlocked none, which the tables are authored against:
// both hold tier 0 rows.
static const struct InfCaveItemDrop *RollDrop(const struct InfCaveItemDrop *table, u32 count, rng_value_t *rng)
{
    u32 tier = ItemTier();
    u32 total = 0, roll, i;

    for (i = 0; i < count; i++)
    {
        if (table[i].minTier <= tier)
            total += table[i].weight;
    }

    if (total == 0)
        return NULL;

    roll = InfCave_RandRange(rng, 0, total - 1);
    for (i = 0; i < count; i++)
    {
        if (table[i].minTier > tier)
            continue;
        if (roll < table[i].weight)
            return &table[i];
        roll -= table[i].weight;
    }
    return NULL;
}

u32 InfCave_RollItemBallCount(void)
{
    rng_value_t rng;
    u32 count = 0;

    if (InfCave_GetRoomType() == INFCAVE_ROOM_TREASURE)
    {
        rng = InfCave_SeedRoomRng(INFCAVE_SALT_BALLS);
        count = InfCave_RandRange(&rng, INFCAVE_TREASURE_MIN_BALLS, INFCAVE_TREASURE_MAX_BALLS);
    }

    if (InfCave_HasModifier(INFCAVE_MOD_TREASURED))
        count += INFCAVE_TREASURED_BALLS;

    return count > INFCAVE_MAX_ITEM_BALLS ? INFCAVE_MAX_ITEM_BALLS : count;
}

bool32 InfCave_IsItemBallTaken(u32 slot)
{
    if (slot >= INFCAVE_MAX_ITEM_BALLS)
        return TRUE;

    return RoomFlagGet(INFCAVE_ROOMFLAG_BALL(slot));
}

// Item ball script special. The ball's slot arrives in VAR_0x800A; the item and
// the amount leave in the vars STD_FIND_ITEM reads. The roll is a pure function
// of the room seed and the slot, so a ball holds the same item after a reload and
// a save-scum cannot reroll it.
void InfCave_SetItemBallItem(void)
{
    u32 slot = VarGet(VAR_0x800A);
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_DROP + slot);
    const struct InfCaveItemDrop *drop;

    // Ball 0 holds the room's depth-scaled drop; the rest hold ordinary supplies.
    if (slot == 0)
        drop = RollDrop(sInfCavePremiumDrops, ARRAY_COUNT(sInfCavePremiumDrops), &rng);
    else
        drop = RollDrop(sInfCaveCommonDrops, ARRAY_COUNT(sInfCaveCommonDrops), &rng);

    gSpecialVar_0x8000 = (drop != NULL) ? drop->item : ITEM_POTION;
    gSpecialVar_0x8001 = (drop != NULL) ? drop->amount : 1;
}

// Records that the ball in VAR_0x800A was emptied, so the generator leaves it out
// when the room is rebuilt.
void InfCave_MarkItemBallTaken(void)
{
    u32 slot = VarGet(VAR_0x800A);

    if (slot < INFCAVE_MAX_ITEM_BALLS)
        RoomFlagSet(INFCAVE_ROOMFLAG_BALL(slot));
}

// Rest room heal, read with specialvar. One heal per room; the bit lives in the
// run struct, so talking to the nurse again after a reload finds it spent.
u16 InfCave_IsShrineUsed(void)
{
    return RoomFlagGet(INFCAVE_ROOMFLAG_SHRINE);
}

void InfCave_UseShrine(void)
{
    RoomFlagSet(INFCAVE_ROOMFLAG_SHRINE);
}

// TRUE while the depth reaches a stock row.
static bool32 ShopRowStocked(const struct InfCaveShopEntry *entry)
{
    return InfCave_GetDepth() >= entry->minDepth;
}

// The mart's item and price lists. They are read while the mart runs, after the
// special that filled them has returned, so they are not allocated.
static EWRAM_DATA u16 sShopMartItems[ARRAY_COUNT(sInfCaveShopStock) + 1] = {0};
static EWRAM_DATA u16 sShopMartPrices[ARRAY_COUNT(sInfCaveShopStock)] = {0};

// TRUE while the merchant has anything to sell at this depth, read with
// specialvar before the mart is opened.
void InfCaveShop_HasStock(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sInfCaveShopStock); i++)
    {
        if (ShopRowStocked(&sInfCaveShopStock[i]))
        {
            gSpecialVar_Result = TRUE;
            return;
        }
    }
    gSpecialVar_Result = FALSE;
}

// Fills the mart's lists from the stocked rows and hands them to the shard mart,
// which charges in shards and shows no currency symbol. The script waits on the
// mart with waitstate.
void InfCaveShop_OpenMart(void)
{
    u32 i, count = 0;

    for (i = 0; i < ARRAY_COUNT(sInfCaveShopStock); i++)
    {
        const struct InfCaveShopEntry *entry = &sInfCaveShopStock[i];

        if (!ShopRowStocked(entry))
            continue;

        sShopMartItems[count] = entry->item;
        sShopMartPrices[count] = entry->price;
        count++;
    }
    sShopMartItems[count] = ITEM_NONE;

    CreateShardMartMenu(sShopMartItems, sShopMartPrices);
}
