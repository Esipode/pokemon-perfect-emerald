// Mew, "The Genetic Wonder". Var indices, pinned by the
// always-true Conditions on EncScript_Mew_Intro:
// 0 Phase (0 curious / 1 playtime / 2 genesis / 3 weakened), 1 Wonder (0-6), 2 Bored (0-3),
// 3 LastCat (1 physical / 2 special / 3 status / 4 switch), 4-7 SeenPhys/SeenSpec/SeenStatus/
// SeenSwitch, 8 Wait (0 off / 1 armed / 2 waiting / 3 satisfied / 4 delighted / 5 disappointed),
// 9 LastGuard (last ANNOUNCED guard tier 0-3), 10 Chant, 11 TurnGuard, 12 MoveGuard, 13 SwitchGuard,
// 14 ChaosGuard.
//
// The fight punishes a habit, not a resource. Changing what KIND of thing you do - a move category
// different from last turn, or a switch - raises Wonder, which drives Mew's guard DOWNWARD as it
// gets absorbed in watching you (ApplyGuard: 92 -> 86 -> 78 -> 68). Repeating a category raises
// Bored; at 3 it fires one Mischief roll off a fixed table and knocks Wonder back down, so spam
// regresses. From Playtime (60% HP) Mew uses enctransform to become the player's active Pokemon and
// re-copies on every switch. At 30% HP it stops fighting for one turn and asks to be shown something
// (a status move or a switch); either branch enters Genesis - raw offense +20%, Mischief every turn.
// Below 10% it reverts, all guards off, Poke Balls unlocked.

// --- Shared subroutines (call/return) ---

// Sole owner of the guard ladder AND its callout. The ladder is not monotonic (Mischief pushes
// Wonder down), so this stores the last ANNOUNCED tier in LastGuard and prints only on a change, in
// either direction. Returns early in Weakened, which owns its own guard.
EncScript_Mew_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mew_ApplyGuard_Done
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Mew_ApplyGuard_T3   @ Wonder 5-6
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Mew_ApplyGuard_T2   @ Wonder 3-4
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Mew_ApplyGuard_T1   @ Wonder 1-2
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 0
	printstring STRINGID_ENCMEWRETREATS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mew_ApplyGuard_T1:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 1
	printstring STRINGID_ENCMEWCLOSER
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mew_ApplyGuard_T2:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encjumpifvar CMP_EQUAL, 9, 2, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 2
	printstring STRINGID_ENCMEWUNSHIELDED
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mew_ApplyGuard_T3:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encjumpifvar CMP_EQUAL, 9, 3, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 3
	printstring STRINGID_ENCMEWABSORBED
	waitmessage B_WAIT_TIME_LONG
EncScript_Mew_ApplyGuard_Done:
	return

// Wonder +1 (cap 6), Bored = 0, then the guard ladder. Prints the "eyes light up" line only when
// Wonder actually moved.
EncScript_Mew_GainWonder:
	encsetvar 2, 0   @ Bored
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Mew_GainWonder_Done
	encaddvar 1, 1
	printstring STRINGID_ENCMEWEYESLIGHTUP
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Mew_ApplyGuard
EncScript_Mew_GainWonder_Done:
	return

// Bored +1 (cap 3). At 2 it prints the drift warning, so the player gets a turn's notice before
// Mischief lands.
EncScript_Mew_GainBored:
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Mew_GainBored_Done
	encaddvar 2, 1
	encjumpifvar CMP_NOT_EQUAL, 2, 2, EncScript_Mew_GainBored_Done
	printstring STRINGID_ENCMEWDRIFTS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mew_GainBored_Done:
	return

