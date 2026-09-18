// Galarian Zapdos, "The Relentless Pursuer". Var indices, pinned by the always-true Conditions on
// EncScript_GalarianZapdos_Init:
// 0 Momentum (0-5), 1 Phase (1-3), 2 EncounterStarted, 3 TurnGuard, 4 HitGuardGain,
// 5 PlayerHitGuard, 6 BossAttackGuard, 7 StatusGuard, 8 DmgMark, 9 DmgPct, 10 BossDmgMark,
// 11 BossDmgPct, 12 ComboLocked, 13 PursuitGuard (dual-purpose - see below), 14 AssaultGuard,
// 15 SEGuard.
//
// MOMENTUM IS THE GUARD, IN BOTH DIRECTIONS. Landed hits and KOs raise it; a Stagger, a blocked
// Protect, a status, or a super-effective hit taken all lower it. EncGZapdos_ClampMomentum is the
// single clamp every writer calls through, [0,5].
//
// PursuitGuard IS SHARED between Pursuit (OnSwitchIn) and KOGain (OnFaint) - both gate "did
// something already happen to this switch/faint pair this dispatch", and a KO's forced replacement
// switch-in deliberately doesn't also draw Pursuit, the same idiom other encounters use for a
// faint+switch-in pair.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE CLAMP. Momentum changes are small (+1/+2/-1/-2), so an underflow wraps to the top of
// the byte range (>200) and an overflow lands just above 5 - the two cases are distinguishable by
// that gap.
EncGZapdos_ClampMomentum:
	encjumpifvar CMP_GREATER_THAN, 0, 200, EncGZapdos_ClampMomentum_Low
	encjumpifvar CMP_GREATER_THAN, 0, 5, EncGZapdos_ClampMomentum_High
	return
EncGZapdos_ClampMomentum_Low:
	encsetvar 0, 0
	return
EncGZapdos_ClampMomentum_High:
	encsetvar 0, 5
	return

// THE COMBO BONUS. Tier-scaled chip damage on top of Zapdos's own landed hit, silenced while
// ComboLocked (a fresh Stagger) and at Momentum 5, where Relentless Assault takes over instead.
EncGZapdos_ApplyComboBonus:
	encjumpifvar CMP_EQUAL, 12, 1, EncGZapdos_ApplyComboBonus_Done
	encjumpifvar CMP_LESS_THAN, 0, 2, EncGZapdos_ApplyComboBonus_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncGZapdos_ApplyComboBonus_6
	encjumpifvar CMP_EQUAL, 0, 3, EncGZapdos_ApplyComboBonus_10
	encjumpifvar CMP_EQUAL, 0, 4, EncGZapdos_ApplyComboBonus_14
	return
EncGZapdos_ApplyComboBonus_6:
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGALARIANZAPDOSCOMBOCONTINUES
	waitmessage B_WAIT_TIME_SHORT
	return
EncGZapdos_ApplyComboBonus_10:
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGALARIANZAPDOSCOMBOCONTINUES
	waitmessage B_WAIT_TIME_SHORT
	return
EncGZapdos_ApplyComboBonus_14:
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -14, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGALARIANZAPDOSCOMBOCONTINUES
	waitmessage B_WAIT_TIME_SHORT
	return
EncGZapdos_ApplyComboBonus_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_GalarianZapdos_Init::
	encsetvar 0, 0     @ Momentum
	encsetvar 1, 1     @ Phase
	encsetvar 2, 1     @ EncounterStarted
	encsetvar 3, 0     @ TurnGuard
	encsetvar 4, 0     @ HitGuardGain
	encsetvar 5, 0     @ PlayerHitGuard
	encsetvar 6, 0     @ BossAttackGuard
	encsetvar 7, 0     @ StatusGuard
	encsetvar 8, 0     @ DmgMark
	encsetvar 9, 0     @ DmgPct
	encsetvar 10, 0    @ BossDmgMark
	encsetvar 11, 0    @ BossDmgPct
	encsetvar 12, 0    @ ComboLocked
	encsetvar 13, 0    @ PursuitGuard
	encsetvar 14, 0    @ AssaultGuard
	encsetvar 15, 0    @ SEGuard
	return

