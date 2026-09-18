#include "global.h"
#include "test/battle.h"
#include "battle_encounter.h"
#include "constants/pokemon.h"

// Stage 15: the encounter command vocabulary (asm/macros/battle_script.inc), run through the real
// interpreter the same way Stage 14's variables.c does. CHANGE_HP is verified with HP_BAR since it
// has real presentation; CHANGE_STAT is silent (outline Sec31) so it's verified the way Stage 14
// verified script variables - a lower-priority trigger at the same checkpoint reads the result back
// through a condition, closing the loop between a command and the condition system it feeds.

// --- CHANGE_HP: single target, damage and heal ------------------------------------------------

static const struct EncounterTrigger sTriggers_ChangeHpDamage[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestChangeHpDamage },
};
static const struct Encounter sEncounter_ChangeHpDamage = { sTriggers_ChangeHpDamage, ARRAY_COUNT(sTriggers_ChangeHpDamage) };

SINGLE_BATTLE_TEST("CHANGE_HP damages a single target")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_ChangeHpDamage);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(100); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(100); HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(opponent, damage: 30);
        MESSAGE("Wobbuffet used Celebrate!");
    }
}

static const struct EncounterTrigger sTriggers_ChangeHpHeal[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestChangeHpHeal },
};
static const struct Encounter sEncounter_ChangeHpHeal = { sTriggers_ChangeHpHeal, ARRAY_COUNT(sTriggers_ChangeHpHeal) };

SINGLE_BATTLE_TEST("CHANGE_HP heals a single target")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_ChangeHpHeal);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(100); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(100); HP(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(opponent, damage: -20);
        MESSAGE("Wobbuffet used Celebrate!");
    }
}

// --- CHANGE_HP: ALL_FOES in doubles -------------------------------------------------------------

static const struct EncounterTrigger sTriggers_ChangeHpAllFoes[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestChangeHpAllFoes },
};
static const struct Encounter sEncounter_ChangeHpAllFoes = { sTriggers_ChangeHpAllFoes, ARRAY_COUNT(sTriggers_ChangeHpAllFoes) };

DOUBLE_BATTLE_TEST("CHANGE_HP with ALL_FOES damages every battler on the boss's foe side")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_ChangeHpAllFoes);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(100); HP(100); }
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(100); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(100); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(100); HP(100); }
    } WHEN {
        TURN {}
    } SCENE {
        // ENC_TARGET_ALL_FOES resolves relative to ENC_BOSS (opponent slot 0), so the foe side is
        // the player side; the command's internal loop visits battlers in ascending index order.
        HP_BAR(playerLeft, damage: 15);
        HP_BAR(playerRight, damage: 15);
    }
}

// --- CHANGE_STAT: single target and ALL_FOES in doubles -----------------------------------------
//
// Silent by design (outline Sec31) - verified the Stage 14 way, via a lower-priority trigger at
// the same checkpoint whose condition reads the changed stat stage back.

static const struct EncounterCondition sConditions_DefRoseOnBoss[] =
{
    { .operand = ENC_OP_STAT_STAGE, .cmp = ENC_CMP_EQ, .arg = ENC_PACK_STAT_ARG(ENC_BOSS, STAT_DEF), .value = DEFAULT_STAT_STAGE + 2 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_ChangeStat[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestChangeStat },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_DefRoseOnBoss, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_ChangeStat = { sTriggers_ChangeStat, ARRAY_COUNT(sTriggers_ChangeStat) };

SINGLE_BATTLE_TEST("CHANGE_STAT raises a stat stage on a single target")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_ChangeStat);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE(""); // priority-10 trigger; only eligible once the boss's Defense stage is +2
        MESSAGE("Wobbuffet used Celebrate!");
    }
}

static const struct EncounterCondition sConditions_AtkFellOnBothFoes[] =
{
    { .operand = ENC_OP_ALL, .arg = 2 },
    { .operand = ENC_OP_STAT_STAGE, .cmp = ENC_CMP_EQ, .arg = ENC_PACK_STAT_ARG(ENC_PLAYER_LEFT, STAT_ATK),  .value = DEFAULT_STAT_STAGE - 1 },
    { .operand = ENC_OP_STAT_STAGE, .cmp = ENC_CMP_EQ, .arg = ENC_PACK_STAT_ARG(ENC_PLAYER_RIGHT, STAT_ATK), .value = DEFAULT_STAT_STAGE - 1 },
    { .operand = ENC_OP_COUNT },
};
static const struct EncounterTrigger sTriggers_ChangeStatAllFoes[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0,  .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestChangeStatAllFoes },
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 10, .flags = ENC_TRIGGER_ONCE, .conditions = sConditions_AtkFellOnBothFoes, .script = EncScript_TestGeneric },
};
static const struct Encounter sEncounter_ChangeStatAllFoes = { sTriggers_ChangeStatAllFoes, ARRAY_COUNT(sTriggers_ChangeStatAllFoes) };

DOUBLE_BATTLE_TEST("CHANGE_STAT with ALL_FOES lowers a stat stage on every battler on the boss's foe side")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_ChangeStatAllFoes);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {}
    } SCENE {
        MESSAGE(""); // priority-10 trigger; only eligible once both player battlers' Attack is -1
    }
}

// --- MEGA_EVOLVE: forced entry point, outside the normal gimmick-selection flow -----------------

static const struct EncounterTrigger sTriggers_MegaEvolve[] =
{
    { .checkpoint = ENC_ON_BATTLE_START, .priority = 0, .flags = ENC_TRIGGER_ONCE, .conditions = NULL, .script = EncScript_TestMegaEvolve },
};
static const struct Encounter sEncounter_MegaEvolve = { sTriggers_MegaEvolve, ARRAY_COUNT(sTriggers_MegaEvolve) };

SINGLE_BATTLE_TEST("MEGA_EVOLVE forces a Mega Evolution outside the gimmick-selection flow")
{
    GIVEN {
        SetPendingBattleEncounter(ENCOUNTER_TEST);
        TestSetEncounter(&sEncounter_MegaEvolve);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_VENUSAUR) { Item(ITEM_VENUSAURITE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_MEGA_EVOLUTION, opponent);
        MESSAGE("The opposing Venusaur has Mega Evolved into Mega Venusaur!");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_VENUSAUR_MEGA);
    }
}

// A fainted/absent target asserts for every state-changing command above (CHANGE_HP, CHANGE_STAT,
// MEGA_EVOLVE) - each has its own assertf on IsBattlerAlive/battler validity in
// src/battle_script_commands.c. Not automatable: assertf's TESTING handler
// (Test_ExitWithResult(TEST_RESULT_INVALID, ...)) exits the test process rather than running the
// recovery block and returning, the same limitation documented in conditions.c and targeting.c.
