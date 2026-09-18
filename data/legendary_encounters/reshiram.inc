// Reshiram, "The Vast White Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Reshiram_Intro:
// 0 Phase (0 Observer / 1 Truth Revealed / 2 White Light / 3 Spent), 1 Rev (REVELATION, 0-5),
// 2 LastBand (the last ANNOUNCED band), 3 TurnGuard, 4 BossMark (Reshiram HP% at turn open),
// 5 Scratch (the copy ENC_SNAP_DAMAGE consumes; reused by the ledger as "the meter went the
// player's way"), 6 Read (0 unset / 1 physical / 2 special / 3 status / 9 the AI declined),
// 7 LastRead, 8 Fed, 9 MarkDepth (0-3), 10 Flame, 11 Charge (0 none / 1-3 turns left / 9 the
// Judgment has resolved), 12 EventGuard, 13 Pressure, 14 Exposed, 15 CueTick.
//
// REVELATION IS THE GUARD, IN BOTH DIRECTIONS. EncScript_Reshiram_ApplyStance is the damage
// reduction's only writer outside _Spent and reads Phase, Rev and Charge absolutely, so a band
// falling undoes a band rising with no separate teardown path. Phase 1's UNREAD value is LOWER than
// Phase 0's on purpose: grinding the meter back down after the flip is the best thing that can
// happen to the player and the number has to reward it visibly.
//
// EVERY OnTurnStart TRIGGER IS SELF-DISABLING through the variable its own script writes. Trigger
// selection re-runs until no condition holds, so a script that can return without closing its own
// latch hangs on the runaway guard - which is why _Reveal writes 9 for "no prediction" and _Expose
// spends a one-turn latch even when it found a bare field.
//
// STAT STAGES ARE WRITTEN ABSOLUTELY, the Rayquaza idiom: +12 clamps to the ceiling, then a single
// drop lands on the stage that was meant. That makes TRUTH MARK's depth immune to a player Haze, a
// Truth Flame scour or a switch landing in between.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase 3 returns immediately: once balls are allowed the
// engine's own catch guard owns the number. Charging costs Reshiram 6 points of cover (8 for the
// Judgment) - concentration is the fight's cleanest damage window.
EncScript_Reshiram_ApplyStance:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Reshiram_ApplyStance_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Reshiram_ApplyStance_P2
	encjumpifvar CMP_NOT_EQUAL, 11, 0, EncScript_Reshiram_ApplyStance_Hot
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Reshiram_ApplyStance_P1
// Phase 0, not charging.
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Reshiram_ApplyStance_86
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Reshiram_ApplyStance_89
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Reshiram_ApplyStance_91
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Reshiram_ApplyStance_86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Reshiram_ApplyStance_89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Reshiram_ApplyStance_91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Reshiram_ApplyStance_P1:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Reshiram_ApplyStance_84
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Reshiram_ApplyStance_89
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Reshiram_ApplyStance_92
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Reshiram_ApplyStance_84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Reshiram_ApplyStance_92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
// Charging, in Phase 0 or 1. Only Phase 1 can actually reach here - _BeginCharge is gated on
// Phase == 1 - but the Phase 0 arm costs four lines and removes the assumption.
EncScript_Reshiram_ApplyStance_Hot:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Reshiram_ApplyStance_HotP1
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Reshiram_ApplyStance_80
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Reshiram_ApplyStance_83
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Reshiram_ApplyStance_85
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return
EncScript_Reshiram_ApplyStance_83:
	encsetdamagereduction ENC_TARGET_BOSS, 83
	return
EncScript_Reshiram_ApplyStance_85:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Reshiram_ApplyStance_HotP1:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Reshiram_ApplyStance_78
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Reshiram_ApplyStance_83
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Reshiram_ApplyStance_86
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Reshiram_ApplyStance_78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
// Phase 2: it has stopped defending. Flat, no band, and the Judgment charge takes another 8.
EncScript_Reshiram_ApplyStance_P2:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Reshiram_ApplyStance_88
	encjumpifvar CMP_GREATER_THAN, 11, 3, EncScript_Reshiram_ApplyStance_88
EncScript_Reshiram_ApplyStance_80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Reshiram_ApplyStance_88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
EncScript_Reshiram_ApplyStance_Done:
	return

