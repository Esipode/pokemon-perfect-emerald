#include "global.h"
#include "infinity_cave.h"
#include "battle_emporium.h"
#include "caps.h"
#include "data.h"
#include "event_data.h"
#include "random.h"
#include "string_util.h"
#include "constants/battle.h"
#include "constants/battle_emporium.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/trainers.h"
#include "constants/battle_ai.h"
#include "trainer_pools.h"
#include "data/infinity_cave_trainers.h"

// Trainer rolls take their own RNG stream off the room seed, so the same room
// always produces the same opponents after a reload.
#define INFCAVE_SALT_TRNR 0x54524E52u // 'TRNR'

// One runtime opponent per room trainer slot. The stub ids TRAINER_INFCAVE_0..7
// carry no data of their own: GetTrainerStructFromId redirects them here while
// gInfCaveBattleActive is set. Nothing is saved, because a reload rebuilds the
// room and its trainers from the run's seeds.
EWRAM_DATA static struct Trainer sInfCaveTrainers[INFCAVE_MAX_TRAINERS] = {0};
EWRAM_DATA bool8 gInfCaveBattleActive = FALSE;

// The redirect in data.h forwards one contiguous id range, so the stub ids must
// stay contiguous and cover every slot.
STATIC_ASSERT(TRAINER_INFCAVE_7 == TRAINER_INFCAVE_0 + INFCAVE_MAX_TRAINERS - 1, sInfCaveTrainerIdRange);
STATIC_ASSERT(TRAINER_INFCAVE_7 < TRAINERS_COUNT, sInfCaveTrainerIdsInRange);

u16 InfCave_GetTrainerId(u32 slot)
{
    if (slot >= INFCAVE_MAX_TRAINERS)
        return TRAINER_NONE;
    return TRAINER_INFCAVE_0 + slot;
}

// Out-of-range ids fall back to slot 0 rather than reading past the array; the
// redirect in data.h only forwards ids inside the stub range.
const struct Trainer *InfCave_GetTrainer(u16 trainerId)
{
    u32 slot = trainerId - TRAINER_INFCAVE_0;

    if (trainerId < TRAINER_INFCAVE_0 || slot >= INFCAVE_MAX_TRAINERS)
        slot = 0;

    return &sInfCaveTrainers[slot];
}

// Writable slot for the roll (Stage 13). Callers own every field of the struct.
struct Trainer *InfCave_GetTrainerSlot(u32 slot)
{
    if (slot >= INFCAVE_MAX_TRAINERS)
        return NULL;
    return &sInfCaveTrainers[slot];
}

void InfCave_ArmTrainers(void)
{
    gInfCaveBattleActive = TRUE;
}

// Disarms the redirect, blanks the slots and clears the stub ids' defeat flags,
// so the next room's trainers challenge the player again. Called from the room
// teardown paths and defensively after a battle (src/battle_setup.c), where a
// white-out must never leave every trainer pointed at sInfCaveTrainers.
void InfCave_ClearTrainers(void)
{
    u32 slot;

    gInfCaveBattleActive = FALSE;
    memset(sInfCaveTrainers, 0, sizeof(sInfCaveTrainers));
    for (slot = 0; slot < INFCAVE_MAX_TRAINERS; slot++)
        FlagClear(TRAINER_FLAGS_START + TRAINER_INFCAVE_0 + slot);
}

// Pool tier for the current depth, shifted by the room type's offset and
// clamped to the tiers that exist.
static u32 TierForRoom(u32 depth, s32 tierOffset)
{
    s32 tier = 0;
    u32 i;

    for (i = 0; i < INFCAVE_TIER_COUNT; i++)
    {
        if (depth >= sInfCaveTierMinDepth[i])
            tier = i;
    }

    tier += tierOffset;
    if (tier < 0)
        tier = 0;
    if (tier >= INFCAVE_TIER_COUNT)
        tier = INFCAVE_TIER_COUNT - 1;

    return tier;
}

// Level every rolled cave mon is generated at: the player's progression level cap
// (so an opponent never out-levels a legal player team by more than the depth
// bonus) plus the depth bonus, plus INFCAVE_MOD_SURGE. Read by
// CreateNPCTrainerPartyFromTrainer while gInfCaveBattleActive is set.
u32 InfCave_GetBattleLevel(void)
{
    s32 level = (s32)GetCurrentLevelCap();
    u32 bonus = InfCave_GetDepth() / INFCAVE_LEVEL_DEPTH_PER_STEP;

    if (bonus > INFCAVE_LEVEL_MAX_BONUS)
        bonus = INFCAVE_LEVEL_MAX_BONUS;
    level += bonus;

    if (InfCave_HasModifier(INFCAVE_MOD_SURGE))
        level += INFCAVE_LEVEL_SURGE_BONUS;

    if (level < 1)
        level = 1;
    if (level > MAX_LEVEL)
        level = MAX_LEVEL;

    return level;
}

