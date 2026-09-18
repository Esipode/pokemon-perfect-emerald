// Regirock, "The Ancient Fortress". Var indices, pinned by
// the always-true Conditions on EncScript_Regirock_Intro:
// 0 Phase (0 The Fortress Rises / 1 Mountain's Wrath / 2 Collapse / 3 Weakened), 1 Fort
// (Fortification 0-5, the spine), 2 LastFort (tier last ANNOUNCED - the re-fire latch), 3 Build
// (rebuild countdown), 4 Building (1 while a rebuild is in progress), 5 BuildMark (HP% mark for the
// rebuild interrupt test), 6 HpMark (HP% mark at turn open, for the quiet-turn test), 7 Fall
// (Rockfall countdown), 8 Collapse (Phase 2 finisher clock), 9 Braced (braced last turn - drives
// the alternating brace cadence AND suppresses quiet growth), 10 Shift (per-turn one-change guard
// shared by the three move-event triggers, in BOTH directions), 11 Shaken (sleep anti-cheese
// guard), 12 TurnGuard.
//
// Groudon is a ratchet you pry back down; Regice eats your turns. Regirock is a WALL you have to
// pick the right tool for, and the wrong tool makes it thicker. Every Fortification effect is
// RE-DERIVED from Var(Fort), never accumulated: the damage reduction is replaced rather than added
// and the three side statuses are cleared and re-raised for the current tier, so a Fortification
// that runs 0-3-1-4-2 leaves no residue and nothing has to remember how to undo a tier.

// --- Shared subroutines (call/return) ---

// Sole owner of everything Fortification means. The fall-through cascade at the bottom is what
// makes the tiers cumulative without restating them: tier 5 drops into Mist, which drops into
// Reflect, which drops into Lucky Chant.
// Baseline 88 is the guideline floor every legendary shares; the top rung is only ever reached by
// letting the wall run away.
EncScript_Regirock_ApplyLayers:
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_LUCKY_CHANT
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regirock_Layers_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Regirock_Layers_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Regirock_Layers_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Regirock_Layers_Two
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Regirock_Layers_One
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Regirock_Layers_One:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Regirock_Layers_Two:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	goto EncScript_Regirock_Layers_Chant
EncScript_Regirock_Layers_Three:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	goto EncScript_Regirock_Layers_Screen
EncScript_Regirock_Layers_Four:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	goto EncScript_Regirock_Layers_Mist
EncScript_Regirock_Layers_Five:
	encsetdamagereduction ENC_TARGET_BOSS, 97
EncScript_Regirock_Layers_Mist:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST, 0
EncScript_Regirock_Layers_Screen:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT, 0
EncScript_Regirock_Layers_Chant:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LUCKY_CHANT, 0
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones on an unchanged tier. Falling back to
// Fort 0 resets the latch through the leading branch, so a wall rebuilt from nothing announces its
// whole climb again.
// Tier 3 gets the Reflect animation because it is the one layer with a screen graphic to show.
EncScript_Regirock_FortCue:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regirock_FortCue_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Regirock_FortCue_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Regirock_FortCue_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Regirock_FortCue_Two
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Regirock_FortCue_One
	encsetvar 2, 0    @ LastFort
	return
EncScript_Regirock_FortCue_One:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regirock_FortCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCREGIROCKTIERONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regirock_FortCue_Two:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Regirock_FortCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCREGIROCKTIERTWO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regirock_FortCue_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Regirock_FortCue_Done
	encsetvar 2, 3
	printstring STRINGID_ENCREGIROCKTIERTHREE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_REFLECT
	return
EncScript_Regirock_FortCue_Four:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Regirock_FortCue_Done
	encsetvar 2, 4
	printstring STRINGID_ENCREGIROCKTIERFOUR
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Regirock_FortCue_Five:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Regirock_FortCue_Done
	encsetvar 2, 5
	printstring STRINGID_ENCREGIROCKTIERFIVE
	waitmessage B_WAIT_TIME_LONG
