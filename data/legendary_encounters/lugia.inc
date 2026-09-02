// Lugia, "The Storm Beneath the Sea". Var indices, pinned by
// the always-true Conditions on EncScript_Lugia_Intro:
// 0 Phase (0 Guardian of the Sea / 1 The Sea Erupts / 2 Ocean's Wrath / 3 Weakened), 1 Storm (0-5),
// 2 Strain (0-6), 3 Eye (0 closed, else 2->1 countdown), 4 Hold (Maelstrom cycle timer; 0 idle,
// 3 surface, 2 deep sea, 1 strike), 5 Break (turns of collapsed guard left), 6 LastGuard (reduction
// last ANNOUNCED), 7 LastStorm (storm tier last APPLIED), 8 TurnGuard, 9 Rise (per-turn guard shared
// by the two storm-rise triggers), 10 Vent, 11 Swell, 12 HpMark (boss HP percentage at turn open;
// ENC_SNAP_DAMAGE turns it into "lost this turn"), 13 Prev (ApplyGuard's before-value scratch).
//
// Storm is a ladder of real battle weather that Lugia climbs itself by attacking, and the fight is
// built on an inversion: calm Lugia is SOFTER than storming Lugia but never tires, while storming
// Lugia is nearly untouchable and chips the field every turn - and accrues Strain. Capping Strain
// collapses the guard to 60 for two turns, which is the only place real damage happens. The
// Maelstrom cycle closes on its own into a restful Eye that sheds Strain, so the player is racing
// the cycle, not surviving it. Turtling through a Maelstrom is survivable and completely useless.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. Prev holds the value last announced so each
// leaf can write LastGuard before comparing; there is no UI for damage reduction, so a single
// showing would be trivially missed and the line has to re-fire on every real change.
EncScript_Lugia_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Lugia_ApplyGuard_Done   @ weakened owns its own guard
	enccopyvar 13, 6
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Lugia_Guard70    @ Strain collapse
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Lugia_Guard89    @ the Eye is a door, not a window
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Lugia_Guard_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Lugia_Guard_P1
	@ Phase 0 caps the ladder at Storm 3, so it only has the two rungs.
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Lugia_Guard86
	goto EncScript_Lugia_Guard80
EncScript_Lugia_Guard_P1:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Lugia_Guard90
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Guard88
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Lugia_Guard84
	goto EncScript_Lugia_Guard78
EncScript_Lugia_Guard_P2:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Lugia_Guard89
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Guard87
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Lugia_Guard82
	goto EncScript_Lugia_Guard74

EncScript_Lugia_Guard70:
	encsetdamagereduction ENC_TARGET_BOSS, 70
	encsetvar 6, 70
	encjumpifvar CMP_EQUAL, 13, 70, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 70, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard74:
	encsetdamagereduction ENC_TARGET_BOSS, 74
	encsetvar 6, 74
	encjumpifvar CMP_EQUAL, 13, 74, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 74, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 6, 78
	encjumpifvar CMP_EQUAL, 13, 78, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 78, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 6, 80
	encjumpifvar CMP_EQUAL, 13, 80, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 80, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard82:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetvar 6, 82
	encjumpifvar CMP_EQUAL, 13, 82, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 82, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 6, 84
	encjumpifvar CMP_EQUAL, 13, 84, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 84, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 6, 86
	encjumpifvar CMP_EQUAL, 13, 86, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 86, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard87:
	encsetdamagereduction ENC_TARGET_BOSS, 87
	encsetvar 6, 87
	encjumpifvar CMP_EQUAL, 13, 87, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 87, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 6, 88
	encjumpifvar CMP_EQUAL, 13, 88, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 88, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	encsetvar 6, 89
	encjumpifvar CMP_EQUAL, 13, 89, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 89, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 6, 90
	encjumpifvar CMP_EQUAL, 13, 90, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 90, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken

EncScript_Lugia_ApplyGuard_Thicken:
	printstring STRINGID_ENCLUGIATHICKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Lugia_ApplyGuard_Slacken:
	printstring STRINGID_ENCLUGIASLACKENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Lugia_ApplyGuard_Done:
	return

