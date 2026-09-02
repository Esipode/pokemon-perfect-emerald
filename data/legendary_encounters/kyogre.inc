// Kyogre, "The Endless Ocean". Var indices, pinned by the
// always-true Conditions on EncScript_Kyogre_Intro:
// 0 Tide (0-5, the water level everything else derives from), 1 Phase (0 Rising Waters / 1 Primal
// Awakening / 2 Great Deluge / 3 Weakened), 2 LastTide (tide tier last ANNOUNCED - the re-fire
// latch), 3 Rise (natural cadence countdown), 4 Mark (Undertow countdown, 0 or 3-1), 5 Switched
// (per-turn guard for the Undertow bail), 6 Deluge (0 idle, 3-1 charging, 5 just resolved, 4 spent),
// 7 Exhaust (exhaustion window countdown), 8 TurnGuard, 9 Cycle (per-turn guard shared by the two
// Deluge clock triggers), 10 Flux (per-turn guard shared by the three move-event tide triggers, in
// BOTH directions), 11 Woke (per-turn guard for the anti-cheese trigger).
//
// Kyogre is a fight over a WATER LEVEL, and unlike every mechanic before it the level moves in both
// directions. The Tide rises on its own, rises when Kyogre lands a Water move, and rises when Fire
// touches it; it falls to Electric and Grass damage, and to holding your ground under the Undertow
// instead of switching away. High Tide raises Kyogre's damage reduction, drowns the player's whole
// side every turn, swamps that side's Speed at Tide 4, and arms the Undertow.
// Only ONE move-event tide change lands per turn (Var(Flux)), in either direction, so losing the
// Speed race is losing the Tide race - and Tide 4 quarters the player's Speed.
// Everything derived from the Tide is RE-DERIVED at TurnOpen rather than accumulated, so a tide
// that runs 0-3-1-4-2 leaves no residue and nothing has to remember how to undo the last tier.

// --- Shared subroutines (call/return) ---

// Sole owner of the tide's damage reduction ladder. TurnOpen skips this entirely while the
// exhaustion window is open, which is what lets the flat 55 stand over whatever the tide is doing.
EncScript_Kyogre_ApplyGuard:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Kyogre_Guard92
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyogre_Guard90
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Kyogre_Guard88
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Kyogre_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Kyogre_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Kyogre_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return

// Every rise in the fight goes through here. encaddvar has no ceiling of its own, so the clamp is a
// guarded branch rather than a bare add.
EncScript_Kyogre_TideUp:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Kyogre_TideUp_Done
	encaddvar 0, 1
	printstring STRINGID_ENCKYOGRETIDERISES
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
EncScript_Kyogre_TideUp_Done:
	return

// Every ebb goes through here. encsubvar on a u8 underflows to 255, so 0 is tested for explicitly;
// the Phase 2 floor of 3 is the second branch - from the Great Deluge on the player can still push
// the water back, just never to calm, and the Deluge check is set at exactly that floor.
EncScript_Kyogre_TideDown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Kyogre_TideDown_Done
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Kyogre_TideDown_Fall
	encjumpifvar CMP_LESS_THAN, 0, 4, EncScript_Kyogre_TideDown_Done
EncScript_Kyogre_TideDown_Fall:
	encsubvar 0, 1
	printstring STRINGID_ENCKYOGRETIDEFALLS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Kyogre_TideDown_Done:
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones. Dropping back to Tide 0 resets the latch
// through the leading branch, so the whole climb re-announces if the water rises again.
EncScript_Kyogre_TideCue:
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Kyogre_TideCue_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyogre_TideCue_Four
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyogre_TideCue_Three
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Kyogre_TideCue_Two
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Kyogre_TideCue_One
	encsetvar 2, 0
	return
EncScript_Kyogre_TideCue_One:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCKYOGRETIDEONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Kyogre_TideCue_Two:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCKYOGRETIDETWO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Kyogre_TideCue_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 3
	printstring STRINGID_ENCKYOGRETIDETHREE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Kyogre_TideCue_Four:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 4
	printstring STRINGID_ENCKYOGRETIDEFOUR
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Kyogre_TideCue_Five:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 5
	printstring STRINGID_ENCKYOGRETIDEFIVE
	waitmessage B_WAIT_TIME_LONG
EncScript_Kyogre_TideCue_Done:
	return

