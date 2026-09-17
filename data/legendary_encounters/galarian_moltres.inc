// Galarian Moltres, "The Dread". Var indices, pinned by the always-true Conditions on
// EncScript_GalarianMoltres_Intro:
// 0 Dread (0-5 shared meter), 1 Phase (0 Normal / 1 Nightmare / 2 Terror), 2 MoveGuard, 3 FaintGuard,
// 4 SwitchGuard, 5 BossHitGuard, 6 NightmareTimer (turns left on the active mon's mark, 0 = none),
// 7 NightmareEffect (0 heal-block / 1 accuracy / 2 periodic drain), 8 LastAnnounced (whether the last
// DARK AURA callout said "in" or "out"), 9 LifeDrainGuard, 10 NightmareGuard (once-per-turn gates for
// the two OnTurnEnd levels - neither's own condition clears itself, so without a guard the
// checkpoint's re-evaluation loop reselects it every pass until the runaway-trigger cap trips).
//
// DREAD IS THE GUARD, IN BOTH DIRECTIONS. EncScript_GalarianMoltres_ApplyDreadLevel is the only writer of
// the offense stage, the damage reduction and the crit flag, and it reads Dread absolutely every
// time, so a band falling undoes a band rising with no separate teardown path.
//
// EVERY EVENT-REACTING TRIGGER CLOSES ITS OWN GUARD. MoveGuard/FaintGuard/SwitchGuard/BossHitGuard
// each cap their subsystem to once per turn/event; the matching *_Clear script runs at a different
// checkpoint so a real turn separates the set from the clear (splitting a guard across one checkpoint
// chases itself through every state in a single dispatch).
//
// STAT STAGES ARE WRITTEN ABSOLUTELY, the Rayquaza idiom: +12 clamps to the ceiling, then a single
// drop lands on the stage that was meant. That keeps the offense bump immune to a player Haze, a
// switch, or a repeated call landing in between.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd/OnFaint/OnSwitchIn, so anything
// here that animates uses playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// One step of Dread, floored at 0 and capped at 5. Silent; every caller prints its own line.
EncScript_GalarianMoltres_DreadUp:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_GalarianMoltres_DreadUp_Done
	encaddvar 0, 1
EncScript_GalarianMoltres_DreadUp_Done:
	return

EncScript_GalarianMoltres_DreadDown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_GalarianMoltres_DreadDown_Done
	encsubvar 0, 1
EncScript_GalarianMoltres_DreadDown_Done:
	return

// THE SINGLE WRITER of the offense stage, the damage reduction and the crit flag. Called after every
// Dread change so all three always match the meter's current value, never an earlier one.
EncScript_GalarianMoltres_ApplyDreadLevel:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_GalarianMoltres_ApplyDreadLevel_Offense1
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6
	goto EncScript_GalarianMoltres_ApplyDreadLevel_Reduction
EncScript_GalarianMoltres_ApplyDreadLevel_Offense1:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -5
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -5
EncScript_GalarianMoltres_ApplyDreadLevel_Reduction:
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_GalarianMoltres_ApplyDreadLevel_Red96
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_GalarianMoltres_ApplyDreadLevel_Red93
	encsetdamagereduction ENC_TARGET_BOSS, 86
	goto EncScript_GalarianMoltres_ApplyDreadLevel_Crit
EncScript_GalarianMoltres_ApplyDreadLevel_Red93:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	goto EncScript_GalarianMoltres_ApplyDreadLevel_Crit
EncScript_GalarianMoltres_ApplyDreadLevel_Red96:
	encsetdamagereduction ENC_TARGET_BOSS, 90
EncScript_GalarianMoltres_ApplyDreadLevel_Crit:
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_GalarianMoltres_ApplyDreadLevel_CritOn
	encsetcrit ENC_TARGET_BOSS, FALSE
	goto EncScript_GalarianMoltres_ApplyDreadLevel_Aura
EncScript_GalarianMoltres_ApplyDreadLevel_CritOn:
	encsetcrit ENC_TARGET_BOSS, TRUE
