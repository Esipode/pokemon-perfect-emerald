// Rayquaza, "The Sky Guardian". Var indices, pinned by the
// always-true Conditions on EncScript_Rayquaza_Intro:
// 0 Phase (0 Sky Guardian / 1 Delta Ascension / 2 Atmosphere Breaks / 3 Weakened),
// 1 Alt (0 Grounded, 1 Low, 2 High, 3 Stratospheric), 2 Press (Atmospheric Pressure, 0-5),
// 3 Fall (Skyfall state: 0 idle, 2 gone, 1 descending), 4 Climb (climb cadence countdown),
// 5 Chaos (Phase 2 weather-roll cadence), 6 TurnGuard, 7 Cycle (per-turn guard shared by the three
// altitude/Skyfall OnTurnStart triggers), 8 Guarded (per-turn guard for SkyGuard), 9 Vent (per-turn
// guard shared by the two weather triggers), 10 Shot (per-turn guard for ShootDown), 11 HpMark
// (boss HP% at turn open; ENC_SNAP_DAMAGE turns it into "lost this turn"), 12 LastPress (Pressure
// tier last ANNOUNCED - the re-fire latch), 13 Pred (encstoreprediction scratch).
//
// Rayquaza is a fight over a POSITION rather than a number. Altitude sets how little damage it
// takes and how fast Atmospheric Pressure builds; full Pressure means it vanishes and Skyfall lands
// on the player. Two levers pull it back down - put weather on the field and it must descend to
// tear the atmosphere apart (but it descends angry, and the anger stacks), or land enough damage in
// one turn to shoot it out of the air. Grounded, its clock is stopped and it is finally hittable.
// Mega Evolution at 50% locks the sky with Strong Winds and confiscates the weather lever; the
// collapse at 22% hands weather back, hostile to Rayquaza too.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction. The altitude and phase lines already announce every change to
// it, so unlike Jirachi's ladder this one needs no callout of its own - the player is told the
// moment Rayquaza moves, every time it moves.
// Mega Rayquaza gains 100/100 defences over base 90/90, so its rungs sit 1-2 points lower to keep
// grounded damage roughly flat across the transition. The Phase 2 collapse is a real ~3x swing.
EncScript_Rayquaza_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rayquaza_GuardBroken
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rayquaza_GuardMega
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Rayquaza_Guard87
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Rayquaza_Guard89
	goto EncScript_Rayquaza_Guard92
EncScript_Rayquaza_GuardMega:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Rayquaza_Guard85
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Rayquaza_Guard88
	goto EncScript_Rayquaza_Guard91
EncScript_Rayquaza_GuardBroken:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Rayquaza_Guard76
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_78
	goto EncScript_Rayquaza_Guard80
EncScript_Rayquaza_Guard76:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	return
EncScript_78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
EncScript_Rayquaza_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Rayquaza_Guard85:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Rayquaza_Guard87:
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return
EncScript_Rayquaza_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Rayquaza_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Rayquaza_Guard91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Rayquaza_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return

// The tail both levers and the Skyfall landing share. Evasion is SET, not nudged: encchangestat
// clamps at the stage bounds, so -12 then +6 lands on neutral from wherever the stage had drifted -
// immune to Haze, Defog and the player's own accuracy drops.
EncScript_Rayquaza_Descend:
	encsetvar 1, 0    @ Alt: Grounded
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6
	call EncScript_Rayquaza_ApplyGuard
	printstring STRINGID_ENCRAYQUAZAGROUNDED
	waitmessage B_WAIT_TIME_LONG
	return

// The Pressure callout, latched on the tier last announced rather than on the raw value, so it
// fires on every real change and never drones. Skyfall zeroes Press, which drops the latch back to
// tier 0 through the leading branch here - so the whole build-up re-announces on the next cycle
// instead of being a one-shot the player can miss.
EncScript_Rayquaza_PressureCue:
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Rayquaza_PressureCue_Two
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_Rayquaza_PressureCue_One
	encsetvar 12, 0
	return