// THE BAND CALLOUT (guideline 14). LastBand holds the last ANNOUNCED band; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a difference - in BOTH directions. The falling lines are as load-bearing as the rising ones: in
// Phase 1 a fall is the player's own work and the fight has no other way to confirm it did anything.
EncScript_Reshiram_Band:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Reshiram_Band_Unread
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Reshiram_Band_Studied
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Reshiram_Band_Understood
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Reshiram_Band_Done
	encsetvar 2, 3
	printstring STRINGID_ENCRESHIRAMBANDKNOWN
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Reshiram_Band_Unread:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Reshiram_Band_Done
	encsetvar 2, 0
	printstring STRINGID_ENCRESHIRAMBANDUNREAD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Reshiram_Band_Studied:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Reshiram_Band_Done
	encsetvar 2, 1
	printstring STRINGID_ENCRESHIRAMBANDSTUDIED
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Reshiram_Band_Understood:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Reshiram_Band_Done
	encsetvar 2, 2
	printstring STRINGID_ENCRESHIRAMBANDUNDERSTOOD
	waitmessage B_WAIT_TIME_LONG
EncScript_Reshiram_Band_Done:
	return

// Revelation falls by one, floored at 0. Silent - every caller prints its own line.
EncScript_Reshiram_RevDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Reshiram_RevDown_Done
	encsubvar 1, 1
EncScript_Reshiram_RevDown_Done:
	return

// RE-APPLY TRUTH MARK at its stored depth, written absolutely so it survives anything that landed in
// between. A depth of 0 returns without touching a stage, so this is safe to call unconditionally -
// which is what every scour in the fight does, because none of them is meant to take the mark with
// it. Silent; the caller supplies the line.
EncScript_Reshiram_ApplyMark:
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Reshiram_ApplyMark_1
	encjumpifvar CMP_EQUAL, 9, 2, EncScript_Reshiram_ApplyMark_2
	encjumpifvar CMP_EQUAL, 9, 3, EncScript_Reshiram_ApplyMark_3
	return
EncScript_Reshiram_ApplyMark_1:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -7
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -7
	return
EncScript_Reshiram_ApplyMark_2:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -8
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -8
	return
EncScript_Reshiram_ApplyMark_3:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -9
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -9
	return

// TRUTH FLAME. Called from _TurnClose when the cycle reaches 0, and deliberately SYMMETRICAL: the
// stat scour takes Reshiram's own Reveal stack with it, which is what stops Reveal towering to +6
// Def over a long fight. The mark is put back afterwards because it is the one thing the flame is
// not meant to burn off. Each test-and-clear command chains to the next step on its fail label, so a
// bare field costs nothing.
EncScript_Reshiram_Flame:
	printstring STRINGID_ENCRESHIRAMTRUTHFLAME
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
	removeweather
	removeterrain
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Reshiram_Flame_Hazards
EncScript_Reshiram_Flame_Hazards:
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Reshiram_Flame_Stats
EncScript_Reshiram_Flame_Stats:
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Reshiram_Flame_Heal
EncScript_Reshiram_Flame_Heal:
	encsethealblock ENC_TARGET_ALL_FOES, 3
	printstring STRINGID_ENCRESHIRAMHEALINGDENIED
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	call EncScript_Reshiram_ApplyMark
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Reshiram_Flame_CycleP0
	encsetvar 10, 3
	return
EncScript_Reshiram_Flame_CycleP0:
	encsetvar 10, 4
	return

// A CHARGE LANDS. Blue Flare in Phase 1, TRUTH'S JUDGMENT in Phase 2. Both are percentages of the
// PLAYER's max HP, so they scale with the player's team rather than with Reshiram's, and both are
// direct HP writes that a Substitute never sees.
EncScript_Reshiram_FireCharge:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Reshiram_FireCharge_Judgment
	printstring STRINGID_ENCRESHIRAMBLUEFLARE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Reshiram_FireCharge_25
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Reshiram_FireCharge_35
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Reshiram_FireCharge_Spent
EncScript_Reshiram_FireCharge_35:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Reshiram_FireCharge_Spent
EncScript_Reshiram_FireCharge_25:
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
EncScript_Reshiram_FireCharge_Spent:
	encsetvar 11, 0    @ Charge
	encsetvar 10, 3    @ it spent itself; the Truth Flame cycle restarts
	call EncScript_Reshiram_ApplyStance
	return
