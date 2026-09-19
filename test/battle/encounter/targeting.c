#include "global.h"
#include "test/test.h"
#include "malloc.h"
#include "battle.h"
#include "battle_encounter.h"
#include "constants/moves.h"

// ResolveEncounterTarget (src/battle_encounter.c) resolves an EncounterTarget to a
// battler bitmask. Unit-tested directly, the same way conditions.c unit-tests EvaluateConditions,
// since the resolver has no dependency on the trigger/script machinery.

static struct BattleStruct *BeginTargetingTest(u32 battlersCount)
{
    struct BattleStruct *battleStruct = AllocZeroed(sizeof(*battleStruct));
    gBattleStruct = battleStruct;
    gBattleStruct->encounter.id = ENCOUNTER_TEST;

    memset(gBattleMons, 0, sizeof(gBattleMons));
    gBattlersCount = battlersCount;
    gBattlerPositions[B_BATTLER_0] = B_POSITION_PLAYER_LEFT;
    gBattlerPositions[B_BATTLER_1] = B_POSITION_OPPONENT_LEFT;
    if (battlersCount == 4)
    {
        gBattlerPositions[B_BATTLER_2] = B_POSITION_PLAYER_RIGHT;
        gBattlerPositions[B_BATTLER_3] = B_POSITION_OPPONENT_RIGHT;
    }

    return battleStruct;
}

static void EndTargetingTest(struct BattleStruct *battleStruct)
{
    gBattleStruct = NULL;
    Free(battleStruct);
}

// --- Test 1: single-slot targets resolve to the right battler in singles -------------------

TEST("Single-slot targets resolve to the right battler in singles")
{
    struct BattleStruct *battleStruct = BeginTargetingTest(2);

    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_BOSS), (1u << B_BATTLER_1));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_PLAYER_LEFT), (1u << B_BATTLER_0));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_OPPONENT_LEFT), (1u << B_BATTLER_1));

    // ENC_TARGET_SELF reads runtime->event.battler directly (ResolveEncounterBattlerRef's ENC_SELF
    // case), no checkpoint validity gate - unlike EVENT_TARGET below.
    gBattleStruct->encounter.event.battler = B_BATTLER_0;
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_SELF), (1u << B_BATTLER_0));

    // EVENT_TARGET reads through GetEncounterEventField, which is only valid once a checkpoint
    // that populates ENC_EVENT_TARGET is current.
    gBattleStruct->encounter.checkpoint = ENC_ON_MOVE_END;
    SetEncounterEvent(B_BATTLER_0, B_BATTLER_1, MOVE_NONE, ENC_CAUSE_MOVE_DAMAGE, 0, 0);
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_EVENT_TARGET), (1u << B_BATTLER_1));

    EndTargetingTest(battleStruct);
}

// --- Test 2: group targets resolve relative to the boss's side in singles ------------------

TEST("ALL_FOES/ALL_ALLIES/ALL_BATTLERS resolve relative to the boss's side in singles")
{
    struct BattleStruct *battleStruct = BeginTargetingTest(2);

    // Boss is B_BATTLER_1 (opponent side): foes are the player side, allies are the opponent side.
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_ALL_FOES), (1u << B_BATTLER_0));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_ALL_ALLIES), (1u << B_BATTLER_1));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_ALL_BATTLERS), (1u << B_BATTLER_0) | (1u << B_BATTLER_1));

    EndTargetingTest(battleStruct);
}

// --- Test 3: every target resolves correctly in doubles -------------------------------------

TEST("Every target resolves correctly in doubles")
{
    struct BattleStruct *battleStruct = BeginTargetingTest(4);

    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_BOSS), (1u << B_BATTLER_1));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_PLAYER_LEFT), (1u << B_BATTLER_0));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_PLAYER_RIGHT), (1u << B_BATTLER_2));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_OPPONENT_LEFT), (1u << B_BATTLER_1));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_OPPONENT_RIGHT), (1u << B_BATTLER_3));

    // Boss (B_BATTLER_1) is on the opponent side: foes are both player battlers, allies are both
    // opponent battlers - this is the "for each targeted battler" loop working identically to
    // singles, just over a wider mask.
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_ALL_FOES), (1u << B_BATTLER_0) | (1u << B_BATTLER_2));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_ALL_ALLIES), (1u << B_BATTLER_1) | (1u << B_BATTLER_3));
    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_ALL_BATTLERS),
              (1u << B_BATTLER_0) | (1u << B_BATTLER_1) | (1u << B_BATTLER_2) | (1u << B_BATTLER_3));

    EndTargetingTest(battleStruct);
}

// --- Test 4 (the critical case): a _RIGHT target in singles -----------------
//
// ENC_TARGET_PLAYER_RIGHT/OPPONENT_RIGHT delegate to ResolveEncounterBattlerRef, which asserts
// when the ref has no battler in the current format. assertf's TESTING handler
// (Test_ExitWithResult(TEST_RESULT_INVALID, ...)) exits the test process outright rather than
// running the recovery block and returning - see test/battle/encounter/conditions.c's identical
// disabled test for the same reason. There is no automated way to observe "asserted, then returned
// an empty mask" from inside the test harness; confirm by manual/debug-build inspection.
#if 0
TEST("A _RIGHT target in singles asserts and resolves to an empty mask (manual only, see comment above)")
{
    struct BattleStruct *battleStruct = BeginTargetingTest(2);

    EXPECT_EQ(ResolveEncounterTarget(ENC_TARGET_PLAYER_RIGHT), 0); // asserts here in a debug/test build

    EndTargetingTest(battleStruct);
}
#endif
