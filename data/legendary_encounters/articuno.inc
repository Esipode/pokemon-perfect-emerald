// Articuno, "The Frozen Battlefield". Var indices, pinned by
// the always-true Conditions on EncScript_Articuno_Intro:
// 0 Phase, 1 Barrier, 2 Plummet, 3 Frost, 4 Chill, 5 FrostLock, 6 Answered, 7 LastBlunt,
// 8 IceGuard, 9 FireGuard, 10 SwitchGuard. The three *Guard vars are set to 1 the first time their
// OnMoveEnd/OnSwitchIn trigger fires and cleared by EncScript_Articuno_TurnReset at end of turn,
// so an Event condition that stays true through the re-evaluation loop can't fire twice per event.
//
// The core loop: Articuno's Ice moves raise Frost (EncScript_Articuno_IceMove); the player's Fire
// moves lower it (EncScript_Articuno_FireResponse); while Frost >= 1 the Ice Barrier regrows every
// turn (EncScript_Articuno_BarrierRaise), at a strength that scales with Frost, until a damaging
// hit shatters it (EncScript_Articuno_BarrierShatter), unleashing a shockwave scaled by the
// current Frost level.
// Absolute Zero (EncScript_Articuno_AbsoluteZero) locks Frost in place - freezing the barrier at
// its current strength for the rest of the fight - and repurposes the Fire answer into resolving a
// recurring "temperature plummets" timer instead.

EncScript_Articuno_Intro::
	printstring STRINGID_ENCARTICUNOINTRO
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Articuno_PhaseFrozenField::
	encsetvar 0, 1   @ Phase = 1 (Frozen Battlefield)
	printstring STRINGID_ENCARTICUNOPHASE1
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	return

EncScript_Articuno_AbsoluteZero::
	encsetvar 0, 2   @ Phase = 2 (Absolute Zero)
	encsetvar 5, 1   @ FrostLock = true
	printstring STRINGID_ENCARTICUNOABSOLUTEZERO
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SNOW, 0
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 1
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 10, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 10, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_SHEER_COLD
	waitanimation
	@ The barrier is now fixed at whatever strength the frozen Frost level supports.
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Articuno_AbsoluteZero_SealNone
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Articuno_AbsoluteZero_SealThin
	printstring STRINGID_ENCARTICUNOBARRIERSEALEDSOLID
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Articuno_AbsoluteZero_SealThin:
	printstring STRINGID_ENCARTICUNOBARRIERSEALEDTHIN
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Articuno_AbsoluteZero_SealNone:
	printstring STRINGID_ENCARTICUNOBARRIERSEALEDNONE
	waitmessage B_WAIT_TIME_LONG
	return

// Fires when the player lands a damaging hit on Articuno while the barrier is up (Var(Barrier)==1
// is the trigger's own guard). Restores the phase-appropriate base reduction, then unleashes a
// shockwave scaled by the current Frost level before it's spent decrementing on the way out.
EncScript_Articuno_BarrierShatter::
	encsetvar 1, 0   @ Barrier = false
	printstring STRINGID_ENCARTICUNOBARRIERSHATTERS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_ICICLE_CRASH
	waitanimation
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Articuno_BarrierShatter_Weakened
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Articuno_BarrierShatter_Locked
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encjumpifvar CMP_EQUAL, 7, 70, EncScript_Articuno_BarrierShatter_Shockwave
	encsetvar 7, 70   @ LastBlunt: re-announce the absorption whenever the barrier's tier changes
	goto EncScript_Articuno_BarrierShatter_Blunted
EncScript_Articuno_BarrierShatter_Locked:
	@ Frost is frozen here (Frozen Domain / Absolute Zero); the between-barrier floor tracks the
	@ level it locked at, so a diligent player who capped Frost low keeps Articuno soft.
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Articuno_BarrierShatter_LockedThin
	encsetdamagereduction ENC_TARGET_BOSS, 85
	goto EncScript_Articuno_BarrierShatter_LockedAnnounce
EncScript_Articuno_BarrierShatter_LockedThin:
	encsetdamagereduction ENC_TARGET_BOSS, 80
EncScript_Articuno_BarrierShatter_LockedAnnounce:
	encjumpifvar CMP_EQUAL, 7, 85, EncScript_Articuno_BarrierShatter_Shockwave
	encsetvar 7, 85
EncScript_Articuno_BarrierShatter_Blunted:
	printstring STRINGID_ENCARTICUNOBARRIERBLUNTED
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Articuno_BarrierShatter_Shockwave
EncScript_Articuno_BarrierShatter_Weakened:
	encsetdamagereduction ENC_TARGET_BOSS, 0
