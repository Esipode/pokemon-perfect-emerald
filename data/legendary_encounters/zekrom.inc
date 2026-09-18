// Zekrom, "The Deep Black Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Zekrom_Intro:
// 0 Phase (0 Ideals / 1 Absolute Conviction / 2 Ideal Awakening / 3 Spent), 1 Conv (CONVICTION,
// 0-5), 2 LastBand (the last ANNOUNCED band), 3 TurnGuard, 4 BossMark (Zekrom HP% at turn open),
// 5 Scratch (the copy ENC_SNAP_DAMAGE consumes; also the enccomparestat and encowned result),
// 6 Ideal (1 Power / 2 Endurance / 3 Speed / 4 Resolve), 7 Style (1 pressure / 2 outlasting /
// 3 speed / 4 defiance), 8 Charge (0 none / 1-3 turns left / 9 the final bolt has resolved),
// 9 Commit (turns left on the declared path; 9 = permanent), 10 Shaken, 11 Switched, 12 Acted,
// 13 Boosted, 14 CueTick, 15 Defiant.
//
// CONVICTION IS THE GUARD, IN BOTH DIRECTIONS. EncScript_Zekrom_ApplyStance is the damage
// reduction's only writer outside _Spent and reads Phase, Conv and Charge absolutely, so a band
// falling undoes a band rising with no separate teardown path. Phase 1's floor (84) is LOWER than
// Phase 0's (86) on purpose: grinding the meter down after the lock is the best thing that can
// happen to the player, and the number has to reward it visibly.
//
// EVERY OnTurnStart TRIGGER IS SELF-DISABLING through the variable its own script writes, and the
// three OnTurnEnd detectors are additionally gated on TurnGuard, which _TurnClose clears last -
// otherwise the re-evaluation pass would re-select them the moment the ledger cleared their flags.
//
// STAT STAGES ARE WRITTEN ABSOLUTELY, the Rayquaza idiom: +12 clamps to the ceiling, then a single
// drop lands on the stage that was meant. That makes a declared stance immune to whatever was
// standing on the board when it was declared.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation. encsetterrain plays the
// terrain's own animation, so Phase 2 needs none of its own.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase 3 returns immediately: once balls are allowed the
// engine's own catch guard owns the number. Gathering a bolt costs Zekrom 5 points of cover in
// Phase 0 and 6 in Phase 1 (8 in Phase 2) - concentration is the fight's cleanest damage window.
EncScript_Zekrom_ApplyStance:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Zekrom_ApplyStance_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zekrom_ApplyStance_P2
// Charge 1-3 is gathering; 0 and the 9 sentinel are both "not gathering".
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Zekrom_ApplyStance_Cool
	encjumpifvar CMP_LESS_THAN, 8, 4, EncScript_Zekrom_ApplyStance_Hot
EncScript_Zekrom_ApplyStance_Cool:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Zekrom_ApplyStance_P1
// Phase 0, not gathering.
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Zekrom_ApplyStance_86
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zekrom_ApplyStance_89
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zekrom_ApplyStance_91
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Zekrom_ApplyStance_86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Zekrom_ApplyStance_89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Zekrom_ApplyStance_91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Zekrom_ApplyStance_P1:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Zekrom_ApplyStance_84
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zekrom_ApplyStance_89
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zekrom_ApplyStance_92
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Zekrom_ApplyStance_84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Zekrom_ApplyStance_92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
// Gathering a bolt, in Phase 0 or 1.
EncScript_Zekrom_ApplyStance_Hot:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Zekrom_ApplyStance_HotP1
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Zekrom_ApplyStance_81
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zekrom_ApplyStance_84
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zekrom_ApplyStance_86
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Zekrom_ApplyStance_81:
	encsetdamagereduction ENC_TARGET_BOSS, 81
	return
EncScript_Zekrom_ApplyStance_HotP1:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Zekrom_ApplyStance_78
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zekrom_ApplyStance_83
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zekrom_ApplyStance_86
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Zekrom_ApplyStance_78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
EncScript_Zekrom_ApplyStance_83:
	encsetdamagereduction ENC_TARGET_BOSS, 83
	return
// Phase 2: the meter is pinned, so the band is gone. Flat, and the final bolt takes another 8.
EncScript_Zekrom_ApplyStance_P2:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Zekrom_ApplyStance_88
	encjumpifvar CMP_GREATER_THAN, 8, 3, EncScript_Zekrom_ApplyStance_88
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Zekrom_ApplyStance_88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
EncScript_Zekrom_ApplyStance_Done:
	return

