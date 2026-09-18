#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

static const struct EncounterTrigger sTriggers_TurnEndOnce[] =
{
    { .checkpoint = ENC_ON_TURN_END, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestTurnEnd },
};
static const struct Encounter sEncounter_TurnEndOnce = { sTriggers_TurnEndOnce, ARRAY_COUNT(sTriggers_TurnEndOnce) };

static const struct EncounterTrigger sTriggers_TurnEndRepeat[] =
{
    { .checkpoint = ENC_ON_TURN_END, .priority = 10, .flags = 0, .conditions = NULL, .script = EncScript_TestTurnEnd },
};
static const struct Encounter sEncounter_TurnEndRepeat = { sTriggers_TurnEndRepeat, ARRAY_COUNT(sTriggers_TurnEndRepeat) };

SINGLE_BATTLE_TEST("ENC_ON_TURN_END with ENC_TRIGGER_ONCE fires at the end of turn 1 and not again")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_TurnEndOnce);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // EncScript_TestTurnEnd's placeholder text; fires once, at the end of turn 1.
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        NOT MESSAGE(""); // must not fire again at the end of turns 2 or 3.
    }
}

SINGLE_BATTLE_TEST("ENC_ON_TURN_END without ENC_TRIGGER_ONCE fires at the end of every turn")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_TurnEndRepeat);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // end of turn 1
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE(""); // end of turn 2 -- the sub-state bit must have reset
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE(""); // end of turn 3
    }
}