// Fills one slot from an identity row and the tier's pool. Every field of the
// struct is written here, so a slot never carries anything from the room before.
// Returns the identity's overworld gfx id for the placer (Stage 15).
static u16 BuildTrainer(u32 slot, u32 roomType, rng_value_t *rng)
{
    const struct InfCaveIdentity *identity;
    const struct InfCaveTrainerSpec *spec;
    struct Trainer *trainer = InfCave_GetTrainerSlot(slot);
    const struct TrainerMon *pool;
    u8 poolSize = 0;
    u32 tier;

    if (trainer == NULL)
        return OBJ_EVENT_GFX_HIKER;

    if (roomType >= INFCAVE_ROOM_COUNT)
        roomType = INFCAVE_ROOM_BATTLE;
    spec = &sInfCaveTrainerSpec[roomType];
    if (spec->partySize == 0)
        spec = &sInfCaveTrainerSpec[INFCAVE_ROOM_BATTLE];

    identity = &sInfCaveIdentities[InfCave_Rand(rng) % INFCAVE_IDENTITY_COUNT];
    tier = TierForRoom(InfCave_GetDepth(), spec->tierOffset);
    pool = GetEmporiumPool(sInfCaveTierPool[tier], &poolSize);

    memset(trainer, 0, sizeof(*trainer));
    StringCopy(trainer->trainerName, identity->name);
    trainer->trainerClass = identity->trainerClass;
    trainer->trainerPic = identity->trainerPic;
    trainer->encounterMusic = identity->encounterMusic;
    trainer->gender = identity->gender;
    // Doubles, weather and the other modifiers are applied in Stage 19.
    trainer->battleType = TRAINER_BATTLE_TYPE_SINGLES;
    trainer->aiFlags = spec->aiFlags;
    trainer->party = pool;
    trainer->partySize = spec->partySize;
    trainer->poolSize = poolSize;
    trainer->poolRuleIndex = POOL_RULESET_BASIC;
    trainer->poolPickIndex = POOL_PICK_DEFAULT;
    trainer->poolPruneIndex = POOL_PRUNE_NONE;
    trainer->overrideTrainer = TRAINER_NONE;

    // Narrow bitfields and table indices: catch truncation here, not at battle setup.
    assertf(trainer->partySize == spec->partySize, "partySize %d truncated to %d", spec->partySize, trainer->partySize);
    assertf(trainer->encounterMusic == identity->encounterMusic, "encounterMusic %d truncated to %d", identity->encounterMusic, trainer->encounterMusic);
    assertf(trainer->trainerPic < TRAINER_PIC_COUNT, "trainerPic %d out of range", trainer->trainerPic);
    assertf(StringLength(identity->name) <= TRAINER_NAME_LENGTH, "identity name longer than %d", TRAINER_NAME_LENGTH);
    assertf(identity->objectGfxId < NUM_OBJ_EVENT_GFX, "objectGfxId %d out of range", identity->objectGfxId);
    assertf(pool != NULL && poolSize >= spec->partySize, "tier %d pool too small: %d", tier, poolSize);

    return identity->objectGfxId;
}

// Rolls the opponent for one of the current room's trainer slots and arms the
// redirect. Deterministic: the slot's stream comes from the room seed, so a
// reload inside the room rebuilds the same trainer.
u16 InfCave_BuildTrainer(u32 slot)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_TRNR + slot);
    u16 objectGfxId = BuildTrainer(slot, InfCave_GetRoomType(), &rng);

    InfCave_ArmTrainers();
    return objectGfxId;
}

// Debug harness: rolls slot 0 off the global RNG, so repeated uses outside a run
// (where the room seed is 0) still give different trainers.
// Debug_EventScript_InfCaveTestBattle fights it.
void InfCave_DebugFillTrainer(void)
{
    rng_value_t rng = LocalRandomSeed(Random32());
    u32 roomType = InfCave_IsInRun() ? InfCave_GetRoomType() : INFCAVE_ROOM_BATTLE;

    BuildTrainer(0, roomType, &rng);
    InfCave_ArmTrainers();
}
