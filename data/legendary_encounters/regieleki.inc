// Regieleki, "The Electrified Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Regieleki_Intro:
// 0 Charge (0-10 meter), 1 Phase (0/1/2), 2 Tier (Overcharge tier, 0/1/2), 3 TurnStartGuard,
// 4 MoveGuard, 5 ElecGuard, 6 SwitchGuard, 7 HitGuard, 8 Scratch, 9 SurgeWindow,
// 10 PlayerCat (DAMAGE_CATEGORY_PHYSICAL=1 / SPECIAL=2 / sentinel STATUS=3 for "nothing landed"),
// 11 PlayerType (TYPE_GROUND=5 / sentinel TYPE_MYSTERY=10 for "nothing landed"), 12 TypeGuard.
//
// CHARGE IS THE GUARD, IN BOTH DIRECTIONS. EncScript_Regieleki_ApplyTierStats and
// EncScript_Regieleki_ApplyReduction are the only writers of Regieleki's Overcharge stats and
// damage reduction outside Electro Drift's own collapse, and both read Tier absolutely - a tier
// falling undoes a tier rising with no separate teardown path.
//
// Event.MoveCategory/Event.MoveType/Terrain are Condition-only fields - unreadable from inside a
// script - so PlayerCat, PlayerType and the Electric Terrain Charge tick are each driven by their
// own dedicated trigger/script pair in the .encounter file rather than branching inside one
// PlayerHitBoss script. See the deviations note at the top of regieleki.encounter.
//
// EVERY OnMoveEnd/OnSwitchIn TRIGGER IS SELF-DISABLING through the guard var its own script sets.
// Event.* stays true for a whole dispatch, so a script that returns without closing its latch
// re-selects until the runaway-script guard trips.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// Clamps Charge to 10. Called after every source that can raise it.
EncScript_Regieleki_ClampCharge:
	encjumpifvar CMP_LESS_THAN, 0, 11, EncScript_Regieleki_ClampCharge_Done
	encsetvar 0, 10
EncScript_Regieleki_ClampCharge_Done:
	return

// THE SINGLE WRITER of Regieleki's Overcharge stat stages. Written absolutely (push to +12,
// which clamps at +6, then the single delta that lands on the stage Tier means) so the tier
// survives a player Haze, a scour, or a switch landing in between.
EncScript_Regieleki_ApplyTierStats:
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -6
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -6
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -6
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Regieleki_ApplyTierStats_Done
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regieleki_ApplyTierStats_T1
// Tier 2.
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	return
EncScript_Regieleki_ApplyTierStats_T1:
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	return
EncScript_Regieleki_ApplyTierStats_Done:
	return

// THE SINGLE WRITER of the damage reduction, outside Electro Drift's own one-time collapse.
EncScript_Regieleki_ApplyReduction:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Regieleki_ApplyReduction_90
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regieleki_ApplyReduction_85
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Regieleki_ApplyReduction_85:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Regieleki_ApplyReduction_90:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return

// --- Trigger entry points ---

// OnBattleStart. Seeds the PlayerCat/PlayerType sentinels so the very first turn's Power Surge
// check (unreachable until Phase 1, but cheap insurance) never reads a zeroed, non-sentinel
// value. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Regieleki_Intro::
	encsetvar 10, 3     @ PlayerCat sentinel: DAMAGE_CATEGORY_STATUS
	encsetvar 11, 10    @ PlayerType sentinel: TYPE_MYSTERY
	return

// Electric Terrain's Charge tick. Gated on TurnStartGuard but never writes it - TurnTally (lower
// priority number, same checkpoint, same first pass) is what closes it for both triggers.
EncScript_Regieleki_TerrainCharge::
	encaddvar 0, 1
	call EncScript_Regieleki_ClampCharge
	return

// Top of every turn: the turn-passes Charge tick, the "Regieleki moves first" bonus, and the
// once-per-turn scaffold's opening half.
EncScript_Regieleki_TurnTally::
	flushtextbox
	encsetvar 3, 1      @ TurnStartGuard - closes this and TerrainCharge
	encaddvar 0, 1       @ a turn passes
	enccomparestat ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 8
	encjumpifvar CMP_NOT_EQUAL, 8, 2, EncScript_Regieleki_TurnTally_Clamp
	encaddvar 0, 1       @ Regieleki is faster this turn
