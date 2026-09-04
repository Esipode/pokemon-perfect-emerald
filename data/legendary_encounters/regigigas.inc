// Regigigas, "The Colossal Titan". Var indices, pinned by the always-true Conditions on
// EncScript_Regigigas_Intro:
// 0 Phase (0 Slumber / 1 The Titan Moves / 2 Full Awakening / 3 Weakened), 1 Awakening (0-5, the
// meter; nothing in the fight lowers it), 2 Rung (rung last APPLIED, stored as rung+1 so 0 means
// "nothing applied yet" - the ladder's idempotence and re-fire latch), 3 Clock (countdown to the
// next free rung), 4 Mark (boss HP% at the last rung), 5 Burst (scratch: Mark copied in,
// damage-since-mark out), 6 GrudgeSetup / 7 GrudgeSwitch / 8 GrudgeStatus (0 unarmed / 1 armed /
// 2 cashed), 9 Force (Continental Force countdown; 0 = not armed yet), 10 Exhaustion (0-5),
// 11 Collapse (collapse-window countdown), 12 TurnGuard, 13 MoveGuard (the player landed a damaging
// move this turn).
//
// The fight is one number the player paces. Chipping keeps the titan dormant, nearly unkillable and
// nearly harmless, and lets it arm grudges against everything the player does; bursting wakes it
// fast into something lethal that knows nothing about them. Neither is dominant and neither is free.

// --- Shared subroutines (call/return) ---

// Sole owner of Regigigas's Attack and Speed stages, its damage reduction and its moveset growth.
// Every rung writes its stages ABSOLUTELY - encchangestat clamps at the stage bounds, so -12 then
// +N lands on DEFAULT_STAT_STAGE+N-6 from wherever the stage had drifted. That is what makes the
// ladder self-healing and what lets the player fight it with Growl and Intimidate between rungs
// without ever permanently neutering it.
// Var(Rung) holds rung+1, so a fresh battle (0) is stale against rung 0 and the opening line lands.
EncScript_Regigigas_ApplyRung:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Regigigas_Rung5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Regigigas_Rung4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Regigigas_Rung3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Regigigas_Rung2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Regigigas_Rung1
// DORMANT. Speed -6, Attack -3, guard 92. No animation - a sleeping titan does not announce itself;
// the line states instead that the fight is going to be about how hard this thing is to hurt.
EncScript_Regigigas_Rung0:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regigigas_Rung_Done
	encsetvar 2, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 3
	encsetdamagereduction ENC_TARGET_BOSS, 92
	printstring STRINGID_ENCREGIGIGASRUNG0
	waitmessage B_WAIT_TIME_LONG
	return
// STIRRING. Still guard 92 - the first rung buys the player nothing but a warning.
EncScript_Regigigas_Rung1:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Regigigas_Rung_Done
	encsetvar 2, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 4
	encsetdamagereduction ENC_TARGET_BOSS, 92
	printstring STRINGID_ENCREGIGIGASRUNG1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return
EncScript_Regigigas_Rung2:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Regigigas_Rung_Done
	encsetvar 2, 3
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 4
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 5
	encsetdamagereduction ENC_TARGET_BOSS, 90
	printstring STRINGID_ENCREGIGIGASRUNG2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_DYNAMAX_GROWTH
	return
// AWAKENED - the arming window for the grudges closes here, and everything armed inside it is
// cashed on the way out. Heavy Slam replaces Pound: 420.0 kg is max power against almost anything.
EncScript_Regigigas_Rung3:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Regigigas_Rung_Done
	encsetvar 2, 4
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encsetdamagereduction ENC_TARGET_BOSS, 88
	printstring STRINGID_ENCREGIGIGASRUNG3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_DYNAMAX_GROWTH
	encsetmove ENC_TARGET_BOSS, 0, MOVE_HEAVY_SLAM
	printstring STRINGID_ENCREGIGIGASREMEMBERSWEIGHT
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Regigigas_Rung_Cash
EncScript_Regigigas_Rung4:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Regigigas_Rung_Done
	encsetvar 2, 5
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 7
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 8
	encsetdamagereduction ENC_TARGET_BOSS, 86
	printstring STRINGID_ENCREGIGIGASRUNG4
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	goto EncScript_Regigigas_Rung_Cash
// FULL AWAKENING. Crush Grip scales with the TARGET's current HP, so sending in a fresh body to eat
// a hit is punished specifically, and Earthquake costs it Protect - a fully awake titan does not
// defend. Continental Force arms here rather than at the 25% threshold, because the meter can reach
// 5 while the fight is still in Phase 1.
EncScript_Regigigas_Rung5:
	encjumpifvar CMP_EQUAL, 2, 6, EncScript_Regigigas_Rung_Done
	encsetvar 2, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 9
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 10
	encsetdamagereduction ENC_TARGET_BOSS, 84
	printstring STRINGID_ENCREGIGIGASRUNG5
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	encsetmove ENC_TARGET_BOSS, 1, MOVE_CRUSH_GRIP
	encsetmove ENC_TARGET_BOSS, 3, MOVE_EARTHQUAKE
	printstring STRINGID_ENCREGIGIGASREMEMBERSGRIP
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_GREATER_THAN, 9, 0, EncScript_Regigigas_Rung_Cash
	encsetvar 9, 6    @ Force: armed
