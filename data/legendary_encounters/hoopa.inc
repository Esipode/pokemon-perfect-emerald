// Hoopa. Author variables (see hoopa.encounter's OnBattleStart block for the full map):
// 0 Phase, 1 Energy, 2 RingState, 3 RingType, 4 RingCycle, 5 LastBand, 6 Tick, 7 HyperTimer,
// 8 ChainTimer, 9 ChainGuard, 10 TheftCount, 11 FuryPortal, 12 FuryArmed, 13 PierceLatch,
// 14 FeedGuard, 15 Taken.

EncScript_Hoopa_Intro::
	encsetvar 0, 0     @ Phase
	encsetvar 1, 3     @ Energy
	encsetvar 2, 0     @ RingState
	encsetvar 3, 0     @ RingType
	encsetvar 4, 0     @ RingCycle
	encsetvar 5, 3     @ LastBand - uninitialized sentinel, forces the first band callout
	encsetvar 6, 0     @ Tick
	encsetvar 7, 0     @ HyperTimer
	encsetvar 8, 0     @ ChainTimer
	encsetvar 9, 0     @ ChainGuard
	encsetvar 10, 0    @ TheftCount
	encsetvar 11, 1    @ FuryPortal
	encsetvar 12, 0    @ FuryArmed
	encsetvar 13, 0    @ PierceLatch
	encsetvar 14, 0    @ FeedGuard
	encsetvar 15, 0    @ Taken
	printstring STRINGID_ENCHOOPAINTRO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Hoopa_ApplyGuard
	return

// Sole writer of Hoopa's damage reduction: 60 + 5 x Energy, a seven-way literal ladder. Announces
// the guard band only when it changes from LastBand.
EncScript_Hoopa_ApplyGuard:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Hoopa_ApplyGuard_E0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Hoopa_ApplyGuard_E1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Hoopa_ApplyGuard_E2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Hoopa_ApplyGuard_E3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Hoopa_ApplyGuard_E4
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Hoopa_ApplyGuard_E5
	encsetdamagereduction ENC_TARGET_BOSS, 92
	goto EncScript_Hoopa_ApplyGuard_High
EncScript_Hoopa_ApplyGuard_E0:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	goto EncScript_Hoopa_ApplyGuard_Low
EncScript_Hoopa_ApplyGuard_E1:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	goto EncScript_Hoopa_ApplyGuard_Low
EncScript_Hoopa_ApplyGuard_E2:
	encsetdamagereduction ENC_TARGET_BOSS, 87
	goto EncScript_Hoopa_ApplyGuard_Mid
EncScript_Hoopa_ApplyGuard_E3:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	goto EncScript_Hoopa_ApplyGuard_Mid
EncScript_Hoopa_ApplyGuard_E4:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	goto EncScript_Hoopa_ApplyGuard_Mid
EncScript_Hoopa_ApplyGuard_E5:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	goto EncScript_Hoopa_ApplyGuard_High
EncScript_Hoopa_ApplyGuard_Low:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Hoopa_ApplyGuard_Done
	encsetvar 5, 0
	printstring STRINGID_ENCHOOPABANDLOW
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Hoopa_ApplyGuard_Done
EncScript_Hoopa_ApplyGuard_Mid:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Hoopa_ApplyGuard_Done
	encsetvar 5, 1
	printstring STRINGID_ENCHOOPABANDMID
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Hoopa_ApplyGuard_Done
EncScript_Hoopa_ApplyGuard_High:
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Hoopa_ApplyGuard_Done
	encsetvar 5, 2
	printstring STRINGID_ENCHOOPABANDHIGH
	waitmessage B_WAIT_TIME_LONG
EncScript_Hoopa_ApplyGuard_Done:
	return

// Telegraphs the next portal for Six Rings and arms FuryFire. Shared by _SixRings (portal 1) and
// _FuryFire (portals 2-6).
EncScript_Hoopa_ArmPortal:
	printstring STRINGID_ENCHOOPAPORTALARM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetvar 12, 1    @ FuryArmed
	return

// Once-per-turn scaffold: ring-state advance, Hyperspace countdown, Ring of Chains chip.
EncScript_Hoopa_Pulse::
	flushtextbox
	encsetvar 6, 1     @ Tick - self-disable immediately
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Hoopa_Pulse_RingCool
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Hoopa_Pulse_RingArm
	goto EncScript_Hoopa_Pulse_Hyper
EncScript_Hoopa_Pulse_RingCool:
	encsetvar 2, 0     @ RingState -> idle
	goto EncScript_Hoopa_Pulse_Hyper
