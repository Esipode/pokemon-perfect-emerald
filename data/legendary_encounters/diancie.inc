// Diancie, "The Jewel Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Diancie_Awakens:
// 0 Phase (1 Crystal Growth / 2 Jewel of Life / 3 Mega / 4 Spent), 1 Facets (0-6), 2 Band (last
// ANNOUNCED band: 0=0-2, 1=3-4, 2=5-6), 3 TurnGuard, 4 Mark (HP% snapshot at turn open; damage-
// since-mark after ENC_SNAP_DAMAGE), 5 Growth (turns since last facet growth), 6 Charge (Storm
// fuse, 0 idle), 7 StormTier (0 none / 1 Diamond Storm / 2 Cataclysm), 8 Broken (facets broken
// during the current charge), 9 Encase (remaining encase turns), 10 MoveLatch, 11 FaintLatch.
//
// FACETS IS THE GUARD. EncScript_Diancie_Guard is the damage reduction's only writer and reads
// Facets and Charge absolutely, so a facet breaking undoes a facet forming with no separate
// teardown path. Charging costs 25 points of cover across the board - concentration is the fight's
// burst window.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction: Idle = 62 + 5*Facets, Charging = 37 + 5*Facets.
EncScript_Diancie_Guard:
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Diancie_Guard_Charging
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Diancie_Guard_Idle0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Diancie_Guard_Idle1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Diancie_Guard_Idle2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Diancie_Guard_Idle3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Diancie_Guard_Idle4
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Diancie_Guard_Idle5
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Diancie_Guard_Idle0:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Diancie_Guard_Idle1:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Diancie_Guard_Idle2:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Diancie_Guard_Idle3:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Diancie_Guard_Idle4:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Diancie_Guard_Idle5:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Diancie_Guard_Charging:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Diancie_Guard_Charge0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Diancie_Guard_Charge1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Diancie_Guard_Charge2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Diancie_Guard_Charge3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Diancie_Guard_Charge4
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Diancie_Guard_Charge5
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Diancie_Guard_Charge0:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Diancie_Guard_Charge1:
	encsetdamagereduction ENC_TARGET_BOSS, 83
	return
EncScript_Diancie_Guard_Charge2:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Diancie_Guard_Charge3:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Diancie_Guard_Charge4:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Diancie_Guard_Charge5:
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return

// THE BAND CALLOUT (guideline 14). Band holds the last ANNOUNCED band; each arm compares it against
// its own literal and stays quiet if that's what was last said, so the line prints only on a
// difference, in both directions.
EncScript_Diancie_Band:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Diancie_Band_B0
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Diancie_Band_B1
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Diancie_Band_Done
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Diancie_Band_B2Thin
	encsetvar 2, 2
	printstring STRINGID_ENCDIANCIELATTICETHICKENS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Diancie_Band_B2Thin:
	encsetvar 2, 2
	printstring STRINGID_ENCDIANCIELATTICETHINS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Diancie_Band_B1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Diancie_Band_Done
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_Diancie_Band_B1Thin
	encsetvar 2, 1
	printstring STRINGID_ENCDIANCIELATTICETHICKENS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Diancie_Band_B1Thin:
	encsetvar 2, 1
	printstring STRINGID_ENCDIANCIELATTICETHINS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Diancie_Band_B0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Diancie_Band_Done
	encsetvar 2, 0
	printstring STRINGID_ENCDIANCIELATTICETHINS
	waitmessage B_WAIT_TIME_LONG
EncScript_Diancie_Band_Done:
	return

// All facet growth routes through here: increment/cap at 6, announce, then re-derive guard/band.
EncScript_Diancie_Grow:
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Diancie_Grow_Settle
	encaddvar 1, 1
	printstring STRINGID_ENCDIANCIEFACETFORMS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
EncScript_Diancie_Grow_Settle:
	call EncScript_Diancie_Guard
	call EncScript_Diancie_Band
	return

// Clears every Encase effect. Only called from sites that already know Encase is active, so the
// announcement always corresponds to a real release.
EncScript_Diancie_Release:
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 2
	encsetdamagereduction ENC_TARGET_ALL_FOES, 0
	encsetvar 9, 0     @ Encase
	printstring STRINGID_ENCDIANCIEENCASEBREAK
	waitmessage B_WAIT_TIME_LONG
	return

// Resolves a completed Diamond Storm charge. StormTier picks the payload table; Broken (facets
// cracked during the fuse) picks the row. Enough breaks collapses the storm into a harmless
// backfire instead of a hit.
EncScript_Diancie_StormResolve:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Diancie_StormResolve_Cataclysm
	encjumpifvar CMP_GREATER_THAN, 8, 2, EncScript_Diancie_StormResolve_Collapse
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Diancie_StormResolve_T1_45
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Diancie_StormResolve_T1_33
	goto EncScript_Diancie_StormResolve_T1_21
