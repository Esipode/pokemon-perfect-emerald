// Azelf, "The Being of Willpower". Var indices, pinned by the
// always-true Conditions on EncScript_Azelf_Intro:
// 0 Phase (0 The Being of Willpower / 1 Resolute / 2 Last Stand / 3 Will Broken / 4 Weakened),
// 1 Resolve (0-5, the meter), 2 Tier (rung last APPLIED - ApplyResolve's idempotence latch),
// 3 Announced (rung last ANNOUNCED - the cue's re-fire latch), 4 TurnGuard, 5 HpMark (boss HP% at
// turn open; ENC_SNAP_DAMAGE turns it into "lost this turn"), 6 Clock (Last Stand countdown),
// 7 StatusGuard (a status has spent its shrug), 8 EndureGuard (per-turn guard shared by Endure and
// Emboldened), 9 SecondWind (0 unspent / 1 spent).
//
// The fight is one comparison repeated every turn: damage taken this turn against a bar (5% of max
// HP in Phase 0, 7% in Phase 1, both measured AFTER the 90 guard). Over the bar and Resolve falls;
// under it and Resolve climbs. Resolve is a ladder of five named rungs, every one of which turns
// back off when the player pushes it down, and the whole economy exists to decide one moment: at
// 30% HP, SECOND WIND either heals Azelf to 45% and jumps it to UNBREAKABLE, or finds nothing there.

// --- Shared subroutines (call/return) ---

// Sole owner of Azelf's Atk, SpAtk and Speed. An ABSOLUTE set, not a delta - encchangestat clamps at
// the stage bounds, so -12 lands on the floor from wherever the stage had drifted and +N walks back
// up to an exact value. That is what makes this idempotent and what lets the ladder move in both
// directions without tracking what it added. Safe precisely because Azelf's moveset carries no
// self-buff, so nothing else in the battle ever writes those three stages.
// Each rung latches on Var(Tier) against its own literal - there is no var-to-var comparison - and
// every path falls through to the cue, which latches separately on Var(Announced).
EncScript_Azelf_ApplyResolve:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Azelf_Rung_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Azelf_Rung_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Azelf_Rung_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Azelf_Rung_Two
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Azelf_Rung_One
EncScript_Azelf_Rung_Zero:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Azelf_Rung_Cue
	encsetvar 2, 0    @ Tier
	call EncScript_Azelf_NeutralStats
	goto EncScript_Azelf_Rung_Cue
EncScript_Azelf_Rung_One:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Azelf_Rung_Cue
	encsetvar 2, 1
	call EncScript_Azelf_NeutralStats
	goto EncScript_Azelf_Rung_Cue
EncScript_Azelf_Rung_Two:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Azelf_Rung_Cue
	encsetvar 2, 2
	call EncScript_Azelf_NeutralStats
	goto EncScript_Azelf_Rung_Cue
EncScript_Azelf_Rung_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Azelf_Rung_Cue
	encsetvar 2, 3
	call EncScript_Azelf_NeutralStats
	goto EncScript_Azelf_Rung_Cue
// UNMATCHED. Base 115 Speed at +2 outruns essentially anything the player brings, which is what
// "it acts first" means in practice.
EncScript_Azelf_Rung_Four:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Azelf_Rung_Cue
	encsetvar 2, 4
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 8
	goto EncScript_Azelf_Rung_Cue
EncScript_Azelf_Rung_Five:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Azelf_Rung_Cue
	encsetvar 2, 5
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 8
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 8
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 8
EncScript_Azelf_Rung_Cue:
	call EncScript_Azelf_Cue
	return

EncScript_Azelf_NeutralStats:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	return

