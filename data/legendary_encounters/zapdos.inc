// Zapdos, "Storm Overload". Var indices, pinned by the
// always-true Conditions on EncScript_Zapdos_Intro:
// 0 Phase, 1 Charge, 2 Overload, 3 Crash, 4 Mark, 5 Answered, 6 Tick, 7 LastGuard,
// 8 TurnGuard, 9 HitGuard, 10 BossGuard, 11 SwitchGuard, 12 TickGuard.
//
// Charge (0-4) is the whole fight: it drives the guard/weather ladder (ApplyTier), Zapdos's Sp. Atk
// (one stage per Charge), and the lightning-strike loop (Storm). It rises from Zapdos's Electric
// moves (BossElectric), the player's Electric moves (ElectricHit), a 25% roll on any other hit
// (AnyHit) and every 3rd turn (TurnOpen); it falls when a Ground move earths it out - by 2
// (GroundHit) - or when a shed sleep/freeze costs it 1 (TurnOpen). At Charge 4 it OVERLOADS
// (EnterOverload): a strike every turn for three turns, then BURNS OUT (Burnout) - Charge to 0,
// self-damage, guard collapses for one turn (CrashEnd). Below 25% HP it locks into permanent
// overload (LastStand); below 10% while there, the storm dies and it becomes catchable (Weakened).

// --- Shared subroutines (call/return; nesting stays <= 2 here) ---

// Sole owner of the Charge-driven guard/weather ladder. Overload, the burnout crash, and the
// Phase 1/2 scripts each own the reduction themselves, so this returns untouched while any of them
// is active. Re-announces the reduction whenever the tier actually changes, in either direction.
EncScript_Zapdos_ApplyTier:
	encjumpifvar CMP_GREATER_THAN, 2, 0, EncScript_Zapdos_ApplyTier_Done   @ Overload owns the tier
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Zapdos_ApplyTier_Done   @ burnout crash owns the tier
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Zapdos_ApplyTier_Done   @ Last Stand / Weakened own the tier
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Zapdos_ApplyTier_Severe
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Zapdos_ApplyTier_Charged
	encsetdamagereduction ENC_TARGET_BOSS, 80
	removeweather
	encjumpifvar CMP_EQUAL, 7, 80, EncScript_Zapdos_ApplyTier_Done
	encjumpifvar CMP_LESS_THAN, 7, 80, EncScript_Zapdos_ApplyTier_CalmHarden
	encsetvar 7, 80
	printstring STRINGID_ENCZAPDOSGUARDSLACKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_CalmHarden:
	encsetvar 7, 80
	printstring STRINGID_ENCZAPDOSGUARDHARDENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_Charged:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	removeweather
	encjumpifvar CMP_EQUAL, 7, 85, EncScript_Zapdos_ApplyTier_Done
	encjumpifvar CMP_LESS_THAN, 7, 85, EncScript_Zapdos_ApplyTier_ChargedHarden
	encsetvar 7, 85
	printstring STRINGID_ENCZAPDOSGUARDSLACKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_ChargedHarden:
	encsetvar 7, 85
	printstring STRINGID_ENCZAPDOSGUARDHARDENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_Severe:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetweather BATTLE_WEATHER_RAIN, 0
	encjumpifvar CMP_EQUAL, 7, 90, EncScript_Zapdos_ApplyTier_Done
	encsetvar 7, 90
	printstring STRINGID_ENCZAPDOSGUARDHARDENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_ApplyTier_Done:
	return

// Charge +1 (capped at 4), one Sp. Atk stage per Charge, then the tier effect for the new value.
// Charge 4 falls through to EnterOverload.
EncScript_Zapdos_GainCharge:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Zapdos_GainCharge_Done   @ already at cap (4 = Overload)
	encaddvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Zapdos_GainCharge_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Zapdos_GainCharge_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Zapdos_GainCharge_3
	goto EncScript_Zapdos_EnterOverload
