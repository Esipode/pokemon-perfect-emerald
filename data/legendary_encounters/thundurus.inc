// Thundurus, "The Bolt Strike Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Thundurus_Intro:
// 0 Phase (0 Static Air / 1 Thunderstorm / 2 Wrath of the Storm / 3 Grounded), 1 Charge (0-6),
// 2 LastBand (the last ANNOUNCED Charge), 3 TurnGuard, 4 Earthed (a Rock/Ice move counted this
// turn), 5 Strike (0 no rod / 1-3 turns left on the countdown / 4 resolving this end of turn),
// 6 BossMark (Thundurus HP% at turn open), 7 Scratch (the copy ENC_SNAP_DAMAGE consumes), 8 Gained,
// 9 Fed, 10 Overcharged, 11 Discharge (turns left in the post-Overload window), 12 RodCd,
// 13 EventGuard, 14 Strikes, 15 Silent.
//
// ONE VAR CARRIES THE ROD AND ITS COUNTDOWN. A rod stands exactly while Strike is non-zero, so
// driving a rod and starting the countdown are one beat and every answer that destroys a rod also
// cancels the countdown. That frees the sixteenth variable for Earthed, which the Rock/Ice trigger
// needs: an event condition stays true for a whole dispatch, so a trigger reacting to a move has to
// gate itself on a variable it sets.
//
// CHARGE IS THE GUARD, IN BOTH DIRECTIONS. EncScript_Thundurus_ApplyStorm is the damage reduction's
// only writer and reads Phase, Charge and Discharge absolutely, so an Overload window closing undoes
// one opening with no separate teardown path. The reeling value is flat at 70 in every phase: the
// Overload window is the fight's single reward and it must not get worse as the fight goes on.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase 3 returns immediately: once balls are allowed the
// engine's own catch guard owns the number.
EncScript_Thundurus_ApplyStorm:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Thundurus_ApplyStorm_Done
	encjumpifvar CMP_GREATER_THAN, 11, 0, EncScript_Thundurus_ApplyStorm_Reel
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Thundurus_ApplyStorm_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Thundurus_ApplyStorm_P1
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Thundurus_ApplyStorm_P0Hot
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return
EncScript_Thundurus_ApplyStorm_P0Hot:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Thundurus_ApplyStorm_P1:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Thundurus_ApplyStorm_P1Hot
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Thundurus_ApplyStorm_P1Hot:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Thundurus_ApplyStorm_P2:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Thundurus_ApplyStorm_P2Hot
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Thundurus_ApplyStorm_P2Hot:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Thundurus_ApplyStorm_Reel:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Thundurus_ApplyStorm_Done:
	return

// THE CHARGE CALLOUT (guideline 14). LastBand holds the last ANNOUNCED Charge; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on a
// difference - in BOTH directions. The low lines are as load-bearing as the high ones: a fall is the
// player's own work (a Protect, a broken charge, a Rock or Ice hit) and the fight has no other way to
// confirm those did anything.
EncScript_Thundurus_AnnounceCharge:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Thundurus_AnnounceCharge_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Thundurus_AnnounceCharge_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Thundurus_AnnounceCharge_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Thundurus_AnnounceCharge_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Thundurus_AnnounceCharge_4
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Thundurus_AnnounceCharge_5
	encjumpifvar CMP_EQUAL, 2, 6, EncScript_Thundurus_AnnounceCharge_Done
	printstring STRINGID_ENCTHUNDURUSCHARGE6
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Thundurus_AnnounceCharge_Latch
EncScript_Thundurus_AnnounceCharge_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Thundurus_AnnounceCharge_Done
	printstring STRINGID_ENCTHUNDURUSCHARGE0
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Thundurus_AnnounceCharge_Latch
EncScript_Thundurus_AnnounceCharge_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Thundurus_AnnounceCharge_Done
	printstring STRINGID_ENCTHUNDURUSCHARGE1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Thundurus_AnnounceCharge_Latch
EncScript_Thundurus_AnnounceCharge_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Thundurus_AnnounceCharge_Done
	printstring STRINGID_ENCTHUNDURUSCHARGE2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Thundurus_AnnounceCharge_Latch
EncScript_Thundurus_AnnounceCharge_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Thundurus_AnnounceCharge_Done
	printstring STRINGID_ENCTHUNDURUSCHARGE3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Thundurus_AnnounceCharge_Latch
