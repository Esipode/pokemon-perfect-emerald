// Meloetta, "The Melody Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Meloetta_Intro:
// 0 Phase (0 Overture / 1 Grand Performance / 2 The Final Note / 3 Spent), 1 Meter (the live meter,
// 0-5), 2 Rhythm (Aria's parked meter), 3 Tempo (Pirouette's parked meter), 4 Form (0 Aria /
// 1 Pirouette), 5 Beat (bar position, 1-4), 6 TurnGuard, 7 PlayerMark (the player's HP% at turn
// open), 8 Scratch (the copy ENC_SNAP_DAMAGE consumes; afterwards, the HP% the player lost this
// turn), 9 Charge (0 none / 2-1 turns left on a crescendo / 9 stumbling), 10 LastCall (the last
// ANNOUNCED beat state - 1 verse / 2 chorus / 3 gathering / 4 quiet), 11 LastMeter (the last
// ANNOUNCED meter level), 12 BeatClass (0 verse / 1 chorus), 13 EventGuard, 14 Encore (finales
// already performed), 15 StatusBreak (a break requested by a trigger condition).
//
// THE BAR OWNS THE GUARD: EncScript_Meloetta_ApplyStance is the damage reduction's only writer
// outside _Spent, and it reads Phase, Charge and the beat the bar is standing on. The meter does
// not make Meloetta harder to hurt - it decides how many of the bar's four beats are CHORUS.
//
// THE ONE RULE ABOUT THE METERS: _Rise and _Break write Meter and the active park var TOGETHER, and
// nothing else writes either - except the three places that reset both outright (_Resolve's two
// endings and _FinalNote) and the two form routes, which swap the live meter with its park.

// --- Shared subroutines (call/return) ---

// Reads Beat against the verse count the meter implies and leaves the answer in BeatClass, so the
// guard, the chorus chip and the beat callout can never disagree about what beat it is. The one
// place in the file that knows the shape of the bar.
//   Meter 0-1 -> 3 verse beats   2-3 -> 2   4 -> 1   5 -> none
EncScript_Meloetta_ClassifyBeat:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Meloetta_ClassifyBeat_Chorus
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Meloetta_ClassifyBeat_M4
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Meloetta_ClassifyBeat_M23
	encjumpifvar CMP_EQUAL, 5, 4, EncScript_Meloetta_ClassifyBeat_Chorus
	goto EncScript_Meloetta_ClassifyBeat_Verse
EncScript_Meloetta_ClassifyBeat_M23:
	encjumpifvar CMP_GREATER_THAN, 5, 2, EncScript_Meloetta_ClassifyBeat_Chorus
	goto EncScript_Meloetta_ClassifyBeat_Verse
EncScript_Meloetta_ClassifyBeat_M4:
	encjumpifvar CMP_GREATER_THAN, 5, 1, EncScript_Meloetta_ClassifyBeat_Chorus
EncScript_Meloetta_ClassifyBeat_Verse:
	encsetvar 12, 0
	return
EncScript_Meloetta_ClassifyBeat_Chorus:
	encsetvar 12, 1
	return

// THE SINGLE WRITER of the damage reduction, written absolutely from Phase, Charge and the beat, so
// a meter falling two levels in one turn undoes a meter rising with no separate teardown path.
// Phase 3 returns immediately: once balls are allowed the engine's own catch guard owns the number.
//                    VERSE   CHORUS
//   Phase 0            84      93
//   Phase 1            86      95
//   Phase 2          a flat 82 - the song has stopped
//   charging a crescendo +3 on whichever beat is running
//   stumbling out of a broken one, a flat 72 for the whole of the next turn
EncScript_Meloetta_ApplyStance:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Meloetta_ApplyStance_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Meloetta_ApplyStance_Quiet
	encjumpifvar CMP_EQUAL, 9, 9, EncScript_Meloetta_ApplyStance_Stumble
	call EncScript_Meloetta_ClassifyBeat
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Meloetta_ApplyStance_Charging
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Meloetta_ApplyStance_P1
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Meloetta_ApplyStance_P0Chorus
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Meloetta_ApplyStance_P0Chorus:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Meloetta_ApplyStance_P1:
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Meloetta_ApplyStance_P1Chorus
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Meloetta_ApplyStance_P1Chorus:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
	@ Gathering a crescendo: the same beat, three points harder.