// Top of every turn: resets every per-dispatch guard, snapshots both HP marks.
EncScript_GalarianZapdos_TurnStartReset::
	encsetvar 3, 1     @ TurnGuard
	encsetvar 4, 0     @ HitGuardGain
	encsetvar 5, 0     @ PlayerHitGuard
	encsetvar 6, 0     @ BossAttackGuard
	encsetvar 13, 0    @ PursuitGuard
	encsetvar 14, 0    @ AssaultGuard
	encsetvar 15, 0    @ SEGuard
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 8, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_SET
	return

EncScript_GalarianZapdos_TurnEndClearGuard::
	encsetvar 3, 0     @ TurnGuard
	return

// RELENTLESS ASSAULT. Momentum 5 fires a genuine bonus hit, then partially burns itself back to 3.
// Phase 3 also chips Zapdos's own HP for "exertion" (it can beat itself).
EncScript_GalarianZapdos_RelentlessAssault::
	encsetvar 14, 1    @ AssaultGuard
	printstring STRINGID_ENCGALARIANZAPDOSRELENTLESSASSAULT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_GalarianZapdos_RelentlessAssault_Phase3
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_GalarianZapdos_RelentlessAssault_Sub
EncScript_GalarianZapdos_RelentlessAssault_Phase3:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, -6, ENC_AMOUNT_PERCENT
EncScript_GalarianZapdos_RelentlessAssault_Sub:
	encsubvar 0, 2     @ Momentum -= 2
	return

EncScript_GalarianZapdos_ComboLockedClear::
	encsetvar 12, 0    @ ComboLocked
	return

EncScript_GalarianZapdos_StatusGuardClear::
	encsetvar 7, 0     @ StatusGuard
	return

// Zapdos's hit landed on the player. Base +1, another +1 on a vicious blow (>=12% of the player's
// max HP in one shot), then the combo bonus if the tier and the lock allow it.
EncScript_GalarianZapdos_MomentumGainOnHit::
	encsetvar 4, 1     @ HitGuardGain
	enccopyvar 9, 8    @ DmgPct = DmgMark
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_DAMAGE, EncScript_GalarianZapdos_MomentumGainOnHit_NoDamage
	goto EncScript_GalarianZapdos_MomentumGainOnHit_Measured
EncScript_GalarianZapdos_MomentumGainOnHit_NoDamage:
	encsetvar 9, 0
EncScript_GalarianZapdos_MomentumGainOnHit_Measured:
	encaddvar 0, 1     @ Momentum +1
	encjumpifvar CMP_LESS_THAN, 9, 12, EncScript_GalarianZapdos_MomentumGainOnHit_Clamp
	encaddvar 0, 1     @ vicious blow, +1 more
	printstring STRINGID_ENCGALARIANZAPDOSVICIOUSBLOW
	waitmessage B_WAIT_TIME_SHORT
EncScript_GalarianZapdos_MomentumGainOnHit_Clamp:
	call EncGZapdos_ClampMomentum
	call EncGZapdos_ApplyComboBonus
	return

// The player's hit landed on Zapdos. The Stagger check: >=18% of Zapdos's max HP in one shot, only
// while Momentum>=3, knocks it down 2 and locks the combo bonus out until it rebuilds.
EncScript_GalarianZapdos_PlayerHitsBoss::
	encsetvar 5, 1     @ PlayerHitGuard
	enccopyvar 11, 10  @ BossDmgPct = BossDmgMark
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_DAMAGE, EncScript_GalarianZapdos_PlayerHitsBoss_NoDamage
	goto EncScript_GalarianZapdos_PlayerHitsBoss_Measured