// The weather half of the ladder, latched on LastStorm so it is safe to call unconditionally - a
// tier that has not moved returns immediately, which is what lets the player's own weather survive
// a quiet turn. The removeweather first is load-bearing: TryChangeBattleWeather refuses any change
// while primal weather is up, so stepping DOWN from Tempest or Maelstrom is impossible without
// clearing first. RemoveAllWeather does clear primal, and prints nothing on its own.
EncScript_Lugia_ApplyStorm:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Lugia_Storm0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Lugia_Storm1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Lugia_Storm2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Lugia_Storm3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Storm4
	goto EncScript_Lugia_Storm5

EncScript_Lugia_Storm0:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetvar 7, 0
	printstring STRINGID_ENCLUGIACALM
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_ApplyStorm_Guard
EncScript_Lugia_Storm1:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetvar 7, 1
	printstring STRINGID_ENCLUGIABREEZE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_ApplyStorm_Guard
	@ From here the weather does the work no scripted modifier has to: rain boosts Lugia's Hydro Pump
	@ and makes its Thunder unmissable.
EncScript_Lugia_Storm2:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_RAIN, 0
	encsetvar 7, 2
	printstring STRINGID_ENCLUGIARAIN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	goto EncScript_Lugia_ApplyStorm_Guard
EncScript_Lugia_Storm3:
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_RAIN, 0
	encsetvar 7, 3
	printstring STRINGID_ENCLUGIASTORM
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	goto EncScript_Lugia_ApplyStorm_Guard
	@ Strong Winds clamps any >= 2x multiplier against a Flying defender back to neutral, so Tempest
	@ cancels exactly Lugia's Flying weaknesses - Electric, Ice and Rock stop being super effective -
	@ and leaves Ghost and Dark still hitting for 2x through its Psychic half. The player's coverage
	@ is rearranged, not walled. It is also primal-class, so the vent door shuts here.
EncScript_Lugia_Storm4:
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_STRONG_WINDS, 0
	encsetvar 7, 4
	printstring STRINGID_ENCLUGIATEMPEST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	goto EncScript_Lugia_ApplyStorm_Guard
EncScript_Lugia_Storm5:
	encjumpifvar CMP_EQUAL, 7, 5, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_RAIN_PRIMAL, 0
	encsetvar 7, 5
	printstring STRINGID_ENCLUGIASEAWEATHER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
EncScript_Lugia_ApplyStorm_Guard:
	call EncScript_Lugia_ApplyGuard
EncScript_Lugia_ApplyStorm_Done:
	return

// Clamped +1 against the phase cap - phase 0 stops at Storm 3, so the Tempest and the Maelstrom are
// things The Sea Erupts unlocks rather than things turn three can hand out.
EncScript_Lugia_RaiseStorm:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Lugia_RaiseStorm_Early
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Lugia_RaiseStorm_Done
	goto EncScript_Lugia_RaiseStorm_Rise
EncScript_Lugia_RaiseStorm_Early:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Lugia_RaiseStorm_Done
EncScript_Lugia_RaiseStorm_Rise:
	encaddvar 1, 1
	call EncScript_Lugia_ApplyStorm
EncScript_Lugia_RaiseStorm_Done:
	return

// Clamped 0-6. Silent: the recurring "its wings falter" cue lives in TurnOpen, so the player gets
// one reading a turn instead of one per point.
EncScript_Lugia_GainStrain:
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Lugia_GainStrain_Done
	encaddvar 2, 1
EncScript_Lugia_GainStrain_Done:
	return

EncScript_Lugia_LoseStrain:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Lugia_LoseStrain_Done
	encsubvar 2, 1
EncScript_Lugia_LoseStrain_Done:
	return

// The end-of-turn field chip, called from TurnClose. Scripted enchangehp rather than a move, so
// Protect and screens do not stop it. ENC_TARGET_ALL_FOES rather than a single slot: doubles-safe
// for free, and it skips a fainted member silently. A Water-type foe is spared outright, which is
// the one clean answer to the Maelstrom that does not cost the player their storm progress.
EncScript_Lugia_Maelstrom:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Lugia_Maelstrom_Done
	jumpiftype BS_PLAYER1, TYPE_WATER, EncScript_Lugia_Maelstrom_Spared
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Lugia_Maelstrom_Chip5
	jumpiftype BS_PLAYER1, TYPE_FLYING, EncScript_Lugia_Maelstrom_Flying
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Maelstrom_Chip7
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_Show
	@ Gravity is the only real grounding field state and it would ground Lugia too, opening it to
	@ Ground moves. A Flying-type foe torn out of the air by the wind is the same fiction, one
	@ jumpiftype deep, and it costs the encounter nothing.
