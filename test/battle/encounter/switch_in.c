#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

static const struct EncounterTrigger sTriggers_SwitchIn[] =
{
    { .checkpoint = ENC_ON_SWITCH_IN, .priority = 0, .flags = 0, .conditions = NULL, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_SwitchIn = { sTriggers_SwitchIn, ARRAY_COUNT(sTriggers_SwitchIn) };

SINGLE_BATTLE_TEST("ENC_ON_SWITCH_IN fires when a specific species is sent out")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_SwitchIn);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_METAPOD);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { SWITCH(player, 1); }
    } SCENE {
        SWITCH_OUT_MESSAGE("Wobbuffet");
        SEND_IN_MESSAGE("Metapod");
        MESSAGE(""); // ENC_ON_SWITCH_IN - entry abilities, hazards, form changes and items have resolved
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}
