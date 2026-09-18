// Chi-Yu, "The Ruinous Bead Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_ChiYu_Intro:
// 0 Flame (0-6), 1 BeadCount (0-3), 2 BossDmgSnap (HP% snapshot at turn open, then the turn's
// damage-taken once the ledger reads it), 3 GreedyState (0 idle / 1 windup turn 1 / 2 windup
// turn 2), 4 GreedyCooldown, 5 IgniteChain, 6 Phase (0 / 1 Wildfire / 2 Consuming Ruin),
// 7 FlameGainGuard, 8 FaintGuard, 9 TurnGuard, 10 FirestormWindup, 11 RuinousScratch,
// 12 LastFlameBand (0 Warm / 1 Scorched / 2 Inferno / 3 Calm - the last announced band),
// 13 RuinousArmed, 14 CueTick.
//
// FLAME IS THE ONE METER. Every source that feeds it (a Fire move landing, a faint it causes, a
// Greedy Flame payoff, an Ignite Chain payoff) runs through EncScript_ChiYu_FlameAdded, which
// clamps at 6 and re-announces the band on change - guideline 14, in both directions.
//
// BossDmgSnap IS SHARED, ON PURPOSE. EncScript_ChiYu_Ledger reads it once per turn and both the
// bead-break check and the Greedy-interrupt check consume that same number, so passive stalling
// can't fake either payoff and neither check can be independently tuned within the same turn.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so every script here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// Clamps Flame at its ceiling of 6 after a caller has already added to it, then re-announces the
// band on change. Every Flame-adding script tail-calls this instead of touching var 0 directly.
EncScript_ChiYu_FlameAdded:
	encjumpifvar CMP_LESS_THAN, 0, 7, EncScript_ChiYu_AnnounceFlame
	encsetvar 0, 6
	goto EncScript_ChiYu_AnnounceFlame

// THE BAND CALLOUT (guideline 14). LastFlameBand holds the last ANNOUNCED band; each arm compares
// it against its own literal and stays quiet if that is what was last said, so the line prints
// only on a difference - in BOTH directions.
EncScript_ChiYu_AnnounceFlame:
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_ChiYu_AnnounceFlame_Calm
	encjumpifvar CMP_LESS_THAN, 0, 4, EncScript_ChiYu_AnnounceFlame_Warm
	encjumpifvar CMP_LESS_THAN, 0, 6, EncScript_ChiYu_AnnounceFlame_Scorched
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_ChiYu_AnnounceFlame_Done
	encsetvar 12, 2
	printstring STRINGID_ENCCHIYUFLAMEINFERNO
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_ChiYu_AnnounceFlame_Scorched:
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_ChiYu_AnnounceFlame_Done
	encsetvar 12, 1
	printstring STRINGID_ENCCHIYUFLAMESCORCHED
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_ChiYu_AnnounceFlame_Warm:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_ChiYu_AnnounceFlame_Done
	encsetvar 12, 0
	printstring STRINGID_ENCCHIYUFLAMEWARM
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_ChiYu_AnnounceFlame_Calm:
	encjumpifvar CMP_EQUAL, 12, 3, EncScript_ChiYu_AnnounceFlame_Done
	encsetvar 12, 3
	printstring STRINGID_ENCCHIYUFLAMECALMS
	waitmessage B_WAIT_TIME_LONG
EncScript_ChiYu_AnnounceFlame_Done:
	return

// A Bead breaks: one fewer banked, one payload back at the player. Caller checks BeadCount > 0.
EncScript_ChiYu_BreakBead:
	encsubvar 1, 1
	printstring STRINGID_ENCCHIYUBEADBREAKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return

// --- Trigger entry points ---

// OnBattleStart. LastFlameBand starts at Calm directly, matching Flame's own starting value, so
// no spurious "the flames are calm" line fires over the intro text. GreedyCooldown starts at 0 -
// a windup can open as early as turn 1.
EncScript_ChiYu_Intro::
	printstring STRINGID_ENCCHIYUOPENING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
	encsetvar 12, 3    @ LastFlameBand
	encsetvar 4, 0     @ GreedyCooldown
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch out can leave it re-armed.
EncScript_ChiYu_TurnOpen::
	flushtextbox
	encsetvar 9, 1     @ TurnGuard
	encsnapshothp ENC_TARGET_BOSS, 2, ENC_SNAP_SET    @ BossDmgSnap = HP% at turn open
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_ChiYu_TurnOpen_Bead
	encsubvar 4, 1     @ GreedyCooldown
