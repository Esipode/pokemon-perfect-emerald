// Heatran, "The Lava Dome". Var indices, pinned by the always-true Conditions on
// EncScript_Heatran_Intro:
// 0 Phase (0 Dormant / 1 Magma Core / 2 Eruption / 3 Weakened), 1 Pressure (0-5, the dial),
// 2 LastPres (the last ANNOUNCED stage), 3 Vent (the vent-window countdown), 4 Pocket (the fuse,
// 3 -> 2 -> 1 -> burst), 5 PocketClock (turns until the next pocket is laid), 6 Shock (the Thermal
// Shock timer), 7 HpMark (the encsnapshothp mark the heavy-hit tick measures against), 8 Spent
// (Phase 2 resolved, by subsiding or by erupting), 9 TurnGuard, 10 PlayerMove, 11 BossMove,
// 12 WeatherGuard, 13 Tick (the Phase 0 half-rate passive climb), 14 Pulse (paces the standing line).
//
// The volcano is the other combatant. Everything Heatran does raises the Pressure, everything the
// player does to put it out lowers it, and neither end of the dial is safe: let it climb and the
// field starts killing them, hold it at 0 and they spent every turn cooling instead of attacking.
// Reaching 5 vents the mountain - a fifth of their side, bought for the only real damage window in
// the fight. Nothing states any of that. The fight only ever describes the mountain.

// --- Shared subroutines (call/return) ---

// The only writers of Pressure, and both silent - announcing is a separate step, so a two-stage drop
// (a Water move) prints one stage line instead of two. Retired once the Phase 2 beat has resolved or
// the catch window is open: past that the dial is not a mechanic any more, and letting it move would
// tear down a field the Supernova deliberately left standing.
EncScript_Heatran_Raise:
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Heatran_Raise_Done            @ Spent
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Heatran_Raise_Done     @ Weakened
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Heatran_Raise_Done
	encaddvar 1, 1      @ Pressure
EncScript_Heatran_Raise_Done:
	return

EncScript_Heatran_Lower:
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Heatran_Lower_Done            @ Spent
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Heatran_Lower_Done     @ Weakened
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Heatran_Lower_Done
	encsubvar 1, 1      @ Pressure
EncScript_Heatran_Lower_Done:
	return

// Lower + announce in one step. Only the player's own cooling moves use it: they happen at OnMoveEnd
// and want the dial to answer them immediately. Everything that raises the dial does so silently and
// lets TurnClose announce the turn's net result once - see EncScript_Heatran_TurnClose.
EncScript_Heatran_PressureDown:
	call EncScript_Heatran_Lower
	call EncScript_Heatran_Announce
	return

// THE CALLOUT. A hidden dial the whole fight hangs off has to re-fire on every real change, not
// once, so the last ANNOUNCED stage is stored and compared and a stage that did not move stays
// silent. Six distinct lines, stage 0 included: the dial falling back to dormant is as much
// information as it climbing, and a player who has just cooled it needs to see that it worked.
// LastPres is written LAST, so ApplyStage below can still see which side of Molten the dial came
// from and only raise the flow on the way in.
EncScript_Heatran_Announce:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Heatran_Announce_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Heatran_Announce_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Heatran_Announce_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Heatran_Announce_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Heatran_Announce_4
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Heatran_Announce_Done
	printstring STRINGID_ENCHEATRANPRESSURE5
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	goto EncScript_Heatran_Announce_Apply

EncScript_Heatran_Announce_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Heatran_Announce_Done
	printstring STRINGID_ENCHEATRANPRESSURE0
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Heatran_Announce_Apply

EncScript_Heatran_Announce_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Heatran_Announce_Done
	printstring STRINGID_ENCHEATRANPRESSURE1
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Heatran_Announce_Apply

EncScript_Heatran_Announce_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Heatran_Announce_Done
	printstring STRINGID_ENCHEATRANPRESSURE2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Heatran_Announce_Apply

EncScript_Heatran_Announce_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Heatran_Announce_Done
	printstring STRINGID_ENCHEATRANPRESSURE3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	goto EncScript_Heatran_Announce_Apply

EncScript_Heatran_Announce_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Heatran_Announce_Done
	printstring STRINGID_ENCHEATRANPRESSURE4
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE

