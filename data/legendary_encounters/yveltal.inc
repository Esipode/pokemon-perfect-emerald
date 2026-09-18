// Yveltal, "The Destruction Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Yveltal_Intro:
// 0 Phase (0 Predation / 1 Wings of Destruction / 2 Destruction Incarnate / 3 Aftermath),
// 1 Destruction (0-10 meter), 2 MarkTurns (0 = unmarked; counts up while marked; DOOMED_THRESHOLD
// fires the payoff and resets), 3 WingChargeTurns (0 = not charging; counts down), 4 Charge (dual
// duty: Phase 0 counts down to the next Life Drain tick, 3->0 cyclic; Phase 2 counts down to The
// Destruction of All resolving, 3->0, once), 5 TurnGuard, 6 LastReductionTier (last ANNOUNCED guard
// tier), 7 Scratch (encreadstat's target in Life Drain; also the consumable copy any ENC_SNAP_DAMAGE
// check reads, so the persistent mark it's copied from is never itself consumed), 8 HandledMoveEvent
// (per-move-event guard shared by DarkFlyingHit/InterruptCheck, cleared every OnTurnStart),
// 9 PhaseMark (dual duty: Phase 1 = player's HP% at turn-open, refreshed every turn; Phase 2 =
// Yveltal's own HP% at the moment Destruction Incarnate began, held for the whole charge).
//
// DESTRUCTION IS THE GUARD AND THE OFFENSE, BOTH WRITTEN ABSOLUTELY. _ApplyGuard and _ApplyOffense
// are the reduction's and the attack stat's only writers, reading Destruction alone, so a band
// falling undoes a band rising with no separate teardown path (the Rayquaza idiom: push stat stages
// to their ceiling, then land a single drop on the stage that was meant).
//
// EVERY OnMoveEnd TRIGGER SELF-DISABLES. DarkFlyingHit and InterruptCheck have no variable of their
// own to fall out of, so they share HandledMoveEvent, which OnTurnStart clears - safe because the two
// react to opposite-direction hits on the same event and can never both be eligible on one move.
// MarkBattler falls out through MarkTurns, which it sets to 1 as its own first line.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so every checkpoint script here
// uses playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase 3 (Aftermath) returns immediately: once balls are
// allowed the engine's own catch guard owns the number.
EncScript_Yveltal_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Yveltal_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Yveltal_ApplyGuard_80
	encjumpifvar CMP_LESS_THAN, 1, 7, EncScript_Yveltal_ApplyGuard_85
	encjumpifvar CMP_LESS_THAN, 1, 10, EncScript_Yveltal_ApplyGuard_90
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Yveltal_ApplyGuard_80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Yveltal_ApplyGuard_85:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Yveltal_ApplyGuard_90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
EncScript_Yveltal_ApplyGuard_Done:
	return

// THE SINGLE WRITER of Yveltal's own offense: Attack and Sp. Atk stages, written absolutely (push to
// +6, then land the single drop that reaches the stage meant) so nothing a player does to Yveltal's
// stats - Haze, a switch, anything - outlives a band change.
EncScript_Yveltal_ApplyOffense:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Yveltal_ApplyOffense_0
	encjumpifvar CMP_LESS_THAN, 1, 7, EncScript_Yveltal_ApplyOffense_1
	encjumpifvar CMP_LESS_THAN, 1, 10, EncScript_Yveltal_ApplyOffense_2
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -10
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -10
	return
EncScript_Yveltal_ApplyOffense_0:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	return
EncScript_Yveltal_ApplyOffense_1:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -11
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -11
	return
EncScript_Yveltal_ApplyOffense_2:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -10
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -10
	return

// THE BAND CALLOUT (guideline 14). LastReductionTier holds the last ANNOUNCED tier; each arm stays
// quiet if that's what was last said, so the line prints only on a real change, in both directions.
EncScript_Yveltal_Band:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Yveltal_Band_Low
	encjumpifvar CMP_LESS_THAN, 1, 7, EncScript_Yveltal_Band_Med
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Yveltal_Band_Done
	encsetvar 6, 2
	printstring STRINGID_ENCYVELTALBANDHIGH
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Yveltal_Band_Low:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Yveltal_Band_Done
	encsetvar 6, 0
	printstring STRINGID_ENCYVELTALBANDLOW
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Yveltal_Band_Med:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Yveltal_Band_Done
	encsetvar 6, 1
	printstring STRINGID_ENCYVELTALBANDMED
	waitmessage B_WAIT_TIME_LONG
EncScript_Yveltal_Band_Done:
	return

// Adds \amount to Destruction, clamped at 10. Var 7 (Scratch) is free at every call site.
EncScript_Yveltal_RaiseDestruction_1:
	encjumpifvar CMP_EQUAL, 1, 10, EncScript_Yveltal_RaiseDestruction_Done
	encaddvar 1, 1