EncScript_Diancie_StormResolve_T1_45:
	printstring STRINGID_ENCDIANCIESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_StormResolve_Done
EncScript_Diancie_StormResolve_T1_33:
	printstring STRINGID_ENCDIANCIESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_StormResolve_Done
EncScript_Diancie_StormResolve_T1_21:
	printstring STRINGID_ENCDIANCIESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_StormResolve_Done
EncScript_Diancie_StormResolve_Cataclysm:
	encjumpifvar CMP_GREATER_THAN, 8, 3, EncScript_Diancie_StormResolve_Collapse
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Diancie_StormResolve_T2_65
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Diancie_StormResolve_T2_50
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Diancie_StormResolve_T2_35
	goto EncScript_Diancie_StormResolve_T2_20
EncScript_Diancie_StormResolve_T2_65:
	printstring STRINGID_ENCDIANCIESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -22, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_StormResolve_Done
EncScript_Diancie_StormResolve_T2_50:
	printstring STRINGID_ENCDIANCIESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_StormResolve_Done
EncScript_Diancie_StormResolve_T2_35:
	printstring STRINGID_ENCDIANCIESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_StormResolve_Done
EncScript_Diancie_StormResolve_T2_20:
	printstring STRINGID_ENCDIANCIESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_StormResolve_Done
EncScript_Diancie_StormResolve_Collapse:
	printstring STRINGID_ENCDIANCIESTORMCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetrecharge ENC_TARGET_BOSS, 2
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -2
	encsubvar 1, 2     @ Facets - the collapse thresholds guarantee this never underflows
EncScript_Diancie_StormResolve_Done:
	encsetvar 7, 0     @ StormTier
	encsetvar 6, 0     @ Charge
	encsetvar 8, 0     @ Broken
	call EncScript_Diancie_Guard
	call EncScript_Diancie_Band
	return

// --- Trigger entry points ---

// OnBattleStart. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Diancie_Awakens::
	printstring STRINGID_ENCDIANCIEAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 1     @ Phase
	encsetvar 2, 0     @ Band: no lattice yet, nothing to announce
	call EncScript_Diancie_Guard
	return

// Top of every turn. Leads with flushtextbox - the action menu is still scrolled open at
// OnTurnStart, and an animation played before any dialogue would render on top of it and corrupt
// it. TurnGuard is set immediately so no branch out of this script can leave it re-armed.
EncScript_Diancie_Turn::
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Diancie_Turn_Done
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Diancie_Turn_Growth
	encsubvar 6, 1     @ Charge
	printstring STRINGID_ENCDIANCIESTORMCHARGE
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
EncScript_Diancie_Turn_Growth:
	encaddvar 5, 1     @ Growth
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Diancie_Turn_GrowthP1
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Diancie_Turn_Storm
	goto EncScript_Diancie_Turn_GrowthFire
EncScript_Diancie_Turn_GrowthP1:
	encjumpifvar CMP_LESS_THAN, 5, 3, EncScript_Diancie_Turn_Storm
EncScript_Diancie_Turn_GrowthFire:
	encsetvar 5, 0
	call EncScript_Diancie_Grow
EncScript_Diancie_Turn_Storm:
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Diancie_Turn_Encase
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Diancie_Turn_Encase
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Diancie_Turn_StormP1
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Diancie_Turn_Encase
	goto EncScript_Diancie_Turn_StormBegin
EncScript_Diancie_Turn_StormP1:
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Diancie_Turn_Encase
EncScript_Diancie_Turn_StormBegin:
	encsetvar 6, 3     @ Charge
	encsetvar 7, 1     @ StormTier
	encsetvar 8, 0     @ Broken
	printstring STRINGID_ENCDIANCIESTORMBEGINS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Diancie_Guard
EncScript_Diancie_Turn_Encase:
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Diancie_Turn_EncaseHold
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Diancie_Turn_Standing
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Diancie_Turn_Standing
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Diancie_Turn_Standing
	encsetvar 9, 3     @ Encase
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	encsethealblock ENC_TARGET_ALL_FOES, 3
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -2
	encsetdamagereduction ENC_TARGET_ALL_FOES, 25
	playanimation BS_OPPONENT1, B_ANIM_TURN_TRAP
	printstring STRINGID_ENCDIANCIEENCASE
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Diancie_Turn_Snapshot
EncScript_Diancie_Turn_EncaseHold:
	printstring STRINGID_ENCDIANCIEENCASEHOLD
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Diancie_Turn_Snapshot
EncScript_Diancie_Turn_Standing:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Diancie_Turn_Snapshot
	printstring STRINGID_ENCDIANCIESTANDINGLATTICE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Diancie_Turn_Snapshot:
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET
EncScript_Diancie_Turn_Done:
	return