EncScript_Rayquaza_PressureCue_One:
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Rayquaza_PressureCue_Done
	encsetvar 12, 1
	printstring STRINGID_ENCRAYQUAZAPRESSUREONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Rayquaza_PressureCue_Two:
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Rayquaza_PressureCue_Done
	encsetvar 12, 2
	printstring STRINGID_ENCRAYQUAZAPRESSURETWO
	waitmessage B_WAIT_TIME_SHORT
EncScript_Rayquaza_PressureCue_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the climb counter needs seeding. Starting it at
// 2 means Rayquaza spends two turns on the ground before it first lifts away, which is the window
// the player is meant to learn the fight in. Air Lock announces itself on entry unprompted, and
// that stock line is the first hint that weather is not going to behave.
EncScript_Rayquaza_Intro::
	encsetvar 4, 2    @ Climb: first climb lands on turn 3
	printstring STRINGID_ENCRAYQUAZAAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// It owns the four per-turn re-entry guards outright. TurnClose deliberately clears none of them -
// Var(Shot) gates an OnTurnEnd trigger, and clearing it from the priority-90 close would make that
// trigger eligible again inside the same dispatch.
EncScript_Rayquaza_TurnOpen::
	flushtextbox
	encsetvar 6, 1     @ TurnGuard
	encsetvar 7, 0     @ Cycle
	encsetvar 8, 0     @ Guarded
	encsetvar 9, 0     @ Vent
	encsetvar 10, 0    @ Shot
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_SET     @ the mark ShootDown reads at turn close
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Rayquaza_TurnOpen_Done   @ Weakened: inert
	@ Atmospheric Pressure income. Grounded pays nothing, which is the whole reward for pulling it
	@ down: the Skyfall clock simply stops. Stratospheric pays nothing either - it is already gone.
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Rayquaza_TurnOpen_Low
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Rayquaza_TurnOpen_High
	goto EncScript_Rayquaza_TurnOpen_Cue
EncScript_Rayquaza_TurnOpen_High:
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Rayquaza_TurnOpen_HighFast
EncScript_Rayquaza_TurnOpen_Low:
	encaddvar 2, 1
	goto EncScript_Rayquaza_TurnOpen_Cue
EncScript_Rayquaza_TurnOpen_HighFast:
	encaddvar 2, 2
EncScript_Rayquaza_TurnOpen_Cue:
	call EncScript_Rayquaza_PressureCue
	@ The recurring out-of-reach line, and the sleep line that replaces it. Nothing in this fight is
	@ an ACTION - a sleeping Rayquaza still climbs, still builds Pressure and still lands Skyfall on
	@ your head, because none of that is a move. That is this encounter's anti-cheese, and one line
	@ makes the point without stating it.
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Rayquaza_TurnOpen_Done
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Rayquaza_TurnOpen_Done
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Rayquaza_TurnOpen_Drifts
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Rayquaza_TurnOpen_Done
	printstring STRINGID_ENCRAYQUAZAOUTOFREACH
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Rayquaza_TurnOpen_Done
EncScript_Rayquaza_TurnOpen_Drifts:
	printstring STRINGID_ENCRAYQUAZADRIFTS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Rayquaza_TurnOpen_Done:
	return

// Every countdown lives here, one checkpoint away from the OnTurnStart triggers that resolve them,
// so a real turn always passes between a timer ticking and the beat it arms.
// While Rayquaza is gone nothing else ticks: the Skyfall step is the only state change that turn.
EncScript_Rayquaza_TurnClose::
	encsetvar 6, 0    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Rayquaza_TurnClose_Done
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Rayquaza_TurnClose_Falling
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Rayquaza_TurnClose_Chaos
	encsubvar 4, 1
EncScript_Rayquaza_TurnClose_Chaos:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Rayquaza_TurnClose_Done
	encsubvar 5, 1
	goto EncScript_Rayquaza_TurnClose_Done
	@ 2 -> 1 is what makes SkyfallLand eligible next turn; SkyfallBegin writes 2 and SkyfallLand is
	@ gated on 1, so no dispatch can run both.
EncScript_Rayquaza_TurnClose_Falling:
	encsetvar 3, 1
EncScript_Rayquaza_TurnClose_Done:
	return

