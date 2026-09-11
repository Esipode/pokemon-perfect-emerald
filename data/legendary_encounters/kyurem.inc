// Kyurem, "The Boundary Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Kyurem_Intro:
// 0 Phase (0 Boundary / 1 Empty Dragon / 2 Broken Dragon / 3 Absolute Zero / 4 Spent), 1 Meter
// (ENTROPY 0-10 in Phases 0-2, ENERGY 0-6 in Phase 3), 2 LastBand (the last ANNOUNCED band),
// 3 TurnGuard, 4 BossMark (Kyurem's HP% at turn open, reused as the player's mark once measured),
// 5 Scratch (the copy ENC_SNAP_DAMAGE consumes; afterwards, how much of Kyurem's max HP the player
// took this turn), 6 Charge (0 none / 1-3 turns left on an Absolute Zero charge / 9 staggered),
// 7 Law (0 none / 1 switching / 2 moves / 3 field / 4 stats), 8 LawTurns, 9 LawNext (the rotation
// cursor, 1-4), 10 EventGuard, 11 Fed (a Fire/Fighting hit counted this turn - also the charge and
// fusion interrupt flag), 12 Fusion (0 none / 1-2 absorbing a dragon the player has met / 3-4
// absorbing one they never met / 5 fused White / 6 fused Black), 13 FusionTurns, 14 Ultimate
// (freezes landed), 15 PlayerMark (the player's HP% at turn open).
//
// THE METER OWNS THE GUARD: EncScript_Kyurem_ApplyStance is the damage reduction's only writer
// outside _Spent, and Entropy is the half of its input the player controls. Kyurem eats what
// happens - every faint, every heal, every screen it strips - and the emptier the battlefield gets
// the harder it is to hurt. Heat and pressure are the only things that put energy back.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction, written absolutely from Phase, Meter, Charge and
// Fusion, so a band falling undoes a band rising with no separate teardown path. Phase 4 returns
// immediately: once balls are allowed the engine's own catch guard owns the number.
//          THIN(0-2)  DRAWN(3-4)  DEEP(5-7)  ABSOLUTE(8-10)   charging/fused +2   staggered flat
//  Phase 0    86         89          92          94                               76
//  Phase 1    88         90          93          95                               78
//  Phase 2    90         92          94          96                               80
//  Phase 3    a flat 97, no band and no modifiers - the lock is not meant to be out-damaged
EncScript_Kyurem_ApplyStance:
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyurem_ApplyStance_Done
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_ApplyStance_Lock
	encjumpifvar CMP_EQUAL, 6, 9, EncScript_Kyurem_ApplyStance_Stagger
	encjumpifvar CMP_GREATER_THAN, 12, 4, EncScript_Kyurem_ApplyStance_Boost   @ fused
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Kyurem_ApplyStance_Boost       @ charging
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Kyurem_ApplyStance_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Kyurem_ApplyStance_P1
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Kyurem_ApplyStance_P0Abs
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Kyurem_ApplyStance_P0Deep
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyurem_ApplyStance_P0Drawn
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Kyurem_ApplyStance_P0Drawn:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Kyurem_ApplyStance_P0Deep:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Kyurem_ApplyStance_P0Abs:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Kyurem_ApplyStance_P1:
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Kyurem_ApplyStance_P1Abs
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Kyurem_ApplyStance_P1Deep
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyurem_ApplyStance_P1Drawn
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Kyurem_ApplyStance_P1Drawn:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Kyurem_ApplyStance_P1Deep:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Kyurem_ApplyStance_P1Abs:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Kyurem_ApplyStance_P2:
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Kyurem_ApplyStance_P2Abs
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Kyurem_ApplyStance_P2Deep
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyurem_ApplyStance_P2Drawn
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Kyurem_ApplyStance_P2Drawn:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Kyurem_ApplyStance_P2Deep:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Kyurem_ApplyStance_P2Abs:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
	@ Charging an Absolute Zero, or wearing a borrowed dragon: the same band, two points harder.
EncScript_Kyurem_ApplyStance_Boost:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Kyurem_ApplyStance_BP2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Kyurem_ApplyStance_BP1
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Kyurem_ApplyStance_BP0Abs
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Kyurem_ApplyStance_BP0Deep
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyurem_ApplyStance_BP0Drawn
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Kyurem_ApplyStance_BP0Drawn:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Kyurem_ApplyStance_BP0Deep:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Kyurem_ApplyStance_BP0Abs:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Kyurem_ApplyStance_BP1:
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Kyurem_ApplyStance_BP1Abs
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Kyurem_ApplyStance_BP1Deep
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyurem_ApplyStance_BP1Drawn
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Kyurem_ApplyStance_BP1Drawn:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Kyurem_ApplyStance_BP1Deep:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Kyurem_ApplyStance_BP1Abs:
	encsetdamagereduction ENC_TARGET_BOSS, 97
	return
