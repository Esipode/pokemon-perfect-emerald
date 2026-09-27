#include "global.h"
#include "infinity_cave.h"
#include "event_data.h"
#include "random.h"
#include "constants/vars.h"

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
    memset(Run(), 0, sizeof(struct InfinityCaveRun));
}

void InfCave_EndRunQuit(void)
{
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

void InfCave_AdvanceDepth(void)
{
    struct InfinityCaveRun *run = Run();

    run->depth++;
    // Derived from runSeed and depth alone, so the room is identical no matter
    // which nodes were taken to reach this depth.
    run->roomSeed = ISO_RANDOMIZE2(run->runSeed + run->depth * 2654435761u);
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