EncScript_Articuno_BarrierShatter_Shockwave:
	printstring STRINGID_ENCARTICUNOSHOCKWAVE
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Articuno_BarrierShatter_Shock0
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Articuno_BarrierShatter_Shock1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Articuno_BarrierShatter_Shock2
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Articuno_BarrierShatter_Shock3
	encchangehp ENC_TARGET_ALL_FOES, -15, ENC_AMOUNT_PERCENT   @ Frost 4
	return
EncScript_Articuno_BarrierShatter_Shock0:
	encchangehp ENC_TARGET_ALL_FOES, -3, ENC_AMOUNT_PERCENT
	return
EncScript_Articuno_BarrierShatter_Shock1:
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Articuno_BarrierShatter_Shock2:
	encchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Articuno_BarrierShatter_Shock3:
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return

// Fires whenever Articuno itself uses an Ice move (Event.MoveType == TYPE_ICE). While Frost is
// locked, only Absolute Zero's plummet timer still listens to it; otherwise it raises Frost and
// applies that tier's effect, capping at 4 (Frozen Domain).
EncScript_Articuno_IceMove::
	encsetvar 8, 1   @ IceGuard: one shot per Ice move; TurnReset clears it
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Articuno_IceMove_Locked
	encjumpifvar CMP_LESS_THAN, 3, 4, EncScript_Articuno_IceMove_Raise
	return   @ Frost already at cap; FrostLock should have engaged by now
EncScript_Articuno_IceMove_Locked:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Articuno_IceMove_NoOp   @ only Absolute Zero re-arms the timer
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Articuno_IceMove_NoOp   @ timer already armed or resolving
	encsetvar 2, 2   @ Plummet = armed
	encsetvar 6, 0   @ Answered = false
	printstring STRINGID_ENCARTICUNOTEMPPLUMMETS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_HAZE
	waitanimation
	return
EncScript_Articuno_IceMove_NoOp:
	return
EncScript_Articuno_IceMove_Raise:
	encaddvar 3, 1   @ Frost += 1
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Articuno_IceMove_Frost1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Articuno_IceMove_Frost2
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Articuno_IceMove_Frost3
	goto EncScript_Articuno_IceMove_Frost4
EncScript_Articuno_IceMove_Frost1:
	encsetweather BATTLE_WEATHER_SNOW, 0
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	printstring STRINGID_ENCARTICUNOFROST1
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Articuno_IceMove_Frost2:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Articuno_IceMove_Frost2Done   @ Chill already applied
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encsetvar 4, 1   @ Chill = true
	printstring STRINGID_ENCARTICUNOFROST2
	waitmessage B_WAIT_TIME_SHORT
EncScript_Articuno_IceMove_Frost2Done:
	return
EncScript_Articuno_IceMove_Frost3:
	printstring STRINGID_ENCARTICUNOFROST3
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Articuno_IceMove_Frost4:
	encsetvar 5, 1   @ FrostLock = true
	printstring STRINGID_ENCARTICUNOFROZENDOMAIN
	waitmessage B_WAIT_TIME_LONG
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 25, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Articuno_IceMove_Frost4BarrierUp   @ barrier already covers this - don't double-apply
	encsetdamagereduction ENC_TARGET_BOSS, 85
EncScript_Articuno_IceMove_Frost4BarrierUp:
	playmoveanimation MOVE_SHEER_COLD
	waitanimation
	return

// Fires whenever the player uses a Fire move, hit or miss, damaging or status - deliberately loose
// (guideline: the player's counterplay lever should feel generous and unambiguous).
EncScript_Articuno_FireResponse::
	encsetvar 9, 1   @ FireGuard: one shot per Fire move; TurnReset clears it
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Articuno_FireResponse_Melt   @ Plummet == resolving: this answers the timer
	encsetvar 6, 1   @ Answered = true
EncScript_Articuno_FireResponse_Melt:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Articuno_FireResponse_Done   @ FrostLock: Fire no longer melts Frost
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Articuno_FireResponse_Done   @ nothing to melt
	encsubvar 3, 1   @ Frost -= 1
	printstring STRINGID_ENCARTICUNOFLAMESPUSHBACK
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_WILL_O_WISP
	waitanimation
	encjumpifvar CMP_NOT_EQUAL, 3, 1, EncScript_Articuno_FireResponse_CheckClear
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Articuno_FireResponse_Done
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 1
	encsetvar 4, 0   @ Chill = false
	printstring STRINGID_ENCARTICUNOCHILLLIFTS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Articuno_FireResponse_CheckClear:
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Articuno_FireResponse_Done
	removeweather
	printstring STRINGID_ENCARTICUNOSNOWCLEARS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Articuno_FireResponse_Done:
	return

