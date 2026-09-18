// Mewtwo, "The Perfect Weapon". Var indices, pinned by the
// always-true Conditions on EncScript_Mewtwo_Intro:
// 0 Phase (0 analysis / 1 adaptation / 2 perfect adaptation / 3 weakened), 1 Form (0 base / 1 Mega X
// / 2 Mega Y), 2 Lean (0-6), 3 Cooldown, 4 Instability (0-3), 5 Overload, 6 Stance (0-3),
// 7 LastGuard, 8 Chant, 9 TurnGuard, 10 MoveGuard, 11 BossGuard, 12 Power, 13 Scratch.
//
// Lean is a tug-of-war the player moves without meaning to: physical +1, special/status -1. When
// Cooldown expires Mewtwo MIRRORS the winning side - Mega X at Lean >= 5, Mega Y at Lean <= 1 - and
// the new species' stat spread does the countering with no extra scripting. Every form change and
// every shrugged sleep/freeze costs Instability, and Instability drives the guard DOWNWARD
// (ApplyGuard). At Instability 3 it Overloads: back to base Mewtwo, guard 40 for two full turns.
// So the only way to open a window is to keep changing what you do - which is also the counterplay
// to the mirroring. Stance (Force in X, Focus in Y) punishes every third landed hit and resets on
// every form change. Phase 2 makes adaptation compulsory, so the adaptation is what kills it.

// --- Shared subroutines (call/return) ---

// Sole owner of the guard ladder AND its callout. The ladder is not monotonic (Overload craters it
// and then hands it back), so this stores the last ANNOUNCED tier in LastGuard and prints only on a
// change, in either direction. Called from every place either input can move.
EncScript_Mewtwo_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mewtwo_ApplyGuard_Done   @ weakened owns its own guard
	encjumpifvar CMP_GREATER_THAN, 5, 1, EncScript_Mewtwo_ApplyGuard_T3   @ Overload 3 or 2
	encjumpifvar CMP_GREATER_THAN, 4, 1, EncScript_Mewtwo_ApplyGuard_T2   @ Instability 2+
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Mewtwo_ApplyGuard_T1
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 0
	printstring STRINGID_ENCMEWTWOREASSEMBLES
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_ApplyGuard_T1:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 1
	printstring STRINGID_ENCMEWTWOFLICKER
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_ApplyGuard_T2:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 2
	printstring STRINGID_ENCMEWTWORIPPLES
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_ApplyGuard_T3:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 3
	printstring STRINGID_ENCMEWTWOERUPTS
	waitmessage B_WAIT_TIME_LONG
EncScript_Mewtwo_ApplyGuard_Done:
	return

// A form change rebuilds the battle stats from the new species, which wipes every raw stat change
// made since. Re-applies the banked ones - Perfect Adaptation's boost, then one Annihilation stack
// at a time - so "permanent" means permanent. Runs after every encformchange except Weakened's.
EncScript_Mewtwo_RestorePower:
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Mewtwo_RestorePower_Stacks
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
EncScript_Mewtwo_RestorePower_Stacks:
	enccopyvar 13, 12   @ Scratch = Power, so the loop below can consume it without losing Power
EncScript_Mewtwo_RestorePower_Loop:
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Mewtwo_RestorePower_Done
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 12, ENC_AMOUNT_PERCENT
	encsubvar 13, 1
	goto EncScript_Mewtwo_RestorePower_Loop
EncScript_Mewtwo_RestorePower_Done:
	return

// The transformation body, shared by FirstAdapt and Adapt. The caller has already written the
// destination into Form. The failure path lands on _Cooldown, not on the return: Adapt's only gate
// is Cooldown == 0, so a script that returned without setting it would be re-selected immediately.
EncScript_Mewtwo_DoAdapt:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mewtwo_DoAdapt_Y
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO_MEGA_X, EncScript_Mewtwo_DoAdapt_Cooldown
	printstring STRINGID_ENCMEWTWOBECOMESX
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Mewtwo_DoAdapt_After
EncScript_Mewtwo_DoAdapt_Y:
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO_MEGA_Y, EncScript_Mewtwo_DoAdapt_Cooldown
	printstring STRINGID_ENCMEWTWOBECOMESY
	waitmessage B_WAIT_TIME_LONG
