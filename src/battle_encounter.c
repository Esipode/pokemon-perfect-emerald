#include "global.h"
#include "battle_encounter.h"
#include "data/battle_encounters.h"

// firedTriggers is a u32 bitmap; one bit per trigger.
STATIC_ASSERT(MAX_ENCOUNTER_TRIGGERS == 32, EncounterFiredTriggersBitmapMismatch);
STATIC_ASSERT(sizeof(struct EncounterRuntime) <= 64, EncounterRuntimeTooLarge);

const struct Encounter *GetEncounter(enum EncounterId id)
{
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
