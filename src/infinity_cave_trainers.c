#include "global.h"
#include "infinity_cave.h"
#include "battle_emporium.h"
#include "battle_setup.h"
#include "caps.h"
#include "data.h"
#include "item.h"
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
#include "battle_transition.h"
#include "trainer_pools.h"
#include "data/infinity_cave_trainers.h"

// Trainer rolls take their own RNG stream off the room seed, so the same room
// always produces the same opponents after a reload.
#define INFCAVE_SALT_TRNR 0x54524E52u // 'TRNR'

// The boss order is drawn off the run's seed rather than a room's, so it is one
// permutation for the whole descent.
#define INFCAVE_SALT_BOSS 0x424F5353u // 'BOSS'

// Trainer count draws its own stream, so the count a room rolls does not move
// when a slot's identity roll changes.
#define INFCAVE_SALT_TCNT 0x54434E54u // 'TCNT'

// One runtime opponent per room trainer slot. The stub ids TRAINER_INFCAVE_0..7
// carry no data of their own: GetTrainerStructFromId redirects them here while
// gInfCaveBattleActive is set. Nothing is saved, because a reload rebuilds the
// room and its trainers from the run's seeds.
EWRAM_DATA static struct Trainer sInfCaveTrainers[INFCAVE_MAX_TRAINERS] = {0};
EWRAM_DATA bool8 gInfCaveBattleActive = FALSE;

// Depth tier each built slot rolled on. The pool rules need it (band, gimmick
// kind) and struct Trainer has nowhere to carry it, so it is kept beside the
// slots and read back through the slot the trainer pointer belongs to.
EWRAM_DATA static u8 sInfCaveTrainerTier[INFCAVE_MAX_TRAINERS] = {0};

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
    memset(sInfCaveTrainerTier, 0, sizeof(sInfCaveTrainerTier));
    for (slot = 0; slot < INFCAVE_MAX_TRAINERS; slot++)
        FlagClear(TRAINER_FLAGS_START + TRAINER_INFCAVE_0 + slot);
}

// Bosses the run has already put down. Descending past a boss room needs its
// boss beaten, so every boss depth strictly below the current one is cleared.
static u32 BossesCleared(void)
{
    u32 depth = InfCave_GetDepth();

    if (depth == 0)
        return 0;

    return (depth - 1) / INFCAVE_BOSS_INTERVAL;
}

// Largest party the current depth lets a room type field: its spec size plus one
// member per boss beaten, capped at a legal party.
static u32 MaxPartySize(const struct InfCaveTrainerSpec *spec)
{
    u32 size = spec->partySize + BossesCleared();

    if (size > PARTY_SIZE)
        size = PARTY_SIZE;

    return size;
}

// Party size one rolled cave trainer fields. The floor holds at one until
// INFCAVE_PARTY_RAMP_STEPS bosses are down, then rises with each further boss,
// so a deep room stops fielding short teams at all.
static u32 RollPartySize(const struct InfCaveTrainerSpec *spec, rng_value_t *rng)
{
    u32 cleared = BossesCleared();
    u32 max = MaxPartySize(spec);
    u32 min = 1;

    if (spec->fullParty)
        return max;

    if (cleared > INFCAVE_PARTY_RAMP_STEPS)
        min += cleared - INFCAVE_PARTY_RAMP_STEPS;
    if (min > max)
        min = max;

    return InfCave_RandRange(rng, min, max);
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

// How many trainers the current room places: the room type's rolled count plus
// INFCAVE_MOD_SWARM, clamped to the stub ids that exist. Deterministic off the
// room seed, so the placer and the room's scripts agree after a reload.
u32 InfCave_RollTrainerCount(void)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_TCNT);
    u32 roomType = InfCave_GetRoomType();
    const struct InfCaveTrainerSpec *spec;
    u32 count;

    if (roomType >= INFCAVE_ROOM_COUNT)
        return 0;

    spec = &sInfCaveTrainerSpec[roomType];
    if (spec->maxCount == 0)
        return 0;

    count = InfCave_RandRange(&rng, spec->minCount, spec->maxCount);
    if (InfCave_HasModifier(INFCAVE_MOD_SWARM))
        count += INFCAVE_SWARM_TRAINERS;

    if (count > INFCAVE_MAX_TRAINERS)
        count = INFCAVE_MAX_TRAINERS;

    return count;
}