EncScript_Regigigas_Rung_Cash:
	call EncScript_Regigigas_Cash
EncScript_Regigigas_Rung_Done:
	return

// THE COLOSSUS REMEMBERS. Cashed from inside ApplyRung rather than on its own trigger, so every
// route to rung 3 or above pays out correctly - the clock, a burst, the 50% jump straight to 3, or
// the 25% pin to 5. Each latch goes 1 -> 2, so a later re-application of the rung says nothing.
EncScript_Regigigas_Cash:
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_Regigigas_Cash_Switch
	encsetvar 6, 2    @ GrudgeSetup: the standing anti-setup rule is live from here
	printstring STRINGID_ENCREGIGIGASGRUDGESETUP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
EncScript_Regigigas_Cash_Switch:
	encjumpifvar CMP_NOT_EQUAL, 7, 1, EncScript_Regigigas_Cash_Status
	encsetvar 7, 2    @ GrudgeSwitch
	printstring STRINGID_ENCREGIGIGASGRUDGESWITCH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TURN_TRAP
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
EncScript_Regigigas_Cash_Status:
	encjumpifvar CMP_NOT_EQUAL, 8, 1, EncScript_Regigigas_Cash_Done
	encsetvar 8, 2    @ GrudgeStatus
	printstring STRINGID_ENCREGIGIGASGRUDGESTATUS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 0
EncScript_Regigigas_Cash_Done:
	return

// Every path that raises the meter goes through here, so the burst mark and the clock can never
// fall out of step with it. Awakening is capped at 5 rather than allowed to run past, because the
// rung branch reads "> 4" as rung 5.
EncScript_Regigigas_Awaken:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Regigigas_Awaken_Mark
	encaddvar 1, 1    @ Awakening
EncScript_Regigigas_Awaken_Mark:
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET    @ Mark: the burst window is per-rung
	call EncScript_Regigigas_ClockArm
	call EncScript_Regigigas_ApplyRung
	return

// The clock's re-arm. One higher than the gap it produces, because the resolution fires at 1:
// 5 is a free rung every fourth turn, 4 every third. One place, so retuning the pace is one literal.
EncScript_Regigigas_ClockArm:
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Regigigas_ClockArm_Fast
	encsetvar 3, 5    @ Slumber
	return
EncScript_Regigigas_ClockArm_Fast:
	encsetvar 3, 4    @ from 50% the titan stirs faster
	return

EncScript_Regigigas_NeutralDefences:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 6
	return

EncScript_Regigigas_NeutralAll:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	call EncScript_Regigigas_NeutralDefences
	return

// --- TurnOpen's countdowns, in the order they run ---

// The collapse window's heartbeat and countdown. The line runs every turn the window is open, so a
// player who missed the collapse message still cannot miss that the guard is down.
EncScript_Regigigas_CollapseTick:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Regigigas_CollapseTick_Done
	encjumpifvar CMP_LESS_THAN, 11, 2, EncScript_Regigigas_CollapseTick_Line
	encsubvar 11, 1
EncScript_Regigigas_CollapseTick_Line:
	printstring STRINGID_ENCREGIGIGASSTILLDOWN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Regigigas_CollapseTick_Done:
	return

// EXHAUSTION. +1 per turn Regigigas acts at full power (Continental Force adds another 2), and the
// two lines re-fire on every cycle without a latch var because the meter only ever rises inside a
// cycle and is zeroed by the collapse.
EncScript_Regigigas_ExhaustTick:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Regigigas_ExhaustTick_Done
	encaddvar 10, 1
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Regigigas_ExhaustTick_Slowing
	encjumpifvar CMP_EQUAL, 10, 4, EncScript_Regigigas_ExhaustTick_Faltering
	return
