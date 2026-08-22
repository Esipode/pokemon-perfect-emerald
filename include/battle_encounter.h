#ifndef GUARD_BATTLE_ENCOUNTER_H
#define GUARD_BATTLE_ENCOUNTER_H

#include "battle.h"
#include "constants/battle_encounter.h"

// Set by the overworld script that starts the battle; consumed once at battle start.
void SetPendingBattleEncounter(enum EncounterId id);
enum EncounterId TakePendingBattleEncounter(void);

static inline bool32 IsEncounterActive(void)
{
#if B_ENCOUNTER_SCRIPTING
    return gBattleStruct->encounterId != ENCOUNTER_NONE;
#else
    return FALSE;
#endif
}

#endif // GUARD_BATTLE_ENCOUNTER_H
