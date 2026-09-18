// Spectrier, "The Swift Horse Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Spectrier_Intro:
// 0 Phase (1-3), 1 Dread (0-6, the core meter), 2 Trail (0-3), 3 Fear (0/1), 4 FearSnap
// (Spectrier's HP% when Fear Mark was laid), 5 ChargeStage (0 idle / 1 telegraph in progress),
// 6 Grudge (0-3), 7 NMStage (0 idle / 1 buildup done / 2 barrage resolved), 8 ChargeTimer,
// 9 TrickleTimer, 10 DreadGuard, 11 TurnGuard, 12 SwitchCount, 13 DisableArm, 14 LastTier
// (the announced-band latch, guideline 14), 15 Scratch (free scratch register).
//
// DREAD IS THE GUARD. EncScript_Spectrier_ApplyDreadTier is evasion's only writer and reads Dread
// and Trail absolutely (the Rayquaza idiom: push to -12, then land the exact target stage), so a
// Dread drop or a Trail purge undoes what an earlier rise wrote with no separate teardown path.
// EncScript_Spectrier_DreadUp/_DreadDown are Dread's only writers, clamped to 0-6 and always
// re-applying the tier afterward, so nothing can move Dread without the evasion staying in sync.
//
// EVERY EVENT-REACTING TRIGGER IS SELF-DISABLING through DreadGuard, which every Dread-source
// script sets unconditionally on its one path, and TurnGuard/DreadGuard both clear at the ledger.
// Shadow Charge resolution is two triggers pinned to a literal (Dark vs. everything else) rather
// than one script branching on Event.MoveType, which no script can read directly.
//
// Phase 3 (Nightmare) owns its own turn: the heartbeat and ledger both return early once
// Phase == 3, and ApplyDreadTier no-ops the same way, so nothing set up in Phases 1-2 can leak
// evasion or timers into the buildup/barrage sequence.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of Spectrier's evasion. Base stage by Dread tier (0/1/2/2/2/3/3 for Dread
// 0-6), plus Trail's stacks on top once Dread >= 3 (guideline 9's balance dial - flagged as an
// open review item in the plan, section 9). Phase 3 returns immediately: the Nightmare buildup
// sets its own evasion directly.
EncScript_Spectrier_ApplyDreadTier::
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Spectrier_ApplyDreadTier_Skip
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Spectrier_ApplyDreadTier_T0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Spectrier_ApplyDreadTier_T1
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Spectrier_ApplyDreadTier_T2
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 9    @ base 3 (Dread 5-6)
	goto EncScript_Spectrier_ApplyDreadTier_Trail
EncScript_Spectrier_ApplyDreadTier_T0:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6    @ base 0
	goto EncScript_Spectrier_ApplyDreadTier_Band
EncScript_Spectrier_ApplyDreadTier_T1:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 7    @ base 1
	goto EncScript_Spectrier_ApplyDreadTier_Band
EncScript_Spectrier_ApplyDreadTier_T2:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 8    @ base 2 (Dread 2-4)
EncScript_Spectrier_ApplyDreadTier_Trail:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Spectrier_ApplyDreadTier_Band
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Spectrier_ApplyDreadTier_Trail1
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Spectrier_ApplyDreadTier_Trail2
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 3
	goto EncScript_Spectrier_ApplyDreadTier_Band
EncScript_Spectrier_ApplyDreadTier_Trail2:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 2
	goto EncScript_Spectrier_ApplyDreadTier_Band
EncScript_Spectrier_ApplyDreadTier_Trail1:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 1
EncScript_Spectrier_ApplyDreadTier_Band:
	call EncScript_Spectrier_Band
EncScript_Spectrier_ApplyDreadTier_Skip:
	return