// Drowning chip, from Flood upward. ENC_TARGET_ALL_FOES rather than a single slot: this can KO, and
// a single-slot target asserts on a fainted battler while a group target skips the absent silently.
EncScript_Kyogre_Drown:
	encjumpifvar CMP_LESS_THAN, 0, 3, EncScript_Kyogre_Drown_Done
	printstring STRINGID_ENCKYOGREDROWNING
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_SURF
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Kyogre_Drown_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyogre_Drown_Four
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Kyogre_Drown_Four:
	enchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Kyogre_Drown_Five:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Kyogre_Drown_Done:
	return

// The Undertow. Armed from the Primal Reversion on, whenever the water reaches Tide 4.
// Both the marking turn and the two that follow pay the same price: raw Sp. Def erosion, which is
// permanent and survives Haze and switching, on top of the drowning chip already dealt above.
// Surviving all three turns is worth one tide step (EncScript_Kyogre_TurnClose); switching away
// costs one instead, so bailing is a two-step swing against the player.
EncScript_Kyogre_Undertow:
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Kyogre_Undertow_Pull
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Kyogre_Undertow_Done
	encjumpifvar CMP_LESS_THAN, 0, 4, EncScript_Kyogre_Undertow_Done
	encsetvar 4, 3
	printstring STRINGID_ENCKYOGREUNDERTOWMARK
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_WHIRLPOOL
	goto EncScript_Kyogre_Undertow_Drag
EncScript_Kyogre_Undertow_Pull:
	printstring STRINGID_ENCKYOGREUNDERTOWPULL
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_WHIRLPOOL
EncScript_Kyogre_Undertow_Drag:
	encchangestatvalue ENC_TARGET_ALL_FOES, STAT_SPDEF, -12, ENC_AMOUNT_PERCENT
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
EncScript_Kyogre_Undertow_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the cadence counter needs seeding. Rise counts
// DOWN to 0 and rises the tide on the turn it is already 0, so a re-arm of 2 is a rise every third
// turn - two calm turns to read the fight before the water starts moving.
EncScript_Kyogre_Intro::
	encsetvar 3, 2    @ Rise: first natural rise at the end of turn 3
	printstring STRINGID_ENCKYOGREAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// It owns all five per-turn re-entry guards. Clearing them here rather than at TurnClose is what
// keeps the priority-5 anti-cheese trigger and the OnTurnEnd Deluge triggers out of reach of their
// own dispatch - and Var(Switched) has to be cleared before the player's switch action resolves,
// which is after this checkpoint.
// Every effect below is re-derived from Var(Tide) outright, never accumulated.
EncScript_Kyogre_TurnOpen::
	flushtextbox
	encsetvar 8, 1     @ TurnGuard
	encsetvar 9, 0     @ Cycle
	encsetvar 10, 0    @ Flux
	encsetvar 11, 0    @ Woke
	encsetvar 5, 0     @ Switched
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyogre_TurnOpen_Done   @ Weakened: inert
	@ The exhaustion window replaces the whole tide ladder - the flat 55 stands regardless of what
	@ the water is doing, and it is the only real damage window in the last quarter of the fight.
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Kyogre_TurnOpen_Reeling
	call EncScript_Kyogre_ApplyGuard
	goto EncScript_Kyogre_TurnOpen_Swamp
EncScript_Kyogre_TurnOpen_Reeling:
	printstring STRINGID_ENCKYOGREREELING
	waitmessage B_WAIT_TIME_SHORT
	@ The submerged battlefield. Swamp quarters the player's side's Speed
	@ (GetBattlerTotalSpeedStat), which feeds straight back into the Flux race the whole fight turns
	@ on: falling behind makes falling further behind easier. Set or cleared outright every turn, so
	@ it lifts the moment the water drops back below Tide 4.
EncScript_Kyogre_TurnOpen_Swamp:
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_Kyogre_TurnOpen_Swamped
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
	goto EncScript_Kyogre_TurnOpen_Cue
EncScript_Kyogre_TurnOpen_Swamped:
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
EncScript_Kyogre_TurnOpen_Cue:
	call EncScript_Kyogre_TideCue
	call EncScript_Kyogre_Drown
	call EncScript_Kyogre_Undertow
EncScript_Kyogre_TurnOpen_Done:
	return

// Every countdown lives here, one checkpoint away from the OnTurnStart triggers that resolve them,
// so a real turn always passes between a timer ticking and the beat it arms.
EncScript_Kyogre_TurnClose::
	encsetvar 8, 0    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyogre_TurnClose_Done   @ Weakened: inert
	@ The natural cadence. It does NOT spend Var(Flux), so from the Primal Reversion on the water
	@ gains a step every turn for free and an ebb only ever buys the player a net zero - which is
	@ why holding the line, rather than draining the sea, is the whole Phase 2 game.
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Kyogre_TurnClose_Rise
	encsubvar 3, 1
	goto EncScript_Kyogre_TurnClose_Mark
