// Wo-Chien, "The Ruinous Tablet Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_WoChien_Intro:
// 0 Decay (0-6, never falls), 1 FieldStage (0 Normal / 1 Withered / 2 Rotting / 3 Dead, recomputed
// after every Decay write), 2 TurnGuard (once-per-turn scaffold), 3 DecayGuard (shared "one
// meaningful OnMoveEnd/OnFaint event already resolved this turn" latch), 4 VineActive, 5 RuinMarked,
// 6 Phase (0 Spreading Rot / 1 Rotting Forest / 2 Ruinous Leaves), 7 ChannelActive, 8 SnapVar (shared
// encsnapshothp scratch for the vine-break and Life Drain interrupt threshold checks), 9
// LeavesActive, 10 LeavesTicks, 11 PlayerMark (player's HP% baseline for the turn), 12
// PlayerScratch (the consumable copy ENC_SNAP_DAMAGE/RECOVERY writes into), 13 BossHitHandled
// (per-turn latch shared by the vine-break and channel-interrupt checks).
//
// DECAY NEVER FALLS. That is what makes it the anti-stall meter: turns passing, Wo-Chien's Grass
// move or drain landing, the player losing HP, the player healing, and any faint all push it forward
// and nothing pushes it back. FieldStage is purely a function of Decay, so EncWoChien_RecomputeField-
// Stage is the only writer and always writes absolutely, comparing against the stored value first so
// the crossing announcement prints exactly once per band (no separate "last announced" variable
// needed, since the stage itself never regresses).
//
// DecayGuard IS DELIBERATELY OVERLOADED. Every OnMoveEnd/OnFaint trigger that reacts to Event.* data
// must close its own latch or it re-selects until the runaway-script guard trips (SPEC's "an event-
// reacting trigger must disable itself"). Rather than spend a variable per source, every one of those
// triggers shares DecayGuard: whichever fires first this turn - a Decay-from-event tick, a Ruin Mark
// proc, a faint - sets it, and every other event source stands down for the rest of the turn. That is
// a real simplification (only one such event resolves per turn instead of every source stacking
// independently) accepted to stay inside 16 variables; it reads as "the fight noticed one significant
// thing per turn," which is not far from the design's own framing of Decay as a slow, turn-scale
// pressure meter rather than a per-hit counter. (Vine-break/channel-interrupt use their own latch,
// BossHitHandled, below - DecayGuard being absent from THEIR Conditions was the runaway-chain bug.)
//
// SnapVar IS SHARED TOO. Both the vine-break check and the Life Drain interrupt check consume it via
// ENC_SNAP_DAMAGE against the baseline EncScript_WoChien_TurnOpen lays down each turn. If both were
// somehow live on the same turn, whichever trigger's Priority puts it first consumes the value and
// the other reads a starved (already-zeroed) scratch. Accepted: only one player action lands per
// turn in a singles battle, so this only matters if a future revision adds a second hit source.
//
// PLAYER HP DIRECTION IS DECIDED IN SCRIPT, NOT IN Conditions. A condition can only compare an
// operand against a literal - "Event.NewValue < Event.OldValue" compares two operands and is not
// legal (it silently emits a bare, undeclared "Event" into the generated C, a build-time-only
// failure). EncScript_WoChien_DecayFromPlayerHp instead tries ENC_SNAP_DAMAGE, then ENC_SNAP_RECOVERY,
// against PlayerMark/PlayerScratch - the same baseline/working-copy split Reshiram uses for
// BossMark/Scratch, needed because ENC_SNAP_DAMAGE/RECOVERY consume the var they read.
//
// BossHitHandled CLOSES THE VINE-BREAK/CHANNEL-INTERRUPT LATCH. Both of those triggers key on
// Event.Battler == B_POSITION_OPPONENT_LEFT, which stays true for the whole OnMoveEnd re-evaluation
// loop; a miss used to only set the shared DecayGuard, which isn't part of either trigger's own
// Conditions, so the trigger kept re-selecting until the 8-scripts-per-checkpoint cap tripped
// ("runaway trigger chain"). BossHitHandled is reset every turn in TurnOpen and set on every path
// through both scripts - break/interrupt or miss alike - so the checkpoint always closes.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so every script here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of FieldStage. Reads Decay, and since Decay only ever rises, a straight
// high-to-low ladder is enough - no band ever needs to un-announce itself.
EncScript_WoChien_RecomputeFieldStage:
	encjumpifvar CMP_GREATER_THAN, 0, 5, EncScript_WoChien_RecomputeFieldStage_Dead
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_WoChien_RecomputeFieldStage_Rotting
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_WoChien_RecomputeFieldStage_Withered
	return