EncScript_Meloetta_ApplyStance_Charging:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Meloetta_ApplyStance_CP1
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Meloetta_ApplyStance_CP0Chorus
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return
EncScript_Meloetta_ApplyStance_CP0Chorus:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Meloetta_ApplyStance_CP1:
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Meloetta_ApplyStance_CP1Chorus
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Meloetta_ApplyStance_CP1Chorus:
	encsetdamagereduction ENC_TARGET_BOSS, 98
	return
	@ The broken crescendo. Beat-independent and phase-independent: what the song gathered is not
	@ protecting Meloetta while it is coming apart.
EncScript_Meloetta_ApplyStance_Stumble:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Meloetta_ApplyStance_Quiet:
	encsetdamagereduction ENC_TARGET_BOSS, 82
EncScript_Meloetta_ApplyStance_Done:
	return

// THE METER CALLOUT (guideline 14). LastMeter holds the last ANNOUNCED level; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a difference - in BOTH directions. A falling meter is the player's own work and the fight has no
// other way to confirm it, so it gets a line of its own rather than the level's.
EncScript_Meloetta_Announce:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Meloetta_Announce_M5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Meloetta_Announce_M4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Meloetta_Announce_M3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Meloetta_Announce_M2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Meloetta_Announce_M1
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Meloetta_Announce_Done
	encsetvar 11, 0
	goto EncScript_Meloetta_Announce_Fall
EncScript_Meloetta_Announce_M1:
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Meloetta_Announce_Done
	encjumpifvar CMP_GREATER_THAN, 11, 1, EncScript_Meloetta_Announce_Fall1
	encsetvar 11, 1
	printstring STRINGID_ENCMELOETTAMETER1
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Meloetta_Announce_Fall1:
	encsetvar 11, 1
	goto EncScript_Meloetta_Announce_Fall
EncScript_Meloetta_Announce_M2:
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Meloetta_Announce_Done
	encjumpifvar CMP_GREATER_THAN, 11, 2, EncScript_Meloetta_Announce_Fall2
	encsetvar 11, 2
	printstring STRINGID_ENCMELOETTAMETER2
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Meloetta_Announce_Fall2:
	encsetvar 11, 2
	goto EncScript_Meloetta_Announce_Fall
EncScript_Meloetta_Announce_M3:
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Meloetta_Announce_Done
	encjumpifvar CMP_GREATER_THAN, 11, 3, EncScript_Meloetta_Announce_Fall3
	encsetvar 11, 3
	printstring STRINGID_ENCMELOETTAMETER3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Meloetta_Announce_Fall3:
	encsetvar 11, 3
	goto EncScript_Meloetta_Announce_Fall
EncScript_Meloetta_Announce_M4:
	encjumpifvar CMP_EQUAL, 11, 4, EncScript_Meloetta_Announce_Done
	encjumpifvar CMP_GREATER_THAN, 11, 4, EncScript_Meloetta_Announce_Fall4
	encsetvar 11, 4
	printstring STRINGID_ENCMELOETTAMETER4
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Meloetta_Announce_Fall4:
	encsetvar 11, 4
	goto EncScript_Meloetta_Announce_Fall
EncScript_Meloetta_Announce_M5:
	encjumpifvar CMP_EQUAL, 11, 5, EncScript_Meloetta_Announce_Done
	encsetvar 11, 5
	printstring STRINGID_ENCMELOETTAMETER5
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Meloetta_Announce_Fall:
	printstring STRINGID_ENCMELOETTAMETERFALL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Meloetta_Announce_Done:
	return

