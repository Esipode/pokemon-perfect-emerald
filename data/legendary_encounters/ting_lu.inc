// Ting-Lu battle scripts. Var indices, fixed by first appearance in ting_lu.encounter:
// 0 Phase (0 Ruinous Ground / 1 Ancient Mountain / 2 Mountain's Collapse), 1 Weight (0-6,
// monotonic), 2 Ground (0-3, follows Weight), 3 Charge (0-3, Earthquake cycle), 4 SinkWarn (0/1),
// 5 TurnGuard (once-per-turn scaffold), 6 Mark (Ruin Energy's baseline HP%), 7 Scratch (the copy
// ENC_SNAP_DAMAGE consumes to read live Ruin Energy without destroying Mark).

EncScript_TingLu_Init::
	encsetvar 0, 0    @ Phase = 0
	encsetvar 1, 0    @ Weight = 0
	encsetvar 2, 0    @ Ground = 0
	encsetvar 3, 0    @ Charge = 0
	encsetvar 4, 0    @ SinkWarn = 0
	encsetvar 5, 0    @ TurnGuard = 0
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET    @ Mark = 100
	encsetvar 7, 0    @ Scratch = 0
	printstring STRINGID_ENCTINGLUINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Shared: bumps Ground to match Weight's tier (2/4/6 -> 1/2/3), never downgrades. Called wherever
// Weight rises, so Ground can't lag a Weight jump (e.g. Ancient Mountain's jump-to-max).
EncScript_TingLu_AdvanceGround:
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_TingLu_AdvanceGround_Three
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_TingLu_AdvanceGround_Two
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_TingLu_AdvanceGround_One
	return
EncScript_TingLu_AdvanceGround_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_TingLu_AdvanceGround_Done
	encsetvar 2, 3
	return
EncScript_TingLu_AdvanceGround_Two:
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_TingLu_AdvanceGround_Done
	encsetvar 2, 2
	return
EncScript_TingLu_AdvanceGround_One:
	encjumpifvar CMP_GREATER_THAN, 2, 0, EncScript_TingLu_AdvanceGround_Done
	encsetvar 2, 1
EncScript_TingLu_AdvanceGround_Done:
	return

// Shared: resolves the stored Earthquake, tiered by Weight (the permanent hardening) with Ruin
// Energy (Scratch, the live meter) as a secondary escalator toward the ceiling. 16% baseline, 23%
// medium, 30% ceiling - never higher (guideline 7).
EncScript_TingLu_Earthquake:
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCTINGLUUNLEASHESQUAKE
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_TingLu_Earthquake_Heavy
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_TingLu_Earthquake_Medium
	encjumpifvar CMP_GREATER_THAN, 7, 39, EncScript_TingLu_Earthquake_Medium
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	return
EncScript_TingLu_Earthquake_Medium:
	encchangehp ENC_TARGET_PLAYER_LEFT, -23, ENC_AMOUNT_PERCENT
	return
EncScript_TingLu_Earthquake_Heavy:
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	return

EncScript_TingLu_AncientMountain::
	encsetvar 0, 1    @ Phase = 1
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_TingLu_AncientMountain_Hardening
	encsetvar 1, 6    @ Weight jumps to max outright
EncScript_TingLu_AncientMountain_Hardening:
	call EncScript_TingLu_AdvanceGround
	// Immovable: permanent raw stat changes, so Haze or a switch can't undo them (unlike a stage).
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 20, ENC_AMOUNT_PERCENT
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -2
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCTINGLUGATHERSWEIGHT
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTINGLUIMMOVABLE
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TingLu_MountainCollapse::
	encsetvar 0, 2    @ Phase = 2
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCTINGLUCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	return

// OnTurnStart, TurnGuard == 0. Sets TurnGuard first, so every branch below closes the once-per-turn
// latch regardless of which path it takes.
EncScript_TingLu_TurnOpen::
	flushtextbox    // OnTurnStart-only: the action menu scrolls BG0, and only a printstring/
	                // flushtextbox scrolls it back before an animation can safely play here.
	encsetvar 5, 1    @ TurnGuard = 1
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_TingLu_TurnOpen_Sinkhole
	encaddvar 3, 1    @ Charge += 1
	printstring STRINGID_ENCTINGLUCHARGES
	waitmessage B_WAIT_TIME_LONG