EncScript_Regirock_FortCue_Done:
	return

// Called by every path that moves Fortification. TurnOpen calls it once at the end of its pass; the
// move-driven shifts call it themselves so a layer broken mid-turn is felt on the very next hit
// rather than next turn.
EncScript_Regirock_ApplyFort:
	call EncScript_Regirock_ApplyLayers
	call EncScript_Regirock_FortCue
	return

// The only two places Fortification ever moves. encaddvar has no ceiling and encsubvar underflows a
// u8 to 255, so both bounds are tested first - and routing every change through here is what
// guarantees each one is announced.
EncScript_Regirock_FortUp:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Regirock_FortUp_Done
	encaddvar 1, 1
	printstring STRINGID_ENCREGIROCKFORTUP
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_HARDEN
EncScript_Regirock_FortUp_Done:
	return

EncScript_Regirock_FortDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regirock_FortDown_Done
	encsubvar 1, 1
	printstring STRINGID_ENCREGIROCKFORTDOWN
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_ROCK_SMASH
EncScript_Regirock_FortDown_Done:
	return

// The quiet turn: the spec's "several turns pass without it being significantly damaged" kept
// literally. HpMark was set at the previous turn open, so ENC_SNAP_DAMAGE turns it into "percent of
// max HP lost over the last turn"; the command jumps its fail label when nothing was written, which
// is the zero-damage case and routes to the same place a small hit does.
// Skipped after a braced turn - a turn Regirock spent behind Protect is not the player's failure to
// hurt it. The mark is re-taken on every path so the window is always exactly one turn.
// 2% of max HP is a deliberately low bar: the layer punishes a turn spent doing NOTHING to
// Regirock, not a turn that failed to do enough.
EncScript_Regirock_QuietTurn:
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Regirock_QuietTurn_Mark   @ Braced
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_DAMAGE, EncScript_Regirock_QuietTurn_Quiet
	encjumpifvar CMP_GREATER_THAN, 6, 1, EncScript_Regirock_QuietTurn_Mark
EncScript_Regirock_QuietTurn_Quiet:
	printstring STRINGID_ENCREGIROCKQUIET
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regirock_FortUp
EncScript_Regirock_QuietTurn_Mark:
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET
	return

// Phase 2 only: the mountain sheds a layer by itself every turn and the debris rains on the party,
// pulling against the rebuild clock. That is the spec's "Fortification rapidly fluctuates",
// expressed as two existing clocks pulling opposite ways.
// Refused at Fortification 0 for the same reason Rockfall is - there is nothing left to fall.
EncScript_Regirock_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Regirock_Shed_Done
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regirock_Shed_Done
	call EncScript_Regirock_FortDown
	printstring STRINGID_ENCREGIROCKBREAKAWAY
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_ROCK_THROW
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
EncScript_Regirock_Shed_Done:
	return

// The rebuild clock. Build counts down here and fires at 1 rather than being ticked to 0 and fired
// in the same pass, so the announced cadence is the cadence the player actually feels. 0 is inert.
// encsetrecharge with 1 at OnTurnStart costs Regirock THIS turn's action and the engine prints its
// own "must recharge!" line when the action would have resolved, so the free turn is real and
// visible. The mark taken here is what RebuildResolve measures the interrupt against.
EncScript_Regirock_BuildTick:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Regirock_BuildTick_Done
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Regirock_BuildTick_Begin
	encsubvar 3, 1
	return
EncScript_Regirock_BuildTick_Begin:
	printstring STRINGID_ENCREGIROCKREBUILD
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_POLISH
	encsetrecharge ENC_TARGET_BOSS, 1
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_SET   @ BuildMark
	encsetvar 4, 1    @ Building
EncScript_Regirock_BuildTick_Done:
	return

// Re-arm values are one higher than the gap they produce, because BuildTick fires on 1: 4 is a
// rebuild every fourth turn, 3 every third.
EncScript_Regirock_BuildArm:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regirock_BuildArm_Slow
	encsetvar 3, 3
	return