// One rung up the ladder. Sets Var(Cycle) because in Delta Ascension the re-arm value is 0 by
// design (it climbs every turn) - without the shared guard this trigger's own condition would still
// be true on re-evaluation and it would climb to the ceiling inside a single dispatch.
EncScript_Rayquaza_Climb::
	encsetvar 7, 1    @ Cycle
	encaddvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 7    @ absolute +1, not a nudge
	call EncScript_Rayquaza_ApplyGuard
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Rayquaza_Climb_High
	printstring STRINGID_ENCRAYQUAZACLIMBSLOW
	goto EncScript_Rayquaza_Climb_Arm
EncScript_Rayquaza_Climb_High:
	printstring STRINGID_ENCRAYQUAZACLIMBSHIGH
EncScript_Rayquaza_Climb_Arm:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	@ Cadence by phase: every 3rd turn as the Sky Guardian, EVERY turn in Delta Ascension, every 2nd
	@ once the atmosphere breaks.
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rayquaza_Climb_ArmFast
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rayquaza_Climb_ArmMid
	encsetvar 4, 2
	return
EncScript_Rayquaza_Climb_ArmMid:
	encsetvar 4, 1
	return
EncScript_Rayquaza_Climb_ArmFast:
	encsetvar 4, 0
	return

// High Rayquaza reads the player and slips the attack it expects. encstoreprediction writes the
// predicted CATEGORY: 0 no read / 1 physical / 2 special / 3 status. A predicted special move is
// never evaded, which is the rule the player can actually find - special attacks always reach it,
// physical ones can be read. With no read at all it falls back to a flat roll so the sky is never
// simply free.
// encsetprotect blocks everything, so a player whose strongest option is physical but who fires a
// special move that turn can still be stopped; the line claims Rayquaza read their INTENT, which is
// exactly what the prediction is.
EncScript_Rayquaza_SkyGuard::
	encsetvar 8, 1    @ Guarded
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 13
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Rayquaza_SkyGuard_Done
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Rayquaza_SkyGuard_Evade
	encjumpifchance 35, EncScript_Rayquaza_SkyGuard_Evade
EncScript_Rayquaza_SkyGuard_Done:
	return
EncScript_Rayquaza_SkyGuard_Evade:
	printstring STRINGID_ENCRAYQUAZAREADSYOU
	waitmessage B_WAIT_TIME_SHORT
	encsetprotect ENC_TARGET_BOSS
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	return

// Skyfall, turn one. encsetprotect is only meaningful at OnTurnStart (gProtectStructs is wiped
// after end-of-turn effects), which is exactly where this runs, and it expires on its own;
// encsetrecharge 1 here costs Rayquaza THIS turn's action. Between them the turn is mutually dead,
// which is the honest expression of "it is not on the field" - no command removes a battler, and
// the one animation that genuinely slides a sprite away has nothing that slides it back.
EncScript_Rayquaza_SkyfallBegin::
	encsetvar 7, 1    @ Cycle
	printstring STRINGID_ENCRAYQUAZASKYFALLWARN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	printstring STRINGID_ENCRAYQUAZAVANISHED
	waitmessage B_WAIT_TIME_LONG
	encsetvar 2, 0    @ Press
	encsetvar 3, 2    @ Fall: gone
	encsetvar 1, 3    @ Alt: Stratospheric
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 7
	call EncScript_Rayquaza_ApplyGuard
	encsetprotect ENC_TARGET_BOSS
	encsetrecharge ENC_TARGET_BOSS, 1
	return

// Skyfall, turn two - the hardest turn in the fight and the best one. Rayquaza is NOT protected and
// NOT recharging on the way down, so its move and the player's both resolve on top of the scripted
// hit; and it ends standing on the ground with its clock at zero. The Skyfall is how you get your
// turn, which is the whole symmetry of the encounter.
// ENC_TARGET_ALL_FOES rather than a single slot: a single-slot target asserts on a fainted battler,
// a group target skips the absent silently.
EncScript_Rayquaza_SkyfallLand::
	encsetvar 7, 1    @ Cycle
	encsetvar 3, 0    @ Fall: idle
	printstring STRINGID_ENCRAYQUAZASKYFALLCOMING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SKY_ATTACK
	printstring STRINGID_ENCRAYQUAZASKYFALL
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rayquaza_SkyfallLand_Delta
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rayquaza_SkyfallLand_Late
	enchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Rayquaza_SkyfallLand_Down