// Shards one cleared battle in the current room pays: the room type's base
// reward grown by depth, then scaled by the modifiers that advertise a richer
// payout. 0 outside a run, so the debug test battle pays nothing.
u32 InfCave_GetBattleShards(void)
{
    u32 roomType = InfCave_GetRoomType();
    u32 percent = 100;
    u32 base;

    if (!InfCave_IsInRun() || roomType >= INFCAVE_ROOM_COUNT)
        return 0;

    base = sInfCaveTrainerSpec[roomType].shardReward;
    if (base == 0)
        return 0;

    percent += InfCave_GetDepth() * INFCAVE_SHARD_DEPTH_PERCENT;
    percent += InfCave_GetModifierShardPercent();

    return base * percent / 100;
}

// TRUE once every trainer the room actually stood has been beaten. A room that
// stood none is never "cleared", so the empty-placement case pays no bonus.
static bool32 RoomTrainersCleared(void)
{
    u32 count = InfCave_GetRoomTrainerCount();
    u32 i;

    if (count == 0)
        return FALSE;

    for (i = 0; i < count; i++)
    {
        if (!HasTrainerBeenFought(InfCave_GetTrainerId(i)))
            return FALSE;
    }
    return TRUE;
}

// Post-battle script special. Pays the battle out, and one extra battle's worth on
// top when that win emptied the room: the trainer's defeat flag is already set by
// the time the payout line runs, so the last win in a room pays both.
void InfCave_PayBattleShards(void)
{
    InfCave_AddShards(InfCave_GetBattleShards());

    if (RoomTrainersCleared())
        InfCave_AddShards(InfCave_GetBattleShards());
}

// Tier a built slot rolled on, found from the slot the trainer pointer addresses.
// Anything outside the slot array reads tier 0, so a stray pointer prunes to the
// shallowest band instead of indexing past the table.
u32 InfCave_GetTrainerTier(const struct Trainer *trainer)
{
    u32 slot;

    if (trainer < &sInfCaveTrainers[0] || trainer >= &sInfCaveTrainers[INFCAVE_MAX_TRAINERS])
        return 0;

    slot = trainer - &sInfCaveTrainers[0];
    if (sInfCaveTrainerTier[slot] >= INFCAVE_TIER_COUNT)
        return 0;

    return sInfCaveTrainerTier[slot];
}

// enum Type INFCAVE_MOD_MONOTYPE restricts the pools to, or TYPE_NONE when the
// modifier is not active.
static u32 MonotypeArg(void)
{
    u32 i;

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        if (InfCave_GetModifier(i) == INFCAVE_MOD_MONOTYPE)
            return InfCave_GetModifierArg(i);
    }
    return TYPE_NONE;
}

// Filler legality for a rolled cave trainer: a common (non-legendary, non-form)
// species inside the tier's base-stat-total band, and on-type under
// INFCAVE_MOD_MONOTYPE. applyMonotype is FALSE on POOL_PRUNE_INFCAVE's relaxed
// second pass, where the type restriction would leave too few members.
bool32 InfCave_MonAllowedAsFiller(const struct Trainer *trainer, const struct TrainerMon *mon, bool32 applyMonotype)
{
    u32 tier = InfCave_GetTrainerTier(trainer);
    u32 type;

    if (!IsSpeciesCommonWithinBst(mon->species, sInfCaveTierBst[tier].maxBst))
        return FALSE;
    if (GetSpeciesBaseStatTotal(mon->species) < sInfCaveTierBst[tier].minBst)
        return FALSE;

    if (!applyMonotype)
        return TRUE;

    type = MonotypeArg();
    if (type == TYPE_NONE)
        return TRUE;

    return GetSpeciesType(mon->species, 0) == type || GetSpeciesType(mon->species, 1) == type;
}

