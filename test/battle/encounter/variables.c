#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

// Stage 14: the sENCOUNTER_VAR addressing convention - encsetvar/encaddvar/encjumpifvar (asm/
// macros/battle_script.inc) write and branch on gEncounterVars (src/battle_encounter.c) through
// real battle scripts (data/battle_scripts_encounters.s), and ENC_OP_VAR conditions read the same
// array. These run real turns so the scripts actually execute through the interpreter, not just
// the C-level dispatch tested elsewhere in this directory.

// --- Test 1: a script's write is visible to a later trigger's condition (the key test) --------

static const struct EncounterCondition sConditions_Var0Set[] =
{
    { .operand = ENC_OP_VAR, .cmp = ENC_CMP_EQ, .arg = 0, .value = 1 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_SetThenRead[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestSetVar },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_Var0Set, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_SetThenRead = { sTriggers_SetThenRead, ARRAY_COUNT(sTriggers_SetThenRead) };

SINGLE_BATTLE_TEST("EncScript_TestSetVar's encsetvar write is visible to a later trigger's ENC_OP_VAR condition")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_SetThenRead);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // priority-10 trigger, pass 2 -- only eligible once var 0 == 1
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}

// --- Test 2: encaddvar accumulates across turns -------------------------------------------------

static const struct EncounterCondition sConditions_Var0AtLeast2[] =
{
    { .operand = ENC_OP_VAR, .cmp = ENC_CMP_GE, .arg = 0, .value = 2 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_Accumulate[] =
{
    { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_Var0AtLeast2, .script = EncScript_TestGeneric },
    { .checkpoint = ENC_ON_TURN_END,   .priority = 0, .flags = 0,                .conditions = NULL, .script = EncScript_TestAddVar },
};
static const struct Encounter sEncounter_Accumulate = { sTriggers_Accumulate, ARRAY_COUNT(sTriggers_Accumulate) };

SINGLE_BATTLE_TEST("encaddvar accumulates one per turn until a threshold condition fires")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_Accumulate);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        // Turn 1: var 0 == 0 at TURN_START, below the threshold.
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        // Turn 1 TURN_END: encaddvar fires silently, var 0 goes 0 -> 1.
        // Turn 2: var 0 == 1 at TURN_START, still below the threshold.
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        // Turn 2 TURN_END: var 0 goes 1 -> 2.
        // Turn 3: var 0 == 2 at TURN_START -- threshold crossed.
        MESSAGE("");
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}

// --- Test 3: encjumpifvar branches correctly both ways ------------------------------------------

static const struct EncounterCondition sConditions_Var1WentHigh[] =
{
    { .operand = ENC_OP_VAR, .cmp = ENC_CMP_EQ, .arg = 1, .value = 1 },
    { .operand = ENC_OP_COUNT },
};

static const struct EncounterTrigger sTriggers_BranchHigh[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestSeedVarHigh },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 5,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestBranch },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_Var1WentHigh, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_BranchHigh = { sTriggers_BranchHigh, ARRAY_COUNT(sTriggers_BranchHigh) };

SINGLE_BATTLE_TEST("encjumpifvar takes the branch when the comparison holds (var 0 == 5 > 4)")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_BranchHigh);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // var 0 seeded to 5; the branch is taken and var 1 becomes 1
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}

static const struct EncounterTrigger sTriggers_BranchLow[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestSeedVarLow },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 5,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestBranch },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_Var1WentHigh, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_BranchLow = { sTriggers_BranchLow, ARRAY_COUNT(sTriggers_BranchLow) };

SINGLE_BATTLE_TEST("encjumpifvar falls through when the comparison doesn't hold (var 0 == 2, not > 4)")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_BranchLow);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        NOT MESSAGE(""); // var 1 never became 1, so the gated trigger never fires
    }
}

// --- Test 4: call into a shared sub-script and return -------------------------------------------

static const struct EncounterCondition sConditions_CallSubDone[] =
{
    { .operand = ENC_OP_VAR, .cmp = ENC_CMP_EQ, .arg = 0, .value = 1 }, // the subroutine ran
    { .operand = ENC_OP_VAR, .cmp = ENC_CMP_EQ, .arg = 1, .value = 1 }, // the caller resumed after `return`
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_CallSub[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestCallSub },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_CallSubDone, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_CallSub = { sTriggers_CallSub, ARRAY_COUNT(sTriggers_CallSub) };

SINGLE_BATTLE_TEST("call into a shared sub-script returns control to the caller after return")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_CallSub);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // fires only once both the subroutine's write and the caller's post-call write landed
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}

// --- Test 5: per-battle scoping -------------------------------------------------------------

// Relies on running after a test above that leaves var 0 and var 1 non-zero (EncScript_TestCallSub,
// just above) without an intervening full-process reset - the same way every SINGLE_BATTLE_TEST in
// this file shares one process. If AllocateBattleResources (battle_util2.c) stopped calling
// ResetEncounterVars, this is the test that would catch a boss's leftover phase leaking into the
// next, unrelated battle.
static const struct EncounterCondition sConditions_Var0Zero[] =
{
    { .operand = ENC_OP_VAR, .cmp = ENC_CMP_EQ, .arg = 0, .value = 0 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_VarsResetAtBattleStart[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_Var0Zero, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_VarsResetAtBattleStart = { sTriggers_VarsResetAtBattleStart, ARRAY_COUNT(sTriggers_VarsResetAtBattleStart) };

SINGLE_BATTLE_TEST("gEncounterVars reads zero at the start of a battle even after an earlier battle left it dirty")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_VarsResetAtBattleStart);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // would not fire here if the previous test's var 0 == 1 had leaked in
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}