// The rung line, latched on Var(Announced) rather than on Var(Tier) so it fires on every real change
// in either direction and never repeats an unchanged rung. Climbing prints the new rung; falling
// prints the one it fell to, down to "Azelf's stance loosens." No rung is ever a silent number.
EncScript_Azelf_Cue:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Azelf_Cue_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Azelf_Cue_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Azelf_Cue_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Azelf_Cue_Two
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Azelf_Cue_One
EncScript_Azelf_Cue_Zero:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Azelf_Cue_Done
	encsetvar 3, 0    @ Announced
	printstring STRINGID_ENCAZELFSTEADY
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	return
EncScript_Azelf_Cue_One:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Azelf_Cue_Done
	encsetvar 3, 1
	printstring STRINGID_ENCAZELFUNYIELDING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return
EncScript_Azelf_Cue_Two:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Azelf_Cue_Done
	encsetvar 3, 2
	printstring STRINGID_ENCAZELFUNDIMINISHED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return
EncScript_Azelf_Cue_Three:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Azelf_Cue_Done
	encsetvar 3, 3
	printstring STRINGID_ENCAZELFUNBOWED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return
EncScript_Azelf_Cue_Four:
	encjumpifvar CMP_EQUAL, 3, 4, EncScript_Azelf_Cue_Done
	encsetvar 3, 4
	printstring STRINGID_ENCAZELFUNMATCHED
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_AGILITY
	return
EncScript_Azelf_Cue_Five:
	encjumpifvar CMP_EQUAL, 3, 5, EncScript_Azelf_Cue_Done
	encsetvar 3, 5
	printstring STRINGID_ENCAZELFUNBREAKABLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
EncScript_Azelf_Cue_Done:
	return

// Every path that moves Resolve goes through these two, so the clamp lives in exactly one place and
// the ladder and the cue can never fall out of step with the meter.
EncScript_Azelf_GainResolve:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Azelf_GainResolve_Done
	encaddvar 1, 1
	call EncScript_Azelf_ApplyResolve
EncScript_Azelf_GainResolve_Done:
	return

EncScript_Azelf_LoseResolve:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Azelf_LoseResolve_Done
	encsubvar 1, 1
	call EncScript_Azelf_ApplyResolve
EncScript_Azelf_LoseResolve_Done:
	return

// UNDIMINISHED's actual mechanism: at Resolve >= 2 the stat stages are re-asserted absolutely at
// every turn open, so anything that dragged them down overnight is undone. Silent - Tier is forced
// stale to make the stat write happen, but Announced is untouched, so the cue sees an unchanged
// rung and says nothing. The announced version is the Undiminished trigger's job.
EncScript_Azelf_Reassert:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Azelf_Reassert_Done
	encsetvar 2, 255  @ Tier: forced stale
	call EncScript_Azelf_ApplyResolve
EncScript_Azelf_Reassert_Done:
	return

// The Last Stand countdown, ticked here and resolved at OnTurnEnd - one checkpoint away, so a tick
// can never eat a step off a fresh arm. Held at 1 for the resolution to consume.
EncScript_Azelf_ClockTick:
	encjumpifvar CMP_LESS_THAN, 6, 2, EncScript_Azelf_ClockTick_Done
	encsubvar 6, 1
EncScript_Azelf_ClockTick_Done:
	return

// The Resolve 5 lash. A standing per-turn punish rather than a single charged detonation, so sitting
// at UNBREAKABLE is continuously unaffordable and stops costing the moment the player pushes the
// meter back to 4. ENC_TARGET_ALL_FOES rather than a single slot: doubles-safe for free, and it
// skips a fainted member silently.
EncScript_Azelf_Lash:
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Azelf_Lash_Done
	printstring STRINGID_ENCAZELFLASH
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_EXTRASENSORY
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Azelf_Lash_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so nothing needs seeding. Deliberately does NOT call
// ApplyResolve: Resolve 0 is the neutral state, and announcing it on turn one would spend the
// "stance loosens" line before it can mean anything.
EncScript_Azelf_Intro::
	printstring STRINGID_ENCAZELFAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail so no branch out of this script can leave it re-armed.
