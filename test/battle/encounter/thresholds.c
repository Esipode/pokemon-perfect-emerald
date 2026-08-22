#include "global.h"
#include "test/test.h"
#include "malloc.h"
#include "battle.h"
#include "battle_encounter.h"

// The outline's edge-detection cases (51% -> 49% fires, 49% -> 48% doesn't, a boss starting
// below the threshold doesn't fire at ENC_ON_BATTLE_START, etc.) need an HP-percent condition to
// evaluate, and struct EncounterCondition doesn't exist until Stage 11. What's tested here is the
// snapshot mechanism those cases depend on: prevHp[] is captured before ENC_ON_BATTLE_START's
// triggers run and re-captured at the end of every checkpoint's dispatch. The outline cases
// themselves are added to this file in Stage 11, once EvaluateConditions() replaces the
// hardcoded wasTrue = FALSE stub in TryRunEncounterCheckpoint.

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

TEST("TryRunEncounterCheckpoint captures the pre-battle HP snapshot on its first ENC_ON_BATTLE_START pass")
{
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    memset(gBattleMons, 0, sizeof(gBattleMons));
    gBattleMons[0].hp = 40;
    gBattleMons[1].hp = 100;

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == NULL);
    EXPECT_EQ(battleStruct->encounter.prevHp[0], 40);
    EXPECT_EQ(battleStruct->encounter.prevHp[1], 100);

    EndEncounterTest(battleStruct);
}

TEST("prevHp updates at the end of a checkpoint's dispatch, not the start")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = ENC_TRIGGER_ONCE | ENC_TRIGGER_ON_ENTER, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    memset(gBattleMons, 0, sizeof(gBattleMons));
    gBattleMons[0].hp = 100;

    // Baseline snapshot, same as a real battle start.
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == NULL);
    EXPECT_EQ(battleStruct->encounter.prevHp[0], 100);

    // Trigger fires; stand in for its script dealing damage before the checkpoint is re-entered.
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA);
    gBattleMons[0].hp = 40;
    EXPECT_EQ(battleStruct->encounter.prevHp[0], 100); // must not have moved mid-checkpoint

    // No more eligible triggers this checkpoint - dispatch is done, snapshot updates now.
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == NULL);
    EXPECT_EQ(battleStruct->encounter.prevHp[0], 40);

    EndEncounterTest(battleStruct);
}

TEST("ENC_TRIGGER_ON_ENTER combined with ENC_TRIGGER_ONCE still fires at most once")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_FAINT, .priority = 0, .flags = ENC_TRIGGER_ONCE | ENC_TRIGGER_ON_ENTER, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == sScriptA);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == NULL);

    EndEncounterTest(battleStruct);
}

// Locks in the documented Stage 10 interim: with no real conditions to evaluate, nowTrue is
// vacuously TRUE and wasTrue is hardcoded FALSE, so ON_ENTER alone doesn't suppress repeat
// firing. Stage 11 changes this for NULL-condition triggers specifically (EvaluateConditions(NULL)
// is TRUE on both sides, so a condition-less ON_ENTER trigger stops firing after its first edge) -
// revisit this test then.
TEST("ENC_TRIGGER_ON_ENTER alone fires every checkpoint while conditions are unimplemented")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_FAINT, .priority = 0, .flags = ENC_TRIGGER_ON_ENTER, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == sScriptA);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == sScriptA);

    EndEncounterTest(battleStruct);
}
