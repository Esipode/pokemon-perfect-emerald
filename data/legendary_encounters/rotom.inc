// Rotom, "The Possessive Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Rotom_Intro:
// 0 Phase (0 Idle Machine / 1 Rogue Program / 2 Total System Takeover / 3 Weakened), 1 Form (the
// rotation position, 1-5; survives an overload so the cycle does not restart at Heat), 2 Protocol
// (the LIVE form's per-turn behaviour, same values, 0 = out of form), 3 Clock (countdown to the next
// possession), 4 Overclock (0-5), 5 LastOC (last ANNOUNCED Overclock), 6 Hardened (the Overclock >= 3
// guard-bump latch), 7 Overload (window countdown; 0 = not in one), 8 Hijack (which hijack is next,
// 0-2), 9 HijackDue, 10 TurnGuard, 11 MoveGuard (the Electric lever's per-turn re-entry guard).
//
// Two numbers pulling against each other. FORM decides what fight this turn is; OVERCLOCK decides
// whether the player's damage means anything. Rotom raises the meter with everything it does, and
// the player can raise it themselves with Electric moves - the worst damage type in the game against
// a Rotom - to buy the overload window early. Neither pace is free.

// --- Shared subroutines (call/return) ---

// The SOLE owner of Rotom's damage reduction, so every writer - the overload, the reboot, the two
// phase transitions and the hardening - agrees on one number. An open overload window outranks
// everything else: without that first branch, a phase threshold crossed mid-window would slam the
// guard back up to 90/92 and close the window the player had already earned.
// Hardened is a flat 93 rather than a per-phase bump: by Phase 2 the base guard is already 92, and
// by then the fight is mostly window anyway.
EncScript_Rotom_ApplyGuard:
	encjumpifvar CMP_GREATER_THAN, 7, 0, EncScript_Rotom_ApplyGuard_Blown
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Rotom_ApplyGuard_Hard
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rotom_ApplyGuard_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rotom_ApplyGuard_P1
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Rotom_ApplyGuard_P1:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Rotom_ApplyGuard_P2:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Rotom_ApplyGuard_Hard:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
// The window's guard. Crossing into Phase 2 mid-window correctly deepens 55 to 40 rather than
// cancelling the window.
EncScript_Rotom_ApplyGuard_Blown:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rotom_ApplyGuard_Critical
	encsetdamagereduction ENC_TARGET_BOSS, 76
	return
EncScript_Rotom_ApplyGuard_Critical:
	encsetdamagereduction ENC_TARGET_BOSS, 72
	return

// RecalcBattlerStats rebuilds the battle stats from the new species on every encformchange, wiping
// anything encchangestatvalue wrote before it. So this has to be reapplied exactly once after every
// form change that ACTUALLY happened, and never on a path that did not change form - same contract
// as EncScript_Deoxys_Bulk. Rotom's forms are 50/65/107/107/105/86: a 50 base HP legendary at the
// level cap is short-lived even at guard 88, and the Overclock loop needs the fight to last.
EncScript_Rotom_Bulk:
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 60, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 60, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 40, ENC_AMOUNT_PERCENT
	return

// Every form's entry hands out stat stages, and five possessions of +1s would end with an untouchable
// +6 evasion machine. So each possession writes its form's stages ABSOLUTELY: everything back to
// neutral first, then the form's own. encchangestat clamps at the stage bounds, so -12 lands on
// neutral from wherever the stage had drifted.
EncScript_Rotom_Neutral:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6
	return

// The old protocol's field comes down before the new one goes up. Terrain is only ever Mow's, so the
// removal and its background repaint (the same pair BattleScript_TerrainEnds uses) are gated on it
// rather than run every possession. Tailwind belongs to Fan, not to the battle.
EncScript_Rotom_ClearField:
	removeweather
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encjumpifvar CMP_NOT_EQUAL, 2, 5, EncScript_Rotom_ClearField_Done
	removeterrain
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
EncScript_Rotom_ClearField_Done:
	return