EncScript_Hoopa_Pulse_RingArm:
	encsetvar 2, 2     @ RingState -> armed
	printstring STRINGID_ENCHOOPARINGWIDENS
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Hoopa_Pulse_TeethArm
	goto EncScript_Hoopa_Pulse_Hyper
EncScript_Hoopa_Pulse_TeethArm:
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCHOOPATEETHARM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_PROTECTED_ITSELF
EncScript_Hoopa_Pulse_Hyper:
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Hoopa_Pulse_Chains
	encsubvar 7, 1
	printstring STRINGID_ENCHOOPAHYPERTURN
	waitmessage B_WAIT_TIME_LONG
EncScript_Hoopa_Pulse_Chains:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Hoopa_Pulse_Recur
	encchangehp ENC_TARGET_ALL_FOES, -7, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCHOOPACHAINSTICK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TURN_TRAP
	encsetvar 9, 1     @ ChainGuard
	goto EncScript_Hoopa_Pulse_Done
EncScript_Hoopa_Pulse_Recur:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Hoopa_Pulse_Done
	printstring STRINGID_ENCHOOPARINGSTURN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Hoopa_Pulse_Done:
	return

// Chooses the next ring from RingCycle: Hands -> Thieves -> Teeth (Phase 0), plus Chains
// (Phase 1). Deterministic literal ladder, not RNG - the order is meant to be learned.
EncScript_Hoopa_RingTelegraph::
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Hoopa_RingTelegraph_Hands
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Hoopa_RingTelegraph_Thieves
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Hoopa_RingTelegraph_Teeth
	@ RingCycle == 3: Chains (Phase 1 only)
	encsetvar 3, 4
	encsetvar 4, 0
	goto EncScript_Hoopa_RingTelegraph_Print
EncScript_Hoopa_RingTelegraph_Hands:
	encsetvar 3, 1
	encsetvar 4, 1
	goto EncScript_Hoopa_RingTelegraph_Print
EncScript_Hoopa_RingTelegraph_Thieves:
	encsetvar 3, 2
	encsetvar 4, 2
	goto EncScript_Hoopa_RingTelegraph_Print
EncScript_Hoopa_RingTelegraph_Teeth:
	encsetvar 3, 3
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Hoopa_RingTelegraph_TeethUnbound
	encsetvar 4, 0     @ Phase 0 wraps back to Hands
	goto EncScript_Hoopa_RingTelegraph_Print
EncScript_Hoopa_RingTelegraph_TeethUnbound:
	encsetvar 4, 3     @ Phase 1 advances to Chains
EncScript_Hoopa_RingTelegraph_Print:
	encsetvar 2, 1     @ RingState = telegraphed
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Hoopa_RingTelegraph_PrintHands
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Hoopa_RingTelegraph_PrintThieves
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Hoopa_RingTelegraph_PrintTeeth
	printstring STRINGID_ENCHOOPATELEGRAPHCHAINS
	goto EncScript_Hoopa_RingTelegraph_Common
EncScript_Hoopa_RingTelegraph_PrintHands:
	printstring STRINGID_ENCHOOPATELEGRAPHHANDS
	goto EncScript_Hoopa_RingTelegraph_Common
EncScript_Hoopa_RingTelegraph_PrintThieves:
	printstring STRINGID_ENCHOOPATELEGRAPHTHIEVES
	goto EncScript_Hoopa_RingTelegraph_Common
EncScript_Hoopa_RingTelegraph_PrintTeeth:
	printstring STRINGID_ENCHOOPATELEGRAPHTEETH
EncScript_Hoopa_RingTelegraph_Common:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// A ring resolves while the player is Protected: it shatters against the guard instead of
// landing. Sets RingState to cooling itself so the plain resolve trigger cannot also fire.
EncScript_Hoopa_Shatter::
	encsetvar 2, 3     @ RingState = cooling
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Hoopa_Shatter_Floor
	encsubvar 1, 2
	goto EncScript_Hoopa_Shatter_Damage
EncScript_Hoopa_Shatter_Floor:
	encsetvar 1, 0
EncScript_Hoopa_Shatter_Damage:
	encchangehp ENC_TARGET_BOSS, -6, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCHOOPASHATTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_PROTECTED_ITSELF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Hoopa_ApplyGuard
	return