EncScript_Rayquaza_SkyfallLand_Delta:
	enchangehp ENC_TARGET_ALL_FOES, -40, ENC_AMOUNT_PERCENT
	goto EncScript_Rayquaza_SkyfallLand_Down
EncScript_Rayquaza_SkyfallLand_Late:
	enchangehp ENC_TARGET_ALL_FOES, -33, ENC_AMOUNT_PERCENT
EncScript_Rayquaza_SkyfallLand_Down:
	call EncScript_Rayquaza_Descend
	return

// Lever 1. Air Lock means the player's weather never DID anything, so setting it is purely a lure -
// and the Air Lock entry message already told them so on turn one. The stat stages are the price
// and they STACK; MAX_STAT_STAGE caps them for free, so nothing needs tracking. Pressure is
// untouched: the reward for grounding it is that the clock stops, which is quieter and stronger
// than any number.
EncScript_Rayquaza_Vent::
	encsetvar 9, 1    @ Vent
	printstring STRINGID_ENCRAYQUAZATEARS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	removeweather
	call EncScript_Rayquaza_Descend
	printstring STRINGID_ENCRAYQUAZAENOUGH
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	return

// Lever 1b. Bring your own Kyogre or Groudon and Rayquaza silences it personally, for double the
// anger. Primordial Sea and Desolate Land re-apply on switch-in, so pivoting the setter back in
// re-triggers this - a learnable trap rather than an exploit, and the only weather the player
// cannot normally lose.
EncScript_Rayquaza_VentPrimal::
	encsetvar 9, 1    @ Vent
	printstring STRINGID_ENCRAYQUAZACONFLICT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	removeweather
	call EncScript_Rayquaza_Descend
	printstring STRINGID_ENCRAYQUAZAENOUGH
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	return

// Lever 2. HpMark was written at turn open with ENC_SNAP_SET; ENC_SNAP_DAMAGE turns it into
// "percentage of max HP lost this turn", cumulative across multi-hits, chip and status.
// The guard ladder makes the threshold self-balancing: 6% through an 87 guard needs a raw hit worth
// about 46% of max HP, through a 92 guard about 75%. It gets harder the higher Rayquaza is, which
// is exactly right.
EncScript_Rayquaza_ShootDown::
	encsetvar 10, 1    @ Shot
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_DAMAGE, EncScript_Rayquaza_ShootDown_None
	encjumpifvar CMP_LESS_THAN, 11, 6, EncScript_Rayquaza_ShootDown_None
	printstring STRINGID_ENCRAYQUAZASHOTDOWN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	call EncScript_Rayquaza_Descend
EncScript_Rayquaza_ShootDown_None:
	return

// Phase 2's broken sky. Uniform 1-in-5 across the five outcomes (20 / 25 / 33 / 50 / fall-through).
// Air Lock left with the Mega, so this is the only phase where weather is mechanically live - and
// Rayquaza is Dragon/Flying with no sand or hail immunity, so the atmosphere finally chips it too.
// Each roll is announced, and each overwrites whatever the player set, which is the emergent way of
// saying "you do not control the sky".
EncScript_Rayquaza_WeatherChaos::
	encsetvar 5, 2    @ Chaos: two-turn cadence
	encjumpifchance 20, EncScript_Rayquaza_Chaos_Rain
	encjumpifchance 25, EncScript_Rayquaza_Chaos_Sun
	encjumpifchance 33, EncScript_Rayquaza_Chaos_Sand
	encjumpifchance 50, EncScript_Rayquaza_Chaos_Hail
	printstring STRINGID_ENCRAYQUAZACHAOSCLEAR
	waitmessage B_WAIT_TIME_SHORT
	removeweather
	return
EncScript_Rayquaza_Chaos_Rain:
	printstring STRINGID_ENCRAYQUAZACHAOSRAIN
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return
EncScript_Rayquaza_Chaos_Sun:
	printstring STRINGID_ENCRAYQUAZACHAOSSUN
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return
EncScript_Rayquaza_Chaos_Sand:
	printstring STRINGID_ENCRAYQUAZACHAOSSAND
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_SANDSTORM, 0
	playanimation BS_OPPONENT1, B_ANIM_SANDSTORM_CONTINUES
	return
