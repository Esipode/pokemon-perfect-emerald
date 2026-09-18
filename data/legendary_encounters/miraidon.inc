// Miraidon, "The Iron Serpent Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Miraidon_Init:
// 0 Phase (0 Network / 1 Future Engine / 2 Paradox Collapse), 1 Analysis (0-6), 2 Nodes (0-3),
// 3 PredictCat (encstoreprediction result: 0 none/1 phys/2 spec/3 status), 4 PredictActive,
// 5 LastReduction (guarded/exposed callout latch, guideline 14), 6 OverclockTurns (0-3),
// 7 ErrorCount (0-3), 8 ErrorActive (0-3), 9 ScanStat (0/STAT_ATK/STAT_SPATK),
// 10 ScanTurns (0-3), 11 LastMoveType (reserved, unused - see the .encounter header's note on the
// type-repeat substitution), 12 LastCategory (0 unset/1 physical/2 special), 13 TurnGuard,
// 14 MoveGuard (2-bit mask: bit0 Analyze group, bit1 NodeDestroy), 15 SnapVar (0-100).
//
// EVERY OnMoveEnd TRIGGER THAT ISN'T Once/OnEnter CLOSES ITS OWN LATCH, because Event.* stays true
// for the whole checkpoint dispatch and a script can't move it (the re-evaluation loop just
// reselects the same trigger until the 8-scripts-per-checkpoint guard trips). AnalyzeResisted/
// Physical/Special and NodeDestroy share one guard var packed as two bits rather than costing two
// full variables - each script only ever adds the power-of-two for the bit it owns, which is safe
// precisely because its own trigger condition already proved that bit was clear.
//
// ANALYSIS METER GROWS FROM THREE INDEPENDENT FACTS about the same hit (resisted type / repeated
// category / correct prediction) but Event.* can only be read in a trigger's Conditions, never
// inside a script - so resisted-type and repeated-category are necessarily separate triggers, and
// only the higher-priority one fires per hit (this project's existing idiom - see
// genesect.encounter's three ANALYZE triggers for the same shape).
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// Analysis +1, clamped at 6.
EncScript_Miraidon_AnalysisUp1:
	encaddvar 1, 1
	encjumpifvar CMP_LESS_THAN, 1, 7, EncScript_Miraidon_AnalysisUp1_Done
	encsetvar 1, 6
EncScript_Miraidon_AnalysisUp1_Done:
	return

// Analysis -1, floored at 0.
EncScript_Miraidon_AnalysisDown1:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Miraidon_AnalysisDown1_Done
	encsubvar 1, 1
EncScript_Miraidon_AnalysisDown1_Done:
	return

// Analysis -2, floored at 0.
EncScript_Miraidon_AnalysisDown2:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Miraidon_AnalysisDown2_Zero
	encsubvar 1, 2
	return
EncScript_Miraidon_AnalysisDown2_Zero:
	encsetvar 1, 0
	return

// --- Trigger entry points ---

// OnBattleStart. Electric Terrain is permanent from turn 0, which is what makes Hadron Engine pay
// off without any extra scripting. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Miraidon_Init::
	printstring STRINGID_ENCMIRAIDONNETWORKONLINE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 2, 1     @ Nodes
	encsetvar 5, 1     @ LastReduction: guarded
	encsetterrain ENC_TERRAIN_ELECTRIC
	encsnapshothp ENC_TARGET_BOSS, 15, ENC_SNAP_SET
	return

// Top of every turn. TurnGuard is set immediately so no branch out can leave it re-armed. CALCULATING
// is armed here (once Analysis >= 3) and resolved at OnMoveEnd by PredictHit/PredictMiss.
EncScript_Miraidon_TurnOpen::
	encsetvar 13, 1    @ TurnGuard
	encsetvar 3, 0     @ PredictCat
	encsetvar 4, 0     @ PredictActive
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Miraidon_TurnOpen_Done
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 3
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Miraidon_TurnOpen_Done
	encsetvar 4, 1     @ PredictActive
EncScript_Miraidon_TurnOpen_Done:
	return

EncScript_Miraidon_CalculatingOn::
	printstring STRINGID_ENCMIRAIDONCALCULATING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Miraidon_CalculatingOff::
	printstring STRINGID_ENCMIRAIDONIDLE
	waitmessage B_WAIT_TIME_SHORT
	return

// A resisted type lands on Miraidon. Self-disables through MoveGuard bit 0 (+1) - safe unconditionally
// because the trigger's own condition already proved that bit was clear.
EncScript_Miraidon_AnalyzeResisted::
	encaddvar 14, 1    @ MoveGuard bit0
	call EncScript_Miraidon_AnalysisUp1
	printstring STRINGID_ENCMIRAIDONANALYSISRISE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// A Physical hit lands. Records the category and, if it matches last turn's, feeds Analysis - the
// pattern the player is meant to notice and break.
EncScript_Miraidon_AnalyzePhysical::
	encaddvar 14, 1    @ MoveGuard bit0
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Miraidon_AnalyzePhysical_Repeat
	encsetvar 12, 1    @ LastCategory: physical
	return