// The Transform beat. enctransform copies the player's active Pokemon onto Mew (species, stats, stat
// stages, types, ability, moves); HP/level/item/status and the party mon are untouched, so a caught
// Mew is still SPECIES_MEW. Re-copying (a later switch) preserves the first original species for the
// revert. Jumps to _Failed on the legitimate states that block a copy.
EncScript_Mew_Copy:
	printstring STRINGID_ENCMEWCOPIES
	waitmessage B_WAIT_TIME_SHORT
	enctransform ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, EncScript_Mew_Copy_Failed
	printstring STRINGID_ENCMEWWEARSYOURSHAPE
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Mew_Copy_Done   @ Genesis: re-apply the raw boost the copy just wiped
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
EncScript_Mew_Copy_Done:
	return
EncScript_Mew_Copy_Failed:
	printstring STRINGID_ENCMEWCANTQUITEGETIT
	waitmessage B_WAIT_TIME_SHORT
	return

// The Mischief outcomes. Each is silent of the Wonder cost and Bored reset - the dispatcher
// (EncScript_Mew_Mischief) owns those - and just applies its own effect and animation.
EncScript_Mew_Mischief_Copycat:
	printstring STRINGID_ENCMEWCOPYCAT
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

EncScript_Mew_Mischief_Whim:
	printstring STRINGID_ENCMEWSKY
	waitmessage B_WAIT_TIME_SHORT
	encjumpifchance 33, EncScript_Mew_Mischief_Whim_Sun
	encjumpifchance 50, EncScript_Mew_Mischief_Whim_Rain
	encsetweather BATTLE_WEATHER_SANDSTORM, 0
	playanimation BS_PLAYER1, B_ANIM_SANDSTORM_CONTINUES
	return
EncScript_Mew_Mischief_Whim_Sun:
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_PLAYER1, B_ANIM_SUN_CONTINUES
	return
EncScript_Mew_Mischief_Whim_Rain:
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_PLAYER1, B_ANIM_RAIN_CONTINUES
	return

EncScript_Mew_Mischief_Recovery:
	printstring STRINGID_ENCMEWRECOVERS
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, 10, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

EncScript_Mew_Mischief_Tag:
	printstring STRINGID_ENCMEWTAG
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
	return

EncScript_Mew_Mischief_Generosity:
	printstring STRINGID_ENCMEWCLEARSFIELD
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_PLAYER_LEFT, 15, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	removeweather
	return

// --- Trigger scripts ---

// No transformation yet and no trainerslide - this is a wild Pokemon.
EncScript_Mew_Intro::
	printstring STRINGID_ENCMEWINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Runs once at the top of every turn (TurnGuard gate): clears the per-event guards, then - while the
// mechanic is live - runs the recurring callout every 3rd turn, keyed to whether Wonder is high.
EncScript_Mew_TurnOpen::
	encsetvar 11, 1   @ TurnGuard
	encsetvar 12, 0   @ MoveGuard
	encsetvar 13, 0   @ SwitchGuard
	encsetvar 14, 0   @ ChaosGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mew_TurnOpen_Done   @ weakened: no more callouts
	encaddvar 10, 1   @ Chant
	encjumpifvar CMP_LESS_THAN, 10, 3, EncScript_Mew_TurnOpen_Done
	encsetvar 10, 0
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Mew_TurnOpen_Entranced   @ Wonder >= 3
	printstring STRINGID_ENCMEWWATCHES
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Mew_TurnOpen_Done
EncScript_Mew_TurnOpen_Entranced:
	printstring STRINGID_ENCMEWENTRANCED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mew_TurnOpen_Done:
	return

// OnTurnEnd, priority 90 - runs last. Clears TurnGuard, then in Genesis the disappointed branch's
// recurring chip: 6% off the player's side every turn for the rest of the fight.
EncScript_Mew_TurnClose::
	encsetvar 11, 0   @ TurnGuard
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Mew_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 8, 5, EncScript_Mew_TurnClose_Done
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
EncScript_Mew_TurnClose_Done:
	return

// The player attacked physically. First-time beat: Attack +1 stage. Then the variety read - a
// category different from last turn raises Wonder, a repeat raises Bored.
EncScript_Mew_PlayerPhysical::
	encsetvar 12, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Mew_PlayerPhysical_Variety
	encsetvar 4, 1   @ SeenPhys
	printstring STRINGID_ENCMEWFASCINATED
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
EncScript_Mew_PlayerPhysical_Variety:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Mew_PlayerPhysical_Repeat
	encsetvar 3, 1   @ LastCat = physical
	call EncScript_Mew_GainWonder
	return