// THE BEAT CALLOUT (guideline 14). The fight's recurring cue: the state of the bar, printed the
// moment it changes and again at the top of every bar so a player who missed the first line still
// gets told. The guard numbers themselves are never stated - the line points at the cause.
EncScript_Meloetta_CallBeat:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Meloetta_CallBeat_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Meloetta_CallBeat_Quiet
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Meloetta_CallBeat_Gather
	encjumpifvar CMP_EQUAL, 9, 2, EncScript_Meloetta_CallBeat_Gather
	call EncScript_Meloetta_ClassifyBeat
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Meloetta_CallBeat_Chorus
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Meloetta_CallBeat_Recur
	encsetvar 10, 1
	printstring STRINGID_ENCMELOETTAVERSE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Meloetta_CallBeat_Chorus:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Meloetta_CallBeat_Recur
	encsetvar 10, 2
	printstring STRINGID_ENCMELOETTACHORUS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Meloetta_CallBeat_Gather:
	encsetvar 10, 3
	printstring STRINGID_ENCMELOETTAGATHER
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Meloetta_CallBeat_Quiet:
	encjumpifvar CMP_EQUAL, 10, 4, EncScript_Meloetta_CallBeat_Done
	encsetvar 10, 4
	printstring STRINGID_ENCMELOETTAQUIET
	waitmessage B_WAIT_TIME_LONG
	return
	@ Nothing about the bar changed. Say it again anyway once a bar, at beat 1 - one message is
	@ trivially missed, and the mechanic is live for the whole fight.
EncScript_Meloetta_CallBeat_Recur:
	encjumpifvar CMP_NOT_EQUAL, 5, 1, EncScript_Meloetta_CallBeat_Done
	printstring STRINGID_ENCMELOETTARECUR
	waitmessage B_WAIT_TIME_SHORT
EncScript_Meloetta_CallBeat_Done:
	return

// The meter rises one, to a ceiling of 5, mirrored into whichever park var the active form owns.
EncScript_Meloetta_Rise:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Meloetta_Rise_Done
	encaddvar 1, 1
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Meloetta_Rise_Tempo
	enccopyvar 2, 1
	goto EncScript_Meloetta_Rise_Say
EncScript_Meloetta_Rise_Tempo:
	enccopyvar 3, 1
EncScript_Meloetta_Rise_Say:
	call EncScript_Meloetta_Announce
EncScript_Meloetta_Rise_Done:
	return

// A RHYTHM BREAK: the meter falls two, to a floor of 0, AND the bar restarts at beat 1 - so
// breaking the song is always worth a full verse of soft beats, even at meter 0. Silent when there
// is nothing left to break, so a player sitting behind Protect at meter 0 is not told about it
// every turn.
EncScript_Meloetta_Break:
	encjumpifvar CMP_NOT_EQUAL, 1, 0, EncScript_Meloetta_Break_Do
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Meloetta_Break_Done
EncScript_Meloetta_Break_Do:
	printstring STRINGID_ENCMELOETTABREAK
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Meloetta_Break_Zero
	encsubvar 1, 2
	goto EncScript_Meloetta_Break_Mirror
EncScript_Meloetta_Break_Zero:
	encsetvar 1, 0
EncScript_Meloetta_Break_Mirror:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Meloetta_Break_Tempo
	enccopyvar 2, 1
	goto EncScript_Meloetta_Break_Bar
EncScript_Meloetta_Break_Tempo:
	enccopyvar 3, 1
EncScript_Meloetta_Break_Bar:
	encsetvar 5, 0     @ the bar restarts - 0 because _TurnClose's advance still runs this turn
	call EncScript_Meloetta_Announce
EncScript_Meloetta_Break_Done:
	return

