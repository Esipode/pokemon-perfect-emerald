#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

static const struct EncounterTrigger sTriggers_TurnStart[] =
{
    { .checkpoint = ENC_ON_TURN_START, .priority = 0, .flags = 0, .conditions = NULL, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_TurnStart = { sTriggers_TurnStart, ARRAY_COUNT(sTriggers_TurnStart) };

SINGLE_BATTLE_TEST("ENC_ON_TURN_START fires at the start of turn 3, before either battler acts")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_TurnStart);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // ENC_ON_TURN_START, turn 1
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // ENC_ON_TURN_START, turn 2
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // ENC_ON_TURN_START, turn 3 - before either battler's move this turn
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}
