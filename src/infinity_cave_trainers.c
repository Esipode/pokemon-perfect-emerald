#include "global.h"
#include "infinity_cave.h"
#include "data.h"
#include "event_data.h"
#include "string_util.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/trainers.h"
#include "constants/battle_ai.h"
#include "trainer_pools.h"

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

// Debug harness for the redirect, ahead of the Stage 13 roll: a fixed identity
// and party in slot 0. Debug_EventScript_InfCaveTestBattle fights it.
static const struct TrainerMon sInfCaveDebugParty[] =
{
    { .species = SPECIES_GRAVELER, .lvl = 40, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER },
    { .species = SPECIES_SANDSLASH, .lvl = 40, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER },
};

void InfCave_DebugFillTrainer(void)
{
    struct Trainer *trainer = InfCave_GetTrainerSlot(0);

    memset(trainer, 0, sizeof(*trainer));
    StringCopy(trainer->trainerName, COMPOUND_STRING("CAVER"));
    trainer->trainerClass = TRAINER_CLASS_HIKER;
    trainer->trainerPic = TRAINER_PIC_HIKER;
    trainer->encounterMusic = TRAINER_ENCOUNTER_MUSIC_HIKER;
    trainer->gender = TRAINER_GENDER_MALE;
    trainer->battleType = TRAINER_BATTLE_TYPE_SINGLES;
    trainer->aiFlags = AI_FLAG_SMART_TRAINER;
    trainer->party = sInfCaveDebugParty;
    trainer->partySize = ARRAY_COUNT(sInfCaveDebugParty);
    trainer->poolSize = 0;
    trainer->poolRuleIndex = POOL_RULESET_BASIC;
    trainer->poolPickIndex = POOL_PICK_DEFAULT;
    trainer->poolPruneIndex = POOL_PRUNE_NONE;
    trainer->overrideTrainer = TRAINER_NONE;

    InfCave_ArmTrainers();
}