// THE REBOOT - the anti-abuse rule, and deliberately not an immunity. Status lands on Rotom fully and
// buys real turns; it just gets flushed with the old form, so its shelf life is the possession clock
// (4 turns, then 2, then 1). A sleep landed right after a possession is worth three landed just
// before one, which is skill expression rather than a locked door.
EncScript_Rotom_Flush:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Rotom_Flush_Clear
	return
EncScript_Rotom_Flush_Clear:
	printstring STRINGID_ENCROTOMFLUSH
	waitmessage B_WAIT_TIME_SHORT
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	return

// The possession clock's re-arm. One higher than the gap it produces, because the resolution trigger
// fires at 1: 5 is a possession every fourth turn, 3 every second, 2 every turn. One place, so
// retuning the cadence is one literal per phase.
EncScript_Rotom_ClockArm:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rotom_ClockArm_Every
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rotom_ClockArm_Fast
	encsetvar 3, 5
	return
EncScript_Rotom_ClockArm_Fast:
	encsetvar 3, 3
	return
EncScript_Rotom_ClockArm_Every:
	encsetvar 3, 2
	return

// OVERCLOCK - the meter, and its announcer. Every raise in the fight routes through here so a rung is
// printed exactly once and every real change is printed: LastOC holds the last ANNOUNCED value, which
// is the "store it and compare" pattern a hidden mechanic needs. The meter is pinned at 5 rather than
// allowed to run past, so a turn that raises it three times still reads as one rung.
// The rung's animation is B_ANIM_STATS_CHANGE, never B_ANIM_TERA_CHARGE. Tera Charge is half of a
// pair: it blends the whole screen to white and only B_ANIM_TERA_ACTIVATE blends it back, so played
// bare from an encounter script it leaves the background and the move-selection box white for the
// rest of the battle. Stats Change is also the right shape for a beat that can fire four times in one
// turn at Phase 2 - its script is sprites only, with no blend state to leak however often it runs.
EncScript_Rotom_Overclock:
	encjumpifvar CMP_GREATER_THAN, 4, 4, EncScript_Rotom_Overclock_Announce
	encaddvar 4, 1
EncScript_Rotom_Overclock_Announce:
	encjumpifvar CMP_EQUAL, 4, 5, EncScript_Rotom_Overclock_5
	encjumpifvar CMP_EQUAL, 4, 4, EncScript_Rotom_Overclock_4
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Rotom_Overclock_3
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Rotom_Overclock_2
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Rotom_Overclock_1
	return
EncScript_Rotom_Overclock_5:
	encjumpifvar CMP_EQUAL, 5, 5, EncScript_Rotom_Overclock_Done
	encsetvar 5, 5
	printstring STRINGID_ENCROTOMOVERCLOCK5
	goto EncScript_Rotom_Overclock_Say
EncScript_Rotom_Overclock_4:
	encjumpifvar CMP_EQUAL, 5, 4, EncScript_Rotom_Overclock_Done
	encsetvar 5, 4
	printstring STRINGID_ENCROTOMOVERCLOCK4
	goto EncScript_Rotom_Overclock_Say
EncScript_Rotom_Overclock_3:
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Rotom_Overclock_Done
	encsetvar 5, 3
	printstring STRINGID_ENCROTOMOVERCLOCK3
	goto EncScript_Rotom_Overclock_Say
EncScript_Rotom_Overclock_2:
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Rotom_Overclock_Done
	encsetvar 5, 2
	printstring STRINGID_ENCROTOMOVERCLOCK2
	goto EncScript_Rotom_Overclock_Say
EncScript_Rotom_Overclock_1:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Rotom_Overclock_Done
	encsetvar 5, 1
	printstring STRINGID_ENCROTOMOVERCLOCK1