EncScript_Mewtwo_DoAdapt_After:
	encsetvar 6, 0   @ Stance - rebuilding the body interrupts whatever it was building
	encsetvar 2, 3   @ Lean back to neutral; the tug-of-war restarts
	encjumpifvar CMP_GREATER_THAN, 4, 2, EncScript_Mewtwo_DoAdapt_Cooldown   @ Instability clamps at 3
	encaddvar 4, 1
EncScript_Mewtwo_DoAdapt_Cooldown:
	encsetvar 3, 3
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Mewtwo_DoAdapt_Finish
	encsetvar 3, 1   @ Phase 2: it re-evaluates every turn and can no longer stop
EncScript_Mewtwo_DoAdapt_Finish:
	call EncScript_Mewtwo_RestorePower
	call EncScript_Mewtwo_ApplyGuard
	return

// Stance +1; at 3 it spends, and which payoff lands is the active form's. Only ever called while
// Form is non-zero, so the X branch is the correct fallthrough.
EncScript_Mewtwo_GainStance:
	encaddvar 6, 1
	encjumpifvar CMP_LESS_THAN, 6, 3, EncScript_Mewtwo_GainStance_Done
	encsetvar 6, 0
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mewtwo_GainStance_Focus
	// Annihilation (Mega X): a chunk off the player's side, and Mewtwo banks another Attack stack.
	// Escalation - the longer a Force meter is left to fill, the harder X hits for the rest of the
	// fight.
	printstring STRINGID_ENCMEWTWOFORCE
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_ALL_FOES, -15, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	// Bank the stack before applying it, so the live stat and Power never disagree at the cap -
	// RestorePower re-applies exactly what Power says after a form change wipes the stat.
	encjumpifvar CMP_GREATER_THAN, 12, 5, EncScript_Mewtwo_GainStance_Done   @ Power stacks cap at 6
	encaddvar 12, 1
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 12, ENC_AMOUNT_PERCENT
	return
EncScript_Mewtwo_GainStance_Focus:
	// Mind Crush (Mega Y): a smaller chunk, plus stat drops. Control rather than escalation.
	printstring STRINGID_ENCMEWTWOFOCUS
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
EncScript_Mewtwo_GainStance_Done:
	return

// Lean -1, floored at 0 (encsubvar wraps, so the floor has to be explicit). Shared by the special
// and status branches - for Mewtwo, a turn spent on setup reads the same as attacking specially.
EncScript_Mewtwo_LeanSpecial:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Mewtwo_LeanSpecial_Done
	encsubvar 2, 1
EncScript_Mewtwo_LeanSpecial_Done:
	return

// --- Trigger scripts ---

// No transformation yet and no trainerslide - this is a wild Pokemon. Lean starts neutral (vars
// zero-initialise, so 3 has to be written here).
EncScript_Mewtwo_Intro::
	encsetvar 2, 3   @ Lean
	printstring STRINGID_ENCMEWTWOINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Runs once at the top of every turn (TurnGuard gate): clears the per-event guards, ticks the
// adaptation cooldown, runs the recurring callout, then shrugs off sleep/freeze. The shrug is the
// one place status HELPS the player - it costs Mewtwo Instability, a third of a damage window.
EncScript_Mewtwo_TurnOpen::
	encsetvar 9, 1    @ TurnGuard
	encsetvar 10, 0   @ MoveGuard
	encsetvar 11, 0   @ BossGuard
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Mewtwo_TurnOpen_Chant
	encsubvar 3, 1    @ Cooldown