EncScript_Miraidon_AnalyzePhysical_Repeat:
	call EncScript_Miraidon_AnalysisUp1
	printstring STRINGID_ENCMIRAIDONREPEATREAD
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Miraidon_AnalyzeSpecial::
	encaddvar 14, 1    @ MoveGuard bit0
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Miraidon_AnalyzeSpecial_Repeat
	encsetvar 12, 2    @ LastCategory: special
	return
EncScript_Miraidon_AnalyzeSpecial_Repeat:
	call EncScript_Miraidon_AnalysisUp1
	printstring STRINGID_ENCMIRAIDONREPEATREAD
	waitmessage B_WAIT_TIME_LONG
	return

// 5%+ of Miraidon's own max HP in one hit knocks a Node offline - the player's one lever against a
// meter that otherwise only climbs. SnapVar is re-marked on every path so a multi-hit move checks
// each hit against the HP left after the previous one, not the turn-open baseline.
EncScript_Miraidon_NodeDestroy::
	encaddvar 14, 2    @ MoveGuard bit1
	encsnapshothp ENC_TARGET_BOSS, 15, ENC_SNAP_DAMAGE, EncScript_Miraidon_NodeDestroy_Remark
	encjumpifvar CMP_LESS_THAN, 15, 5, EncScript_Miraidon_NodeDestroy_Remark
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Miraidon_NodeDestroy_Remark
	encsubvar 2, 1     @ Nodes
	call EncScript_Miraidon_AnalysisDown1
	printstring STRINGID_ENCMIRAIDONNODEOFFLINE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
EncScript_Miraidon_NodeDestroy_Remark:
	encsnapshothp ENC_TARGET_BOSS, 15, ENC_SNAP_SET
	return

EncScript_Miraidon_MoveGuardClear::
	encsetvar 14, 0
	return

// The player's move resolves while CALCULATING is live and matches the predicted category: Miraidon
// hits back for a flat percentage. Self-disables through PredictActive, which both arms clear.
EncScript_Miraidon_PredictHit::
	encsetvar 4, 0     @ PredictActive
	call EncScript_Miraidon_AnalysisUp1
	printstring STRINGID_ENCMIRAIDONCALCCONFIRMED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	return

// The catch-all: CALCULATING was live and the category didn't match (mutually exclusive with
// PredictHit by priority - it runs first and clears PredictActive, which is this trigger's own
// condition too). Errors only count in Paradox Collapse, where System Failure punishes them.
EncScript_Miraidon_PredictMiss::
	encsetvar 4, 0     @ PredictActive
	call EncScript_Miraidon_AnalysisDown2
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Miraidon_PredictMiss_Print
	encaddvar 7, 1     @ ErrorCount
	encjumpifvar CMP_LESS_THAN, 7, 4, EncScript_Miraidon_PredictMiss_Print
	encsetvar 7, 3
EncScript_Miraidon_PredictMiss_Print:
	printstring STRINGID_ENCMIRAIDONMISCALCULATION
	waitmessage B_WAIT_TIME_LONG
	return

// THE LEDGER: passive Analysis trickle, SCAN start/tick, Overclock's residual chip, and the System
// Failure countdown - all folded into the one TurnGuard-gated script so none of them is a bare
// countdown trigger re-selecting itself within a single checkpoint pass. Self-disables by clearing
// TurnGuard last.
EncScript_Miraidon_TurnClose::
	encjumpifvar CMP_LESS_THAN, 0, 1, EncScript_Miraidon_TurnClose_Scan
	call EncScript_Miraidon_AnalysisUp1
EncScript_Miraidon_TurnClose_Scan:
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Miraidon_TurnClose_ScanTick
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Miraidon_TurnClose_Overclock
	encsetvar 10, 3    @ ScanTurns
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Miraidon_TurnClose_ScanAtk
	encsetvar 9, STAT_SPATK
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	goto EncScript_Miraidon_TurnClose_ScanShow
EncScript_Miraidon_TurnClose_ScanAtk:
	encsetvar 9, STAT_ATK
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
EncScript_Miraidon_TurnClose_ScanShow:
	printstring STRINGID_ENCMIRAIDONSCANCOMPLETE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	goto EncScript_Miraidon_TurnClose_Overclock
EncScript_Miraidon_TurnClose_ScanTick:
	encsubvar 10, 1
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Miraidon_TurnClose_Overclock
	encjumpifvar CMP_EQUAL, 9, STAT_ATK, EncScript_Miraidon_TurnClose_ScanFadeAtk
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	goto EncScript_Miraidon_TurnClose_ScanFadeDone
EncScript_Miraidon_TurnClose_ScanFadeAtk:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -1
EncScript_Miraidon_TurnClose_ScanFadeDone:
	encsetvar 9, 0
	printstring STRINGID_ENCMIRAIDONSCANFADES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Miraidon_TurnClose_Overclock:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Miraidon_TurnClose_Error
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCMIRAIDONOVERCLOCKTICK
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encsubvar 6, 1     @ OverclockTurns
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Miraidon_TurnClose_Error
	encsubvar 2, 1     @ Nodes