EncScript_Yveltal_RaiseDestruction_Done:
	return
EncScript_Yveltal_RaiseDestruction_2:
	encjumpifvar CMP_EQUAL, 1, 10, EncScript_Yveltal_RaiseDestruction_Done
	encaddvar 1, 1
	encjumpifvar CMP_EQUAL, 1, 10, EncScript_Yveltal_RaiseDestruction_Done
	encaddvar 1, 1
	return

// Re-marks whoever is in the player's one battler slot - always PLAYER_LEFT (guideline 11's known
// singles-only gap), which is also why a mark never needs a separate switch-in re-announcement.
EncScript_Yveltal_MarkBattler::
	encsetvar 2, 1     @ MarkTurns
	printstring STRINGID_ENCYVELTALMARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// LIFE DRAIN. encreadstat checks Attack then Sp. Atk for a live positive stage (raw domain, 6 =
// neutral) and steals exactly that excess - Atk first, since it's the thematically "stolen power"
// stat this fight leads with. Var 7 (Scratch) holds the read result; encchangestat's stage argument
// is a compile-time literal, so the exact amount read is resolved by branching, not by feeding the
// variable straight through.
EncScript_Yveltal_LifeDrain:
	printstring STRINGID_ENCYVELTALLIFEDRAIN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	encreadstat ENC_TARGET_PLAYER_LEFT, STAT_ATK, 7
	encjumpifvar CMP_GREATER_THAN, 7, 6, EncScript_Yveltal_LifeDrain_StealAtk
	encreadstat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, 7
	encjumpifvar CMP_GREATER_THAN, 7, 6, EncScript_Yveltal_LifeDrain_StealSpAtk
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealAtk:
	encjumpifvar CMP_LESS_THAN, 7, 8, EncScript_Yveltal_LifeDrain_StealAtk1
	encjumpifvar CMP_LESS_THAN, 7, 9, EncScript_Yveltal_LifeDrain_StealAtk2
	encjumpifvar CMP_LESS_THAN, 7, 10, EncScript_Yveltal_LifeDrain_StealAtk3
	encjumpifvar CMP_LESS_THAN, 7, 11, EncScript_Yveltal_LifeDrain_StealAtk4
	encjumpifvar CMP_LESS_THAN, 7, 12, EncScript_Yveltal_LifeDrain_StealAtk5
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -6
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealAtk5:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -5
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 5
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealAtk4:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -4
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 4
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealAtk3:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -3
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 3
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealAtk2:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -2
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealAtk1:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -1
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealSpAtk:
	encjumpifvar CMP_LESS_THAN, 7, 8, EncScript_Yveltal_LifeDrain_StealSpAtk1
	encjumpifvar CMP_LESS_THAN, 7, 9, EncScript_Yveltal_LifeDrain_StealSpAtk2
	encjumpifvar CMP_LESS_THAN, 7, 10, EncScript_Yveltal_LifeDrain_StealSpAtk3
	encjumpifvar CMP_LESS_THAN, 7, 11, EncScript_Yveltal_LifeDrain_StealSpAtk4
	encjumpifvar CMP_LESS_THAN, 7, 12, EncScript_Yveltal_LifeDrain_StealSpAtk5
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealSpAtk5:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -5
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 5
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealSpAtk4:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -4
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 4
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealSpAtk3:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -3
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 3
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealSpAtk2:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	goto EncScript_Yveltal_LifeDrain_Hp
EncScript_Yveltal_LifeDrain_StealSpAtk1:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
EncScript_Yveltal_LifeDrain_Hp:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	call EncScript_Yveltal_RaiseDestruction_1
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
	return

// DESTRUCTION PULSE. Test-and-clear chains straight through, matching encclearscreens/hazards'
// shared shape; Scratch (var 7) is free here and carries "something was actually found" across the
// two steps.
EncScript_Yveltal_DestructionPulse::
	encsetvar 7, 0
	encclearscreens ENC_TARGET_ALL_ALLIES, EncScript_Yveltal_DestructionPulse_Hazards
	encsetvar 7, 1
EncScript_Yveltal_DestructionPulse_Hazards:
	encclearhazards ENC_TARGET_ALL_ALLIES, EncScript_Yveltal_DestructionPulse_Check
	encsetvar 7, 1
EncScript_Yveltal_DestructionPulse_Check:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Yveltal_DestructionPulse_Done
	printstring STRINGID_ENCYVELTALPULSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_RESTORE_BG
	call EncScript_Yveltal_RaiseDestruction_1
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
EncScript_Yveltal_DestructionPulse_Done:
	return