// TRUE if mon holds a Mega Stone its own species can use.
static bool32 MonHoldsOwnMegaStone(const struct TrainerMon *mon)
{
    const struct FormChange *formChanges = GetSpeciesFormChanges(mon->species);
    u32 i;

    if (formChanges == NULL)
        return FALSE;

    for (i = 0; formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        if (formChanges[i].method == FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM
         && formChanges[i].param1 == mon->heldItem)
            return TRUE;
    }
    return FALSE;
}

// Ace legality under INFCAVE_MOD_GIMMICK: the mon must carry the gimmick its
// tier's pool is built around and be reachable at the room's battle level. Mega
// Stones are matched against the holder's own form-change table; a Z-Crystal's
// move requirement is guaranteed by the pool rows themselves (see
// src/data/battle_emporium.h).
bool32 InfCave_MonMatchesGimmick(const struct Trainer *trainer, const struct TrainerMon *mon)
{
    u32 tier = InfCave_GetTrainerTier(trainer);

    if (EmporiumSpeciesMinLevel(mon->species) > InfCave_GetBattleLevel())
        return FALSE;

    switch (sInfCaveTierPool[tier])
    {
    case EMPORIUM_ZMOVE:
        return gItemsInfo[mon->heldItem].sortType == ITEM_TYPE_Z_CRYSTAL;
    case EMPORIUM_MEGA:
        return MonHoldsOwnMegaStone(mon);
    case EMPORIUM_TERA:
        return mon->teraType != TYPE_NONE;
    default:
        return FALSE;
    }
}

// Sets one field of a struct Trainer's startingStatus from its enum id, the same
// mapping SetStartingStatus uses for the EWRAM copy.
#define UNPACK_STARTING_STATUS_TO_TRAINER(_enum, _fieldName, ...) case _enum: statuses->_fieldName = TRUE; break;

static void SetTrainerStartingStatus(struct StartingStatuses *statuses, u32 status)
{
    switch (status)
    {
    STARTING_STATUS_DEFINITIONS(UNPACK_STARTING_STATUS_TO_TRAINER);
    }
}

// Battle side of the modifiers, run once the slot's own fields are written. The
// room's weather and terrain are the permanent starting statuses, so they hold
// for the whole battle; battle_main.c merges the opponents' statuses into
// gStartingStatuses at setup. Doubles needs no party-size gate:
// OW_DOUBLE_APPROACH_WITH_ONE_MON is on, so a player down to one usable mon
// fights the double battle with it.
void InfCave_ApplyModifiersToTrainer(struct Trainer *trainer)
{
    u32 i;

    if (trainer == NULL)
        return;

    if (InfCave_HasModifier(INFCAVE_MOD_DOUBLES))
        trainer->battleType = TRAINER_BATTLE_TYPE_DOUBLES;

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        u32 modifier = InfCave_GetModifier(i);

        if (modifier == INFCAVE_MOD_WEATHER || modifier == INFCAVE_MOD_TERRAIN)
            SetTrainerStartingStatus(&trainer->startingStatus, InfCave_GetModifierArg(i));
    }
}