EncScript_Rotom_Overclock_Say:
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Rotom_Harden
EncScript_Rotom_Overclock_Done:
	return

// The wall. From Overclock 3 the guard goes 88 -> 93, once per climb; the latch is cleared by the
// overload that ends the climb. Damage reduction has no on-screen indicator, so this line plus
// TurnOpen's recurring one are the only way the player learns their attacks stopped working.
EncScript_Rotom_Harden:
	encjumpifvar CMP_LESS_THAN, 4, 3, EncScript_Rotom_Harden_Done
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Rotom_Harden_Done
	encsetvar 6, 1
	call EncScript_Rotom_ApplyGuard
	printstring STRINGID_ENCROTOMHARDENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
EncScript_Rotom_Harden_Done:
	return

// --- THE POSSESSION CYCLE ---

// The single owner of Form, the boss's species, its signature move slot, the field, its stat stages
// and the clock. Called from the possession trigger, from the intro and from the end of an overload
// window. Form advances by one, and by one more half the time, wrapping 6 -> 1 and 7 -> 2 - so it
// never repeats the appliance it is already wearing and is still unpredictable.
EncScript_Rotom_Possess:
	call EncScript_Rotom_ClearField
	encaddvar 1, 1
	encjumpifchance 50, EncScript_Rotom_Possess_Wrap
	encaddvar 1, 1
EncScript_Rotom_Possess_Wrap:
	encjumpifvar CMP_EQUAL, 1, 7, EncScript_Rotom_Possess_Wrap7
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Rotom_Possess_Wrap6
	goto EncScript_Rotom_Possess_Branch
EncScript_Rotom_Possess_Wrap7:
	encsetvar 1, 2
	goto EncScript_Rotom_Possess_Branch
EncScript_Rotom_Possess_Wrap6:
	encsetvar 1, 1
EncScript_Rotom_Possess_Branch:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Rotom_Form_Mow
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Rotom_Form_Fan
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Rotom_Form_Frost
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Rotom_Form_Wash

// Each form's encformchange fail label skips only the two things that depend on the shape actually
// having changed - the raw-stat floor and the reboot - and lands on the body, because a failed form
// change must not also cost the fight its field, its signature move or its stat baseline.

// OVERHEAT PROTOCOL. The form the player wants to see: it is the one that cooks itself, +1 Overclock
// every turn on top of everything else, and the fastest road to an overload window.
EncScript_Rotom_Form_Heat:
	encformchange ENC_TARGET_BOSS, SPECIES_ROTOM_HEAT, EncScript_Rotom_Form_Heat_Body, B_ANIM_FORM_CHANGE
	call EncScript_Rotom_Bulk
	call EncScript_Rotom_Flush
EncScript_Rotom_Form_Heat_Body:
	call EncScript_Rotom_Neutral
	encsetvar 2, 1    @ Protocol
	encsetmove ENC_TARGET_BOSS, 3, MOVE_OVERHEAT
	printstring STRINGID_ENCROTOMPROTOCOLHEAT
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	goto EncScript_Rotom_Possess_Tail

// FLOOD PROTOCOL. The flood scours BOTH sides: it costs the player their screens, and it is also the
// only thing in the fight that takes down the Light Screen the FIELD hijack gave Rotom. Regen is
// scripted at TurnClose rather than left to an item, at 6% a turn.
EncScript_Rotom_Form_Wash:
	encformchange ENC_TARGET_BOSS, SPECIES_ROTOM_WASH, EncScript_Rotom_Form_Wash_Body, B_ANIM_FORM_CHANGE
	call EncScript_Rotom_Bulk
	call EncScript_Rotom_Flush
EncScript_Rotom_Form_Wash_Body:
	call EncScript_Rotom_Neutral
	encsetvar 2, 2    @ Protocol
	encsetmove ENC_TARGET_BOSS, 3, MOVE_HYDRO_PUMP
	printstring STRINGID_ENCROTOMPROTOCOLWASH
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Rotom_Possess_Tail
	printstring STRINGID_ENCROTOMWASHSCOURS
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Rotom_Possess_Tail

