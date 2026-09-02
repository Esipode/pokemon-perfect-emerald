// Groudon, "The Living Continent". Var indices, pinned by
// the always-true Conditions on EncScript_Groudon_Intro:
// 0 Land (0-5, the landmass everything else derives from), 1 Phase (0 The Land Rises / 1 Primal
// Groudon / 2 Continental Collapse / 3 Weakened), 2 LastLand (landmass tier last ANNOUNCED - the
// re-fire latch), 3 Fuse (quake countdown, 1-4), 4 Grow (natural growth countdown), 5 Collapse
// (Phase 2 collapse cadence), 6 Shift (per-turn guard shared by the three move-event triggers, in
// BOTH directions), 7 Shaken (per-turn guard for the paralysis anti-cheese trigger), 8 TurnGuard,
// 9 Snap (encsnapshothp scratch for the collapse finisher), 10 Sunlit (per-turn guard for the sun
// re-assertion trigger).
//
// Kyogre is a fight over a level you hold; Groudon is a fight over a RATCHET you have to pry back
// down. The Land climbs on its own, climbs when Groudon lands a Ground or Fire move, and climbs
// when Fire touches it. It comes down for exactly one reason: a damaging Water move hitting
// Groudon. Only ONE move-driven change lands per turn (Var(Shift)), in either direction, and
// Groudon's 90 Speed is usually fast enough to spend it first.
// The Land does three things at once: it raises Groudon's damage reduction, it scorches the whole
// player side every turn from Scorched Plateau up, and it sets the QUAKE FUSE - both how hard the
// quake hits and how few turns there are between quakes.
// Everything derived from the Land is RE-DERIVED at TurnOpen rather than accumulated, so a landmass
// that runs 0-3-1-4-2 leaves no residue and nothing has to remember how to undo the last tier.

// --- Shared subroutines (call/return) ---

// Sole owner of the landmass damage reduction ladder. 90 is the guideline baseline and the bottom
// rung; the top rung is reached only by letting the land run away.
EncScript_Groudon_ApplyGuard:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Groudon_Guard95
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_Guard92
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Groudon_Guard88
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Groudon_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Groudon_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Groudon_Guard95:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return

// Every climb in the fight goes through here. encaddvar has no ceiling of its own, so the clamp is
// a guarded branch rather than a bare add.
EncScript_Groudon_LandUp:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Groudon_LandUp_Done
	encaddvar 0, 1
	printstring STRINGID_ENCGROUDONLANDRISES
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
EncScript_Groudon_LandUp_Done:
	return

// Every erosion goes through here. encsubvar on a u8 underflows to 255, so 0 is tested explicitly.
// Unlike Kyogre's tide there is no phase floor: "Groudon does not easily give territory back" is
// expressed as scarcity of the lever - one Water move per turn, against growth that never stops -
// rather than as a hard bound the player can't push past.
EncScript_Groudon_LandDown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Groudon_LandDown_Done
	encsubvar 0, 1
	printstring STRINGID_ENCGROUDONLANDFALLS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Groudon_LandDown_Done:
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones. Dropping back to Land 0 resets the latch
// through the leading branch, so the whole climb re-announces if the land rises again.
EncScript_Groudon_LandCue:
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Groudon_LandCue_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_LandCue_Four
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Groudon_LandCue_Three
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Groudon_LandCue_Two
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Groudon_LandCue_One
	encsetvar 2, 0
	return
EncScript_Groudon_LandCue_One:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Groudon_LandCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCGROUDONLANDONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Groudon_LandCue_Two:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Groudon_LandCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCGROUDONLANDTWO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Groudon_LandCue_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Groudon_LandCue_Done
	encsetvar 2, 3
	printstring STRINGID_ENCGROUDONLANDTHREE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Groudon_LandCue_Four:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Groudon_LandCue_Done
	encsetvar 2, 4
	printstring STRINGID_ENCGROUDONLANDFOUR
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Groudon_LandCue_Five:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Groudon_LandCue_Done
	encsetvar 2, 5
	printstring STRINGID_ENCGROUDONLANDFIVE
	waitmessage B_WAIT_TIME_LONG
EncScript_Groudon_LandCue_Done:
	return

