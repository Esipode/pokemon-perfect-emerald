#ifndef GUARD_CONSTANTS_BATTLE_ENCOUNTER_H
#define GUARD_CONSTANTS_BATTLE_ENCOUNTER_H

// ENCOUNTER_NONE must be 0 so a zeroed gBattleStruct means "no encounter".
enum EncounterId
{
    ENCOUNTER_NONE,
    ENCOUNTER_TEST,
    ENCOUNTER_COUNT,
};

#define MAX_ENCOUNTER_VARS                     16
#define MAX_ENCOUNTER_TRIGGERS                 32
#define MAX_ENCOUNTER_COND_DEPTH                4
#define MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT    4

// Points in battle logic where triggers can fire. ENC_ON_DAMAGE is deliberately
// absent: HP changes are recorded at the damage commit point and dispatched at
// ENC_ON_MOVE_END / ENC_ON_TURN_END instead.
enum EncounterCheckpoint
{
    ENC_ON_BATTLE_START,
    ENC_ON_TURN_START,
    ENC_ON_MOVE_END,
    ENC_ON_FAINT,
    ENC_ON_SWITCH_IN,
    ENC_ON_TURN_END,
    ENC_ON_BATTLE_END,
    ENC_CHECKPOINT_COUNT,
};

#define ENC_TRIGGER_ONCE   (1 << 0)   // disable this trigger after it executes

#endif // GUARD_CONSTANTS_BATTLE_ENCOUNTER_H
