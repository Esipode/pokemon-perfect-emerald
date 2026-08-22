#include "global.h"
#include "test/test.h"
#include "malloc.h"
#include "battle.h"
#include "battle_encounter.h"

// Content is irrelevant; only script identity (pointer equality) is checked.
static const u8 sScriptA[] = {0};
static const u8 sScriptB[] = {0};
static const u8 sScriptC[] = {0};

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

TEST("TryRunEncounterCheckpoint returns NULL when no encounter is active")
{
    struct BattleStruct *battleStruct = AllocZeroed(sizeof(*battleStruct));
    gBattleStruct = battleStruct;
    // encounter.id is left at ENCOUNTER_NONE by the zeroed alloc.

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == NULL);

    gBattleStruct = NULL;
    Free(battleStruct);
}

TEST("TryRunEncounterCheckpoint returns the script of a matching trigger")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = 0, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_BATTLE_START) == sScriptA);

    EndEncounterTest(battleStruct);
}

TEST("TryRunEncounterCheckpoint returns NULL for a trigger at a different checkpoint")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = 0, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_END) == NULL);

    EndEncounterTest(battleStruct);
}

TEST("TryRunEncounterCheckpoint picks the lowest-priority-value trigger among eligible triggers")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_TURN_START, .priority = 10, .flags = 0, .conditions = NULL, .script = sScriptA },
        { .checkpoint = ENC_ON_TURN_START, .priority = 0,  .flags = 0, .conditions = NULL, .script = sScriptB },
        { .checkpoint = ENC_ON_TURN_START, .priority = 20, .flags = 0, .conditions = NULL, .script = sScriptC },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptB);

    EndEncounterTest(battleStruct);
}

TEST("TryRunEncounterCheckpoint breaks equal-priority ties by table order, deterministically")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_TURN_START, .priority = 5, .flags = 0, .conditions = NULL, .script = sScriptA },
        { .checkpoint = ENC_ON_TURN_START, .priority = 5, .flags = 0, .conditions = NULL, .script = sScriptB },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_TURN_START) == sScriptA);

    EndEncounterTest(battleStruct);
}

TEST("TryRunEncounterCheckpoint fires an ENC_TRIGGER_ONCE trigger only once")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_FAINT, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == sScriptA);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == NULL);

    EndEncounterTest(battleStruct);
}

TEST("TryRunEncounterCheckpoint fires a non-ENC_TRIGGER_ONCE trigger every time")
{
    static const struct EncounterTrigger sTriggers[] =
    {
        { .checkpoint = ENC_ON_FAINT, .priority = 0, .flags = 0, .conditions = NULL, .script = sScriptA },
    };
    static const struct Encounter sEncounter = { sTriggers, ARRAY_COUNT(sTriggers) };
    struct BattleStruct *battleStruct = BeginEncounterTest(&sEncounter);

    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == sScriptA);
    EXPECT(TryRunEncounterCheckpoint(ENC_ON_FAINT) == sScriptA);

    EndEncounterTest(battleStruct);
}
