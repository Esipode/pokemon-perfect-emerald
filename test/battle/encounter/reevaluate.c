#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

// These triggers are all unconditional (.conditions = NULL); ENC_TRIGGER_ONCE stands in for a real
// condition turning itself off, which is what lets a lower-priority trigger become the winner on a
// later pass instead of the same top-priority trigger being re-selected forever.

static const struct EncounterTrigger sTriggers_ChainAtBattleStart[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestBattleStart },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestBattleStart },
};
static const struct Encounter sEncounter_ChainAtBattleStart = { sTriggers_ChainAtBattleStart, ARRAY_COUNT(sTriggers_ChainAtBattleStart) };

SINGLE_BATTLE_TEST("ENC_ON_BATTLE_START re-evaluates after a script completes and runs the next eligible trigger")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_ChainAtBattleStart);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // priority 0 trigger, pass 1
        MESSAGE(""); // priority 10 trigger, pass 2 -- only eligible once the winner fired
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}

static const struct EncounterTrigger sTriggers_ThreeAtTurnEnd[] =
{
    { .checkpoint = ENC_ON_TURN_END, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestTurnEnd },
    { .checkpoint = ENC_ON_TURN_END, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestTurnEnd },
    { .checkpoint = ENC_ON_TURN_END, .priority = 20, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestTurnEnd },
};
static const struct Encounter sEncounter_ThreeAtTurnEnd = { sTriggers_ThreeAtTurnEnd, ARRAY_COUNT(sTriggers_ThreeAtTurnEnd) };

SINGLE_BATTLE_TEST("ENC_ON_TURN_END runs every eligible trigger across passes, not just the first winner")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_ThreeAtTurnEnd);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // priority 0, pass 1
        MESSAGE(""); // priority 10, pass 2 -- the priority-0 loser from pass 1 is not lost, it just already ran
        MESSAGE(""); // priority 20, pass 3
    }
}

static const struct EncounterTrigger sTriggers_OnceAcrossPasses[] =
{
    { .checkpoint = ENC_ON_TURN_END, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestTurnEnd },
    { .checkpoint = ENC_ON_TURN_END, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestTurnEnd },
};
static const struct Encounter sEncounter_OnceAcrossPasses = { sTriggers_OnceAcrossPasses, ARRAY_COUNT(sTriggers_OnceAcrossPasses) };

SINGLE_BATTLE_TEST("ENC_TRIGGER_ONCE triggers fired within one checkpoint stay fired on a later checkpoint entry")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_OnceAcrossPasses);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // priority 0 trigger, turn 1 pass 1
        MESSAGE(""); // priority 10 trigger, turn 1 pass 2
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        NOT MESSAGE(""); // both already fired; turn 2's end-turn checkpoint has nothing eligible
    }
}

// Test 4 -- "Runaway bounded": five always-eligible non-ONCE triggers at one
// checkpoint. TryRunEncounterCheckpoint's assertf trips on the 5th call (TEST_RESULT_INVALID);
// the suite has no mechanism to mark a test as expecting INVALID (only fatal_assertf/CRASH has
// one, via Test_ExpectCrash), so this can't live in the suite as a normal TEST() without turning
// `make check` red forever. Enable the block below, run this file filtered to this test name,
// and confirm: exactly 4 "" messages appear, the run ends INVALID with the runaway assertion
// message (not a hang/timeout), then disable it again.
#if 0
static const struct EncounterTrigger sTriggers_Runaway[] =
{
    { .checkpoint = ENC_ON_TURN_END, .priority = 0, .flags = 0, .conditions = NULL, .script = EncScript_TestTurnEnd },
    { .checkpoint = ENC_ON_TURN_END, .priority = 1, .flags = 0, .conditions = NULL, .script = EncScript_TestTurnEnd },
    { .checkpoint = ENC_ON_TURN_END, .priority = 2, .flags = 0, .conditions = NULL, .script = EncScript_TestTurnEnd },
    { .checkpoint = ENC_ON_TURN_END, .priority = 3, .flags = 0, .conditions = NULL, .script = EncScript_TestTurnEnd },
    { .checkpoint = ENC_ON_TURN_END, .priority = 4, .flags = 0, .conditions = NULL, .script = EncScript_TestTurnEnd },
};
static const struct Encounter sEncounter_Runaway = { sTriggers_Runaway, ARRAY_COUNT(sTriggers_Runaway) };

SINGLE_BATTLE_TEST("ENC_ON_TURN_END stops dispatching after MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT scripts (manual only, see comment above)")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_Runaway);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE("");
        MESSAGE("");
        MESSAGE("");
        MESSAGE("");
        // assertf trips here; the test ends INVALID instead of reaching a 5th MESSAGE("").
    }
}
#endif
