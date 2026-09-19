#include "global.h"
#include "test/test.h"
#include "malloc.h"
#include "battle.h"
#include "battle_encounter.h"

// Edge-detection cases: 51% -> 49% fires, 49% -> 48% doesn't, and a boss starting below the
// threshold doesn't fire at ENC_ON_BATTLE_START.
//
// A *condition-less* ON_ENTER trigger never edges: EvaluateConditions(NULL, ...) is TRUE regardless
// of useSnapshot, so nowTrue and wasTrue are always equal - there's no FALSE state to transition
// out of.

static const u8 sScriptA[] = {0};

static struct BattleStruct *BeginEncounterTest(const struct Encounter *encounter)
{
    struct BattleStruct *battleStruct = AllocZeroed(sizeof(*battleStruct));
    gBattleStruct = battleStruct;
    gBattleStruct->encounter.id = ENCOUNTER_TEST;
    TestSetEncounter(encounter);

    // Minimal singles battle field, for conditions that resolve a battler ref (ENC_OPPONENT_LEFT).
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

TEST("TryRunEncounterCheckpoint captures the pre-battle HP snapshot on its first ENC_ON_BATTLE_START pass")
{
    static const struct Encounter sEncounter = { NULL, 0 };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    gBattleMons[0].hp = 40;
    gBattleMons[1].hp = 100;

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == NULL);
    EXPECT_EQ(battleStruct->encounter.prevHp[0], 40);
    EXPECT_EQ(battleStruct->encounter.prevHp[1], 100);

    EndEncounterTest(battleStruct);
}

TEST("prevHp updates at the end of a checkpoint's dispatch, not the start")
{
    // ENC_TRIGGER_ONCE alone is enough to get one deterministic fire-then-stop; what's under test
    // is prevHp timing, not edge detection.
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

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

static const struct EncounterCondition sConditions_LowHp[] =
{
    { .operand = ENC_OP_HP_PERCENT, .cmp = ENC_CMP_LE, .arg = ENC_OPPONENT_LEFT, .value = 50 },
    { .operand = ENC_OP_COUNT },
};

static const struct EncounterTrigger sTriggers_LowHpOnceOnEnter[] =
{
    { .checkpoint = ENC_ON_FAINT, .priority = 0, .flags = ENC_TRIGGER_ONCE | ENC_TRIGGER_ON_ENTER, .conditions = sConditions_LowHp, .script = sScriptA },
};
static const struct Encounter sEncounter_LowHpOnceOnEnter = { sTriggers_LowHpOnceOnEnter, ARRAY_COUNT(sTriggers_LowHpOnceOnEnter) };

TEST("ENC_TRIGGER_ON_ENTER combined with ENC_TRIGGER_ONCE still fires at most once")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_LowHpOnceOnEnter);

    gBattleMons[B_BATTLER_1].maxHP = 100;
    gBattleMons[B_BATTLER_1].hp = 51; // above the threshold - baseline snapshot
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == NULL);

    gBattleMons[B_BATTLER_1].hp = 40; // crosses below 50%
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == sScriptA); // the crossing - fires
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == NULL);     // already fired (ONCE)

    EndEncounterTest(battleStruct);
}

static const struct EncounterTrigger sTriggers_LowHpOnEnter[] =
{
    { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = ENC_TRIGGER_ON_ENTER, .conditions = sConditions_LowHp, .script = sScriptA },
};
static const struct Encounter sEncounter_LowHpOnEnter = { sTriggers_LowHpOnEnter, ARRAY_COUNT(sTriggers_LowHpOnEnter) };

TEST("A level condition fires ON_ENTER on the crossing and not again while it stays true (51% -> 49% -> 48%)")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_LowHpOnEnter);

    gBattleMons[B_BATTLER_1].maxHP = 100;
    gBattleMons[B_BATTLER_1].hp = 51;
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == NULL); // baseline: above the threshold

    gBattleMons[B_BATTLER_1].hp = 49; // crosses to at-or-below 50% - this is the edge
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA);

    // prevHp only refreshes once a dispatch finds nothing eligible (guarantee #7) - without ONCE,
    // re-entering ENC_ON_TURN_START itself right now would still see the pre-crossing snapshot and
    // fire again. A later checkpoint with nothing eligible for this encounter is what actually
    // refreshes it, same as the rest of a real turn passing before the next ENC_ON_TURN_START.
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_END) == NULL);

    gBattleMons[B_BATTLER_1].hp = 48; // still below the threshold at the next checkpoint - not a new crossing
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == NULL);

    EndEncounterTest(battleStruct);
}

static const struct EncounterTrigger sTriggers_LowHpOnEnterAtBattleStart[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0, .flags = ENC_TRIGGER_ON_ENTER, .conditions = sConditions_LowHp, .script = sScriptA },
};
static const struct Encounter sEncounter_LowHpOnEnterAtBattleStart = { sTriggers_LowHpOnEnterAtBattleStart, ARRAY_COUNT(sTriggers_LowHpOnEnterAtBattleStart) };

TEST("A boss starting the battle already below the threshold doesn't fire ON_ENTER at ENC_ON_BATTLE_START")
{
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter_LowHpOnEnterAtBattleStart);

    gBattleMons[B_BATTLER_1].maxHP = 100;
    gBattleMons[B_BATTLER_1].hp = 40; // already below the threshold when the battle begins

    // prevHp is captured from this same starting HP before triggers run, so nowTrue == wasTrue -
    // there is no crossing to have happened yet.
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == NULL);

    EndEncounterTest(battleStruct);
}

// EvaluateConditions(NULL, ...) is TRUE regardless of useSnapshot, so nowTrue and wasTrue are
// always equal for a condition-less trigger - there is no FALSE state for it to transition out of,
// so ON_ENTER alone can never select it. Give ON_ENTER a real, snapshot-sensitive condition (see
// the HP threshold tests above) to get real edge detection.
TEST("ENC_TRIGGER_ON_ENTER alone never fires for a condition-less trigger")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_FAINT, .priority = 0, .flags = ENC_TRIGGER_ON_ENTER, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == NULL);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == NULL);

    EndEncounterTest(battleStruct);
}
