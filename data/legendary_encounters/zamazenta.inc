// Zamazenta, "The Warrior Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Zamazenta_Init:
// 0 Phase (0 Phase0 / 1 Crowned Shield / 2 Last Stand / 3 Spent), 1 Resolve (0-100),
// 2 ShieldLayers (0-3), 3 TurnGuard, 4 TurnMark (HP% at turn open; ENC_SNAP_DAMAGE reuses it as
// this turn's damage% at Upkeep), 5 LeftHits (physical, 0-3), 6 CenterHits (special, 0-3),
// 7 RightHits (status, 0-3), 8 FortressBroken, 9 GuardStored (0-30), 10 GuardArmed, 11 LastBand
// (reserved), 12 HitGuard (self-disable for the category trackers), 13 BashGuard (self-disable
// for Shield Bash Rebuild).
//
// EncScript_Zamazenta_RecomputeReduction is the single writer of the damage reduction from
// EncScript_Zamazenta_Init onward - a band falling always undoes a band rising with no separate
// teardown path.
//
// HitGuard/BashGuard exist because Event.* conditions stay true for the whole re-evaluation loop
// of a single OnMoveEnd occurrence (determinism rule 3): without a guard, TrackLeftHit or
// ShieldBashRebuild would re-select themselves against the same landed hit until the
// per-checkpoint script cap trips. Both guards are cleared once per turn by UpkeepClear, which is
// enough - a singles turn gives Zamazenta and the player exactly one move each.
//
// Stat stages are written absolutely (Last Stand's +2/+2): clamp to +6 with a +12 write, then
// apply the single delta that lands on the intended stage, so a Haze or a switch landing in
// between can't desync it from what the script meant.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so every script here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase 3 (Spent) returns immediately - once Balls are
// allowed the engine's own catch guard owns the number.
EncScript_Zamazenta_RecomputeReduction:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Zamazenta_RecomputeReduction_Done
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Zamazenta_RecomputeReduction_Phase1Plus
// Phase 0: 84 + 4*ShieldLayers -> 84/88/92/96
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Zamazenta_RecomputeReduction_88
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Zamazenta_RecomputeReduction_90
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Zamazenta_RecomputeReduction_92
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Zamazenta_RecomputeReduction_88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Zamazenta_RecomputeReduction_90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Zamazenta_RecomputeReduction_92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Zamazenta_RecomputeReduction_Phase1Plus:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zamazenta_RecomputeReduction_LastStand
// Phase 1: Fortress up = 94, collapsed = 88 (same floor as Phase 0's weakest band)
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Zamazenta_RecomputeReduction_88
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Zamazenta_RecomputeReduction_LastStand:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Zamazenta_RecomputeReduction_Done:
	return

// --- OnBattleStart ---

EncScript_Zamazenta_Init::
	encsetvar 0, 0
	encsetvar 1, 0
	encsetvar 2, 0
	encsetvar 3, 0
	encsetvar 4, 100
	encsetvar 5, 0
	encsetvar 6, 0
	encsetvar 7, 0
	encsetvar 8, 0
	encsetvar 9, 0
	encsetvar 10, 0
	encsetvar 11, 0
	encsetvar 12, 0
	encsetvar 13, 0
	call EncScript_Zamazenta_RecomputeReduction
	return

// --- Once-per-turn scaffold ---

EncScript_Zamazenta_UpkeepClear::
	encsetvar 3, 0
	encsnapshothp ENC_TARGET_BOSS, 4
	encsetvar 12, 0
	encsetvar 13, 0
	return

EncScript_Zamazenta_Upkeep::
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_DAMAGE   // TurnMark -> damage% taken this turn
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Zamazenta_Upkeep_Cleanse
// ---- Phase 0: Resolve growth ----
	encaddvar 1, 5                                      // turn survived
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Zamazenta_Upkeep_NoDmgBonus
	encaddvar 1, 5                                      // player landed damage this turn
EncScript_Zamazenta_Upkeep_NoDmgBonus:
	encjumpifvar CMP_LESS_THAN, 1, 101, EncScript_Zamazenta_Upkeep_Crack
	encsetvar 1, 100
EncScript_Zamazenta_Upkeep_Crack:
// ---- Crack check: >=12% HP in one turn cracks the newest layer and banks Guard charge ----
	encjumpifvar CMP_LESS_THAN, 4, 12, EncScript_Zamazenta_Upkeep_SetGuard
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Zamazenta_Upkeep_SetGuard
	encsubvar 2, 1
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTASHIELDCRACKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encjumpifvar CMP_LESS_THAN, 9, 16, EncScript_Zamazenta_Upkeep_GuardAddFull
	encsetvar 9, 30
	goto EncScript_Zamazenta_Upkeep_GuardAdded
EncScript_Zamazenta_Upkeep_GuardAddFull:
	encaddvar 9, 15
EncScript_Zamazenta_Upkeep_GuardAdded:
	encjumpifvar CMP_LESS_THAN, 9, 20, EncScript_Zamazenta_Upkeep_SetGuard
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Zamazenta_Upkeep_SetGuard
	encsetvar 10, 1
	printstring STRINGID_ENCZAMAZENTAGUARDCOUNTERBRACES
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Zamazenta_Upkeep_SetGuard
EncScript_Zamazenta_Upkeep_Cleanse:
// ---- Fortress Cleanse: Phase >=1, every turn, resets Zamazenta's own stages and status ----
	encresetstats ENC_TARGET_BOSS, EncScript_Zamazenta_Upkeep_NoStats