// MARK TICK, called from the ledger while MarkTurns > 0: escalating bonus damage on the marked
// battler, branched (no multiply available) rather than computed.
EncScript_Yveltal_MarkTick:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Yveltal_MarkTick_1
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Yveltal_MarkTick_2
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Yveltal_MarkTick_1:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -3, ENC_AMOUNT_PERCENT
	return
EncScript_Yveltal_MarkTick_2:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
	return

// DOOMED. The mark's payoff at DOOMED_THRESHOLD (4) turns held - a devastating finisher that also
// resets the mark so the cycle can begin again.
EncScript_Yveltal_Doomed:
	printstring STRINGID_ENCYVELTALDOOMED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	encsetvar 2, 0     @ MarkTurns
	call EncScript_Yveltal_RaiseDestruction_1
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
	return

// OBLIVION WING resolves: damage scaled to Destruction's own band, a heal for Yveltal, a further
// Destruction gain, and a fresh mark on the player - all four pieces of the phase's thesis in one
// beat.
EncScript_Yveltal_ResolveOblivionWing:
	printstring STRINGID_ENCYVELTALWINGHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Yveltal_ResolveOblivionWing_15
	encjumpifvar CMP_LESS_THAN, 1, 7, EncScript_Yveltal_ResolveOblivionWing_22
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Yveltal_ResolveOblivionWing_Heal
EncScript_Yveltal_ResolveOblivionWing_22:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Yveltal_ResolveOblivionWing_Heal
EncScript_Yveltal_ResolveOblivionWing_15:
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
EncScript_Yveltal_ResolveOblivionWing_Heal:
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	call EncScript_Yveltal_RaiseDestruction_2
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
	call EncScript_Yveltal_MarkBattler
	return

// DETONATION. Reached either by The Destruction of All resolving in full or by the player
// interrupting it - the exact mirror of the Properties: block plus everything the fight raised, so
// nothing survives into the catch window (guideline 10).
EncScript_Yveltal_Detonate:
	encsetvar 0, 3     @ Phase: Aftermath
	encsetvar 1, 0     @ Destruction
	printstring STRINGID_ENCYVELTALFADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Yveltal_Detonate_NoStats
EncScript_Yveltal_Detonate_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCYVELTALWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger entry points ---

// OnBattleStart. _ApplyGuard/_ApplyOffense run on turn 0 so the guard already reflects the table
// rather than the Properties: seed. LastReductionTier starts at 9 so the opening band line is a real
// announcement. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Yveltal_Intro::
	printstring STRINGID_ENCYVELTALINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCYVELTALINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 4, 3     @ Charge: first Life Drain in 3 turns
	encsetvar 6, 9     @ LastReductionTier: nothing announced yet
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
	return

// Top of every turn. TurnGuard is set immediately so no branch out can leave it re-armed.
// HandledMoveEvent is cleared here, not at OnTurnEnd, so it covers the whole turn including any move
// that resolves before this checkpoint's own OnTurnEnd.
EncScript_Yveltal_TurnOpen::
	encsetvar 5, 1     @ TurnGuard
	encsetvar 8, 0     @ HandledMoveEvent
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Yveltal_TurnOpen_Wing   @ Phase 0: no Oblivion yet
	encsethealblock ENC_TARGET_PLAYER_LEFT, 5
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Yveltal_TurnOpen_Wing
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_SET   @ PhaseMark: this turn's opening HP%
EncScript_Yveltal_TurnOpen_Wing:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Yveltal_TurnOpen_Charge
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Yveltal_TurnOpen_Charge
	encsubvar 3, 1     @ WingChargeTurns - resolves at TurnClose when this reads 1
	flushtextbox
	printstring STRINGID_ENCYVELTALWINGWARN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Yveltal_TurnOpen_Charge:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Yveltal_TurnOpen_ChargeP2
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Yveltal_TurnOpen_Cue
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Yveltal_TurnOpen_Cue
	encsubvar 4, 1     @ Charge: Phase 0's Life Drain cycle (silent countdown)
	goto EncScript_Yveltal_TurnOpen_Cue
EncScript_Yveltal_TurnOpen_ChargeP2:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Yveltal_TurnOpen_Cue
	encsubvar 4, 1     @ Charge: The Destruction of All - resolves at TurnClose when this reads 1
	flushtextbox
	printstring STRINGID_ENCYVELTALINCARNATEWARN
	waitmessage B_WAIT_TIME_SHORT
// The recurring cue (guideline 14): a short line every turn Destruction is above baseline.
EncScript_Yveltal_TurnOpen_Cue:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Yveltal_TurnOpen_Done
	flushtextbox
	printstring STRINGID_ENCYVELTALCUE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Yveltal_TurnOpen_Done:
	return