EncScript_Heatran_Announce_Apply:
	call EncScript_Heatran_ApplyStage
	call EncScript_Heatran_ApplyStance
	call EncScript_Heatran_ApplyGuard
	enccopyvar 2, 1     @ LastPres = Pressure
EncScript_Heatran_Announce_Done:
	return

// What each stage puts on the field, written absolutely rather than additively - so the dial moving
// down un-does the flow without a separate teardown path. Called from the announcer, so what is
// standing always matches the number the player was just shown.
// The sun is timed rather than permanent on purpose: a permanent one would have to be taken down by
// name on the way back to Dormant, and the only opcode for that is removeweather, which would also
// eat the rain the player put up to cool with. Timed, it lapses on its own and the player's own
// weather is never touched.
EncScript_Heatran_ApplyStage:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Heatran_ApplyStage_Molten
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	return
// LAVA FLOW. The sun here is doing real work in two directions at once: 1.5x on Heatran's Fire moves
// AND 0.5x on the player's Water moves - so the stage that most needs cooling is the stage where the
// strong coolant stops working and Ice becomes correct.
EncScript_Heatran_ApplyStage_Molten:
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Heatran_ApplyStage_Done   @ already Molten or past it
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 0
	encsetweather BATTLE_WEATHER_SUN, 5
	printstring STRINGID_ENCHEATRANLAVAFLOW
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Heatran_ApplyStage_Done
	printstring STRINGID_ENCHEATRANSCOURED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Heatran_ApplyStage_Done:
	return

// Heatran's stance, written absolutely for the same reason ApplyStage is: a fight that crosses the
// same stages a dozen times would otherwise end on a pile of accumulated steps.
// SP. ATK IS DELIBERATELY NOT IN HERE. An absolute write to it would hand Heatran back the -2 its own
// OVERHEAT costs it every time the dial moved - which from 50% down is most turns, and reads from the
// outside as the boss buffing itself over and over. It would wipe the player's Snarl or Eerie Impulse
// the same way. Attack and Speed are left alone for that reason too; the only stat stages this
// encounter owns are the two nothing else in the fight writes.
EncScript_Heatran_ApplyStance:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 6
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Heatran_ApplyStance_Down       @ Spent
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Heatran_ApplyStance_Down   @ inside a vent window
	return
EncScript_Heatran_ApplyStance_Down:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -2
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -2
	return

// THE SOLE OWNER of the damage reduction, so the vent, the shock, the stages and the three phase
// transitions all agree on one number. A ladder, first match wins. An open vent window outranks
// everything: a phase threshold crossed mid-window must not slam the guard back up and close a
// window the player already paid a fifth of their side for.
EncScript_Heatran_ApplyGuard:
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Heatran_Guard_Vent
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Heatran_Guard_Spent
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Heatran_Guard_Shock
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Heatran_Guard_Base
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Heatran_Guard_HotPh2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Heatran_Guard_HotPh1
	encsetdamagereduction ENC_TARGET_BOSS, 92      @ the heat shell, Phase 0
	return
EncScript_Heatran_Guard_HotPh1:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Heatran_Guard_HotPh2:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Heatran_Guard_Base:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Heatran_Guard_BasePh2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Heatran_Guard_BasePh1
	encsetdamagereduction ENC_TARGET_BOSS, 88      @ the phase's base
	return
EncScript_Heatran_Guard_BasePh1:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Heatran_Guard_BasePh2:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
// THE WINDOW. The player's damage per turn swings roughly fivefold between waiting and this.
EncScript_Heatran_Guard_Vent:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Heatran_Guard_Vent0
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
EncScript_Heatran_Guard_Vent0:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Heatran_Guard_Shock:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Heatran_Guard_Spent:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return

// THE TIMERS ARE SILENT. Every one of them used to print its own standing line at turn open, so a
// turn with a vent window, a shock and a lit pocket on it opened with three message boxes before the
// player had done anything - and that is before the field chip, the cooling line and the stage line
// at the other end of the turn. EncScript_Heatran_Warnings below prints ONE line for all of them.
// The shock's release still speaks for itself: it is a real state change, not a standing condition.
EncScript_Heatran_VentTick:
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Heatran_VentTick_Done
	encsubvar 3, 1      @ Vent