// ARIA -> PIROUETTE. Not scripted stat inflation - it is the species: encformchange rebuilds stats,
// types and ability from SPECIES_MELOETTA_PIROUETTE, whose Attack and Speed are both 128 against
// Aria's 77 and 90. The moveset is swapped explicitly because encformchange keeps the moves the
// battler was holding. Neither form route touches Beat; the caller decides whether the bar
// restarts, because the Phase 1 cycle IS the rhythm and must not reset it.
EncScript_Meloetta_ToPirouette:
	printstring STRINGID_ENCMELOETTATOPIROUETTE
	waitmessage B_WAIT_TIME_LONG
	enccopyvar 2, 1    @ park Rhythm
	enccopyvar 1, 3    @ load Tempo
	encsetvar 4, 1     @ Form
	encformchange ENC_TARGET_BOSS, SPECIES_MELOETTA_PIROUETTE, EncScript_Meloetta_ToPirouette_Moves, B_ANIM_FORM_CHANGE
EncScript_Meloetta_ToPirouette_Moves:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_CLOSE_COMBAT
	encsetmove ENC_TARGET_BOSS, 1, MOVE_ACROBATICS
	encsetmove ENC_TARGET_BOSS, 2, MOVE_QUICK_ATTACK
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BRICK_BREAK
	goto EncScript_Meloetta_Shift_Shed

// PIROUETTE -> ARIA. The dance collapses and the singer comes back.
EncScript_Meloetta_ToAria:
	printstring STRINGID_ENCMELOETTATOARIA
	waitmessage B_WAIT_TIME_LONG
	enccopyvar 3, 1    @ park Tempo
	enccopyvar 1, 2    @ load Rhythm
	encsetvar 4, 0     @ Form
	encsetcrit ENC_TARGET_BOSS, FALSE
	encformchange ENC_TARGET_BOSS, SPECIES_MELOETTA, EncScript_Meloetta_ToAria_Moves, B_ANIM_FORM_CHANGE
EncScript_Meloetta_ToAria_Moves:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_PSYCHIC
	encsetmove ENC_TARGET_BOSS, 1, MOVE_HYPER_VOICE
	encsetmove ENC_TARGET_BOSS, 2, MOVE_ECHOED_VOICE
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SHADOW_BALL
	@ From Phase 1 a shift sheds whatever the player put on Meloetta, which is what stops a status
	@ lock from holding the rhythm broken for the rest of the fight (guideline 8 - displacement, not
	@ immunity: it still works, it just has to be re-applied around the cycle).
EncScript_Meloetta_Shift_Shed:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Meloetta_Shift_Done
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
EncScript_Meloetta_Shift_Done:
	call EncScript_Meloetta_Announce
	call EncScript_Meloetta_ApplyStance
	return

// --- Trigger scripts ---

EncScript_Meloetta_Intro::
	printstring STRINGID_ENCMELOETTAINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCMELOETTAINTRO2
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCMELOETTAINTRO3
	waitmessage B_WAIT_TIME_LONG
	encsetvar 5, 1     @ Beat - the bar opens on its first beat
	call EncScript_Meloetta_CallBeat
	call EncScript_Meloetta_ApplyStance
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that prints before any
// animation renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch can leave it re-armed. Takes the mark the turn is measured against as a
// percentage (the only portable choice under Level: LevelCap and New Game Plus offsets), ticks the
// crescendo (the decrement half of the split countdown - _Resolve at OnTurnEnd is the other half),
// re-asserts the critical-hit ratio ABSOLUTELY, then lands the new beat's guard and calls it.
EncScript_Meloetta_TurnOpen::
	flushtextbox
	encsetvar 6, 1     @ TurnGuard
	encsetvar 15, 0    @ StatusBreak - the requests of the previous turn are spent
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 7, ENC_SNAP_SET
	encjumpifvar CMP_NOT_EQUAL, 9, 2, EncScript_Meloetta_TurnOpen_Crit
	encsetvar 9, 1
	@ Pirouette's raised critical-hit ratio, from meter 3. Written absolutely every turn in both
	@ directions, which is exactly why encsetcrit was built reversible - a meter that falls two
	@ levels in one turn needs no teardown path.
