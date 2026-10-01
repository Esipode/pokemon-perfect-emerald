#include "global.h"
#include "test/battle.h"

#if B_UPDATED_MOVE_FLAGS >= GEN_2
SINGLE_BATTLE_TEST("Struggle does not receive STAB from typeless users")
{
    s32 typedDamage;
    s32 typelessDamage;

    GIVEN {
        ASSUME(GetMoveType(MOVE_STRUGGLE) == TYPE_MYSTERY);
        ASSUME(GetMoveEffect(MOVE_BURN_UP) == EFFECT_FAIL_IF_NOT_ARG_TYPE);
        ASSUME(IsMoveEffectRemoveSpeciesType(MOVE_BURN_UP, MOVE_EFFECT_REMOVE_ARG_TYPE, TYPE_FIRE) == TRUE);
        ASSUME(GetSpeciesType(SPECIES_ARCANINE, 0) == TYPE_FIRE);
        ASSUME(GetSpeciesType(SPECIES_ARCANINE, 1) == TYPE_FIRE);
        PLAYER(SPECIES_ARCANINE) { MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
        TURN { MOVE(player, MOVE_BURN_UP); }
        TURN { MOVE(player, MOVE_STRUGGLE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(opponent, captureDamage: &typedDamage);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BURN_UP, player);
        HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(opponent, captureDamage: &typelessDamage);
    } THEN {
        EXPECT_EQ(player->types[0], TYPE_MYSTERY);
        EXPECT_EQ(player->types[1], TYPE_MYSTERY);
        EXPECT_EQ(player->types[2], TYPE_MYSTERY);
        EXPECT_EQ(typedDamage, typelessDamage);
    }
}
#endif

SINGLE_BATTLE_TEST("Struggle deals recoil 1/4 of damage dealt (Gen 2-3)")
{
    s32 directDamage;
    s32 recoilDamage;
    u32 genConfig;

    PARAMETRIZE { genConfig = GEN_2; }
    PARAMETRIZE { genConfig = GEN_3; }

    GIVEN {
        WITH_CONFIG(B_STRUGGLE_RECOIL, genConfig);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(opponent, captureDamage: &directDamage);
        HP_BAR(player, captureDamage: &recoilDamage);
    } THEN {
        EXPECT_MUL_EQ(directDamage, UQ_4_12(0.25), recoilDamage);
    }
}

SINGLE_BATTLE_TEST("Struggle deals recoil 1/4 of user's hp (Gen 4+)")
{
    ASSUME(GetMoveEffect(MOVE_STRUGGLE) == EFFECT_STRUGGLE);

    s32 recoil;
    u32 atkStat = 0;
    u32 hpStat = 0;

    PARAMETRIZE { atkStat = 100; hpStat = 200; }
    PARAMETRIZE { atkStat = 50; hpStat = 200; }
    PARAMETRIZE { atkStat = 100; hpStat = 300; }

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(hpStat); HP(hpStat); Attack(atkStat); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(player, captureDamage: &recoil);
    } THEN {
        EXPECT_MUL_EQ(hpStat, Q_4_12(0.25), recoil);
    }
}

SINGLE_BATTLE_TEST("Struggle is Normal-type in Gen 1 and typeless in Gen 2+")
{
    ASSUME(GetSpeciesType(SPECIES_DRIFBLIM, 0) == TYPE_GHOST);

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_DRIFBLIM);
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        if (GetConfig(B_UPDATED_MOVE_FLAGS) >= GEN_2) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
            HP_BAR(opponent);
        } else {
            MESSAGE("It doesn't affect the opposing Drifblim…");
        }
    } THEN {
        if (GetConfig(B_UPDATED_MOVE_FLAGS) >= GEN_2)
            EXPECT_EQ(GetMoveType(MOVE_STRUGGLE), TYPE_MYSTERY);
        else
            EXPECT_EQ(GetMoveType(MOVE_STRUGGLE), TYPE_NORMAL);
    }
}

SINGLE_BATTLE_TEST("Struggle does not receive STAB from Normal-type users")
{
    // Compare with Cut, which does receive normal-type STAB
    ASSUME(GetSpeciesType(SPECIES_ZANGOOSE, 0) == TYPE_NORMAL);
    ASSUME(GetMovePower(MOVE_CUT) == GetMovePower(MOVE_STRUGGLE));
    ASSUME(GetMoveCategory(MOVE_CUT) == GetMoveCategory(MOVE_STRUGGLE));
    ASSUME(GetMoveType(MOVE_CUT) == TYPE_NORMAL);
    if (GetConfig(B_UPDATED_MOVE_FLAGS) >= GEN_2)
        ASSUME(GetMoveType(MOVE_STRUGGLE) == TYPE_MYSTERY);
    else
        ASSUME(GetMoveType(MOVE_STRUGGLE) == TYPE_NORMAL);

    s32 cutDamage;
    s32 struggleDamage;

    GIVEN {
        PLAYER(SPECIES_ZANGOOSE);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CUT); }
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CUT, player);
        HP_BAR(opponent, captureDamage: &cutDamage);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(opponent, captureDamage: &struggleDamage);
    } THEN {
        EXPECT_MUL_EQ(struggleDamage, Q_4_12(1.5), cutDamage);
    }
}