EncScript_Zapdos_GainCharge_1:
	printstring STRINGID_ENCZAPDOSSPARKS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_CHARGE
	waitanimation
	call EncScript_Zapdos_ApplyTier
	return
EncScript_Zapdos_GainCharge_2:
	printstring STRINGID_ENCZAPDOSCHARGE2
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_ApplyTier
	return
EncScript_Zapdos_GainCharge_3:
	printstring STRINGID_ENCZAPDOSSEVERESTORM
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Zapdos_ApplyTier
	playmoveanimation MOVE_RAIN_DANCE
	waitanimation
EncScript_Zapdos_GainCharge_Done:
	return

// Charge -1 (floored at 0), giving back one Sp. Atk stage, then the tier effect. Used by the
// sleep/freeze shed; the Ground lever uses LoseChargeBig below.
EncScript_Zapdos_LoseCharge:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Zapdos_LoseCharge_Nothing
	encsubvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	printstring STRINGID_ENCZAPDOSDISCHARGES
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	call EncScript_Zapdos_ApplyTier
	return
EncScript_Zapdos_LoseCharge_Nothing:
	printstring STRINGID_ENCZAPDOSNOTHINGTOEARTH
	waitmessage B_WAIT_TIME_SHORT
	return

// Ground move: Charge -2 (floored at 0), Sp. Atk given back to match, one message.
EncScript_Zapdos_LoseChargeBig:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Zapdos_LoseCharge_Nothing
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Zapdos_LoseChargeBig_One
	encsubvar 1, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -2
	goto EncScript_Zapdos_LoseChargeBig_Announce
EncScript_Zapdos_LoseChargeBig_One:
	encsubvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
EncScript_Zapdos_LoseChargeBig_Announce:
	printstring STRINGID_ENCZAPDOSDISCHARGES
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	call EncScript_Zapdos_ApplyTier
	return

// Charge 4: three turns of huge offense and an every-turn strike, on a soft (70) guard.
EncScript_Zapdos_EnterOverload:
	encsetvar 2, 4   @ Overload duration: OverloadTick steps 4->3->2->1 (3 strikes), then Burnout at 1
	encsetweather BATTLE_WEATHER_RAIN, 0
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encsetdamagereduction ENC_TARGET_BOSS, 70
	encsetvar 7, 70
	printstring STRINGID_ENCZAPDOSOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCZAPDOSUNSTABLE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ZAP_CANNON
	waitanimation
	return

// --- Trigger scripts ---

EncScript_Zapdos_Intro::
	encsetvar 7, 80   @ LastGuard: seed to the Properties reduction so the first tier change reads right
	printstring STRINGID_ENCZAPDOSINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Runs once at the top of every turn (TurnGuard gate): clears the per-turn re-entry guards, sheds
// sleep/freeze at the cost of a Charge, and ticks the every-3rd-turn passive charge. The shed and
// the passive tick are storm-phase behaviour; once Weakened the storm is dead and neither runs.
EncScript_Zapdos_TurnOpen::
	encsetvar 8, 1    @ TurnGuard
	encsetvar 9, 0    @ HitGuard
	encsetvar 10, 0   @ BossGuard
	encsetvar 11, 0   @ SwitchGuard
	encsetvar 12, 0   @ TickGuard
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zapdos_TurnOpen_Done   @ Weakened: storm is over
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Zapdos_TurnOpen_Shed
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Zapdos_TurnOpen_Shed
	goto EncScript_Zapdos_TurnOpen_Tick
EncScript_Zapdos_TurnOpen_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCZAPDOSSHAKESITOFF
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_THUNDER_SHOCK
	waitanimation
	encjumpifvar CMP_GREATER_THAN, 2, 0, EncScript_Zapdos_TurnOpen_Tick   @ overloaded: cured, but overload owns the tier
	call EncScript_Zapdos_LoseCharge