// THE DREAD BAND CALLOUT (guideline 14). LastTier holds the last ANNOUNCED band; each arm
// compares it against its own literal and stays quiet if that's what was last said. Dread 0
// prints nothing (returning to baseline needs no line) and Dread 4 has its own dedicated line at
// the move-lock itself, so this covers bands 1, 2, 3 and 5.
EncScript_Spectrier_Band:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Spectrier_Band_T0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Spectrier_Band_T1
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Spectrier_Band_T3
	encjumpifvar CMP_EQUAL, 14, 5, EncScript_Spectrier_Band_Done
	encsetvar 14, 5
	printstring STRINGID_ENCSPECTRIERDREADTIER5
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Spectrier_Band_T0:
	encsetvar 14, 0
	return
EncScript_Spectrier_Band_T1:
	encjumpifvar CMP_EQUAL, 14, 1, EncScript_Spectrier_Band_Done
	encsetvar 14, 1
	printstring STRINGID_ENCSPECTRIERDREADTIER1
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Spectrier_Band_T3:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Spectrier_Band_T2
	encjumpifvar CMP_EQUAL, 14, 3, EncScript_Spectrier_Band_Done
	encsetvar 14, 3
	printstring STRINGID_ENCSPECTRIERDREADTIER3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Spectrier_Band_T2:
	encjumpifvar CMP_EQUAL, 14, 2, EncScript_Spectrier_Band_Done
	encsetvar 14, 2
	printstring STRINGID_ENCSPECTRIERDREADTIER2
	waitmessage B_WAIT_TIME_LONG
EncScript_Spectrier_Band_Done:
	return

// Dread's only writers, clamped 0-6, always re-applying the tier so evasion never drifts out of
// sync with the meter.
EncScript_Spectrier_DreadUp:
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Spectrier_DreadUp_Done
	encaddvar 1, 1
	call EncScript_Spectrier_ApplyDreadTier
EncScript_Spectrier_DreadUp_Done:
	return

EncScript_Spectrier_DreadDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Spectrier_DreadDown_Done
	encsubvar 1, 1
	call EncScript_Spectrier_ApplyDreadTier
EncScript_Spectrier_DreadDown_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Spectrier_Intro::
	printstring STRINGID_ENCSPECTRIERINTRO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	encsetvar 9, 2     @ TrickleTimer
	encsetvar 8, 5     @ ChargeTimer
	encsetvar 14, 9    @ LastTier: nothing announced yet
	return

// Top of every turn: ticks ChargeTimer and fires the Shadow Charge telegraph, then handles the
// Dread-4 move-lock re-arm. Phase 3 returns immediately - Nightmare owns the turn.
EncScript_Spectrier_Heartbeat::
	flushtextbox
	encsetvar 11, 1    @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Spectrier_Heartbeat_Done
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Spectrier_Heartbeat_Telegraph
	encsubvar 8, 1
	goto EncScript_Spectrier_Heartbeat_Disable
EncScript_Spectrier_Heartbeat_Telegraph:
	printstring STRINGID_ENCSPECTRIERCHARGETELEGRAPH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encsetvar 5, 1     @ ChargeStage
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Spectrier_Heartbeat_TelegraphP1
	encsetvar 8, 3     @ Phase 2+: tighter cadence
	goto EncScript_Spectrier_Heartbeat_Disable
EncScript_Spectrier_Heartbeat_TelegraphP1:
	encsetvar 8, 5
EncScript_Spectrier_Heartbeat_Disable:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Spectrier_Heartbeat_Done
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Spectrier_Heartbeat_Done
	encdisablemove ENC_TARGET_PLAYER_LEFT, 3, EncScript_Spectrier_Heartbeat_Done
	printstring STRINGID_ENCSPECTRIERDREADTIER4
	waitmessage B_WAIT_TIME_LONG
	encsetvar 13, 4    @ DisableArm
EncScript_Spectrier_Heartbeat_Done:
	return

// A faint is information. DreadGuard caps this to Spectrier's one Dread gain per turn.
EncScript_Spectrier_FaintDread::
	call EncScript_Spectrier_DreadUp
	encsetvar 10, 1    @ DreadGuard
	return