EncScript_Meloetta_TurnOpen_Crit:
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Meloetta_TurnOpen_NoCrit
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Meloetta_TurnOpen_NoCrit
	encsetcrit ENC_TARGET_BOSS, TRUE
	goto EncScript_Meloetta_TurnOpen_Stance
EncScript_Meloetta_TurnOpen_NoCrit:
	encsetcrit ENC_TARGET_BOSS, FALSE
EncScript_Meloetta_TurnOpen_Stance:
	call EncScript_Meloetta_ApplyStance
	call EncScript_Meloetta_CallBeat
	encsetvar 13, 0    @ EventGuard
	return

// THE CRESCENDO telegraph, announced before the player picks a move so the turn can be spent on the
// answer. Self-disabling: the script sets Charge. Opens with its own dialogue rather than
// flushtextbox for the same BG0 reason - it animates.
EncScript_Meloetta_BeginCrescendo::
	printstring STRINGID_ENCMELOETTACRESC1
	waitmessage B_WAIT_TIME_LONG
	encsetvar 9, 2     @ Charge
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Meloetta_BeginCrescendo_Say
EncScript_Meloetta_BeginCrescendo_Say:
	call EncScript_Meloetta_ApplyStance
	printstring STRINGID_ENCMELOETTACRESC2
	waitmessage B_WAIT_TIME_LONG
	return

// The hall falls silent. A faint feeds the song exactly as a landed action does.
EncScript_Meloetta_Silence::
	encsetvar 13, 1    @ EventGuard
	printstring STRINGID_ENCMELOETTASILENCE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Meloetta_Rise
	return

// A mid-turn switch (U-turn, Eject Button). Sets EventGuard and nothing else - it exists only so a
// faint later in the same turn is not counted twice.
EncScript_Meloetta_TheyEntered::
	encsetvar 13, 1    @ EventGuard
	return

// A break route a script cannot test for itself - Meloetta carrying a status, or the player having
// protected. Both triggers write the same flag, so whichever fires first disables the other and
// _TurnClose and _Resolve each read one number.
EncScript_Meloetta_BreakRequest::
	encsetvar 15, 1    @ StatusBreak
	return

// THE LEDGER, and the only place the turn's subsystems resolve. Scratch survives the whole script
// after the measurement, and _Resolve reads it again one trigger later, so "did Meloetta's action
// connect" is measured once and answered the same way everywhere.
EncScript_Meloetta_TurnClose::
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Meloetta_TurnClose_Quiet
	@ The stumbling turn is over. It was set at the close of the PREVIOUS turn, so the whole of this
	@ turn was spent attacking into the 72 guard.
	encjumpifvar CMP_NOT_EQUAL, 9, 9, EncScript_Meloetta_TurnClose_Measure
	encsetvar 9, 0
EncScript_Meloetta_TurnClose_Measure:
	enccopyvar 8, 7    @ Scratch = PlayerMark, the copy ENC_SNAP_DAMAGE consumes
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 8, ENC_SNAP_DAMAGE, EncScript_Meloetta_TurnClose_NoDamage
	goto EncScript_Meloetta_TurnClose_Ledger
EncScript_Meloetta_TurnClose_NoDamage:
	encsetvar 8, 0
	@ Three break routes, one test each: a status on Meloetta or a Protect (both arrived as
	@ StatusBreak), or an action that took less than 6% of the player's max HP - a miss, an
	@ immunity, a switch into a wall, a status move, a sleeping or flinched Meloetta.