EncScript_Regigigas_ExhaustTick_Slowing:
	printstring STRINGID_ENCREGIGIGASSLOWING
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regigigas_ExhaustTick_Faltering:
	printstring STRINGID_ENCREGIGIGASFALTERING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Regigigas_ExhaustTick_Done:
	return

// CONTINENTAL FORCE's telegraph. Armed to 6, which is a five-turn cycle: five ticks bring it to 2,
// and the turn it sits at 2 is the turn it warns at the top and fires at the end. The player gets
// exactly one action with the knowledge - enough to heal, set up a sacrifice or take it
// deliberately, and not enough to make it a non-event.
EncScript_Regigigas_ForceTick:
	encjumpifvar CMP_LESS_THAN, 9, 3, EncScript_Regigigas_ForceTick_Warn
	encsubvar 9, 1
	return
EncScript_Regigigas_ForceTick_Warn:
	encjumpifvar CMP_NOT_EQUAL, 9, 2, EncScript_Regigigas_ForceTick_Done
	printstring STRINGID_ENCREGIGIGASFORCECHARGE
	waitmessage B_WAIT_TIME_LONG
	@ Totem Flare, not Tera Charge: Tera Charge blends the screen to white and only Tera Activate
	@ blends it back, so using it alone whites the battle out permanently.
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
EncScript_Regigigas_ForceTick_Done:
	return

// The dormant phase's recurring beat: a third of the time the colossus simply is not there yet.
// encsetrecharge with 1 at OnTurnStart costs it THIS turn's action and prints the engine's own
// "must recharge!" line underneath.
// Retired at Awakening 4, where the scripted strike takes over - the clock can carry the meter that
// high while the fight is still in Phase 0, and a titan that strikes first is not one that "does
// not stir". The two rolls are gated so they can never contradict each other on the same turn.
EncScript_Regigigas_DormantRoll:
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Regigigas_DormantRoll_Done
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Regigigas_DormantRoll_Done
	encjumpifchance 35, EncScript_Regigigas_DormantRoll_Skip
	return
EncScript_Regigigas_DormantRoll_Skip:
	printstring STRINGID_ENCREGIGIGASDOESNOTSTIR
	waitmessage B_WAIT_TIME_SHORT
	encsetrecharge ENC_TARGET_BOSS, 1
EncScript_Regigigas_DormantRoll_Done:
	return

// The scripted strike. There is no extra-action command, so the titan acting BEFORE the player is
// how "it acts twice" is delivered - the same felt experience, and it reads better on a colossus.
// Scripted encchangehp rather than a move, so Protect does not stop it. TurnOpen has already
// returned if the window is collapsed, so a collapsed titan is genuinely harmless.
EncScript_Regigigas_StrikeRoll:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Regigigas_StrikeRoll_Done
	encjumpifchance 35, EncScript_Regigigas_StrikeRoll_Hit
	return
EncScript_Regigigas_StrikeRoll_Hit:
	printstring STRINGID_ENCREGIGIGASSECONDSTRIKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_STOMP
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
EncScript_Regigigas_StrikeRoll_Done:
	return

// The free-rung clock. Held at 1 for AwakenTick to consume at OnTurnEnd, one checkpoint away.
EncScript_Regigigas_ClockTick:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Regigigas_ClockTick_Done
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Regigigas_ClockTick_Done
	encsubvar 3, 1
EncScript_Regigigas_ClockTick_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so this seeds only what must be non-zero: the clock and
// the burst mark. The closing ApplyRung lands rung 0's stats, its guard and its line.
// Slow Start is left alone on purpose - it halves Attack and Speed for five turns on top of rung 0,
// and the engine's own "Regigigas can't get it going!" is better opening dialogue than anything
// written for the purpose.
EncScript_Regigigas_Intro::
	call EncScript_Regigigas_ClockArm
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET    @ Mark
	printstring STRINGID_ENCREGIGIGASSLUMBERS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Regigigas_ApplyRung
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set and
// MoveGuard cleared immediately rather than at the tail, so no branch out of this script can leave
// either wrong.
EncScript_Regigigas_TurnOpen::
	flushtextbox
	encsetvar 12, 1    @ TurnGuard
	encsetvar 13, 0    @ MoveGuard: the strike-detector is armed again
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Regigigas_TurnOpen_Done   @ Weakened: inert
	call EncScript_Regigigas_CollapseTick
	encjumpifvar CMP_GREATER_THAN, 11, 0, EncScript_Regigigas_TurnOpen_Done  @ collapsed: nothing else
	call EncScript_Regigigas_ExhaustTick
	call EncScript_Regigigas_ForceTick
	call EncScript_Regigigas_DormantRoll
	call EncScript_Regigigas_StrikeRoll
	call EncScript_Regigigas_ClockTick