EncScript_Mew_PlayerPhysical_Repeat:
	call EncScript_Mew_GainBored
	return

// The player attacked specially. First-time beat: Sp. Atk +1 stage.
EncScript_Mew_PlayerSpecial::
	encsetvar 12, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Mew_PlayerSpecial_Variety
	encsetvar 5, 1   @ SeenSpec
	printstring STRINGID_ENCMEWSTUDIES
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
EncScript_Mew_PlayerSpecial_Variety:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Mew_PlayerSpecial_Repeat
	encsetvar 3, 2   @ LastCat = special
	call EncScript_Mew_GainWonder
	return
EncScript_Mew_PlayerSpecial_Repeat:
	call EncScript_Mew_GainBored
	return

// The player used a status move. First-time beat: Def +1 and Sp. Def +1. Also answers the challenge
// (Wait 2 -> 3) - a status move is one of the two things Mew wants to be shown.
EncScript_Mew_PlayerStatus::
	encsetvar 12, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Mew_PlayerStatus_Variety
	encsetvar 6, 1   @ SeenStatus
	printstring STRINGID_ENCMEWCOPIESTECHNIQUE
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
EncScript_Mew_PlayerStatus_Variety:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Mew_PlayerStatus_Repeat
	encsetvar 3, 3   @ LastCat = status
	call EncScript_Mew_GainWonder
	goto EncScript_Mew_PlayerStatus_Challenge
EncScript_Mew_PlayerStatus_Repeat:
	call EncScript_Mew_GainBored
EncScript_Mew_PlayerStatus_Challenge:
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Mew_PlayerStatus_Satisfy
	return
EncScript_Mew_PlayerStatus_Satisfy:
	encsetvar 8, 3   @ challenge satisfied
	return

// A player Pokemon switched in. First-time beat: Mew heals 8%. A switch always raises Wonder (it is
// variety by definition), answers the challenge, and - from Playtime on - Mew re-copies the new mon.
EncScript_Mew_SwitchIn::
	encsetvar 13, 1   @ SwitchGuard
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Mew_SwitchIn_Variety
	encsetvar 7, 1   @ SeenSwitch
	printstring STRINGID_ENCMEWFLITSBACK
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Mew_SwitchIn_Variety:
	encsetvar 3, 4   @ LastCat = switch
	call EncScript_Mew_GainWonder
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Mew_SwitchIn_Satisfy
	goto EncScript_Mew_SwitchIn_Copy
EncScript_Mew_SwitchIn_Satisfy:
	encsetvar 8, 3   @ challenge satisfied
EncScript_Mew_SwitchIn_Copy:
	encjumpifvar CMP_LESS_THAN, 0, 1, EncScript_Mew_SwitchIn_Done   @ still curious: no transform yet
	call EncScript_Mew_Copy
EncScript_Mew_SwitchIn_Done:
	return

// 60% HP: Playtime. Mew decides the fight is a game and takes on the player's shape. Registered at
// both OnMoveEnd and OnTurnEnd; both gate on Phase == 0, which this overwrites.
EncScript_Mew_Playtime::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCMEWPLAYTIME
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	call EncScript_Mew_Copy
	return

// 30% HP: the challenge. Phase goes to 2 up front so the Weakened trigger is armed even if the
// player bursts Mew through the waiting turn. Wait = 1 arms the state machine; WaitStep picks it up
// next turn.
EncScript_Mew_Challenge::
	encsetvar 0, 2   @ Phase 2
	encsetvar 8, 1   @ Wait = armed
	printstring STRINGID_ENCMEWDEMAND
	waitmessage B_WAIT_TIME_LONG
	return