EncScript_GalarianZapdos_PlayerHitsBoss_NoDamage:
	encsetvar 11, 0
EncScript_GalarianZapdos_PlayerHitsBoss_Measured:
	encjumpifvar CMP_LESS_THAN, 11, 18, EncScript_GalarianZapdos_PlayerHitsBoss_Done
	encjumpifvar CMP_LESS_THAN, 0, 3, EncScript_GalarianZapdos_PlayerHitsBoss_Done
	printstring STRINGID_ENCGALARIANZAPDOSSTAGGERED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsubvar 0, 2
	call EncGZapdos_ClampMomentum
	encsetvar 12, 1    @ ComboLocked
EncScript_GalarianZapdos_PlayerHitsBoss_Done:
	return

// The same hit, tested for Zapdos's own Fighting-type weakness (Flying/Psychic/Fairy, gated in the
// trigger's Conditions - Event.MoveType can't be read here).
EncScript_GalarianZapdos_SELoss::
	encsetvar 15, 1    @ SEGuard
	printstring STRINGID_ENCGALARIANZAPDOSWEAKNESSHIT
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsubvar 0, 1
	call EncGZapdos_ClampMomentum
	return

EncScript_GalarianZapdos_ProtectedLoss::
	encsetvar 6, 1     @ BossAttackGuard
	printstring STRINGID_ENCGALARIANZAPDOSOVEREXTENDS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsubvar 0, 1
	call EncGZapdos_ClampMomentum
	return

EncScript_GalarianZapdos_StatusLoss::
	encsetvar 7, 1     @ StatusGuard
	printstring STRINGID_ENCGALARIANZAPDOSTHROWNOFF
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsubvar 0, 1
	call EncGZapdos_ClampMomentum
	return

// +2 Momentum on a KO. Reuses PursuitGuard so the forced replacement switch-in doesn't also draw
// Pursuit.
EncScript_GalarianZapdos_KOGain::
	encsetvar 13, 1    @ PursuitGuard
	printstring STRINGID_ENCGALARIANZAPDOSMOMENTUMSURGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encaddvar 0, 2
	call EncGZapdos_ClampMomentum
	return

// A switch-in draws Pursuit unless the incoming mon is Ground-type, which shrugs it off for a
// Momentum tick instead. jumpiftype reads the battler directly, so it's valid at this checkpoint.
EncScript_GalarianZapdos_Pursuit::
	encsetvar 13, 1    @ PursuitGuard
	jumpiftype BS_PLAYER1, TYPE_GROUND, EncScript_GalarianZapdos_Pursuit_Countered
	printstring STRINGID_ENCGALARIANZAPDOSGIVESCHASE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_GalarianZapdos_Pursuit_P1
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	return
EncScript_GalarianZapdos_Pursuit_P1:
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	return
EncScript_GalarianZapdos_Pursuit_Countered:
	printstring STRINGID_ENCGALARIANZAPDOSPURSUITCOUNTERED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	encsubvar 0, 1
	call EncGZapdos_ClampMomentum
	return

// --- Phase transitions ---

// 50% - BREAKNECK SPEED. Guard hardens to 92; Pursuit intensifies (read directly in _Pursuit off
// Phase). Momentum thresholds are unchanged from Phase 1.
EncScript_GalarianZapdos_Phase2::
	encsetvar 1, 2     @ Phase
	printstring STRINGID_ENCGALARIANZAPDOSFRENZY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return

// 15% - LAST STAND. Guard drops to 80 (more hittable), balls unlock, catch rate raised. No
// encsetsurvive/teardown: this fight is meant to still be won or lost fighting, not spent-and-caught
// like a scripted last-stand boss.
EncScript_GalarianZapdos_Phase3::
	encsetvar 1, 3     @ Phase
	printstring STRINGID_ENCGALARIANZAPDOSREFUSESTOSTOP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	return