// CRYO PROTOCOL. Snow is what makes Blizzard never miss, and the Speed drop is what stops the player
// simply outrunning the appliance they least want to face.
EncScript_Rotom_Form_Frost:
	encformchange ENC_TARGET_BOSS, SPECIES_ROTOM_FROST, EncScript_Rotom_Form_Frost_Body, B_ANIM_FORM_CHANGE
	call EncScript_Rotom_Bulk
	call EncScript_Rotom_Flush
EncScript_Rotom_Form_Frost_Body:
	call EncScript_Rotom_Neutral
	encsetvar 2, 3    @ Protocol
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BLIZZARD
	printstring STRINGID_ENCROTOMPROTOCOLFROST
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SNOW, 0
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	goto EncScript_Rotom_Possess_Tail

// SPEED PROTOCOL. Tailwind, +2 Speed and +1 evasion, and on top of that it strikes at TurnOpen before
// the player acts - there is no extra-action command, and acting first is the same felt experience.
EncScript_Rotom_Form_Fan:
	encformchange ENC_TARGET_BOSS, SPECIES_ROTOM_FAN, EncScript_Rotom_Form_Fan_Body, B_ANIM_FORM_CHANGE
	call EncScript_Rotom_Bulk
	call EncScript_Rotom_Flush
EncScript_Rotom_Form_Fan_Body:
	call EncScript_Rotom_Neutral
	encsetvar 2, 4    @ Protocol
	encsetmove ENC_TARGET_BOSS, 3, MOVE_AIR_SLASH
	printstring STRINGID_ENCROTOMPROTOCOLFAN
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 1
	goto EncScript_Rotom_Possess_Tail

// HARVEST PROTOCOL. Grassy Terrain's own end-turn regen is the healing, so no script is needed for it
// - and it heals the player too. That is correct: Mow is the protocol the player can turn around, and
// it pays for that by being the worst Electric matchup in the fight (0.25x, so feeding the motor here
// is nearly free damage-wise for Rotom).
EncScript_Rotom_Form_Mow:
	encformchange ENC_TARGET_BOSS, SPECIES_ROTOM_MOW, EncScript_Rotom_Form_Mow_Body, B_ANIM_FORM_CHANGE
	call EncScript_Rotom_Bulk
	call EncScript_Rotom_Flush
EncScript_Rotom_Form_Mow_Body:
	call EncScript_Rotom_Neutral
	encsetvar 2, 5    @ Protocol
	encsetmove ENC_TARGET_BOSS, 3, MOVE_LEAF_STORM
	printstring STRINGID_ENCROTOMPROTOCOLMOW
	waitmessage B_WAIT_TIME_LONG
	encsetterrain ENC_TERRAIN_GRASSY, 0
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG    @ repaints the field to the new terrain, the
	                                                 @ same call BattleScript_TerrainEnds uses
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1

// Taking an appliance is itself work: every possession is +1 Overclock, which is what makes Rotom's
// own clock a road to the overload window even against a player who never feeds it.
EncScript_Rotom_Possess_Tail:
	call EncScript_Rotom_Overclock
	call EncScript_Rotom_ClockArm
	return

// --- TurnOpen's pieces ---

// The window's heartbeat and countdown. The line runs every turn it is open, so a player who missed
// the overload message still cannot miss that the guard is down. Held at 1 for the Reboot trigger to
// consume at OnTurnEnd, one checkpoint away.
EncScript_Rotom_OverloadTick:
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Rotom_OverloadTick_Line
	encsubvar 7, 1
EncScript_Rotom_OverloadTick_Line:
	printstring STRINGID_ENCROTOMSTILLDOWN
	waitmessage B_WAIT_TIME_SHORT
	return

