#include "global.h"
#include "battle_encounter.h"

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
