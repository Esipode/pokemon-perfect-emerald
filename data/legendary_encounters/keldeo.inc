// Keldeo, "The Colt Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Keldeo_Intro:
// 0 Phase (0 Apprentice / 1 Mentors' Teachings / 2 True Resolve / 3 Spent), 1 Resolve (0-5),
// 2 LastRes (the last ANNOUNCED Resolve), 3 Rank (revocable perks applied: 0 none / 1 Speed /
// 2 Speed + crit), 4 TurnGuard, 5 Duel (0 none / 1 running), 6 DuelTurns, 7 DuelCd, 8 BossMark
// (Keldeo HP% at turn open), 9 Scratch (the copy ENC_SNAP_DAMAGE consumes), 10 Charge (0 none /
// 1 blade raised / 2 reeling), 11 EventGuard, 12 Riposted (0 / 1 armed / 2 answered), 13 Gained
// (super-effective +1 taken this turn), 14 Resolute (form fired, meter locked at 5), 15 Ultimate
// (Sacred Swords landed).
//
// THE DUEL IS THE GUARD: EncScript_Keldeo_ApplyStance is the damage reduction's only writer, and
// whether a duel is running is its real input. RESOLVE is pure offence - it never touches the guard.
// The player is playing two clocks: win duels and the windows keep coming; lose them and Resolve
// climbs to 5, where Keldeo becomes Resolute and the duels stop for good.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction, written absolutely from Phase, Duel and Charge, so a
// duel ending undoes a duel starting with no separate teardown path. Phase 3 returns immediately:
// once balls are allowed the engine's own catch guard owns the number.
EncScript_Keldeo_ApplyStance:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Keldeo_ApplyStance_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Keldeo_ApplyStance_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Keldeo_ApplyStance_P1
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Keldeo_ApplyStance_P0Duel
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Keldeo_ApplyStance_P0Reel
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Keldeo_ApplyStance_P0Duel:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Keldeo_ApplyStance_P0Reel:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
EncScript_Keldeo_ApplyStance_P1:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Keldeo_ApplyStance_P1Duel
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Keldeo_ApplyStance_P1Reel
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Keldeo_ApplyStance_P1Duel:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Keldeo_ApplyStance_P1Reel:
	encsetdamagereduction ENC_TARGET_BOSS, 79
	return
EncScript_Keldeo_ApplyStance_P2:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Keldeo_ApplyStance_P2Reel
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Keldeo_ApplyStance_P2Reel:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Keldeo_ApplyStance_Done:
	return

// THE RESOLVE CALLOUT (guideline 14). LastRes holds the last ANNOUNCED level; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a difference - in BOTH directions. Level 0's line is as load-bearing as level 5's: a fall is the
// player's own work and the fight has no other way to confirm it.
EncScript_Keldeo_AnnounceResolve:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Keldeo_AnnounceResolve_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Keldeo_AnnounceResolve_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Keldeo_AnnounceResolve_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Keldeo_AnnounceResolve_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Keldeo_AnnounceResolve_4
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Keldeo_AnnounceResolve_Done
	printstring STRINGID_ENCKELDEORESOLVE5
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Keldeo_AnnounceResolve_Latch
EncScript_Keldeo_AnnounceResolve_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Keldeo_AnnounceResolve_Done
	printstring STRINGID_ENCKELDEORESOLVE0
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Keldeo_AnnounceResolve_Latch
EncScript_Keldeo_AnnounceResolve_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Keldeo_AnnounceResolve_Done
	printstring STRINGID_ENCKELDEORESOLVE1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Keldeo_AnnounceResolve_Latch
EncScript_Keldeo_AnnounceResolve_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Keldeo_AnnounceResolve_Done
	printstring STRINGID_ENCKELDEORESOLVE2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Keldeo_AnnounceResolve_Latch
EncScript_Keldeo_AnnounceResolve_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Keldeo_AnnounceResolve_Done
	printstring STRINGID_ENCKELDEORESOLVE3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Keldeo_AnnounceResolve_Latch
