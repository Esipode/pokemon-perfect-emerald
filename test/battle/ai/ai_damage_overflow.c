#include "global.h"
#include "test/battle.h"

// Regression test for Bug E: struct SimulatedDamage (include/battle.h) is the AI's
// own damage estimate - a storage path entirely separate from gBattleStruct's
// moveDamage/passiveHpUpdate (Bug C). Before Stage 5 its four fields were u16, so
// even though CalculateMoveDamage's return value is a correctly-computed s32 (once
// Stages 2-3 fix the upstream overflow), AI_CalcDamage's writes into
// gAiLogicData->simulatedDmg silently wrapped modulo 65,536 for MAX_LEVEL matchups -
// corrupting AI move/switch decisions independently of the player-visible HP bug.
// See "Damage Calc Patch.md", Stage 5 / Bug E.
//
// Same Charizard/Flare Blitz-vs-Wobbuffet matchup as
// test/battle/move_effect/recoil_overflow.c (Stage 4's primary repro), with the AI
// on the attacking side this time so its own simulated damage can be inspected
// directly:
//
//   levelFactor = 2 * 1000 / 5 + 2                       = 402
//   base = 120 * 1000 * 402 / 20 / 50 + 2                = 48,242
//   STAB (Charizard is Fire-type, Flare Blitz is Fire)    x1.5   = 72,363
//   AI roll types (no random factor applied, GetDamageByRollType):
//     minimum (85%) = 61,508   median (93%) = 67,297   maximum (100%) = 72,363
//
// median and maximum both clear the old u16 ceiling (65,535); a pre-Stage-5 build
// would report a wrapped, much smaller (or misleadingly still-plausible-looking)
// value here instead.
AI_SINGLE_BATTLE_TEST("AI's simulated damage for a level-1000 attacker is large and positive, not wrapped small by a u16 field")
{
    GIVEN {
        ASSUME(GetMovePower(MOVE_FLARE_BLITZ) == 120);
        ASSUME(GetMoveType(MOVE_FLARE_BLITZ) == TYPE_FIRE);
        ASSUME(GetMoveCategory(MOVE_FLARE_BLITZ) == DAMAGE_CATEGORY_PHYSICAL);
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        // Full HP going in, so Blaze's <=1/3-HP power boost does not apply.
        PLAYER(SPECIES_WOBBUFFET) { Level(21); Defense(20); HP(100000); MaxHP(100000); }
        OPPONENT(SPECIES_CHARIZARD) { Level(MAX_LEVEL); Attack(1000); Moves(MOVE_FLARE_BLITZ); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_FLARE_BLITZ); }
    } THEN {
        enum BattlerId battlerAtk = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        enum BattlerId battlerDef = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        u32 moveIndex = 0;

        EXPECT_GT(gAiLogicData->simulatedDmg[battlerAtk][battlerDef][moveIndex].median, 65535);
        EXPECT_GT(gAiLogicData->simulatedDmg[battlerAtk][battlerDef][moveIndex].maximum, 65535);
    }
}