// A ring resolves unopposed.
EncScript_Hoopa_RingResolve::
	encsetvar 2, 3     @ RingState = cooling
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Hoopa_RingResolve_Hands
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Hoopa_RingResolve_Thieves
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Hoopa_RingResolve_Teeth
	@ RingType == 4: Chains
	encsetvar 8, 3     @ ChainTimer
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCHOOPACHAINSHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TURN_TRAP
	goto EncScript_Hoopa_RingResolve_Spend
EncScript_Hoopa_RingResolve_Hands:
	encsetrecharge ENC_TARGET_ALL_FOES, 2
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCHOOPAHANDSHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	goto EncScript_Hoopa_RingResolve_Spend
EncScript_Hoopa_RingResolve_Thieves:
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Hoopa_RingResolve_ThievesReturn
	encsetvar 15, 0    @ Taken = false
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Hoopa_RingResolve_ThievesScreens
	encsetvar 15, 1
EncScript_Hoopa_RingResolve_ThievesScreens:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Hoopa_RingResolve_ThievesHazards
	encsetvar 15, 1
EncScript_Hoopa_RingResolve_ThievesHazards:
	encclearhazards ENC_TARGET_ALL_FOES, EncScript_Hoopa_RingResolve_ThievesRaise
	encsetvar 15, 1
EncScript_Hoopa_RingResolve_ThievesRaise:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT, 5
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN, 5
	encsetvar 10, 1    @ TheftCount
	encjumpifvar CMP_EQUAL, 15, 0, EncScript_Hoopa_RingResolve_ThievesNone
	printstring STRINGID_ENCHOOPATHIEVESHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	goto EncScript_Hoopa_RingResolve_Spend
EncScript_Hoopa_RingResolve_ThievesNone:
	printstring STRINGID_ENCHOOPATHIEVESNONE
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Hoopa_RingResolve_Spend
EncScript_Hoopa_RingResolve_ThievesReturn:
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 4
	encsetvar 10, 0
	printstring STRINGID_ENCHOOPATHIEVESRETURN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
	goto EncScript_Hoopa_RingResolve_Spend
EncScript_Hoopa_RingResolve_Teeth:
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCHOOPATEETHHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
EncScript_Hoopa_RingResolve_Spend:
	call EncScript_Hoopa_SpendEnergy
	return

// Energy -1, floor 0 (encsubvar wraps at 0, so the floor is an explicit skip). No-op while
// HyperTimer is locked - Phase 1's three Hyperspace turns freeze Energy entirely.
EncScript_Hoopa_SpendEnergy:
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Hoopa_SpendEnergy_Done
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Hoopa_SpendEnergy_Done
	encsubvar 1, 1
EncScript_Hoopa_SpendEnergy_Done:
	call EncScript_Hoopa_ApplyGuard
	return

// Ring of Chains: decrements once per turn it chipped in _Pulse, closes at 0.
EncScript_Hoopa_ChainTick::
	encsetvar 9, 0     @ ChainGuard - self-disable
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Hoopa_ChainTick_Done
	encsubvar 8, 1
	encjumpifvar CMP_NOT_EQUAL, 8, 0, EncScript_Hoopa_ChainTick_Done
	encchangehp ENC_TARGET_ALL_FOES, -15, ENC_AMOUNT_PERCENT
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	printstring STRINGID_ENCHOOPACHAINSBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
EncScript_Hoopa_ChainTick_Done:
	return

// Hoopa Unbound at 50%.
EncScript_Hoopa_Unbound::
	encsetvar 0, 1     @ Phase = Unbound
	printstring STRINGID_ENCHOOPAUNBOUND
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_HOOPA_UNBOUND, EncScript_Hoopa_Unbound_Move, B_ANIM_FORM_CHANGE
EncScript_Hoopa_Unbound_Move:
	encsetmove ENC_TARGET_BOSS, 2, MOVE_HYPERSPACE_FURY
	encsetvar 1, 6     @ Energy
	encsetvar 7, 4     @ HyperTimer
	encsetvar 2, 0     @ RingState
	encsetvar 3, 0     @ RingType
	encsetvar 4, 0     @ RingCycle - restart the cycle so Chains slots in cleanly
	encsethealblock ENC_TARGET_ALL_FOES, 3
	printstring STRINGID_ENCHOOPAHYPERSPACE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Hoopa_ApplyGuard
	return

// Hyperspace collapses: Energy -> 0, the phase's intended big damage window.
EncScript_Hoopa_HyperCollapse::
	encsetvar 7, 0     @ HyperTimer - self-disable
	encsetvar 1, 0     @ Energy
	encsethealblock ENC_TARGET_ALL_FOES, 0
	printstring STRINGID_ENCHOOPACOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	call EncScript_Hoopa_ApplyGuard
	return