EncScript_Zapdos_TurnOpen_Tick:
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Zapdos_TurnOpen_Done   @ passive charge only cycles in the storm phase
	encaddvar 6, 1   @ Tick
	encjumpifvar CMP_LESS_THAN, 6, 3, EncScript_Zapdos_TurnOpen_Done
	encsetvar 6, 0
	printstring STRINGID_ENCZAPDOSSTORMBUILDS
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_GainCharge
EncScript_Zapdos_TurnOpen_Done:
	return

// Overload -> 1 is the sentinel Burnout keys on. Charge back to 0, the +6 from charging unwound,
// weather gone, a chunk of self-damage, and a one-turn 60 guard while Crash is set.
EncScript_Zapdos_Burnout::
	encsetvar 2, 0   @ Overload off
	encsetvar 1, 0   @ Charge 0
	encsetvar 6, 0   @ Tick 0
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6
	removeweather
	encsetdamagereduction ENC_TARGET_BOSS, 60
	encsetvar 7, 60
	encsetvar 3, 1   @ Crash: CrashEnd closes this window at end of turn
	printstring STRINGID_ENCZAPDOSBURNSOUT
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCZAPDOSREELING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EXPLOSION
	waitanimation
	encchangehp ENC_TARGET_BOSS, -10, ENC_AMOUNT_PERCENT
	return

// Resolves a pending strike, then lays the next one while Charge >= 3 and not overloaded. Doing
// both here keeps the cadence continuous and the OnTurnStart budget at 3.
EncScript_Zapdos_Storm::
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Zapdos_Storm_Lay
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Zapdos_Storm_Grounded
	printstring STRINGID_ENCZAPDOSLIGHTNINGSTRIKE
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER
	waitanimation
	goto EncScript_Zapdos_Storm_ClearMark
EncScript_Zapdos_Storm_Grounded:
	printstring STRINGID_ENCZAPDOSBOLTGROUNDED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_Storm_ClearMark:
	encsetvar 4, 0   @ Mark cleared
EncScript_Zapdos_Storm_Lay:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Zapdos_Storm_Done       @ Charge < 3: storm isn't marking
	encjumpifvar CMP_GREATER_THAN, 2, 0, EncScript_Zapdos_Storm_Done    @ overloaded: its own strike covers this
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Zapdos_Storm_Done    @ a mark is already in flight
	encsetvar 4, 2   @ Mark: laid this turn; TurnClose steps it to 1
	encsetvar 5, 0   @ Answered
	printstring STRINGID_ENCZAPDOSMARKS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_LOCK_ON
	waitanimation
EncScript_Zapdos_Storm_Done:
	return

// Below 25% HP: permanent overload, guard back up to 82, no more burnout window to exploit.
EncScript_Zapdos_LastStand::
	encsetvar 0, 1   @ Phase 1 (Last Stand)
	encsetvar 3, 0   @ Crash off
	encsetvar 1, 4   @ Charge 4 (Overload)
	encsetvar 2, 4   @ Overload duration: not stepped down while Phase >= 1
	encsetweather BATTLE_WEATHER_RAIN, 0
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetvar 7, 82
	printstring STRINGID_ENCZAPDOSLASTSTAND
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_THUNDER
	waitanimation
	return

// Below 10% HP while in Last Stand: the storm collapses, every guard and immunity drops, and
// Poke Balls are unblocked. The only catch window in the fight.
EncScript_Zapdos_Weakened::
	encsetvar 0, 2   @ Phase 2 (Weakened)
	encsetvar 2, 0   @ Overload off - a strike resolving after the storm died would be incoherent
	encsetvar 4, 0   @ Mark cleared
	encsetvar 1, 0   @ Charge 0
	encsetvar 3, 0   @ Crash off
	removeweather
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	encsetvar 7, 0   @ LastGuard
	printstring STRINGID_ENCZAPDOSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Zapdos itself uses an Electric move: Charge +1.
EncScript_Zapdos_BossElectric::
	encsetvar 10, 1   @ BossGuard
	call EncScript_Zapdos_GainCharge
	return

