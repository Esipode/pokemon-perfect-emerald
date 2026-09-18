// Registeel, "The Adaptive Machine". Var indices, pinned by
// the always-true Conditions on EncScript_Registeel_Intro:
// 0 Phase (0 Analysis / 1 Optimization / 2 Perfect Configuration / 3 Weakened), 1 Load (adaptations
// currently held, 0-4 - written ONLY by encadapt/encpurgeadapt so it can never drift from the real
// board), 2 Stance (stance last APPLIED, the re-fire latch: 0 none yet / 1 Assault / 2 Balanced /
// 3 Fortress), 3 Decay (countdown to the oldest adaptation expiring), 4 MoveGuard (per-turn
// re-entry guard for the filing trigger), 5 StatusGuard (a status has spent its purge),
// 6 TurnGuard, 7 Result (encadapt's ENC_ADAPT_RESULT_* outcome), 8 Clock (Perfect Configuration
// countdown to Critical Overheat), 9 Cooling (post-overheat turns remaining), 10 Announced (Load
// tier last announced - the recurring-cue latch).
//
// Regirock is a wall you pick the right tool for; Registeel is a wall that becomes the WRONG tool's
// wall. The board lives in the engine (encadapt/encpurgeadapt, ENC_MAX_ADAPTATIONS), not in a var -
// Var(Load) is only ever written by those two commands, so the stance ladder and the recurring cue
// are always derived from the real board rather than from a script's bookkeeping of it.

// --- Shared subroutines (call/return) ---

// Sole owner of Registeel's Atk and SpAtk, and the fight's second axis: everything it puts into
// plating comes out of its weapons. An ABSOLUTE set, not a delta - encchangestat clamps at the
// stage bounds, so -12 then +N lands on DEFAULT_STAT_STAGE+N-6 from wherever the stage had drifted.
// Safe here precisely because Clear Body plus a moveset with no self-buff means this ladder is the
// only thing that ever writes those two stages.
// Phase 2 short-circuits to ASSAULT before Load is read at all: PERFECT CONFIGURATION is the one
// state where filling its board does not disarm it.
EncScript_Registeel_ApplyStance:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Registeel_Stance_Assault
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Registeel_Stance_Assault
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Registeel_Stance_Fortress
EncScript_Registeel_Stance_Balanced:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Registeel_Stance_Done
	encsetvar 2, 2    @ Stance: Balanced
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 7
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 7
	printstring STRINGID_ENCREGISTEELBALANCED
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_GEAR_UP
	return
EncScript_Registeel_Stance_Assault:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Registeel_Stance_Done
	encsetvar 2, 1    @ Stance: Assault
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 8
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 8
	printstring STRINGID_ENCREGISTEELASSAULT
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_HONE_CLAWS
	return
EncScript_Registeel_Stance_Fortress:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Registeel_Stance_Done
	encsetvar 2, 3    @ Stance: Fortress
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 5
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 5
	printstring STRINGID_ENCREGISTEELFORTRESS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_IRON_DEFENSE
EncScript_Registeel_Stance_Done:
	return

// The recurring board callout, latched on the tier last ANNOUNCED rather than on the raw Load, so
// it fires on every real change in either direction and never drones on an unchanged tier. An empty
// board says nothing here - it speaks through the ASSAULT stance line instead.
EncScript_Registeel_LoadCue:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Registeel_LoadCue_Two
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Registeel_LoadCue_One
	encsetvar 10, 0    @ Announced
	return
EncScript_Registeel_LoadCue_One:
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Registeel_LoadCue_Done
	encsetvar 10, 1
	printstring STRINGID_ENCREGISTEELLOADONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Registeel_LoadCue_Two:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Registeel_LoadCue_Done
	encsetvar 10, 2
	printstring STRINGID_ENCREGISTEELLOADTWO
	waitmessage B_WAIT_TIME_SHORT
EncScript_Registeel_LoadCue_Done:
	return

// Called by every path that can move the board - filing, the status purge, the decay tick, the
// overheat wipe and the catch window - so the stance and the cue can never fall out of step with
// the adaptations actually held.
EncScript_Registeel_BoardChanged:
	call EncScript_Registeel_ApplyStance
	call EncScript_Registeel_LoadCue
	return

// The decay clock's re-arm. One higher than the gap it produces, because the resolution fires at 1:
// 5 is an expiry every fourth turn, 7 every sixth. Optimization files more and holds more, and pays
// for it by letting go far more slowly.
EncScript_Registeel_DecayArm:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Registeel_DecayArm_Fast
	encsetvar 3, 7    @ Optimization: every sixth turn
	return