EncScript_Thundurus_AnnounceCharge_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Thundurus_AnnounceCharge_Done
	printstring STRINGID_ENCTHUNDURUSCHARGE4
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Thundurus_AnnounceCharge_Latch
EncScript_Thundurus_AnnounceCharge_5:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Thundurus_AnnounceCharge_Done
	printstring STRINGID_ENCTHUNDURUSCHARGE5
	waitmessage B_WAIT_TIME_LONG
EncScript_Thundurus_AnnounceCharge_Latch:
	enccopyvar 2, 1     @ LastBand = Charge
EncScript_Thundurus_AnnounceCharge_Done:
	return

// Charge climbs, clamped at 6. Two entry points so a caller never has to do the clamp itself.
EncScript_Thundurus_Charge2:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Thundurus_Charge2_Cap
	encaddvar 1, 2
	return
EncScript_Thundurus_Charge2_Cap:
	encsetvar 1, 6
	return

EncScript_Thundurus_Charge1:
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Thundurus_Charge1_Done
	encaddvar 1, 1
EncScript_Thundurus_Charge1_Done:
	return

// Charge falls, floored at 0.
EncScript_Thundurus_Drain2:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Thundurus_Drain2_Floor
	encsubvar 1, 2
	return
EncScript_Thundurus_Drain2_Floor:
	encsetvar 1, 0
	return

EncScript_Thundurus_Drain1:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Thundurus_Drain1_Done
	encsubvar 1, 1
EncScript_Thundurus_Drain1_Done:
	return

// OVERCHARGED, latched on Overcharged so the perks are granted once and taken back once. The guard's
// Charge >= 5 step is deliberately NOT latched here - _ApplyStorm writes it absolutely from the live
// Charge, so it moves on its own. -13% is the inverse of a +15% taken on the raised value.
EncScript_Thundurus_ApplyOvercharge:
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Thundurus_ApplyOvercharge_Done
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Thundurus_ApplyOvercharge_Done
	encsetvar 10, 1
	encsetcrit ENC_TARGET_BOSS, TRUE
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 10, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Thundurus_ApplyOvercharge_Done:
	return

EncScript_Thundurus_RevokeOvercharge:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Thundurus_RevokeOvercharge_Done
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Thundurus_RevokeOvercharge_Done
	encsetvar 10, 0
	encsetcrit ENC_TARGET_BOSS, FALSE
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, -10, ENC_AMOUNT_PERCENT
EncScript_Thundurus_RevokeOvercharge_Done:
	return

// The rod comes down and the countdown dies with it. Silent - every caller prints its own line - and
// a no-op when no rod stands, so it may be called unconditionally. The cooldown is only ever raised,
// never lowered, so a rod destroyed at OnTurnEnd cannot shorten the gap a later one is owed.
EncScript_Thundurus_BreakRod:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Thundurus_BreakRod_Done
	encsetvar 5, 0     @ Strike: rod gone, countdown cancelled
	encjumpifvar CMP_NOT_EQUAL, 12, 0, EncScript_Thundurus_BreakRod_Done
	encsetvar 12, 1    @ RodCd
EncScript_Thundurus_BreakRod_Done:
	return

// The tail every script that moved Charge shares: the perks, the guard and the callout, in that
// order, so the line the player reads always matches the number the guard was just written from.
EncScript_Thundurus_Settle:
	call EncScript_Thundurus_ApplyOvercharge
	call EncScript_Thundurus_RevokeOvercharge
	call EncScript_Thundurus_ApplyStorm
	call EncScript_Thundurus_AnnounceCharge
	return

// --- Trigger scripts ---

// LastBand is parked on a value Charge can never hold so the callout's latch fires on turn 0: the
// player is told the air is Still before anything happens, and every later change reads as a change.
EncScript_Thundurus_Intro::
	printstring STRINGID_ENCTHUNDURUSINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTHUNDURUSINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Thundurus_ApplyStorm
	encsetvar 2, 9     @ LastBand: nothing announced yet
	call EncScript_Thundurus_AnnounceCharge
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch can leave it re-armed. Ticks the two countdowns' decrement halves, climbs
// the meter on its own from Phase 1, takes the mark the turn's damage is measured against, and runs
// the countdown reprint and the recurring cue.
EncScript_Thundurus_TurnOpen::
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Thundurus_TurnOpen_Clear   @ Phase 3: quiet
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Thundurus_TurnOpen_Window
	encsubvar 12, 1    @ RodCd
EncScript_Thundurus_TurnOpen_Window:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Thundurus_TurnOpen_Climb
	encsubvar 11, 1    @ Discharge: the Overload window closes on its own
EncScript_Thundurus_TurnOpen_Climb:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Thundurus_TurnOpen_Mark
	call EncScript_Thundurus_Charge1   @ the storm answers on its own from Thunderstorm on