EncScript_Lugia_Maelstrom_Flying:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Maelstrom_Chip11
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_ShowFlying
EncScript_Lugia_Maelstrom_Chip11:
	enchangehp ENC_TARGET_ALL_FOES, -7, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_ShowFlying
EncScript_Lugia_Maelstrom_Chip7:
	enchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_Show
EncScript_Lugia_Maelstrom_Chip5:
	enchangehp ENC_TARGET_ALL_FOES, -3, ENC_AMOUNT_PERCENT
EncScript_Lugia_Maelstrom_Show:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCLUGIACHIP
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_Maelstrom_Acc
EncScript_Lugia_Maelstrom_ShowFlying:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCLUGIACHIPFLYING
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_Maelstrom_Acc
EncScript_Lugia_Maelstrom_Spared:
	printstring STRINGID_ENCLUGIACHIPSPARED
	waitmessage B_WAIT_TIME_SHORT
	@ Accuracy erosion is Maelstrom-only, and it is answerable - Haze, Mist, or riding the cycle out
	@ to the Eye, which hands the stages back with a switch.
EncScript_Lugia_Maelstrom_Acc:
	encjumpifvar CMP_NOT_EQUAL, 1, 5, EncScript_Lugia_Maelstrom_Done
	encchangestat ENC_TARGET_ALL_FOES, STAT_ACC, -1
	printstring STRINGID_ENCLUGIAERODES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Lugia_Maelstrom_Done:
	return

// The payoff. Sixty is the softest state in the fight by a wide margin - roughly 2.9x the damage
// the player gets through a Tempest guard of 92 - and it is only reachable by attacking hard in the
// worst conditions the fight offers. ApplyStorm clears the weather on the way down; the explicit
// ApplyGuard after it covers the case where the tier did not actually move, and is silent when the
// tail call already announced the 70.
EncScript_Lugia_Collapse:
	printstring STRINGID_ENCLUGIACOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1   @ Storm knocked back to Breeze
	encsetvar 2, 0   @ Strain
	encsetvar 3, 0   @ Eye
	encsetvar 4, 0   @ Hold
	encsetvar 5, 2   @ Break: two full turns of collapsed guard
	enchangehp ENC_TARGET_BOSS, -7, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	call EncScript_Lugia_ApplyStorm
	call EncScript_Lugia_ApplyGuard
	return

// The conditional status immunity: while Storm >= 2 the sea will not let Lugia rest, so sleep and
// freeze are shed at the top of every turn - but below Storm 2 they stick. Sleep-locking Lugia is
// possible, and the price is venting the storm first, which is exactly the trade the fight is about.
// curestatus clears everything, so sleeping a storming Lugia also throws away the player's Toxic.
EncScript_Lugia_ShedStatus:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Lugia_ShedStatus_Done
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Lugia_ShedStatus_Cure
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Lugia_ShedStatus_Cure
	goto EncScript_Lugia_ShedStatus_Done
EncScript_Lugia_ShedStatus_Cure:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCLUGIASHEDS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Lugia_ShedStatus_Done:
	return

// --- Trigger scripts ---

// LastGuard is seeded to the Properties reduction so the first real change reads as a change. The
// second line is the hint that the weather is a second health bar, and that Lugia owns it.
EncScript_Lugia_Intro::
	encsetvar 6, 86    @ LastGuard
	encsetvar 7, 0     @ LastStorm
	encsetvar 12, 100  @ HpMark
	printstring STRINGID_ENCLUGIAINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCLUGIASKY
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn
// re-entry guards, marks the boss HP the Strain race is measured against, then keeps the three
// recurring callouts running - the collapsed guard, the Eye, and the standing Strain cue.
EncScript_Lugia_TurnOpen::
	flushtextbox
	encsetvar 8, 1    @ TurnGuard
	encsetvar 9, 0    @ Rise
	encsetvar 10, 0   @ Vent
	encsetvar 14, 0   @ Cycle
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Lugia_TurnOpen_Done   @ weakened: inert
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_SET
	call EncScript_Lugia_ShedStatus
	call EncScript_Lugia_ApplyStorm
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Lugia_TurnOpen_Eye
	printstring STRINGID_ENCLUGIAUNSTEADY
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_TurnOpen_Done
EncScript_Lugia_TurnOpen_Eye:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Lugia_TurnOpen_Strain
	printstring STRINGID_ENCLUGIAEYECALM
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_TurnOpen_Done
EncScript_Lugia_TurnOpen_Strain:
	encjumpifvar CMP_GREATER_THAN, 2, 4, EncScript_Lugia_TurnOpen_StrainHigh
	encjumpifvar CMP_LESS_THAN, 2, 3, EncScript_Lugia_TurnOpen_Done
	printstring STRINGID_ENCLUGIASTRAIN
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_TurnOpen_Done
EncScript_Lugia_TurnOpen_StrainHigh:
	printstring STRINGID_ENCLUGIASTRAINHIGH
	waitmessage B_WAIT_TIME_SHORT
