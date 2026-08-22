/* TEMPORARY SCAFFOLDING — replaced by tools/encounterproc output in Stage 13.
   Do not author real encounters here. */

static const struct EncounterTrigger sTriggers_Test[] =
{
    { .checkpoint = ENC_ON_BATTLE_START,
      .priority   = 10,
      .flags      = ENC_TRIGGER_ONCE,
      .conditions = NULL,
      .script     = EncScript_TestBattleStart },
};

const struct Encounter gEncounters[ENCOUNTER_COUNT] =
{
    [ENCOUNTER_TEST] = { sTriggers_Test, ARRAY_COUNT(sTriggers_Test) },
};