EncScript_Thundurus_TurnOpen_Mark:
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET
	call EncScript_Thundurus_Settle
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Thundurus_TurnOpen_Two
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Thundurus_TurnOpen_One
	goto EncScript_Thundurus_TurnOpen_Cue
EncScript_Thundurus_TurnOpen_Two:
	printstring STRINGID_ENCTHUNDURUSCOUNT2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Thundurus_TurnOpen_Cue
EncScript_Thundurus_TurnOpen_One:
	printstring STRINGID_ENCTHUNDURUSCOUNT1
	waitmessage B_WAIT_TIME_LONG
EncScript_Thundurus_TurnOpen_Cue:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Thundurus_TurnOpen_Clear
	printstring STRINGID_ENCTHUNDURUSSTATICCUE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Thundurus_TurnOpen_Clear:
	encsetvar 4, 0     @ Earthed
	encsetvar 8, 0     @ Gained
	encsetvar 9, 0     @ Fed
	encsetvar 13, 0    @ EventGuard
	return

// A LIGHTNING ROD, driven into the player's active Pokemon, and the three-turn countdown in the same
// beat. Self-disabling: its own condition reads Strike == 0.
EncScript_Thundurus_DriveRod::
	printstring STRINGID_ENCTHUNDURUSROD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	encsetvar 5, 3     @ Strike: LIGHTNING IN 3 TURNS
	printstring STRINGID_ENCTHUNDURUSCOUNT3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

// The storm shakes off sleep and freeze from Phase 1. curestatus takes an explicit battler -
// BS_TARGET/BS_ATTACKER are stale at a turn checkpoint. Status1 is exclusive, so the trigger's own
// sleep/freeze gate is what keeps Toxic and paralysis out of this.
EncScript_Thundurus_ShakesOff::
	printstring STRINGID_ENCTHUNDURUSSHAKESOFF
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// THE AIR CONDUCTS. Any Electric-type move resolving on the field feeds the charge - the player's
// included, which is the fight's central trap, and from Phase 2 Volt Absorb heals off it on top.
// Self-disables through Fed.
EncScript_Thundurus_Conduct::
	encsetvar 9, 1     @ Fed
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Thundurus_Conduct_Fin
	call EncScript_Thundurus_Charge1
	printstring STRINGID_ENCTHUNDURUSCONDUCT
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Thundurus_Conduct_Fin:
	call EncScript_Thundurus_Settle
	return

// THE CHARGE EARTHS. Rock and Ice are the two types the storm recoils from, one per turn.
EncScript_Thundurus_Earth::
	encsetvar 4, 1     @ Earthed
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Thundurus_Earth_Fin
	call EncScript_Thundurus_Drain1
	printstring STRINGID_ENCTHUNDURUSEARTH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
EncScript_Thundurus_Earth_Fin:
	call EncScript_Thundurus_Settle
	return

// One of the player's Pokemon fell and the storm feeds on it. EventGuard stops the replacement
// switch-in being processed a second time.
EncScript_Thundurus_TheyFell::
	encsetvar 13, 1    @ EventGuard
	call EncScript_Thundurus_Charge2
	printstring STRINGID_ENCTHUNDURUSTHEYFELL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Thundurus_Settle
	return

// THE ROD LOSES ITS GROUND - the switching answer from the three the countdown offers. The rod and
// its countdown both go; Charge is untouched, so switching is the tempo answer rather than the
// suppression one.
EncScript_Thundurus_TheyEntered::
	encsetvar 13, 1    @ EventGuard
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Thundurus_TheyEntered_Done
	call EncScript_Thundurus_BreakRod
	printstring STRINGID_ENCTHUNDURUSRODGROUNDED
	waitmessage B_WAIT_TIME_LONG
EncScript_Thundurus_TheyEntered_Done:
	return

// THE LEDGER, and the only place per-turn state is cleared. Measures how much of Thundurus's max HP
// the player took off this turn and leaves it in Scratch: 5% or more feeds the charge once, 20% or
// more collapses a gathering discharge outright.
// The independent storm strike is folded into the middle of this script rather than owning a trigger
// (see the .encounter note). It is read BEFORE the countdown block so a turn that just broke a
// strike is never chipped by the storm on top of it.
EncScript_Thundurus_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Thundurus_TurnClose_Quiet   @ Phase 3: quiet
	encsetvar 7, 0     @ Scratch
	enccopyvar 7, 6    @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_DAMAGE, EncScript_Thundurus_TurnClose_NoDamage
	goto EncScript_Thundurus_TurnClose_Feed