EncScript_Kyurem_ApplyStance_BP2:
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Kyurem_ApplyStance_BP2Abs
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Kyurem_ApplyStance_BP2Deep
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyurem_ApplyStance_BP2Drawn
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Kyurem_ApplyStance_BP2Drawn:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Kyurem_ApplyStance_BP2Deep:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Kyurem_ApplyStance_BP2Abs:
	encsetdamagereduction ENC_TARGET_BOSS, 98
	return
	@ Reeling from a broken charge or a collapsed resonance - the biggest window in the fight, and
	@ band-independent: what Kyurem hoarded is not protecting it while it is coming apart.
EncScript_Kyurem_ApplyStance_Stagger:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Kyurem_ApplyStance_StagP2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Kyurem_ApplyStance_StagP1
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
EncScript_Kyurem_ApplyStance_StagP1:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Kyurem_ApplyStance_StagP2:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Kyurem_ApplyStance_Lock:
	encsetdamagereduction ENC_TARGET_BOSS, 97
EncScript_Kyurem_ApplyStance_Done:
	return

// THE ENTROPY CALLOUT (guideline 14). LastBand holds the last ANNOUNCED band; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a difference - in BOTH directions. A falling band matters as much as a rising one: it is the
// player's own work and the fight has no other way to confirm it. In Phase 3 the meter is ENERGY
// instead, and every step of it is announced the same way.
EncScript_Kyurem_AnnounceBand:
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyurem_AnnounceBand_Done
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_AnnounceBand_Energy
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Kyurem_AnnounceBand_B3
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Kyurem_AnnounceBand_B2
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyurem_AnnounceBand_B1
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 0
	printstring STRINGID_ENCKYUREMBAND0
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Kyurem_AnnounceBand_B1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 1
	printstring STRINGID_ENCKYUREMBAND1
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Kyurem_AnnounceBand_B2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 2
	printstring STRINGID_ENCKYUREMBAND2
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Kyurem_AnnounceBand_B3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 3
	printstring STRINGID_ENCKYUREMBAND3
	waitmessage B_WAIT_TIME_LONG
	return
	@ ENERGY 1-5. 0 is where the lock starts and says so itself; 6 is the SHATTER, which speaks for
	@ itself. Enumerated rather than compared: conditions and jumps only test against literals.
EncScript_Kyurem_AnnounceBand_Energy:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Kyurem_AnnounceBand_E1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Kyurem_AnnounceBand_E2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Kyurem_AnnounceBand_E3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Kyurem_AnnounceBand_E4
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Kyurem_AnnounceBand_E5
	return
EncScript_Kyurem_AnnounceBand_E1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 1
	goto EncScript_Kyurem_AnnounceBand_Say
EncScript_Kyurem_AnnounceBand_E2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 2
	goto EncScript_Kyurem_AnnounceBand_Say
EncScript_Kyurem_AnnounceBand_E3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 3
	goto EncScript_Kyurem_AnnounceBand_Say
EncScript_Kyurem_AnnounceBand_E4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 4
	goto EncScript_Kyurem_AnnounceBand_Say
EncScript_Kyurem_AnnounceBand_E5:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Kyurem_AnnounceBand_Done
	encsetvar 2, 5
EncScript_Kyurem_AnnounceBand_Say:
	printstring STRINGID_ENCKYUREMENERGY
	waitmessage B_WAIT_TIME_SHORT
EncScript_Kyurem_AnnounceBand_Done:
	return

// Entropy +1, or ENERGY +1 in the lock. NOT announced here - the turn's scripts call _AnnounceBand
// once at the end, so however many times the meter moves the player reads one line about it.
EncScript_Kyurem_Gain:
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyurem_Gain_Done
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_Gain_Energy
	encjumpifvar CMP_GREATER_THAN, 1, 9, EncScript_Kyurem_Gain_Done
	encaddvar 1, 1
	return
EncScript_Kyurem_Gain_Energy:
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Kyurem_Gain_Done
	encaddvar 1, 1
EncScript_Kyurem_Gain_Done:
	return

// Entropy -1. Nothing in the fight lowers ENERGY, so the lock never reaches this.
EncScript_Kyurem_Lose:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Kyurem_Lose_Done
	encsubvar 1, 1
EncScript_Kyurem_Lose_Done:
	return

// Clears whichever FROZEN LAW is live. Only the trapping law needs a teardown - the other three are
// re-applied per turn, or the engine's own Disable timer owns them.
EncScript_Kyurem_EndLaw:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Kyurem_EndLaw_Done
	encjumpifvar CMP_NOT_EQUAL, 7, 1, EncScript_Kyurem_EndLaw_Clear
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
EncScript_Kyurem_EndLaw_Clear:
	encsetvar 7, 0
	encsetvar 8, 0
	printstring STRINGID_ENCKYUREMTHAWLAW
	waitmessage B_WAIT_TIME_SHORT
EncScript_Kyurem_EndLaw_Done:
	return

