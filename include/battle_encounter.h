#ifndef GUARD_BATTLE_ENCOUNTER_H
#define GUARD_BATTLE_ENCOUNTER_H

#include "battle.h"
#include "constants/battle_encounter.h"

// 6 bytes, ROM-resident. A trigger's conditions pointer addresses an array terminated by an
// ENC_OP_COUNT operand, avoiding a separate count.
struct EncounterCondition
{
    u8  operand;     // enum EncounterOperand
    u8  cmp;         // enum EncounterCmp
    u16 arg;         // operand-specific: battler ref, var index, ENC_PACK_STAT_ARG
    s16 value;       // right-hand side
};

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
extern const u8 EncScript_TestGeneric[];

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
// 7. runtime->prevHp is captured before ENC_ON_BATTLE_START's first pass and re-captured once a
//    checkpoint has no more eligible triggers - the "previous" state ENC_TRIGGER_ON_ENTER compares
//    against is always "as of the last checkpoint", never mid-checkpoint.
const u8 *TryRunEncounterCheckpoint(enum EncounterCheckpoint checkpoint);

// Populates the current checkpoint's event context. Call sites only need to pass the fields their
// checkpoint actually has data for (see sCheckpointEventFields in battle_encounter.c) - unset
// fields are left zeroed and reading them via GetEncounterEventField asserts.
//
// Call this AFTER TryRunEncounterCheckpoint, not before: TryRunEncounterCheckpoint clears the
// event context on checkpoint entry (the first pass of a checkpoint only), so calling this first
// would have that clear wipe it straight back out on pass 1.
void SetEncounterEvent(u8 battler, u8 target, u16 move, enum EncounterEventCause cause, s16 oldValue, s16 newValue);

// Reads one field of the current checkpoint's event context. Asserts (recovery: return FALSE) if
// field is not valid for the checkpoint currently dispatching, e.g. reading ENC_EVENT_MOVE at
// ENC_ON_BATTLE_START - that would otherwise silently return a stale value from an earlier
// checkpoint. field is one of the ENC_EVENT_* constants in constants/battle_encounter.h.
bool32 GetEncounterEventField(u32 field, s32 *out);

// Resolves a battler reference (enum EncounterBattlerRef) to a concrete battler id. Shared with
// Stage 15's command targeting so both use one vocabulary. Returns FALSE for a _RIGHT ref in a
// singles battle (recovery: *battlerOut left untouched) rather than resolving to an inactive
// battler's stale gBattleMons entry - never read that.
bool32 ResolveEncounterBattlerRef(u32 ref, u8 *battlerOut);

// Reads one operand of live battle state or event context, for comparison against a condition's
// value. useSnapshot (Stage 10) redirects ENC_OP_HP / ENC_OP_HP_PERCENT to runtime->prevHp instead
// of live HP; every other operand ignores it.
s32 GetEncounterOperand(enum EncounterOperand operand, u32 arg, bool32 useSnapshot);

// Walks conds - an array terminated by an ENC_OP_COUNT operand - ANDing every comparison and
// short-circuiting on the first failure. NULL is vacuously TRUE: a trigger with no conditions is
// always eligible.
bool32 EvaluateConditions(const struct EncounterCondition *conds, bool32 useSnapshot);

static inline bool32 IsEncounterActive(void)
{
#if B_ENCOUNTER_SCRIPTING
    return gBattleStruct->encounter.id != ENCOUNTER_NONE;
#else
    return FALSE;
#endif
}

#endif // GUARD_BATTLE_ENCOUNTER_H
