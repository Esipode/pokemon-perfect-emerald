#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

SINGLE_BATTLE_TEST("ENC_ON_BATTLE_START interrupts the intro and returns control to the battle")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // EncScript_TestBattleStart's placeholder text; its presence proves the script ran.
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}