// THE REELING BEAT, shared by a broken charge and a collapsed resonance. Charge 9 is read by
// _ApplyStance as the staggered guard and cleared at the close of the NEXT turn, so the player has a
// whole turn of actions against it. Guarded against firing twice in one turn.
EncScript_Kyurem_Stagger:
	encjumpifvar CMP_EQUAL, 6, 9, EncScript_Kyurem_Stagger_Done
	encsetvar 6, 9
	encsetrecharge ENC_TARGET_BOSS, 2      @ at OnTurnEnd = next turn's action
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Kyurem_ApplyStance
	printstring STRINGID_ENCKYUREMOPENGUARD
	waitmessage B_WAIT_TIME_LONG
EncScript_Kyurem_Stagger_Done:
	return

// Takes a fused Kyurem back to base. The revert IS the instability the spec calls for: the borrowed
// form tears itself apart and costs Kyurem 6% of its max HP on the way out.
EncScript_Kyurem_Revert:
	encjumpifvar CMP_LESS_THAN, 12, 5, EncScript_Kyurem_Revert_Done
	printstring STRINGID_ENCKYUREMFUSIONTEARS
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_KYUREM, EncScript_Kyurem_Revert_Move, B_ANIM_FORM_CHANGE
EncScript_Kyurem_Revert_Move:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BLIZZARD
	restoremovepp BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_BOSS, -6, ENC_AMOUNT_PERCENT
	encsetvar 12, 0
	encsetvar 13, 3     @ FusionTurns: the cooldown before it may reach again
	call EncScript_Kyurem_ApplyStance
EncScript_Kyurem_Revert_Done:
	return

// --- Trigger scripts ---

EncScript_Kyurem_Intro::
	printstring STRINGID_ENCKYUREMINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKYUREMINTRO2
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKYUREMINTRO3
	waitmessage B_WAIT_TIME_LONG
	encsetvar 9, 1     @ LawNext: the rotation opens on FROZEN SWITCHING
	call EncScript_Kyurem_ApplyStance
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that prints before any
// animation renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch can leave it re-armed. Ticks every countdown (the decrement half of the
// split), takes the two marks the turn is measured against as percentages (the only portable choice
// under Level: LevelCap and New Game Plus offsets), re-applies the live law, re-arms the VOID's heal
// block, runs one recurring cue and clears the per-turn state.
EncScript_Kyurem_TurnOpen::
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 15, ENC_SNAP_SET
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Kyurem_TurnOpen_LawTick
	encjumpifvar CMP_EQUAL, 6, 9, EncScript_Kyurem_TurnOpen_LawTick   @ staggered, not a countdown
	encsubvar 6, 1
EncScript_Kyurem_TurnOpen_LawTick:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Kyurem_TurnOpen_FusionTick
	encsubvar 8, 1
EncScript_Kyurem_TurnOpen_FusionTick:
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Kyurem_TurnOpen_LawEffect
	encsubvar 13, 1
	@ The live law's per-turn bite. The lock strips the field every turn whatever law is nominally
	@ standing, which is what makes "get weather or terrain to stick" worth ENERGY.
EncScript_Kyurem_TurnOpen_LawEffect:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_TurnOpen_Strip
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Kyurem_TurnOpen_Stats
	encjumpifvar CMP_NOT_EQUAL, 7, 3, EncScript_Kyurem_TurnOpen_Void
EncScript_Kyurem_TurnOpen_Strip:
	removeweather
	jumpifterrain CMP_EQUAL, B_TERRAIN_NONE, EncScript_Kyurem_TurnOpen_Void
	removeterrain
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	goto EncScript_Kyurem_TurnOpen_Void
EncScript_Kyurem_TurnOpen_Stats:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Kyurem_TurnOpen_Void
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	@ THE VOID. Re-armed every turn rather than extended, so the engine's tick never reaches zero and
	@ its expiry line never fires (the documented encsethealblock pattern).
EncScript_Kyurem_TurnOpen_Void:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_TurnOpen_VoidBite
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Kyurem_TurnOpen_Cue
	encjumpifvar CMP_LESS_THAN, 1, 8, EncScript_Kyurem_TurnOpen_Cue
EncScript_Kyurem_TurnOpen_VoidBite:
	encsethealblock ENC_TARGET_ALL_FOES, 3
	printstring STRINGID_ENCKYUREMVOIDBITES
	waitmessage B_WAIT_TIME_SHORT
	@ One recurring cue a turn, in priority order charge > law > resonance (guideline 14).
EncScript_Kyurem_TurnOpen_Cue:
	encjumpifvar CMP_EQUAL, 6, 9, EncScript_Kyurem_TurnOpen_Clear
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Kyurem_TurnOpen_CueCharge
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Kyurem_TurnOpen_CueLaw
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Kyurem_TurnOpen_Clear
	encjumpifvar CMP_GREATER_THAN, 12, 4, EncScript_Kyurem_TurnOpen_Clear   @ fused: the form speaks
	printstring STRINGID_ENCKYUREMABSORB
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Kyurem_TurnOpen_Clear
EncScript_Kyurem_TurnOpen_CueCharge:
	printstring STRINGID_ENCKYUREMCHARGEREMIND
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Kyurem_TurnOpen_Clear
EncScript_Kyurem_TurnOpen_CueLaw:
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Kyurem_TurnOpen_CueLaw4
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Kyurem_TurnOpen_CueLaw3
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Kyurem_TurnOpen_CueLaw2
	printstring STRINGID_ENCKYUREMHOLD1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Kyurem_TurnOpen_Clear