// The Judgment cannot be broken and does not repeat. Charge 9 is what the Phase 3 trigger watches.
EncScript_Reshiram_FireCharge_Judgment:
	printstring STRINGID_ENCRESHIRAMJUDGMENTFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -25, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPDEF, -1
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	encsetvar 11, 9
	call EncScript_Reshiram_ApplyStance
	return

// --- Trigger entry points ---

// OnBattleStart. _ApplyStance runs on turn 0 so the guard already reflects the table rather than the
// Properties: seed, and LastBand starts at 9 so the opening band line is a real announcement.
// No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Reshiram_Intro::
	printstring STRINGID_ENCRESHIRAMINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCRESHIRAMINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	call EncScript_Reshiram_ApplyStance
	encsetvar 10, 4    @ Flame
	encsetvar 2, 9     @ LastBand: nothing announced yet
	call EncScript_Reshiram_Band
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch out can leave it re-armed.
EncScript_Reshiram_TurnOpen::
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET
	enccopyvar 7, 6    @ LastRead = Read
	encsetvar 6, 0     @ Read: re-arms _Reveal
	encsetvar 8, 0     @ Fed
	encsetvar 13, 0    @ Pressure
	encsetvar 12, 0    @ EventGuard - a switch that landed after _TurnClose left this set
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_Reshiram_TurnOpen_Flame
	encsubvar 14, 1    @ Exposed
EncScript_Reshiram_TurnOpen_Flame:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Reshiram_TurnOpen_Charge   @ Phase 2: no flame
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Reshiram_TurnOpen_Charge
	encsubvar 10, 1
	encjumpifvar CMP_NOT_EQUAL, 10, 1, EncScript_Reshiram_TurnOpen_Charge
	printstring STRINGID_ENCRESHIRAMFLAMEGATHERS
	waitmessage B_WAIT_TIME_LONG
EncScript_Reshiram_TurnOpen_Charge:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Reshiram_TurnOpen_White
	encjumpifvar CMP_GREATER_THAN, 11, 3, EncScript_Reshiram_TurnOpen_White   @ 9: already resolved
	encsubvar 11, 1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Reshiram_TurnOpen_JudgmentCue
	printstring STRINGID_ENCRESHIRAMCHARGECUE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Reshiram_TurnOpen_White
EncScript_Reshiram_TurnOpen_JudgmentCue:
	printstring STRINGID_ENCRESHIRAMJUDGMENTCUE
	waitmessage B_WAIT_TIME_SHORT
// WHITE LIGHT's per-turn scour: no evasion boost and no setup survives a turn in Phase 2.
EncScript_Reshiram_TurnOpen_White:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Reshiram_TurnOpen_Cue
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Reshiram_TurnOpen_WhiteMark
EncScript_Reshiram_TurnOpen_WhiteMark:
	call EncScript_Reshiram_ApplyMark
// The recurring cue (guideline 14). Alternated so a long fight does not read as a stuck message.
EncScript_Reshiram_TurnOpen_Cue:
	call EncScript_Reshiram_Band
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Reshiram_TurnOpen_Done
	encjumpifvar CMP_EQUAL, 15, 0, EncScript_Reshiram_TurnOpen_CueA
	encsetvar 15, 0
	printstring STRINGID_ENCRESHIRAMNOTHINGHIDDEN
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Reshiram_TurnOpen_CueA:
	encsetvar 15, 1
	printstring STRINGID_ENCRESHIRAMSTILLWATCHING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Reshiram_TurnOpen_Done:
	return

// REVEAL. encstoreprediction reads the engine's real AI_FLAG_PREDICT_MOVE answer, so outguessing it
// is a genuine act rather than a scripted coin flip. Reshiram says what it read and raises the
// matching defence for the turn - the counter is "change category", and it is meant to be worked out
// from the two lines together. The status arm is the one that ATTACKS instead of defending: a
// Pokemon built to see through you is built to beat setup turns.
// It can decline to answer (0). That writes 9 rather than leaving Read at 0, because Read == 0 is
// this trigger's own condition and a script that returns without closing its latch re-selects
// forever. 9 is also outside the 1-3 the repeat test compares, so a quiet turn never reads as a
// repeat on the next one.
EncScript_Reshiram_Reveal::
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 6
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Reshiram_Reveal_Physical
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Reshiram_Reveal_Special
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Reshiram_Reveal_Status
	encsetvar 6, 9
	return
