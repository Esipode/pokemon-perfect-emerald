#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"

static const struct EncounterTrigger sTriggers_MoveEnd[] =
{
    { .checkpoint = ENC_ON_MOVE_END, .priority = 0, .flags = 0, .conditions = NULL, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_MoveEnd = { sTriggers_MoveEnd, ARRAY_COUNT(sTriggers_MoveEnd) };

SINGLE_BATTLE_TEST("ENC_ON_MOVE_END fires once after each battler's move resolves")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_MoveEnd);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE(""); // ENC_ON_MOVE_END after the player's move
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // ENC_ON_MOVE_END after the opponent's move
    }
}

SINGLE_BATTLE_TEST("ENC_ON_MOVE_END fires once for a multi-hit move, not once per hit")
{
    PASSES_RANDOMLY(100, 100, RNG_HITS);

    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_MoveEnd);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SKILL_LINK); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_BULLET_SEED); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BULLET_SEED, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BULLET_SEED, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BULLET_SEED, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BULLET_SEED, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BULLET_SEED, player);
        MESSAGE("The Pokémon was hit 5 time(s)!");
        MESSAGE(""); // ENC_ON_MOVE_END fires once, after all 5 hits resolve
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        MESSAGE(""); // ENC_ON_MOVE_END after the opponent's own move
    }
}