EncScript_Kyurem_TurnOpen_CueLaw2:
	printstring STRINGID_ENCKYUREMHOLD2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Kyurem_TurnOpen_Clear
EncScript_Kyurem_TurnOpen_CueLaw3:
	printstring STRINGID_ENCKYUREMHOLD3
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Kyurem_TurnOpen_Clear
EncScript_Kyurem_TurnOpen_CueLaw4:
	printstring STRINGID_ENCKYUREMHOLD4
	waitmessage B_WAIT_TIME_SHORT
EncScript_Kyurem_TurnOpen_Clear:
	encsetvar 10, 0    @ EventGuard
	encsetvar 11, 0    @ Fed
	return

// A FROZEN LAW, imposed for 3 turns from Entropy 5 and rotated one per imposition so a repeat fight
// does not open the same way. Self-disabling: the script sets Law. FROZEN MOVES takes the next law
// in the rotation instead of wasting the imposition when there is nothing to disable - turn 1, no
// PP, or something already locked.
EncScript_Kyurem_Freeze::
	encjumpifvar CMP_EQUAL, 9, 4, EncScript_Kyurem_Freeze_Law4
	encjumpifvar CMP_EQUAL, 9, 3, EncScript_Kyurem_Freeze_Law3
	encjumpifvar CMP_EQUAL, 9, 2, EncScript_Kyurem_Freeze_Law2
	encsetvar 7, 1     @ FROZEN SWITCHING
	encsetvar 9, 2
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	playmoveanimation MOVE_MEAN_LOOK
	printstring STRINGID_ENCKYUREMFROZE1
	goto EncScript_Kyurem_Freeze_Hold
EncScript_Kyurem_Freeze_Law2:
	encsetvar 9, 3
	encdisablemove ENC_TARGET_PLAYER_LEFT, 3, EncScript_Kyurem_Freeze_Law3
	encsetvar 7, 2     @ FROZEN MOVES
	printstring STRINGID_ENCKYUREMFROZE2
	goto EncScript_Kyurem_Freeze_Hold
EncScript_Kyurem_Freeze_Law3:
	encsetvar 7, 3     @ FROZEN FIELD
	encsetvar 9, 4
	removeweather
	removeterrain
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	printstring STRINGID_ENCKYUREMFROZE3
	goto EncScript_Kyurem_Freeze_Hold
EncScript_Kyurem_Freeze_Law4:
	encsetvar 7, 4     @ FROZEN STATS
	encsetvar 9, 1
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Kyurem_Freeze_Law4Say
EncScript_Kyurem_Freeze_Law4Say:
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCKYUREMFROZE4
EncScript_Kyurem_Freeze_Hold:
	waitmessage B_WAIT_TIME_LONG
	encsetvar 8, 3     @ LawTurns
	return

// The ABSOLUTE ZERO telegraph, announced before the player picks a move so the turn can be spent on
// the answer. Self-disabling: the script sets Charge. Three turns in Phase 0, two after - and the
// charge can be broken on ANY of them, not only the one it would land on.
EncScript_Kyurem_BeginCharge::
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Kyurem_BeginCharge_Three
	encsetvar 6, 2
	goto EncScript_Kyurem_BeginCharge_Say
EncScript_Kyurem_BeginCharge_Three:
	encsetvar 6, 3
EncScript_Kyurem_BeginCharge_Say:
	printstring STRINGID_ENCKYUREMCHARGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	call EncScript_Kyurem_ApplyStance
	printstring STRINGID_ENCKYUREMCHARGEGUARD
	waitmessage B_WAIT_TIME_LONG
	return

// DRAGON RESONANCE (Phase 2). Kyurem reaches for Reshiram or Zekrom; encowned asks what the player's
// save knows about each one, which is the only route from "what has the player done outside this
// battle" into an encounter script. Both met - a coin flip decides. One met - that one. Neither -
// only a faint echo answers, and the payoff is halved. Scratch is safe to borrow: it carries a
// measurement between _TurnClose's ledger and the end of that same script, and this runs at
// OnTurnStart.
EncScript_Kyurem_Resonate::
	encowned SPECIES_RESHIRAM, 5
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Kyurem_Resonate_NoFlame
	encowned SPECIES_ZEKROM, 5
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Kyurem_Resonate_Flame
	encjumpifchance 50, EncScript_Kyurem_Resonate_Bolt
	goto EncScript_Kyurem_Resonate_Flame
EncScript_Kyurem_Resonate_NoFlame:
	encowned SPECIES_ZEKROM, 5
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Kyurem_Resonate_Bolt
	encjumpifchance 50, EncScript_Kyurem_Resonate_FaintBolt
	encsetvar 12, 3    @ absorbing a Reshiram it has no memory of
	goto EncScript_Kyurem_Resonate_FaintSay
