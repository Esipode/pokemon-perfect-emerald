#include "global.h"
#include "test/battle.h"

// Regression test for the reported bug: a level-1000 Charizard used Flare Blitz
// against a level-21 opponent, lost *all* of its HP to recoil, and the *opponent*
// was healed to full HP instead of taking damage.
//
// Root cause (Bug C): CalculateMoveDamage's correctly-computed s32 result was
// stored into `gBattleStruct->moveDamage`, an s16 field. At MAX_LEVEL magnitudes
// the raw computed damage routinely exceeds 32,767 and wraps via two's-complement
// into a negative number, which `MoveDamageDataHpUpdate`/`PassiveDataHpUpdate`
// faithfully treat as "heal instead of damage" (a legitimate mechanic elsewhere,
// disastrous here). Recoil then inherits and re-truncates the already-corrupted
// value a second time.
//
// This test only exercises the reported symptom correctly once Stages 2-3 (the
// upstream CalculateBaseDamage/fpmath overflow fixes) and this stage (widening
// moveDamage/passiveHpUpdate to s32) have all landed.
//
//   levelFactor = 2 * 1000 / 5 + 2                       = 402
//   base = 120 * 1000 * 402 / 20 / 50 + 2                = 48,242
//   STAB (Charizard is Fire-type, Flare Blitz is Fire)    x1.5   = 72,363
//   type effectiveness (Psychic defender, neutral to Fire) x1.0
//   damage roll (85%-100%, integer division)             = 61,508 .. 72,363
//
// Both ends of that range are well above the old s16 ceiling (32,767) - this is
// exactly the "computed damage well within the worked-example range" that used
// to wrap. The opponent's max HP (100) is far below either end of the range, so
// it faints outright regardless of the exact roll - the wrapped pre-fix value
// could instead have come out negative and healed it to full.
//
//   recoil (33% of the raw, un-clamped damage, floor division) = 20,297 .. 23,879
//
// The attacker's max HP (60,000) is set well above any possible recoil value so
// a correct fix leaves it alive with a real, bounded HP loss - not fainted (the
// reported bug's wrapped-positive outcome) and not still at full HP (the wrapped
// -negative "heal" outcome a different matchup could have produced instead).
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