EncScript_ChiYu_TurnOpen_Bead:
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_ChiYu_TurnOpen_Passive   @ Wildfire only
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_ChiYu_TurnOpen_Passive       @ BeadCount capped
	encaddvar 1, 1     @ BeadCount auto-spawn
EncScript_ChiYu_TurnOpen_Passive:
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_ChiYu_TurnOpen_HealBlock @ Wildfire passive gain
	encaddvar 0, 1
	call EncScript_ChiYu_FlameAdded
EncScript_ChiYu_TurnOpen_HealBlock:
	encjumpifvar CMP_LESS_THAN, 0, 4, EncScript_ChiYu_TurnOpen_Cue       @ Scorched+ re-arms Heal Block
	encsethealblock ENC_TARGET_ALL_FOES, 2
EncScript_ChiYu_TurnOpen_Cue:
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_ChiYu_TurnOpen_Done      @ recurring cue, Warm+ only
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_ChiYu_TurnOpen_CueA
	encsetvar 14, 0
	goto EncScript_ChiYu_TurnOpen_Done
EncScript_ChiYu_TurnOpen_CueA:
	encsetvar 14, 1
	printstring STRINGID_ENCCHIYUHEATPRESSES
	waitmessage B_WAIT_TIME_SHORT
EncScript_ChiYu_TurnOpen_Done:
	return

// GREEDY FLAME opens. Two turns of telegraph before the payoff; a hard hit during either breaks
// it at the ledger.
EncScript_ChiYu_GreedyStart::
	encsetvar 3, 1     @ GreedyState: windup turn 1
	printstring STRINGID_ENCCHIYUGREEDYWINDUP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

// Silent - the windup's second turn plays out and the ledger resolves it at OnTurnEnd.
EncScript_ChiYu_GreedyAdvance::
	encsetvar 3, 2
	return

// FIRESTORM telegraphs one turn ahead of the release.
EncScript_ChiYu_FirestormArm::
	encsetvar 10, 1    @ FirestormWindup
	printstring STRINGID_ENCCHIYUFIRESTORMTELEGRAPH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	return

// FIRESTORM releases. Leads with dialogue (the OnTurnStart animate-before-menu gotcha).
EncScript_ChiYu_FirestormResolve::
	encsetvar 10, 0    @ close the latch first
	printstring STRINGID_ENCCHIYUFIRESTORMFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	encsetvar 0, 0     @ Flame resets
	call EncScript_ChiYu_AnnounceFlame
	return

// Flame gain: the boss's own Fire move landed. Once per turn - the guard is this script's first
// line, closed on the only path through it.
EncScript_ChiYu_FlameGainMove::
	encsetvar 7, 1     @ FlameGainGuard
	encaddvar 0, 1
	call EncScript_ChiYu_FlameAdded
	return

// Flame gain: the boss caused a faint.
EncScript_ChiYu_FlameGainFaint::
	encsetvar 8, 1     @ FaintGuard
	encaddvar 0, 1
	call EncScript_ChiYu_FlameAdded
	return

// THE LEDGER. Reads the turn's damage to the boss once and resolves bead-break, the Greedy
// windup, and Ignite Chain from that single number. Closes every once-per-turn guard on every
// path out, including the quiet one.
EncScript_ChiYu_Ledger::
	encsnapshothp ENC_TARGET_BOSS, 2, ENC_SNAP_DAMAGE, EncScript_ChiYu_Ledger_NoDamage
	goto EncScript_ChiYu_Ledger_Measured
EncScript_ChiYu_Ledger_NoDamage:
	encsetvar 2, 0
EncScript_ChiYu_Ledger_Measured:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_ChiYu_Ledger_Greedy
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_ChiYu_Ledger_BeadBreak    @ >= 8%
	goto EncScript_ChiYu_Ledger_Greedy
EncScript_ChiYu_Ledger_BeadBreak:
	call EncScript_ChiYu_BreakBead
EncScript_ChiYu_Ledger_Greedy:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_ChiYu_Ledger_Ignite
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_ChiYu_Ledger_GreedyInterrupted   @ >= 10%
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_ChiYu_Ledger_GreedyPayoff
	goto EncScript_ChiYu_Ledger_Ignite
