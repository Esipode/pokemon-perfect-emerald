// Chien-Pao, "The Ruinous Sword Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_ChienPao_Init:
// 0 Momentum (0-6), 1 Phase (0 Hunt / 1 Whiteout / 2 Avalanche), 2 ChainCount, 3 DashState,
// 4 BladeCharge, 5 HPMark, 6 BladeHPMark, 7 SpeedCompare, 8 MoveHandled, 9 SwitchHandled,
// 10 BattleStarted, 11 TurnGuard.
//
// MOMENTUM IS CLAMPED THROUGH TWO SHARED SUBROUTINES, never a bare encaddvar/encsubvar on Var 0.
// Var 0 is a u8: an unclamped encsubvar at Momentum == 0 wraps to 255, which would read as
// permanently "high Momentum" to every >= gate in this file and never recover. MomentumUp/Down are
// the only writers, and a delta of 2 is two calls rather than a raw add/sub of 2.
//
// EVERY OnTurnStart TRIGGER IS SELF-DISABLING. TurnSetup uses the TurnGuard scaffold (closed as its
// first line); DashDeclare/BladeDeclare disable themselves naturally by setting DashState/
// BladeCharge to 1, which is also their own gating condition.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

EncScript_ChienPao_MomentumUp:
	encjumpifvar CMP_EQUAL, 0, 6, EncScript_ChienPao_MomentumUp_Done
	encaddvar 0, 1
EncScript_ChienPao_MomentumUp_Done:
	return

EncScript_ChienPao_MomentumDown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_ChienPao_MomentumDown_Done
	encsubvar 0, 1
EncScript_ChienPao_MomentumDown_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_ChienPao_Init::
	printstring STRINGID_ENCCHIENPAOTEARS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	encsetvar 10, 1    @ BattleStarted
	return

// Top of every turn. TurnGuard is closed first so no branch out can leave it re-armed. The speed
// check feeds Momentum directly: Chien-Pao moving first is itself aggression.
EncScript_ChienPao_TurnSetup::
	encsetvar 11, 1    @ TurnGuard
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 5, ENC_SNAP_SET   @ HPMark
	encsetvar 8, 0     @ MoveHandled
	encsetvar 9, 0     @ SwitchHandled
	enccomparestat ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 7   @ SpeedCompare
	encjumpifvar CMP_NOT_EQUAL, 7, 2, EncScript_ChienPao_TurnSetup_Done   @ 2 = boss faster
	call EncScript_ChienPao_MomentumUp
EncScript_ChienPao_TurnSetup_Done:
	return

// SWORD DASH telegraph (Phase 0, Momentum >= 4). Paid off by MoveHandler on a landed hit, or
// punished by DashBreak at OnTurnEnd on a miss/Protect.
EncScript_ChienPao_DashDeclare::
	encsetvar 3, 1     @ DashState
	printstring STRINGID_ENCCHIENPAODASHVANISH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

// DashState still 1 at OnTurnEnd means the turn ended without MoveHandler's Dash payoff branch
// running - a miss or a Protect. Always clears DashState on its one path.
EncScript_ChienPao_DashBreak::
	encsetvar 3, 0     @ DashState
	call EncScript_ChienPao_MomentumDown
	call EncScript_ChienPao_MomentumDown
	printstring STRINGID_ENCCHIENPAODASHBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// RUINOUS BLADE telegraph (Phase 1, Momentum >= 5). Snapshots the boss's own HP so BladeResolve can
// tell whether the player interrupted it with a real hit.
EncScript_ChienPao_BladeDeclare::
	encsetvar 4, 1     @ BladeCharge
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET   @ BladeHPMark
	printstring STRINGID_ENCCHIENPAOBLADECHARGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

// BladeResolve. Jumping to _Release means no damage was recorded on the boss since BladeHPMark -
// the player did NOT land a big enough hit to interrupt. Falling through (damage was recorded)
// is the interrupt path. Both paths clear BladeCharge on their own way out.
EncScript_ChienPao_BladeResolve::
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_DAMAGE, EncScript_ChienPao_BladeResolve_Release
	call EncScript_ChienPao_MomentumDown
	call EncScript_ChienPao_MomentumDown
	printstring STRINGID_ENCCHIENPAOBLADEINTERRUPT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetvar 4, 0     @ BladeCharge
	return
EncScript_ChienPao_BladeResolve_Release:
	encclearscreens ENC_TARGET_PLAYER_LEFT, EncScript_ChienPao_BladeResolve_NoScreens
EncScript_ChienPao_BladeResolve_NoScreens:
	printstring STRINGID_ENCCHIENPAOBLADERELEASE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -22, ENC_AMOUNT_PERCENT
	call EncScript_ChienPao_MomentumUp
	call EncScript_ChienPao_MomentumUp
	encsetvar 4, 0     @ BladeCharge
	return

// Player switched under the Hunt. Costs Momentum - "switching while Hunted" is the tension.
EncScript_ChienPao_SwitchHandler::
	encsetvar 9, 1     @ SwitchHandled
	call EncScript_ChienPao_MomentumUp
	printstring STRINGID_ENCCHIENPAOSWITCHPRESSURE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// THE MOVE HANDLER. Guard closed first, before any branch, so both the hit and miss paths close