EncScript_Meloetta_TurnClose_Ledger:
	encjumpifvar CMP_EQUAL, 15, 1, EncScript_Meloetta_TurnClose_Broke
	encjumpifvar CMP_LESS_THAN, 8, 6, EncScript_Meloetta_TurnClose_Broke
	call EncScript_Meloetta_Rise
	goto EncScript_Meloetta_TurnClose_Chip
EncScript_Meloetta_TurnClose_Broke:
	call EncScript_Meloetta_Break
	@ THE REFRAIN. From meter 3 every chorus beat chips the player's side, off max HP, so neither
	@ Protect nor a switch nor a wall turns it off.
EncScript_Meloetta_TurnClose_Chip:
	call EncScript_Meloetta_ClassifyBeat
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Meloetta_TurnClose_Follow
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Meloetta_TurnClose_Follow
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Meloetta_TurnClose_ChipP1
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Meloetta_TurnClose_Chip9
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Meloetta_TurnClose_Chip7
	encchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_TurnClose_ChipSay
EncScript_Meloetta_TurnClose_Chip7:
	encchangehp ENC_TARGET_ALL_FOES, -7, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_TurnClose_ChipSay
EncScript_Meloetta_TurnClose_Chip9:
	encchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_TurnClose_ChipSay
EncScript_Meloetta_TurnClose_ChipP1:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Meloetta_TurnClose_Chip11
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Meloetta_TurnClose_Chip9
	encchangehp ENC_TARGET_ALL_FOES, -7, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_TurnClose_ChipSay
EncScript_Meloetta_TurnClose_Chip11:
	encchangehp ENC_TARGET_ALL_FOES, -11, ENC_AMOUNT_PERCENT
EncScript_Meloetta_TurnClose_ChipSay:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCMELOETTACHIP
	waitmessage B_WAIT_TIME_SHORT
	@ THE FOLLOW-UP. Pirouette only, from meter 4, and only on a turn the dance actually connected -
	@ the one effect in the fight that rewards Meloetta for the same thing the meter rewards it for.
EncScript_Meloetta_TurnClose_Follow:
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Meloetta_TurnClose_Forms
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Meloetta_TurnClose_Forms
	encjumpifvar CMP_LESS_THAN, 8, 6, EncScript_Meloetta_TurnClose_Forms
	printstring STRINGID_ENCMELOETTAFOLLOWUP
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_QUICK_ATTACK
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Meloetta_TurnClose_Follow8
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_TurnClose_Forms
EncScript_Meloetta_TurnClose_Follow8:
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	@ The meter-driven form routes. These two DO restart the bar: a new form is a new song.
EncScript_Meloetta_TurnClose_Forms:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Meloetta_TurnClose_FormsPir
	encjumpifvar CMP_NOT_EQUAL, 1, 5, EncScript_Meloetta_TurnClose_Advance
	call EncScript_Meloetta_ToPirouette
	goto EncScript_Meloetta_TurnClose_NewBar
EncScript_Meloetta_TurnClose_FormsPir:
	encjumpifvar CMP_NOT_EQUAL, 1, 0, EncScript_Meloetta_TurnClose_Advance
	call EncScript_Meloetta_ToAria
	@ A new form is a new song: the bar restarts at its first beat, and the turn's advance is spent
	@ doing it - which is also what keeps a meter-driven shift from tripping the Phase 1 cycle below
	@ in the same turn.
EncScript_Meloetta_TurnClose_NewBar:
	encsetvar 5, 1     @ Beat
	goto EncScript_Meloetta_TurnClose_Finish
	@ The bar advances, 1 -> 2 -> 3 -> 4 -> 1.
EncScript_Meloetta_TurnClose_Advance:
	encjumpifvar CMP_EQUAL, 5, 4, EncScript_Meloetta_TurnClose_Wrap
	encaddvar 5, 1
	goto EncScript_Meloetta_TurnClose_Cycle