EncScript_Kyogre_TurnClose_Rise:
	call EncScript_Kyogre_TideUp
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Kyogre_TurnClose_RiseSlow
	encsetvar 3, 0    @ Primal onward: every turn
	goto EncScript_Kyogre_TurnClose_Mark
EncScript_Kyogre_TurnClose_RiseSlow:
	encsetvar 3, 2    @ Rising Waters: every third turn
	@ The Undertow countdown. Reaching 0 from 1 is the payoff for holding position through all three
	@ marked turns.
EncScript_Kyogre_TurnClose_Mark:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Kyogre_TurnClose_Deluge
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Kyogre_TurnClose_Deluge
	printstring STRINGID_ENCKYOGREUNDERTOWHELD
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyogre_TideDown
	@ The spent-Deluge sentinel, walked down here rather than at TurnOpen so the turn after a
	@ resolution is a guaranteed turn of rest: 5 means "resolved this turn", 4 means "spent", and
	@ only 0 re-arms DelugeBegin. Clearing 4 waits on the exhaustion window running out.
EncScript_Kyogre_TurnClose_Deluge:
	encjumpifvar CMP_EQUAL, 6, 5, EncScript_Kyogre_TurnClose_Spent
	encjumpifvar CMP_NOT_EQUAL, 6, 4, EncScript_Kyogre_TurnClose_Exhaust
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Kyogre_TurnClose_Exhaust
	encsetvar 6, 0
	goto EncScript_Kyogre_TurnClose_Exhaust
EncScript_Kyogre_TurnClose_Spent:
	encsetvar 6, 4
EncScript_Kyogre_TurnClose_Exhaust:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Kyogre_TurnClose_Done
	encsubvar 7, 1
EncScript_Kyogre_TurnClose_Done:
	return

// Anti-cheese. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at
// OnTurnStart - and the water climbs for the trouble, so putting the sea to sleep is not free.
// Leads with flushtextbox for the same reason TurnOpen does: this runs one priority earlier.
EncScript_Kyogre_WillNotBeStilled::
	flushtextbox
	encsetvar 11, 1    @ Woke
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCKYOGRENOTSTILLED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyogre_TideUp
	return

// The three move-event tide triggers, all spending the one shared Var(Flux) charge. Whichever
// resolves first that turn is the only one that lands, in either direction - that cap is the point,
// not an over-shared guard: an Origin Pulse that beats the player's Thunderbolt does not just add a
// step, it locks the player's step out.
EncScript_Kyogre_Ebb::
	encsetvar 10, 1    @ Flux
	call EncScript_Kyogre_TideDown
	return

EncScript_Kyogre_SurgeFire::
	encsetvar 10, 1    @ Flux
	printstring STRINGID_ENCKYOGREANSWERSFLAME
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Kyogre_TideUp
	return

EncScript_Kyogre_SurgeWater::
	encsetvar 10, 1    @ Flux
	call EncScript_Kyogre_TideUp
	return

// Bailing out from under the Undertow. The three turns of Sp. Def erosion already paid are wasted,
// and the water gains the step the player was three turns from taking back.
EncScript_Kyogre_UndertowBail::
	encsetvar 5, 1    @ Switched
	encsetvar 4, 0    @ Mark
	printstring STRINGID_ENCKYOGREUNDERTOWFLED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyogre_TideUp
	return

// The Great Deluge, turn one of three. Var(Cycle) stops DelugeTick chasing this straight back down
// in the same dispatch.
EncScript_Kyogre_DelugeBegin::
	encsetvar 9, 1    @ Cycle
	encsetvar 6, 3    @ Deluge
	printstring STRINGID_ENCKYOGREDELUGEBEGINS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return

// Turns two and three, announced every turn so the player can count the wave down and read the
// resolution coming.
EncScript_Kyogre_DelugeTick::
	encsetvar 9, 1    @ Cycle
	encsubvar 6, 1
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Kyogre_DelugeTick_Near
	printstring STRINGID_ENCKYOGREDELUGECOUNT
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return
EncScript_Kyogre_DelugeTick_Near:
	printstring STRINGID_ENCKYOGREDELUGENEAR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return