// The Fan strike. Scripted encchangehp rather than a move, so it lands before the player's action and
// Protect does not stop it - stated in the cheat-sheet rather than hidden.
EncScript_Rotom_FanStrike:
	encjumpifvar CMP_NOT_EQUAL, 2, 4, EncScript_Rotom_FanStrike_Done
	printstring STRINGID_ENCROTOMFANSTRIKE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STRONG_WINDS
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
EncScript_Rotom_FanStrike_Done:
	return

// The recurring half of the hardening callout. One message at the moment the guard rises is trivially
// missed over a fight this long, so the warning runs for as long as the wall is up.
EncScript_Rotom_HardWarn:
	encjumpifvar CMP_LESS_THAN, 4, 3, EncScript_Rotom_HardWarn_Done
	printstring STRINGID_ENCROTOMSTRAINING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Rotom_HardWarn_Done:
	return

EncScript_Rotom_ClockTick:
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Rotom_ClockTick_Done
	encsubvar 3, 1
EncScript_Rotom_ClockTick_Done:
	return

// The hijack schedule, read off the freshly ticked clock. Phase 0 seizes one system per possession
// cycle, on the middle turn; Phase 1 seizes on every turn a possession does not fire; Phase 2 seizes
// every turn, alongside the possession.
EncScript_Rotom_HijackSchedule:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rotom_HijackSchedule_Arm
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rotom_HijackSchedule_Rogue
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Rotom_HijackSchedule_Arm
	return
EncScript_Rotom_HijackSchedule_Rogue:
	encjumpifvar CMP_NOT_EQUAL, 3, 2, EncScript_Rotom_HijackSchedule_Done
EncScript_Rotom_HijackSchedule_Arm:
	encsetvar 9, 1    @ HijackDue
EncScript_Rotom_HijackSchedule_Done:
	return

// --- TurnClose's pieces ---

// Wash's regen. Announced every turn it fires: 6% a turn compounds over a fight this long, and a
// health bar quietly refilling with no line is exactly the "my attacks stopped working" failure.
EncScript_Rotom_WashRegen:
	encjumpifvar CMP_NOT_EQUAL, 2, 2, EncScript_Rotom_WashRegen_Done
	printstring STRINGID_ENCROTOMWASHREGEN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 6, ENC_AMOUNT_PERCENT
EncScript_Rotom_WashRegen_Done:
	return

// Heat's accelerant, and the per-phase passives. Rotom coming apart faster the closer it is to dying
// is what turns Phase 2's chaos into the player's endgame rather than a wall.
EncScript_Rotom_PassiveTick:
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Rotom_PassiveTick_Phase
	call EncScript_Rotom_Overclock
EncScript_Rotom_PassiveTick_Phase:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Rotom_PassiveTick_Done
	call EncScript_Rotom_Overclock
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Rotom_PassiveTick_Done
	call EncScript_Rotom_Overclock
EncScript_Rotom_PassiveTick_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so nothing needs seeding: the opening possession takes
// Form from 0 to Heat or Wash, applies the raw-stat floor through the form change, and arms the clock.
// Rotom is never seen in its base form until it blows out, which is what makes the base form read as
// damage rather than as a default.
EncScript_Rotom_Intro::
	printstring STRINGID_ENCROTOMINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCROTOMINTRO2
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Rotom_Possess
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set and
// MoveGuard cleared immediately rather than at the tail, so no branch out of this script can leave
// either wrong. A blown-out Rotom does nothing but count down - no strike, no clock, no hijack.
EncScript_Rotom_TurnOpen::
	flushtextbox
	encsetvar 10, 1    @ TurnGuard
	encsetvar 11, 0    @ MoveGuard: the Electric lever is armed again
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Rotom_TurnOpen_Done   @ Weakened: inert
	encjumpifvar CMP_GREATER_THAN, 7, 0, EncScript_Rotom_TurnOpen_Blown
	call EncScript_Rotom_FanStrike
	call EncScript_Rotom_HardWarn
	call EncScript_Rotom_ClockTick
	call EncScript_Rotom_HijackSchedule
	return