EncScript_Lugia_TurnOpen_Done:
	return

// Hold 2. encsetprotect gives Lugia a real Protect, so Feint, never-miss moves and contact
// punishment all resolve exactly as they would against one, and it expires on its own. Lugia still
// attacks this turn: it is pure pressure, and the player's job is to heal, switch or set up before
// the strike lands next turn.
EncScript_Lugia_DeepSea::
	flushtextbox
	encsetvar 14, 1   @ Cycle
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCLUGIADIVE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_DIVE
	printstring STRINGID_ENCLUGIACHURN
	waitmessage B_WAIT_TIME_LONG
	return

// Hold 1. Scripted damage rather than a move, so Protect and screens do not stop it either.
EncScript_Lugia_Strike::
	flushtextbox
	encsetvar 14, 1   @ Cycle
	printstring STRINGID_ENCLUGIASTRIKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SURF
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	call EncScript_Lugia_RaiseStorm
	call EncScript_Lugia_GainStrain
	return

// Aeroblast is the ladder's biggest single push. Rise is the shared per-turn guard, so a turn that
// climbs two rungs cannot also climb a third off the surge trigger.
EncScript_Lugia_StormAeroblast::
	encsetvar 9, 1   @ Rise
	call EncScript_Lugia_RaiseStorm
	call EncScript_Lugia_RaiseStorm
	return

// Hydro Pump and Thunder each feed the storm one rung. Psychic feeds it nothing, which is the free
// turn the ladder occasionally hands back.
EncScript_Lugia_StormSurge::
	encsetvar 9, 1   @ Rise
	call EncScript_Lugia_RaiseStorm
	return

// The panic button. Venting from Storm 2-3 always lands at 0-1, which is no weather at all - so it
// washes the player's own weather away in the same breath and costs every point of progress toward
// a Strain collapse. It buys safety, and that is all it buys.
EncScript_Lugia_Vent::
	encsetvar 10, 1   @ Vent
	printstring STRINGID_ENCLUGIAVENT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_DEFOG
	encsubvar 1, 2
	call EncScript_Lugia_ApplyStorm
	return

// The Maelstrom's answer to pivot stalling.
EncScript_Lugia_Undertow::
	encsetvar 11, 1   @ Swell
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCLUGIAUNDERTOW
	waitmessage B_WAIT_TIME_SHORT
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown and the whole Strain economy, in an order that matters: Break steps down FIRST so a
// collapse armed further down this same script gets both its turns, and the Eye is checked before
// the Hold so the two halves of the cycle can never run in one dispatch.
EncScript_Lugia_TurnClose::
	encsetvar 8, 0    @ TurnGuard
	encsetvar 11, 0   @ Swell
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Lugia_TurnClose_Phase
	encsubvar 5, 1
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Lugia_TurnClose_Phase
	call EncScript_Lugia_ApplyGuard   @ the guard coming back up re-announces itself
EncScript_Lugia_TurnClose_Phase:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Lugia_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Lugia_TurnClose_Hold
	@ The Eye: no chip, no Strain gain, a small mend and a 94 guard. It is a heal-and-reposition
	@ window for both sides, deliberately too hard to be farmed as a damage window.
	enchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCLUGIAEYEMENDS
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Lugia_LoseStrain
	call EncScript_Lugia_LoseStrain
	encsubvar 3, 1
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Lugia_TurnClose_Done
	printstring STRINGID_ENCLUGIAEYECLOSES
	waitmessage B_WAIT_TIME_LONG
	@ Spending the Eye on a weather move is the reward: the storm comes back one rung instead of
	@ three or four, because the sky is already answering to somebody else.
	jumpifhalfword CMP_NOT_EQUAL, gBattleWeather, B_WEATHER_NONE, EncScript_Lugia_TurnClose_EyeHeld
	encsetvar 1, 3
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Lugia_TurnClose_EyeApply
	encsetvar 1, 4
	goto EncScript_Lugia_TurnClose_EyeApply