EncScript_Mewtwo_TurnOpen_Chant:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Mewtwo_TurnOpen_Status   @ analysis: nothing to comment on
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mewtwo_TurnOpen_Status   @ weakened
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Mewtwo_TurnOpen_Status   @ overloaded: it has its own lines
	encaddvar 8, 1    @ Chant
	encjumpifvar CMP_LESS_THAN, 8, 3, EncScript_Mewtwo_TurnOpen_Status
	encsetvar 8, 0
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Mewtwo_TurnOpen_Watches
	printstring STRINGID_ENCMEWTWOSTRAINS
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Mewtwo_TurnOpen_Status
EncScript_Mewtwo_TurnOpen_Watches:
	printstring STRINGID_ENCMEWTWOWATCHES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mewtwo_TurnOpen_Status:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mewtwo_TurnOpen_Done   @ weakened: status sticks now
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Mewtwo_TurnOpen_Shrug
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Mewtwo_TurnOpen_Shrug
	goto EncScript_Mewtwo_TurnOpen_Done
EncScript_Mewtwo_TurnOpen_Shrug:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCMEWTWOSHRUGS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_GREATER_THAN, 4, 2, EncScript_Mewtwo_TurnOpen_Done   @ Instability clamps at 3
	encaddvar 4, 1
	call EncScript_Mewtwo_ApplyGuard
EncScript_Mewtwo_TurnOpen_Done:
	return

// OnTurnEnd, priority 90 - runs last. Clears TurnGuard and steps the Overload window down, so it
// always resolves at a later checkpoint than the one that opened it.
EncScript_Mewtwo_TurnClose::
	encsetvar 9, 0   @ TurnGuard
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Mewtwo_TurnClose_Done
	encsubvar 5, 1   @ Overload 3 -> 2 -> 1
EncScript_Mewtwo_TurnClose_Done:
	return

// The player attacked physically: Lean climbs toward Mega X, capped at 6.
EncScript_Mewtwo_PlayerPhysical::
	encsetvar 10, 1   @ MoveGuard
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Mewtwo_PlayerPhysical_Done
	encaddvar 2, 1    @ Lean
EncScript_Mewtwo_PlayerPhysical_Done:
	return

// The player attacked specially: Lean falls toward Mega Y.
EncScript_Mewtwo_PlayerSpecial::
	encsetvar 10, 1   @ MoveGuard
	call EncScript_Mewtwo_LeanSpecial
	return

// A status move counts the same way, and Mega Y feeds on it directly - Focus fills off the turns
// the player spends not attacking, which is exactly what Y is meant to punish.
EncScript_Mewtwo_PlayerStatus::
	encsetvar 10, 1   @ MoveGuard
	call EncScript_Mewtwo_LeanSpecial
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Mewtwo_PlayerStatus_Done
	call EncScript_Mewtwo_GainStance
EncScript_Mewtwo_PlayerStatus_Done:
	return

// Mewtwo's own move landed on the player: the active form's Stance meter climbs.
EncScript_Mewtwo_BossHit::
	encsetvar 11, 1   @ BossGuard
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mewtwo_BossHit_Done   @ base Mewtwo has no stance
	call EncScript_Mewtwo_GainStance
EncScript_Mewtwo_BossHit_Done:
	return

// Turn 4: the analysis ends and the first transformation always fires, so the reveal lands as a
// transformation rather than a shrug. Lean >= 4 picks X, everything else picks Y.
EncScript_Mewtwo_FirstAdapt::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCMEWTWOANALYZED
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Mewtwo_FirstAdapt_X
	encsetvar 1, 2   @ Form = Mega Y
	goto EncScript_Mewtwo_FirstAdapt_Do
EncScript_Mewtwo_FirstAdapt_X:
	encsetvar 1, 1   @ Form = Mega X
EncScript_Mewtwo_FirstAdapt_Do:
	call EncScript_Mewtwo_DoAdapt
	return