EncScript_Kyurem_Resonate_FaintBolt:
	encsetvar 12, 4    @ absorbing a Zekrom it has no memory of
EncScript_Kyurem_Resonate_FaintSay:
	printstring STRINGID_ENCKYUREMRESFAINT
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Kyurem_Resonate_Gather
EncScript_Kyurem_Resonate_Flame:
	encsetvar 12, 1
	goto EncScript_Kyurem_Resonate_Remember
EncScript_Kyurem_Resonate_Bolt:
	encsetvar 12, 2
EncScript_Kyurem_Resonate_Remember:
	printstring STRINGID_ENCKYUREMRESREMEMBER
	waitmessage B_WAIT_TIME_LONG
EncScript_Kyurem_Resonate_Gather:
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Kyurem_Resonate_Electric
	encjumpifvar CMP_EQUAL, 12, 4, EncScript_Kyurem_Resonate_Electric
	printstring STRINGID_ENCKYUREMRESFIRE
	waitmessage B_WAIT_TIME_LONG
	encsetmove ENC_TARGET_BOSS, 3, MOVE_FUSION_FLARE
	goto EncScript_Kyurem_Resonate_Arm
EncScript_Kyurem_Resonate_Electric:
	printstring STRINGID_ENCKYUREMRESBOLT
	waitmessage B_WAIT_TIME_LONG
	encsetmove ENC_TARGET_BOSS, 3, MOVE_FUSION_BOLT
EncScript_Kyurem_Resonate_Arm:
	restoremovepp BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encsetvar 13, 1    @ FusionTurns: the absorb resolves at the close of the NEXT turn. The player
	                   @ gets two chances at it - the close of this turn and the close of that one -
	                   @ which is as long as Phase 2 lasts
	printstring STRINGID_ENCKYUREMABSORB
	waitmessage B_WAIT_TIME_LONG
	return

// HEAT. A Fire- or Fighting-type move landed on Kyurem. Fed makes it count once a turn and is what
// _TurnClose reads as the charge and resonance interrupt. In the Phase 3 lock the same blow fills
// ENERGY instead - the meter has flipped, and heat is what breaks the ice.
EncScript_Kyurem_Heat::
	encsetvar 11, 1    @ Fed
	printstring STRINGID_ENCKYUREMHEAT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_Heat_Energy
	call EncScript_Kyurem_Lose
	call EncScript_Kyurem_Lose
	return
EncScript_Kyurem_Heat_Energy:
	call EncScript_Kyurem_Gain
	call EncScript_Kyurem_Gain
	return

// A Pokemon fainted, either side's - Kyurem eats what happens. EventGuard stops the replacement
// switch-in being processed a second time against the same faint.
EncScript_Kyurem_Consume::
	encsetvar 10, 1    @ EventGuard
	printstring STRINGID_ENCKYUREMCONSUME
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_LEECH_SEED_DRAIN
	call EncScript_Kyurem_Gain
	call EncScript_Kyurem_Gain
	return

// A mid-turn switch (U-turn, Eject Button). Sets EventGuard and nothing else - it exists only so a
// faint later in the same turn is not counted twice.
EncScript_Kyurem_TheyEntered::
	encsetvar 10, 1    @ EventGuard
	return

// THE LEDGER, and the only place the turn's subsystems resolve. One script rather than six triggers:
// Kyurem has more end-of-turn work than any sibling, and six OnTurnEnd triggers plus a phase
// transition would sit one script away from the documented 8-per-checkpoint cap. Self-disables by
// clearing TurnGuard. Scratch survives the whole script after the measurement, so the charge test,
// the resonance test and the Phase 3 economy all read the same number without measuring again.
EncScript_Kyurem_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyurem_TurnClose_Quiet
	@ The reeling turn is over. It was set at the close of the PREVIOUS turn, so the whole of this
	@ turn was spent attacking into the staggered guard.
	encjumpifvar CMP_NOT_EQUAL, 6, 9, EncScript_Kyurem_TurnClose_Measure
	encsetvar 6, 0
	call EncScript_Kyurem_ApplyStance
EncScript_Kyurem_TurnClose_Measure:
	enccopyvar 5, 4    @ Scratch = BossMark, the copy ENC_SNAP_DAMAGE consumes
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Kyurem_TurnClose_NoDamage
	goto EncScript_Kyurem_TurnClose_Heal
EncScript_Kyurem_TurnClose_NoDamage:
	encsetvar 5, 0