EncScript_Regieleki_TurnTally_Clamp:
	call EncScript_Regieleki_ClampCharge
// The recurring cue (guideline 14): which band is live, every turn it's live.
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Regieleki_TurnTally_Overcharge
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regieleki_TurnTally_Rising
	return
EncScript_Regieleki_TurnTally_Overcharge:
	printstring STRINGID_ENCREGIELEKIOVERCHARGE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regieleki_TurnTally_Rising:
	printstring STRINGID_ENCREGIELEKICHARGERISING
	waitmessage B_WAIT_TIME_SHORT
	return

// Regieleki's own attack connects. Costs it 2 Charge once Grid Expansion has doubled accrual
// (Phase >= 1), 1 otherwise.
EncScript_Regieleki_BossAttacked::
	encsetvar 4, 1      @ MoveGuard
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Regieleki_BossAttacked_P0
	encaddvar 0, 2
	call EncScript_Regieleki_ClampCharge
	return
EncScript_Regieleki_BossAttacked_P0:
	encaddvar 0, 1
	call EncScript_Regieleki_ClampCharge
	return

// An Electric move lands on Regieleki - absorbed into more Charge rather than resisted.
EncScript_Regieleki_ElectricHit::
	encsetvar 5, 1      @ ElecGuard
	encaddvar 0, 1
	call EncScript_Regieleki_ClampCharge
	return

// The player switches - information the grid reads as activity.
EncScript_Regieleki_PlayerSwitched::
	encsetvar 6, 1      @ SwitchGuard
	encaddvar 0, 1
	call EncScript_Regieleki_ClampCharge
	return

// A damaging hit lands on Regieleki - the only brake on the loop.
EncScript_Regieleki_PlayerHitBoss_Physical::
	encsetvar 10, 1     @ PlayerCat: DAMAGE_CATEGORY_PHYSICAL
	encsetvar 7, 1      @ HitGuard
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regieleki_PlayerHitBoss_Physical_Done
	encsubvar 0, 1
EncScript_Regieleki_PlayerHitBoss_Physical_Done:
	return

EncScript_Regieleki_PlayerHitBoss_Special::
	encsetvar 10, 2     @ PlayerCat: DAMAGE_CATEGORY_SPECIAL
	encsetvar 7, 1      @ HitGuard
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regieleki_PlayerHitBoss_Special_Done
	encsubvar 0, 1
EncScript_Regieleki_PlayerHitBoss_Special_Done:
	return

// Records whether the hit that just landed was Ground-type, for the Power Surge check.
EncScript_Regieleki_PlayerHitBoss_Ground::
	encsetvar 11, 5     @ PlayerType: TYPE_GROUND
	encsetvar 12, 1     @ TypeGuard
	return

EncScript_Regieleki_PlayerHitBoss_OtherType::
	encsetvar 11, 10    @ PlayerType sentinel: TYPE_MYSTERY (not Ground)
	encsetvar 12, 1     @ TypeGuard
	return

// GRID EXPANSION - 50% HP. Accrual from Regieleki's own attacks doubles from here on
// (EncScript_Regieleki_BossAttacked reads Phase); the Power Surge window opens immediately.
EncScript_Regieleki_GridExpansionEnter::
	encsetvar 1, 1      @ Phase
	encsetvar 9, 1      @ SurgeWindow
	encaddvar 0, 2
	call EncScript_Regieleki_ClampCharge
	printstring STRINGID_ENCREGIELEKISURGING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// The Power Surge window resolves at the end of the turn it opened on. Grounded beats mitigated
// beats full, in that order.
EncScript_Regieleki_SurgeResolve::
	encsetvar 9, 0      @ SurgeWindow closes regardless of outcome
	encjumpifvar CMP_NOT_EQUAL, 11, 5, EncScript_Regieleki_SurgeResolve_CheckHit   @ 5 = TYPE_GROUND
	printstring STRINGID_ENCREGIELEKISURGEGROUNDED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return
EncScript_Regieleki_SurgeResolve_CheckHit:
	encjumpifvar CMP_EQUAL, 10, 3, EncScript_Regieleki_SurgeResolve_Full   @ 3 = DAMAGE_CATEGORY_STATUS sentinel
	printstring STRINGID_ENCREGIELEKISURGEHITS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Regieleki_SurgeResolve_Full:
	printstring STRINGID_ENCREGIELEKISURGEHITS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	return

