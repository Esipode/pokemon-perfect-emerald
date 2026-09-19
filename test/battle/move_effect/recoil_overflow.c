#include "global.h"
#include "test/battle.h"

// Regression test for the reported bug: a level-1000 Charizard used Flare Blitz
// against a level-21 opponent, lost *all* of its HP to recoil, and the *opponent*
// was healed to full HP instead of taking damage.
//
// Guards against gBattleStruct->moveDamage/passiveHpUpdate truncating a
// CalculateMoveDamage result. At MAX_LEVEL the damage exceeds 32,767, wraps
// negative in an s16, and MoveDamageDataHpUpdate/PassiveDataHpUpdate treat it as
// a heal. Recoil then re-truncates the corrupted value.
//
//   levelFactor = 2 * 1000 / 5 + 2                       = 402
//   base = 120 * 1000 * 402 / 20 / 50 + 2                = 48,242
//   STAB (Charizard is Fire-type, Flare Blitz is Fire)    x1.5   = 72,363
//   type effectiveness (Psychic defender, neutral to Fire) x1.0
//   damage roll (85%-100%, integer division)             = 61,508 .. 72,363
//
// The opponent's max HP (100) is far below that range, so it faints regardless
// of the roll.
//
//   recoil (33% of the raw, un-clamped damage, floor division) = 20,297 .. 23,879
//
// The attacker's max HP (60,000) exceeds any possible recoil, so a correct
// result leaves it alive with bounded HP loss.
SINGLE_BATTLE_TEST("Recoil overflow: level-1000 Flare Blitz damages the target and gives the user bounded recoil, not a wrapped heal/wipe")
{
    u16 hp;
    GIVEN {
        ASSUME(GetMovePower(MOVE_FLARE_BLITZ) == 120);
        ASSUME(GetMoveType(MOVE_FLARE_BLITZ) == TYPE_FIRE);
        ASSUME(GetMoveCategory(MOVE_FLARE_BLITZ) == DAMAGE_CATEGORY_PHYSICAL);
        ASSUME(GetMoveRecoil(MOVE_FLARE_BLITZ) == 33);
        // Charizard is Fire/Flying (STAB applies); Wobbuffet is Psychic (neutral to Fire).
        // Full HP going in, so Blaze's <=1/3-HP power boost does not apply.
        PLAYER(SPECIES_CHARIZARD) { Level(MAX_LEVEL); Attack(1000); HP(60000); MaxHP(60000); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(21); Defense(20); HP(100); MaxHP(100); }
    } WHEN {
        TURN {
            MOVE(player, MOVE_FLARE_BLITZ, WITH_RNG(RNG_DAMAGE_MODIFIER, 0), criticalHit: FALSE);
        }
    }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FLARE_BLITZ, player);
        // Opponent takes real damage and faints - not healed to full via a wrapped negative.
        HP_BAR(opponent, hp: 0);
        MESSAGE("The opposing Wobbuffet fainted!");
        // Attacker takes bounded recoil - not wiped to 0 via a wrapped-positive value.
        HP_BAR(player, captureHP: &hp);
    }
    THEN {
        EXPECT_GT(hp, 30000);
        EXPECT_LT(hp, 45000);
    }
}