EncScript_Heatran_VentTick_Done:
	return

EncScript_Heatran_ShockTick:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Heatran_ShockTick_Done
	encsubvar 6, 1      @ Shock
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Heatran_ShockTick_Done
	call EncScript_Heatran_ApplyGuard
	printstring STRINGID_ENCHEATRANSHOCKENDS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Heatran_ShockTick_Done:
	return

// THE RECURRING CHANNEL, capped at one line a turn. Guideline 14 wants a standing cue for as long as
// a hidden mechanic is running; it does not want four of them stacked. So the most urgent live
// mechanic speaks and the rest stay quiet: a lit pocket has a deadline the player must act on, a vent
// window is the damage they must spend, the shock is why their damage stopped landing, and the flow
// is the slow background - which only speaks every other turn, since it is the least urgent of the
// four and the one whose stage line already announced itself.
EncScript_Heatran_Warnings:
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Heatran_Warnings_Pocket
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Heatran_Warnings_Vent
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Heatran_Warnings_Shock
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Heatran_Warnings_Done
	encjumpifvar CMP_NOT_EQUAL, 14, 0, EncScript_Heatran_Warnings_Done   @ Pulse: every other turn
	printstring STRINGID_ENCHEATRANSTANDING
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Heatran_Warnings_Pocket:
	printstring STRINGID_ENCHEATRANPOCKETSTANDING
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Heatran_Warnings_Vent:
	printstring STRINGID_ENCHEATRANVENTSTANDING
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Heatran_Warnings_Shock:
	printstring STRINGID_ENCHEATRANSHOCKSTANDING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Heatran_Warnings_Done:
	return

// MAGMA POCKETS. A mark with a fuse, laid while the floor is glowing and no pocket is already lit.
// The fuse is ticked here and resolved at OnTurnEnd, so a tick can never eat a step off a fuse armed
// the same turn.
EncScript_Heatran_PocketTick:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Heatran_PocketTick_Clock
	encjumpifvar CMP_LESS_THAN, 4, 2, EncScript_Heatran_PocketTick_Done
	encsubvar 4, 1      @ Pocket
	return
EncScript_Heatran_PocketTick_Clock:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Heatran_PocketTick_Done   @ nothing opens below Heated
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Heatran_PocketTick_Lay
	encsubvar 5, 1      @ PocketClock
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Heatran_PocketTick_Done
EncScript_Heatran_PocketTick_Lay:
	call EncScript_Heatran_LayPocket
EncScript_Heatran_PocketTick_Done:
	return

// The pockets come faster as the mountain gets hotter, which is the spec's "at higher Pressure,
// Magma Pockets fire more often": every third turn at rest, every second in the Magma Core, and
// every turn once the basin is boiling over.
EncScript_Heatran_LayPocket:
	encsetvar 4, 3      @ Pocket: the fuse
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Heatran_LayPocket_Fast
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Heatran_LayPocket_Slow
	encsetvar 5, 2      @ PocketClock
	goto EncScript_Heatran_LayPocket_Arm
EncScript_Heatran_LayPocket_Fast:
	encsetvar 5, 1
	goto EncScript_Heatran_LayPocket_Arm
EncScript_Heatran_LayPocket_Slow:
	encsetvar 5, 3
EncScript_Heatran_LayPocket_Arm:
	printstring STRINGID_ENCHEATRANPOCKETARMED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SHELL_TRAP_SETUP
	return

// The field's own damage. Below Molten it is the floor; at Molten and above the Sea of Fire is doing
// the burning, and the extra only lands on what cannot get off the ground.
// "Grounded" is approximated: there is no grounded-check opcode, so this reads Levitate and a Flying
// type and does not see Air Balloon, Magnet Rise or Telekinesis. The cheat-sheet documents the rule
// as it is actually implemented rather than as "grounded".
EncScript_Heatran_FieldChip:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Heatran_FieldChip_Done
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Heatran_FieldChip_Flow
	printstring STRINGID_ENCHEATRANFIELDBURN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Heatran_FieldChip_Flow:
	jumpifability BS_PLAYER1, ABILITY_LEVITATE, EncScript_Heatran_FieldChip_Done
	jumpiftype BS_PLAYER1, TYPE_FLYING, EncScript_Heatran_FieldChip_Done
	printstring STRINGID_ENCHEATRANGROUNDED
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	enchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
EncScript_Heatran_FieldChip_Done:
	return