// The heartbeats are guideline 14's standing channel - a player who missed the rung line is told,
// every turn, that the state is still live.
EncScript_Azelf_TurnOpen::
	flushtextbox
	encsetvar 4, 1    @ TurnGuard
	encsetvar 8, 0    @ EndureGuard: the Endure/Emboldened lever is armed again
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_Azelf_TurnOpen_Done   @ Weakened: inert
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_SET   @ the mark TurnClose reads for the verdict
	call EncScript_Azelf_Reassert
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Azelf_TurnOpen_LastStand
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Azelf_TurnOpen_Done   @ Will Broken: nothing burns
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Azelf_TurnOpen_Done
	printstring STRINGID_ENCAZELFBURNS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Azelf_TurnOpen_LastStand:
	call EncScript_Azelf_ClockTick
	printstring STRINGID_ENCAZELFHOLDS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Azelf_TurnOpen_Done:
	return

// The turn verdict - the core loop, and the only script that moves Resolve from combat. HpMark was
// written at turn open with ENC_SNAP_SET; ENC_SNAP_DAMAGE turns it into "percentage of max HP lost
// this turn", cumulative across multi-hits, chip and status. Its failInstr is the zero-damage case
// falling straight through, so Protect turns, setup turns and switches are neutral rather than
// feeding the meter - only trying and failing feeds it.
// The lash runs BEFORE the verdict on purpose: a turn that pushes Resolve off 5 still pays the lash
// it earned by damaging Azelf while it was at 5.
// The bar is the phase's, read off Var(Phase) rather than stored. Both figures are measured after
// the 90 guard, so 5% net is a raw hit worth roughly half Azelf's max HP and 7% roughly seventy
// percent of it - escalating exactly when the player's options have improved.
EncScript_Azelf_TurnClose::
	encsetvar 4, 0    @ TurnGuard
	encsetvar 8, 0    @ EndureGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Azelf_TurnClose_Done   @ Will Broken / Weakened
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Azelf_TurnClose_Done
	call EncScript_Azelf_Lash
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Azelf_TurnClose_Done   @ Last Stand: Resolve is pinned
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Azelf_TurnClose_Resolute
	encjumpifvar CMP_LESS_THAN, 5, 5, EncScript_Azelf_TurnClose_Steeled
	goto EncScript_Azelf_TurnClose_Faltered
EncScript_Azelf_TurnClose_Resolute:
	encjumpifvar CMP_LESS_THAN, 5, 7, EncScript_Azelf_TurnClose_Steeled
// BURST - the player cleared the bar.
EncScript_Azelf_TurnClose_Faltered:
	printstring STRINGID_ENCAZELFFALTERS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Azelf_LoseResolve
	return
EncScript_Azelf_TurnClose_Steeled:
	printstring STRINGID_ENCAZELFSTEELS
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Azelf_GainResolve
EncScript_Azelf_TurnClose_Done:
	return

// ENDURE. Fires when Extrasensory misses too - OnMoveEnd still dispatches for a move that whiffed
// and the event's value fields are stale after a miss, so there is no way to tell "survived it"
// from "it missed". "You were not taken down by it" is a fair reading of the lever, and the
// alternative is no lever.
EncScript_Azelf_Endure::
	encsetvar 8, 1    @ EndureGuard
	printstring STRINGID_ENCAZELFENDURED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Azelf_LoseResolve
	return

EncScript_Azelf_Emboldened::
	encsetvar 8, 1    @ EndureGuard
	printstring STRINGID_ENCAZELFEMBOLDENED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Azelf_GainResolve
	return

// UNYIELDING. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at a turn
// checkpoint. The shrug feeds the meter, which is what turns statusing Azelf from a good idea into
// a bad one the moment the ladder comes online; the line says so plainly the first time it happens.
EncScript_Azelf_Unyielding::
	encsetvar 7, 1    @ StatusGuard
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCAZELFSHRUGS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	call EncScript_Azelf_GainResolve
	return

// Silent - nothing has happened the player needs told. It only re-arms the shrug, so each one costs
// a fresh application rather than a standing burn.
EncScript_Azelf_StatusCleared::
	encsetvar 7, 0    @ StatusGuard
	return

