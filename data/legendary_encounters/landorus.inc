// Landorus, "The Abundance Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Landorus_Intro:
// 0 Phase (0 Barren Country / 1 Therian Power / 2 Wrath of the Land / 3 Settled), 1 Stability (0-5),
// 2 LastStab (the last ANNOUNCED Stability), 3 Tier (tier perks applied, 0-5), 4 TurnGuard,
// 5 Pillars (0-3), 6 PillarCd, 7 Spent (a pillar is standing this turn), 8 BossMark (Landorus HP% at
// turn open), 9 Scratch (the copy ENC_SNAP_DAMAGE consumes), 10 Shift (0 none / 1 charging /
// 2 reeling), 11 Deso, 12 EventGuard, 13 Quakes, 14 Settled, 15 Gained.
//
// THE LAND IS THE GUARD, in two halves. EncScript_Landorus_ApplyGround is the damage reduction's
// only writer and reads Phase and Shift, but the reduction is deliberately the softest of the five
// Gen 5 bosses: the real mitigation is EARTH PILLARS, a countable stack of turns the player can see,
// spend cheap turns burning, and strip with the same Fire and Ice moves that hold STABILITY down.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so anything
// direction-sensitive uses playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction, written absolutely from Phase and Shift, so a collapse
// ending undoes a collapse starting with no separate teardown path. Phase 3 returns immediately:
// once balls are allowed the engine's own catch guard owns the number.
EncScript_Landorus_ApplyGround:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Landorus_ApplyGround_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Landorus_ApplyGround_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Landorus_ApplyGround_P1
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Landorus_ApplyGround_P0Reel
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Landorus_ApplyGround_P0Reel:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Landorus_ApplyGround_P1:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Landorus_ApplyGround_P1Reel
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Landorus_ApplyGround_P1Reel:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Landorus_ApplyGround_P2:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Landorus_ApplyGround_P2Reel
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Landorus_ApplyGround_P2Reel:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Landorus_ApplyGround_Done:
	return

// THE STABILITY CALLOUT (guideline 14). LastStab holds the last ANNOUNCED level; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on a
// difference - in BOTH directions. Level 0's line is as load-bearing as level 5's: a fall is the
// player's own work and the fight has no other way to confirm Desolation did anything.
EncScript_Landorus_AnnounceStability:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Landorus_AnnounceStability_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Landorus_AnnounceStability_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Landorus_AnnounceStability_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Landorus_AnnounceStability_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Landorus_AnnounceStability_4
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Landorus_AnnounceStability_Done
	printstring STRINGID_ENCLANDORUSSTAB5
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Landorus_AnnounceStability_Latch
EncScript_Landorus_AnnounceStability_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Landorus_AnnounceStability_Done
	printstring STRINGID_ENCLANDORUSSTAB0
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Landorus_AnnounceStability_Latch
EncScript_Landorus_AnnounceStability_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Landorus_AnnounceStability_Done
	printstring STRINGID_ENCLANDORUSSTAB1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Landorus_AnnounceStability_Latch
EncScript_Landorus_AnnounceStability_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Landorus_AnnounceStability_Done
	printstring STRINGID_ENCLANDORUSSTAB2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Landorus_AnnounceStability_Latch
EncScript_Landorus_AnnounceStability_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Landorus_AnnounceStability_Done
	printstring STRINGID_ENCLANDORUSSTAB3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Landorus_AnnounceStability_Latch
EncScript_Landorus_AnnounceStability_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Landorus_AnnounceStability_Done
	printstring STRINGID_ENCLANDORUSSTAB4
	waitmessage B_WAIT_TIME_LONG
EncScript_Landorus_AnnounceStability_Latch:
	enccopyvar 2, 1     @ LastStab = Stability
EncScript_Landorus_AnnounceStability_Done:
	return

// Grants whichever tier perks the current Stability has earned but Tier has not yet applied, in
// order, catching up as many steps as one call needs. Only tiers 1 and 2 hand out something that has
// to be un-granted explicitly (a stat stage, a terrain); tier 3 seeds the first pillar, and tiers 4
// and 5 are read straight off Stability by the triggers that use them, so they turn on and off with
// the meter for free. The Stability callout carries the dialogue, so the stat stage is silent.
EncScript_Landorus_ApplyTier:
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Landorus_ApplyTier_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Landorus_ApplyTier_Two
	encsetvar 3, 1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
EncScript_Landorus_ApplyTier_Two:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Landorus_ApplyTier_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 1, EncScript_Landorus_ApplyTier_Three
	encsetvar 3, 2
	encsetterrain ENC_TERRAIN_GRASSY, 0
	printstring STRINGID_ENCLANDORUSFERTILE
	waitmessage B_WAIT_TIME_LONG