EncScript_Kyurem_TurnClose_Heal:
	enccopyvar 4, 15   @ BossMark is spent; it is the player's mark now
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 4, ENC_SNAP_RECOVERY, EncScript_Kyurem_TurnClose_Economy
	printstring STRINGID_ENCKYUREMHEALFED
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Kyurem_Gain
EncScript_Kyurem_TurnClose_Economy:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_TurnClose_Lock
	@ Phases 0-2: the world runs down by itself, and only real pressure pushes back.
	call EncScript_Kyurem_Gain
	encjumpifvar CMP_LESS_THAN, 5, 6, EncScript_Kyurem_TurnClose_Drain
	call EncScript_Kyurem_Lose
	goto EncScript_Kyurem_TurnClose_Drain
	@ Phase 3: the meter is ENERGY, and the player is the one filling it.
EncScript_Kyurem_TurnClose_Lock:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Kyurem_TurnClose_LockPressure
	call EncScript_Kyurem_Gain
	call EncScript_Kyurem_Gain
EncScript_Kyurem_TurnClose_LockPressure:
	encjumpifvar CMP_LESS_THAN, 5, 5, EncScript_Kyurem_TurnClose_LockField
	call EncScript_Kyurem_Gain
EncScript_Kyurem_TurnClose_LockField:
	jumpifterrain CMP_NOT_EQUAL, B_TERRAIN_NONE, EncScript_Kyurem_TurnClose_LockFieldUp
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_ANY, EncScript_Kyurem_TurnClose_Charge
EncScript_Kyurem_TurnClose_LockFieldUp:
	call EncScript_Kyurem_Gain
	goto EncScript_Kyurem_TurnClose_Charge
	@ ENERGY DRAIN (Phases 0-2, from Entropy 3): the first find of screens, then stat stages, then
	@ the field. Every one of those commands is both the test and the take.
EncScript_Kyurem_TurnClose_Drain:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Kyurem_TurnClose_Charge
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Kyurem_TurnClose_DrainStats
	goto EncScript_Kyurem_TurnClose_DrainTook
EncScript_Kyurem_TurnClose_DrainStats:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Kyurem_TurnClose_DrainField
	goto EncScript_Kyurem_TurnClose_DrainTook
EncScript_Kyurem_TurnClose_DrainField:
	jumpifterrain CMP_NOT_EQUAL, B_TERRAIN_NONE, EncScript_Kyurem_TurnClose_DrainFieldTake
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_ANY, EncScript_Kyurem_TurnClose_DrainEmpty
EncScript_Kyurem_TurnClose_DrainFieldTake:
	removeweather
	removeterrain
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
EncScript_Kyurem_TurnClose_DrainTook:
	printstring STRINGID_ENCKYUREMDRAIN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_LEECH_SEED_DRAIN
	call EncScript_Kyurem_Gain
	goto EncScript_Kyurem_TurnClose_Charge
EncScript_Kyurem_TurnClose_DrainEmpty:
	printstring STRINGID_ENCKYUREMDRAINEMPTY
	waitmessage B_WAIT_TIME_SHORT
	@ THE CHARGE. Two interrupts, tested every turn the charge is live rather than only on the turn
	@ it would land: heat, or a fifteenth of Kyurem's max HP taken this turn.
EncScript_Kyurem_TurnClose_Charge:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Kyurem_TurnClose_Fusion
	encjumpifvar CMP_EQUAL, 6, 9, EncScript_Kyurem_TurnClose_Fusion
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Kyurem_TurnClose_Break
	encjumpifvar CMP_GREATER_THAN, 5, 14, EncScript_Kyurem_TurnClose_Break
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_Kyurem_TurnClose_Fusion   @ still gathering
	goto EncScript_Kyurem_TurnClose_Zero
EncScript_Kyurem_TurnClose_Break:
	encsetvar 6, 0
	printstring STRINGID_ENCKYUREMBROKEN
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyurem_Lose
	call EncScript_Kyurem_Lose
	call EncScript_Kyurem_Stagger
	goto EncScript_Kyurem_TurnClose_Fusion
	@ The freeze lands. ABSOLUTE ZERO in Phase 0, GLACIATE from Phase 1 - one mechanic under two
	@ names, where the later one also holds the standing law in place and sheds Kyurem's own status.
EncScript_Kyurem_TurnClose_Zero:
	encsetvar 6, 0
	encaddvar 14, 1    @ Ultimate
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Kyurem_TurnClose_ZeroName
	printstring STRINGID_ENCKYUREMGLACIATE
	goto EncScript_Kyurem_TurnClose_ZeroHit
EncScript_Kyurem_TurnClose_ZeroName:
	printstring STRINGID_ENCKYUREMZERO
EncScript_Kyurem_TurnClose_ZeroHit:
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_GLACIATE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyurem_TurnClose_ZeroLock
	encjumpifvar CMP_GREATER_THAN, 14, 2, EncScript_Kyurem_TurnClose_Zero30
	encjumpifvar CMP_EQUAL, 14, 2, EncScript_Kyurem_TurnClose_Zero26
	encchangehp ENC_TARGET_ALL_FOES, -22, ENC_AMOUNT_PERCENT
	goto EncScript_Kyurem_TurnClose_ZeroAfter
EncScript_Kyurem_TurnClose_Zero26:
	encchangehp ENC_TARGET_ALL_FOES, -26, ENC_AMOUNT_PERCENT
	goto EncScript_Kyurem_TurnClose_ZeroAfter