EncScript_Reshiram_Reveal_Physical:
	printstring STRINGID_ENCRESHIRAMREADPHYSICAL
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Reshiram_Reveal_Repeat
	goto EncScript_Reshiram_Reveal_Show
EncScript_Reshiram_Reveal_Special:
	printstring STRINGID_ENCRESHIRAMREADSPECIAL
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Reshiram_Reveal_Repeat
	goto EncScript_Reshiram_Reveal_Show
EncScript_Reshiram_Reveal_Status:
	printstring STRINGID_ENCRESHIRAMREADSTATUS
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Reshiram_Reveal_Repeat
EncScript_Reshiram_Reveal_Show:
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return
// The tutorial line for the repeat rule, and it fires every turn the player keeps repeating.
EncScript_Reshiram_Reveal_Repeat:
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCRESHIRAMSEENTHATBEFORE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 8, 1     @ Fed
	return

// EXPOSE, the targeted strip. Each test-and-clear chains to the next on its fail label; Embargo has
// nothing to test, so it only runs when an earlier step actually landed. A bare field spends a
// one-turn latch rather than the full cooldown, so the strip is waiting again the moment the player
// builds something - the latch exists only because the trigger has to close it to stop re-selecting.
// Scratch is free at OnTurnStart - _TurnClose rewrites it from BossMark before reading it - so it
// carries "something landed" across the three steps here.
EncScript_Reshiram_Expose::
	encsetvar 5, 0
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Reshiram_Expose_Stats
	encsetvar 5, 1
EncScript_Reshiram_Expose_Stats:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Reshiram_Expose_Hazards
	encsetvar 5, 1
EncScript_Reshiram_Expose_Hazards:
	encclearhazards ENC_TARGET_ALL_FOES, EncScript_Reshiram_Expose_Check
	encsetvar 5, 1
EncScript_Reshiram_Expose_Check:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Reshiram_Expose_Bare
	encsetembargo ENC_TARGET_ALL_FOES, 3
	printstring STRINGID_ENCRESHIRAMEXPOSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_ITEM_KNOCKOFF
	call EncScript_Reshiram_ApplyMark   @ the stat reset above took the mark with it
	encsetvar 14, 3    @ Exposed
	return
EncScript_Reshiram_Expose_Bare:
	encsetvar 14, 1    @ nothing to take; ready again next turn
	return

// TRUTH MARK is laid once, on whoever is standing there, and deepens at the ledger instead of here.
EncScript_Reshiram_Mark::
	printstring STRINGID_ENCRESHIRAMTRUTHMARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 9, 1     @ MarkDepth
	call EncScript_Reshiram_ApplyMark
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	return

// THE BLUE FLARE TELEGRAPH, announced before the player picks a move so the turn can be spent on the
// answer. Three charge turns is two full turns of play after the announcement.
EncScript_Reshiram_BeginCharge::
	printstring STRINGID_ENCRESHIRAMIDENTIFIEDTARGET
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	encsetvar 11, 3    @ Charge
	call EncScript_Reshiram_ApplyStance   @ concentration costs it cover
	return

// Somebody walked in. Switching is information, so it feeds the meter at the ledger; the mark
// follows the newcomer, which is the spec's "the Mark transfers". A gathering charge is NOT reset -
// switching out of a Blue Flare does not buy time.
EncScript_Reshiram_AttentionShifts::
	encsetvar 12, 1    @ EventGuard
	encsetvar 8, 1     @ Fed
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Reshiram_AttentionShifts_Charge
	printstring STRINGID_ENCRESHIRAMATTENTIONSHIFTS
	waitmessage B_WAIT_TIME_LONG
	encsetvar 9, 1     @ MarkDepth: re-laid at depth 1 on the newcomer
	call EncScript_Reshiram_ApplyMark
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
EncScript_Reshiram_AttentionShifts_Charge:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Reshiram_AttentionShifts_Done
	encjumpifvar CMP_GREATER_THAN, 11, 3, EncScript_Reshiram_AttentionShifts_Done
	printstring STRINGID_ENCRESHIRAMRETARGET
	waitmessage B_WAIT_TIME_LONG