EncScript_WoChien_RecomputeFieldStage_Withered:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_WoChien_RecomputeFieldStage_Done
	encsetvar 1, 1
	printstring STRINGID_ENCWOCHIENFIELDWITHERS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_WoChien_RecomputeFieldStage_Rotting:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_WoChien_RecomputeFieldStage_Done
	encsetvar 1, 2
	printstring STRINGID_ENCWOCHIENFIELDROTS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_WoChien_RecomputeFieldStage_Dead:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_WoChien_RecomputeFieldStage_Done
	encsetvar 1, 3
	printstring STRINGID_ENCWOCHIENFIELDDIES
	waitmessage B_WAIT_TIME_LONG
EncScript_WoChien_RecomputeFieldStage_Done:
	return

// Decay +1, capped at 6, then recompute. The common case - turn passing, a single event tick, an
// unmarked faint.
EncScript_WoChien_DecayAdd1:
	encjumpifvar CMP_EQUAL, 0, 6, EncScript_WoChien_DecayAdd1_Recompute
	encaddvar 0, 1
EncScript_WoChien_DecayAdd1_Recompute:
	call EncScript_WoChien_RecomputeFieldStage
	return

// Decay +3, capped at 6. Only the marked-faint spike uses this.
EncScript_WoChien_DecayAdd3:
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_WoChien_DecayAdd3_Cap
	encaddvar 0, 3
	goto EncScript_WoChien_DecayAdd3_Recompute
EncScript_WoChien_DecayAdd3_Cap:
	encsetvar 0, 6
EncScript_WoChien_DecayAdd3_Recompute:
	call EncScript_WoChien_RecomputeFieldStage
	return

// Ruinous Leaves' escalating chip: 5/7/9/11/12, one step per turn it has been live, capped at 12
// (guideline 7's single-turn threshold ceiling) well under the 16% real-hit baseline since this is a
// passive field tick, not a landed hit.
EncScript_WoChien_LeavesChip:
	printstring STRINGID_ENCWOCHIENLEAVESTICK
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_GREATER_THAN, 10, 3, EncScript_WoChien_LeavesChip_Max
	encaddvar 10, 1
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_WoChien_LeavesChip_T1
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_WoChien_LeavesChip_T2
	encjumpifvar CMP_EQUAL, 10, 3, EncScript_WoChien_LeavesChip_T3
	encchangehp ENC_TARGET_PLAYER_LEFT, -11, ENC_AMOUNT_PERCENT
	return
EncScript_WoChien_LeavesChip_T1:
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
	return
EncScript_WoChien_LeavesChip_T2:
	encchangehp ENC_TARGET_PLAYER_LEFT, -7, ENC_AMOUNT_PERCENT
	return
EncScript_WoChien_LeavesChip_T3:
	encchangehp ENC_TARGET_PLAYER_LEFT, -9, ENC_AMOUNT_PERCENT
	return
EncScript_WoChien_LeavesChip_Max:
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	return

// --- Trigger entry points ---

// OnBattleStart. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_WoChien_Intro::
	printstring STRINGID_ENCWOCHIENINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCWOCHIENINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SWAMP
	return

// Top of every turn. Leads with flushtextbox - the BG0 gotcha. TurnGuard is set immediately so no
// branch out can leave it re-armed. SnapVar and PlayerMark are both re-baselined every turn for the
// checks that read them later this same turn.
EncScript_WoChien_TurnOpen::
	flushtextbox
	encsetvar 2, 1     @ TurnGuard
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 11, ENC_SNAP_SET
	encsetvar 13, 0    @ BossHitHandled
	call EncScript_WoChien_DecayAdd1
	return