// their own latch. encsnapshothp jumping to _Miss means no damage was recorded on the player since
// HPMark - Chien-Pao's hit missed or was blocked this turn.
EncScript_ChienPao_MoveHandler::
	encsetvar 8, 1     @ MoveHandled
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 5, ENC_SNAP_DAMAGE, EncScript_ChienPao_MoveHandler_Miss
	call EncScript_ChienPao_MomentumUp        @ attacked
	encaddvar 2, 1                            @ ChainCount
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_ChienPao_MoveHandler_Opening
	call EncScript_ChienPao_MomentumUp        @ consecutive hits
EncScript_ChienPao_MoveHandler_Opening:
	encjumpifchance 15, EncScript_ChienPao_MoveHandler_OpeningHit
	goto EncScript_ChienPao_MoveHandler_Chip
EncScript_ChienPao_MoveHandler_OpeningHit:
	call EncScript_ChienPao_MomentumUp
	printstring STRINGID_ENCCHIENPAOOPENING
	waitmessage B_WAIT_TIME_LONG
EncScript_ChienPao_MoveHandler_Chip:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -8, ENC_AMOUNT_PERCENT   @ the Hunted chip - always on
	encjumpifvar CMP_NOT_EQUAL, 3, 1, EncScript_ChienPao_MoveHandler_Whiteout
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT  @ Sword Dash payoff
	call EncScript_ChienPao_MomentumUp
	call EncScript_ChienPao_MomentumUp
	encsetvar 3, 0     @ DashState
EncScript_ChienPao_MoveHandler_Whiteout:
	encjumpifvar CMP_NOT_EQUAL, 1, 1, EncScript_ChienPao_MoveHandler_Avalanche   @ Phase != 1
	encjumpifvar CMP_NOT_EQUAL, 0, 6, EncScript_ChienPao_MoveHandler_Avalanche  @ Momentum != 6
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -18, ENC_AMOUNT_PERCENT  @ Whiteout bonus spike - self-costs
	call EncScript_ChienPao_MomentumDown
	call EncScript_ChienPao_MomentumDown
EncScript_ChienPao_MoveHandler_Avalanche:
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_ChienPao_MoveHandler_Done   @ Phase != 2
	encjumpifvar CMP_GREATER_THAN, 2, 8, EncScript_ChienPao_MoveHandler_Chain30
	encjumpifvar CMP_EQUAL, 2, 8, EncScript_ChienPao_MoveHandler_Chain28
	encjumpifvar CMP_EQUAL, 2, 7, EncScript_ChienPao_MoveHandler_Chain25
	encjumpifvar CMP_EQUAL, 2, 6, EncScript_ChienPao_MoveHandler_Chain22
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_ChienPao_MoveHandler_Chain19
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_ChienPao_MoveHandler_Chain16
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_ChienPao_MoveHandler_Chain13
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_ChienPao_MoveHandler_Chain10
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_ChienPao_MoveHandler_Chain7
	playanimation BS_PLAYER1, B_ANIM_MON_HIT   @ ChainCount == 0: -4
	encchangehp ENC_TARGET_PLAYER_LEFT, -4, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain7:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -7, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain10:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain13:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -13, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain16:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain19:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -19, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain22:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -22, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain25:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -25, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain28:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
	goto EncScript_ChienPao_MoveHandler_Done
EncScript_ChienPao_MoveHandler_Chain30:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
EncScript_ChienPao_MoveHandler_Done:
	return
// The miss path. ChainCount resets to 0 regardless of phase, so Avalanche never inherits an
// inflated chain from an earlier phase. DashState/BladeCharge are deliberately left untouched -
// DashBreak/BladeResolve at OnTurnEnd own that punishment.
EncScript_ChienPao_MoveHandler_Miss:
	encsetvar 2, 0     @ ChainCount
	return

// --- Phase transitions ---

// 50% HP - WHITEOUT. Permanent raw Speed boost, not a stage, so it survives Haze/switching.
EncScript_ChienPao_EnterWhiteout::
	encsetvar 1, 1     @ Phase
	printstring STRINGID_ENCCHIENPAOWHITEOUT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 40, ENC_AMOUNT_PERCENT
	return

// 15% HP - AVALANCHE. Momentum maxed and the chain reset so the first hit of the phase reads as
// ChainCount == 0. Survive: True guarantees the catch window opens.
EncScript_ChienPao_EnterAvalanche::
	encsetvar 1, 2     @ Phase
	encsetvar 0, 6     @ Momentum
	encsetvar 2, 0     @ ChainCount
	encsetsurvive ENC_TARGET_BOSS, TRUE
	printstring STRINGID_ENCCHIENPAOAVALANCHE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	return

// 10% HP - the catch window (guideline 10). Survive clears before Balls are allowed, so the
// automatic catch-window damage guard takes over cleanly.
EncScript_ChienPao_FinalOpening::
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 25
	printstring STRINGID_ENCCHIENPAOFINALOPENING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

// --- Momentum band flavor (guideline 14, first-crossing only - see the plan's known gaps) ---

EncScript_ChienPao_MomentumBrisk::
	printstring STRINGID_ENCCHIENPAOMOMENTUMBRISK
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_ChienPao_MomentumBlizzard::
	printstring STRINGID_ENCCHIENPAOMOMENTUMBLIZZARD
	waitmessage B_WAIT_TIME_LONG
	return

// Closes the once-per-turn scaffold TurnSetup opened.
EncScript_ChienPao_TurnClose::
	encsetvar 11, 0    @ TurnGuard
	return