EncScript_Zamazenta_Upkeep_NoStats:
	curestatus BS_OPPONENT1
EncScript_Zamazenta_Upkeep_SetGuard:
	encsetvar 3, 1
	return

// --- Shield Layers (Phase 0, off Resolve thresholds) ---

EncScript_Zamazenta_ShieldLayerGrow1::
	encsetvar 2, 1
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTASHIELDLAYERFORMS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Zamazenta_ShieldLayerGrow2::
	encsetvar 2, 2
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTASHIELDLAYERFORMS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Zamazenta_ShieldLayerGrow3::
	encsetvar 2, 3
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTASHIELDLAYERFORMS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// --- Shield Bash Rebuild (Iron Head landing; BashGuard closes before any branch) ---

EncScript_Zamazenta_ShieldBashRebuild::
	encsetvar 13, 1
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Zamazenta_ShieldBashRebuild_Done
	encaddvar 2, 1
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTASHIELDBASHREBUILDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
EncScript_Zamazenta_ShieldBashRebuild_Done:
	return

// --- Guard Counter release (disarming first closes its own condition) ---

EncScript_Zamazenta_GuardCounterRelease::
	encsetvar 10, 0
	encjumpifvar CMP_LESS_THAN, 9, 24, EncScript_Zamazenta_GuardCounterRelease_Tier1
	encjumpifvar CMP_LESS_THAN, 9, 28, EncScript_Zamazenta_GuardCounterRelease_Tier2
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Zamazenta_GuardCounterRelease_Announce
EncScript_Zamazenta_GuardCounterRelease_Tier2:
	encchangehp ENC_TARGET_PLAYER_LEFT, -22, ENC_AMOUNT_PERCENT
	goto EncScript_Zamazenta_GuardCounterRelease_Announce
EncScript_Zamazenta_GuardCounterRelease_Tier1:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
EncScript_Zamazenta_GuardCounterRelease_Announce:
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCZAMAZENTAGUARDCOUNTERRELEASE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 9, 0
	return

// --- Break-the-Fortress category tracking (Phase 1 only; HitGuard closes before any branch) ---

EncScript_Zamazenta_TrackLeftHit::
	encsetvar 12, 1
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Zamazenta_TrackLeftHit_Done
	encaddvar 5, 1
EncScript_Zamazenta_TrackLeftHit_Done:
	return

EncScript_Zamazenta_TrackCenterHit::
	encsetvar 12, 1
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zamazenta_TrackCenterHit_Done
	encaddvar 6, 1
EncScript_Zamazenta_TrackCenterHit_Done:
	return

EncScript_Zamazenta_TrackRightHit::
	encsetvar 12, 1
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Zamazenta_TrackRightHit_Done
	encaddvar 7, 1
EncScript_Zamazenta_TrackRightHit_Done:
	return

EncScript_Zamazenta_ShieldDestroyedLeft::
	printstring STRINGID_ENCZAMAZENTALEFTSHIELDDESTROYED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

EncScript_Zamazenta_ShieldDestroyedCenter::
	printstring STRINGID_ENCZAMAZENTACENTERSHIELDDESTROYED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

EncScript_Zamazenta_ShieldDestroyedRight::
	printstring STRINGID_ENCZAMAZENTARIGHTSHIELDDESTROYED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

EncScript_Zamazenta_FortressCollapses::
	encsetvar 8, 1
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTAFORTRESSCOLLAPSES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

// --- Phase transitions ---

EncScript_Zamazenta_CrownedShield::
	encsetvar 0, 1
	encsetvar 1, 100
	encsetvar 5, 0
	encsetvar 6, 0
	encsetvar 7, 0
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTACROWNEDSHIELDENTER
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_ZAMAZENTA_CROWNED, EncScript_Zamazenta_CrownedShield_Done, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Zamazenta_CrownedShield_Done:
	return

EncScript_Zamazenta_LastStand::
	encsetvar 0, 2
	encsetvar 1, 100
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -4
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -4
	call EncScript_Zamazenta_RecomputeReduction
	printstring STRINGID_ENCZAMAZENTALASTSTANDENTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// --- Catch window ---

EncScript_Zamazenta_BehemothBash::
	encsetvar 0, 3
	printstring STRINGID_ENCZAMAZENTABEHEMOTHBASHGATHERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encjumpifvar CMP_LESS_THAN, 9, 10, EncScript_Zamazenta_BehemothBash_Tier1
	encjumpifvar CMP_LESS_THAN, 9, 20, EncScript_Zamazenta_BehemothBash_Tier2
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Zamazenta_BehemothBash_Unleashed
EncScript_Zamazenta_BehemothBash_Tier2:
	encchangehp ENC_TARGET_PLAYER_LEFT, -25, ENC_AMOUNT_PERCENT
	goto EncScript_Zamazenta_BehemothBash_Unleashed
EncScript_Zamazenta_BehemothBash_Tier1:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
EncScript_Zamazenta_BehemothBash_Unleashed:
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCZAMAZENTABEHEMOTHBASHUNLEASHED
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 0
	encsetvar 2, 0
	encsetvar 8, 0
	encsetvar 9, 0
	encsetvar 10, 0
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Zamazenta_BehemothBash_NoStats
EncScript_Zamazenta_BehemothBash_NoStats:
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCZAMAZENTANOWISTHEMOMENT
	waitmessage B_WAIT_TIME_LONG
	return