// THE BAND CALLOUT (guideline 14). LastBand holds the last ANNOUNCED band; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a difference - in BOTH directions. The falling lines are as load-bearing as the rising ones: a
// fall is the player's own work and the guard has no other way to confirm it did anything.
// Bands: 0-1 DOUBT, 2-3 CONVICTION, 4 CERTAINTY, 5 ABSOLUTE.
EncScript_Zekrom_Band:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Zekrom_Band_Doubt
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zekrom_Band_Conviction
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zekrom_Band_Certainty
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Zekrom_Band_Done
	encsetvar 2, 3
	printstring STRINGID_ENCZEKROMBANDABSOLUTE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_Band_Doubt:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Zekrom_Band_Done
	encsetvar 2, 0
	printstring STRINGID_ENCZEKROMBANDDOUBT
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_Band_Conviction:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Zekrom_Band_Done
	encsetvar 2, 1
	printstring STRINGID_ENCZEKROMBANDCONVICTION
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_Band_Certainty:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Zekrom_Band_Done
	encsetvar 2, 2
	printstring STRINGID_ENCZEKROMBANDCERTAINTY
	waitmessage B_WAIT_TIME_LONG
EncScript_Zekrom_Band_Done:
	return

// One step of travel, floored at 0 and clamped at 5. Both silent; every caller prints its own line.
EncScript_Zekrom_ConvDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Zekrom_ConvDown_Done
	encsubvar 1, 1
EncScript_Zekrom_ConvDown_Done:
	return

EncScript_Zekrom_ConvUp:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Zekrom_ConvUp_Done
	encaddvar 1, 1
EncScript_Zekrom_ConvUp_Done:
	return

// THE DECLARED STANCE, written at declaration time and never per turn. It leads by taking the
// PREVIOUS Ideal's side effects off unconditionally - both are no-ops when they were not up - then
// Hazes its own stages so a stance cannot stack on the one before it, then writes the new stance
// absolutely. A player Haze landing between declarations genuinely strips the stance until the next
// one, which is a real reward rather than a bug.
EncScript_Zekrom_ApplyIdeal:
	encclearsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_TAILWIND
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encresetstats ENC_TARGET_BOSS, EncScript_Zekrom_ApplyIdeal_Stance
EncScript_Zekrom_ApplyIdeal_Stance:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Zekrom_ApplyIdeal_Power
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Zekrom_ApplyIdeal_Endurance
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zekrom_ApplyIdeal_Speed
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Zekrom_ApplyIdeal_Resolve
	return
EncScript_Zekrom_ApplyIdeal_Power:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -4
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return
EncScript_Zekrom_ApplyIdeal_Endurance:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -5
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -5
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return
EncScript_Zekrom_ApplyIdeal_Speed:
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -4
	encsetsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	return
// Resolve takes no stage at all. Its stance is that you do not get to change course either.
EncScript_Zekrom_ApplyIdeal_Resolve:
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCZEKROMTRAPPED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	return

// The declaration line. Each one hints at its own counter without naming it, and it is reused by
// both phase locks so the Ideal is always restated when it is re-written.
EncScript_Zekrom_IdealLine:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Zekrom_IdealLine_Power
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Zekrom_IdealLine_Endurance
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zekrom_IdealLine_Speed
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Zekrom_IdealLine_Resolve
	return
EncScript_Zekrom_IdealLine_Power:
	printstring STRINGID_ENCZEKROMIDEALPOWER
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_IdealLine_Endurance:
	printstring STRINGID_ENCZEKROMIDEALENDURANCE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_IdealLine_Speed:
	printstring STRINGID_ENCZEKROMIDEALSPEED
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_IdealLine_Resolve:
	printstring STRINGID_ENCZEKROMIDEALRESOLVE
	waitmessage B_WAIT_TIME_LONG
	return

// The telegraph's second line, naming the Ideal's version of the bolt. A player who has seen Power's
// payload once knows what is coming the next time Power is declared.
EncScript_Zekrom_BoltLine:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Zekrom_BoltLine_Power
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Zekrom_BoltLine_Endurance
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zekrom_BoltLine_Speed
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Zekrom_BoltLine_Resolve
	return