// The scorch chip, from Scorched Plateau upward. ENC_TARGET_ALL_FOES rather than a single slot:
// this can KO, and a single-slot target asserts on a fainted battler while a group target skips the
// absent silently.
EncScript_Groudon_Scorch:
	encjumpifvar CMP_LESS_THAN, 0, 3, EncScript_Groudon_Scorch_Done
	printstring STRINGID_ENCGROUDONSCORCH
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_LAVA_PLUME
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Groudon_Scorch_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_Scorch_Four
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Groudon_Scorch_Four:
	enchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Groudon_Scorch_Five:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Groudon_Scorch_Done:
	return

// The natural cadence. It does NOT spend Var(Shift), so from the Primal Reversion on the land gains
// a step every turn for free and an erosion only ever buys the player a net zero - which is why
// holding the line, rather than levelling the continent, is the whole second-half game.
// Grow counts DOWN and rises the land on the turn it is already 0, so a re-arm of 2 is a rise every
// third turn and a re-arm of 0 is a rise every turn.
EncScript_Groudon_Growth:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Groudon_Growth_Rise
	encsubvar 4, 1
	return
EncScript_Groudon_Growth_Rise:
	call EncScript_Groudon_LandUp
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Groudon_Growth_Slow
	encsetvar 4, 0    @ Primal onward: every turn
	return
EncScript_Groudon_Growth_Slow:
	encsetvar 4, 2    @ The Land Rises: every third turn
	return

// The quake fuse, ticked here at TurnOpen and resolved at OnTurnEnd, so the tick and the re-arm can
// never collide inside one dispatch and silently shorten every fuse by one.
// The warning fires every single turn - the loudest possible statement that something is coming -
// and the player always gets at least one turn of it at every tier.
EncScript_Groudon_FuseTick:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Groudon_FuseTick_Done   @ inert (Phase 3 reset)
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Groudon_FuseTick_Warn
	encsubvar 3, 1
EncScript_Groudon_FuseTick_Warn:
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Groudon_FuseTick_Far
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Groudon_FuseTick_Near
	printstring STRINGID_ENCGROUDONPREPARING
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Groudon_FuseTick_Near:
	printstring STRINGID_ENCGROUDONCRACK
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Groudon_FuseTick_Far:
	printstring STRINGID_ENCGROUDONTREMBLE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Groudon_FuseTick_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the two countdowns need seeding: the first
// natural rise and the first quake both land on turn 3.
// Drought has already put a five-turn sun up by now; removeweather first is what lets encsetweather
// (which does nothing when that weather is already in force) restate it as permanent.
EncScript_Groudon_Intro::
	encsetvar 3, 4    @ Fuse: first quake at the end of turn 3
	encsetvar 4, 2    @ Grow: first natural rise on turn 3
	printstring STRINGID_ENCGROUDONAWAKENS
	waitmessage B_WAIT_TIME_LONG
	removeweather
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// TurnGuard is set immediately rather than at the tail so no branch out of this script can leave it
// re-armed. Every effect below is re-derived from Var(Land) outright, never accumulated.
EncScript_Groudon_TurnOpen::
	flushtextbox
	encsetvar 8, 1    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Groudon_TurnOpen_Done   @ Weakened: inert
	call EncScript_Groudon_Growth
	call EncScript_Groudon_ApplyGuard
	call EncScript_Groudon_LandCue
	call EncScript_Groudon_Scorch
	call EncScript_Groudon_FuseTick
	@ The collapse cadence, ticked here for the same reason the fuse is - CollapseTick re-arms it at
	@ OnTurnEnd and must not be able to eat its own re-arm.
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Groudon_TurnOpen_Unstable
	encsubvar 5, 1
EncScript_Groudon_TurnOpen_Unstable:
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Groudon_TurnOpen_Done
	printstring STRINGID_ENCGROUDONUNSTABLE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Groudon_TurnOpen_Done:
	return

// TurnClose owns nothing but the per-turn guard resets. Every countdown lives at TurnOpen instead,
// one checkpoint away from the OnTurnEnd resolutions that re-arm them.
EncScript_Groudon_TurnClose::
	encsetvar 8, 0     @ TurnGuard
	encsetvar 6, 0     @ Shift
	encsetvar 7, 0     @ Shaken
	encsetvar 10, 0    @ Sunlit
	return

// Anti-cheese. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at
// OnTurnStart - and the land climbs for the trouble, so paralysing the continent is not free.
// Leads with flushtextbox for the same reason TurnOpen does: this runs two priorities earlier.
EncScript_Groudon_WillNotBeShaken::
	flushtextbox
	encsetvar 7, 1    @ Shaken
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCGROUDONNOTSHAKEN
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Groudon_LandUp
	return

