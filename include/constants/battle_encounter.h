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

#define ENC_TRIGGER_ONCE       (1 << 0)   // disable this trigger after it executes
#define ENC_TRIGGER_ON_ENTER   (1 << 1)   // fire only on the transition FALSE -> TRUE

// ONCE and ON_ENTER are independent and combine meaningfully:
//
//   Flags          | Behavior
//   ---------------+----------------------------------------------------------
//   neither        | fires at every checkpoint where conditions hold (level)
//   ONCE           | fires the first time conditions hold, then never
//   ON_ENTER       | fires each time conditions become true after being false
//   ONCE|ON_ENTER  | fires on the first crossing only - the phase-transition default
//
// A condition like hp_percent <= 50 is a level - it stays true once crossed. ON_ENTER
// turns it into an edge - the moment it became true - which is what a one-shot phase
// transition actually wants; ONCE alone would also fire at the first checkpoint where
// the level already happened to be true (e.g. a boss that starts the battle below the
// threshold), which is usually not intended.

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

// Battler references, shared with Stage 15's command targeting so authors learn one vocabulary.
// Resolved through ResolveEncounterBattlerRef (battle_encounter.h); the _RIGHT refs are invalid in
// a singles battle and resolution fails safely rather than reading an inactive battler's stale
// gBattleMons entry.
enum EncounterBattlerRef
{
    ENC_BOSS,            // the encounter's subject; opponent slot 0
    ENC_SELF,            // the battler that raised the event
    ENC_PLAYER_LEFT,
    ENC_PLAYER_RIGHT,
    ENC_OPPONENT_LEFT,
    ENC_OPPONENT_RIGHT,
    ENC_BATTLER_REF_COUNT,
};

// ENC_OP_STAT_STAGE's arg packs a battler ref and an enum Stat into one u16 - both are small enough
// to share it.
#define ENC_PACK_STAT_ARG(battlerRef, stat) ((battlerRef) | ((stat) << 3))
#define ENC_UNPACK_STAT_BATTLER(arg)        ((arg) & 0x7)
#define ENC_UNPACK_STAT_ID(arg)             ((arg) >> 3)

// What GetEncounterOperand reads. Live battle state sources straight from gBattleMons/field state;
// event context sources from the current checkpoint's struct EncounterEvent (Stage 08) via
// GetEncounterEventField, so its validity mask applies automatically - reading an event operand at
// a checkpoint that doesn't populate it asserts.
//
// The three group operands (Stage 12) are placed first so `operand < ENC_OP_FIRST_LEAF` is a cheap
// "is this a group node, not a comparison" test. A group node reuses struct EncounterCondition's
// arg field as a child count instead of an operand argument; see EvalNode in battle_encounter.c.
enum EncounterOperand
{
    ENC_OP_ALL,    // arg = number of immediate child nodes; true if all are true
    ENC_OP_ANY,    // arg = number of immediate child nodes; true if any is true
    ENC_OP_NOT,    // arg unused; inverts the single node that follows

    ENC_OP_FIRST_LEAF,

    // --- live battle state ---
    ENC_OP_HP = ENC_OP_FIRST_LEAF,   // arg = battler ref
    ENC_OP_HP_PERCENT,      // arg = battler ref
    ENC_OP_MAX_HP,          // arg = battler ref
    ENC_OP_SPECIES,         // arg = battler ref
    ENC_OP_ABILITY,         // arg = battler ref
    ENC_OP_STATUS,          // arg = battler ref
    ENC_OP_STAT_STAGE,      // arg packs battler + enum Stat, see ENC_PACK_STAT_ARG
    ENC_OP_TYPE,            // arg = battler ref
    ENC_OP_WEATHER,         // no arg
    ENC_OP_TERRAIN,         // no arg
    ENC_OP_TURN,            // no arg
    ENC_OP_BATTLER_COUNT,   // no arg - for doubles-aware conditions
    ENC_OP_VAR,             // arg = index into runtime->vars

    // --- event context (Stage 08) ---
    ENC_OP_EVENT_BATTLER,
    ENC_OP_EVENT_TARGET,
    ENC_OP_EVENT_MOVE,
    ENC_OP_EVENT_CAUSE,
    ENC_OP_EVENT_OLD_VALUE,
    ENC_OP_EVENT_NEW_VALUE,

    ENC_OP_COUNT,
};

// Deliberately not CMP_EQUAL/CMP_NOT_EQUAL/etc. (Cmd_jumpifbyte, battle_script_commands.c): that
// vocabulary has no LE/GE and adds bitwise comparisons conditions don't need.
enum EncounterCmp
{
    ENC_CMP_EQ,
    ENC_CMP_NE,
    ENC_CMP_LT,
    ENC_CMP_LE,
    ENC_CMP_GT,
    ENC_CMP_GE,
};

#endif // GUARD_CONSTANTS_BATTLE_ENCOUNTER_H