EncScript_Regigigas_TurnOpen_Done:
	return

// The turn's measurement, then the guard resets. Every countdown lives at TurnOpen instead.
EncScript_Regigigas_TurnClose::
	call EncScript_Regigigas_BurstCheck
	call EncScript_Regigigas_Rewind
	encsetvar 12, 0    @ TurnGuard
	encsetvar 13, 0    @ MoveGuard
	return

// The burst detector - the meter's second input and the one the player chooses. Mark holds the boss
// HP percentage at the last rung; the copy is consumed by ENC_SNAP_DAMAGE so the mark itself
// survives. ENC_SNAP_DAMAGE always writes (0 when nothing was dealt), so the comparison afterwards
// is safe with or without a fail label. Awaken re-marks, so the 12% window is per-rung rather than
// cumulative. Clock and burst can both resolve on the same turn - two rungs from one enormous hit,
// which is meant to feel like exactly that.
EncScript_Regigigas_BurstCheck:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Regigigas_BurstCheck_Done
	enccopyvar 5, 4    @ Burst = Mark
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE
	encjumpifvar CMP_LESS_THAN, 5, 12, EncScript_Regigigas_BurstCheck_Done
	call EncScript_Regigigas_Awaken
EncScript_Regigigas_BurstCheck_Done:
	return

// The player's lever. MoveGuard == 0 is exactly "no damaging move of the player's resolved this
// turn" - a status move, a switch, a Protect, a full-paralysis turn. One beat and one explicit
// trade: the clock goes back and Regigigas gets health for it. The meter itself never moves, which
// is what stops the stall being a lock; the cap is what stops a player banking twenty turns of
// clock; the regen is what stops it being free. It dies at 25% without a special case, because
// there is no meter left to pace.
EncScript_Regigigas_Rewind:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Regigigas_Rewind_Done
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Regigigas_Rewind_Done
	printstring STRINGID_ENCREGIGIGASFALTERS
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regigigas_ClockAdd
	printstring STRINGID_ENCREGIGIGASKNITS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
EncScript_Regigigas_Rewind_Done:
	return

// Clock += 2, capped at the phase's own armed value - a rewind can undo a tick and no more.
EncScript_Regigigas_ClockAdd:
	encaddvar 3, 2
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Regigigas_ClockAdd_Fast
	encjumpifvar CMP_GREATER_THAN, 3, 5, EncScript_Regigigas_ClockAdd_CapSlow
	return
EncScript_Regigigas_ClockAdd_CapSlow:
	encsetvar 3, 5
	return
EncScript_Regigigas_ClockAdd_Fast:
	encjumpifvar CMP_GREATER_THAN, 3, 4, EncScript_Regigigas_ClockAdd_CapFast
	return
EncScript_Regigigas_ClockAdd_CapFast:
	encsetvar 3, 4
	return

// Records that a damaging move of the player's resolved this turn, for TurnClose to read. That is
// the entire script; setting the var is also what disables the trigger for the rest of the turn.
EncScript_Regigigas_PlayerStruck::
	encsetvar 13, 1    @ MoveGuard
	return

// The three arms - silent, one line each. The reveal is at Awakening 3.
EncScript_Regigigas_ArmSetup::
	encsetvar 6, 1    @ GrudgeSetup
	return

EncScript_Regigigas_ArmSwitch::
	encsetvar 7, 1    @ GrudgeSwitch
	return

EncScript_Regigigas_ArmStatus::
	encsetvar 8, 1    @ GrudgeStatus
	return

// The clock resolving. Awaken re-arms it, so this trigger's own condition is false on the way out.
EncScript_Regigigas_AwakenTick::
	call EncScript_Regigigas_Awaken
	return

// The standing anti-setup rule, live once the setup grudge has been cashed. Attack, Sp. Attack and
// Speed go back to neutral through the same absolute write the ladder uses; the strip makes the
// trigger's own condition false, and the player is free to set up again and be shaken off again.
EncScript_Regigigas_CrushBoosts::
	printstring STRINGID_ENCREGIGIGASCRUSHESBOOSTS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, -12
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, 6
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, -12
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, 6
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -12
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 6
	return