// Articuno shrugs off any non-volatile status at the top of its turn - sleep-locking a scripted
// boss is the single biggest trivializer, and this is the encounter's answer to it (other statuses
// are left to stick; see the encounter's design notes for why this one is purged).
EncScript_Articuno_ShakeOff::
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Articuno_ShakeOff_Wake
	return
EncScript_Articuno_ShakeOff_Wake:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCARTICUNOSHRUGSOFFSLEEP
	waitmessage B_WAIT_TIME_SHORT
	return

EncScript_Articuno_PlummetTick::
	encsetvar 2, 1   @ Plummet = resolving
	printstring STRINGID_ENCARTICUNOCOLDDEEPENS
	waitmessage B_WAIT_TIME_SHORT
	return

// While Frost >= 1 and the barrier isn't already up, it regrows every turn - the crux of the loop.
// Barrier strength tracks Frost: Fire pressure that holds Frost low keeps the regrown barrier thin
// and barely above the phase floor. When Frost locks (hits 4, or Absolute Zero begins) it freezes
// at whatever level the player left it, so the barrier keeps regrowing at that fixed strength -
// diligent Fire pressure in phase 1 carries through the rest of the fight.
EncScript_Articuno_BarrierRaise::
	encsetvar 1, 1   @ Barrier = true
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Articuno_BarrierRaise_Frost1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Articuno_BarrierRaise_Frost2
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Articuno_BarrierRaise_Frost3
	encsetdamagereduction ENC_TARGET_BOSS, 95   @ Frost 4 (Frozen Domain)
	goto EncScript_Articuno_BarrierRaise_Solid
EncScript_Articuno_BarrierRaise_Frost3:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	goto EncScript_Articuno_BarrierRaise_Solid
EncScript_Articuno_BarrierRaise_Frost2:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	goto EncScript_Articuno_BarrierRaise_Thin
EncScript_Articuno_BarrierRaise_Frost1:
	encsetdamagereduction ENC_TARGET_BOSS, 84
EncScript_Articuno_BarrierRaise_Thin:
	printstring STRINGID_ENCARTICUNOBARRIERTHIN
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MIST
	waitanimation
	return
EncScript_Articuno_BarrierRaise_Solid:
	printstring STRINGID_ENCARTICUNOBARRIERUP
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MIST
	waitanimation
	return

// A switch-in toll: heavy Frost bites the newcomer directly, and a lingering Chill re-applies its
// Speed drop (stat stages reset on switch, so this is the only way it reaches a fresh Pokemon).
EncScript_Articuno_FrozenGround::
	encsetvar 10, 1   @ SwitchGuard: one shot per switch-in; TurnReset clears it
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Articuno_FrozenGround_CheckChill
	printstring STRINGID_ENCARTICUNOFROZENGROUND
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_SELF, -12, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_HAIL_CONTINUES
EncScript_Articuno_FrozenGround_CheckChill:
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Articuno_FrozenGround_Done
	encchangestat ENC_TARGET_SELF, STAT_SPEED, -1
EncScript_Articuno_FrozenGround_Done:
	return

EncScript_Articuno_Weakened::
	encsetvar 0, 3   @ Phase = 3 (Weakened)
	encsetvar 1, 0   @ Barrier = false
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -1
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCARTICUNOWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Split across two checkpoints on purpose (OnTurnStart ticks, OnTurnEnd resolves) so a real turn
// passes in between instead of the countdown chasing itself through both states in one dispatch.
EncScript_Articuno_PlummetResolve::
	encsetvar 2, 0   @ Plummet = idle
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Articuno_PlummetResolve_Answered
	printstring STRINGID_ENCARTICUNOFIELDFREEZES
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_HAIL_CONTINUES
	return
EncScript_Articuno_PlummetResolve_Answered:
	printstring STRINGID_ENCARTICUNOFLAMESHOLD
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_BOSS, -5, ENC_AMOUNT_PERCENT
	return

// Clears the per-event guards at end of turn so IceMove/FireResponse/FrozenGround can fire again
// next turn. In a singles fight each side acts once per turn, so "once per turn" == "once per move".
EncScript_Articuno_TurnReset::
	encsetvar 8, 0    @ IceGuard
	encsetvar 9, 0    @ FireGuard
	encsetvar 10, 0   @ SwitchGuard
	return