// The wave breaks, and it resolves against the Tide rather than against the player's HP - the whole
// endgame is holding the water at the Phase 2 floor for three straight turns.
// Failing that costs 45% of the party's health: a full-health Pokemon lives it, one that has been
// drowning at Tide 5 does not, and that is the lesson.
// Holding it earns the only real damage window in the fight. encsetrecharge 2 at OnTurnEnd costs
// Kyogre its NEXT action (the timer is decremented after this checkpoint) and prints the stock
// "must recharge!" line; Exhaust is seeded at 3 because TurnClose spends one tick this same
// dispatch, leaving two full turns at reduction 76.
EncScript_Kyogre_DelugeResolve::
	encsetvar 6, 5    @ Deluge: resolved this turn
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_Kyogre_DelugeResolve_Hits
	printstring STRINGID_ENCKYOGREDELUGEFAILS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 7, 3    @ Exhaust
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encsetrecharge ENC_TARGET_BOSS, 2
	return
EncScript_Kyogre_DelugeResolve_Hits:
	printstring STRINGID_ENCKYOGREDELUGEHITS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ORIGIN_PULSE
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	encsetvar 0, 5    @ Tide: the sea takes the field
	return

// 50% - PRIMAL REVERSION. The form change carries the mechanic on its own: both forms are 100 base
// HP so the health bar does not jump, and the macro's trailing switchinabilities fires Primordial
// Sea, whose rain cannot be removed and makes Fire moves fail outright. The "Fire raises the tide"
// rule stops being a punishment and becomes a flat denial, which is why EncScript_Kyogre_SurgeFire
// is gated to Phase 0.
// From here the natural cadence goes from one step every three turns to one step every turn -
// everything the player was already doing still works, they just have to do it twice as fast.
EncScript_Kyogre_PrimalReversion::
	encsetvar 1, 1    @ Phase
	printstring STRINGID_ENCKYOGREPRIMAL
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_KYOGRE_PRIMAL, EncScript_Kyogre_PrimalReversion_NoForm, B_ANIM_PRIMAL_REVERSION
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 25, ENC_AMOUNT_PERCENT
EncScript_Kyogre_PrimalReversion_NoForm:
	encsetvar 3, 0    @ Rise: the sea climbs every turn from here
	printstring STRINGID_ENCKYOGREANCIENTPOWER
	waitmessage B_WAIT_TIME_LONG
	return

// 25% - THE GREAT DELUGE. Survive is set here rather than in Properties: so the AI reads Kyogre as
// killable for three quarters of the fight; from this point it guarantees the player actually
// reaches the 10% catch window instead of losing it to one oversized hit during an exhaustion turn.
// The Immunities: list closes the fixed-damage, Perish Song and Destiny Bond holes Survive does not.
// The tide is floored at 3 from here (EncScript_Kyogre_TideDown), and pushed up to that floor now if
// the player held it lower - "permanently high" and "bring the water down to survive" only coexist
// if the floor and the Deluge's own check sit at the same number.
EncScript_Kyogre_GreatDeluge::
	encsetvar 1, 2    @ Phase
	printstring STRINGID_ENCKYOGREBEYONDCONTROL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 15, ENC_AMOUNT_PERCENT
	encsetvar 6, 0    @ Deluge: arms the first charge next turn
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Kyogre_GreatDeluge_Done
	encsetvar 0, 3    @ Tide: floored
EncScript_Kyogre_GreatDeluge_Done:
	return

// 10% - the catch window. Reverting drops Primordial Sea, so the endless sea drains back to ordinary
// weather - the visual confirmation that the mechanic is over - and it guarantees the caught Pokemon
// is a plain Kyogre. removeweather runs after the revert because the macro's switchinabilities fires
// base Kyogre's Drizzle on the way through.
// Clearing every tide effect is trivial precisely because all of them were derived rather than
// accumulated. From this point the engine's automatic catch-window damage guard keeps the catch
// target alive; encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Kyogre_Weakened::
	encsetvar 1, 3    @ Phase
	encformchange ENC_TARGET_BOSS, SPECIES_KYOGRE, EncScript_Kyogre_Weakened_Release, B_ANIM_PRIMAL_REVERSION
EncScript_Kyogre_Weakened_Release:
	removeweather
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
	encsetvar 0, 0    @ Tide
	encsetvar 2, 0    @ LastTide
	encsetvar 4, 0    @ Mark
	encsetvar 6, 0    @ Deluge
	encsetvar 7, 0    @ Exhaust
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 20
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCKYOGREWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
