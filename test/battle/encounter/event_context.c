#include "global.h"
#include "test/test.h"
#include "malloc.h"
#include "battle.h"
#include "battle_encounter.h"

// Content is irrelevant; TryRunEncounterCheckpoint only needs a trigger to enter the checkpoint.
static const u8 sScriptA[] = {0};

static struct BattleStruct *BeginEncounterTest(const struct Encounter *encounter)
{
    struct BattleStruct *battleStruct = AllocZeroed(sizeof(*battleStruct));
    gBattleStruct = battleStruct;
    gBattleStruct->encounter.id = ENCOUNTER_TEST;
    TestSetEncounter(encounter);
    return battleStruct;
}

static void EndEncounterTest(struct BattleStruct *battleStruct)
{
    TestSetEncounter(NULL);
    gBattleStruct = NULL;
    Free(battleStruct);
}

static const struct EncounterTrigger sTriggers_TurnEnd[] =
{
    { .checkpoint = ENC_ON_TURN_END, .priority = 0, .flags = 0, .conditions = NULL, .script = sScriptA },
};
static const struct Encounter sEncounter_TurnEnd = { sTriggers_TurnEnd, ARRAY_COUNT(sTriggers_TurnEnd) };

TEST("GetEncounterEventField reads back what SetEncounterEvent wrote at ENC_ON_TURN_END")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_TurnEnd);
    s32 value;

    // Mirrors battle_main.c's ENC_ON_TURN_END call site: TryRunEncounterCheckpoint first (clears
    // the event on checkpoint entry), then SetEncounterEvent (populates it).
    TryRunEncounterCheckpoint(ENC_ON_TURN_END);
    SetEncounterEvent(B_BATTLER_1, 0, MOVE_NONE, ENC_CAUSE_END_TURN, 0, 0);

    EXPECT(GetEncounterEventField(ENC_EVENT_CAUSE, &value));
    EXPECT_EQ(value, ENC_CAUSE_END_TURN);

    EXPECT(GetEncounterEventField(ENC_EVENT_BATTLER, &value));
    EXPECT_EQ(value, B_BATTLER_1);

    EndEncounterTest(battleStruct);
}

TEST("A later ENC_ON_TURN_END dispatch's event context does not carry the previous one's values")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_TurnEnd);
    s32 value;

    // Turn 1's end-turn event.
    TryRunEncounterCheckpoint(ENC_ON_TURN_END);
    SetEncounterEvent(B_BATTLER_0, 0, MOVE_NONE, ENC_CAUSE_END_TURN, 0, 0);
    EXPECT(GetEncounterEventField(ENC_EVENT_BATTLER, &value));
    EXPECT_EQ(value, B_BATTLER_0);

    // Turn 2's end-turn event, a different battler. The real call site re-populates on every
    // dispatch of the checkpoint, so turn 1's battler must not leak through.
    TryRunEncounterCheckpoint(ENC_ON_TURN_END);
    SetEncounterEvent(B_BATTLER_1, 0, MOVE_NONE, ENC_CAUSE_END_TURN, 0, 0);
    EXPECT(GetEncounterEventField(ENC_EVENT_BATTLER, &value));
    EXPECT_EQ(value, B_BATTLER_1);

    EndEncounterTest(battleStruct);
}

// Test 2 from the spec -- "Reading EVENT_MOVE at ENC_ON_BATTLE_START asserts rather than
// returning a stale value." assertf's test-build handler (Test_ExitWithResult) ends the test
// immediately as TEST_RESULT_INVALID; the suite has no mechanism to mark a test as expecting
// INVALID (see reevaluate.c's runaway test for the same limitation), so this can't live in the
// suite as a normal TEST() without turning `make check` red forever. Enable the block below, run
// this file filtered to this test name, and confirm it ends INVALID with the "event field ... not
// valid at checkpoint ..." assertion message, then disable it again.
#if 0
TEST("GetEncounterEventField asserts reading a field ENC_ON_BATTLE_START doesn't populate (manual only, see comment above)")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_TurnEnd);
    s32 value;

    TryRunEncounterCheckpoint(ENC_ON_BATTLE_START);
    GetEncounterEventField(ENC_EVENT_MOVE, &value); // asserts here; mask for ENC_ON_BATTLE_START is 0.

    EndEncounterTest(battleStruct);
}
#endif