EncScript_Landorus_ApplyTier_Three:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Landorus_ApplyTier_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 2, EncScript_Landorus_ApplyTier_Four
	encsetvar 3, 3
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Landorus_ApplyTier_Four
	call EncScript_Landorus_RaisePillar
EncScript_Landorus_ApplyTier_Four:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Landorus_ApplyTier_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Landorus_ApplyTier_Five
	encsetvar 3, 4
EncScript_Landorus_ApplyTier_Five:
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Landorus_ApplyTier_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 4, EncScript_Landorus_ApplyTier_Done
	encsetvar 3, 5
EncScript_Landorus_ApplyTier_Done:
	return

// The mirror, taking Tier back down toward the current Stability. Standing pillars are deliberately
// NOT deleted on the way past tier 3 - they are the player's to break, not the meter's to reclaim.
EncScript_Landorus_RevokeTier:
	encjumpifvar CMP_EQUAL, 3, 5, EncScript_Landorus_RevokeTier_From5
	encjumpifvar CMP_EQUAL, 3, 4, EncScript_Landorus_RevokeTier_From4
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Landorus_RevokeTier_From3
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Landorus_RevokeTier_From2
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Landorus_RevokeTier_From1
	return
EncScript_Landorus_RevokeTier_From5:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Landorus_RevokeTier_Done
	encsetvar 3, 4
EncScript_Landorus_RevokeTier_From4:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Landorus_RevokeTier_Done
	encsetvar 3, 3
EncScript_Landorus_RevokeTier_From3:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Landorus_RevokeTier_Done
	encsetvar 3, 2
EncScript_Landorus_RevokeTier_From2:
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Landorus_RevokeTier_Done
	removeterrain
	encsetvar 3, 1
EncScript_Landorus_RevokeTier_From1:
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Landorus_RevokeTier_Done
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encsetvar 3, 0
EncScript_Landorus_RevokeTier_Done:
	return

// Adds one pillar up to the cap of 3. A no-op at the cap, so every caller may call it
// unconditionally.
EncScript_Landorus_RaisePillar:
	encjumpifvar CMP_GREATER_THAN, 5, 2, EncScript_Landorus_RaisePillar_Done
	encaddvar 5, 1
	printstring STRINGID_ENCLANDORUSPILLARUP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
EncScript_Landorus_RaisePillar_Done:
	return

// Drops one pillar if any stand, and starts the cooldown so the next one cannot stand immediately.
// The cooldown is only ever raised, never lowered - a pillar spent at OnTurnEnd must not shorten the
// two-turn gap _PillarStands set. Silent no-op at zero.
EncScript_Landorus_BreakPillar:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Landorus_BreakPillar_Done
	encsubvar 5, 1
	printstring STRINGID_ENCLANDORUSPILLARFALLS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Landorus_BreakPillar_Done
	encsetvar 6, 1     @ PillarCd
EncScript_Landorus_BreakPillar_Done:
	return

// --- Trigger scripts ---