// Vines attach. Self-disabling through VineActive.
EncScript_WoChien_VineAttach::
	printstring STRINGID_ENCWOCHIENVINESWRAP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SWAMP
	encsetvar 4, 1     @ VineActive
	return

// Ruin Mark settles on whoever is standing there. Self-disabling through RuinMarked, which only
// clears at a marked faint.
EncScript_WoChien_RuinMarkSet::
	printstring STRINGID_ENCWOCHIENRUINMARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encsetvar 5, 1     @ RuinMarked
	encsethealblock ENC_TARGET_PLAYER_LEFT, 5
	return

// Life Drain telegraph, Once/OnEnter on the Phase 1 edge - no extra guard needed.
EncScript_WoChien_ChannelStart::
	flushtextbox
	printstring STRINGID_ENCWOCHIENCHANNELSTART
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encsetvar 7, 1     @ ChannelActive
	return

// The Decay-from-event sources. Each closes the shared DecayGuard latch on every path, since none of
// them can leave without doing so (the "event-reacting trigger must disable itself" rule).
EncScript_WoChien_DecayFromGrass::
	call EncScript_WoChien_DecayAdd1
	encsetvar 3, 1     @ DecayGuard
	return

EncScript_WoChien_DecayFromDrain::
	call EncScript_WoChien_DecayAdd1
	encsetvar 3, 1     @ DecayGuard
	return

// The player's HP moved - loss or heal - at OnMoveEnd. Tries ENC_SNAP_DAMAGE first, then
// ENC_SNAP_RECOVERY, each against a fresh copy of PlayerMark since the mode consumes what it reads;
// either a genuine miss (neither writes a nonzero value) or either branch closes DecayGuard, so this
// dispatch never re-selects.
EncScript_WoChien_DecayFromPlayerHp::
	enccopyvar 12, 11     @ PlayerScratch = PlayerMark
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 12, ENC_SNAP_DAMAGE, EncScript_WoChien_DecayFromPlayerHp_TryHeal
	call EncScript_WoChien_DecayAdd1
	goto EncScript_WoChien_DecayFromPlayerHp_Close
EncScript_WoChien_DecayFromPlayerHp_TryHeal:
	enccopyvar 12, 11     @ PlayerScratch = PlayerMark, fresh copy
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 12, ENC_SNAP_RECOVERY, EncScript_WoChien_DecayFromPlayerHp_Close
	call EncScript_WoChien_DecayAdd1
EncScript_WoChien_DecayFromPlayerHp_Close:
	encsetvar 3, 1     @ DecayGuard
	return

// Ruin Mark's extra damage on every one of Wo-Chien's landed hits against the marked mon. Closes
// DecayGuard too, which also lets this same hit's player-HP Decay tick stand down.
EncScript_WoChien_RuinMarkExtraDamage::
	printstring STRINGID_ENCWOCHIENRUINMARKSTING
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encsetvar 3, 1     @ DecayGuard
	return

// Vine break: a real haymaker (>=20% of Wo-Chien's max HP since this turn's baseline). Closes
// VineActive on a break, and BossHitHandled on every path (break or miss) - BossHitHandled is what
// is actually in this trigger's own Conditions, so a miss can't leave it re-selecting. The spore
// burst costs the breaker a flat HP tax rather than a status opcode with a stale-battler risk outside
// a move context (Stage 4 substitution for the plan's scripted poison).
EncScript_WoChien_VineBreak::
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_DAMAGE, EncScript_WoChien_VineBreak_Miss
	encjumpifvar CMP_LESS_THAN, 8, 20, EncScript_WoChien_VineBreak_Miss
	printstring STRINGID_ENCWOCHIENVINESBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetvar 4, 0     @ VineActive
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
	encsetvar 13, 1    @ BossHitHandled
	return
EncScript_WoChien_VineBreak_Miss:
	encsetvar 13, 1    @ BossHitHandled
	return

// Life Drain interrupt: the same 20% haymaker breaks the channel. Same guard shape as the vine break
// - BossHitHandled closes it on every path, break or miss.
EncScript_WoChien_ChannelInterrupt::
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_DAMAGE, EncScript_WoChien_ChannelInterrupt_Miss
	encjumpifvar CMP_LESS_THAN, 8, 20, EncScript_WoChien_ChannelInterrupt_Miss
	printstring STRINGID_ENCWOCHIENCHANNELINTERRUPT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 7, 0     @ ChannelActive
	encsetvar 13, 1    @ BossHitHandled
	return