EncScript_ChiYu_Ledger_GreedyInterrupted:
	printstring STRINGID_ENCCHIYUGREEDYINTERRUPTED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetvar 3, 0     @ GreedyState
	encsetvar 4, 4     @ GreedyCooldown
	goto EncScript_ChiYu_Ledger_Ignite
EncScript_ChiYu_Ledger_GreedyPayoff:
	printstring STRINGID_ENCCHIYUGREEDYPAYOFF
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetvar 3, 0     @ GreedyState
	encsetvar 4, 4     @ GreedyCooldown
	encaddvar 0, 2
	call EncScript_ChiYu_FlameAdded
EncScript_ChiYu_Ledger_Ignite:
	jumpifstatus BS_PLAYER1, STATUS1_BURN, EncScript_ChiYu_Ledger_IgniteBurned
	encsetvar 5, 0     @ IgniteChain resets - cured or never burned
	goto EncScript_ChiYu_Ledger_Done
EncScript_ChiYu_Ledger_IgniteBurned:
	encaddvar 5, 1
	encjumpifvar CMP_LESS_THAN, 5, 3, EncScript_ChiYu_Ledger_Done
	printstring STRINGID_ENCCHIYUFLAMESSPREAD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 5, 0     @ IgniteChain
	encaddvar 0, 3
	call EncScript_ChiYu_FlameAdded
EncScript_ChiYu_Ledger_Done:
	encsetvar 7, 0     @ FlameGainGuard
	encsetvar 8, 0     @ FaintGuard
	encsetvar 9, 0     @ TurnGuard
	return

// --- Phase transitions ---

// 50% - WILDFIRE.
EncScript_ChiYu_PhaseWildfire::
	encsetvar 6, 1     @ Phase
	printstring STRINGID_ENCCHIYUWILDFIRE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return

// 15% - CONSUMING RUIN. Arms Ruinous Pyre for a one-turn telegraph.
EncScript_ChiYu_FinalPhaseEnter::
	encsetvar 6, 2     @ Phase
	printstring STRINGID_ENCCHIYUFINALPHASE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 13, 1    @ RuinousArmed
	return

// RUINOUS PYRE releases. Bands Flame + the Beads it is about to devour into a damage tier, then
// resets everything to calm. Leads with dialogue (the OnTurnStart animate-before-menu gotcha).
EncScript_ChiYu_RuinousResolve::
	encsetvar 13, 0    @ close the latch first
	printstring STRINGID_ENCCHIYURUINOUSPYRE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
	enccopyvar 11, 0   @ RuinousScratch = Flame
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_ChiYu_RuinousResolve_Band
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_ChiYu_RuinousResolve_Add1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_ChiYu_RuinousResolve_Add2
	encaddvar 11, 3
	goto EncScript_ChiYu_RuinousResolve_Band
EncScript_ChiYu_RuinousResolve_Add2:
	encaddvar 11, 2
	goto EncScript_ChiYu_RuinousResolve_Band
EncScript_ChiYu_RuinousResolve_Add1:
	encaddvar 11, 1
EncScript_ChiYu_RuinousResolve_Band:
	encjumpifvar CMP_LESS_THAN, 11, 4, EncScript_ChiYu_RuinousResolve_18
	encjumpifvar CMP_LESS_THAN, 11, 7, EncScript_ChiYu_RuinousResolve_24
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	goto EncScript_ChiYu_RuinousResolve_Reset
EncScript_ChiYu_RuinousResolve_24:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -24, ENC_AMOUNT_PERCENT
	goto EncScript_ChiYu_RuinousResolve_Reset
EncScript_ChiYu_RuinousResolve_18:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
EncScript_ChiYu_RuinousResolve_Reset:
	encsetvar 1, 0     @ BeadCount devoured
	encsetvar 0, 0     @ Flame resets
	call EncScript_ChiYu_AnnounceFlame
	return

// 10% - Balls unlock. No teardown: Survive: True keeps the boss standing while the player closes
// the fight out for real.
EncScript_ChiYu_BallsUnlock::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	printstring STRINGID_ENCCHIYUNOWCATCH
	waitmessage B_WAIT_TIME_LONG
	return