EncScript_Reshiram_AttentionShifts_Done:
	return

// THE LEDGER. Measures the turn, deepens or breaks what the turn earned, and only then moves the
// meter - so the guard follows the meter and the callout follows the guard.
EncScript_Reshiram_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Reshiram_TurnClose_Quiet
	encsetvar 5, 0     @ Scratch
	enccopyvar 5, 4    @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Reshiram_TurnClose_NoDamage
	goto EncScript_Reshiram_TurnClose_Measured
EncScript_Reshiram_TurnClose_NoDamage:
	encsetvar 5, 0     @ nothing taken off Reshiram this turn
EncScript_Reshiram_TurnClose_Measured:
	encjumpifvar CMP_LESS_THAN, 5, 5, EncScript_Reshiram_TurnClose_Deepen
	encsetvar 13, 1    @ Pressure
EncScript_Reshiram_TurnClose_Deepen:
	encsetvar 5, 0     @ Scratch is free from here; the ledger reuses it
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Reshiram_TurnClose_Charge
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Reshiram_TurnClose_Charge
	encjumpifvar CMP_GREATER_THAN, 9, 2, EncScript_Reshiram_TurnClose_Charge
	encaddvar 9, 1     @ the mark grows only for a Pokemon that is actually hurting it
	printstring STRINGID_ENCRESHIRAMMARKDEEPENS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Reshiram_ApplyMark
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
// The charge resolves, breaks, or keeps gathering. A break is Phase 0-1 only: TRUTH'S JUDGMENT
// cannot be interrupted.
EncScript_Reshiram_TurnClose_Charge:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Reshiram_TurnClose_Flame
	encjumpifvar CMP_GREATER_THAN, 11, 3, EncScript_Reshiram_TurnClose_Flame
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Reshiram_TurnClose_Fire
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Reshiram_TurnClose_Flame
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Reshiram_TurnClose_Flame
	printstring STRINGID_ENCRESHIRAMLOSTTHETHREAD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetvar 11, 0
	call EncScript_Reshiram_RevDown
	encsetvar 5, 1     @ the meter went the player's way
	goto EncScript_Reshiram_TurnClose_Flame
EncScript_Reshiram_TurnClose_Fire:
	call EncScript_Reshiram_FireCharge
EncScript_Reshiram_TurnClose_Flame:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Reshiram_TurnClose_Ledger
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Reshiram_TurnClose_Ledger
	call EncScript_Reshiram_Flame
// THE REVELATION LEDGER. Phase 0 climbs on its own and the player can only slow it. Phase 1 has
// flipped: standing still is what costs Reshiram its certainty. Phase 2 skips the ledger - Rev is
// pinned at 5 and the guard is flat.
EncScript_Reshiram_TurnClose_Ledger:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Reshiram_TurnClose_Clamp
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Reshiram_TurnClose_LedgerP1
	encaddvar 1, 1     @ Phase 0: it learns a little every turn no matter what
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Reshiram_TurnClose_LedgerP0Press
	encaddvar 1, 1
EncScript_Reshiram_TurnClose_LedgerP0Press:
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Reshiram_TurnClose_Clamp
	call EncScript_Reshiram_RevDown
	encsetvar 5, 1
	goto EncScript_Reshiram_TurnClose_Clamp
EncScript_Reshiram_TurnClose_LedgerP1:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Reshiram_TurnClose_LedgerP1Vary
	encaddvar 1, 1     @ repeated a category, or switched
	goto EncScript_Reshiram_TurnClose_LedgerP1Press
EncScript_Reshiram_TurnClose_LedgerP1Vary:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Reshiram_TurnClose_LedgerP1Press
	call EncScript_Reshiram_RevDown
	encsetvar 5, 1
EncScript_Reshiram_TurnClose_LedgerP1Press:
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Reshiram_TurnClose_Clamp
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Reshiram_TurnClose_Clamp
	call EncScript_Reshiram_RevDown
	encsetvar 5, 1
EncScript_Reshiram_TurnClose_Clamp:
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Reshiram_TurnClose_Confirm
	encsetvar 1, 5