EncScript_WoChien_ChannelInterrupt_Miss:
	encsetvar 13, 1    @ BossHitHandled
	return

// Faint while Ruin Marked: the big spike. Closes RuinMarked (its own latch, so the mark is free to
// resettle on whoever replaces the fainted mon) and the shared DecayGuard.
EncScript_WoChien_FaintMarked::
	printstring STRINGID_ENCWOCHIENRUINMARKFAINT
	waitmessage B_WAIT_TIME_LONG
	encsetvar 5, 0     @ RuinMarked
	call EncScript_WoChien_DecayAdd3
	encsetvar 3, 1     @ DecayGuard
	return

// Any other faint: the small tick. Closes the shared DecayGuard.
EncScript_WoChien_FaintAny::
	call EncScript_WoChien_DecayAdd1
	encsetvar 3, 1     @ DecayGuard
	return

// 50% HP - Rotting Forest. Decay is forced into the Rotting band immediately (if it isn't there
// already), which fires the field-stage announcement itself if that is a new crossing.
EncScript_WoChien_Phase2Transition::
	encsetvar 6, 1     @ Phase
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_WoChien_Phase2Transition_Skip
	encsetvar 0, 4
	call EncScript_WoChien_RecomputeFieldStage
EncScript_WoChien_Phase2Transition_Skip:
	printstring STRINGID_ENCWOCHIENFOREST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// 15% HP - Ruinous Leaves, the catch window (guideline 10). Tears down vines and Ruin Mark - neither
// mechanic is meant to keep running once the field is Dead and the chip damage takes over - and
// clears Wo-Chien's boss-only guards so the fight actually ends. Field-wide Heal Block and the chip
// tick both keep running: this is a live, pressuring window, not a frozen one.
EncScript_WoChien_Phase3Transition::
	encsetvar 6, 2     @ Phase
	encsetvar 9, 1     @ LeavesActive
	encsetvar 0, 6     @ Decay: forced to Dead
	call EncScript_WoChien_RecomputeFieldStage
	encsetvar 4, 0     @ VineActive
	encsetvar 5, 0     @ RuinMarked
	encsetvar 7, 0     @ ChannelActive
	printstring STRINGID_ENCWOCHIENLEAVESFALL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	return

// THE LEDGER. Everything that needs to happen once, every turn, for as long as its own state holds -
// folded into one TurnGuard-gated script instead of separate standalone triggers, since none of these
// conditions (FieldStage/RuinMarked/VineActive/Phase/ChannelActive) flips on its own and a bare
// standalone trigger on any of them would re-select every pass within this same OnTurnEnd dispatch.
// Self-disables by clearing DecayGuard and TurnGuard.
EncScript_WoChien_TurnClose::
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_WoChien_TurnClose_RuinHeal
	encsethealblock ENC_TARGET_ALL_ALLIES, 5
	printstring STRINGID_ENCWOCHIENHEALBLOCKED
	waitmessage B_WAIT_TIME_SHORT
EncScript_WoChien_TurnClose_RuinHeal:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_WoChien_TurnClose_Vines
	encsethealblock ENC_TARGET_PLAYER_LEFT, 5
EncScript_WoChien_TurnClose_Vines:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_WoChien_TurnClose_SelfHeal
	printstring STRINGID_ENCWOCHIENVINESDRAIN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
EncScript_WoChien_TurnClose_SelfHeal:
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_WoChien_TurnClose_Channel
	printstring STRINGID_ENCWOCHIENSELFHEAL
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
EncScript_WoChien_TurnClose_Channel:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_WoChien_TurnClose_Leaves
	printstring STRINGID_ENCWOCHIENCHANNELDRAIN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	encsetvar 7, 0     @ ChannelActive
EncScript_WoChien_TurnClose_Leaves:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_WoChien_TurnClose_Done
	call EncScript_WoChien_LeavesChip
EncScript_WoChien_TurnClose_Done:
	encsetvar 3, 0     @ DecayGuard
	encsetvar 2, 0     @ TurnGuard
	return