EncScript_Keldeo_AnnounceResolve_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Keldeo_AnnounceResolve_Done
	printstring STRINGID_ENCKELDEORESOLVE4
	waitmessage B_WAIT_TIME_LONG
EncScript_Keldeo_AnnounceResolve_Latch:
	enccopyvar 2, 1     @ LastRes = Resolve
EncScript_Keldeo_AnnounceResolve_Done:
	return

// Grants whichever revocable perk the current Resolve has earned but Rank has not yet applied, one
// step at a time. The Resolve callout carries the dialogue for both ranks, so these are silent.
EncScript_Keldeo_ApplyRank:
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Keldeo_ApplyRank_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Keldeo_ApplyRank_Two
	encsetvar 3, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
EncScript_Keldeo_ApplyRank_Two:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Keldeo_ApplyRank_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 1, EncScript_Keldeo_ApplyRank_Done
	encsetvar 3, 2
	encsetcrit ENC_TARGET_BOSS, TRUE
EncScript_Keldeo_ApplyRank_Done:
	return

// The mirror, taking Rank back down toward the current Resolve. Arithmetic rather than a snapshot,
// kept small against the +-6 stage clamp.
EncScript_Keldeo_RevokeRank:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Keldeo_RevokeRank_FromTwo
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Keldeo_RevokeRank_FromOne
	return
EncScript_Keldeo_RevokeRank_FromTwo:
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Keldeo_RevokeRank_Done
	encsetcrit ENC_TARGET_BOSS, FALSE
	encsetvar 3, 1
EncScript_Keldeo_RevokeRank_FromOne:
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Keldeo_RevokeRank_Done
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -1
	encsetvar 3, 0
EncScript_Keldeo_RevokeRank_Done:
	return

// Resolve +1. NOT announced here - the turn's scripts call _AnnounceResolve once at the end, so
// however many times the meter moves the player reads one line about it. Returns immediately once
// Resolute has latched (the lock), clamps at 5, applies whichever rank it just earned, and at 5
// drives _BecomeResolute.
EncScript_Keldeo_GainOne:
	encjumpifvar CMP_EQUAL, 14, 1, EncScript_Keldeo_GainOne_Done       @ Resolute lock
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Keldeo_GainOne_Rise
	call EncScript_Keldeo_BecomeResolute                               @ already at 5, not yet Resolute
	return
EncScript_Keldeo_GainOne_Rise:
	encaddvar 1, 1
	call EncScript_Keldeo_ApplyRank
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Keldeo_GainOne_Done
	call EncScript_Keldeo_BecomeResolute
EncScript_Keldeo_GainOne_Done:
	return

// Resolve -1. The lock floors it - a Resolute Keldeo never falls. Floors at 0 and revokes whatever
// rank it just lost.
EncScript_Keldeo_Lose:
	encjumpifvar CMP_EQUAL, 14, 1, EncScript_Keldeo_Lose_Done          @ Resolute lock
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Keldeo_Lose_Done
	encsubvar 1, 1
	call EncScript_Keldeo_RevokeRank
EncScript_Keldeo_Lose_Done:
	return

// RESOLUTE. Latches the lock, pins Resolve at 5, prints its two lines, changes form, arms Secret
// Sword in slot 3, raises raw Sp. Atk / Speed, makes sure both ranks are applied, and ends the
// current duel if one is running - Keldeo has nothing left to prove.
EncScript_Keldeo_BecomeResolute:
	encsetvar 14, 1     @ Resolute lock
	encsetvar 1, 5      @ Resolve pinned at 5
	printstring STRINGID_ENCKELDEORESOLUTE1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKELDEORESOLUTE2
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_KELDEO_RESOLUTE, EncScript_Keldeo_BecomeResolute_Move, B_ANIM_FORM_CHANGE
EncScript_Keldeo_BecomeResolute_Move:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SECRET_SWORD
	restoremovepp BS_OPPONENT1
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 15, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 10, ENC_AMOUNT_PERCENT
	call EncScript_Keldeo_ApplyRank
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Keldeo_BecomeResolute_EndDuel
	call EncScript_Keldeo_ApplyStance
	return
