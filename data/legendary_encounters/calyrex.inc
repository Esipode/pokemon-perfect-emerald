// Calyrex, "The King Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Calyrex_Intro:
// 0 Phase (0 Decrees / 1 Rider / 2 Kingdom Restored / 3 Spent), 1 Authority (0-10),
// 2 DecreeType (0 no switch / 1 no weakened), 3 DecreeTimer (0 = the decree resolves this turn end),
// 4 ViolationFlag, 5 WeakStreak, 6 MountForm (0 Ice / 1 Shadow), 7 KCActive, 8 KCTurns,
// 9 GambitCooldown, 10 TurnGuard, 11 WeakLatch, 12 GambitWon.
//
// AUTHORITY IS THE GUARD AND THE GAMBIT. EncScript_Calyrex_ApplyGuard is the damage reduction's only
// writer after Properties and reads Phase, Authority and GambitWon absolutely; every Authority
// change goes through AuthUp*/AuthDown*, which clamp to 0-10 and tail into it.
//
// Every OnTurnStart/OnTurnEnd trigger closes its own latch: TurnOpen sets TurnGuard, TurnClose clears
// it and WeakLatch; the weak-check pair close WeakLatch; SwitchViolation and KCFailure close through
// ViolationFlag and KCActive.
//
// gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so animations use playanimation with an
// explicit battler.

// --- Shared subroutines ---

// THE SINGLE WRITER of the damage reduction. Spent (Phase 3) leaves the catch-window value alone.
EncScript_Calyrex_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Calyrex_ApplyGuard_Done
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Calyrex_ApplyGuard_Won
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Calyrex_ApplyGuard_P2
	encjumpifvar CMP_LESS_THAN, 1, 9, EncScript_Calyrex_ApplyGuard_90
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Calyrex_ApplyGuard_90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Calyrex_ApplyGuard_P2:
	encjumpifvar CMP_LESS_THAN, 1, 9, EncScript_Calyrex_ApplyGuard_95
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Calyrex_ApplyGuard_95:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Calyrex_ApplyGuard_Won:
	encsetdamagereduction ENC_TARGET_BOSS, 85
EncScript_Calyrex_ApplyGuard_Done:
	return

// Authority adjusters, clamped to 0-10. Each tails into ApplyGuard, which returns to the caller.
EncScript_Calyrex_AuthUp1:
	encaddvar 1, 1    @ Authority
	goto EncScript_Calyrex_AuthClamp
EncScript_Calyrex_AuthUp2:
	encaddvar 1, 2    @ Authority
EncScript_Calyrex_AuthClamp:
	encjumpifvar CMP_LESS_THAN, 1, 11, EncScript_Calyrex_ApplyGuard
	encsetvar 1, 10    @ Authority
	goto EncScript_Calyrex_ApplyGuard
EncScript_Calyrex_AuthDown2:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Calyrex_AuthZero
	encsubvar 1, 2    @ Authority
	goto EncScript_Calyrex_ApplyGuard
EncScript_Calyrex_AuthDown3:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Calyrex_AuthZero
	encsubvar 1, 3    @ Authority
	goto EncScript_Calyrex_ApplyGuard
EncScript_Calyrex_AuthZero:
	encsetvar 1, 0    @ Authority
	goto EncScript_Calyrex_ApplyGuard

// --- Entry points ---

// Decrees alternate starting with "do not switch": the first flip reads 1 and lands on 0.
EncScript_Calyrex_Intro::
	encsetvar 2, 1    @ DecreeType
	return

// Turn scaffold, open half. Phase 0 issues or ticks the decree and restates it every turn; later
// phases restate an active King's Command, tick the Gambit cooldown, and voice a high Authority.
EncScript_Calyrex_TurnOpen::
	encsetvar 10, 1    @ TurnGuard
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Calyrex_TurnOpen_Rider
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Calyrex_TurnOpen_Tick
	encsetvar 3, 2    @ DecreeTimer
	encsetvar 4, 0    @ ViolationFlag
	encsetvar 5, 0    @ WeakStreak
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Calyrex_TurnOpen_ToWeak
	encsetvar 2, 0    @ DecreeType
	goto EncScript_Calyrex_TurnOpen_Say
EncScript_Calyrex_TurnOpen_ToWeak:
	encsetvar 2, 1    @ DecreeType
	goto EncScript_Calyrex_TurnOpen_Say
EncScript_Calyrex_TurnOpen_Tick:
	encsubvar 3, 1    @ DecreeTimer
