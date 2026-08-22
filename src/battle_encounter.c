#include "global.h"
#include "battle_encounter.h"
#include "data/battle_encounters.h"

// firedTriggers is a u32 bitmap; one bit per trigger.
STATIC_ASSERT(MAX_ENCOUNTER_TRIGGERS == 32, EncounterFiredTriggersBitmapMismatch);
STATIC_ASSERT(sizeof(struct EncounterRuntime) <= 64, EncounterRuntimeTooLarge);

#if TESTING
static const struct Encounter *sTestEncounter;

void TestSetEncounter(const struct Encounter *encounter)
{
    sTestEncounter = encounter;
}
#endif

const struct Encounter *GetEncounter(enum EncounterId id)
{
#if TESTING
    if (sTestEncounter != NULL)
        return sTestEncounter;
#endif
    assertf(id > ENCOUNTER_NONE && id < ENCOUNTER_COUNT, "invalid encounter id %d", id)
    {
        return NULL;
    }
    assertf(gEncounters[id].triggerCount <= MAX_ENCOUNTER_TRIGGERS, "encounter %d has too many triggers", id)
    {
        return NULL;
    }
    return &gEncounters[id];
}

// Selects the highest-priority (lowest value) eligible trigger for checkpoint, marks it
// fired if it's ENC_TRIGGER_ONCE, and returns its script. Ties break by table order.
const u8 *TryRunEncounterCheckpoint(enum EncounterCheckpoint checkpoint)
{
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    const struct EncounterTrigger *best = NULL;
    u32 bestIndex = 0;

    if (!IsEncounterActive())
        return NULL;

    const struct Encounter *encounter = GetEncounter(runtime->id);
    if (encounter == NULL)
        return NULL;

    for (u32 i = 0; i < encounter->triggerCount; i++)
    {
        const struct EncounterTrigger *trigger = &encounter->triggers[i];
        bool32 invalid = FALSE;

        assertf(trigger->checkpoint < ENC_CHECKPOINT_COUNT, "encounter %d trigger %d: invalid checkpoint %d", runtime->id, i, trigger->checkpoint)
        {
            invalid = TRUE;
        }
        assertf(trigger->script != NULL, "encounter %d trigger %d: NULL script", runtime->id, i)
        {
            invalid = TRUE;
        }
        if (invalid)
            continue;

        if (trigger->checkpoint != checkpoint)
            continue;
        if (runtime->firedTriggers & (1u << i))
            continue;
        // conditions always pass here; Stage 11 adds real evaluation.

        if (best == NULL || trigger->priority < best->priority)
        {
            best = trigger;
            bestIndex = i;
        }
    }

    if (best == NULL)
        return NULL;

    if (best->flags & ENC_TRIGGER_ONCE)
        runtime->firedTriggers |= (1u << bestIndex);

    runtime->checkpoint = checkpoint;
    return best->script;
}

// Set by the overworld script that starts the battle, taken exactly once by battle start.
EWRAM_DATA static enum EncounterId sPendingEncounter = ENCOUNTER_NONE;

void SetPendingBattleEncounter(enum EncounterId id)
{
    sPendingEncounter = id;
}

enum EncounterId TakePendingBattleEncounter(void)
{
    enum EncounterId id = sPendingEncounter;
    sPendingEncounter = ENCOUNTER_NONE;
    return id;
}
