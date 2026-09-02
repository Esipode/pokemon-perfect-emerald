#ifndef GUARD_CONSTANTS_BATTLE_ENCOUNTER_H
#define GUARD_CONSTANTS_BATTLE_ENCOUNTER_H

// ENCOUNTER_NONE must be 0 so a zeroed gBattleStruct means "no encounter".
enum EncounterId
{
    ENCOUNTER_NONE,
    ENCOUNTER_TEST,
    ENCOUNTER_LEGENDARY_BARRIER,   // Stage 15 example, outline Sec33
    ENCOUNTER_TRAINER_MEGA,        // Stage 15 example, outline Sec34
    ENCOUNTER_STORM_HERALD,        // Stage 16 example: three chained phase transitions
    ENCOUNTER_ARTICUNO,
    ENCOUNTER_ZAPDOS,
    ENCOUNTER_MOLTRES,
    ENCOUNTER_MEWTWO,
    ENCOUNTER_MEW,
    ENCOUNTER_RAIKOU,
    ENCOUNTER_ENTEI,
    ENCOUNTER_SUICUNE,
    ENCOUNTER_CELEBI,
    ENCOUNTER_LUGIA,
    ENCOUNTER_HO_OH,
    ENCOUNTER_DEOXYS,
    ENCOUNTER_JIRACHI,
    ENCOUNTER_RAYQUAZA,
    ENCOUNTER_KYOGRE,
    ENCOUNTER_GROUDON,
    ENCOUNTER_REGICE,
    ENCOUNTER_REGIROCK,
    ENCOUNTER_COUNT,
};

#define MAX_ENCOUNTER_VARS                     16
#define MAX_ENCOUNTER_TRIGGERS                 32
#define MAX_ENCOUNTER_COND_DEPTH                4
#define MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT    8

// gEncounterVars (src/battle_encounter.c) is a fixed EWRAM array, not a member of gBattleStruct -
// gBattleStruct is a heap pointer resolved only at runtime, so a battle script (which can only
// encode a link-time constant address) couldn't address into it directly. This gives battle
// scripts the same addressing convention gBattleScripting's s* macros use (sMOVEEND_STATE etc.,
// battle_script_commands.h): `setbyte sENCOUNTER_VAR(n), value` assembles like any other bytePtr,
// for a script that writes the opcode directly. n is a raw index (0..MAX_ENCOUNTER_VARS-1).
// asm/macros/battle_script.inc's encsetvar/encaddvar/encjumpifvar cover the common case instead -
// they expand to `gEncounterVars + n` directly rather than this macro, since a .inc file (pulled
// in via .include, not #include) is never seen by the C preprocessor that would expand this one.
// GetEncounterOperand's ENC_OP_VAR reads the same array for conditions.
#define sENCOUNTER_VAR(n) (gEncounterVars + (n))

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

// Battler-set targets for commands (Stage 15). Resolved through ResolveEncounterTarget
// (battle_encounter.h) to a bitmask rather than a single battler id - the only representation that
// treats ALL_FOES/ALL_ALLIES/ALL_BATTLERS and a single slot uniformly, so a command's "for each
// targeted battler" logic is identical in singles and doubles. Every single-slot entry resolves
// through the same lookup as EncounterBattlerRef above, so an author who knows one vocabulary knows
// both; the entries that don't appear there (the three group targets, plus EVENT_TARGET) exist only
// because a command needs a *set of battlers*, which a condition never does.
//
// Format validity is the point of this abstraction: a single-slot entry invalid for the current
// battle format (e.g. ENC_TARGET_PLAYER_RIGHT in singles) resolves to an empty mask rather than an
// inactive battler's stale gBattleMons entry - see ResolveEncounterTarget. A state-changing command
// must still assert if the mask it gets back contains a fainted/absent battler; that check belongs
// to the command, not to resolution.
//
// No `actor` field exists alongside this (outline Sec30). Presentation commands (trainerslidein,
// printstring, playse, ...) already act on the trainer or the screen and take no battler; mechanic
// commands take an EncounterTarget. That split is already structural in the existing opcode set, so
// adding an actor field would be speculative generality against a distinction the engine already
// encodes.
enum EncounterTarget
{
    ENC_TARGET_BOSS,
    ENC_TARGET_SELF,           // ENC_SELF - the battler that raised the current event
    ENC_TARGET_EVENT_TARGET,   // the other side of the event: who it happened to, not who did it
    ENC_TARGET_PLAYER_LEFT,
    ENC_TARGET_PLAYER_RIGHT,
    ENC_TARGET_OPPONENT_LEFT,
    ENC_TARGET_OPPONENT_RIGHT,
    ENC_TARGET_ALL_FOES,       // every battler not on the boss's side
    ENC_TARGET_ALL_ALLIES,     // every battler on the boss's side, boss included
    ENC_TARGET_ALL_BATTLERS,
    ENC_TARGET_COUNT,
};