// Hyperspace Hole/Fury pierced a Protect. Purely a callout - the move already ignored Protect on
// its own merits; this just tells the player why.
EncScript_Hoopa_Pierce::
	encsetvar 13, 1    @ PierceLatch - self-disable
	printstring STRINGID_ENCHOOPAPIERCE
	waitmessage B_WAIT_TIME_LONG
	return

// A player Pokemon fainted: feeds Hoopa 1 Energy, capped at 6.
EncScript_Hoopa_FeedOnFaint::
	encsetvar 14, 1    @ FeedGuard - self-disable
	printstring STRINGID_ENCHOOPAFAINT
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Hoopa_FeedOnFaint_Done
	encaddvar 1, 1
EncScript_Hoopa_FeedOnFaint_Done:
	call EncScript_Hoopa_ApplyGuard
	return

// Idle regeneration - the cap (Energy <= 5) is already the trigger's own condition.
EncScript_Hoopa_EnergyRegen::
	encaddvar 1, 1
	call EncScript_Hoopa_ApplyGuard
	return

// Clears the once-per-turn latches. Runs last at OnTurnEnd so every other checkpoint script this
// turn already saw them set.
EncScript_Hoopa_TickReset::
	encsetvar 6, 0     @ Tick
	encsetvar 13, 0    @ PierceLatch
	encsetvar 14, 0    @ FeedGuard
	return

// Phase 2 - The Six Rings, at 15%. Normal ring mechanics stop; Survive holds the boss up so the
// last portal always lands. Trap/Heal Block are released so switching and healing stay open.
EncScript_Hoopa_SixRings::
	encsetvar 0, 2     @ Phase = Six Rings
	encsetvar 1, 6     @ Energy
	encsetvar 2, 0     @ RingState
	encsetvar 3, 0     @ RingType
	encsetvar 4, 0     @ RingCycle
	encsetvar 7, 0     @ HyperTimer
	encsetvar 8, 0     @ ChainTimer
	encsetvar 9, 0     @ ChainGuard
	encsetvar 11, 1    @ FuryPortal
	encsetvar 12, 0    @ FuryArmed
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsethealblock ENC_TARGET_ALL_FOES, 0
	printstring STRINGID_ENCHOOPASIXRINGS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Hoopa_ApplyGuard
	call EncScript_Hoopa_ArmPortal
	return

// Fires the armed portal, then arms the next one (portals 2-6 telegraph on the same turn the
// previous one fires, so all six cost seven turns rather than twelve).
EncScript_Hoopa_FuryFire::
	encsetvar 12, 0    @ FuryArmed - self-disable
	flushtextbox
	printstring STRINGID_ENCHOOPAPORTALFIRE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Hoopa_FuryFire_P1
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Hoopa_FuryFire_P2
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Hoopa_FuryFire_P3
	encjumpifvar CMP_EQUAL, 11, 4, EncScript_Hoopa_FuryFire_P4
	encjumpifvar CMP_EQUAL, 11, 5, EncScript_Hoopa_FuryFire_P5
	@ portal 6
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
	goto EncScript_Hoopa_FuryFire_Next
EncScript_Hoopa_FuryFire_P1:
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Hoopa_FuryFire_Next
EncScript_Hoopa_FuryFire_P2:
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Hoopa_FuryFire_Next
EncScript_Hoopa_FuryFire_P3:
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	goto EncScript_Hoopa_FuryFire_Next
EncScript_Hoopa_FuryFire_P4:
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	goto EncScript_Hoopa_FuryFire_Next
EncScript_Hoopa_FuryFire_P5:
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
EncScript_Hoopa_FuryFire_Next:
	encaddvar 11, 1    @ FuryPortal++
	encjumpifvar CMP_EQUAL, 11, 7, EncScript_Hoopa_FuryFire_Done
	call EncScript_Hoopa_ArmPortal
EncScript_Hoopa_FuryFire_Done:
	return

// Catch-window teardown.
EncScript_Hoopa_Spent::
	encsetvar 0, 3     @ Phase = Spent
	printstring STRINGID_ENCHOOPAFADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0
	encsetvar 2, 0
	encsetvar 3, 0
	encsetvar 4, 0
	encsetvar 7, 0
	encsetvar 8, 0
	encsetvar 9, 0
	encsetvar 10, 0
	encsetvar 11, 7
	encsetvar 12, 0
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Hoopa_Spent_NoStats
EncScript_Hoopa_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCHOOPAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