// End of every turn. Order is load-bearing: snapshot -> shatter -> storm -> heal -> encase tick ->
// clear latches.
EncScript_Diancie_Resolve::
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Diancie_Resolve_Clear
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_DAMAGE, EncScript_Diancie_Resolve_NoDamage
	goto EncScript_Diancie_Resolve_Shatter
EncScript_Diancie_Resolve_NoDamage:
	encsetvar 4, 0
EncScript_Diancie_Resolve_Shatter:
	encjumpifvar CMP_LESS_THAN, 4, 5, EncScript_Diancie_Resolve_Storm
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Diancie_Resolve_Storm
	encsubvar 1, 1     @ Facets - only one may break per turn
	printstring STRINGID_ENCDIANCIESHATTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Diancie_Resolve_ShatterEncase
	encaddvar 8, 1     @ Broken
EncScript_Diancie_Resolve_ShatterEncase:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Diancie_Resolve_ShatterGuard
	call EncScript_Diancie_Release
EncScript_Diancie_Resolve_ShatterGuard:
	call EncScript_Diancie_Guard
	call EncScript_Diancie_Band
EncScript_Diancie_Resolve_Storm:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Diancie_Resolve_Heal
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Diancie_Resolve_Heal
	call EncScript_Diancie_StormResolve
EncScript_Diancie_Resolve_Heal:
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Diancie_Resolve_EncaseTick
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Diancie_Resolve_EncaseTick
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Diancie_Resolve_Heal3
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	goto EncScript_Diancie_Resolve_HealShow
EncScript_Diancie_Resolve_Heal3:
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
EncScript_Diancie_Resolve_HealShow:
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	printstring STRINGID_ENCDIANCIEGARDENHEAL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Diancie_Resolve_EncaseTick:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Diancie_Resolve_Clear
	encsubvar 9, 1
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Diancie_Resolve_Clear
	call EncScript_Diancie_Release
EncScript_Diancie_Resolve_Clear:
	encsetvar 10, 0    @ MoveLatch
	encsetvar 11, 0    @ FaintLatch
	encsetvar 3, 0     @ TurnGuard
	return

// Diancie lands a Rock or Fairy move on the player. Latch closed immediately since the trigger
// re-selects until it is.
EncScript_Diancie_Radiance::
	encsetvar 10, 1    @ MoveLatch
	call EncScript_Diancie_Grow
	return

// A player Pokemon faints. Same latch pattern as Radiance.
EncScript_Diancie_Drink::
	encsetvar 11, 1    @ FaintLatch
	call EncScript_Diancie_Grow
	return

// --- Phase transitions ---

// 70% - Jewel of Life. Permanent Misty Terrain; growth cadence and Storm threshold tighten from
// here on (read directly off Phase in _Turn/_StormResolve, nothing extra to set).
EncScript_Diancie_JewelOfLife::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCDIANCIEJEWELOFLIFE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetterrain ENC_TERRAIN_MISTY
	call EncScript_Diancie_Grow
	return

// 40% - Mega Diancie. Facets snap to 6 and Cataclysm (Storm Tier 2) begins its fuse immediately.
EncScript_Diancie_Mega::
	encsetvar 0, 3     @ Phase
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Diancie_Mega_NoEncase
	call EncScript_Diancie_Release
EncScript_Diancie_Mega_NoEncase:
	printstring STRINGID_ENCDIANCIEMEGA
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_DIANCIE_MEGA, EncScript_Diancie_Mega_FormFail, B_ANIM_FORM_CHANGE
EncScript_Diancie_Mega_FormFail:
	encsetvar 1, 6     @ Facets
	call EncScript_Diancie_Guard
	call EncScript_Diancie_Band
	encsetvar 6, 4     @ Charge
	encsetvar 7, 2     @ StormTier
	encsetvar 8, 0     @ Broken
	printstring STRINGID_ENCDIANCIECATACLYSM
	waitmessage B_WAIT_TIME_LONG
	return

// 10% - Spent, and the catch window (guideline 10). Teardown mirrors the Properties: block plus
// everything the fight raised.
EncScript_Diancie_Spent::
	encsetvar 0, 4     @ Phase
	printstring STRINGID_ENCDIANCIEFADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encformchange ENC_TARGET_BOSS, SPECIES_DIANCIE, EncScript_Diancie_Spent_NoRevert, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Diancie_Spent_NoRevert:
	encsetvar 1, 0     @ Facets
	encsetvar 2, 0     @ Band
	encsetvar 5, 0     @ Growth
	encsetvar 6, 0     @ Charge
	encsetvar 7, 0     @ StormTier
	encsetvar 8, 0     @ Broken
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Diancie_Spent_NoEncase
	call EncScript_Diancie_Release
EncScript_Diancie_Spent_NoEncase:
	removeterrain
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Diancie_Spent_NoStats
EncScript_Diancie_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCDIANCIEWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
