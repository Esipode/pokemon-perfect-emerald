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

// Test encounter scripts (data/battle_scripts_encounters.s).
extern const u8 EncScript_TestBattleStart[];
extern const u8 EncScript_TestTurnEnd[];

// Engine-owned shim: all encounter scripts end with `return`; checkpoints dispatched
// from a non-script engine callback (e.g. BATTLE_START) call the encounter script from
// this shim so the callback stack still unwinds via `end2`.
extern const u8 BattleScript_EncounterCheckpointEnd2[];

const struct Encounter *GetEncounter(enum EncounterId id);

// Set by the overworld script that starts the battle; consumed once at battle start.
void SetPendingBattleEncounter(enum EncounterId id);
enum EncounterId TakePendingBattleEncounter(void);

#if TESTING
// Overrides GetEncounter's id-indexed lookup so tests can supply their own trigger
// tables without touching real encounter data. NULL restores normal lookup.
void TestSetEncounter(const struct Encounter *encounter);
#endif

// priority 0 = highest, runs first; priority 255 = lowest, runs last.
// Selects the highest-priority eligible trigger for checkpoint and returns its script,
// or NULL if none is eligible. Does not run the script.
//
// Ordering guarantees (call sites loop on this until it returns NULL):
// 1. At a checkpoint, the eligible trigger with the lowest priority runs first.
// 2. Ties break by trigger table order, lowest index first.
// 3. After a script completes, all triggers are re-evaluated against the new state.
// 4. A trigger that lost on priority stays eligible and can run in a later pass.
// 5. ENC_TRIGGER_ONCE triggers are marked fired on selection, so they cannot run twice
//    even within one checkpoint.
// 6. At most MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT scripts run per checkpoint.
const u8 *TryRunEncounterCheckpoint(enum EncounterCheckpoint checkpoint);

static inline bool32 IsEncounterActive(void)
{
#if B_ENCOUNTER_SCRIPTING
    return gBattleStruct->encounter.id != ENCOUNTER_NONE;
#else
    return FALSE;
#endif
}

#endif // GUARD_BATTLE_ENCOUNTER_H