// --- Overcharge tier ladder (OnTurnEnd) ---

EncScript_Regieleki_TierUp1::
	encsetvar 2, 1      @ Tier
	call EncScript_Regieleki_ApplyTierStats
	call EncScript_Regieleki_ApplyReduction
	printstring STRINGID_ENCREGIELEKIOVERCHARGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Regieleki_TierUp2::
	encsetvar 2, 2      @ Tier
	call EncScript_Regieleki_ApplyTierStats
	call EncScript_Regieleki_ApplyReduction
	printstring STRINGID_ENCREGIELEKIOVERCHARGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Regieleki_TierDown::
	encsetvar 2, 0      @ Tier - ApplyTierStats/ApplyReduction write absolutely, so dropping
	call EncScript_Regieleki_ApplyTierStats   @ straight to 0 from either tier lands correctly
	call EncScript_Regieleki_ApplyReduction
	printstring STRINGID_ENCREGIELEKISTABILIZES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return

// Tier 2's bonus burst: a chance each turn to fire again, at the cost of feeding Charge further.
// FlashStepGuard closes on both the roll and the miss - Tier stays at 2 either way, so without
// its own latch this re-selects every re-evaluation pass at OnTurnEnd until the runaway guard
// trips.
EncScript_Regieleki_FlashStep::
	encsetvar 13, 1     @ FlashStepGuard
	encjumpifchance 40, EncScript_Regieleki_FlashStep_Fire
	return
EncScript_Regieleki_FlashStep_Fire:
	printstring STRINGID_ENCREGIELEKIFLASHSTEP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encaddvar 0, 2
	call EncScript_Regieleki_ClampCharge
	return

// CRITICAL OVERLOAD - 15% HP. Charge forced to max, Overcharge locked at Tier 2.
EncScript_Regieleki_CriticalOverloadEnter::
	encsetvar 1, 2      @ Phase
	encsetvar 0, 10     @ Charge forced to max
	encsetvar 2, 2       @ Tier locked
	call EncScript_Regieleki_ApplyTierStats
	call EncScript_Regieleki_ApplyReduction
	printstring STRINGID_ENCREGIELEKICRITICALOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// ELECTRO DRIFT - fires once Critical Overload has forced Charge to 10, then the grid collapses.
// The intended catch window: reduction craters to 60 and stays there (this is the only script
// that writes damage reduction after Phase reaches 2 - ApplyReduction is not called again).
EncScript_Regieleki_ElectroDrift::
	printstring STRINGID_ENCREGIELEKIELECTRODRIFT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -24, ENC_AMOUNT_PERCENT
	encsetvar 0, 0      @ Charge collapses
	encsetvar 2, 0      @ Tier reverts
	call EncScript_Regieleki_ApplyTierStats
	encsetdamagereduction ENC_TARGET_BOSS, 90
	printstring STRINGID_ENCREGIELEKICOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

// Balls unlock at 10% HP (guideline 10). No Phase gate in the trigger table, so this can in
// principle beat Electro Drift's COLLAPSE line to the punch on an overkill hit - it carries its
// own short "now is the moment" line for that case.
EncScript_Regieleki_BallsUnlock::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	encsetsurvive ENC_TARGET_BOSS, FALSE
	printstring STRINGID_ENCREGIELEKIBALLSUNLOCK
	waitmessage B_WAIT_TIME_LONG
	return

// The once-per-turn scaffold's closing half, and the sentinel reset for a turn where nothing
// landed on Regieleki at all.
EncScript_Regieleki_ClearGuards::
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Regieleki_ClearGuards_Type
	encsetvar 10, 3     @ PlayerCat sentinel: DAMAGE_CATEGORY_STATUS
EncScript_Regieleki_ClearGuards_Type:
	encjumpifvar CMP_NOT_EQUAL, 12, 0, EncScript_Regieleki_ClearGuards_Zero
	encsetvar 11, 10    @ PlayerType sentinel: TYPE_MYSTERY
EncScript_Regieleki_ClearGuards_Zero:
	encsetvar 3, 0      @ TurnStartGuard
	encsetvar 4, 0      @ MoveGuard
	encsetvar 5, 0      @ ElecGuard
	encsetvar 6, 0      @ SwitchGuard
	encsetvar 7, 0      @ HitGuard
	encsetvar 12, 0     @ TypeGuard
	encsetvar 13, 0     @ FlashStepGuard
	return