EncScript_Regirock_BuildArm_Slow:
	encsetvar 3, 4
	return

// The Rockfall and collapse clocks. Both tick here and resolve at OnTurnEnd - the documented split,
// so a re-arm can never be eaten by its own tick.
EncScript_Regirock_FallTick:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regirock_FallTick_Done
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Regirock_FallTick_Done
	encsubvar 7, 1
EncScript_Regirock_FallTick_Done:
	return

EncScript_Regirock_FallArm:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Regirock_FallArm_Mid
	encsetvar 7, 3    @ Collapse: a Rockfall every second turn
	return
EncScript_Regirock_FallArm_Mid:
	encsetvar 7, 4    @ Mountain's Wrath: every third turn
	return

EncScript_Regirock_CollapseClock:
	encjumpifvar CMP_LESS_THAN, 8, 2, EncScript_Regirock_CollapseClock_Done
	encsubvar 8, 1
EncScript_Regirock_CollapseClock_Done:
	return

// Mountain Form's brace, at Fortification 5 only. encsetprotect at OnTurnStart is the one checkpoint
// where it works - gProtectStructs is cleared after end-of-turn effects, so set here it covers the
// whole turn and expires on its own.
// Var(Braced) is both the alternation flag and the quiet-turn suppressor: a turn that braces sets
// it, the next turn reads it (skipping the quiet growth, since a braced turn is not the player's
// failure) and clears it, so the brace lands every other turn. Falling below Fortification 5 clears
// it through the same tail, so a lost layer can never leave the suppressor stuck on.
EncScript_Regirock_Brace:
	encjumpifvar CMP_NOT_EQUAL, 1, 5, EncScript_Regirock_Brace_Rest
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Regirock_Brace_Rest
	encsetvar 9, 1    @ Braced
	printstring STRINGID_ENCREGIROCKBRACE
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_PROTECT
	encsetprotect ENC_TARGET_BOSS
	return
EncScript_Regirock_Brace_Rest:
	encsetvar 9, 0    @ Braced
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the clocks and the marks need seeding.
// Braced starts at 1 purely to suppress the quiet-turn test on turn 1, which would otherwise see
// zero damage taken before anyone has moved and hand out a free layer; the Fortification 0 tail of
// the brace routine clears it in the same pass.
EncScript_Regirock_Intro::
	encsetvar 3, 4    @ Build: the first reconstruction lands on turn 4
	encsetvar 9, 1    @ Braced
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET   @ HpMark
	printstring STRINGID_ENCREGIROCKAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_STEALTH_ROCK
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail so no branch out of this script can leave it re-armed.
// Order matters. The quiet-turn test reads the mark before anything else can move Regirock's HP;
// the three clocks tick before the ladder is re-derived, so a layer gained or lost by a clock is
// already reflected in the guard the player meets this turn; the brace runs last because it is the
// only step that depends on the final value of Fortification.
// The closing line is the recurring "the mechanic is still on" heartbeat - a player who missed the
// tier callout still sees, every turn, that something is wrong with the wall.
EncScript_Regirock_TurnOpen::
	flushtextbox
	encsetvar 12, 1    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Regirock_TurnOpen_Done   @ Weakened: inert
	call EncScript_Regirock_QuietTurn
	call EncScript_Regirock_Shed
	call EncScript_Regirock_BuildTick
	call EncScript_Regirock_FallTick
	call EncScript_Regirock_CollapseClock
	call EncScript_Regirock_ApplyFort
	call EncScript_Regirock_Brace
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Regirock_TurnOpen_Done
	printstring STRINGID_ENCREGIROCKGRIND
	waitmessage B_WAIT_TIME_SHORT
EncScript_Regirock_TurnOpen_Done:
	return