// WHAT THE MEGA IS FOR. Mega Heatran's stat line alone is not a phase: the Sp. Atk spike is pulled
// straight back down (see EncScript_Heatran_MagmaCore), the Attack gain is dead against a moveset
// with no physical move in it, and a Sp. Def jump the player cannot see is not a mechanic. So the
// Mega gets one of its own, and it is the fusion made literal - fused with the mountain, it feeds on
// the mountain.
//   While Mega, a dial at Molten or above heals Heatran 3% every turn.
// This INVERTS the phase 0 incentive, which is the whole point. Until now a hot dial was purely the
// player's friend: it was how they bought a vent window. From here a dial parked hot pays Heatran
// rent every turn it sits there, so the climb has to be timed rather than left running - and venting
// (which resets the dial to 0) stops the healing as well as opening the window, so the loop the fight
// already taught is still the answer. Phase 1 only: at Eruption the mountain has taken over from
// Heatran, and a heal against a +2 a turn climb would stall the endgame outright.
EncScript_Heatran_CoreDrink:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Heatran_CoreDrink_Done   @ Magma Core only
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Heatran_CoreDrink_Done   @ nothing to drink below Molten
	printstring STRINGID_ENCHEATRANDRINKS
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Heatran_CoreDrink_Done:
	return

// "How badly did it get hit this turn", measured against the mark TurnOpen set. encsnapshothp is the
// one command that can express that, and it works in percent, so it is safe under Level: LevelCap
// and New Game Plus offsets. Its failLabel is the test as well as the record.
EncScript_Heatran_HeavyHit:
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_DAMAGE, EncScript_Heatran_HeavyHit_Done
	encjumpifvar CMP_LESS_THAN, 7, 15, EncScript_Heatran_HeavyHit_Done
	call EncScript_Heatran_Raise
EncScript_Heatran_HeavyHit_Done:
	return

// THE PLUGGED VENT - this encounter's answer to guideline 8, and it is not an immunity. Status lands
// on Heatran and works completely: sleep still costs it its turns, paralysis still costs it its
// speed. It just drives the volcano toward the thing that hurts the PLAYER, because a vent nobody
// chose to open still costs them a fifth of their side and puts the lava flow up on the way there.
// Toxic is the sharpest version of that and is deliberately left playable - bounded to a flat 1/16
// by FlatToxicDamage, it is a permanent +1 a turn, which is a strategy rather than an exploit.
// Fire/Steel closes the rest on its own: Heatran cannot be burned, and Fire-types cannot be frozen.
EncScript_Heatran_StatusTick:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Heatran_StatusTick_Do
	return
EncScript_Heatran_StatusTick_Do:
	call EncScript_Heatran_Raise
	encjumpifvar CMP_NOT_EQUAL, 14, 1, EncScript_Heatran_StatusTick_Quiet   @ Pulse: every other turn
	printstring STRINGID_ENCHEATRANPLUGGED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Heatran_StatusTick_Quiet:
	return

// Turns passing, on the phase's own schedule: every second turn while it is dormant, every turn in
// the Magma Core, and twice a turn once the mountain is erupting - which is the Phase 2 countdown,
// expressed as the number the player has been reading all fight rather than a new one.
EncScript_Heatran_PassiveTick:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Heatran_PassiveTick_Erupt
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Heatran_PassiveTick_Core
	encaddvar 13, 1     @ Tick
	encjumpifvar CMP_LESS_THAN, 13, 2, EncScript_Heatran_PassiveTick_Done
	encsetvar 13, 0
EncScript_Heatran_PassiveTick_Core:
	call EncScript_Heatran_Raise
	return
EncScript_Heatran_PassiveTick_Erupt:
	call EncScript_Heatran_Raise
	call EncScript_Heatran_Raise
EncScript_Heatran_PassiveTick_Done:
	return