// The land refuses to be watered from the Primal Reversion on. Announced rather than silent so the
// refusal reads as Groudon doing something, instead of as the player's Rain Dance quietly ceasing
// to matter.
EncScript_Groudon_LandWillNotBeWatered::
	flushtextbox
	encsetvar 10, 1    @ Sunlit
	encsetweather BATTLE_WEATHER_SUN, 0
	printstring STRINGID_ENCGROUDONSUNRETURNS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return

// The three move-event triggers, all spending the one shared Var(Shift) charge. Whichever resolves
// first that turn is the only one that lands, in either direction - that cap is the point, not an
// over-shared guard: a Precipice Blades that beats the player's Surf does not just add a step, it
// locks the player's step out, and the turn nets +2 once natural growth is counted.
EncScript_Groudon_Erosion::
	encsetvar 6, 1    @ Shift
	call EncScript_Groudon_LandDown
	return

EncScript_Groudon_FeedFlame::
	encsetvar 6, 1    @ Shift
	printstring STRINGID_ENCGROUDONFEEDSFLAME
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Groudon_LandUp
	return

EncScript_Groudon_FeedBoss::
	encsetvar 6, 1    @ Shift
	call EncScript_Groudon_LandUp
	return

// The quake. Its magnitude is read off Var(Land) at the moment it lands, and so is its own re-arm -
// higher land means a harder quake AND a shorter fuse, which is the whole reason erosion is worth a
// turn. Re-arms to 4 at Land 0-2, 3 at Land 3-4 and 2 at Land 5, so PRIMAL LAND quakes every turn.
// The Speed drop is a stat STAGE, not a raw cut: a permanent Speed loss compounding every two turns
// for a whole fight is unrecoverable, while a stage drop is something Haze or a switch can answer.
// Every player-facing effect targets ENC_TARGET_ALL_FOES - the damage above it can KO, and a
// single-slot target asserts on a fainted battler.
EncScript_Groudon_QuakeResolve::
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Groudon_QuakeResolve_Break
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_QuakeResolve_Major
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Groudon_QuakeResolve_Quake
	printstring STRINGID_ENCGROUDONTREMOR
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MAGNITUDE
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	encsetvar 3, 4    @ Fuse
	return
EncScript_Groudon_QuakeResolve_Quake:
	printstring STRINGID_ENCGROUDONQUAKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EARTHQUAKE
	enchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Groudon_QuakeResolve_ArmLong
	encsetvar 3, 3    @ Fuse
	return
EncScript_Groudon_QuakeResolve_ArmLong:
	encsetvar 3, 4    @ Fuse
	return
	@ From here up the quake also strips the player's screens - the one command that can take a
	@ Reflect or Light Screen off outside a move. It takes a fail label rather than failing, so the
	@ same command is both "were there any?" and "remove them".
EncScript_Groudon_QuakeResolve_Major:
	printstring STRINGID_ENCGROUDONMAJORQUAKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EARTHQUAKE
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encsetvar 3, 3    @ Fuse
	goto EncScript_Groudon_QuakeResolve_Strip
EncScript_Groudon_QuakeResolve_Break:
	printstring STRINGID_ENCGROUDONBREAK
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FISSURE
	enchangehp ENC_TARGET_ALL_FOES, -28, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encsetvar 3, 2    @ Fuse
EncScript_Groudon_QuakeResolve_Strip:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Groudon_QuakeResolve_Done
	printstring STRINGID_ENCGROUDONSTRIPPED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Groudon_QuakeResolve_Done:
	return

// Phase 2's clock: the continent Groudon spent the fight building caves in on a fixed cadence,
// taking Groudon down with the player's party. Re-arms to 3, so one collapse every two turns.
// The snapshot branch is not optional. enchangehp writes passiveHpUpdate directly rather than going
// through the damage path where the Survive: clamp lives, so an unguarded tick could take Groudon
// to 0, faint it, and end the fight as an ordinary win with no catch window ever opening. Guarding
// it turns that hole into the phase's finisher: the last collapse breaks Groudon deterministically
// at 8%, which is inside the 10% catch window, and EncScript_Groudon_Weakened picks it up in this
// same dispatch.
EncScript_Groudon_CollapseTick::
	printstring STRINGID_ENCGROUDONCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FISSURE
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_SET
	encjumpifvar CMP_GREATER_THAN, 9, 15, EncScript_Groudon_CollapseTick_Grind
	encsetvar 9, 8
	encrewindhp ENC_TARGET_BOSS, 9
	goto EncScript_Groudon_CollapseTick_Party