EncScript_Calyrex_TurnOpen_Say:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Calyrex_TurnOpen_SayNoSwitch
	printstring STRINGID_ENCCALYREXDECREENOWEAKENED
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Calyrex_TurnOpen_Recur
EncScript_Calyrex_TurnOpen_SayNoSwitch:
	printstring STRINGID_ENCCALYREXDECREENOSWITCH
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Calyrex_TurnOpen_Recur
EncScript_Calyrex_TurnOpen_Rider:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Calyrex_TurnOpen_Done
	encjumpifvar CMP_NOT_EQUAL, 7, 1, EncScript_Calyrex_TurnOpen_Cool
	printstring STRINGID_ENCCALYREXKINGSCOMMAND
	waitmessage B_WAIT_TIME_LONG
EncScript_Calyrex_TurnOpen_Cool:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Calyrex_TurnOpen_Recur
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Calyrex_TurnOpen_Recur
	encsubvar 9, 1    @ GambitCooldown
EncScript_Calyrex_TurnOpen_Recur:
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Calyrex_TurnOpen_Done
	printstring STRINGID_ENCCALYREXAUTHORITY
	waitmessage B_WAIT_TIME_LONG
EncScript_Calyrex_TurnOpen_Done:
	return

// Turn scaffold, close half. Resolves the decree when its timer has run out and counts King's
// Command turns.
EncScript_Calyrex_TurnClose::
	encsetvar 10, 0    @ TurnGuard
	encsetvar 11, 0    @ WeakLatch
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Calyrex_TurnClose_KC
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Calyrex_TurnClose_KC
	call EncScript_Calyrex_DecreeResolve
EncScript_Calyrex_TurnClose_KC:
	encjumpifvar CMP_NOT_EQUAL, 7, 1, EncScript_Calyrex_TurnClose_Done
	encaddvar 8, 1    @ KCTurns
	encjumpifvar CMP_LESS_THAN, 8, 3, EncScript_Calyrex_TurnClose_Done
	encsetvar 7, 0    @ KCActive
	call EncScript_Calyrex_AuthDown2
	printstring STRINGID_ENCCALYREXKCSUCCESS
	waitmessage B_WAIT_TIME_LONG
EncScript_Calyrex_TurnClose_Done:
	return

EncScript_Calyrex_DecreeResolve:
	encsetvar 5, 0    @ WeakStreak
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Calyrex_DecreeResolve_Kept
	call EncScript_Calyrex_AuthUp1
	printstring STRINGID_ENCCALYREXDECREEVIOLATED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return
EncScript_Calyrex_DecreeResolve_Kept:
	printstring STRINGID_ENCCALYREXDECREERESPECTED
	waitmessage B_WAIT_TIME_LONG
	return

// Two turn-ends in a row at or below a third of the lead's HP breaks "do not stay weakened".
EncScript_Calyrex_WeakStreakInc::
	encsetvar 11, 1    @ WeakLatch
	encaddvar 5, 1    @ WeakStreak
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Calyrex_WeakStreakInc_Done
	encsetvar 4, 1    @ ViolationFlag
EncScript_Calyrex_WeakStreakInc_Done:
	return

EncScript_Calyrex_WeakStreakReset::
	encsetvar 11, 1    @ WeakLatch
	encsetvar 5, 0    @ WeakStreak
	return

EncScript_Calyrex_SwitchViolation::
	encsetvar 4, 1    @ ViolationFlag
	return

// Lost Kingdom: each Authority band permanently reshapes the field.
EncScript_Calyrex_LostKingdomStage1::
	printstring STRINGID_ENCCALYREXKINGDOMSTAGE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	encsetterrain ENC_TERRAIN_PSYCHIC
	return

EncScript_Calyrex_LostKingdomStage2::
	printstring STRINGID_ENCCALYREXKINGDOMSTAGE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetfieldstatus ENC_FIELD_GRAVITY
	return

EncScript_Calyrex_LostKingdomStage3::
	printstring STRINGID_ENCCALYREXKINGDOMSTAGE3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Calyrex_ApplyGuard
	return