EncScript_Registeel_DecayArm_Fast:
	encsetvar 3, 5    @ Analysis: every fourth turn
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the decay clock needs seeding. The closing
// ApplyStance lands the opening ASSAULT stance at Load 0, which states the dial's existence on turn
// one - an empty board is the dangerous board, and the fight says so before the player has moved.
EncScript_Registeel_Intro::
	call EncScript_Registeel_DecayArm
	printstring STRINGID_ENCREGISTEELAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	call EncScript_Registeel_ApplyStance
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail so no branch out of this script can leave it re-armed.
// Every countdown ticks here and resolves at OnTurnEnd, one checkpoint away, so a tick can never
// eat a step off a fresh re-arm. Each is held at 1 for its resolution to consume.
// The cooling line is the standing "the mechanic is off" heartbeat - a player who missed the
// overheat still sees, every turn of the window, that it is still open.
EncScript_Registeel_TurnOpen::
	flushtextbox
	encsetvar 6, 1    @ TurnGuard
	encsetvar 4, 0    @ MoveGuard: the filing trigger is armed again
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Registeel_TurnOpen_Done   @ Weakened: inert
	encjumpifvar CMP_GREATER_THAN, 9, 1, EncScript_Registeel_TurnOpen_Cooling
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Registeel_TurnOpen_CoolLine
	call EncScript_Registeel_ClockTick
	call EncScript_Registeel_DecayTick
	return
EncScript_Registeel_TurnOpen_Cooling:
	encsubvar 9, 1    @ Cooling
EncScript_Registeel_TurnOpen_CoolLine:
	printstring STRINGID_ENCREGISTEELCOOLING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Registeel_TurnOpen_Done:
	return

// Perfect Configuration's countdown. Only runs in Phase 2 and only while Analysis is online, so the
// clock and the cooling window can never advance in the same turn.
EncScript_Registeel_ClockTick:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Registeel_ClockTick_Done
	encjumpifvar CMP_LESS_THAN, 8, 2, EncScript_Registeel_ClockTick_Done
	encsubvar 8, 1
EncScript_Registeel_ClockTick_Done:
	return

// The decay clock, which is what stops a two-type attacker ever being hard-locked: wait, and the
// board opens on its own. Waiting costs turns, and turns are what Registeel is spending on Iron
// Head. Phase 2 holds everything it has filed, so nothing ticks there.
EncScript_Registeel_DecayTick:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Registeel_DecayTick_Done
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Registeel_DecayTick_Done
	encsubvar 3, 1
EncScript_Registeel_DecayTick_Done:
	return

// TurnClose owns nothing but the per-turn guard resets. Every countdown lives at TurnOpen instead.
EncScript_Registeel_TurnClose::
	encsetvar 6, 0    @ TurnGuard
	encsetvar 4, 0    @ MoveGuard
	return

// ANALYSIS. MoveGuard is set first: an Event.* condition stays true for the whole dispatch and
// cannot be moved by the script, so an event-reacting trigger has to disable itself or the
// re-evaluation loop re-selects it until the per-checkpoint script cap trips. That also makes the
// filing exactly once per turn however many strikes a multi-hit move landed.
// The percent/slot pair is the phase's, picked here rather than stored, so widening the board is one
// changed literal. encadapt writes Var(Load) itself, and ignores anything that missed or had no
// effect - leaving Var(Result) on the sentinel set below, which the report cascade falls straight
// through. That is why the sentinel is written before the branch and not inside it.
EncScript_Registeel_Analyse::
	encsetvar 4, 1    @ MoveGuard
	encsetvar 7, 255  @ Result: no outcome unless encadapt writes one
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Registeel_Analyse_Perfect
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Registeel_Analyse_Optimized
	encadapt ENC_TARGET_BOSS, 40, 2, 1, 7
	goto EncScript_Registeel_Analyse_Report
EncScript_Registeel_Analyse_Optimized:
	encadapt ENC_TARGET_BOSS, 50, 3, 1, 7
	goto EncScript_Registeel_Analyse_Report
EncScript_Registeel_Analyse_Perfect:
	encadapt ENC_TARGET_BOSS, 55, 4, 1, 7
EncScript_Registeel_Analyse_Report:
	encjumpifvar CMP_EQUAL, 7, ENC_ADAPT_RESULT_EVICTED, EncScript_Registeel_Analyse_Evicted
	encjumpifvar CMP_EQUAL, 7, ENC_ADAPT_RESULT_FILED, EncScript_Registeel_Analyse_Filed
	encjumpifvar CMP_EQUAL, 7, ENC_ADAPT_RESULT_HARDENED, EncScript_Registeel_Analyse_Hardened
	return            @ nothing landed - the command left the board and both vars alone
EncScript_Registeel_Analyse_Filed:
	printstring STRINGID_ENCREGISTEELFILED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	goto EncScript_Registeel_Analyse_Done
EncScript_Registeel_Analyse_Hardened:
	printstring STRINGID_ENCREGISTEELHARDENED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	goto EncScript_Registeel_Analyse_Done