// TurnClose owns nothing but the per-turn guard resets. Every countdown lives at TurnOpen instead,
// one checkpoint away from the OnTurnEnd resolutions that re-arm them.
EncScript_Regirock_TurnClose::
	encsetvar 12, 0    @ TurnGuard
	encsetvar 10, 0    @ Shift: the one move-driven change is armed again
	encsetvar 11, 0    @ Shaken
	return

// Anti-cheese. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at
// OnTurnStart - and the wall thickens for the trouble, so putting the mountain to sleep is not free.
// Leads with flushtextbox for the same reason TurnOpen does: this runs one priority earlier.
EncScript_Regirock_WillNotSleep::
	flushtextbox
	encsetvar 11, 1    @ Shaken
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCREGIROCKNOSLEEP
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Regirock_FortUp
	call EncScript_Regirock_ApplyFort
	return

// The three shift scripts. Each sets Var(Shift) itself: an Event.* condition stays true for the
// whole dispatch and cannot be moved by the script, so an event-reacting trigger has to disable
// itself or the re-evaluation loop re-selects it until the per-checkpoint script cap trips.
// The break is the fight's safety valve and is deliberately damage-independent - it does not look
// at how much the move did, only at its type.
EncScript_Regirock_Break::
	encsetvar 10, 1    @ Shift
	call EncScript_Regirock_FortDown
	call EncScript_Regirock_ApplyFort
	return

EncScript_Regirock_Build::
	encsetvar 10, 1    @ Shift
	call EncScript_Regirock_FortUp
	call EncScript_Regirock_ApplyFort
	return

EncScript_Regirock_BossBuild::
	encsetvar 10, 1    @ Shift
	call EncScript_Regirock_FortUp
	call EncScript_Regirock_ApplyFort
	return

// The rebuild, resolved. BuildMark holds the HP% taken at turn open, so ENC_SNAP_DAMAGE turns it
// into "percent of max HP lost since the reconstruction began"; the fail label is the zero-damage
// case and routes to the same place a small hit does.
// 3% of max HP through an 88-97% reduction is a bar a real attack clears and a wasted turn does
// not, so interrupting is about SPENDING the free turn rather than about passing a burst check.
// Completing is worth one layer and a 3% heal, so the swing across the interrupt is two layers
// rather than three. The clock is re-armed up front so every path leaves it armed.
EncScript_Regirock_RebuildResolve::
	encsetvar 4, 0    @ Building
	call EncScript_Regirock_BuildArm
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Regirock_RebuildResolve_Whole
	encjumpifvar CMP_LESS_THAN, 5, 3, EncScript_Regirock_RebuildResolve_Whole
	printstring STRINGID_ENCREGIROCKREBUILDBROKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_BLAST
	call EncScript_Regirock_FortDown
	goto EncScript_Regirock_RebuildResolve_Apply
EncScript_Regirock_RebuildResolve_Whole:
	printstring STRINGID_ENCREGIROCKREBUILDDONE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Regirock_FortUp
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Regirock_RebuildResolve_Apply:
	call EncScript_Regirock_ApplyFort
	return

// Rockfall. Regirock breaks off a layer and throws it, so the payload scales with how thick the
// wall was - and at Fortification 0 it is REFUSED and told so. That refusal is the payoff for the
// whole break-the-wall game: a stripped fortress has nothing to throw, and keeping it stripped is
// now offence as well as defence.
// The raw Defense loss is the fight's one accumulating effect, and it is monotone - Rockfall only
// ever fires downward - so the second half genuinely gets easier the more Regirock throws. 3% a
// throw, in proportion to the smaller payload each throw now costs the player.
// ENC_TARGET_ALL_FOES rather than a single slot throughout: Rockfall can KO, and a group target
// skips a fainted battler silently where a single-slot target would assert.
EncScript_Regirock_RockfallResolve::
	call EncScript_Regirock_FallArm
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regirock_RockfallResolve_Nothing
	printstring STRINGID_ENCREGIROCKROCKFALL
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_SLIDE
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regirock_RockfallResolve_Huge
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Regirock_RockfallResolve_Big
	encchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	goto EncScript_Regirock_RockfallResolve_Spend