EncScript_Groudon_CollapseTick_Grind:
	enchangehp ENC_TARGET_BOSS, -5, ENC_AMOUNT_PERCENT
EncScript_Groudon_CollapseTick_Party:
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	encsetvar 5, 3    @ Collapse
	return

// 50% - PRIMAL REVERSION. SPECIES_GROUDON_PRIMAL is Ground/Fire, so the matchup shifts mid-fight:
// Water goes to 4x and is clamped back to 2x by CapTypeEffectiveness (exactly what that property is
// for), Grass falls to neutral, and Fire becomes resisted - the player's erosion tool stays
// best-in-class while their Grass coverage quietly stops being an answer.
// The Ability: ABILITY_DROUGHT override survives this for free, because RecalcBattlerStats re-
// applies it after every form change. That is the whole reason it is an override: Primal Groudon's
// real Desolate Land would make Water moves fail outright and delete the player's only erosion
// lever at the halfway mark.
// Eruption's power scales with Groudon's remaining HP, so at 50% it is already halved and by Phase 2
// nearly nothing - its offense would collapse exactly when it is supposed to become terrifying.
// Fire Blast is flat, and "it stops radiating and starts firing" is the right read for the form.
// From here the natural cadence goes from one step every three turns to one step every turn.
EncScript_Groudon_PrimalReversion::
	encsetvar 1, 1    @ Phase
	printstring STRINGID_ENCGROUDONPRIMAL
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_GROUDON_PRIMAL, EncScript_Groudon_PrimalReversion_NoForm, B_ANIM_PRIMAL_REVERSION
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 25, ENC_AMOUNT_PERCENT
EncScript_Groudon_PrimalReversion_NoForm:
	encsetmove ENC_TARGET_BOSS, 1, MOVE_FIRE_BLAST
	encsetvar 4, 0    @ Grow: the land climbs every turn from here
	printstring STRINGID_ENCGROUDONANCIENTPOWER
	waitmessage B_WAIT_TIME_LONG
	return

// 25% - CONTINENTAL COLLAPSE. No stat bump: Groudon is not getting stronger here, it is being
// crushed. Survive is set here rather than in Properties: so the AI reads Groudon as killable for
// three quarters of the fight, and from this point it guarantees the player reaches the catch
// window instead of losing it to one oversized hit. The Immunities: list closes the fixed-damage,
// Perish Song and Destiny Bond holes Survive does not.
// The landmass ladder, the natural growth and the quake fuse all keep running underneath, so
// "survive" is not "wait" - at PRIMAL LAND that is 12% scorch plus a 28% quake every turn on top of
// the collapse, and the only way to make it survivable is to keep eroding.
EncScript_Groudon_ContinentalCollapse::
	encsetvar 1, 2    @ Phase
	printstring STRINGID_ENCGROUDONCOLLAPSEBEGINS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FISSURE
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encsetvar 5, 2    @ Collapse: the first collapse lands one turn from now
	return

// 10% - the catch window. The sun going out is the visual confirmation that the mechanic is over,
// and reverting guarantees the caught Pokemon is a plain Groudon. removeweather runs after the
// revert because the macro's trailing switchinabilities fires the Drought override on the way
// through; the sun-re-assertion trigger is gated to Phase <= 2 and so stays out of it from here.
// Clearing every landmass effect is trivial precisely because all of them were derived rather than
// accumulated. From this point the engine's automatic catch-window damage guard keeps the catch
// target alive; encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Groudon_Weakened::
	encsetvar 1, 3    @ Phase
	encformchange ENC_TARGET_BOSS, SPECIES_GROUDON, EncScript_Groudon_Weakened_Release, B_ANIM_PRIMAL_REVERSION
EncScript_Groudon_Weakened_Release:
	removeweather
	encsetvar 0, 0    @ Land
	encsetvar 2, 0    @ LastLand
	encsetvar 3, 0    @ Fuse
	encsetvar 4, 0    @ Grow
	encsetvar 5, 0    @ Collapse
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 20
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCGROUDONWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