// OnTurnStart, the turn after the challenge. Mew's offense collapses for exactly this turn (Atk and
// Sp. Atk driven to the floor), which reads as it half-heartedly waiting. Its guard is untouched, so
// the turn is genuinely free.
EncScript_Mew_WaitStep::
	encsetvar 8, 2   @ Wait = waiting
	printstring STRINGID_ENCMEWDEMAND
	waitmessage B_WAIT_TIME_SHORT
	printstring STRINGID_ENCMEWWAITING
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	return

// OnTurnEnd of the waiting turn. Restores offense to neutral (drive to the cap, then back 6 - the
// established "back to neutral" idiom), resolves delighted (Wait 3) vs disappointed, then enters
// Genesis: raw Atk/Sp. Atk +20% and Mischief every turn from here.
EncScript_Mew_WaitResolve::
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Mew_WaitResolve_Weakened   @ Mew got bursted past the challenge
	encjumpifvar CMP_EQUAL, 8, 3, EncScript_Mew_WaitResolve_Delighted
	encsetvar 8, 5   @ disappointed
	printstring STRINGID_ENCMEWDISAPPOINTED
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Mew_WaitResolve_Genesis
EncScript_Mew_WaitResolve_Delighted:
	encsetvar 8, 4   @ delighted
	printstring STRINGID_ENCMEWDELIGHTED
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_PLAYER_LEFT, 30, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	encsetvar 1, 6   @ Wonder maxed; nothing lowers it in Genesis
	call EncScript_Mew_ApplyGuard
EncScript_Mew_WaitResolve_Genesis:
	printstring STRINGID_ENCMEWGENESIS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
	return
EncScript_Mew_WaitResolve_Weakened:
	encsetvar 8, 4   @ close the state machine; Weakened already owns the fight
	return

// OnTurnStart. Fires at Bored 3, or every turn once Genesis is running. Owns ChaosGuard, the Bored
// reset, and - while Phase <= 1 - the Wonder cost of being predictable. Rolls one outcome off the
// fixed table: 20% Copycat, 20% Whim, 20% Recovery, 25% Tag, 15% Generosity, with Transform
// replacing 40% of the Copycat slot once Mew is in Playtime.
EncScript_Mew_Mischief::
	encsetvar 14, 1   @ ChaosGuard
	encsetvar 2, 0    @ Bored
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Mew_Mischief_Roll   @ Genesis: no "bored" line
	printstring STRINGID_ENCMEWBORED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mew_Mischief_Roll:
	encjumpifchance 20, EncScript_Mew_Mischief_SlotCopycat
	encjumpifchance 25, EncScript_Mew_Mischief_SlotWhim
	encjumpifchance 33, EncScript_Mew_Mischief_SlotRecovery
	encjumpifchance 63, EncScript_Mew_Mischief_SlotTag
	call EncScript_Mew_Mischief_Generosity
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotCopycat:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Mew_Mischief_SlotCopycat_Plain
	encjumpifchance 40, EncScript_Mew_Mischief_SlotTransform
EncScript_Mew_Mischief_SlotCopycat_Plain:
	call EncScript_Mew_Mischief_Copycat
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotTransform:
	call EncScript_Mew_Copy
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotWhim:
	call EncScript_Mew_Mischief_Whim
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotRecovery:
	call EncScript_Mew_Mischief_Recovery
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotTag:
	call EncScript_Mew_Mischief_Tag
EncScript_Mew_Mischief_Cost:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Mew_Mischief_Done   @ Genesis: predictability is free
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mew_Mischief_Done
	encsubvar 1, 1   @ Wonder -1
	call EncScript_Mew_ApplyGuard
EncScript_Mew_Mischief_Done:
	return

// 10% HP: Mew tires of the game. It reverts to its own shape, every guard and the survive clamp
// come off (the automatic catch-window guard takes over on the trailing pass), and Poke Balls are
// unblocked. Registered at both checkpoints; gated on Phase == 2 so it can't precede the challenge.
EncScript_Mew_Weakened::
	encsetvar 0, 3   @ Phase 3
	encuntransform ENC_TARGET_BOSS
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMEWWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