// LastStab is parked on a value Stability can never hold so the callout's latch fires on turn 0: the
// player is told the land is Barren before anything happens, and every later change reads as a change.
EncScript_Landorus_Intro::
	printstring STRINGID_ENCLANDORUSINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCLANDORUSINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Landorus_ApplyGround
	encsetvar 2, 9     @ LastStab: nothing announced yet
	call EncScript_Landorus_AnnounceStability
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch can leave it re-armed. Ticks the pillar cooldown (the decrement half of
// the split countdown), climbs the meter on its own from Phase 1, pays the Abundance heal BEFORE the
// mark is taken so it is never counted as the player's damage, moves the tier perks in both
// directions, and runs the recurring cue.
EncScript_Landorus_TurnOpen::
	flushtextbox
	encsetvar 4, 1     @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Landorus_TurnOpen_Clear   @ Phase 3: quiet
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Landorus_TurnOpen_Climb
	encsubvar 6, 1     @ PillarCd
EncScript_Landorus_TurnOpen_Climb:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Landorus_TurnOpen_Heal
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Landorus_TurnOpen_Heal
	encaddvar 1, 1     @ Therian Power: the land answers on its own
EncScript_Landorus_TurnOpen_Heal:
	encjumpifvar CMP_LESS_THAN, 3, 4, EncScript_Landorus_TurnOpen_Mark
	printstring STRINGID_ENCLANDORUSABUNDANCE
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
EncScript_Landorus_TurnOpen_Mark:
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_SET
	call EncScript_Landorus_ApplyTier
	call EncScript_Landorus_RevokeTier
	call EncScript_Landorus_ApplyGround
	call EncScript_Landorus_AnnounceStability
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Landorus_TurnOpen_Clear
	printstring STRINGID_ENCLANDORUSLANDCUE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Landorus_TurnOpen_Clear:
	encsetvar 11, 0    @ Deso
	encsetvar 12, 0    @ EventGuard
	encsetvar 15, 0    @ Gained
	return

// Phase 1+ anti-abuse: the sky is smothered at the top of every turn. Self-disabling - it clears the
// weather its own trigger condition reads.
EncScript_Landorus_Smother::
	printstring STRINGID_ENCLANDORUSSMOTHER
	waitmessage B_WAIT_TIME_SHORT
	removeweather
	return

// AN EARTH PILLAR STANDS. encsetprotect hands Landorus the same protection Protect itself grants, so
// Feint, never-miss moves and contact punishment all resolve exactly as they do against a real
// Protect. PillarCd = 2 plus the decrement in _TurnOpen guarantees at least one clear turn between
// two pillar turns; the pillar itself is spent at _TurnClose.
EncScript_Landorus_PillarStands::
	printstring STRINGID_ENCLANDORUSPILLARSTANDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_PROTECTED_ITSELF
	encsetprotect ENC_TARGET_BOSS
	encsetvar 7, 1     @ Spent
	encsetvar 6, 2     @ PillarCd
	return

// THE TECTONIC SHIFT telegraph. Raised at turn start so the player can spend this turn on the answer
// - a Fire or Ice move landing collapses it - and resolved at OnTurnEnd the same turn.
EncScript_Landorus_ShiftCharge::
	printstring STRINGID_ENCLANDORUSSHIFT1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCLANDORUSSHIFT2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encsetvar 10, 1    @ Shift: charging
	return

// DESOLATION. Fire and Ice are the two types the land recoils from, and one Desolation is taken per
// turn. In Wrath of the Land the meter is locked and the refusal prints instead of the drop - that
// line IS the cue that the rules changed. The pillar falls either way, and a charging Tectonic Shift
// collapses on top of it.
EncScript_Landorus_Desolation::
	encsetvar 11, 1    @ Deso
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Landorus_Desolation_Locked
	printstring STRINGID_ENCLANDORUSDESOLATION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Landorus_Desolation_Pillar
	encsubvar 1, 1     @ Stability
	goto EncScript_Landorus_Desolation_Pillar
EncScript_Landorus_Desolation_Locked:
	printstring STRINGID_ENCLANDORUSDESOREFUSED
	waitmessage B_WAIT_TIME_LONG
EncScript_Landorus_Desolation_Pillar:
	call EncScript_Landorus_BreakPillar
	encjumpifvar CMP_NOT_EQUAL, 10, 1, EncScript_Landorus_Desolation_Fin
	encsetvar 10, 2    @ Shift: reeling
	encsetvar 5, 0     @ every standing pillar comes down with the charge
	printstring STRINGID_ENCLANDORUSSHIFTCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	encsetrecharge ENC_TARGET_BOSS, 2      @ at OnMoveEnd = next turn's action
EncScript_Landorus_Desolation_Fin:
	call EncScript_Landorus_RevokeTier
	call EncScript_Landorus_ApplyGround
	call EncScript_Landorus_AnnounceStability
	return

// THE GROUND STIRS. Any Ground-type move resolving on the field feeds the land - the player's
// included, which is the fight's central trap. Self-disables through Gained.
EncScript_Landorus_GroundStirs::
	encsetvar 15, 1    @ Gained
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Landorus_GroundStirs_Fin
	encaddvar 1, 1     @ Stability
	printstring STRINGID_ENCLANDORUSGROUNDSTIRS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Landorus_GroundStirs_Fin:
	call EncScript_Landorus_ApplyTier
	call EncScript_Landorus_ApplyGround
	call EncScript_Landorus_AnnounceStability
	return

// One of the player's Pokemon fell and the land takes it. EventGuard stops the replacement switch-in
// being processed a second time.
EncScript_Landorus_TheyFell::
	encsetvar 12, 1    @ EventGuard
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Landorus_TheyFell_Fin
	encaddvar 1, 1     @ Stability
	printstring STRINGID_ENCLANDORUSTHEYFELL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Landorus_TheyFell_Fin:
	call EncScript_Landorus_ApplyTier
	call EncScript_Landorus_ApplyGround
	call EncScript_Landorus_AnnounceStability
	return

// A mid-turn switch. Sets EventGuard and nothing else - it exists only so a faint later in the same
// turn is not counted twice.
EncScript_Landorus_TheyEntered::
	encsetvar 12, 1    @ EventGuard
	return

// THE LEDGER. Measures how much of Landorus's max HP the player took off this turn and leaves it in
// Scratch: 5% or more splits a pillar by force. Then the standing pillar spends itself, a reeling
// window closes exactly one player turn after it opened, and CONTINENTAL RUMBLE fires if the meter is
// holding at 5. Self-disables by clearing TurnGuard, which is also the rumble's latch.
EncScript_Landorus_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Landorus_TurnClose_Quiet   @ Phase 3: quiet
	encsetvar 9, 0     @ Scratch
	enccopyvar 9, 8    @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_DAMAGE, EncScript_Landorus_TurnClose_NoDamage
	encjumpifvar CMP_LESS_THAN, 9, 5, EncScript_Landorus_TurnClose_Spend
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Landorus_TurnClose_Spend
	printstring STRINGID_ENCLANDORUSPILLARCRACKS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Landorus_BreakPillar
	goto EncScript_Landorus_TurnClose_Spend
EncScript_Landorus_TurnClose_NoDamage:
	encsetvar 9, 0     @ nothing taken off Landorus this turn
EncScript_Landorus_TurnClose_Spend:
	encjumpifvar CMP_NOT_EQUAL, 7, 1, EncScript_Landorus_TurnClose_Reel
	call EncScript_Landorus_BreakPillar
	encsetvar 7, 0     @ Spent
EncScript_Landorus_TurnClose_Reel:
	encjumpifvar CMP_NOT_EQUAL, 10, 2, EncScript_Landorus_TurnClose_Rumble
	encsetvar 10, 0    @ Shift: the reeling window is over
	call EncScript_Landorus_ApplyGround
EncScript_Landorus_TurnClose_Rumble:
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Landorus_TurnClose_Quiet
	encjumpifvar CMP_NOT_EQUAL, 14, 0, EncScript_Landorus_TurnClose_Quiet   @ Settled
	call EncScript_Landorus_Rumble
EncScript_Landorus_TurnClose_Quiet:
	encsetvar 11, 0    @ Deso
	encsetvar 12, 0    @ EventGuard
	encsetvar 15, 0    @ Gained
	encsetvar 4, 0     @ TurnGuard
	return

// TECTONIC SHIFT lands - the charge was never collapsed. Hazards on BOTH sides, the player's screens
// and the player's stat stages are all scoured; each of those commands is test-and-clear with a fail
// label, so the script branches around whatever had nothing to strip and one line covers the lot.
// Scratch is free to reuse here: _TurnClose at priority 80 has already read it.
EncScript_Landorus_ShiftResolve::
	printstring STRINGID_ENCLANDORUSSHIFTLANDS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EARTHQUAKE
	encsetvar 9, 0     @ Scratch: nothing scoured yet
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Landorus_ShiftResolve_Screens
	encsetvar 9, 1
EncScript_Landorus_ShiftResolve_Screens:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Landorus_ShiftResolve_Stats
	encsetvar 9, 1
EncScript_Landorus_ShiftResolve_Stats:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Landorus_ShiftResolve_Scour
	encsetvar 9, 1
EncScript_Landorus_ShiftResolve_Scour:
	encjumpifvar CMP_NOT_EQUAL, 9, 1, EncScript_Landorus_ShiftResolve_Damage
	printstring STRINGID_ENCLANDORUSSHIFTSCOUR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
EncScript_Landorus_ShiftResolve_Damage:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_GREATER_THAN, 13, 3, EncScript_Landorus_ShiftResolve_D24
	encjumpifvar CMP_GREATER_THAN, 13, 1, EncScript_Landorus_ShiftResolve_D20
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Landorus_ShiftResolve_Fin
EncScript_Landorus_ShiftResolve_D20:
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Landorus_ShiftResolve_Fin
EncScript_Landorus_ShiftResolve_D24:
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Landorus_ShiftResolve_Fin:
	call EncScript_Landorus_RaisePillar
	encaddvar 13, 1    @ Quakes
	encsetvar 10, 0    @ Shift
	return

// CONTINENTAL RUMBLE, the tier-5 payload: the land itself chips the player's side every turn the
// meter holds at 5. The quake throws a fresh pillar up whenever none are standing, so a player who
// burns the stack still has to keep burning it.
// A subroutine called from the tail of _TurnClose, NOT a trigger of its own: nothing in this script
// moves the conditions that would select it (the meter at 5, not yet Settled), so as a trigger it
// re-selected every pass until the per-checkpoint script cap tripped.
EncScript_Landorus_Rumble:
	encjumpifvar CMP_GREATER_THAN, 13, 2, EncScript_Landorus_Rumble_Harder
	printstring STRINGID_ENCLANDORUSRUMBLE
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Landorus_Rumble_Hit
EncScript_Landorus_Rumble_Harder:
	printstring STRINGID_ENCLANDORUSRUMBLEHARDER
	waitmessage B_WAIT_TIME_LONG
EncScript_Landorus_Rumble_Hit:
	playmoveanimation MOVE_MAGNITUDE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	encaddvar 13, 1    @ Quakes
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Landorus_Rumble_Done
	call EncScript_Landorus_RaisePillar
EncScript_Landorus_Rumble_Done:
	return

// THE EARTH SETTLES - six quakes into Wrath of the Land, the land Landorus has been spending runs
// out. Stability falls to 0 and is announced (the fall is the whole beat), every pillar comes down,
// Landorus loses its next action, and Shift = 2 puts the guard at 74 until _TurnClose closes the
// window one turn later. Settled stops the rumble for good; the +1 a turn still runs, so the player
// spends the window watching the land rebuild.
EncScript_Landorus_EarthSettles::
	encsetvar 14, 1    @ Settled
	printstring STRINGID_ENCLANDORUSSETTLES1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCLANDORUSSETTLES2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Stability
	call EncScript_Landorus_RevokeTier
	encsetvar 5, 0     @ Pillars
	encsetvar 6, 0     @ PillarCd
	encsetvar 7, 0     @ Spent
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	encsetrecharge ENC_TARGET_BOSS, 2      @ at OnTurnEnd = next turn's action
	encsetvar 10, 2    @ Shift: reeling
	call EncScript_Landorus_ApplyGround
	call EncScript_Landorus_AnnounceStability
	return

// --- Phases ---

// 50% - THERIAN POWER. The forme change, Sandsear Storm in slot 2, raw Sp. Atk +15% / Def +10%
// (survives Haze), permanent Gravity and a smothered sky. From here the land climbs +1 a turn on its
// own. The new species brings Intimidate with it, but nothing switched in, so it does not fire - the
// ability is flavour rather than a free Attack drop.
EncScript_Landorus_Therian::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCLANDORUSTHERIAN1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCLANDORUSTHERIAN2
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_LANDORUS_THERIAN, EncScript_Landorus_Therian_Move, B_ANIM_FORM_CHANGE
EncScript_Landorus_Therian_Move:
	encsetmove ENC_TARGET_BOSS, 2, MOVE_SANDSEAR_STORM
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCLANDORUSSTORM
	waitmessage B_WAIT_TIME_LONG
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 10, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCLANDORUSGRAVITY
	waitmessage B_WAIT_TIME_LONG
	encsetfieldstatus ENC_FIELD_GRAVITY, 0
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCLANDORUSSMOTHER
	waitmessage B_WAIT_TIME_SHORT
	removeweather
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Landorus_Therian_Pillar
	encaddvar 1, 1     @ Stability
EncScript_Landorus_Therian_Pillar:
	call EncScript_Landorus_RaisePillar
	call EncScript_Landorus_ApplyTier
	call EncScript_Landorus_ApplyGround
	call EncScript_Landorus_AnnounceStability
	return

// 15% - WRATH OF THE LAND. Stability is forced to 5 and LOCKED: Desolation is refused from here and
// says so. The pillars refill, and the six-quake countdown to THE EARTH SETTLES starts.
EncScript_Landorus_Wrath::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCLANDORUSWRATH1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCLANDORUSWRATH2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 1, 5     @ Stability, locked
	call EncScript_Landorus_ApplyTier
	encsetvar 5, 2     @ Pillars
	encsetvar 6, 1     @ PillarCd
	encsetvar 13, 0    @ Quakes
	call EncScript_Landorus_ApplyGround
	call EncScript_Landorus_AnnounceStability
	return

// 1 HP - SETTLED, and the catch window (guideline 10). The land lets go of everything it was holding
// and the ball is allowed. Every trigger that could still fire is gated Phase <= 2, so from here the
// fight is quiet. encsetdamagereduction 99 writes the value the automatic catch guard will RESTORE,
// not the live one.
EncScript_Landorus_Spent::
	encsetvar 0, 3     @ Phase
	encsetvar 10, 0    @ Shift
	encsetvar 5, 0     @ Pillars
	encsetvar 7, 0     @ Spent
	encsetvar 1, 0     @ Stability
	printstring STRINGID_ENCLANDORUSSETTLED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Landorus_RevokeTier
	call EncScript_Landorus_AnnounceStability
	encclearfieldstatus ENC_FIELD_GRAVITY
	removeterrain
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCLANDORUSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
