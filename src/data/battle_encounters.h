/* TEMPORARY SCAFFOLDING — replaced by tools/encounterproc output in Stage 13.
   Do not author real encounters here. */

// Trivially true (the turn counter is unsigned) - exercises a real conditions array without
// depending on battle state the tests using this table don't set up.
static const struct EncounterCondition sConditions_Test[] =
{
    { .operand = ENC_OP_TURN, .cmp = ENC_CMP_GE, .arg = 0, .value = 0 },
    { .operand = ENC_OP_COUNT },
};

static const struct EncounterTrigger sTriggers_Test[] =
{
    { .checkpoint = ENC_ON_BATTLE_START,
      .priority   = 10,
      .flags      = ENC_TRIGGER_ONCE,
      .conditions = sConditions_Test,
      .script     = EncScript_TestBattleStart },
};

const struct Encounter gEncounters[ENCOUNTER_COUNT] =
{
    [ENCOUNTER_TEST] = { sTriggers_Test, ARRAY_COUNT(sTriggers_Test) },
};
