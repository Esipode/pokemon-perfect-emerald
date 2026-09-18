#include "global.h"
#include "test/test.h"
#include "malloc.h"
#include "battle.h"
#include "battle_encounter.h"
#include "constants/moves.h"

// Content is irrelevant; only script identity (pointer equality) is checked.
static const u8 sScriptA[] = {0};

static struct BattleStruct *BeginEncounterTest(const struct Encounter *encounter)
{
    struct BattleStruct *battleStruct = AllocZeroed(sizeof(*battleStruct));
    gBattleStruct = battleStruct;
    gBattleStruct->encounter.id = ENCOUNTER_TEST;
    TestSetEncounter(encounter);

    // Minimal singles battle field, for conditions that resolve a battler ref.
    memset(gBattleMons, 0, sizeof(gBattleMons));
    gBattlersCount = 2;
    gBattlerPositions[B_BATTLER_0] = B_POSITION_PLAYER_LEFT;
    gBattlerPositions[B_BATTLER_1] = B_POSITION_OPPONENT_LEFT;

    return battleStruct;
}

static void EndEncounterTest(struct BattleStruct *battleStruct)
{
    TestSetEncounter(NULL);
    gBattleStruct = NULL;
    Free(battleStruct);
}

// --- Test 1: a battle-state condition on its own -------------------------------------------

static const struct EncounterCondition sConditions_LowHp[] =
{
    { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_LowHp[] =
{
    { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = 0, .conditions = sConditions_LowHp, .script = sScriptA },
};
static const struct Encounter sEncounter_LowHp = { sTriggers_LowHp, ARRAY_COUNT(sTriggers_LowHp) };

TEST("A battle-state condition fires only when the operand satisfies the comparison")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_LowHp);
    gBattleMons[B_BATTLER_1].maxHP = 100;

    gBattleMons[B_BATTLER_1].hp = 51;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == NULL); // above the threshold

    gBattleMons[B_BATTLER_1].hp = 50;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA); // at the threshold

    EndEncounterTest(battleStruct);
}

// --- Test 2: an event condition on its own --------------------------------------------------

static const struct EncounterCondition sConditions_Earthquake[] =
{
    { .operand = ENC_OP_EVENT_MOVE, .cmp = ENC_CMP_EQ, .arg = 0, .value = MOVE_EARTHQUAKE },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_Earthquake[] =
{
    { .checkpoint = ENC_ON_MOVE_END, .priority = 0, .flags = 0, .conditions = sConditions_Earthquake, .script = sScriptA },
};
static const struct Encounter sEncounter_Earthquake = { sTriggers_Earthquake, ARRAY_COUNT(sTriggers_Earthquake) };

TEST("An event condition fires only when the event field satisfies the comparison")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_Earthquake);

    // The real call sites populate the event before dispatching, so a condition sees it on the
    // very first pass of the checkpoint.
    SetEncounterEvent(B_BATTLER_1, B_BATTLER_0, MOVE_TACKLE, ENC_CAUSE_MOVE_DAMAGE, 0, 0);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == NULL); // wrong move

    SetEncounterEvent(B_BATTLER_1, B_BATTLER_0, MOVE_EARTHQUAKE, ENC_CAUSE_MOVE_DAMAGE, 0, 0);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == sScriptA); // right move

    EndEncounterTest(battleStruct);
}

// --- Test 3: both together - outline Sec13's worked example --------------------------------

static const struct EncounterCondition sConditions_EarthquakeAndLowHp[] =
{
    { .operand = ENC_OP_EVENT_MOVE, .cmp = ENC_CMP_EQ, .arg = 0,                .value = MOVE_EARTHQUAKE },
    { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_EarthquakeAndLowHp[] =
{
    { .checkpoint = ENC_ON_MOVE_END, .priority = 0, .flags = 0, .conditions = sConditions_EarthquakeAndLowHp, .script = sScriptA },
};
static const struct Encounter sEncounter_EarthquakeAndLowHp = { sTriggers_EarthquakeAndLowHp, ARRAY_COUNT(sTriggers_EarthquakeAndLowHp) };

TEST("A trigger with both an event and a battle-state condition fires only when both hold")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_EarthquakeAndLowHp);
    gBattleMons[B_BATTLER_1].maxHP = 100;

    // Move matches, HP doesn't.
    gBattleMons[B_BATTLER_1].hp = 51;
    SetEncounterEvent(B_BATTLER_1, B_BATTLER_0, MOVE_EARTHQUAKE, ENC_CAUSE_MOVE_DAMAGE, 0, 0);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == NULL);

    // HP matches, move doesn't.
    gBattleMons[B_BATTLER_1].hp = 50;
    SetEncounterEvent(B_BATTLER_1, B_BATTLER_0, MOVE_TACKLE, ENC_CAUSE_MOVE_DAMAGE, 0, 0);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == NULL);

    // Both match.
    SetEncounterEvent(B_BATTLER_1, B_BATTLER_0, MOVE_EARTHQUAKE, ENC_CAUSE_MOVE_DAMAGE, 0, 0);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == sScriptA);

    EndEncounterTest(battleStruct);
}