// The shared tail of both Phase 2 resolutions. Spent retires the phase's climb, the pocket clock and
// the vent trigger for the rest of the battle. LastPres is written alongside Pressure so the tail's
// own line replaces the dormant one rather than following it.
EncScript_Heatran_SpentTail:
	encsetvar 8, 1      @ Spent
	encsetvar 1, 0      @ Pressure
	encsetvar 2, 0      @ LastPres
	encsetvar 3, 0      @ Vent
	encsetvar 4, 0      @ Pocket
	encsetvar 5, 0      @ PocketClock
	encsetvar 6, 0      @ Shock
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	call EncScript_Heatran_ApplyStance
	call EncScript_Heatran_ApplyGuard
	printstring STRINGID_ENCHEATRANSPENT
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the pocket clock is seeded. Pressure starts at 0
// and the player gets to watch it move for the first time on its own.
EncScript_Heatran_Intro::
	printstring STRINGID_ENCHEATRANINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCHEATRANINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 5, 3      @ PocketClock
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set and every
// re-entry guard cleared immediately rather than at the tail, so no branch out of this script can
// leave one wrong.
EncScript_Heatran_TurnOpen::
	flushtextbox
	encsetvar 9, 1      @ TurnGuard
	encsetvar 10, 0     @ PlayerMove
	encsetvar 11, 0     @ BossMove
	encsetvar 12, 0     @ WeatherGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Heatran_TurnOpen_Done   @ Weakened: inert
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_SET
	@ Pulse alternates 1, 0, 1, 0 - the turn parity the recurring lines pace themselves against. It
	@ belongs to the turn now, not to the dial, so nothing else may write it.
	encaddvar 14, 1
	encjumpifvar CMP_LESS_THAN, 14, 2, EncScript_Heatran_TurnOpen_Ticks
	encsetvar 14, 0
EncScript_Heatran_TurnOpen_Ticks:
	call EncScript_Heatran_VentTick
	call EncScript_Heatran_ShockTick
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Heatran_TurnOpen_Done          @ Spent: the mountain is out
	call EncScript_Heatran_PocketTick
	call EncScript_Heatran_Warnings
EncScript_Heatran_TurnOpen_Done:
	return

// THE COOLANTS. Water is the strongest and the trap: Heatran is Fire/Steel, so Water is 2x on it and
// the fastest way down the dial - and taking two stages off in one action is exactly what the rock
// punishes. Ice never reaches that branch, which is the whole point: the player always has a safe way
// to cool, it is just half as fast.
EncScript_Heatran_CoolWater::
	encsetvar 10, 1     @ PlayerMove
	printstring STRINGID_ENCHEATRANCOOLWATER
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Heatran_CoolWater_Single
	call EncScript_Heatran_Lower
	call EncScript_Heatran_Lower
	call EncScript_Heatran_Announce
	encsetvar 6, 3      @ Shock: THERMAL SHOCK, three turns behind a 96% guard
	call EncScript_Heatran_ApplyGuard
	printstring STRINGID_ENCHEATRANTHERMALSHOCK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	return
EncScript_Heatran_CoolWater_Single:
	call EncScript_Heatran_PressureDown
	return

EncScript_Heatran_CoolIce::
	encsetvar 10, 1     @ PlayerMove
	printstring STRINGID_ENCHEATRANCOOLICE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Heatran_PressureDown
	return

EncScript_Heatran_Stoked::
	encsetvar 11, 1     @ BossMove
	printstring STRINGID_ENCHEATRANSTOKED
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Heatran_Raise   @ silent; TurnClose announces the turn's stage once
	return

EncScript_Heatran_WeatherCool::
	encsetvar 12, 1     @ WeatherGuard
	printstring STRINGID_ENCHEATRANWEATHERCOOL
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Heatran_Lower   @ silent; TurnClose announces the turn's stage once
	return

// The first of the pocket's three outs: walk off it. Magma Storm is what makes this cost something -
// it traps, so the switch is gone for as long as it holds and the player is standing on a lit pocket
// with only Protect or the dial left. That threat is a real move on its own accuracy roll, which is
// why encsettrapped appears nowhere in this encounter.
EncScript_Heatran_PocketDefused::
	encsetvar 4, 0      @ Pocket
	printstring STRINGID_ENCHEATRANPOCKETSWITCHED
	waitmessage B_WAIT_TIME_LONG
	return