// The player's Electric move lands on Zapdos: Charge +1, and it's told outright the first time so
// feeding the boss with your own attack doesn't read as a bug.
EncScript_Zapdos_ElectricHit::
	encsetvar 9, 1   @ HitGuard
	printstring STRINGID_ENCZAPDOSDRINKSITIN
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_GainCharge
	return

// Any other move aimed at Zapdos: 25% chance to feed Charge.
EncScript_Zapdos_AnyHit::
	encsetvar 9, 1   @ HitGuard
	encjumpifchance 25, EncScript_Zapdos_AnyHit_Feed
	return
EncScript_Zapdos_AnyHit_Feed:
	printstring STRINGID_ENCZAPDOSFEEDSONIMPACT
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_GainCharge
	return

// A Ground move hits Zapdos. It can't damage Electric/Flying, so this turn deals no damage - that
// is the cost of the lever. It answers a pending mark, and against a fully overloaded Zapdos it
// forces an immediate discharge (remaining strikes cancelled, burnout pulled forward); otherwise
// it earths off 2 Charge.
EncScript_Zapdos_GroundHit::
	encsetvar 9, 1   @ HitGuard
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Zapdos_GroundHit_Answer
	goto EncScript_Zapdos_GroundHit_Resolve
EncScript_Zapdos_GroundHit_Answer:
	encsetvar 5, 1   @ Answered
EncScript_Zapdos_GroundHit_Resolve:
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Zapdos_GroundHit_Earth   @ Last Stand: overload is permanent
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Zapdos_GroundHit_Earth
	encsetvar 2, 1   @ Overload -> burnout sentinel; Burnout fires next turn start
	printstring STRINGID_ENCZAPDOSEARTHSOUT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	return
EncScript_Zapdos_GroundHit_Earth:
	call EncScript_Zapdos_LoseChargeBig
	return

// A fresh Pokemon enters: it answers a pending mark, and while Charge >= 3 the charged air tolls it
// for HP - cheaper than eating a strike, but not free.
EncScript_Zapdos_SwitchIn::
	encsetvar 11, 1   @ SwitchGuard
	encjumpifvar CMP_LESS_THAN, 4, 1, EncScript_Zapdos_SwitchIn_Toll
	encsetvar 5, 1   @ Answered
	printstring STRINGID_ENCZAPDOSLOSESITSTARGET
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_SwitchIn_Toll:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Zapdos_SwitchIn_Done
	printstring STRINGID_ENCZAPDOSCHARGEDAIR
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_SELF, -8, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER_SHOCK
	waitanimation
EncScript_Zapdos_SwitchIn_Done:
	return

// Overload strike (OnTurnEnd), then step the counter down - except in Last Stand, where overload
// is permanent. TickGuard stops the step-down from chasing itself through the counter in one pass.
EncScript_Zapdos_OverloadTick::
	encsetvar 12, 1   @ TickGuard
	printstring STRINGID_ENCZAPDOSSTRIKE
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER
	waitanimation
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Zapdos_OverloadTick_Done   @ Last Stand: no step-down
	encsubvar 2, 1
EncScript_Zapdos_OverloadTick_Done:
	return

// Closes the one-turn post-burnout soft window: guard back to the Charge-0 tier (60 -> 80).
EncScript_Zapdos_CrashEnd::
	encsetvar 3, 0   @ Crash off
	printstring STRINGID_ENCZAPDOSSTEADIES
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_ApplyTier
	return

// OnTurnEnd, priority 90 - runs last. Clears TurnGuard and steps a mark laid this turn (2 -> 1) so
// its warning lasts a full turn before EncScript_Zapdos_Storm resolves it next turn start.
EncScript_Zapdos_TurnClose::
	encsetvar 8, 0   @ TurnGuard
	encjumpifvar CMP_NOT_EQUAL, 4, 2, EncScript_Zapdos_TurnClose_Done
	encsetvar 4, 1
	printstring STRINGID_ENCZAPDOSAIRCRACKLES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_TurnClose_Done:
	return