// --- Test 4: ENC_OP_VAR ---------------------------------------------------------------------

static const struct EncounterCondition sConditions_Phase1[] =
{
    { .operand = ENC_OP_VAR, .cmp = ENC_CMP_EQ, .arg = 0, .value = 1 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_Phase1[] =
{
    { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = 0, .conditions = sConditions_Phase1, .script = sScriptA },
};
static const struct Encounter sEncounter_Phase1 = { sTriggers_Phase1, ARRAY_COUNT(sTriggers_Phase1) };

TEST("ENC_OP_VAR reads a variable an earlier script set")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_Phase1);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == NULL); // vars[0] defaults to 0

    battleStruct->encounter.vars[0] = 1; // stand-in for an earlier script's SET_VARIABLE (Stage 14)
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA);

    EndEncounterTest(battleStruct);
}

// --- Test 5: comparison operators at their boundary -----------------------------------------

TEST("Each comparison operator evaluates correctly at its boundary")
{
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);
    struct EncounterCondition conds[2] =
    {
        { .operand = ENC_OP_HP_PERCENT, .arg = ENC_OPPONENT_LEFT, .value = 50 },
        { .operand = ENC_OP_COUNT },
    };

    gBattleMons[B_BATTLER_1].maxHP = 100;
    gBattleMons[B_BATTLER_1].hp = 50; // exactly at the boundary

    conds[0].cmp = ENC_CMP_EQ; EXPECT(EvaluateConditions(conds, FALSE) == TRUE);
    conds[0].cmp = ENC_CMP_NE; EXPECT(EvaluateConditions(conds, FALSE) == FALSE);
    conds[0].cmp = ENC_CMP_LT; EXPECT(EvaluateConditions(conds, FALSE) == FALSE);
    conds[0].cmp = ENC_CMP_LE; EXPECT(EvaluateConditions(conds, FALSE) == TRUE); // fires at exactly 50
    conds[0].cmp = ENC_CMP_GT; EXPECT(EvaluateConditions(conds, FALSE) == FALSE);
    conds[0].cmp = ENC_CMP_GE; EXPECT(EvaluateConditions(conds, FALSE) == TRUE);

    gBattleMons[B_BATTLER_1].hp = 51; // one past the boundary
    conds[0].cmp = ENC_CMP_LE;
    EXPECT(EvaluateConditions(conds, FALSE) == FALSE); // not at 51

    EndEncounterTest(battleStruct);
}

// --- Tests 6 & 7: assertions (manual only) --------------------------------------------------
//
// assertf's test-build handler ends the test immediately as TEST_RESULT_INVALID (see
// event_context.c's comment for why these can't live in the suite as normal TEST() cases). Enable
// one block at a time, run filtered to that test name, confirm it ends INVALID with the expected
// assertion message, then disable it again.

#if 0
TEST("GetEncounterOperand asserts on an invalid operand (manual only, see comment above)")
{
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    GetEncounterOperand(ENC_OP_COUNT, 0, FALSE); // asserts here; ENC_OP_COUNT is the terminator, not a real operand.

    EndEncounterTest(battleStruct);
}
#endif

#if 0
TEST("A _RIGHT battler ref asserts and fails the condition in a singles battle (manual only, see comment above)")
{
    static const struct EncounterCondition sConditions[] =
    {
        { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_PLAYER_RIGHT, .value = 50 },
        { .operand = ENC_OP_COUNT },
    };
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter); // gBattlersCount = 2, singles

    EvaluateConditions(sConditions, FALSE); // asserts here; ENC_PLAYER_RIGHT has no battler in singles.

    EndEncounterTest(battleStruct);
}
#endif