EncScript_Keldeo_BecomeResolute_EndDuel:
	call EncScript_Keldeo_EndDuel
	return

// Ends a running duel and starts its cooldown, then restores the guard absolutely.
EncScript_Keldeo_EndDuel:
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 5, 0     @ Duel
	encsetvar 6, 0     @ DuelTurns
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Keldeo_EndDuel_Cd3
	encsetvar 7, 2     @ DuelCd, Phase 1+
	goto EncScript_Keldeo_EndDuel_Fin
EncScript_Keldeo_EndDuel_Cd3:
	encsetvar 7, 3     @ DuelCd, Phase 0
EncScript_Keldeo_EndDuel_Fin:
	call EncScript_Keldeo_ApplyStance
	printstring STRINGID_ENCKELDEODUELEND
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger scripts ---

EncScript_Keldeo_Intro::
	printstring STRINGID_ENCKELDEOINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKELDEOINTRO2
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Keldeo_ApplyStance
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that prints before any
// animation renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch can leave it re-armed. Ticks the duel cooldown and the duel timer (the
// decrement half of the split countdown), takes the HP mark the turn is measured against as a
// percentage (the only portable choice under Level: LevelCap and New Game Plus offsets), runs the
// recurring cue, and clears the per-turn state.
EncScript_Keldeo_TurnOpen::
	flushtextbox
	encsetvar 4, 1     @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Keldeo_TurnOpen_Clear   @ Phase 3: quiet
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Keldeo_TurnOpen_DuelTick
	encsubvar 7, 1     @ DuelCd
EncScript_Keldeo_TurnOpen_DuelTick:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Keldeo_TurnOpen_Mark
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Keldeo_TurnOpen_Mark
	encsubvar 6, 1     @ DuelTurns
EncScript_Keldeo_TurnOpen_Mark:
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_SET
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Keldeo_TurnOpen_DuelCue
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Keldeo_TurnOpen_ReelCue
	goto EncScript_Keldeo_TurnOpen_Clear
EncScript_Keldeo_TurnOpen_DuelCue:
	printstring STRINGID_ENCKELDEODUELREMIND
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Keldeo_TurnOpen_Clear
EncScript_Keldeo_TurnOpen_ReelCue:
	printstring STRINGID_ENCKELDEOSWORDOPEN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Keldeo_TurnOpen_Clear:
	encsetvar 11, 0    @ EventGuard
	encsetvar 12, 0    @ Riposted
	encsetvar 13, 0    @ Gained
	return

// Phase 1+ anti-abuse: a status is shed at the top of every turn - it keeps working but costs a move
// and feeds Resolve +1. Self-disabling: it cures the status its own trigger condition reads.
EncScript_Keldeo_Shed::
	printstring STRINGID_ENCKELDEOSHEDS
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_AnnounceResolve
	return

// THE CHALLENGE - the fight's centrepiece. Traps the player's active Pokemon for the duel's length,
// drops the guard, and from Resolve 4 opens with THE DISARMING STROKE: the stat stages and screens
// are wiped. Both of those commands are test-and-clear, so the flourish is simply skipped when there
// was nothing to strip.
EncScript_Keldeo_Challenge::
	printstring STRINGID_ENCKELDEOCHALLENGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	encsetvar 5, 1     @ Duel
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Keldeo_Challenge_P0Turns
	encsetvar 6, 4     @ DuelTurns, Phase 1
	goto EncScript_Keldeo_Challenge_Disarm
EncScript_Keldeo_Challenge_P0Turns:
	encsetvar 6, 3     @ DuelTurns, Phase 0
EncScript_Keldeo_Challenge_Disarm:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Keldeo_Challenge_Guard
	printstring STRINGID_ENCKELDEODISARM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Keldeo_Challenge_DisarmScreens
EncScript_Keldeo_Challenge_DisarmScreens:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Keldeo_Challenge_DisarmAnim
EncScript_Keldeo_Challenge_DisarmAnim:
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
EncScript_Keldeo_Challenge_Guard:
	call EncScript_Keldeo_ApplyStance
	printstring STRINGID_ENCKELDEODUELGUARD
	waitmessage B_WAIT_TIME_LONG
	return