// The eviction line is the whole tutorial: in one sentence, and without a word of instruction, it
// tells the player the board has a SIZE and that they are the one deciding what falls off it.
EncScript_Registeel_Analyse_Evicted:
	printstring STRINGID_ENCREGISTEELEVICTED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
EncScript_Registeel_Analyse_Done:
	call EncScript_Registeel_BoardChanged
	return

// The decay clock resolved. The purge is unconditional and the LINE is what is conditional, so an
// empty board costs nothing and says nothing; the clock is re-armed up front so every path leaves
// it armed.
EncScript_Registeel_Decay::
	call EncScript_Registeel_DecayArm
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Registeel_Decay_Done
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_OLDEST, 1
	printstring STRINGID_ENCREGISTEELDEGRADED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	call EncScript_Registeel_BoardChanged
EncScript_Registeel_Decay_Done:
	return

// Force Reconfiguration. Feeding the machine contradictory readings costs it the NEWEST adaptation -
// the one the player just watched it file. StatusGuard is spent here and handed back only when the
// boss is clean again, so each purge costs a fresh application rather than a standing burn.
EncScript_Registeel_ForceReconfig::
	encsetvar 5, 1    @ StatusGuard
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_NEWEST, 1
	printstring STRINGID_ENCREGISTEELCONTRADICTION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	call EncScript_Registeel_BoardChanged
	return

// Silent - nothing has happened the player needs told. It only re-arms the lever.
EncScript_Registeel_StatusCleared::
	encsetvar 5, 0    @ StatusGuard
	return

// 55% - OPTIMIZATION. Three slots at 50%, and the decay slows from every fourth turn to every
// sixth. The first phase where a normal four-move Pokemon can be comprehensively answered, and the
// first where the FORTRESS rung of the dial is easy to reach.
// The decay clock is re-armed through the same routine so the new, slower cadence starts here
// rather than at the end of whatever countdown Analysis had running.
EncScript_Registeel_Optimization::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCREGISTEELOPTIMIZATION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	call EncScript_Registeel_DecayArm
	return

// 25% - PERFECT CONFIGURATION. Four slots at 55%, nothing decays, and the stance is pinned to
// ASSAULT regardless of Load: the only state in the fight where filling its board does not disarm
// it. Status is now the only thing that opens the board at all, which is what makes the lever worth
// having carried for twenty turns.
// Clock is fixed and takes no input the player can give it, so this is endurance rather than
// another meter to farm.
EncScript_Registeel_PerfectConfig::
	encsetvar 0, 2    @ Phase
	encsetvar 8, 6    @ Clock: five turns to CRITICAL OVERHEAT
	printstring STRINGID_ENCREGISTEELPERFECTCONFIG
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Registeel_ApplyStance
	return

// CRITICAL OVERHEAT. The board burns away, Analysis goes offline and the flat guard - which has not
// moved all fight - collapses for three turns. Every type the player owns is doing more than twice
// its usual damage, and there is nothing Registeel can do about it.
// Clock is zeroed so the countdown cannot resolve twice; SystemsOnline re-arms it on the way back.
EncScript_Registeel_CriticalOverheat::
	encsetvar 8, 0    @ Clock
	encsetvar 9, 4    @ Cooling: three full turns of window
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_ALL, 1
	encsetdamagereduction ENC_TARGET_BOSS, 60
	printstring STRINGID_ENCREGISTEELOVERHEAT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_OVERHEAT
	call EncScript_Registeel_BoardChanged
	return

// The window closes. The re-arm is a SAFETY VALVE, not a rhythm to farm: the 25%-to-10% band is
// narrow and the clock takes no input, so this exists only so a player who cannot close 15% of a
// health bar inside one window is not deadlocked against a boss holding a permanent full board.
EncScript_Registeel_SystemsOnline::
	encsetvar 9, 0    @ Cooling
	encsetvar 8, 6    @ Clock
	encsetdamagereduction ENC_TARGET_BOSS, 84
	printstring STRINGID_ENCREGISTEELONLINE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	call EncScript_Registeel_ApplyStance
	return

// 10% - the catch window, the mirror image of the Properties: block. The board is wiped and both
// stances' latches cleared, and Atk/SpAtk go back to neutral through the same -12 the ladder uses,
// so nothing the fight built can outlive it. Phase 3 makes TurnOpen return through its leading
// branch, so no clock can restart after this point.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Registeel_Weakened::
	encsetvar 0, 3    @ Phase
	encsetvar 2, 0    @ Stance
	encsetvar 3, 0    @ Decay
	encsetvar 8, 0    @ Clock
	encsetvar 9, 0    @ Cooling
	encsetvar 10, 0   @ Announced
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_ALL, 1
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCREGISTEELWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