EncScript_GalarianMoltres_ApplyDreadLevel_Aura:
// DARK AURA onset/fade (guideline 14): LastAnnounced(8) holds whether the last callout said "in" (5)
// or "out" (0), so the line only reprints on an actual crossing.
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_GalarianMoltres_ApplyDreadLevel_AuraEnter
	encjumpifvar CMP_NOT_EQUAL, 8, 5, EncScript_GalarianMoltres_ApplyDreadLevel_Done
	encsetvar 8, 0
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	printstring STRINGID_ENCGALARIANMOLTRESAURAFADE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianMoltres_ApplyDreadLevel_AuraEnter:
	encjumpifvar CMP_EQUAL, 8, 5, EncScript_GalarianMoltres_ApplyDreadLevel_Done
	encsetvar 8, 5
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	printstring STRINGID_ENCGALARIANMOLTRESAURAENTER
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianMoltres_ApplyDreadLevel_Done:
	return

// Rolls and applies a Nightmare mark on the mon that just switched in. Called from SwitchResist/
// SwitchVulnerable once Phase >= 1 and no mark is currently running.
EncScript_GalarianMoltres_NightmareRoll:
	encsetvar 6, 3
	printstring STRINGID_ENCGALARIANMOLTRESNIGHTMAREMARK
	waitmessage B_WAIT_TIME_LONG
	encjumpifchance 34, EncScript_GalarianMoltres_NightmareRoll_HealBlock
	encjumpifchance 52, EncScript_GalarianMoltres_NightmareRoll_Accuracy
	encsetvar 7, 2   @ periodic drain - ticked by EncScript_GalarianMoltres_NightmareTick
	return
EncScript_GalarianMoltres_NightmareRoll_HealBlock:
	encsetvar 7, 0
	encsethealblock ENC_TARGET_PLAYER_LEFT, 3
	return
EncScript_GalarianMoltres_NightmareRoll_Accuracy:
	encsetvar 7, 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ACC, -1
	return

// --- Entry points (named in galarian_moltres.encounter) ---

EncScript_GalarianMoltres_Intro::
	call EncScript_GalarianMoltres_ApplyDreadLevel
	return

EncScript_GalarianMoltres_DarkHit::
	encsetvar 2, 1
	call EncScript_GalarianMoltres_DreadUp
	call EncScript_GalarianMoltres_ApplyDreadLevel
	printstring STRINGID_ENCGALARIANMOLTRESDARKHIT
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_ChipHit::
	encsetvar 2, 1
	call EncScript_GalarianMoltres_DreadUp
	call EncScript_GalarianMoltres_ApplyDreadLevel
	printstring STRINGID_ENCGALARIANMOLTRESCHIPHIT
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_ClearMoveGuard::
	encsetvar 2, 0
	return

EncScript_GalarianMoltres_CourageCheck::
	encsetvar 5, 1
	call EncScript_GalarianMoltres_DreadDown
	call EncScript_GalarianMoltres_ApplyDreadLevel
	printstring STRINGID_ENCGALARIANMOLTRESCOURAGEHIT
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_ClearBossHitGuard::
	encsetvar 5, 0
	return

EncScript_GalarianMoltres_FaintCheck::
	encsetvar 3, 1
	call EncScript_GalarianMoltres_DreadUp
	call EncScript_GalarianMoltres_DreadUp
	call EncScript_GalarianMoltres_ApplyDreadLevel
	printstring STRINGID_ENCGALARIANMOLTRESFAINTFEED
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_ClearFaintGuard::
	encsetvar 3, 0
	return

EncScript_GalarianMoltres_SwitchResist::
	encsetvar 4, 1
	printstring STRINGID_ENCGALARIANMOLTRESSTEELSNERVE
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_GalarianMoltres_SwitchResist_Done
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_GalarianMoltres_SwitchResist_Done
	call EncScript_GalarianMoltres_NightmareRoll
EncScript_GalarianMoltres_SwitchResist_Done:
	return

