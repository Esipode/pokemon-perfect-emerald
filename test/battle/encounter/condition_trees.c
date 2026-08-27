#include "global.h"
#include "test/test.h"
#include "malloc.h"
#include "battle.h"
#include "battle_encounter.h"

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

// --- Test 1: the outline Sec14 tree, verbatim -----------------------------------------------
//
// ALL
// |-- battler == boss
// |-- hp_percent <= 50
// `-- ANY
//     |-- weather == RAIN
//     `-- weather == SUN
//
// B_BATTLER_1 stands in for "battler == boss": ENC_BOSS resolves to it in this harness
// (B_POSITION_OPPONENT_LEFT), and ENC_OP_EVENT_BATTLER reads a concrete battler id, not a ref.

static const struct EncounterCondition sConditions_OutlineTree[] =
{
    { .operand = ENC_OP_ALL, .arg = 3 }, // arg is a child count here, not a battler ref
    { .operand = ENC_OP_EVENT_BATTLER, .cmp = ENC_CMP_EQ, .arg = 0, .value = B_BATTLER_1 },
    { .operand = ENC_OP_HP_PERCENT,    .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
    { .operand = ENC_OP_ANY, .arg = 2 },
    { .operand = ENC_OP_WEATHER, .cmp = ENC_CMP_EQ, .value = B_WEATHER_RAIN_NORMAL },
    { .operand = ENC_OP_WEATHER, .cmp = ENC_CMP_EQ, .value = B_WEATHER_SUN_NORMAL },
    { .operand = ENC_OP_COUNT },
};

static const struct EncounterTrigger sTriggers_OutlineTree[] =
{
    { .checkpoint = ENC_ON_MOVE_END, .priority = 0, .flags = 0, .conditions = sConditions_OutlineTree, .script = sScriptA },
};
static const struct Encounter sEncounter_OutlineTree = { sTriggers_OutlineTree, ARRAY_COUNT(sTriggers_OutlineTree) };

TEST("The outline Sec14 tree fires in rain or sun, not in sandstorm, and not above the HP threshold regardless of weather")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_OutlineTree);
    gBattleMons[B_BATTLER_1].maxHP = 100;
    gBattleMons[B_BATTLER_1].hp = 50; // at the HP threshold

    SetEncounterEvent(B_BATTLER_1, B_BATTLER_0, MOVE_NONE, ENC_CAUSE_MOVE_DAMAGE, 0, 0);

    gBattleWeather = B_WEATHER_RAIN_NORMAL;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == sScriptA);

    gBattleWeather = B_WEATHER_SUN_NORMAL;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == sScriptA);

    gBattleWeather = B_WEATHER_SANDSTORM;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == NULL); // ANY fails: neither rain nor sun

    gBattleWeather = B_WEATHER_RAIN_NORMAL;
    gBattleMons[B_BATTLER_1].hp = 60; // above the threshold
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_MOVE_END) == NULL); // ALL fails regardless of weather

    EndEncounterTest(battleStruct);
}

// --- Test 2: NOT around a single leaf ---------------------------------------------------------

static const struct EncounterCondition sConditions_NotLowHp[] =
{
    { .operand = ENC_OP_NOT },
    { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_NotLowHp[] =
{
    { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = 0, .conditions = sConditions_NotLowHp, .script = sScriptA },
};
static const struct Encounter sEncounter_NotLowHp = { sTriggers_NotLowHp, ARRAY_COUNT(sTriggers_NotLowHp) };

TEST("NOT inverts the single node that follows it")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_NotLowHp);
    gBattleMons[B_BATTLER_1].maxHP = 100;

    gBattleMons[B_BATTLER_1].hp = 51;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA); // NOT(hp<=50) - true above 50%

    gBattleMons[B_BATTLER_1].hp = 50;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == NULL); // NOT(hp<=50) - false at 50%

    EndEncounterTest(battleStruct);
}