EncScript_Kyurem_TurnClose_Zero30:
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
EncScript_Kyurem_TurnClose_ZeroAfter:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Kyurem_TurnClose_ZeroScreens
EncScript_Kyurem_TurnClose_ZeroScreens:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Kyurem_TurnClose_ZeroSnow
EncScript_Kyurem_TurnClose_ZeroSnow:
	encsetweather BATTLE_WEATHER_SNOW      @ permanent - and what makes Blizzard never miss
	call EncScript_Kyurem_Gain
	call EncScript_Kyurem_Gain
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Kyurem_TurnClose_ZeroAgain
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Kyurem_TurnClose_ZeroNewLaw
	encaddvar 8, 3     @ the standing law is frozen three turns deeper
	goto EncScript_Kyurem_TurnClose_ZeroShed
EncScript_Kyurem_TurnClose_ZeroNewLaw:
	call EncScript_Kyurem_Freeze
EncScript_Kyurem_TurnClose_ZeroShed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
EncScript_Kyurem_TurnClose_ZeroAgain:
	encjumpifvar CMP_GREATER_THAN, 14, 2, EncScript_Kyurem_TurnClose_ZeroAgain2
	encjumpifvar CMP_EQUAL, 14, 2, EncScript_Kyurem_TurnClose_ZeroAgain1
	goto EncScript_Kyurem_TurnClose_Fusion
EncScript_Kyurem_TurnClose_ZeroAgain1:
	printstring STRINGID_ENCKYUREMAGAIN1
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Kyurem_TurnClose_Fusion
EncScript_Kyurem_TurnClose_ZeroAgain2:
	printstring STRINGID_ENCKYUREMAGAIN2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Kyurem_TurnClose_Fusion
	@ The lock's freeze is a clock, not a climax: a flat 18%, no escalation, no weather (the lock
	@ strips the field every turn anyway), and it still shakes off whatever the player put on Kyurem.
EncScript_Kyurem_TurnClose_ZeroLock:
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Kyurem_TurnClose_ZeroLockScreens
EncScript_Kyurem_TurnClose_ZeroLockScreens:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Kyurem_TurnClose_ZeroLockShed
EncScript_Kyurem_TurnClose_ZeroLockShed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	@ THE RESONANCE. The same two interrupts the charge uses, tested every turn of the absorb.
EncScript_Kyurem_TurnClose_Fusion:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Kyurem_TurnClose_LawTimer
	encjumpifvar CMP_GREATER_THAN, 12, 4, EncScript_Kyurem_TurnClose_Fused
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Kyurem_TurnClose_FusionBreak
	encjumpifvar CMP_GREATER_THAN, 5, 14, EncScript_Kyurem_TurnClose_FusionBreak
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Kyurem_TurnClose_LawTimer
	goto EncScript_Kyurem_TurnClose_FusionDone
EncScript_Kyurem_TurnClose_FusionBreak:
	printstring STRINGID_ENCKYUREMFUSIONFAILS
	waitmessage B_WAIT_TIME_LONG
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BLIZZARD
	restoremovepp BS_OPPONENT1
	call EncScript_Kyurem_Lose
	call EncScript_Kyurem_Lose
	call EncScript_Kyurem_Lose
	encsetvar 12, 0
	encsetvar 13, 3    @ FusionTurns: the cooldown before it may reach again
	call EncScript_Kyurem_Stagger
	goto EncScript_Kyurem_TurnClose_LawTimer
	@ UNSTABLE FUSION. The form change brings Turboblaze / Teravolt with it for free, and the engine
	@ announces those itself.
EncScript_Kyurem_TurnClose_FusionDone:
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Kyurem_TurnClose_FusionBlack
	encjumpifvar CMP_EQUAL, 12, 4, EncScript_Kyurem_TurnClose_FusionBlack
	encformchange ENC_TARGET_BOSS, SPECIES_KYUREM_WHITE, EncScript_Kyurem_TurnClose_FusionPayoff, B_ANIM_FORM_CHANGE
	goto EncScript_Kyurem_TurnClose_FusionPayoff
EncScript_Kyurem_TurnClose_FusionBlack:
	encformchange ENC_TARGET_BOSS, SPECIES_KYUREM_BLACK, EncScript_Kyurem_TurnClose_FusionPayoff, B_ANIM_FORM_CHANGE
EncScript_Kyurem_TurnClose_FusionPayoff:
	encjumpifvar CMP_GREATER_THAN, 12, 2, EncScript_Kyurem_TurnClose_FusionHalf
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 10, ENC_AMOUNT_PERCENT
	goto EncScript_Kyurem_TurnClose_FusionMark
	@ The player never met either dragon, so only an echo answered: half the payoff.