// THE VENT, and the loop the whole fight turns on. The blast costs the player about a fifth of their
// side and buys them the only three turns in the fight where their damage lands - so racing the dial
// up to a window is a real trade rather than a free button.
EncScript_Heatran_Vent::
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Heatran_Supernova
	printstring STRINGID_ENCHEATRANCOREVENT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Heatran_Vent_Core
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Heatran_Vent_Window
EncScript_Heatran_Vent_Core:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
// Vent is set BEFORE the dial is driven down, so the ApplyStance and ApplyGuard the announcer calls
// both read the window that has just opened. The lost action is what makes it feel like a window
// rather than a number the player cannot see.
EncScript_Heatran_Vent_Window:
	encsetvar 3, 4      @ Vent: three turns, ticked at TurnOpen
	encsetvar 4, 0      @ Pocket: the blast takes the lit pocket with it
	encsetvar 6, 0      @ Shock: the rock has just been blown off
	encsetvar 1, 0      @ Pressure
	call EncScript_Heatran_Announce
	encsetrecharge ENC_TARGET_BOSS, 2   @ at OnTurnEnd this costs it NEXT turn's action
	call EncScript_Heatran_ApplyGuard
	return

// Phase 2's failure state. Enormous and survivable - enchangehp faints a battler it takes to 0,
// exactly like weather chip, so a player who reaches this on a nearly-dead team loses. That weight is
// the point, and the catch window is still reachable either way: both resolutions end spent.
EncScript_Heatran_Supernova:
	printstring STRINGID_ENCHEATRANSUPERNOVA
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	enchangehp ENC_TARGET_ALL_FOES, -50, ENC_AMOUNT_PERCENT
	call EncScript_Heatran_SpentTail
// Raised AFTER the tail, which drives the dial to 0 and takes the flow down with it. What the
// eruption left behind is not the dial's to take back - the field changed.
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 0
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	return

EncScript_Heatran_VentRecover::
	encsetvar 3, 0      @ Vent
	call EncScript_Heatran_ApplyStance
	call EncScript_Heatran_ApplyGuard
	printstring STRINGID_ENCHEATRANVENTRECOVER
	waitmessage B_WAIT_TIME_LONG
	return

// Phase 2's success state: the player out-cooled a +2 a turn climb all the way to 0.
EncScript_Heatran_Subsided::
	printstring STRINGID_ENCHEATRANSUBSIDED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Heatran_SpentTail
	return

// All three of the spec's outs meet here. Cooling the dial below Molten is the third: the pocket has
// nothing left under it to come up through.
EncScript_Heatran_MagmaBurst::
	encsetvar 4, 0      @ Pocket
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Heatran_MagmaBurst_Cooled
	printstring STRINGID_ENCHEATRANPOCKETBURST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	enchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	return
EncScript_Heatran_MagmaBurst_Cooled:
	printstring STRINGID_ENCHEATRANPOCKETCOOLED
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Heatran_MagmaBurstBlocked::
	encsetvar 4, 0      @ Pocket
	printstring STRINGID_ENCHEATRANPOCKETBLOCKED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return

// WeatherGuard is deliberately NOT cleared here. EncScript_Heatran_WeatherCool runs at this same
// checkpoint at a lower priority, so clearing its guard from here re-armed it inside the same
// dispatch and it fired twice every turn the weather was up - two messages and -2 pressure for one
// turn of rain. TurnOpen is the only place it may be cleared. PlayerMove and BossMove are safe to
// clear here because their triggers live at OnMoveEnd.
// Every raise below is silent and ONE Announce covers the lot, so a turn where the heavy-hit tick,
// the plugged vent and the passive climb all fire prints a single stage line instead of four.
EncScript_Heatran_TurnClose::
	encsetvar 9, 0      @ TurnGuard
	encsetvar 10, 0     @ PlayerMove
	encsetvar 11, 0     @ BossMove
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Heatran_TurnClose_Done   @ Weakened: inert
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Heatran_TurnClose_Done          @ Spent: the mountain is out
	call EncScript_Heatran_FieldChip
	call EncScript_Heatran_CoreDrink
	call EncScript_Heatran_HeavyHit
	call EncScript_Heatran_StatusTick
	call EncScript_Heatran_PassiveTick
	call EncScript_Heatran_Announce
EncScript_Heatran_TurnClose_Done:
	return

