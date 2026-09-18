#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

static const struct EncounterTrigger sTriggers_Faint[] =
{
    { .checkpoint = ENC_ON_FAINT, .priority = 0, .flags = 0, .conditions = NULL, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_Faint = { sTriggers_Faint, ARRAY_COUNT(sTriggers_Faint) };

SINGLE_BATTLE_TEST("ENC_ON_FAINT fires when the boss's first party member faints")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_Faint);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); }
        OPPONENT(SPECIES_VOLTORB);
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); SEND_OUT(opponent, 1); }
    } SCENE {
        MESSAGE("Wobbuffet used Tackle!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
        MESSAGE("The opposing Wobbuffet fainted!");
        MESSAGE(""); // ENC_ON_FAINT - after EXP and absent flags are settled, not during the faint animation
        SEND_IN_MESSAGE("Voltorb");
    }
}