EncScript_TingLu_TurnOpen_Sinkhole:
	// A chance roll needs its own latch even on the miss path - here TurnGuard already provides it,
	// since TurnOpen is the sole trigger for this checkpoint and won't be re-selected mid-turn.
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_TingLu_TurnOpen_Done
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_TingLu_TurnOpen_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_TingLu_TurnOpen_SinkRollLate
	encjumpifchance 25, EncScript_TingLu_TurnOpen_SinkHit
	goto EncScript_TingLu_TurnOpen_Done
EncScript_TingLu_TurnOpen_SinkRollLate:
	encjumpifchance 45, EncScript_TingLu_TurnOpen_SinkHit
	goto EncScript_TingLu_TurnOpen_Done
EncScript_TingLu_TurnOpen_SinkHit:
	encsetvar 4, 1    @ SinkWarn = 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCTINGLUGROUNDSPLITS
	waitmessage B_WAIT_TIME_LONG
EncScript_TingLu_TurnOpen_Done:
	return

// OnSwitchIn. Its own gate (SinkWarn == 1) is exactly what this script clears, so the trigger can't
// re-select once it's run - no extra guard needed.
EncScript_TingLu_SinkholeClearOnSwitch::
	encsetvar 4, 0    @ SinkWarn = 0, escaped in time
	return

// OnTurnEnd, TurnGuard == 1. Sinkhole resolve, then the Ancient-Earth passive chip, then Earthquake
// release, then Ruin Energy -> Weight banking, in that order. Clears TurnGuard first.
EncScript_TingLu_Ledger::
	encsetvar 5, 0    @ TurnGuard = 0
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_TingLu_Ledger_SkipSink
	encsetvar 4, 0    @ SinkWarn = 0, the warning resolves either way
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCTINGLUGROUNDCOLLAPSES
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_TingLu_Ledger_SinkLate
	encchangehp ENC_TARGET_PLAYER_LEFT, -8, ENC_AMOUNT_PERCENT
	goto EncScript_TingLu_Ledger_SkipSink
EncScript_TingLu_Ledger_SinkLate:
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
EncScript_TingLu_Ledger_SkipSink:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_TingLu_Ledger_Chip
	goto EncScript_TingLu_Ledger_SkipChip
EncScript_TingLu_Ledger_Chip:
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCTINGLUGATHERSWEIGHT
	waitmessage B_WAIT_TIME_LONG
EncScript_TingLu_Ledger_SkipChip:
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_TingLu_Ledger_SkipQuake
	enccopyvar 7, 6    @ Scratch = Mark
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_DAMAGE, EncScript_TingLu_Ledger_SkipQuake
	call EncScript_TingLu_Earthquake
	encsetvar 3, 0    @ Charge = 0, spent
EncScript_TingLu_Ledger_SkipQuake:
	enccopyvar 7, 6    @ Scratch = Mark
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_DAMAGE, EncScript_TingLu_Ledger_Done
	encjumpifvar CMP_LESS_THAN, 7, 25, EncScript_TingLu_Ledger_Done
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_TingLu_Ledger_Done
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET    @ Mark resets, this tier is banked
	encaddvar 1, 1    @ Weight += 1
	call EncScript_TingLu_AdvanceGround
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCTINGLUGATHERSWEIGHT
	waitmessage B_WAIT_TIME_LONG
EncScript_TingLu_Ledger_Done:
	return

EncScript_TingLu_CatchWindow::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	printstring STRINGID_ENCTINGLUCATCHWINDOW
	waitmessage B_WAIT_TIME_LONG
	return

// Releases every stored Ruin Energy point in one last Earthquake, then the exhaustion teardown -
// undoes Immovable's Def bump and drops the reduction to the Phase 3 floor. Weight/Ground reset is
// flavor only; the fight is nearly over.
EncScript_TingLu_FinalAttack::
	enccopyvar 7, 6    @ Scratch = Mark
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_DAMAGE, EncScript_TingLu_FinalAttack_Spend
EncScript_TingLu_FinalAttack_Spend:
	call EncScript_TingLu_Earthquake
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET    @ Mark resets, Ruin Energy fully spent
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -20, ENC_AMOUNT_PERCENT
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 1, 0    @ Weight = 0
	encsetvar 2, 0    @ Ground = 0
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	printstring STRINGID_ENCTINGLUEXHAUSTION
	waitmessage B_WAIT_TIME_LONG
	return