EncScript_Thundurus_TurnClose_NoDamage:
	encsetvar 7, 0     @ nothing taken off Thundurus this turn
EncScript_Thundurus_TurnClose_Feed:
	encjumpifvar CMP_LESS_THAN, 7, 5, EncScript_Thundurus_TurnClose_Idle
	encjumpifvar CMP_NOT_EQUAL, 8, 0, EncScript_Thundurus_TurnClose_Idle
	encsetvar 8, 1     @ Gained: the damage feed is capped at one a turn
	call EncScript_Thundurus_Charge1
EncScript_Thundurus_TurnClose_Idle:
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Thundurus_TurnClose_Count
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Thundurus_TurnClose_Count
	encjumpifvar CMP_NOT_EQUAL, 11, 0, EncScript_Thundurus_TurnClose_Count
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Thundurus_TurnClose_Count
	printstring STRINGID_ENCTHUNDURUSIDLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
EncScript_Thundurus_TurnClose_Count:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Thundurus_TurnClose_Settle
	encjumpifvar CMP_LESS_THAN, 7, 20, EncScript_Thundurus_TurnClose_Tick
	printstring STRINGID_ENCTHUNDURUSBREAK1
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Thundurus_BreakRod
	call EncScript_Thundurus_Drain2
	printstring STRINGID_ENCTHUNDURUSBREAK2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetrecharge ENC_TARGET_BOSS, 2      @ at OnTurnEnd = next turn's action
	goto EncScript_Thundurus_TurnClose_Settle
EncScript_Thundurus_TurnClose_Tick:
	call EncScript_Thundurus_Charge1   @ the rod stood through the whole turn
	encsubvar 5, 1
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Thundurus_TurnClose_Settle
	encsetvar 5, 4     @ resolving: _StrikeEarthed / _StrikeResolve take it from here
EncScript_Thundurus_TurnClose_Settle:
	call EncScript_Thundurus_Settle
EncScript_Thundurus_TurnClose_Quiet:
	encsetvar 4, 0     @ Earthed
	encsetvar 8, 0     @ Gained
	encsetvar 9, 0     @ Fed
	encsetvar 13, 0    @ EventGuard
	encsetvar 3, 0     @ TurnGuard
	return

// THE BOLT EARTHS ITSELF - the Protect answer. Protected is a whole-turn latch, which is exactly what
// a delayed effect resolving at OnTurnEnd has to test against, so a player who protected for an
// unrelated reason this turn gets the block too. That errs in the player's favour.
EncScript_Thundurus_StrikeEarthed::
	printstring STRINGID_ENCTHUNDURUSRODEARTHED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_PROTECTED_ITSELF
	call EncScript_Thundurus_BreakRod
	call EncScript_Thundurus_Drain2
	call EncScript_Thundurus_Settle
	return

// THE STRIKE LANDS. The damage band is read off Charge, and from Charge 3 up the shock also takes a
// Speed stage - what "the charge raises paralysis chance" actually feels like at the controller.
// Charge +2 on top, which is how a player reaches OVERLOAD at all.
EncScript_Thundurus_StrikeResolve::
	encjumpifvar CMP_GREATER_THAN, 14, 2, EncScript_Thundurus_StrikeResolve_Hard
	printstring STRINGID_ENCTHUNDURUSSTRIKE
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Thundurus_StrikeResolve_Hit
EncScript_Thundurus_StrikeResolve_Hard:
	printstring STRINGID_ENCTHUNDURUSSTRIKEHARD
	waitmessage B_WAIT_TIME_LONG
EncScript_Thundurus_StrikeResolve_Hit:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Thundurus_StrikeResolve_D35
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Thundurus_StrikeResolve_D30
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Thundurus_StrikeResolve_D22
	encchangehp ENC_TARGET_ALL_FOES, -15, ENC_AMOUNT_PERCENT
	goto EncScript_Thundurus_StrikeResolve_Tally
EncScript_Thundurus_StrikeResolve_D22:
	encchangehp ENC_TARGET_ALL_FOES, -22, ENC_AMOUNT_PERCENT
	goto EncScript_Thundurus_StrikeResolve_Slow
EncScript_Thundurus_StrikeResolve_D30:
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Thundurus_StrikeResolve_Slow
EncScript_Thundurus_StrikeResolve_D35:
	encchangehp ENC_TARGET_ALL_FOES, -35, ENC_AMOUNT_PERCENT