EncScript_Rotom_TurnOpen_Blown:
	call EncScript_Rotom_OverloadTick
EncScript_Rotom_TurnOpen_Done:
	return

// The turn's passive meter movement, then the guard resets. Everything else lives at TurnOpen.
EncScript_Rotom_TurnClose::
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Rotom_TurnClose_Guards
	encjumpifvar CMP_GREATER_THAN, 7, 0, EncScript_Rotom_TurnClose_Guards
	call EncScript_Rotom_WashRegen
	call EncScript_Rotom_PassiveTick
EncScript_Rotom_TurnClose_Guards:
	encsetvar 10, 0    @ TurnGuard
	encsetvar 11, 0    @ MoveGuard
	return

// THE LEVER. An Electric move is 0.5x against every Rotom form and 0.25x against Mow, so this is a
// turn spent dealing almost nothing in exchange for a rung on the meter - and the meter is the only
// thing that opens the window where real damage lands. Setting MoveGuard is also what disables the
// trigger for the rest of the turn.
EncScript_Rotom_FedTheMotor::
	encsetvar 11, 1    @ MoveGuard
	printstring STRINGID_ENCROTOMFEDTHEMOTOR
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Rotom_Overclock
	return

EncScript_Rotom_PossessTick::
	call EncScript_Rotom_Possess
	return

// The three hijacks, rotating 0 -> 1 -> 2 -> 0 so all three appear before any repeats. Every one ends
// with a rung on the meter: Rotom cannot seize a system without cooking itself, which is the trade
// that makes the endgame closable.
// CONTROL's fail label falls through into POWER rather than wasting the beat - a turn-1 hijack (no
// move used yet) and a player already disabled are ordinary states, and the fallthrough also
// overwrites the rotation with the hijack that actually happened.
EncScript_Rotom_Hijack::
	encsetvar 9, 0    @ HijackDue
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Rotom_Hijack_Field
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Rotom_Hijack_Power
EncScript_Rotom_Hijack_Control:
	encsetvar 8, 1
	encdisablemove ENC_TARGET_PLAYER_LEFT, 4, EncScript_Rotom_Hijack_Power
	printstring STRINGID_ENCROTOMHIJACKCONTROL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SNATCH_MOVE
	goto EncScript_Rotom_Hijack_Charge
EncScript_Rotom_Hijack_Power:
	encsetvar 8, 2
	encsetembargo ENC_TARGET_PLAYER_LEFT, 5
	printstring STRINGID_ENCROTOMHIJACKPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MAGIC_ROOM
	goto EncScript_Rotom_Hijack_Charge
EncScript_Rotom_Hijack_Field:
	encsetvar 8, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN, 0
	encjumpifchance 50, EncScript_Rotom_Hijack_Swamp
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 4
	printstring STRINGID_ENCROTOMHIJACKFIELDFIRE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	goto EncScript_Rotom_Hijack_Charge
EncScript_Rotom_Hijack_Swamp:
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP, 4
	printstring STRINGID_ENCROTOMHIJACKFIELDSWAMP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SWAMP
EncScript_Rotom_Hijack_Charge:
	call EncScript_Rotom_Overclock
	return

// SYSTEM OVERLOAD. The possession drops, the field goes with it, the signature move is gone and the
// guard falls off a cliff for three turns (four, and further, from 25%). Base form is genuinely weak
// and that is the point: MOVE_ASTONISH is 30 power off Rotom's 65 Attack, so losing the appliance is
// losing the fight's damage. The window is opened by a lost turn - encsetrecharge with 2 at OnTurnEnd
// costs it NEXT turn's action - and closed by the Reboot trigger at Overload == 1.
EncScript_Rotom_SystemOverload::
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rotom_Overload_Critical
	encsetvar 7, 4    @ Overload: three turns of window after the free one
	printstring STRINGID_ENCROTOMSYSTEMOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Rotom_Overload_Body
