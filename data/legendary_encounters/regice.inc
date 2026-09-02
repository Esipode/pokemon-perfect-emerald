// Regice, "The Frozen Clock". Var indices, pinned by the
// always-true Conditions on EncScript_Regice_Intro:
// 0 Phase (0 The Frozen Clock / 1 Deep Freeze / 2 Frozen Tomb / 3 Weakened), 1 Chill (0-5),
// 2 LastChill (Chill tier last ANNOUNCED - the re-fire latch), 3 Rise (cadence countdown to the
// next Chill), 4 Tomb (0 open / 1 sealed), 5 Shed ("the cold no longer troubles Regice" has been
// said), 6 TurnGuard, 7 MoveGuard (per-turn guard shared by the two thaw triggers), 8 Rolled
// (per-turn guard for the lost-action roll), 9 Slowed (Speed stages the cold holds on the CURRENT
// player battler), 10 HpMark (boss HP% at turn open), 11 Sky (per-turn guard for the re-freeze).
//
// Articuno's Frost feeds a barrier you shatter and Celebi's Echo rewinds your damage. Regice's
// CHILL eats your TURNS, and it rises on a clock rather than on an action - nothing done to Regice
// slows it down, only the temperature does. Chill 1-2 stiffens both sides honestly; at 3 the field
// starts eating one side's action at random and Regice sheds the cold; at 5 the player loses a turn
// outright and Regice does not. The same value drives the damage reduction, so a stalled fight is a
// fight being lost.
// Four levers bring it down: a Fire move on Regice (-2), a Fighting move (-1), harsh sunlight
// standing at turn close (-1), and a heavy blow of 12% of Regice's HP in one turn (-1). Regice's
// only answer is the sky: it puts its hail back at the next turn open, so a sun cast buys one thaw.
// Every Chill change routes through ChillUp/ChillDown so the Speed stages, the boss mirroring and
// the ledger in Var(Slowed) can never drift out of step with the value itself.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction ladder, a pure function of Chill with no phase term - which is
// what lets the Chill callout double as the guard callout. Stops at 95 rather than the 99 cap:
// Absolute Zero should be frightening, not a window where the player can neither act nor damage.
EncScript_Regice_ApplyGuard:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regice_Guard95
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Regice_Guard94
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Regice_Guard93
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Regice_Guard91
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Regice_Guard89
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Regice_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Regice_Guard91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Regice_Guard93:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Regice_Guard94:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Regice_Guard95:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones. Falling back to Chill 0 resets the latch,
// so the whole build-up re-announces if the cold climbs again. Chill 5 stays on the tier-2 latch -
// Absolute Zero prints its own, louder line.
EncScript_Regice_ChillCue:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Regice_ChillCue_TierTwo
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Regice_ChillCue_TierOne
	encsetvar 2, 0    @ LastChill
	return
EncScript_Regice_ChillCue_TierOne:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regice_ChillCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCREGICECHILLONE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Regice_ChillCue_TierTwo:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Regice_ChillCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCREGICECHILLTWO
	waitmessage B_WAIT_TIME_LONG
EncScript_Regice_ChillCue_Done:
	return

// Called by every path that moves Chill, after ChillUp/ChillDown has run.
EncScript_Regice_ApplyChill:
	call EncScript_Regice_ApplyGuard
	call EncScript_Regice_ChillCue
	return

// One rung colder, and the only place the cold ever rises - so the line here is the guarantee that
// every single change to the guard ladder is announced, with the tier callout layered on top of it.
// Stat STAGES rather than a raw Speed cut, and a delta rather than an absolute set: a delta cannot
// silently eat the player's own Dragon Dance, the stages go away on a switch (the free lever Frozen
// Tomb later confiscates), and Haze clears the cold off both sides at once.
// Regice takes the drop with the field only through Chill 2. Above that it stops feeling the cold
// and the player does not - the fight saying out loud, one rung early, that the symmetry is over.
// Clear Body is the reason this works: encchangestat writes statStages directly rather than going
// through ChangeStatBuffs, so the encounter's own drop lands while every player-side attempt to
// slow Regice bounces off the ability.
EncScript_Regice_ChillUp:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Regice_ChillUp_Done
	encaddvar 1, 1
	printstring STRINGID_ENCREGICEDEEPENS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encaddvar 9, 1    @ Slowed
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Regice_ChillUp_Sheds
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -1
	return