// A repeated switch is information too - the 2nd+ switch-in this window feeds Dread. DreadGuard
// is set as the first line, before anything else, so every path closes this trigger's own latch.
EncScript_Spectrier_SwitchDread::
	encsetvar 10, 1    @ DreadGuard
	encjumpifvar CMP_EQUAL, 12, 3, EncScript_Spectrier_SwitchDread_Guard
	encaddvar 12, 1    @ SwitchCount
EncScript_Spectrier_SwitchDread_Guard:
	encjumpifvar CMP_LESS_THAN, 12, 2, EncScript_Spectrier_SwitchDread_Done
	call EncScript_Spectrier_DreadUp
EncScript_Spectrier_SwitchDread_Done:
	return

// Defensive/status play feeds Dread.
EncScript_Spectrier_StatusDread::
	call EncScript_Spectrier_DreadUp
	encsetvar 10, 1
	return

// Spectrier's own Ghost hits landing feed Dread - it grows more sure of itself.
EncScript_Spectrier_GhostHitDread::
	call EncScript_Spectrier_DreadUp
	encsetvar 10, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// A themed Dark hit purges one Shadow Trail stack and raises Grudge instead of "destroying a
// marker on a board" (guideline 3's substitute for the idea's positioning system). DreadGuard is
// set as the first line, before anything else, so every path - including a full board - closes
// this trigger's own latch (a Grudge already at 3 has no other branch to fall through).
EncScript_Spectrier_TrailPurge::
	encsetvar 10, 1    @ DreadGuard
	encsubvar 2, 1     @ Trail
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Spectrier_TrailPurge_Apply
	encaddvar 6, 1     @ Grudge
EncScript_Spectrier_TrailPurge_Apply:
	call EncScript_Spectrier_ApplyDreadTier
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

// Shadow Charge, grounded: the player answered with the themed Dark move the telegraph called
// for. Small chip, Dread eases back - the reward for reading the tell correctly.
EncScript_Spectrier_ChargeGrounded::
	encsetvar 5, 0     @ ChargeStage
	printstring STRINGID_ENCSPECTRIERCHARGEGROUNDED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
	call EncScript_Spectrier_DreadDown
	return

// Shadow Charge, full hit: any other answer eats the charge. Grudge (Phase 2+) scales the payload
// up to the guideline-7 ceiling.
EncScript_Spectrier_ChargeFull::
	encsetvar 5, 0     @ ChargeStage
	printstring STRINGID_ENCSPECTRIERCHARGEFULLHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Spectrier_ChargeFull_20
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Spectrier_ChargeFull_25
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Spectrier_ChargeFull_30
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Spectrier_ChargeFull_Up
EncScript_Spectrier_ChargeFull_20:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Spectrier_ChargeFull_Up
EncScript_Spectrier_ChargeFull_25:
	encchangehp ENC_TARGET_PLAYER_LEFT, -25, ENC_AMOUNT_PERCENT
	goto EncScript_Spectrier_ChargeFull_Up
EncScript_Spectrier_ChargeFull_30:
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
EncScript_Spectrier_ChargeFull_Up:
	call EncScript_Spectrier_DreadUp
	return

// Fear Mark lifts once real damage has landed on Spectrier since it was laid. Scratch carries the
// baseline into the snapshot so FearSnap itself survives being measured, the same idiom the
// ledger uses for BossMark. DreadGuard is set as the very first line so the below-threshold path
// (which touches no other variable this trigger's condition reads) still closes the latch.
EncScript_Spectrier_FearRemove::
	encsetvar 10, 1    @ DreadGuard
	enccopyvar 15, 4
	encsnapshothp ENC_TARGET_BOSS, 15, ENC_SNAP_DAMAGE, EncScript_Spectrier_FearRemove_None
	encjumpifvar CMP_LESS_THAN, 15, 3, EncScript_Spectrier_FearRemove_None
	printstring STRINGID_ENCSPECTRIERFEARREMOVED
	waitmessage B_WAIT_TIME_LONG
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	encsetvar 3, 0     @ Fear
	return