EncScript_Zekrom_BoltLine_Power:
	printstring STRINGID_ENCZEKROMBOLTPOWER
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_BoltLine_Endurance:
	printstring STRINGID_ENCZEKROMBOLTENDURANCE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_BoltLine_Speed:
	printstring STRINGID_ENCZEKROMBOLTSPEED
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zekrom_BoltLine_Resolve:
	printstring STRINGID_ENCZEKROMBOLTRESOLVE
	waitmessage B_WAIT_TIME_LONG
	return

// A BOLT STRIKE LANDS. The payload is a percentage of the PLAYER's max HP, so it scales with the
// player's team rather than with Zekrom's, and it is a direct HP write that a Substitute never sees.
// Each Ideal carries its own payload and its own extra; absolute certainty (Conv == 5) adds 4 more.
// Phase 2 fires BOLT STRIKE: IDEAL instead - bigger, unbreakable, and it sets Charge to 9, which is
// what the Phase 3 trigger watches.
EncScript_Zekrom_FireBolt:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zekrom_FireBolt_Final
	printstring STRINGID_ENCZEKROMBOLTSTRIKE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Zekrom_FireBolt_Power
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Zekrom_FireBolt_Endurance
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zekrom_FireBolt_Speed
// Resolve, and the fallback for an Ideal that was never declared.
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Zekrom_FireBolt_Resolve5
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Zekrom_FireBolt_Spent
EncScript_Zekrom_FireBolt_Resolve5:
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	goto EncScript_Zekrom_FireBolt_Spent
EncScript_Zekrom_FireBolt_Power:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Zekrom_FireBolt_Power5
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	goto EncScript_Zekrom_FireBolt_PowerDrop
EncScript_Zekrom_FireBolt_Power5:
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
EncScript_Zekrom_FireBolt_PowerDrop:
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPDEF, -1
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	goto EncScript_Zekrom_FireBolt_Spent
EncScript_Zekrom_FireBolt_Endurance:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Zekrom_FireBolt_Endurance5
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Zekrom_FireBolt_EnduranceHeal
EncScript_Zekrom_FireBolt_Endurance5:
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
EncScript_Zekrom_FireBolt_EnduranceHeal:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	goto EncScript_Zekrom_FireBolt_Spent
EncScript_Zekrom_FireBolt_Speed:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Zekrom_FireBolt_Speed5
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	goto EncScript_Zekrom_FireBolt_Spent
EncScript_Zekrom_FireBolt_Speed5:
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
EncScript_Zekrom_FireBolt_Spent:
	encsetvar 8, 0     @ Charge
	call EncScript_Zekrom_ApplyStance
	return
// BOLT STRIKE: IDEAL. It cannot be broken, it does not repeat, and the phase ends on it either way.
EncScript_Zekrom_FireBolt_Final:
	printstring STRINGID_ENCZEKROMBOLTIDEALFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Zekrom_FireBolt_FinalPower
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Zekrom_FireBolt_FinalEndurance
	goto EncScript_Zekrom_FireBolt_FinalDone
EncScript_Zekrom_FireBolt_FinalPower:
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPDEF, -1
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	goto EncScript_Zekrom_FireBolt_FinalDone
EncScript_Zekrom_FireBolt_FinalEndurance:
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Zekrom_FireBolt_FinalDone:
	encsetvar 8, 9     @ Charge: what the Phase 3 trigger watches
	call EncScript_Zekrom_ApplyStance
	return

// --- Trigger entry points ---

// OnBattleStart. _ApplyStance runs on turn 0 so the guard already reflects the table rather than the
// Properties: seed, and LastBand starts at 9 so the opening band line is a real announcement.
// encowned is the fight's one read of anything outside the battle: a player who has CAUGHT Reshiram
// starts Zekrom at Conviction 1, because it recognises a trainer who has already answered the other
// question. Commit is left at 0, so _ChoosePath declares the first Ideal on turn 1.
// No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Zekrom_Intro::
	printstring STRINGID_ENCZEKROMINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCZEKROMINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 2, 9     @ LastBand: nothing announced yet
	encowned SPECIES_RESHIRAM, 5
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Zekrom_Intro_Stance
	printstring STRINGID_ENCZEKROMKNOWSTHEWHITE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1     @ Conv
EncScript_Zekrom_Intro_Stance:
	encsetvar 5, 0
	call EncScript_Zekrom_ApplyStance
	call EncScript_Zekrom_Band
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch out can leave it re-armed.
EncScript_Zekrom_TurnOpen::
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET
	encsetvar 11, 0    @ Switched - a switch that landed after _TurnClose left this set