EncScript_Regice_ChillUp_Sheds:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Regice_ChillUp_Done
	encsetvar 5, 1    @ Shed
	printstring STRINGID_ENCREGICESHEDS
	waitmessage B_WAIT_TIME_LONG
EncScript_Regice_ChillUp_Done:
	return

// One rung warmer. encsubvar on a u8 underflows to 255, so the floor is tested explicitly.
// The two sides are given their stage back on separate tests, because they can be out of step:
// Regice only ever took the rungs at Chill 1 and 2, so it only gets one back when the rung being
// undone is one of those, while the player's give-back is metered by Var(Slowed) - which a switch
// zeroes, so a thaw after a switch can never hand out Speed the cold never took.
EncScript_Regice_ChillDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_ChillDown_Done
	encsubvar 1, 1
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Regice_ChillDown_Player
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
EncScript_Regice_ChillDown_Player:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Regice_ChillDown_Done
	encsubvar 9, 1    @ Slowed
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 1
EncScript_Regice_ChillDown_Done:
	return

// The clock. Rise counts down and the cold advances on the turn it is already 0, so a re-arm of 2
// is a rise every third turn and a re-arm of 0 is a rise every turn. The re-arm happens before the
// cap test, so a capped clock still resets rather than sitting at 0 and re-firing.
// Phase 0 caps at 4: the player meets the Speed drop, the random lost actions and all four thaw
// levers before Absolute Zero is ever on the table.
EncScript_Regice_Clock:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Regice_Clock_Advance
	encsubvar 3, 1
	return
EncScript_Regice_Clock_Advance:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regice_Clock_ArmSlow
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Regice_Clock_ArmMid
	encsetvar 3, 0    @ Frozen Tomb: every turn
	goto EncScript_Regice_Clock_Rise
EncScript_Regice_Clock_ArmSlow:
	encsetvar 3, 2    @ The Frozen Clock: every third turn
	goto EncScript_Regice_Clock_Rise
EncScript_Regice_Clock_ArmMid:
	encsetvar 3, 1    @ Deep Freeze: every second turn
EncScript_Regice_Clock_Rise:
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Regice_Clock_Do
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Regice_Clock_Done
EncScript_Regice_Clock_Do:
	call EncScript_Regice_ChillUp
EncScript_Regice_Clock_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the clock needs seeding: the first Chill lands
// at the close of turn 3. The hail is permanent (turns = 0) and one-sided by ordinary type rules -
// Regice is pure Ice, so nothing has to be scripted for it to be the only thing standing in it.
// It is the background clock that says this fight cannot be waited out.
EncScript_Regice_Intro::
	encsetvar 3, 2    @ Rise
	printstring STRINGID_ENCREGICEAWAKENS
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_HAIL, 0
	printstring STRINGID_ENCREGICEHAILFIELD
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// TurnGuard is set immediately rather than at the tail so no branch out of this script can leave it
// re-armed. The HP mark is taken here and read at TurnClose, which is what makes the heavy-blow
// lever mean "this turn" no matter how many hits it took.
// The sleep line is the anti-cheese, stated once a turn and never explained: putting Regice under
// is a legal play that buys nothing, because none of this fight is Regice's action.
EncScript_Regice_TurnOpen::
	flushtextbox
	encsetvar 6, 1     @ TurnGuard
	encsetvar 7, 0     @ MoveGuard: the thaw levers are armed again
	encsetvar 8, 0     @ Rolled: the lost-action roll is armed again
	encsetvar 11, 0    @ Sky: the re-freeze is armed again
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_SET
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Regice_TurnOpen_Done   @ Weakened: inert
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Regice_TurnOpen_Sleeps
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Regice_TurnOpen_Done
	printstring STRINGID_ENCREGICECHILLRECUR
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regice_TurnOpen_Sleeps:
	printstring STRINGID_ENCREGICESLEEPS
	waitmessage B_WAIT_TIME_LONG