EncScript_Meloetta_TurnClose_Wrap:
	encsetvar 5, 1
	@ GRAND PERFORMANCE. From Phase 1 the forms turn over on their own on the bar's even beats -
	@ every second turn - so a status lock has to be re-applied around the cycle and the player
	@ never settles into one matchup. Deliberately does NOT restart the bar: the cycle is part of
	@ the rhythm now, and keying it to the even beats is also what stops a rhythm break, which
	@ always lands the next turn on beat 1, from shedding Meloetta's status the moment it lands.
EncScript_Meloetta_TurnClose_Cycle:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Meloetta_TurnClose_Finish
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Meloetta_TurnClose_Flip
	encjumpifvar CMP_NOT_EQUAL, 5, 4, EncScript_Meloetta_TurnClose_Finish
EncScript_Meloetta_TurnClose_Flip:
	printstring STRINGID_ENCMELOETTACYCLE
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Meloetta_TurnClose_FlipAria
	call EncScript_Meloetta_ToPirouette
	goto EncScript_Meloetta_TurnClose_Finish
EncScript_Meloetta_TurnClose_FlipAria:
	call EncScript_Meloetta_ToAria
EncScript_Meloetta_TurnClose_Finish:
	call EncScript_Meloetta_ApplyStance
	encsetvar 13, 0    @ EventGuard
EncScript_Meloetta_TurnClose_Quiet:
	encsetvar 6, 0     @ TurnGuard
	return

// THE CRESCENDO resolves, one whole turn after it was telegraphed. Reads the same two numbers the
// ledger read - a break request, or an action that took less than 6% of the player's max HP - so
// the answer to the crescendo is the same answer the rest of the fight asks for, played on the one
// turn it matters most.
EncScript_Meloetta_Resolve::
	encjumpifvar CMP_EQUAL, 15, 1, EncScript_Meloetta_Resolve_Interrupt
	encjumpifvar CMP_LESS_THAN, 8, 6, EncScript_Meloetta_Resolve_Interrupt
	printstring STRINGID_ENCMELOETTAFINALE1
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_RELIC_SONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Meloetta_Resolve_P1
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_Meloetta_Resolve_D28
	encjumpifvar CMP_EQUAL, 14, 1, EncScript_Meloetta_Resolve_D32
	encjumpifvar CMP_EQUAL, 14, 2, EncScript_Meloetta_Resolve_D36
	encchangehp ENC_TARGET_ALL_FOES, -40, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_Resolve_After
EncScript_Meloetta_Resolve_D28:
	encchangehp ENC_TARGET_ALL_FOES, -28, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_Resolve_After
EncScript_Meloetta_Resolve_D32:
	encchangehp ENC_TARGET_ALL_FOES, -32, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_Resolve_After
EncScript_Meloetta_Resolve_D36:
	encchangehp ENC_TARGET_ALL_FOES, -36, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_Resolve_After
EncScript_Meloetta_Resolve_P1:
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_Meloetta_Resolve_D34
	encjumpifvar CMP_EQUAL, 14, 1, EncScript_Meloetta_Resolve_D38
	encjumpifvar CMP_EQUAL, 14, 2, EncScript_Meloetta_Resolve_D42
	encchangehp ENC_TARGET_ALL_FOES, -46, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_Resolve_After
EncScript_Meloetta_Resolve_D34:
	encchangehp ENC_TARGET_ALL_FOES, -34, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_Resolve_After
EncScript_Meloetta_Resolve_D38:
	encchangehp ENC_TARGET_ALL_FOES, -38, ENC_AMOUNT_PERCENT
	goto EncScript_Meloetta_Resolve_After
EncScript_Meloetta_Resolve_D42:
	encchangehp ENC_TARGET_ALL_FOES, -42, ENC_AMOUNT_PERCENT
	@ The song restarts rather than stopping: both meters to 2, the bar to its first beat. The fight
	@ recovers from its own climax instead of locking the player into a full-chorus bar forever.