// THE SACRED SWORD telegraph (Resolute only). Set at OnTurnStart so the player sees it and can spend
// the turn on the interrupt; _SwordResolve resolves it at OnTurnEnd the same turn.
EncScript_Keldeo_RaiseBlade::
	encsetvar 10, 1    @ Charge: blade raised
	printstring STRINGID_ENCKELDEOBLADERAISED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

// TEMPERED. A Flying / Psychic / Electric / Grass / Fairy move landed on Keldeo. This is the fight's
// coverage trap: the obvious super-effective answer is the thing that raises the meter, and
// ABILITY_JUSTIFIED is the same trap one layer down for the neutral Dark answer. Self-disables
// through Gained (set in the trigger's Conditions path via this script).
EncScript_Keldeo_Tempered::
	encsetvar 13, 1    @ Gained
	printstring STRINGID_ENCKELDEOTEMPERED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_AnnounceResolve
	return

// Keldeo won the duel (it knocked out the Pokemon it challenged): +2, then the duel ends. EventGuard
// stops the replacement switch-in being processed a second time.
EncScript_Keldeo_DuelWon::
	encsetvar 11, 1    @ EventGuard
	printstring STRINGID_ENCKELDEODUELWON
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_AnnounceResolve
	call EncScript_Keldeo_EndDuel
	return

// A mid-turn switch (U-turn, Eject Button). Sets EventGuard and nothing else - it exists only so a
// faint later in the same turn is not double-counted as a duel win.
EncScript_Keldeo_TheyEntered::
	encsetvar 11, 1    @ EventGuard
	return

// THE LEDGER. Runs at OnTurnEnd priority 80 so it sees a finished turn. Measures the health the
// player took off Keldeo this turn: any amount arms Riposte, and the amount is left in Scratch for
// the sword-break test at priority 90. Self-disables by clearing TurnGuard.
EncScript_Keldeo_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Keldeo_TurnClose_Quiet   @ Phase 3: quiet
	encjumpifvar CMP_NOT_EQUAL, 10, 2, EncScript_Keldeo_TurnClose_Measure
	encsetvar 10, 0    @ Charge: the reeling turn is over
	call EncScript_Keldeo_ApplyStance
EncScript_Keldeo_TurnClose_Measure:
	encsetvar 9, 0     @ Scratch
	enccopyvar 9, 8    @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_DAMAGE, EncScript_Keldeo_TurnClose_NoDamage
	encsetvar 12, 1    @ Riposted armed
	goto EncScript_Keldeo_TurnClose_Announce
EncScript_Keldeo_TurnClose_NoDamage:
	encsetvar 9, 0     @ no health taken off Keldeo this turn
EncScript_Keldeo_TurnClose_Announce:
	call EncScript_Keldeo_AnnounceResolve
EncScript_Keldeo_TurnClose_Quiet:
	encsetvar 4, 0     @ TurnGuard
	return

// RIPOSTE (Resolve 3+, no duel running). Keldeo is not taking another turn - the counter answers a
// blow that already landed. Inside a duel Keldeo fights straight, which is why the trigger is gated
// on Duel == 0.
EncScript_Keldeo_Riposte::
	encsetvar 12, 2    @ Riposted answered
	printstring STRINGID_ENCKELDEORIPOSTE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_AQUA_JET
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Keldeo_Riposte_Hard   @ Resolve 4+
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Keldeo_Riposte_Hard:
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	return

// The duel timer ran out with the challenged Pokemon still standing: Keldeo's resolve falters, -1,
// and the duel ends.
EncScript_Keldeo_DuelSurvived::
	printstring STRINGID_ENCKELDEODUELSURVIVED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Keldeo_Lose
	call EncScript_Keldeo_AnnounceResolve
	call EncScript_Keldeo_EndDuel
	return