// The commitment ticks down. At 0 the path wavers and _ChoosePath (priority 12) re-declares on this
// same turn, so no turn is ever spent on an undeclared stance. 9 is permanent and never ticks.
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Zekrom_TurnOpen_Endure
	encjumpifvar CMP_GREATER_THAN, 9, 8, EncScript_Zekrom_TurnOpen_Endure
	encsubvar 9, 1
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Zekrom_TurnOpen_Endure
	printstring STRINGID_ENCZEKROMPATHWAVERS
	waitmessage B_WAIT_TIME_LONG
// Endurance upkeep, only while a path is actually standing.
EncScript_Zekrom_TurnOpen_Endure:
	encjumpifvar CMP_NOT_EQUAL, 6, 2, EncScript_Zekrom_TurnOpen_Charge
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Zekrom_TurnOpen_Charge
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
// The bolt gathers. Under Speed it resolves HERE, before the player acts - that is the whole of
// Speed's threat, and the reason its payload is the second smallest.
// Charge 9 is a two-meaning sentinel: in Phase 2 it means the final bolt has resolved and the
// Phase 3 trigger is watching it, and in Phases 0-1 it is Speed's one-turn cooldown - _BeginCharge
// is gated on Charge == 0, so without it a bolt that fired at priority 10 would be re-telegraphed at
// priority 16 on the same turn and Speed's cycle would run a turn shorter than everyone else's.
EncScript_Zekrom_TurnOpen_Charge:
	encjumpifvar CMP_LESS_THAN, 8, 4, EncScript_Zekrom_TurnOpen_ChargeTick
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zekrom_TurnOpen_Trap
	encsetvar 8, 0     @ Phases 0-1: the cooldown is spent, and _BeginCharge may re-arm this turn
	goto EncScript_Zekrom_TurnOpen_Trap
EncScript_Zekrom_TurnOpen_ChargeTick:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Zekrom_TurnOpen_Trap
	encsubvar 8, 1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zekrom_TurnOpen_FinalCue
	encjumpifvar CMP_NOT_EQUAL, 6, 3, EncScript_Zekrom_TurnOpen_ChargeCue
	encjumpifvar CMP_NOT_EQUAL, 8, 1, EncScript_Zekrom_TurnOpen_ChargeCue
	call EncScript_Zekrom_FireBolt
	encsetvar 8, 9     @ the cooldown, cleared at the top of the next turn
	call EncScript_Zekrom_ApplyStance
	goto EncScript_Zekrom_TurnOpen_Trap
EncScript_Zekrom_TurnOpen_ChargeCue:
	printstring STRINGID_ENCZEKROMBOLTCUE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Zekrom_TurnOpen_Trap
EncScript_Zekrom_TurnOpen_FinalCue:
	printstring STRINGID_ENCZEKROMBOLTIDEALCUE
	waitmessage B_WAIT_TIME_SHORT
// Resolve re-arms its trap: encsettrapped survives end of turn, but a switch-in or a phase script
// could have cleared it.
EncScript_Zekrom_TurnOpen_Trap:
	encjumpifvar CMP_NOT_EQUAL, 6, 4, EncScript_Zekrom_TurnOpen_Cue
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
// The recurring cue (guideline 14), alternated so a long fight does not read as a stuck message.
EncScript_Zekrom_TurnOpen_Cue:
	call EncScript_Zekrom_Band
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Zekrom_TurnOpen_Done
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_Zekrom_TurnOpen_CueA
	encsetvar 14, 0
	printstring STRINGID_ENCZEKROMPATHISSET
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zekrom_TurnOpen_CueA:
	encsetvar 14, 1
	printstring STRINGID_ENCZEKROMWILLNOTCHANGE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zekrom_TurnOpen_Done:
	return

// THE DECLARATION. Four roughly equal arms from three chained rolls: 25, then 33 of the remaining
// 75, then 50 of the remaining 50. encjumpifchance is memoryless and nothing stores the previous
// pick, so the same Ideal twice in a row is possible - it reads as Zekrom doubling down.
EncScript_Zekrom_ChoosePath::
	encjumpifchance 25, EncScript_Zekrom_ChoosePath_Power
	encjumpifchance 33, EncScript_Zekrom_ChoosePath_Endurance
	encjumpifchance 50, EncScript_Zekrom_ChoosePath_Speed
	encsetvar 6, 4     @ Resolve
	goto EncScript_Zekrom_ChoosePath_Declare