EncScript_Regice_TurnOpen_Done:
	return

// Regice puts its own sky back. This is what prices the sun lever: the sun pays out one rung at
// TurnClose and is gone by the time the player acts again, so a cast is worth exactly one thaw and
// no more. No Chill cost for the re-freeze itself - charging for it would make the lever worthless.
EncScript_Regice_Refreeze::
	flushtextbox
	encsetvar 11, 1    @ Sky
	removeweather
	encsetweather BATTLE_WEATHER_HAIL, 0
	printstring STRINGID_ENCREGICEREFREEZE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	return

// ABSOLUTE ZERO. encsetrecharge with 1 at OnTurnStart costs that battler THIS turn's action, which
// is the whole reason this resolves at turn open. Regice is not put on the timer, so the one turn
// the field takes from the player is a turn Regice still gets - the asymmetry the honest, symmetric
// Chill 1-2 was setting up.
// B_ANIM_TRICK_ROOM rather than a weather loop: it is a whole-field warp, which is what this is.
// The 8% heal is the cold preserving what stands in it.
// It then walks back to Chill 2, not 0. The field cannot hold absolute zero, but it does not go
// back to room temperature either - a recoverable position rather than a reset, and in Phase 2 it
// also opens the Tomb for one turn, handing the player their switch window right after the worst
// beat in the fight.
EncScript_Regice_AbsoluteZero::
	flushtextbox
	printstring STRINGID_ENCREGICEABSOLUTEZERO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	statusanimation BS_PLAYER1, STATUS1_FREEZE
	encsetrecharge ENC_TARGET_ALL_FOES, 1
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ApplyChill
	return

// Chill 3-4: the field eats one side's action. At 3 it only happens half the time; at 4 it always
// does, and either side can be the one that loses the turn - Regice is not exempt until Chill 5.
// statusanimation is purely cosmetic (Cmd_statusanimation never writes status1), so the field looks
// like it froze somebody without any status being applied that a Lum Berry could answer.
EncScript_Regice_FrozenAction::
	flushtextbox
	encsetvar 8, 1    @ Rolled
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Regice_FrozenAction_Roll
	encjumpifchance 50, EncScript_Regice_FrozenAction_Roll
	return
EncScript_Regice_FrozenAction_Roll:
	encjumpifchance 50, EncScript_Regice_FrozenAction_Boss
	printstring STRINGID_ENCREGICEFROZENYOU
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_PLAYER1, STATUS1_FREEZE
	encsetrecharge ENC_TARGET_ALL_FOES, 1
	return
EncScript_Regice_FrozenAction_Boss:
	printstring STRINGID_ENCREGICEFROZENBOSS
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_OPPONENT1, STATUS1_FREEZE
	encsetrecharge ENC_TARGET_BOSS, 1
	return

// The two move-driven thaws, sharing the one MoveGuard charge. Fire is worth double, which is what
// makes a Fire attacker the clean answer to the clock - and why Ancient Power is in Regice's
// moveset, so bringing one is not free.
EncScript_Regice_ThawFire::
	encsetvar 7, 1    @ MoveGuard
	printstring STRINGID_ENCREGICETHAWFIRE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ApplyChill
	return

EncScript_Regice_ThawStrike::
	encsetvar 7, 1    @ MoveGuard
	printstring STRINGID_ENCREGICETHAWSTRIKE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ApplyChill
	return

// The cold's grip leaves with the mon it was on: stat stages are per-battler and reset on a switch,
// so the ledger has to reset with them or a later thaw would hand the incoming mon free Speed.
EncScript_Regice_ColdLetsGo::
	encsetvar 9, 0    @ Slowed
	return