EncScript_Rotom_Overload_Critical:
	encsetvar 7, 5
	printstring STRINGID_ENCROTOMCRITICALFAILURE
	waitmessage B_WAIT_TIME_LONG
EncScript_Rotom_Overload_Body:
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	call EncScript_Rotom_ClearField
	encsetvar 2, 0    @ Protocol: out of form
	encsetvar 4, 0    @ Overclock
	encsetvar 5, 0    @ LastOC
	encsetvar 6, 0    @ Hardened: the next climb re-announces the wall
	encsetvar 9, 0    @ HijackDue: nothing is seized out of an overload
	encformchange ENC_TARGET_BOSS, SPECIES_ROTOM, EncScript_Rotom_Overload_Strip, B_ANIM_FORM_CHANGE_INSTANT
	call EncScript_Rotom_Bulk
EncScript_Rotom_Overload_Strip:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_ASTONISH
	call EncScript_Rotom_Neutral
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -2
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -2
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -2
	call EncScript_Rotom_ApplyGuard    @ Overload is already set, so this lands the window's guard
	encsetrecharge ENC_TARGET_BOSS, 2
	return

// The window closes and the loop starts over. ApplyGuard restores the phase's own number - Hardened
// was cleared by the overload, so this is the base guard, not the wall - and Possess puts Rotom back
// into an appliance and re-arms everything.
EncScript_Rotom_Reboot::
	encsetvar 7, 0    @ Overload
	call EncScript_Rotom_ApplyGuard
	printstring STRINGID_ENCROTOMREBOOT
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Rotom_Possess
	return

// 50% - ROGUE PROGRAM. Possession every two turns, a hijack on every off-turn, +1 Overclock passively
// per turn, guard 88 -> 90. The screens the player was hiding behind come down on the way in.
EncScript_Rotom_RogueProgram::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCROTOMROGUEPROGRAM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Rotom_RogueProgram_Arm
	printstring STRINGID_ENCROTOMTEARSSCREENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Rotom_RogueProgram_Arm:
	call EncScript_Rotom_ApplyGuard
	call EncScript_Rotom_ClockArm
	return

// 25% - TOTAL SYSTEM TAKEOVER. Possession EVERY turn, hijack every turn, +2 Overclock a turn on top
// of those - so the meter maxes within two turns of entering the phase and every two turns after.
// The band is deliberately mostly WINDOW: at guard 40 for four turns a narrow 25%-to-10% band is
// closable, and Rotom's own escalation is what kills it rather than the player out-damaging it.
EncScript_Rotom_Takeover::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCROTOMTAKEOVER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Rotom_ApplyGuard
	call EncScript_Rotom_ClockArm
	return

// 10% - the catch window, the mirror image of the Properties: block plus everything the fight built.
// Phase 3 retires every other trigger, so no clock, hijack, strike or overload can restart after it.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Rotom_Weakened::
	encsetvar 3, 0    @ Clock
	encsetvar 4, 0    @ Overclock
	encsetvar 5, 0    @ LastOC
	encsetvar 6, 0    @ Hardened
	encsetvar 7, 0    @ Overload
	encsetvar 9, 0    @ HijackDue
	call EncScript_Rotom_ClearField
	encsetvar 0, 3    @ Phase
	encsetvar 2, 0    @ Protocol
	encformchange ENC_TARGET_BOSS, SPECIES_ROTOM, EncScript_Rotom_Weakened_Strip, B_ANIM_FORM_CHANGE_INSTANT
	call EncScript_Rotom_Bulk
EncScript_Rotom_Weakened_Strip:
	call EncScript_Rotom_Neutral
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
	encsetembargo ENC_TARGET_PLAYER_LEFT, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 40
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCROTOMWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