EncScript_Azelf_Undiminished::
	printstring STRINGID_ENCAZELFSNAPBACK
	waitmessage B_WAIT_TIME_LONG
	encsetvar 2, 255  @ Tier: forced stale so the stages are rewritten
	call EncScript_Azelf_ApplyResolve
	playmoveanimation MOVE_HONE_CLAWS
	return

// 70% - the ladder's first free rung. Guarantees the meter climbs at least once on its own, so a
// purely defensive player still meets a named state early enough to read what it means.
EncScript_Azelf_Steeling::
	printstring STRINGID_ENCAZELFNARROWS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Azelf_GainResolve
	return

// 45% - RESOLUTE. The bar rises from 5% to 7%, and the line is the cue for it. The second free rung:
// together with Steeling it guarantees a passive player arrives at Second Wind's check with UNBOWED
// already armed.
EncScript_Azelf_Resolute::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCAZELFRESOLUTE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	call EncScript_Azelf_GainResolve
	return

// 30% - SECOND WIND, the fight's decision point and the largest single swing in it. Var(SecondWind)
// is spent up front so both branches close the door behind them.
// UNBOWED armed: 15% of a max HP bar handed back, plus a free trip to UNBREAKABLE. A player who has
// been chipping carefully watches half their progress evaporate. Broken in time: it reaches for its
// will and comes up empty, and the fight simply carries on downhill. That difference is what the
// whole Resolve economy is for.
EncScript_Azelf_SecondWind::
	encsetvar 9, 1    @ SecondWind: spent
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Azelf_SecondWind_Hollow
	printstring STRINGID_ENCAZELFREFUSES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	enchangehp ENC_TARGET_BOSS, 45, ENC_AMOUNT_TO_PERCENT
	printstring STRINGID_ENCAZELFFLOODSBACK
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 5    @ Resolve: UNBREAKABLE WILL comes online
	call EncScript_Azelf_ApplyResolve
	return
EncScript_Azelf_SecondWind_Hollow:
	printstring STRINGID_ENCAZELFHOLLOW
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	return

// 20% - LAST STAND. Four full turns of siege: Resolve is pinned at 5 so nothing the player does
// moves the meter, the guard tightens, and the player cannot switch or flee. Survive: has been on
// since battle start, so this cannot be ended early - it is endurance, not a damage race.
EncScript_Azelf_LastStand::
	encsetvar 0, 2    @ Phase
	encsetvar 6, 5    @ Clock: four full turns
	encsetvar 1, 5    @ Resolve: pinned
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	encsetdamagereduction ENC_TARGET_BOSS, 93
	printstring STRINGID_ENCAZELFLASTSTAND
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Azelf_ApplyResolve
	return

// The clock ran out. Everything the will was holding up goes at once: the ladder falls to nothing,
// the hold is released, and the guard collapses from 93 to 55 - the player's next hit is worth
// roughly nine times what it was. Tier and Announced are forced stale so the rung line fires on the
// way down rather than being swallowed by the latch.
EncScript_Azelf_WillBroken::
	encsetvar 0, 3    @ Phase
	encsetvar 6, 0    @ Clock
	encsetvar 1, 0    @ Resolve
	encsetvar 2, 255  @ Tier: forced stale
	encsetvar 3, 255  @ Announced: forced stale
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 75
	printstring STRINGID_ENCAZELFBREAKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	call EncScript_Azelf_ApplyResolve
	return

// 10% - the catch window, the mirror image of the Properties: block. Guideline 10 exactly: no ball
// can be thrown before this point. Every counter the fight built is zeroed so nothing outlives it,
// and Phase 4 makes TurnOpen return through its leading branch, so no clock can restart.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Azelf_Weakened::
	encsetvar 0, 4    @ Phase
	encsetvar 1, 0    @ Resolve
	encsetvar 2, 0    @ Tier
	encsetvar 3, 0    @ Announced
	encsetvar 6, 0    @ Clock
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	call EncScript_Azelf_NeutralStats
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCAZELFWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