u32 InfCave_GetBossIndex(void)
{
    rng_value_t rng = InfCave_SeedRunRng(INFCAVE_SALT_BOSS);
    u8 order[INFCAVE_BOSS_COUNT];
    u32 ordinal = InfCave_GetDepth() / INFCAVE_BOSS_INTERVAL;
    u32 i;

    for (i = 0; i < INFCAVE_BOSS_COUNT; i++)
        order[i] = i;

    // Shuffled rather than walked in table order, so two runs do not open on the
    // same boss; the permutation itself is what stops a repeat inside one run.
    for (i = INFCAVE_BOSS_COUNT - 1; i > 0; i--)
    {
        u32 j = InfCave_RandRange(&rng, 0, i);
        u8 swap = order[i];

        order[i] = order[j];
        order[j] = swap;
    }

    // A boss room forced by debug at a depth the cadence does not call for reads
    // the first entry rather than indexing off the end.
    if (ordinal == 0)
        ordinal = 1;

    return order[(ordinal - 1) % INFCAVE_BOSS_COUNT];
}

// Fills slot 0 from a boss row. The party is the row's named trainer's authored
// team when that team is already a full boss party, and a rolled pool team
// otherwise, so a boss whose canon team is short still fields six. mugshotColor
// is what gives the boss the Elite Four transition: GetTrainerBattleTransition
// reads it straight off this struct.
static u16 BuildBoss(u32 slot)
{
    u32 index = InfCave_GetBossIndex();
    const struct InfCaveBoss *boss = &sInfCaveBosses[index];
    const struct InfCaveTrainerSpec *spec = &sInfCaveTrainerSpec[INFCAVE_ROOM_BOSS];
    struct Trainer *trainer = InfCave_GetTrainerSlot(slot);
    const struct Trainer *source = NULL;
    u32 tier = TierForRoom(InfCave_GetDepth(), spec->tierOffset);
    u32 partySize = MaxPartySize(spec);

    if (trainer == NULL)
        return OBJ_EVENT_GFX_HIKER;

    if (boss->partyTrainer != TRAINER_NONE)
        source = GetTrainerStructFromId(boss->partyTrainer);

    memset(trainer, 0, sizeof(*trainer));
    StringCopy(trainer->trainerName, boss->name);
    trainer->trainerClass = TRAINER_CLASS_CHALLENGER;
    trainer->trainerPic = boss->trainerPic;
    trainer->encounterMusic = boss->encounterMusic;
    trainer->mugshotColor = boss->mugshotColor;
    trainer->gender = boss->gender;
    trainer->battleType = TRAINER_BATTLE_TYPE_SINGLES;
    trainer->aiFlags = spec->aiFlags;
    trainer->partySize = partySize;
    trainer->overrideTrainer = TRAINER_NONE;

    if (source != NULL && source->poolSize == 0 && source->partySize >= partySize)
    {
        // poolSize 0 makes DoTrainerPartyPool take the party straight, in order.
        // The tail is taken rather than the head, so a boss cut short by the ramp
        // keeps its ace and drops its openers.
        trainer->party = source->party + (source->partySize - partySize);
    }
    else
    {
        u8 poolSize = 0;

        trainer->party = GetEmporiumPool(sInfCaveTierPool[tier], &poolSize);
        trainer->poolSize = poolSize;
        trainer->poolRuleIndex = POOL_RULESET_INFCAVE;
        trainer->poolPickIndex = POOL_PICK_INFCAVE;
        trainer->poolPruneIndex = POOL_PRUNE_INFCAVE;
        assertf(trainer->party != NULL && poolSize >= partySize, "boss tier %d pool too small: %d", tier, poolSize);
    }
    sInfCaveTrainerTier[slot] = tier;
    InfCave_ApplyModifiersToTrainer(trainer);

    // Narrow bitfields and table indices: catch truncation here, not at battle setup.
    assertf(trainer->partySize == partySize, "partySize %d truncated to %d", partySize, trainer->partySize);
    assertf(trainer->encounterMusic == boss->encounterMusic, "encounterMusic %d truncated to %d", boss->encounterMusic, trainer->encounterMusic);
    assertf(trainer->mugshotColor == boss->mugshotColor, "mugshotColor %d truncated to %d", boss->mugshotColor, trainer->mugshotColor);
    assertf(boss->mugshotColor != MUGSHOT_COLOR_NONE, "boss %d has no mugshot colour", index);
    assertf(trainer->trainerPic < TRAINER_PIC_COUNT, "trainerPic %d out of range", trainer->trainerPic);
    assertf(StringLength(boss->name) <= TRAINER_NAME_LENGTH, "boss name longer than %d", TRAINER_NAME_LENGTH);
    assertf(boss->objectGfxId < NUM_OBJ_EVENT_GFX, "objectGfxId %d out of range", boss->objectGfxId);

    return boss->objectGfxId;
}