// The cooldown expired. Phase 1: Lean decides, and the middle of the range is a dead zone it sits
// out quietly. Phase 2: it flips regardless of Lean - the adaptation has stopped being a choice, so
// every expiry is another point of Instability and the damage windows arrive on their own.
EncScript_Mewtwo_Adapt::
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Mewtwo_Adapt_Forced
	encjumpifvar CMP_GREATER_THAN, 2, 4, EncScript_Mewtwo_Adapt_WantX   @ Lean >= 5
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Mewtwo_Adapt_WantY      @ Lean <= 1
	encsetvar 3, 1   @ dead zone: re-check next turn, silently
	return
EncScript_Mewtwo_Adapt_WantX:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mewtwo_Adapt_Hold
	encsetvar 1, 1
	goto EncScript_Mewtwo_Adapt_Do
EncScript_Mewtwo_Adapt_WantY:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mewtwo_Adapt_Hold
	encsetvar 1, 2
	goto EncScript_Mewtwo_Adapt_Do
EncScript_Mewtwo_Adapt_Hold:
	// It is already in the form the player is asking for, so nothing changes and nothing
	// destabilises. The line is the nudge: it considered rebuilding and did not need to.
	encsetvar 3, 2   @ Cooldown
	printstring STRINGID_ENCMEWTWOISEE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_Adapt_Forced:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mewtwo_Adapt_ForceY
	encsetvar 1, 1   @ base or Mega Y -> Mega X
	goto EncScript_Mewtwo_Adapt_Do
EncScript_Mewtwo_Adapt_ForceY:
	encsetvar 1, 2
EncScript_Mewtwo_Adapt_Do:
	call EncScript_Mewtwo_DoAdapt
	return

// Instability 3: the body gives. It collapses back to base Mewtwo, takes a chunk out of itself, and
// its guard craters for two full turns. Overload and Instability are written before anything that
// can fail, so this trigger can never re-select itself. ApplyGuard prints the eruption line.
EncScript_Mewtwo_OverloadStart::
	encsetvar 5, 3   @ Overload - two full turns, stepped down by TurnClose
	encsetvar 4, 0   @ Instability
	encsetvar 6, 0   @ Stance
	encsetvar 2, 3   @ Lean
	encsetvar 3, 3   @ Cooldown - it can't adapt its way out of the window
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mewtwo_OverloadStart_Collapse
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO, EncScript_Mewtwo_OverloadStart_Collapse, B_ANIM_ULTRA_BURST
	encsetvar 1, 0   @ Form = base
	call EncScript_Mewtwo_RestorePower
EncScript_Mewtwo_OverloadStart_Collapse:
	call EncScript_Mewtwo_ApplyGuard
	encchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

// Two turns later: the window closes and the guard goes back to whatever the Instability ladder
// says, which ApplyGuard announces.
EncScript_Mewtwo_OverloadEnd::
	encsetvar 5, 0   @ Overload
	call EncScript_Mewtwo_ApplyGuard
	return

// 25% HP: the adaptation stops being a choice. The cooldown drops to a single turn, both offensive
// stats climb, and Adapt starts flipping the form whether Lean asked for it or not.
EncScript_Mewtwo_PerfectAdaptation::
	encsetvar 0, 2   @ Phase 2
	encsetvar 3, 1   @ Cooldown
	encsetvar 6, 0   @ Stance
	printstring STRINGID_ENCMEWTWOPERFECT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
	return

// 10% HP: the only catch window. The revert is what guarantees the player catches a SPECIES_MEWTWO
// rather than a Mega, instead of leaning on the species' FORM_CHANGE_END_BATTLE backstop. Every
// guard drops; the automatic catch-window damage guard takes over from here.
EncScript_Mewtwo_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 4, 0   @ Instability
	encsetvar 5, 0   @ Overload
	encsetvar 6, 0   @ Stance
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mewtwo_Weakened_Guards
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO, EncScript_Mewtwo_Weakened_Guards, B_ANIM_FORM_CHANGE
	encsetvar 1, 0   @ Form = base
EncScript_Mewtwo_Weakened_Guards:
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMEWTWOWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