// Oblivion Wing's telegraph. Three turns is two full turns of play after the announcement.
EncScript_Yveltal_BeginOblivionWing::
	flushtextbox
	printstring STRINGID_ENCYVELTALWINGBEGIN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	encsetvar 3, 3     @ WingChargeTurns
	return

// The interrupt. Cumulative damage to Yveltal since Destruction Incarnate began is measured through
// Scratch, copied from the persistent PhaseMark so the baseline survives being read.
EncScript_Yveltal_InterruptCheck::
	encsetvar 8, 1     @ HandledMoveEvent
	enccopyvar 7, 9    @ Scratch = PhaseMark
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_DAMAGE, EncScript_Yveltal_InterruptCheck_Done
	encjumpifvar CMP_LESS_THAN, 7, 5, EncScript_Yveltal_InterruptCheck_Done
	encsetvar 4, 0     @ Charge: cut short
	printstring STRINGID_ENCYVELTALINTERRUPTED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	call EncScript_Yveltal_Detonate
EncScript_Yveltal_InterruptCheck_Done:
	return

// Yveltal landing a Dark/Flying hit.
EncScript_Yveltal_DarkFlyingHit::
	encsetvar 8, 1     @ HandledMoveEvent
	call EncScript_Yveltal_RaiseDestruction_1
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
	return

// THE LEDGER. Resolves the Life Drain cycle, the mark tick/DOOMED payoff, the Phase 1 passive gain,
// and whichever charge just ran out - in that order, so a Destruction change earlier in the script is
// what the charge resolution's own damage band reads.
EncScript_Yveltal_TurnClose::
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Yveltal_TurnClose_Mark
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Yveltal_TurnClose_Mark
	call EncScript_Yveltal_LifeDrain
	encsetvar 4, 3     @ Charge: cycle restarts
EncScript_Yveltal_TurnClose_Mark:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Yveltal_TurnClose_Passive
	encaddvar 2, 1     @ MarkTurns
	encjumpifvar CMP_LESS_THAN, 2, 4, EncScript_Yveltal_TurnClose_MarkTick
	call EncScript_Yveltal_Doomed
	goto EncScript_Yveltal_TurnClose_Passive
EncScript_Yveltal_TurnClose_MarkTick:
	call EncScript_Yveltal_MarkTick
EncScript_Yveltal_TurnClose_Passive:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Yveltal_TurnClose_Wing
	encsetvar 7, 0
	enccopyvar 7, 9    @ Scratch = PhaseMark (player's HP% at turn open)
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 7, ENC_SNAP_DAMAGE, EncScript_Yveltal_TurnClose_Wing
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Yveltal_TurnClose_Wing
	call EncScript_Yveltal_RaiseDestruction_1
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
EncScript_Yveltal_TurnClose_Wing:
	encjumpifvar CMP_NOT_EQUAL, 3, 1, EncScript_Yveltal_TurnClose_Incarnate
	call EncScript_Yveltal_ResolveOblivionWing
	encsetvar 3, 0     @ WingChargeTurns
	goto EncScript_Yveltal_TurnClose_Done
EncScript_Yveltal_TurnClose_Incarnate:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Yveltal_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Yveltal_TurnClose_Done
	printstring STRINGID_ENCYVELTALINCARNATEHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	encsetvar 4, 0     @ Charge
	call EncScript_Yveltal_Detonate
EncScript_Yveltal_TurnClose_Done:
	encsetvar 5, 0     @ TurnGuard
	return

// --- Phase transitions ---

// 50% - WINGS OF DESTRUCTION. Oblivion re-arms every turn from here on; the passive gain and Oblivion
// Wing's own telegraph both come alive through Phase == 1 alone, no separate flag needed.
EncScript_Yveltal_WingsOfDestruction::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCYVELTALPHASE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
	return

// 20% - DESTRUCTION INCARNATE. Destruction is forced to 10 and nothing in the fight can lower it
// again, so no re-locking trigger is needed. PhaseMark is set once here as the interrupt's baseline
// and held for the whole charge.
EncScript_Yveltal_DestructionIncarnate::
	encsetvar 0, 2     @ Phase
	encsetvar 1, 10    @ Destruction: forced and permanent
	printstring STRINGID_ENCYVELTALPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Yveltal_ApplyGuard
	call EncScript_Yveltal_ApplyOffense
	call EncScript_Yveltal_Band
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_SET   @ PhaseMark: the interrupt's baseline
	encsetvar 4, 3     @ Charge: The Destruction of All begins charging
	printstring STRINGID_ENCYVELTALINCARNATEBEGIN
	waitmessage B_WAIT_TIME_LONG
	return