// THE SACRED SWORD test resolves, the same turn the blade was raised. Two interrupts, checked in
// order: a status on Keldeo, or 25% of its max HP taken this turn (left in Scratch by _TurnClose).
// Resolve is already locked at 5, so neither branch touches the meter - the spec's "gains
// significant Resolve" is the escalating damage instead.
EncScript_Keldeo_SwordResolve::
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Keldeo_SwordResolve_Broken
	encjumpifvar CMP_LESS_THAN, 9, 25, EncScript_Keldeo_SwordResolve_Lands
EncScript_Keldeo_SwordResolve_Broken:
	encsetvar 10, 2    @ Charge: reeling
	printstring STRINGID_ENCKELDEOSWORDBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	encsetrecharge ENC_TARGET_BOSS, 2      @ at OnTurnEnd = next turn's action
	call EncScript_Keldeo_ApplyStance
	printstring STRINGID_ENCKELDEOSWORDOPEN
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Keldeo_SwordResolve_Lands:
	encsetvar 10, 0    @ Charge cleared
	encaddvar 15, 1    @ Ultimate
	printstring STRINGID_ENCKELDEOSACREDSWORD
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SACRED_SWORD
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_GREATER_THAN, 15, 2, EncScript_Keldeo_SwordResolve_L3
	encjumpifvar CMP_EQUAL, 15, 2, EncScript_Keldeo_SwordResolve_L2
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	return
EncScript_Keldeo_SwordResolve_L2:
	encchangehp ENC_TARGET_ALL_FOES, -36, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCKELDEOAGAIN1
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Keldeo_SwordResolve_L3:
	encchangehp ENC_TARGET_ALL_FOES, -42, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCKELDEOAGAIN2
	waitmessage B_WAIT_TIME_LONG
	return

// --- Phases ---

// 50% - MENTORS' TEACHINGS. Slot 2 becomes Hydro Pump, raw Sp. Atk +15% (survives Haze), +2 Resolve,
// and the guard steps to 94/82. From here a status is shed every turn (and feeds the meter), duels
// last 4 turns instead of 3, and the cooldown between them falls to 2.
EncScript_Keldeo_Teachings::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCKELDEOTEACHINGS1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKELDEOTEACHINGS2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetmove ENC_TARGET_BOSS, 2, MOVE_HYDRO_PUMP
	restoremovepp BS_OPPONENT1
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 15, ENC_AMOUNT_PERCENT
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_AnnounceResolve
	call EncScript_Keldeo_ApplyStance
	printstring STRINGID_ENCKELDEOSHEDCUE
	waitmessage B_WAIT_TIME_LONG
	return

// 20% - TRUE RESOLVE. Resolve is forced to 5, which drives _BecomeResolute if it has not already
// fired, and any running duel ends. Survive: True carries the fight to 1 HP from here - the only
// damage window left is breaking the Sacred Sword mid-charge.
EncScript_Keldeo_TrueResolve::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCKELDEOTRUERESOLVE1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCKELDEOTRUERESOLVE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_GainOne
	call EncScript_Keldeo_AnnounceResolve
	encjumpifvar CMP_NOT_EQUAL, 5, 1, EncScript_Keldeo_TrueResolve_Stance
	call EncScript_Keldeo_EndDuel
	return
EncScript_Keldeo_TrueResolve_Stance:
	call EncScript_Keldeo_ApplyStance
	return

// 1 HP - SPENT, and the catch window (guideline 10). One last Secret Sword, then the guard, the
// immunities, the cap and Survive all come off and the ball is allowed. Every trigger that could
// still fire is gated Phase <= 2, so from here the fight is quiet. encsetdamagereduction 99 writes
// the value the automatic catch guard will RESTORE, not the live one.
EncScript_Keldeo_Spent::
	encsetvar 0, 3     @ Phase
	encsetvar 10, 0    @ Charge
	printstring STRINGID_ENCKELDEOFALLS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SECRET_SWORD
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCKELDEOSTILLSTANDING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Resolve
	call EncScript_Keldeo_RevokeRank
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 5, 0     @ Duel
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCKELDEOWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