// --- Phases ---

// 50% - MAGMA CORE, and the Mega Evolution. Mega Heatran is 175 base Sp. Atk against base Heatran's
// 130; at the level cap, behind sunlight, that is a spike the guard curve was never balanced against
// - so the form change is followed immediately by a raw -20% Sp. Atk write, which lands it near where
// it was. RecalcBattlerStats rebuilds stats from the new species on every form change, so the
// correction must run exactly once, on the path that actually changed shape and never on one that did
// not - hence the fail label skipping it.
// What the Mega keeps for free is the half of its stat line that reads as ARMOUR rather than damage:
// Sp. Def 106 -> 141, which is what the guard curve's 91/94 is sitting on top of. What it gains that
// the player can actually see is EncScript_Heatran_CoreDrink - fused with the mountain, it feeds on
// the mountain, and a dial parked hot now pays it rent. The phase's threat is still meant to come
// from the volcano rather than from a stat line; the Mega is what makes the volcano its.
// encformchange rather than encmegaevolve: encmegaevolve derives the destination from a held Mega
// Stone, which would mean handing the boss a Heatranite that Knock Off or Trick could take off it
// mid-fight, and that a caught Heatran would keep.
EncScript_Heatran_MagmaCore::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCHEATRANMAGMACORE
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_HEATRAN_MEGA, EncScript_Heatran_MagmaCore_Body, B_ANIM_MEGA_EVOLUTION
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, -20, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCHEATRANMEGA
	waitmessage B_WAIT_TIME_LONG
EncScript_Heatran_MagmaCore_Body:
	encsetmove ENC_TARGET_BOSS, 1, MOVE_OVERHEAT   @ replaces Lava Plume; its own Sp. Atk drop is the fight's theme in a move
	encsetvar 5, 2      @ PocketClock: the pockets come faster
	call EncScript_Heatran_Raise
	call EncScript_Heatran_Raise
	call EncScript_Heatran_Announce
	call EncScript_Heatran_ApplyStance
	call EncScript_Heatran_ApplyGuard
	return

// 25% - ERUPTION. The dial is slammed to Molten and climbs +2 every turn on its own, and the player
// has to drive it DOWN: Ice is -1, Water is -2 but shocks, rain is another -1, so holding it flat
// needs two of the three. Reaching 0 subsides it; reaching 5 is the Supernova. Both end spent, so a
// player who cannot out-cool the climb is badly hurt rather than locked out of the catch window.
EncScript_Heatran_Eruption::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCHEATRANERUPTION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ZMOVE_ACTIVATE
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Heatran_Eruption_Apply
	encsetvar 1, 3      @ Pressure
EncScript_Heatran_Eruption_Apply:
	call EncScript_Heatran_Announce
	call EncScript_Heatran_ApplyStance
	call EncScript_Heatran_ApplyGuard
	return

// 10% - the catch window, the mirror image of the Properties: block plus everything the fight built.
// Phase 3 retires every remaining trigger; all of them are gated Phase <= 2.
// Poke Balls are Blocked for the whole battle until this runs, and it can only run from Phase 2 at
// 10% or below. From here the engine's automatic catch-window damage guard owns the reduction -
// encsetdamagereduction is setting the value that guard will restore, not the live one.
// The status is cured because the guard covers the damage formula and a passive tick, but a Toxic
// standing on a boss that no longer has Survive: True is the one thing that could still kill the
// catch target while the window is open.
EncScript_Heatran_Weakened::
	encsetvar 0, 3      @ Phase
	encsetvar 1, 0      @ Pressure
	encsetvar 2, 0      @ LastPres
	encsetvar 3, 0      @ Vent
	encsetvar 4, 0      @ Pocket
	encsetvar 5, 0      @ PocketClock
	encsetvar 6, 0      @ Shock
	encsetvar 8, 0      @ Spent
	encsetvar 14, 0     @ Pulse
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	removeweather
	encformchange ENC_TARGET_BOSS, SPECIES_HEATRAN, EncScript_Heatran_Weakened_Body, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Heatran_Weakened_Body:
	encsetmove ENC_TARGET_BOSS, 1, MOVE_LAVA_PLUME   @ Overheat goes back with the form
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 40
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCHEATRANWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