EncScript_Spectrier_FearRemove_None:
	return

// THE LEDGER: ambient Dread trickle, Shadow Trail growth, Fear Mark application, DisableArm
// countdown, SwitchCount decay. Self-disables by clearing TurnGuard/DreadGuard. Phase 3 returns
// immediately - Nightmare owns the turn.
EncScript_Spectrier_Ledger::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Spectrier_Ledger_Done
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Spectrier_Ledger_Trickle
	encsubvar 9, 1     @ TrickleTimer
	goto EncScript_Spectrier_Ledger_Disable
EncScript_Spectrier_Ledger_Trickle:
	encsetvar 9, 2
	call EncScript_Spectrier_DreadUp
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Spectrier_Ledger_TrickleFear
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Spectrier_Ledger_TrickleFear
	encaddvar 2, 1     @ Trail grows
	call EncScript_Spectrier_ApplyDreadTier
EncScript_Spectrier_Ledger_TrickleFear:
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Spectrier_Ledger_Disable
	printstring STRINGID_ENCSPECTRIERFEARAPPLIED
	waitmessage B_WAIT_TIME_LONG
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encsetvar 3, 1     @ Fear
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET
EncScript_Spectrier_Ledger_Disable:
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Spectrier_Ledger_Switch
	encsubvar 13, 1    @ DisableArm
EncScript_Spectrier_Ledger_Switch:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Spectrier_Ledger_Done
	encsubvar 12, 1    @ SwitchCount
EncScript_Spectrier_Ledger_Done:
	encsetvar 10, 0    @ DreadGuard
	encsetvar 11, 0    @ TurnGuard
	return

// --- Phase transitions ---

// 50% - SOUL STEED. Faster and hungrier: a raw Speed bump (permanent, invisible, and the right
// tool since a stage would be undone by a switch) plus a tighter Charge cadence.
EncScript_Spectrier_Phase2::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCSPECTRIERPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE_INSTANT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 20, ENC_AMOUNT_PERCENT
	encsetvar 8, 3     @ ChargeTimer tightens immediately
	return

// <=25% HP or Dread==6 - NIGHTMARE RIDER. Forces Dread to 6 and tears down every Phase 1-2
// mechanic (Trail, Fear's trap, Charge, the move-lock) so nothing leaks into the buildup turn.
// The buildup itself is the one evasion spike this phase owns outright - no attack this turn, a
// strong tell instead (guideline 9, reused animation only).
EncScript_Spectrier_Phase3::
	encsetvar 0, 3     @ Phase
	encsetvar 1, 6     @ Dread forced to max
	encsetvar 2, 0     @ Trail
	encsetvar 3, 0     @ Fear
	encsetvar 5, 0     @ ChargeStage
	encsetvar 6, 0     @ Grudge
	encsetvar 7, 1     @ NMStage: buildup
	encsetvar 13, 0    @ DisableArm
	printstring STRINGID_ENCSPECTRIERPHASE3
	waitmessage B_WAIT_TIME_LONG
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Spectrier_Phase3_Evasion
EncScript_Spectrier_Phase3_Evasion:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 8    @ +2 stage, the buildup tell
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// The buildup turn resolves into one unavoidable Astral Barrage, then Spectrier drops its guard:
// stats reset (the buildup evasion included) and it becomes vulnerable, per guideline 10.
EncScript_Spectrier_Barrage::
	encsetvar 7, 2     @ NMStage: resolved
	printstring STRINGID_ENCSPECTRIERBARRAGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -25, ENC_AMOUNT_PERCENT
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Spectrier_Barrage_Done
EncScript_Spectrier_Barrage_Done:
	return

// <=10% HP in Phase 3 - the catch window (guideline 10).
EncScript_Spectrier_BallUnlock::
	printstring STRINGID_ENCSPECTRIERBALLUNLOCK
	waitmessage B_WAIT_TIME_LONG
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 45
	return