EncScript_Meloetta_Resolve_After:
	encaddvar 14, 1    @ Encore
	encsetvar 9, 0     @ Charge
	encsetvar 1, 2     @ Meter
	encsetvar 2, 2     @ Rhythm
	encsetvar 3, 2     @ Tempo
	encsetvar 5, 1     @ Beat
	call EncScript_Meloetta_Announce
	printstring STRINGID_ENCMELOETTAFINALE2
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Meloetta_ApplyStance
	return
	@ THE PERFORMANCE IS INTERRUPTED. Both meters empty, Meloetta loses its next action, and its
	@ guard falls to 72 for the whole of that turn - the biggest window in the fight, and the payoff
	@ for having answered a telegraph a turn in advance.
EncScript_Meloetta_Resolve_Interrupt:
	printstring STRINGID_ENCMELOETTAINT1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 9, 9     @ Charge: stumbling
	encsetvar 1, 0     @ Meter
	encsetvar 2, 0     @ Rhythm
	encsetvar 3, 0     @ Tempo
	encsetvar 5, 1     @ Beat
	encsetrecharge ENC_TARGET_BOSS, 2      @ at OnTurnEnd = next turn's action
	call EncScript_Meloetta_Announce
	printstring STRINGID_ENCMELOETTAINT2
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Meloetta_ApplyStance
	return

// --- Phases ---

// 50% - GRAND PERFORMANCE. The guard band moves to 86/95, the chorus chip and the follow-up each
// gain +2, Relic Song moves to 34%, a form shift now sheds Meloetta's status, and from here the
// forms cycle on their own every second beat. The stat growth is raw, so Haze and the form changes
// both leave it standing, and both halves of the cycle get it.
EncScript_Meloetta_GrandPerformance::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCMELOETTAGRAND1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCMELOETTAGRAND2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 12, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 12, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCMELOETTAGRAND3
	waitmessage B_WAIT_TIME_LONG
	encsetvar 10, 0    @ LastCall - both callouts re-fire on the new phase's values
	encsetvar 11, 0    @ LastMeter
	call EncScript_Meloetta_Announce
	call EncScript_Meloetta_CallBeat
	call EncScript_Meloetta_ApplyStance
	return

// 15% - THE FINAL NOTE. The fight pauses, Meloetta restores 8% of its max HP, and then the song
// STOPS: both meters die, the bar stops counting, the form locks to Aria and the guard falls to a
// flat 82 for the calm final stretch. The heal is what makes that stretch happen at all - it puts
// Meloetta back to roughly 23%, so there is real fight left between the pause and the catch window
// rather than a threshold the player crosses twice in one turn.
EncScript_Meloetta_FinalNote::
	printstring STRINGID_ENCMELOETTAFINAL1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCMELOETTAFINAL2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	encsetvar 0, 2     @ Phase
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Meloetta_FinalNote_Stop
	call EncScript_Meloetta_ToAria
EncScript_Meloetta_FinalNote_Stop:
	encsetvar 1, 0     @ Meter
	encsetvar 2, 0     @ Rhythm
	encsetvar 3, 0     @ Tempo
	encsetvar 9, 0     @ Charge
	encsetvar 11, 0    @ LastMeter
	encsetvar 12, 0    @ BeatClass
	encsetcrit ENC_TARGET_BOSS, FALSE
	encsetvar 10, 4    @ LastCall: quiet, and the line below is that callout
	printstring STRINGID_ENCMELOETTAFINAL3
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Meloetta_ApplyStance
	return

// 10% - SPENT, and the catch window (guideline 10). The first and only point the player may throw a
// ball: the final phase, at 10% HP or less. _ApplyStance returns immediately at Phase 3, so nothing
// can re-raise the guard behind the window, and encsetdamagereduction 99 writes the value the
// automatic catch guard will RESTORE, not the live one.
EncScript_Meloetta_Spent::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCMELOETTASPENT1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetcrit ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMELOETTAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