EncScript_Zekrom_ChoosePath_Power:
	encsetvar 6, 1
	goto EncScript_Zekrom_ChoosePath_Declare
EncScript_Zekrom_ChoosePath_Endurance:
	encsetvar 6, 2
	goto EncScript_Zekrom_ChoosePath_Declare
EncScript_Zekrom_ChoosePath_Speed:
	encsetvar 6, 3
EncScript_Zekrom_ChoosePath_Declare:
	printstring STRINGID_ENCZEKROMCHOSENPATH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Zekrom_IdealLine
	call EncScript_Zekrom_ApplyIdeal
	encsetvar 9, 4     @ Commit
	call EncScript_Zekrom_ApplyStance
	return

// Holding a stat boost is commitment, and the ledger reads it as standing your ground. Silent.
EncScript_Zekrom_HoldsGround::
	encsetvar 13, 1    @ Boosted
	return

// THE TELEGRAPH, announced before the player picks a move so the turn can be spent on the answer.
// Charge 3 ticked at OnTurnStart and resolved at OnTurnEnd is two full turns of play.
EncScript_Zekrom_BeginCharge::
	printstring STRINGID_ENCZEKROMBOLTGATHERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	encsetvar 8, 3     @ Charge
	call EncScript_Zekrom_BoltLine
	call EncScript_Zekrom_ApplyStance   @ concentration costs it cover
	return

// Somebody walked out. The gathering Charge is deliberately NOT reset - switching out of a Bolt
// Strike does not buy time - and the forfeited meter movement is the real cost, paid at the ledger.
// Under Resolve the player cannot get here at all, which is the point of the trap.
EncScript_Zekrom_WatchesRetreat::
	encsetvar 11, 1    @ Switched
	printstring STRINGID_ENCZEKROMWATCHESRETREAT
	waitmessage B_WAIT_TIME_LONG
	return

// The player stood and swung. Silent - the ledger's own line covers it.
EncScript_Zekrom_Attacked::
	encsetvar 12, 1    @ Acted
	return

// POWER'S COUNTER: you outlasted it, at half health or better, or behind a Protect.
EncScript_Zekrom_ShakenOutlast::
	encsetvar 10, 1    @ Shaken
	printstring STRINGID_ENCZEKROMSHAKEOUTLAST
	waitmessage B_WAIT_TIME_LONG
	return

// RESOLVE'S COUNTER: you forced it off its course with a status or a stat drop.
EncScript_Zekrom_ShakenDisrupt::
	encsetvar 10, 1    @ Shaken
	printstring STRINGID_ENCZEKROMSHAKEDISRUPT
	waitmessage B_WAIT_TIME_LONG
	return

// The one feeder that is also a style read.
EncScript_Zekrom_Defiance::
	encsetvar 15, 1    @ Defiant
	encsetvar 7, 4     @ Style: defiance
	return

// THE LEDGER. Measures the turn, resolves or breaks the bolt, and only then moves the meter - so the
// guard follows the meter and the band line follows the guard. Scratch carries the turn's damage
// figure all the way to the charge-break test, which is why nothing zeroes it in between.
EncScript_Zekrom_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Zekrom_TurnClose_Quiet
// SPEED READ. enccomparestat writes 0 A lower / 1 equal / 2 A higher, so anything but 2 means the
// player is at or above Zekrom's turn-order speed - Speed's counter, and a style signal either way.
	enccomparestat ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 5
	encjumpifvar CMP_GREATER_THAN, 5, 1, EncScript_Zekrom_TurnClose_Damage
	encsetvar 7, 3     @ Style: speed
	encjumpifvar CMP_NOT_EQUAL, 6, 3, EncScript_Zekrom_TurnClose_Damage
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Zekrom_TurnClose_Damage
	encsetvar 10, 1    @ Shaken
	printstring STRINGID_ENCZEKROMSHAKESLOWED
	waitmessage B_WAIT_TIME_LONG
// DAMAGE READ. 8% of Zekrom's max HP in one turn is Endurance's counter, and the pressure figure the
// charge break tests against.
EncScript_Zekrom_TurnClose_Damage:
	encsetvar 5, 0
	enccopyvar 5, 4    @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Zekrom_TurnClose_NoDamage
	goto EncScript_Zekrom_TurnClose_Measured