EncScript_Rayquaza_Chaos_Hail:
	printstring STRINGID_ENCRAYQUAZACHAOSHAIL
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_HAIL, 0
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	return

// 50% - DELTA ASCENSION. The form change does three things for free, and each does real work:
// both species are 105 base HP so the health bar does not jump; the macro's trailing
// switchinabilities fires Delta Stream, whose Strong Winds are primal and so refuse every
// subsequent weather - the player's grounding lever is confiscated on screen by a stock message;
// and Air Lock leaves, which is what makes the Phase 2 collapse land later.
// Strong Winds also neutralises Flying's own weaknesses, so the Ice/Rock/Electric matchups soften
// on top of the guard - CapTypeEffectiveness had already clamped the 4x Ice weakness to 2x, and
// this takes it to 1x. Ice-stacking a Mega Rayquaza is meant to stop working.
// Dragon Ascent is written in only now: knowing it beforehand would have satisfied CanMegaEvolve
// and let the AI spend a turn-1 gimmick, but once the species already IS the Mega the check can no
// longer fire.
EncScript_Rayquaza_DeltaAscension::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCRAYQUAZAABSORBS
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCRAYQUAZADELTAASCENSION
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_RAYQUAZA_MEGA, EncScript_Rayquaza_DeltaAscension_NoForm
	encsetmove ENC_TARGET_BOSS, 0, MOVE_DRAGON_ASCENT
EncScript_Rayquaza_DeltaAscension_NoForm:
	encsetvar 4, 0    @ Climb: every turn from here
	call EncScript_Rayquaza_ApplyGuard
	return

// 22% - THE ATMOSPHERE BREAKS. RemoveAllWeather clears primal weather unconditionally and Delta
// Stream is a switch-in ability with no per-turn re-application, so Strong Winds stay gone.
// Survive is set here so an oversized hit cannot skip the catch window at 10%; it is paired with
// the Immunities: list, which closes the fixed-damage / Perish Song / Destiny Bond holes Survive
// does not cover.
EncScript_Rayquaza_AtmosphereBreaks::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCRAYQUAZACONTROLFAILING
	waitmessage B_WAIT_TIME_LONG
	removeweather
	encsetvar 4, 1    @ Climb: every other turn
	encsetvar 5, 1    @ Chaos: first roll next turn
	encsetsurvive ENC_TARGET_BOSS, TRUE
	call EncScript_Rayquaza_ApplyGuard
	return

// 10% - the catch window. Reverting to base Rayquaza is the guideline cue and housekeeping at once:
// it guarantees the caught Pokemon is a plain Rayquaza whatever the form-change table does, and
// 105 = 105 base HP means the bar still does not move.
// Slot 0 is restored to Extreme Speed BEFORE the revert, and that is load-bearing rather than
// tidiness: a base Rayquaza that merely knows Dragon Ascent satisfies CanMegaEvolve, and the AI
// would Mega Evolve again in the middle of the player's catch attempts.
// Every altitude, Pressure, Skyfall and chaos trigger is gated Var(Phase) <= 2, so the whole
// machine goes quiet here. From this point the engine's automatic catch-window damage guard keeps
// the catch target alive; encsetdamagereduction sets the value that guard will restore, not the
// live one.
EncScript_Rayquaza_Weakened::
	encsetvar 0, 3    @ Phase
	printstring STRINGID_ENCRAYQUAZAGIVESOUT
	waitmessage B_WAIT_TIME_LONG
	encsetmove ENC_TARGET_BOSS, 0, MOVE_EXTREME_SPEED
	encformchange ENC_TARGET_BOSS, SPECIES_RAYQUAZA, EncScript_Rayquaza_Weakened_Release, B_ANIM_FORM_CHANGE
EncScript_Rayquaza_Weakened_Release:
	removeweather
	encsetvar 1, 0    @ Alt
	encsetvar 2, 0    @ Press
	encsetvar 3, 0    @ Fall
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCRAYQUAZAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