// CONTINENTAL FORCE. Scripted encchangehp rather than a move, so Protect does not stop it - stated in
// the cheat-sheet rather than hidden. The recharge IS the exhaustion: stock engine, stock "must
// recharge!" line, and a genuine free turn for the player. Force is re-armed up front so the
// trigger cannot re-select inside the checkpoint.
EncScript_Regigigas_ContinentalForce::
	encsetvar 9, 6     @ Force: the next cycle starts now
	encaddvar 10, 2    @ Exhaustion
	printstring STRINGID_ENCREGIGIGASFORCEFIRE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_GIGA_IMPACT
	encchangehp ENC_TARGET_ALL_FOES, -35, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCREGIGIGASFORCEREELING
	waitmessage B_WAIT_TIME_SHORT
	encsetrecharge ENC_TARGET_BOSS, 2    @ at OnTurnEnd this costs it NEXT turn's action
	return

// COLLAPSE. The thing that makes the titan unbeatable is also the thing that makes it fall over:
// stats on the floor, guard 92 -> 55, a lost turn to open the window and three full turns of it.
// Force is re-armed so no Continental Force can land out of a collapse.
EncScript_Regigigas_Collapse::
	encsetvar 10, 0    @ Exhaustion
	encsetvar 11, 4    @ Collapse: three turns of window after the free one
	encsetvar 9, 6     @ Force
	printstring STRINGID_ENCREGIGIGASCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 3
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 4
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 4
	encsetdamagereduction ENC_TARGET_BOSS, 55
	encsetrecharge ENC_TARGET_BOSS, 2
	return

// The window closes. A SAFETY VALVE, not a rhythm to farm: the 25%-to-10% band is 15% of a health
// bar at guard 82, and a player who cannot close it inside one window needs a second attempt rather
// than a deadlock. Rung is made stale so ApplyRung re-lands rung 5 wholesale - stats, guard and its
// line - rather than reading as already applied.
EncScript_Regigigas_Recover::
	encsetvar 11, 0    @ Collapse
	encsetvar 2, 0     @ Rung: stale
	call EncScript_Regigigas_NeutralDefences
	printstring STRINGID_ENCREGIGIGASUPRIGHT
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Regigigas_ApplyRung
	return

// 50% - THE TITAN MOVES. The meter jumps by one and never lands below AWAKENED, which is also what
// guarantees every armed grudge cashes: a player who somehow held it at rung 0 for the whole first
// half gets all three at once, here. Awaken re-arms the clock through ClockArm, which now reads
// Phase 1 and hands back the faster cadence.
EncScript_Regigigas_TitanMoves::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCREGIGIGASTITANMOVES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_DYNAMAX_GROWTH
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Regigigas_TitanMoves_Meter
	printstring STRINGID_ENCREGIGIGASTEARSSCREENS
	waitmessage B_WAIT_TIME_LONG
EncScript_Regigigas_TitanMoves_Meter:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Regigigas_TitanMoves_Step
	encsetvar 1, 2    @ so the +1 below lands on AWAKENED
EncScript_Regigigas_TitanMoves_Step:
	call EncScript_Regigigas_Awaken
	return

// 25% - FULL AWAKENING. The meter is pinned at 5 and the clock retired; ApplyRung lands rung 5 and,
// if the meter had not already reached it in Phase 1, arms Continental Force on the way through.
EncScript_Regigigas_FullAwakening::
	encsetvar 0, 2    @ Phase
	encsetvar 3, 0    @ Clock: nothing left to pace
	encsetvar 1, 5    @ Awakening
	printstring STRINGID_ENCREGIGIGASFULLAWAKENING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Regigigas_ApplyRung
	return

// 10% - the catch window, the mirror image of the Properties: block plus everything the fight built.
// The trap and the Safeguard come off, and all four stat stages go back to neutral, so nothing the
// grudges or the ladder built outlives it. Phase 3 makes TurnOpen return through its leading branch,
// so no clock, strike, Force or exhaustion tick can restart after this point.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Regigigas_Weakened::
	encsetvar 0, 3     @ Phase
	encsetvar 3, 0     @ Clock
	encsetvar 9, 0     @ Force
	encsetvar 10, 0    @ Exhaustion
	encsetvar 11, 0    @ Collapse
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD
	call EncScript_Regigigas_NeutralAll
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCREGIGIGASWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