EncScript_Regirock_RockfallResolve_Big:
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Regirock_RockfallResolve_Spend
EncScript_Regirock_RockfallResolve_Huge:
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Regirock_RockfallResolve_Spend:
	call EncScript_Regirock_FortDown
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -3, ENC_AMOUNT_PERCENT
	call EncScript_Regirock_ApplyFort
	return
EncScript_Regirock_RockfallResolve_Nothing:
	printstring STRINGID_ENCREGIROCKNOTHINGLEFT
	waitmessage B_WAIT_TIME_SHORT
	return

// 50% - MOUNTAIN'S WRATH. It stops hiding behind the fortress and starts throwing it. Fall is set
// to 2 so the first Rockfall lands at the close of the next turn - one turn of warning after the
// line that announces it. The Attack bump is the fight's one offensive step-up, applied once and
// monotonically, so the second half hits harder even as Rockfall bleeds its Defense away.
EncScript_Regirock_MountainsWrath::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCREGIROCKWRATH
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ANCIENT_POWER
	encsetvar 7, 2    @ Fall
	call EncScript_Regirock_BuildArm
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	return

// 25% - COLLAPSE. A layer falls off by itself every turn and the debris rains on the party, the
// rebuild keeps pulling the other way on its own cadence, Rockfall doubles up, and a fixed clock
// starts running down to the finisher. Fall is set to 1 so a Rockfall lands at the end of this very
// turn - the phase arriving with a hit rather than an announcement.
// Survive is set here rather than in Properties: so the AI reads Regirock as killable for three
// quarters of the fight, and from this point it guarantees the player reaches the catch window
// instead of losing it to one oversized hit through a band only 10% of max HP wide.
EncScript_Regirock_Collapse::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCREGIROCKCOLLAPSING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_THROW
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encsetvar 8, 6    @ Collapse: five turns to MOUNTAIN'S COLLAPSE
	encsetvar 7, 1    @ Fall
	call EncScript_Regirock_BuildArm
	return

// MOUNTAIN'S COLLAPSE. The finisher, and the guarantee that the catch window always opens however
// the damage race went: ENC_AMOUNT_TO_PERCENT is an absolute destination, not a delta.
// encclearscreens takes a fail label rather than failing, so the same command is both "did the
// player have anything up?" and "take it away" - whatever they built goes down with the cavern.
// Every clock is zeroed on the way out and the ladder re-derived from Fort 0, so all three side
// statuses and the whole guard go at once. Weakened (priority 7) wins the next re-evaluation pass
// at this same checkpoint.
EncScript_Regirock_CollapseTick::
	encsetvar 8, 0    @ Collapse: the finisher never repeats
	printstring STRINGID_ENCREGIROCKMOUNTAINFALLS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_WRECKER
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Regirock_CollapseTick_Hit
EncScript_Regirock_CollapseTick_Hit:
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetvar 1, 0    @ Fort
	encsetvar 3, 0    @ Build: nothing rebuilds after the mountain falls
	encsetvar 7, 0    @ Fall
	encsetvar 4, 0    @ Building
	call EncScript_Regirock_ApplyFort
	return

// 10% - the catch window, the mirror image of the Properties: block. Every clock is made inert and
// every layer stripped through the same routine that raised them, so no side status can outlive the
// fight it belonged to. Phase 3 makes TurnOpen return through its leading branch, so nothing can
// restart after this point.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Regirock_Weakened::
	encsetvar 0, 3    @ Phase
	encsetvar 1, 0    @ Fort
	encsetvar 2, 0    @ LastFort
	encsetvar 3, 0    @ Build
	encsetvar 4, 0    @ Building
	encsetvar 7, 0    @ Fall
	encsetvar 8, 0    @ Collapse
	encsetvar 9, 0    @ Braced
	call EncScript_Regirock_ApplyLayers
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 25
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCREGIROCKWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
