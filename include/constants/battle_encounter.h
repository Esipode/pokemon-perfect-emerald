#ifndef GUARD_CONSTANTS_BATTLE_ENCOUNTER_H
#define GUARD_CONSTANTS_BATTLE_ENCOUNTER_H

// ENCOUNTER_NONE must be 0 so a zeroed gBattleStruct means "no encounter".
enum EncounterId
{
    ENCOUNTER_NONE,
    ENCOUNTER_COUNT,
};

#define MAX_ENCOUNTER_VARS                     16
#define MAX_ENCOUNTER_TRIGGERS                 32
#define MAX_ENCOUNTER_COND_DEPTH                4
#define MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT    4

#endif // GUARD_CONSTANTS_BATTLE_ENCOUNTER_H