EncScript_Thundurus_StrikeResolve_Slow:
	printstring STRINGID_ENCTHUNDURUSSTRIKESLOW
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
EncScript_Thundurus_StrikeResolve_Tally:
	encaddvar 14, 1    @ Strikes
	call EncScript_Thundurus_BreakRod
	call EncScript_Thundurus_Charge2
	call EncScript_Thundurus_Settle
	return

// OVERLOAD - the payoff, and the only real damage window the fight opens. Thundurus dumps everything
// on the field, tears itself a little on the way out, and its guard falls to 70 for two whole turns
// (Discharge is decremented at the NEXT turn open, so 3 buys two turns of window) while it recharges.
// In Wrath of the Storm the first Overload is instead THE STORM GOES SILENT: the rain and the
// tailwind come off and the window runs three turns.
EncScript_Thundurus_Overload::
	printstring STRINGID_ENCTHUNDURUSOVERLOAD1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTHUNDURUSOVERLOAD2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	encsetvar 1, 0     @ Charge
	call EncScript_Thundurus_BreakRod
	encsetvar 12, 1    @ RodCd
	encsetvar 11, 3    @ Discharge: two turns of reeling
	encsetrecharge ENC_TARGET_BOSS, 2
	printstring STRINGID_ENCTHUNDURUSOVERLOAD3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Thundurus_Overload_Fin
	encjumpifvar CMP_NOT_EQUAL, 15, 0, EncScript_Thundurus_Overload_Fin
	encsetvar 15, 1    @ Silent
	encsetvar 11, 4    @ three turns for the beat the spec asks for
	encsetrecharge ENC_TARGET_BOSS, 3
	removeweather
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	printstring STRINGID_ENCTHUNDURUSSILENT1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTHUNDURUSSILENT2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
EncScript_Thundurus_Overload_Fin:
	call EncScript_Thundurus_Settle
	return

// THE STORM RESUMES. The silence expired and Thundurus is still standing, so the rain and the
// tailwind come back and Charge is put straight back to 5. Silent stays 1: the window is repeatable,
// the beat is not.
EncScript_Thundurus_StormResumes::
	printstring STRINGID_ENCTHUNDURUSRESUME
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_RAIN, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	encsetvar 1, 5     @ Charge
	call EncScript_Thundurus_Settle
	return

// --- Phases ---

// 50% - THUNDERSTORM. Permanent rain, so Thunder never misses and the fight reads as a storm on
// screen; Thunder Wave's slot becomes Thunder, since the priority paralysis was Phase 0's tool and
// the rain-locked Thunder is this phase's. From here Charge climbs +1 a turn on its own, lightning
// falls on the quiet turns, and sleep and freeze stop sticking.
EncScript_Thundurus_Thunderstorm::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCTHUNDURUSSTORM1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTHUNDURUSSTORM2
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	encsetmove ENC_TARGET_BOSS, 1, MOVE_THUNDER
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 10, ENC_AMOUNT_PERCENT
	call EncScript_Thundurus_Charge1
	call EncScript_Thundurus_Settle
	return

// 15% - WRATH OF THE STORM. The Therian forme, Wildbolt Storm in Thunderbolt's slot, a tailwind and a
// raw Speed bump - "Thundurus gains priority", made visible now that the form change has taken
// Prankster away with the Incarnate forme. Charge is forced to 5 and the rod cooldown is cleared, so
// the countdown runs effectively unbroken from here and the player answers one every turn.
EncScript_Thundurus_Wrath::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCTHUNDURUSWRATH1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTHUNDURUSWRATH2
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_THUNDURUS_THERIAN, EncScript_Thundurus_Wrath_Move, B_ANIM_FORM_CHANGE
EncScript_Thundurus_Wrath_Move:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_WILDBOLT_STORM
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCTHUNDURUSTAILWIND
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 20, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encsetvar 1, 5     @ Charge, forced
	encsetvar 12, 0    @ RodCd
	call EncScript_Thundurus_Settle
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// 1 HP - GROUNDED, and the catch window (guideline 10). The storm lets go of everything it was
// holding and the ball is allowed. Every trigger that could still fire is gated Phase <= 2, so from
// here the fight is quiet. encsetdamagereduction 99 writes the value the automatic catch guard will
// RESTORE, not the live one.
EncScript_Thundurus_Grounded::
	encsetvar 0, 3     @ Phase
	encsetvar 1, 0     @ Charge
	encsetvar 11, 0    @ Discharge
	call EncScript_Thundurus_RevokeOvercharge
	call EncScript_Thundurus_BreakRod
	call EncScript_Thundurus_AnnounceCharge
	removeweather
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	printstring STRINGID_ENCTHUNDURUSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