// Fills one slot from an identity row and the tier's pool. Every field of the
// struct is written here, so a slot never carries anything from the room before.
// Returns the identity's overworld gfx id for the placer.
static u16 BuildTrainer(u32 slot, u32 roomType, rng_value_t *rng)
{
    const struct InfCaveIdentity *identity;
    const struct InfCaveTrainerSpec *spec;
    struct Trainer *trainer = InfCave_GetTrainerSlot(slot);
    const struct TrainerMon *pool;
    u8 poolSize = 0;
    u32 tier;
    u32 partySize;

    if (trainer == NULL)
        return OBJ_EVENT_GFX_HIKER;

    // A boss room stands exactly one trainer, and it is a named identity rather
    // than a rolled one.
    if (roomType == INFCAVE_ROOM_BOSS && slot == 0)
        return BuildBoss(slot);

    if (roomType >= INFCAVE_ROOM_COUNT)
        roomType = INFCAVE_ROOM_BATTLE;
    spec = &sInfCaveTrainerSpec[roomType];
    if (spec->partySize == 0)
        spec = &sInfCaveTrainerSpec[INFCAVE_ROOM_BATTLE];

    identity = &sInfCaveIdentities[InfCave_Rand(rng) % INFCAVE_IDENTITY_COUNT];
    partySize = RollPartySize(spec, rng);
    tier = TierForRoom(InfCave_GetDepth(), spec->tierOffset);
    pool = GetEmporiumPool(sInfCaveTierPool[tier], &poolSize);

    memset(trainer, 0, sizeof(*trainer));
    StringCopy(trainer->trainerName, identity->name);
    trainer->trainerClass = identity->trainerClass;
    trainer->trainerPic = identity->trainerPic;
    trainer->encounterMusic = identity->encounterMusic;
    trainer->gender = identity->gender;
    trainer->battleType = TRAINER_BATTLE_TYPE_SINGLES;
    trainer->aiFlags = spec->aiFlags;
    trainer->party = pool;
    trainer->partySize = partySize;
    trainer->poolSize = poolSize;
    trainer->poolRuleIndex = POOL_RULESET_INFCAVE;
    trainer->poolPickIndex = POOL_PICK_INFCAVE;
    trainer->poolPruneIndex = POOL_PRUNE_INFCAVE;
    trainer->overrideTrainer = TRAINER_NONE;
    sInfCaveTrainerTier[slot] = tier;
    InfCave_ApplyModifiersToTrainer(trainer);

    // Narrow bitfields and table indices: catch truncation here, not at battle setup.
    assertf(trainer->partySize == partySize, "partySize %d truncated to %d", partySize, trainer->partySize);
    assertf(trainer->encounterMusic == identity->encounterMusic, "encounterMusic %d truncated to %d", identity->encounterMusic, trainer->encounterMusic);
    assertf(trainer->trainerPic < TRAINER_PIC_COUNT, "trainerPic %d out of range", trainer->trainerPic);
    assertf(StringLength(identity->name) <= TRAINER_NAME_LENGTH, "identity name longer than %d", TRAINER_NAME_LENGTH);
    assertf(identity->objectGfxId < NUM_OBJ_EVENT_GFX, "objectGfxId %d out of range", identity->objectGfxId);
    assertf(pool != NULL && poolSize >= partySize, "tier %d pool too small: %d", tier, poolSize);

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