EncScript_Miraidon_TurnClose_Error:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Miraidon_TurnClose_Done
	encsubvar 8, 1     @ ErrorActive
	encjumpifvar CMP_NOT_EQUAL, 8, 0, EncScript_Miraidon_TurnClose_ErrorCue
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, TRUE
	encsetterrain ENC_TERRAIN_ELECTRIC
	encsetvar 7, 0     @ ErrorCount
	encsetvar 5, 1     @ LastReduction: guarded again
	printstring STRINGID_ENCMIRAIDONRECOMPILING
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Miraidon_TurnClose_Done
EncScript_Miraidon_TurnClose_ErrorCue:
	printstring STRINGID_ENCMIRAIDONERROR
	waitmessage B_WAIT_TIME_SHORT
EncScript_Miraidon_TurnClose_Done:
	encsetvar 13, 0    @ TurnGuard
	return

// Analysis capped at 6 with 2+ Nodes standing: Miraidon commits everything to speed. Self-disables
// through OverclockTurns, which it sets non-zero on its only path.
EncScript_Miraidon_OverclockStart::
	encsetvar 6, 3     @ OverclockTurns
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	printstring STRINGID_ENCMIRAIDONOVERCLOCK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// The residual chip burned through the Nodes it was riding on. Nothing else in this fight touches
// Speed, so the flat -2 undo is exact.
EncScript_Miraidon_OverclockShutdown::
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -2
	printstring STRINGID_ENCMIRAIDONSYSTEMSOFFLINE
	waitmessage B_WAIT_TIME_LONG
	return

// 50% - FUTURE ENGINE ONLINE. Nodes regenerate to 2, opening the road to Overclock.
EncScript_Miraidon_Phase2::
	encsetvar 0, 1     @ Phase
	encsetvar 2, 2     @ Nodes
	printstring STRINGID_ENCMIRAIDONFUTUREENGINE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE_INSTANT
	return

// 20% - PARADOX COLLAPSE. Analysis and Nodes are maxed on entry, and failed predictions start
// building toward System Failure.
EncScript_Miraidon_Phase3::
	encsetvar 0, 2     @ Phase
	encsetvar 1, 6     @ Analysis
	encsetvar 2, 3     @ Nodes
	encsetvar 7, 0     @ ErrorCount
	printstring STRINGID_ENCMIRAIDONPARADOXCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Three failed predictions in Paradox Collapse: the guard drops for 3 turns. Never touches
// DamageReduction itself (guideline 6's 80 floor stays intact) - the lever is the type cap and the
// terrain, and LastReduction only tracks that state for the callout.
EncScript_Miraidon_SystemFailureStart::
	encsetvar 8, 3     @ ErrorActive
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	removeterrain
	encsetvar 5, 0     @ LastReduction: exposed
	printstring STRINGID_ENCMIRAIDONERROR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

// 15% - PARADOX DRIVE, once. 16 + 2*Analysis + 4*Nodes capped at 30, approximated as a Nodes x
// Analysis-band lookup (encchangehp only takes compile-time literals - see the .encounter header).
// Resets every meter the fight built, so the last stretch to the catch window is played clean.
EncScript_Miraidon_ParadoxDrive::
	printstring STRINGID_ENCMIRAIDONPARADOXDRIVE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Miraidon_ParadoxDrive_N0
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Miraidon_ParadoxDrive_N1
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Miraidon_ParadoxDrive_N2
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Miraidon_ParadoxDrive_Reset
EncScript_Miraidon_ParadoxDrive_N0:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Miraidon_ParadoxDrive_18
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Miraidon_ParadoxDrive_22
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
	goto EncScript_Miraidon_ParadoxDrive_Reset
EncScript_Miraidon_ParadoxDrive_N1:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Miraidon_ParadoxDrive_22
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Miraidon_ParadoxDrive_26
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Miraidon_ParadoxDrive_Reset
EncScript_Miraidon_ParadoxDrive_N2:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Miraidon_ParadoxDrive_26
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Miraidon_ParadoxDrive_Reset
EncScript_Miraidon_ParadoxDrive_18:
	encchangehp ENC_TARGET_PLAYER_LEFT, -18, ENC_AMOUNT_PERCENT
	goto EncScript_Miraidon_ParadoxDrive_Reset
EncScript_Miraidon_ParadoxDrive_22:
	encchangehp ENC_TARGET_PLAYER_LEFT, -22, ENC_AMOUNT_PERCENT
	goto EncScript_Miraidon_ParadoxDrive_Reset
EncScript_Miraidon_ParadoxDrive_26:
	encchangehp ENC_TARGET_PLAYER_LEFT, -26, ENC_AMOUNT_PERCENT
EncScript_Miraidon_ParadoxDrive_Reset:
	encsetvar 1, 0     @ Analysis
	encsetvar 2, 0     @ Nodes
	encsetvar 6, 0     @ OverclockTurns
	return

// 10% - the catch window (guideline 10).
EncScript_Miraidon_CatchWindow::
	printstring STRINGID_ENCMIRAIDONCATCHWINDOW
	waitmessage B_WAIT_TIME_LONG
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 60
	return