EncScript_Zekrom_TurnClose_NoDamage:
	encsetvar 5, 0     @ nothing taken off Zekrom this turn
EncScript_Zekrom_TurnClose_Measured:
	encjumpifvar CMP_LESS_THAN, 5, 8, EncScript_Zekrom_TurnClose_Outlast
	encsetvar 7, 1     @ Style: pressure
	encjumpifvar CMP_NOT_EQUAL, 6, 2, EncScript_Zekrom_TurnClose_Defiant
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Zekrom_TurnClose_Defiant
	encsetvar 10, 1    @ Shaken
	printstring STRINGID_ENCZEKROMSHAKEOVERWHELM
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Zekrom_TurnClose_Defiant
// Took nothing off it at all this turn - that is outlasting, and it is a style in its own right.
// A turn that landed 1-7% is neither, and leaves the last decisive signal standing.
EncScript_Zekrom_TurnClose_Outlast:
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Zekrom_TurnClose_Defiant
	encsetvar 7, 2     @ Style: outlasting
EncScript_Zekrom_TurnClose_Defiant:
	encjumpifvar CMP_EQUAL, 15, 0, EncScript_Zekrom_TurnClose_Charge
	encsetvar 7, 4     @ Style: defiance outranks every other read
// THE BOLT RESOLVES, BREAKS, OR KEEPS GATHERING. Resolve's bolt cannot be broken, and neither can
// Phase 2's. Everything else breaks on the same 8% of max HP that contradicts Endurance.
EncScript_Zekrom_TurnClose_Charge:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Zekrom_TurnClose_Ledger
	encjumpifvar CMP_GREATER_THAN, 8, 3, EncScript_Zekrom_TurnClose_Ledger   @ 9: already resolved
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Zekrom_TurnClose_Fire
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Zekrom_TurnClose_Ledger
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Zekrom_TurnClose_Ledger
	encjumpifvar CMP_LESS_THAN, 5, 8, EncScript_Zekrom_TurnClose_Ledger
	printstring STRINGID_ENCZEKROMFOCUSBREAKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetvar 8, 0     @ Charge
	call EncScript_Zekrom_ConvDown
	goto EncScript_Zekrom_TurnClose_Crit   @ the break IS this turn's step; the ledger is skipped
EncScript_Zekrom_TurnClose_Fire:
	call EncScript_Zekrom_FireBolt
// THE CONVICTION LEDGER, one step of travel per turn. Phase 2 skips it - Conv is pinned at 5.
EncScript_Zekrom_TurnClose_Ledger:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zekrom_TurnClose_Crit
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Zekrom_TurnClose_Feed
	printstring STRINGID_ENCZEKROMSHAKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	call EncScript_Zekrom_ConvDown
	goto EncScript_Zekrom_TurnClose_Crit
// The commitment trio: holding boosts, defiance, or staying in AND attacking.
EncScript_Zekrom_TurnClose_Feed:
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Zekrom_TurnClose_Rise
	encjumpifvar CMP_EQUAL, 15, 1, EncScript_Zekrom_TurnClose_Rise
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Zekrom_TurnClose_Crit
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Zekrom_TurnClose_Crit
EncScript_Zekrom_TurnClose_Rise:
	call EncScript_Zekrom_ConvUp
	printstring STRINGID_ENCZEKROMREFUSESTOYIELD
	waitmessage B_WAIT_TIME_LONG
// Absolute certainty strikes true, and it is visible in the crit rate rather than hidden in a
// number. Written absolutely in both directions so the meter can take it back.
EncScript_Zekrom_TurnClose_Crit:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Zekrom_TurnClose_CritOn
	encsetcrit ENC_TARGET_BOSS, FALSE
	goto EncScript_Zekrom_TurnClose_Settle
EncScript_Zekrom_TurnClose_CritOn:
	encsetcrit ENC_TARGET_BOSS, TRUE
EncScript_Zekrom_TurnClose_Settle:
	call EncScript_Zekrom_ApplyStance
	call EncScript_Zekrom_Band
EncScript_Zekrom_TurnClose_Quiet:
	encsetvar 10, 0    @ Shaken
	encsetvar 11, 0    @ Switched
	encsetvar 12, 0    @ Acted
	encsetvar 13, 0    @ Boosted
	encsetvar 15, 0    @ Defiant
	encsetvar 3, 0     @ TurnGuard - cleared last, and it is what closes the three detectors above
	return

// --- Phase transitions ---