// TurnClose owns every Chill change that is not move-driven, in a fixed order: the two passive
// thaws first, then the clock. Ticking the clock here and consuming the lost action at TurnOpen is
// the documented split - a timer that ticks and resolves at the same checkpoint chases itself
// through every state in one dispatch and fires instantly.
EncScript_Regice_TurnClose::
	encsetvar 6, 0    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Regice_TurnClose_Done   @ Weakened: inert
	@ Thaw: harsh sunlight standing at the close of the turn. jumpifhalfword rather than
	@ jumpifweatheraffected - the latter reads gBattlerAttacker, which is stale at a checkpoint.
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_SUN, EncScript_Regice_TurnClose_Blow
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_TurnClose_Blow
	printstring STRINGID_ENCREGICETHAWSUN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	call EncScript_Regice_ChillDown
	@ Thaw: a heavy blow. Measured against the mark TurnOpen took, so it reads "you took 12% off it
	@ this turn" however many hits that took. Deliberately not gated on the physical category
	@ despite the flavour: against 200 base Sp. Def a special attacker needs the lever more, not
	@ less, and gating it would make the lever dead weight for half the rosters that show up.
EncScript_Regice_TurnClose_Blow:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_TurnClose_Clock
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Regice_TurnClose_Clock
	encjumpifvar CMP_LESS_THAN, 10, 12, EncScript_Regice_TurnClose_Clock
	printstring STRINGID_ENCREGICETHAWSTRIKE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regice_ChillDown
EncScript_Regice_TurnClose_Clock:
	call EncScript_Regice_Clock
	call EncScript_Regice_ApplyChill
EncScript_Regice_TurnClose_Done:
	return

// The Tomb. encsettrapped writes the same volatile Mean Look sets with Regice as the trapper, so
// Ghost-types walk out for free (B_GHOSTS_ESCAPE >= GEN_6), a Shed Shell still works, and Regice
// fainting releases everything on its own. Sealing costs the player the switch that was the free
// way to shake the Speed stages off - and in Phase 2 the clock re-seals it every turn, so the loop
// to find is: thaw to break out, switch while the window is open, thaw again.
EncScript_Regice_TombSeal::
	encsetvar 4, 1    @ Tomb
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCREGICETOMBSEAL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	return

EncScript_Regice_TombRelease::
	encsetvar 4, 0    @ Tomb
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	printstring STRINGID_ENCREGICETOMBBREAK
	waitmessage B_WAIT_TIME_SHORT
	return

// 50% - DEEP FREEZE. The ladder is the same ladder and the levers are the same levers; the only
// thing that changes is how fast the cold arrives, and that the cap comes off so Absolute Zero is
// finally on the table. Rise is set to 1 rather than left alone so the new cadence is felt at once.
EncScript_Regice_DeepFreeze::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCREGICEDEEPFREEZE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	encsetvar 3, 1    @ Rise: every second turn from here
	return

// 20% - FROZEN TOMB. The clock goes to every turn and Chill 3 now seals the arena. The stat bump is
// there because one more rung of the guard ladder would not be felt at this point, and the phase
// has to land as something Regice DID.
// Survive is set here rather than in Properties: so the AI reads Regice as killable for four fifths
// of the fight, and from this point it guarantees the player reaches the catch window instead of
// losing it to one oversized hit through a band only 10% of max HP wide. The Immunities: list
// closes the fixed-damage, Perish Song and Destiny Bond holes Survive does not.
EncScript_Regice_FrozenTomb::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCREGICEFROZENTOMBPHASE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TURN_TRAP
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encsetvar 3, 0    @ Rise: every turn from here
	return

// 10% - the catch window. Walking Chill down to 0 through the same subroutine that raised it is
// what hands every held Speed stage back on both sides without the script having to remember how
// many there were. From this point the engine's automatic catch-window damage guard keeps the catch
// target alive; encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Regice_Weakened::
	encsetvar 0, 3    @ Phase
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 4, 0    @ Tomb
	encsetvar 3, 0    @ Rise
	encsetvar 2, 0    @ LastChill
EncScript_Regice_Weakened_Thaw:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_Weakened_Release
	call EncScript_Regice_ChillDown
	goto EncScript_Regice_Weakened_Thaw
EncScript_Regice_Weakened_Release:
	removeweather
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCREGICEWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
