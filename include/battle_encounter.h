#ifndef GUARD_BATTLE_ENCOUNTER_H
#define GUARD_BATTLE_ENCOUNTER_H

#include "battle.h"
#include "constants/battle_encounter.h"

// Defined in Stage 11; a pointer to an incomplete type is enough for now.
struct EncounterCondition;

struct EncounterTrigger
{
    enum EncounterCheckpoint checkpoint;
    u8 priority;                        // lower runs first; 0 = highest
    u8 flags;                           // ENC_TRIGGER_*
    const struct EncounterCondition *conditions;   // NULL = always eligible (Stage 11)
    const u8 *script;                   // battle script label
};

struct Encounter
{
    const struct EncounterTrigger *triggers;
    u8 triggerCount;
};

// Test encounter script (data/battle_scripts_encounters.s).
extern const u8 EncScript_TestBattleStart[];

const struct Encounter *GetEncounter(enum EncounterId id);

// Set by the overworld script that starts the battle; consumed once at battle start.
void SetPendingBattleEncounter(enum EncounterId id);
enum EncounterId TakePendingBattleEncounter(void);

static inline bool32 IsEncounterActive(void)
{
#if B_ENCOUNTER_SCRIPTING
    return gBattleStruct->encounter.id != ENCOUNTER_NONE;
#else
    return FALSE;
#endif
}

#endif // GUARD_BATTLE_ENCOUNTER_H
