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

// What caused the event context (struct EncounterEvent) to be populated.
enum EncounterEventCause
{
    ENC_CAUSE_NONE,
    ENC_CAUSE_MOVE_DAMAGE,
    ENC_CAUSE_RECOIL,
    ENC_CAUSE_DRAIN,
    ENC_CAUSE_END_TURN,          // weather, status, Leftovers
    ENC_CAUSE_ITEM,
    ENC_CAUSE_ABILITY,
    ENC_CAUSE_ENCOUNTER_SCRIPT,   // reserved; encounter scripts don't raise events yet
};

// Per-checkpoint validity bits for struct EncounterEvent (sCheckpointEventFields). A checkpoint
// that doesn't set a bit didn't populate the matching field(s) this dispatch.
#define ENC_EVENT_BATTLER   (1 << 0)
#define ENC_EVENT_TARGET    (1 << 1)
#define ENC_EVENT_MOVE      (1 << 2)
#define ENC_EVENT_CAUSE     (1 << 3)
#define ENC_EVENT_VALUES    (1 << 4)   // covers both oldValue and newValue

// Field selectors for GetEncounterEventField. The battler/target/move/cause selectors double as
// their own validity bit above; the value selectors both check ENC_EVENT_VALUES.
#define ENC_EVENT_OLD_VALUE (1 << 5)
#define ENC_EVENT_NEW_VALUE (1 << 6)

#endif // GUARD_CONSTANTS_BATTLE_ENCOUNTER_H