EncScript_Lugia_TurnClose_EyeHeld:
	encsetvar 1, 1
EncScript_Lugia_TurnClose_EyeApply:
	call EncScript_Lugia_ApplyStorm
	goto EncScript_Lugia_TurnClose_Done
EncScript_Lugia_TurnClose_Hold:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Lugia_TurnClose_Chip
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Lugia_TurnClose_Chip
	printstring STRINGID_ENCLUGIAEYEOPENS
	waitmessage B_WAIT_TIME_LONG
	encsetvar 3, 2   @ Eye
	encsetvar 1, 0   @ Storm: the cycle ends on Lugia's terms, not the player's
	call EncScript_Lugia_ApplyStorm
	goto EncScript_Lugia_TurnClose_Done
EncScript_Lugia_TurnClose_Chip:
	call EncScript_Lugia_Maelstrom
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Lugia_TurnClose_Cap
	call EncScript_Lugia_GainStrain
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Lugia_TurnClose_Heavy
	call EncScript_Lugia_GainStrain
	@ The race itself: ENC_SNAP_DAMAGE turns HpMark from "boss HP at turn open" into "percentage lost
	@ this turn", which is the only way to ask that question against a LevelCap boss whose max HP is
	@ unknown at authoring time. The failInstr fires when nothing was lost.
EncScript_Lugia_TurnClose_Heavy:
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE, EncScript_Lugia_TurnClose_Cap
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Lugia_TurnClose_HeavyP2
	encjumpifvar CMP_LESS_THAN, 12, 6, EncScript_Lugia_TurnClose_Cap
	call EncScript_Lugia_GainStrain
	call EncScript_Lugia_GainStrain
	goto EncScript_Lugia_TurnClose_Cap
EncScript_Lugia_TurnClose_HeavyP2:
	encjumpifvar CMP_LESS_THAN, 12, 4, EncScript_Lugia_TurnClose_Cap
	call EncScript_Lugia_GainStrain
	call EncScript_Lugia_GainStrain
	call EncScript_Lugia_GainStrain
EncScript_Lugia_TurnClose_Cap:
	encjumpifvar CMP_LESS_THAN, 2, 6, EncScript_Lugia_TurnClose_Arm
	call EncScript_Lugia_Collapse
	goto EncScript_Lugia_TurnClose_Done
	@ Arming the Maelstrom cycle. Checked last so a collapse in this same dispatch has already zeroed
	@ Storm out of range, and gated on Eye == 0 so the cycle cannot re-arm the instant it ends.
EncScript_Lugia_TurnClose_Arm:
	encjumpifvar CMP_NOT_EQUAL, 1, 5, EncScript_Lugia_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Lugia_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Lugia_TurnClose_Done
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Lugia_TurnClose_Done
	encsetvar 4, 3   @ Hold: surface, deep sea, strike
	printstring STRINGID_ENCLUGIAMAELSTROM
	waitmessage B_WAIT_TIME_LONG
EncScript_Lugia_TurnClose_Done:
	return

// Phase 1 at 50% HP. The storm cap comes off, so Tempest and the Maelstrom cycle become reachable,
// and Strain starts accruing - the loop is learned with half the fight still left rather than sprung
// at the end.
EncScript_Lugia_SeaErupts::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCLUGIASEAERUPTS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Lugia_RaiseStorm
	call EncScript_Lugia_ApplyGuard
	return

// Phase 2 at 25% HP. The storm is floored at Tempest, Strain accrues twice as fast, and Survive goes
// on so a 60-guard Break window cannot overshoot the catch window.
EncScript_Lugia_OceansWrath::
	encsetvar 0, 2   @ Phase 2
	encsetsurvive ENC_TARGET_BOSS, TRUE
	printstring STRINGID_ENCLUGIAOCEANSWRATH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Lugia_OceansWrath_Apply
	encsetvar 1, 4
EncScript_Lugia_OceansWrath_Apply:
	call EncScript_Lugia_ApplyStorm
	call EncScript_Lugia_ApplyGuard
	return

// Below 10% in phase 2. Everything off, and Survive released so the catch window is a real one.
// CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the window.
EncScript_Lugia_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Storm
	encsetvar 2, 0   @ Strain
	encsetvar 3, 0   @ Eye
	encsetvar 4, 0   @ Hold
	encsetvar 5, 0   @ Break
	encsetvar 7, 0   @ LastStorm
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	removeweather
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 6, 78
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCLUGIAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