EncScript_Kyurem_TurnClose_FusionHalf:
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 10, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 5, ENC_AMOUNT_PERCENT
EncScript_Kyurem_TurnClose_FusionMark:
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Kyurem_TurnClose_FusionMarkBlack
	encjumpifvar CMP_EQUAL, 12, 4, EncScript_Kyurem_TurnClose_FusionMarkBlack
	encsetvar 12, 5    @ fused White
	goto EncScript_Kyurem_TurnClose_FusionSay
EncScript_Kyurem_TurnClose_FusionMarkBlack:
	encsetvar 12, 6    @ fused Black
EncScript_Kyurem_TurnClose_FusionSay:
	encsetvar 13, 4    @ FusionTurns: the form holds four turns, then tears itself apart
	printstring STRINGID_ENCKYUREMFUSED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyurem_ApplyStance
	goto EncScript_Kyurem_TurnClose_LawTimer
EncScript_Kyurem_TurnClose_Fused:
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Kyurem_TurnClose_LawTimer
	call EncScript_Kyurem_Revert
	@ The law timer resolves one checkpoint away from the _TurnOpen decrement (the mandated split).
EncScript_Kyurem_TurnClose_LawTimer:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Kyurem_TurnClose_Finish
	encjumpifvar CMP_NOT_EQUAL, 8, 0, EncScript_Kyurem_TurnClose_Finish
	call EncScript_Kyurem_EndLaw
EncScript_Kyurem_TurnClose_Finish:
	call EncScript_Kyurem_AnnounceBand
	call EncScript_Kyurem_ApplyStance
EncScript_Kyurem_TurnClose_Quiet:
	encsetvar 3, 0     @ TurnGuard
	return

// --- Phases ---

// 50% - EMPTY DRAGON. Slot 3 becomes Blizzard (which never misses under the snow a landed freeze
// leaves behind), raw Sp. Atk +12% (survives Haze), +2 Entropy. From here Absolute Zero resolves as
// GLACIATE, the VOID heal-blocks the player's side at Entropy 8+, and every landed freeze sheds
// Kyurem's own status.
EncScript_Kyurem_EmptyDragon::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCKYUREMCRACKS1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKYUREMCRACKS2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BLIZZARD
	restoremovepp BS_OPPONENT1
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 12, ENC_AMOUNT_PERCENT
	call EncScript_Kyurem_Gain
	call EncScript_Kyurem_Gain
	call EncScript_Kyurem_AnnounceBand
	call EncScript_Kyurem_ApplyStance
	printstring STRINGID_ENCKYUREMVOIDCUE
	waitmessage B_WAIT_TIME_LONG
	return

// 25% - BROKEN DRAGON. Raw Speed +10%, +1 Entropy, and the resonance cooldown is cleared so Kyurem
// may reach for Reshiram or Zekrom at the very next turn open.
EncScript_Kyurem_BrokenDragon::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCKYUREMREACH1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKYUREMREACH2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 10, ENC_AMOUNT_PERCENT
	call EncScript_Kyurem_Gain
	call EncScript_Kyurem_AnnounceBand
	call EncScript_Kyurem_ApplyStance
	encsetvar 13, 0    @ FusionTurns
	return

// 12% - ABSOLUTE ZERO, the lock. Permanent trapping, Heal Block and a stripped field every turn, and
// a freeze every second turn. The meter resets and reads as ENERGY 0-6, which the PLAYER fills.
// Survive: True is load-bearing here: the lock is meant to end by the SHATTER, not by attrition.
EncScript_Kyurem_TheLock::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCKYUREMLOCK1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKYUREMLOCK2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	call EncScript_Kyurem_Revert          @ the imitation cannot survive this
	call EncScript_Kyurem_EndLaw
	encsetvar 7, 1     @ Law: FROZEN SWITCHING, and it never lifts
	encsetvar 8, 99
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	encsetvar 1, 0     @ the meter is ENERGY now
	encsetvar 2, 0
	encsetvar 6, 0     @ Charge: _BeginCharge telegraphs the lock's first freeze next turn
	encsetvar 12, 0
	encsetvar 13, 0
	call EncScript_Kyurem_ApplyStance
	printstring STRINGID_ENCKYUREMTHAWCUE
	waitmessage B_WAIT_TIME_LONG
	return

// THE SHATTER, and the catch window (guideline 10). Reached by filling ENERGY to 6 - or, for a
// player who somehow ground the lock down anyway, by the 1 HP fallback, which exists only so the
// fight cannot deadlock. The backlash puts Kyurem at 6% of its max HP, every restriction comes off
// and the ball is allowed. encsetdamagereduction 99 writes the value the automatic catch guard will
// RESTORE, not the live one.
EncScript_Kyurem_Shatter::
	encsetvar 0, 4     @ Phase
	printstring STRINGID_ENCKYUREMSHATTER1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCKYUREMSHATTER2
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyurem_Revert
	call EncScript_Kyurem_EndLaw
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encsetvar 1, 0     @ Meter
	encsetvar 6, 0     @ Charge
	encsetvar 12, 0    @ Fusion
	encsetvar 13, 0    @ FusionTurns
	encchangehp ENC_TARGET_BOSS, 6, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCKYUREMWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