// Positive confirmation of the right behaviour, on top of the band line - which only speaks when the
// band itself changed, and a fall inside a band would otherwise be silent.
EncScript_Reshiram_TurnClose_Confirm:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Reshiram_TurnClose_Settle
	printstring STRINGID_ENCRESHIRAMNOTWHATITEXPECTED
	waitmessage B_WAIT_TIME_LONG
EncScript_Reshiram_TurnClose_Settle:
	call EncScript_Reshiram_ApplyStance
	call EncScript_Reshiram_Band
EncScript_Reshiram_TurnClose_Quiet:
	encsetvar 8, 0     @ Fed
	encsetvar 13, 0    @ Pressure
	encsetvar 12, 0    @ EventGuard
	encsetvar 3, 0     @ TurnGuard
	return

// --- Phase transitions ---

// 50% - THE TRUTH REVEALED. The moveset swap and the flipped ledger are announced as one beat, so
// the player gets a single clear "the rules changed" moment to reason from. Revelation is forced to
// 5 and STOPS CLIMBING ON ITS OWN: from here it is the player's to grind down.
EncScript_Reshiram_TruthRevealed::
	printstring STRINGID_ENCRESHIRAMSEENTHETRUTH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 1     @ Phase
	encsetvar 1, 5     @ Rev
	encsetmove ENC_TARGET_BOSS, 2, MOVE_FUSION_FLARE   @ Extrasensory out, the signature in
	encsetvar 10, 3    @ the Truth Flame cycle tightens
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Reshiram_TruthRevealed_Mark
EncScript_Reshiram_TruthRevealed_Mark:
	call EncScript_Reshiram_ApplyMark
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	call EncScript_Reshiram_ApplyStance
	call EncScript_Reshiram_Band
	printstring STRINGID_ENCRESHIRAMMAKEITDOUBT
	waitmessage B_WAIT_TIME_LONG
	return

// 15% - THE TRUTH CANNOT BE HIDDEN. WHITE LIGHT is Gravity (every accuracy rises, the field is
// grounded, and semi-invulnerability stops working - nowhere left to hide) plus Mist on Reshiram's
// own side (its accuracy cannot be lowered) plus the per-turn scour _TurnOpen runs in Phase 2.
// The guard DROPS to a flat 88: it has stopped defending and started burning.
// Deliberately NOT B_ANIM_TERA_CHARGE, which blends the screen white and never blends back.
// curestatus is the fight's one status displacement, at 15% HP, after status has been allowed to
// work completely up to that point (guideline 8).
EncScript_Reshiram_WhiteLight::
	printstring STRINGID_ENCRESHIRAMWHITELIGHT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 2     @ Phase
	encsetvar 1, 5     @ Rev
	removeweather
	removeterrain
	encsetfieldstatus ENC_FIELD_GRAVITY
	encsetsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_MIST
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Reshiram_WhiteLight_Mark
EncScript_Reshiram_WhiteLight_Mark:
	call EncScript_Reshiram_ApplyMark
	curestatus BS_OPPONENT1
	encsetvar 11, 3    @ TRUTH'S JUDGMENT begins charging immediately
	call EncScript_Reshiram_ApplyStance
	printstring STRINGID_ENCRESHIRAMJUDGMENT
	waitmessage B_WAIT_TIME_LONG
	return

// SPENT, and the catch window (guideline 10). Reached three ways - the Judgment resolving, or the
// <= 1% deadlock fallback at either checkpoint - and Once on all of them, so the first selection
// disables the rest and Phase 3 closes the conditions on the others. The teardown is the exact
// mirror of the Properties: block plus everything Phase 2 raised, so nothing the fight imposed
// survives into the window. _ApplyStance returns immediately at Phase 3, so nothing can re-raise the
// guard behind it, and encsetdamagereduction 99 writes the value the automatic catch guard will
// RESTORE, not the live one.
EncScript_Reshiram_Spent::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCRESHIRAMFLAMESFADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Rev
	encsetvar 9, 0     @ MarkDepth
	encsetvar 10, 0    @ Flame
	encsetvar 11, 0    @ Charge
	encclearfieldstatus ENC_FIELD_GRAVITY
	encclearsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_MIST
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encsetembargo ENC_TARGET_ALL_FOES, 0
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Reshiram_Spent_NoStats
EncScript_Reshiram_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCRESHIRAMWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