// Phase 1. A coin flip fixes the mount for the rest of the fight and unlocks its signature move in
// slot 0. A failed form change skips the move swap and reveal but still starts the Command.
EncScript_Calyrex_MountUp::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCCALYREXCALLSSTEED
	waitmessage B_WAIT_TIME_LONG
	encjumpifchance 50, EncScript_Calyrex_MountUp_Shadow
	encsetvar 6, 0    @ MountForm
	encformchange ENC_TARGET_BOSS, SPECIES_CALYREX_ICE, EncScript_Calyrex_MountUp_Common, B_ANIM_FORM_CHANGE
	encsetmove ENC_TARGET_BOSS, 0, MOVE_GLACIAL_LANCE
	printstring STRINGID_ENCCALYREXICEREVEAL
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Calyrex_MountUp_Common
EncScript_Calyrex_MountUp_Shadow:
	encsetvar 6, 1    @ MountForm
	encformchange ENC_TARGET_BOSS, SPECIES_CALYREX_SHADOW, EncScript_Calyrex_MountUp_Common, B_ANIM_FORM_CHANGE
	encsetmove ENC_TARGET_BOSS, 0, MOVE_ASTRAL_BARRAGE
	printstring STRINGID_ENCCALYREXSHADOWREVEAL
	waitmessage B_WAIT_TIME_LONG
EncScript_Calyrex_MountUp_Common:
	encsetvar 7, 1    @ KCActive
	encsetvar 8, 0    @ KCTurns
	call EncScript_Calyrex_ApplyGuard
	printstring STRINGID_ENCCALYREXKINGSCOMMAND
	waitmessage B_WAIT_TIME_LONG
	return

// Losing the lead during the Command: Authority surges and the King hits harder.
EncScript_Calyrex_KCFailure::
	encsetvar 7, 0    @ KCActive
	call EncScript_Calyrex_AuthUp2
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	printstring STRINGID_ENCCALYREXKCFAILURE
	waitmessage B_WAIT_TIME_LONG
	return

// Phase 2. Any Command still running is dropped. The Gambit follows once the cooldown ticks out.
EncScript_Calyrex_KingdomRestored::
	encsetvar 0, 2    @ Phase
	encsetvar 7, 0    @ KCActive
	encsetvar 9, 2    @ GambitCooldown
	call EncScript_Calyrex_ApplyGuard
	printstring STRINGID_ENCCALYREXKINGDOMRESTORED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Success chance is 100 - Authority * 10. Failure is the mount's finisher plus a forced recharge.
EncScript_Calyrex_KingsGambit::
	printstring STRINGID_ENCCALYREXKINGSGAMBIT
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Calyrex_KingsGambit_Success
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Calyrex_KingsGambit_Roll90
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Calyrex_KingsGambit_Roll80
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Calyrex_KingsGambit_Roll70
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Calyrex_KingsGambit_Roll60
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Calyrex_KingsGambit_Roll50
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Calyrex_KingsGambit_Roll40
	encjumpifvar CMP_EQUAL, 1, 7, EncScript_Calyrex_KingsGambit_Roll30
	encjumpifvar CMP_EQUAL, 1, 8, EncScript_Calyrex_KingsGambit_Roll20
	encjumpifvar CMP_EQUAL, 1, 9, EncScript_Calyrex_KingsGambit_Roll10
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll90:
	encjumpifchance 90, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll80:
	encjumpifchance 80, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll70:
	encjumpifchance 70, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll60:
	encjumpifchance 60, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll50:
	encjumpifchance 50, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll40:
	encjumpifchance 40, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll30:
	encjumpifchance 30, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll20:
	encjumpifchance 20, EncScript_Calyrex_KingsGambit_Success
	goto EncScript_Calyrex_KingsGambit_Fail
EncScript_Calyrex_KingsGambit_Roll10:
	encjumpifchance 10, EncScript_Calyrex_KingsGambit_Success
EncScript_Calyrex_KingsGambit_Fail:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Calyrex_KingsGambit_FailIce
	printstring STRINGID_ENCCALYREXFINISHERSHADOW
	goto EncScript_Calyrex_KingsGambit_FailHit
EncScript_Calyrex_KingsGambit_FailIce:
	printstring STRINGID_ENCCALYREXFINISHERICE
EncScript_Calyrex_KingsGambit_FailHit:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encsetrecharge ENC_TARGET_BOSS, 1
	call EncScript_Calyrex_AuthUp1
	return
EncScript_Calyrex_KingsGambit_Success:
	encsetvar 12, 1    @ GambitWon
	call EncScript_Calyrex_AuthDown3
	printstring STRINGID_ENCCALYREXGAMBITSUCCESS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

// Catch window: from any phase at <= 10% HP. Drops the pending Command and Gravity, then opens balls.
EncScript_Calyrex_NowIsTheMoment::
	encsetvar 0, 3    @ Phase
	encsetvar 7, 0    @ KCActive
	encclearfieldstatus ENC_FIELD_GRAVITY
	encsetdamagereduction ENC_TARGET_BOSS, 97
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCCALYREXNOWISMOMENT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return