// --- Test 3: nesting to exactly the depth bound passes -----------------------------------------
//
// Four nested ALL(1) wrappers around one leaf - MAX_ENCOUNTER_COND_DEPTH levels of group nodes.
// The leaf itself isn't a group, so it doesn't add to the count; this is the deepest tree that
// must still evaluate normally.

static const struct EncounterCondition sConditions_DepthFour[] =
{
    { .operand = ENC_OP_ALL, .arg = 1 },
    { .operand = ENC_OP_ALL, .arg = 1 },
    { .operand = ENC_OP_ALL, .arg = 1 },
    { .operand = ENC_OP_ALL, .arg = 1 },
    { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_DepthFour[] =
{
    { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = 0, .conditions = sConditions_DepthFour, .script = sScriptA },
};
static const struct Encounter sEncounter_DepthFour = { sTriggers_DepthFour, ARRAY_COUNT(sTriggers_DepthFour) };

TEST("Nesting to exactly MAX_ENCOUNTER_COND_DEPTH evaluates normally")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_DepthFour);
    gBattleMons[B_BATTLER_1].maxHP = 100;
    gBattleMons[B_BATTLER_1].hp = 50;

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA);

    EndEncounterTest(battleStruct);
}

// --- Test 5: ANY short-circuit walks but does not evaluate a skipped sibling -------------------
//
// The catch this proves: a walk-vs-evaluate mistake would either read past the array (if skipping
// doesn't measure the sibling) or trip the invalid sibling's assertion (if "skipping" secretly
// evaluates it). Neither happens here.

TEST("ANY short-circuits on the first TRUE child without evaluating the rest")
{
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);
    struct EncounterCondition conds[] =
    {
        { .operand = ENC_OP_ANY, .arg = 2 },
        { .operand = ENC_OP_TURN, .cmp = ENC_CMP_GE, .arg = 0, .value = 0 }, // always true - decides the ANY here
        { .operand = 0xFF, .cmp = 0xFF, .value = 0 },              // invalid; must never be evaluated
        { .operand = ENC_OP_COUNT },
    };

    EXPECT(EvaluateConditions(conds, FALSE) == TRUE);

    EndEncounterTest(battleStruct);
}

// --- Tests 4 & 6: assertions (manual only) ------------------------------------------------------
//
// assertf's test-build handler ends the test immediately as TEST_RESULT_INVALID (see
// event_context.c's comment for why these can't live in the suite as normal TEST() cases). Enable
// one block at a time, run filtered to that test name, confirm it ends INVALID with the expected
// assertion message, then disable it again.

#if 0
TEST("Nesting one level past MAX_ENCOUNTER_COND_DEPTH asserts (manual only, see comment above)")
{
    static const struct EncounterCondition sConditions[] =
    {
        { .operand = ENC_OP_ALL, .arg = 1 },
        { .operand = ENC_OP_ALL, .arg = 1 },
        { .operand = ENC_OP_ALL, .arg = 1 },
        { .operand = ENC_OP_ALL, .arg = 1 },
        { .operand = ENC_OP_ALL, .arg = 1 }, // the 5th level - asserts here
        { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
        { .operand = ENC_OP_COUNT },
    };
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EvaluateConditions(sConditions, FALSE);

    EndEncounterTest(battleStruct);
}
#endif

#if 0
TEST("A group arg claiming more children than exist asserts (manual only, see comment above)")
{
    static const struct EncounterCondition sConditions[] =
    {
        { .operand = ENC_OP_ALL, .arg = 5 }, // claims 5 children; only 2 exist before the terminator
        { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
        { .operand = ENC_OP_TURN, .cmp = ENC_CMP_GE, .value = 0 },
        { .operand = ENC_OP_COUNT }, // asserts here, on the 3rd claimed child
    };
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EvaluateConditions(sConditions, FALSE);

    EndEncounterTest(battleStruct);
}
#endif
