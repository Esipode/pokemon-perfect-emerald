#include "global.h"
#include "battle_encounter.h"

// firedTriggers is a u32 bitmap; one bit per trigger.
STATIC_ASSERT(MAX_ENCOUNTER_TRIGGERS == 32, EncounterFiredTriggersBitmapMismatch);
STATIC_ASSERT(sizeof(struct EncounterRuntime) <= 64, EncounterRuntimeTooLarge);

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