EncScript_GalarianMoltres_SwitchVulnerable::
	encsetvar 4, 1
	call EncScript_GalarianMoltres_DreadUp
	call EncScript_GalarianMoltres_ApplyDreadLevel
	printstring STRINGID_ENCGALARIANMOLTRESNEWCOMERWATCHED
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_GalarianMoltres_SwitchVulnerable_Done
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_GalarianMoltres_SwitchVulnerable_Done
	call EncScript_GalarianMoltres_NightmareRoll
EncScript_GalarianMoltres_SwitchVulnerable_Done:
	return

EncScript_GalarianMoltres_ClearSwitchGuard::
	encsetvar 4, 0
	return

EncScript_GalarianMoltres_ClearLifeDrainGuard::
	encsetvar 9, 0
	return

EncScript_GalarianMoltres_ClearNightmareGuard::
	encsetvar 10, 0
	return

EncScript_GalarianMoltres_StatusCheck::
	call EncScript_GalarianMoltres_DreadUp
	call EncScript_GalarianMoltres_ApplyDreadLevel
	printstring STRINGID_ENCGALARIANMOLTRESSTATUSDREAD
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_BreakDread::
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -25, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	printstring STRINGID_ENCGALARIANMOLTRESBREAKAURA
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_Phase2Enter::
	encsetvar 1, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCGALARIANMOLTRESPHASE2
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_Phase3Enter::
	encsetvar 1, 2
	encsetvar 0, 5
	call EncScript_GalarianMoltres_ApplyDreadLevel
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCGALARIANMOLTRESPHASE3
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_GalarianMoltres_BallsUnlock::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	printstring STRINGID_ENCGALARIANMOLTRESBALLSUNLOCK
	waitmessage B_WAIT_TIME_LONG
	return

// A level - fires every qualifying turn. Four fixed bands rather than a computed amount so the
// player-facing number and the boss's own heal always agree. LifeDrainGuard(9) is set first: the
// condition it's gated on (Dread >= 2) doesn't clear itself, so without the guard the checkpoint's
// re-evaluation loop would reselect this every pass until the runaway-trigger cap trips.
EncScript_GalarianMoltres_LifeDrain::
	encsetvar 9, 1
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_GalarianMoltres_LifeDrain_5
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_GalarianMoltres_LifeDrain_4
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_GalarianMoltres_LifeDrain_3
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGALARIANMOLTRESLIFEDRAIN2
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianMoltres_LifeDrain_3:
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGALARIANMOLTRESLIFEDRAIN3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianMoltres_LifeDrain_4:
	encchangehp ENC_TARGET_PLAYER_LEFT, -15, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGALARIANMOLTRESLIFEDRAIN4
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianMoltres_LifeDrain_5:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, 4, ENC_AMOUNT_PERCENT
// Dread 5's line doubles as the recurring DARK AURA cue (guideline 14) - Life Drain already fires
// every qualifying turn, so it's the natural home for "the mechanic is still live" instead of a
// thirteenth trigger.
	printstring STRINGID_ENCGALARIANMOLTRESLIFEDRAIN5
	waitmessage B_WAIT_TIME_LONG
	return

// NightmareGuard(10) is set first for the same reason LifeDrainGuard is: Var(NightmareTimer) > 0
// doesn't clear itself, so this level needs its own once-per-checkpoint guard.
EncScript_GalarianMoltres_NightmareTick::
	encsetvar 10, 1
	encsubvar 6, 1
	encjumpifvar CMP_NOT_EQUAL, 7, 2, EncScript_GalarianMoltres_NightmareTick_CheckExpire
	encchangehp ENC_TARGET_PLAYER_LEFT, -3, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGALARIANMOLTRESNIGHTMAREGNAW
	waitmessage B_WAIT_TIME_LONG
EncScript_GalarianMoltres_NightmareTick_CheckExpire:
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_GalarianMoltres_NightmareTick_Done
	printstring STRINGID_ENCGALARIANMOLTRESNIGHTMAREFADE
	waitmessage B_WAIT_TIME_LONG
EncScript_GalarianMoltres_NightmareTick_Done:
	return