// Side-wide statuses encsetsidestatus / encclearsidestatus can raise or drop. A small enum rather
// than a raw SIDE_STATUS_* bitmask because SIDE_STATUS_RAINBOW is 1 << 8 and a script argument is a
// byte - and because an enum lets the command assert on a name it doesn't know. Each entry pairs a
// status bit with the gSideTimers field that ticks it down; a 0 timer never ticks, so 0 turns means
// permanent, the same convention encsetweather uses.
enum EncounterSideStatus
{
    ENC_SIDE_REFLECT,
    ENC_SIDE_LIGHT_SCREEN,
    ENC_SIDE_AURORA_VEIL,
    ENC_SIDE_SAFEGUARD,
    ENC_SIDE_MIST,
    ENC_SIDE_TAILWIND,
    ENC_SIDE_LUCKY_CHANT,
    ENC_SIDE_RAINBOW,
    ENC_SIDE_SEA_OF_FIRE,
    ENC_SIDE_SWAMP,
    ENC_SIDE_COUNT,
};

// --- Encounter properties (battle-start configuration) ------------------------------------------
// Authored in a '.encounter' file's 'Properties:' block and emitted into struct EncounterProperties
// (include/battle_encounter.h). These configure the battle itself rather than react to it, so they
// are applied once at battle start instead of by a trigger's script; the matching enc* commands
// (asm/macros/battle_script.inc) change the same state mid-battle where that makes sense.

#define ENC_LEVEL_NONE  0        // no level override; the opponents keep the level they were built with
#define ENC_LEVEL_CAP   0xFFFF   // set every opponent to GetProgressionLevelCap()

#define ENC_CATCH_RATE_NONE 0    // no override; the species' own catch rate applies

// No ability override; the boss keeps the ability it was built with. Equal to ABILITY_NONE, which
// is never a real ability, so the property doubles as its own "unset" marker.
#define ENC_ABILITY_NONE 0

// Whether the player may throw a Poke Ball. ENC_BALLS_ALLOWED only lifts an encounter's own block -
// it never overrides a rule the battle itself imposes (trainer battle, Ghost without a Silph Scope,
// Nuzlocke, ...). The usual legendary pattern is to start BLOCKED and have the boss's final-phase
// script switch to ALLOWED once it's weakened enough to be worth catching.
enum EncounterBallPolicy
{
    ENC_BALLS_DEFAULT,   // whatever the battle would normally allow
    ENC_BALLS_BLOCKED,
    ENC_BALLS_ALLOWED,
};

// Move classes an encounter can make a battler immune to. Each one either ignores the damage
// formula (and so ignores an encounter's damage reduction) or ends a battler regardless of its HP -
// the two ways a player can otherwise skip straight past a scripted boss fight.
#define ENC_IMMUNE_OHKO         (1 << 0)  // EFFECT_OHKO: Sheer Cold, Fissure, Guillotine, Horn Drill
#define ENC_IMMUNE_FIXED_DAMAGE (1 << 1)  // damage that bypasses the damage formula: Super Fang, Night
                                          // Shade, Seismic Toss, Dragon Rage, Sonic Boom, Psywave,
                                          // Endeavor, Final Gambit, Counter/Mirror Coat/Metal Burst, Bide
#define ENC_IMMUNE_HP_SWAP      (1 << 2)  // Pain Split
#define ENC_IMMUNE_SHARED_KO    (1 << 3)  // Destiny Bond, Perish Song
#define ENC_IMMUNE_ALL          (ENC_IMMUNE_OHKO | ENC_IMMUNE_FIXED_DAMAGE | ENC_IMMUNE_HP_SWAP | ENC_IMMUNE_SHARED_KO)

// Damage reduction is a percentage: 70 means the battler takes 70% less damage from every source
// that runs through the damage formula or a passive HP tick. Capped below 100 deliberately - a
// battler nothing can damage isn't a fight, and a move class that must not work at all belongs in
// ENC_IMMUNE_* above.
#define ENC_MAX_DAMAGE_REDUCTION 99

// How an amount argument is read by the commands that take one (enchangehp, encchangestatvalue).
// PERCENT is relative to the battler's max HP / current stat value, which is what a scripted
// encounter usually wants: the level the boss ends up at isn't known when the script is written
// (level caps, New Game Plus offsets), so a fixed HP number can't be balanced against it.
enum EncounterAmountMode
{
    ENC_AMOUNT_FIXED,
    ENC_AMOUNT_PERCENT,
    ENC_AMOUNT_TO_PERCENT,   // move HP *to* this percent of max, healing or damaging as needed
};

// How encsnapshothp writes a battler's HP percentage into an author variable. LOWEST is what makes
// a stored percentage a monotone floor without a var-to-var comparison (conditions and encjumpifvar
// both compare a var against a literal); RECOVERY answers "how much has this battler healed since
// the mark?" against a literal for the same reason.
enum EncounterSnapshotMode
{
    ENC_SNAP_SET,        // write the current percentage, overwriting
    ENC_SNAP_LOWEST,     // write only if the current percentage is lower than what the var holds
    ENC_SNAP_RECOVERY,   // write max(0, currentPct - var): recovery since the mark
    ENC_SNAP_DAMAGE,     // write max(0, var - currentPct): damage taken since the mark
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
    ENC_OP_VAR,             // arg = index into gEncounterVars (Stage 14)

    // --- event context (Stage 08) ---
    ENC_OP_EVENT_BATTLER,
    ENC_OP_EVENT_TARGET,
    ENC_OP_EVENT_MOVE,
    ENC_OP_EVENT_MOVE_TYPE,  // the event move's base type, so a rule can key off "any Ice move"
    ENC_OP_EVENT_MOVE_CATEGORY,  // the event move's base category, for a rule keyed off how the player attacks
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