// 50% - ABSOLUTE CONVICTION. One beat with one clear "the rules changed" message, so the player has
// a single moment to reason from. The Ideal locks to the MIRROR of how the player has been fighting
// and can never change again, which turns the commitment from a timer into the exploit: one counter,
// learned once, works for the entire back half of the fight.
EncScript_Zekrom_AbsoluteConviction::
	printstring STRINGID_ENCZEKROMABSOLUTECONVICTION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 1     @ Phase
	encsetvar 1, 4     @ Conv
	encsetvar 9, 9     @ Commit: permanent
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Zekrom_AbsoluteConviction_Power
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Zekrom_AbsoluteConviction_Endurance
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Zekrom_AbsoluteConviction_Speed
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Zekrom_AbsoluteConviction_Resolve
// Style 0 - nothing decisive yet, which needs the threshold to be crossed on turn 1. Keep whatever
// is declared, and fall back to Power if even that is unset.
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Zekrom_AbsoluteConviction_Lock
EncScript_Zekrom_AbsoluteConviction_Power:
	encsetvar 6, 1
	goto EncScript_Zekrom_AbsoluteConviction_Lock
EncScript_Zekrom_AbsoluteConviction_Endurance:
	encsetvar 6, 2
	goto EncScript_Zekrom_AbsoluteConviction_Lock
EncScript_Zekrom_AbsoluteConviction_Speed:
	encsetvar 6, 3
	goto EncScript_Zekrom_AbsoluteConviction_Lock
EncScript_Zekrom_AbsoluteConviction_Resolve:
	encsetvar 6, 4
EncScript_Zekrom_AbsoluteConviction_Lock:
	call EncScript_Zekrom_IdealLine
	call EncScript_Zekrom_ApplyIdeal
	encsetmove ENC_TARGET_BOSS, 3, MOVE_OUTRAGE   @ Zen Headbutt out, the move that cannot change course in
	call EncScript_Zekrom_ApplyStance
	call EncScript_Zekrom_Band
	printstring STRINGID_ENCZEKROMWILLNOTCHANGEEVER
	waitmessage B_WAIT_TIME_LONG
	return

// 15% - IDEAL AWAKENING. Conviction is pinned at 5 and Electric Terrain goes up permanently: Zekrom
// is grounded and has no Levitate, so the terrain powers its own Electric moves and blocks sleep on
// both grounded sides. The one curestatus here is the fight's ONLY status displacement, after status
// has been allowed to work completely through Phases 0 and 1 (guideline 8), and the single
// encresetstats is the fight's only evasion scour. The Ideal is untouched - the Awakening is the
// PLAYER's Ideal, inherited from the Phase 1 lock.
EncScript_Zekrom_Awakening::
	printstring STRINGID_ENCZEKROMIDEALAWAKENING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 2     @ Phase
	encsetvar 1, 5     @ Conv
	encsetvar 9, 9     @ Commit: permanent
	removeterrain
	encsetterrain ENC_TERRAIN_ELECTRIC
	curestatus BS_OPPONENT1
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Zekrom_Awakening_Stance
EncScript_Zekrom_Awakening_Stance:
	call EncScript_Zekrom_IdealLine
	call EncScript_Zekrom_ApplyIdeal
	encsetvar 8, 3     @ Charge: BOLT STRIKE: IDEAL begins gathering immediately
	call EncScript_Zekrom_ApplyStance
	printstring STRINGID_ENCZEKROMBOLTIDEAL
	waitmessage B_WAIT_TIME_LONG
	return

// SPENT, and the catch window (guideline 10). Reached three ways - the final bolt resolving, or the
// <= 1% deadlock fallback at either checkpoint - and Once on all of them, so the first selection
// disables the rest and Phase 3 closes the conditions on the others. The teardown is the exact
// mirror of the Properties: block plus everything the fight raised. Phase is written FIRST so
// _ApplyStance can never re-raise the guard behind it, and encsetdamagereduction 99 writes the value
// the automatic catch guard will RESTORE, not the live one.
EncScript_Zekrom_Spent::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCZEKROMCONVICTIONFALTERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Conv
	encsetvar 6, 0     @ Ideal
	encsetvar 8, 0     @ Charge
	encsetvar 9, 0     @ Commit
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encclearsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_TAILWIND
	encsetcrit ENC_TARGET_BOSS, FALSE
	removeterrain
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Zekrom_Spent_NoStats
EncScript_Zekrom_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCZEKROMWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
