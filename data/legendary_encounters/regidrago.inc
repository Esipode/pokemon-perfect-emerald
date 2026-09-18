@ Regidrago encounter scripts. Var index map (declared in this order by
@ src/data/legendary_encounters/regidrago.encounter, first appearance wins):
@ 0 Energy (0-100), 1 Phase (0 Awakened/1 AncientDragon/2 Overload), 2 OrbState (0 none/1 active),
@ 3 OrbTimer, 4 MawState (0 idle/1 telegraphed), 5 MawTimer, 6 HungerStreak, 7 TurnGuard,
@ 8 BossMark (turn-open boss HP%, then this turn's damage taken), 9 PlayerMark (turn-open player
@ HP%, then this turn's damage taken - what the boss dealt), 10 DragonUsed (latch, cleared at turn
@ open), 11 LastTier (last announced Energy tier), 12 TheftActive (Phase 1+, cleared by an Orb
@ kill), 13 Overloaded (Phase 2 entered), 14 BankedEnergy (Energy snapshotted at Phase 2 entry,
@ scales Eternabeam), 15 FaintHandled (self-disabling latch for the OnFaint trigger, cleared at
@ turn open).

EncScript_Regidrago_Intro::
	printstring STRINGID_ENCREGIDRAGOINTRO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

@ Clamps Energy (var 0) down to 100 after any addition. Additions are all small (<=20) so a single
@ call never wraps a u8; only ever called after encaddvar on var 0.
EncScript_Regidrago_ClampEnergy:
	encjumpifvar CMP_LESS_THAN, 0, 101, EncScript_Regidrago_ClampEnergy_Done
	encsetvar 0, 100
EncScript_Regidrago_ClampEnergy_Done:
	return

@ ---------------------------------------------------------------------------------------------
@ Phase transitions
@ ---------------------------------------------------------------------------------------------

EncScript_Regidrago_AncientDragon::
	encsetvar 1, 1   @ Phase = AncientDragon
	printstring STRINGID_ENCREGIDRAGOPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	return

EncScript_Regidrago_Overload::
	encsetvar 1, 2     @ Phase = Overload
	encsetvar 12, 0    @ TheftActive ends - a flat per-turn drain replaces it
	enccopyvar 14, 0   @ BankedEnergy = Energy banked right before it's forced up
	encsetvar 0, 100   @ Energy forced to 100
	encsetvar 11, 100  @ LastTier already maxed - no more tier callouts this fight
	encsetvar 13, 1    @ Overloaded = TRUE
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetcrit ENC_TARGET_BOSS, TRUE
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -4   @ absolute +2 stage
	printstring STRINGID_ENCREGIDRAGOPHASE3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

@ Energy hit 0 under Overload, or the boss dropped to 10% HP - whichever came first. Fires the
@ finisher, then tears down every mechanic the fight raised and opens the catch window.
EncScript_Regidrago_Eternabeam::
	printstring STRINGID_ENCREGIDRAGOETERNABEAM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	@ payload scaled by the Energy banked at Overload entry (guideline 7: 30% explosive ceiling)
	encjumpifvar CMP_GREATER_THAN, 14, 74, EncScript_Regidrago_Eternabeam_BigHit
	encjumpifvar CMP_GREATER_THAN, 14, 39, EncScript_Regidrago_Eternabeam_MidHit
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Regidrago_Eternabeam_AfterHit
EncScript_Regidrago_Eternabeam_MidHit:
	encchangehp ENC_TARGET_PLAYER_LEFT, -22, ENC_AMOUNT_PERCENT
	goto EncScript_Regidrago_Eternabeam_AfterHit
EncScript_Regidrago_Eternabeam_BigHit:
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
EncScript_Regidrago_Eternabeam_AfterHit:
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetvar 0, 0
	encsetvar 14, 0
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encsetcrit ENC_TARGET_BOSS, FALSE
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Regidrago_Eternabeam_NoStats
EncScript_Regidrago_Eternabeam_NoStats:
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCREGIDRAGOBALLSUNLOCK
	waitmessage B_WAIT_TIME_LONG
	return

@ ---------------------------------------------------------------------------------------------
@ Once-per-turn scaffold: opens the turn, snapshots HP marks, rolls new Orb/Maw telegraphs.
@ ---------------------------------------------------------------------------------------------

EncScript_Regidrago_TurnOpen::
	encsetvar 7, 1    @ TurnGuard opens
	encsetvar 10, 0   @ DragonUsed latch cleared
	encsetvar 15, 0   @ FaintHandled latch cleared
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_SET
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Regidrago_TurnOpen_MawRoll
	encsubvar 3, 1    @ OrbTimer -= 1 (0 means it's consumed uneaten at TurnClose)
EncScript_Regidrago_TurnOpen_MawRoll:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Regidrago_TurnOpen_OrbRoll
	encjumpifchance 15, EncScript_Regidrago_TurnOpen_MawBegin
	goto EncScript_Regidrago_TurnOpen_OrbRoll
EncScript_Regidrago_TurnOpen_MawBegin:
	encsetvar 4, 1
	encsetvar 5, 2
	printstring STRINGID_ENCREGIDRAGOMAWOPEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
EncScript_Regidrago_TurnOpen_OrbRoll:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regidrago_TurnOpen_Done
	encjumpifchance 20, EncScript_Regidrago_TurnOpen_OrbBegin
	goto EncScript_Regidrago_TurnOpen_Done
EncScript_Regidrago_TurnOpen_OrbBegin:
	encsetvar 2, 1
	encsetvar 3, 3
	printstring STRINGID_ENCREGIDRAGOORBSPAWN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
EncScript_Regidrago_TurnOpen_Done:
	return

@ Dragon-move bonus. Regidrago's own move is checked first (higher priority) and both share the
@ DragonUsed latch, so only one bonus lands per turn even if both sides used a Dragon move.
EncScript_Regidrago_DragonMoveBonusBoss::
	encsetvar 10, 1
	encaddvar 0, 10
	call EncScript_Regidrago_ClampEnergy
	return

EncScript_Regidrago_DragonMoveBonusPlayer::
	encsetvar 10, 1
	encaddvar 0, 6
	call EncScript_Regidrago_ClampEnergy
	return

EncScript_Regidrago_EnergyFromFaint::
	encsetvar 15, 1
	encaddvar 0, 20
	call EncScript_Regidrago_ClampEnergy
	return

@ ---------------------------------------------------------------------------------------------
@ The ledger. Runs once per turn at OnTurnEnd: Energy gain/drain, Orb, Maw, Hunger, Theft, and the
@ tier callouts, all as one script so the checkpoint's 8-script cap is never in play.
@ ---------------------------------------------------------------------------------------------

EncScript_Regidrago_TurnClose::
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_DAMAGE        @ var8 = boss damage taken this turn
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_DAMAGE @ var9 = player damage taken this turn
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Regidrago_TurnClose_Drain

	@ --- Energy from real damage traded, bucketed off the two deltas above ---
	encjumpifvar CMP_GREATER_THAN, 8, 19, EncScript_Regidrago_TurnClose_BossHitBig
	encjumpifvar CMP_GREATER_THAN, 8, 4, EncScript_Regidrago_TurnClose_BossHitSmall
	encjumpifvar CMP_GREATER_THAN, 8, 0, EncScript_Regidrago_TurnClose_BossHitTiny
	goto EncScript_Regidrago_TurnClose_BossHitDone
EncScript_Regidrago_TurnClose_BossHitBig:
	encaddvar 0, 15
	call EncScript_Regidrago_ClampEnergy
	goto EncScript_Regidrago_TurnClose_BossHitDone
EncScript_Regidrago_TurnClose_BossHitSmall:
	encaddvar 0, 8
	call EncScript_Regidrago_ClampEnergy
	goto EncScript_Regidrago_TurnClose_BossHitDone
EncScript_Regidrago_TurnClose_BossHitTiny:
	encaddvar 0, 3
	call EncScript_Regidrago_ClampEnergy
EncScript_Regidrago_TurnClose_BossHitDone:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Regidrago_TurnClose_PassiveRise
	encaddvar 0, 5
	call EncScript_Regidrago_ClampEnergy

EncScript_Regidrago_TurnClose_PassiveRise:
	encaddvar 0, 1     @ baseline drift, every phase
	call EncScript_Regidrago_ClampEnergy
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Regidrago_TurnClose_Hunger
	encaddvar 0, 3     @ Ancient Dragon's auto-rise, on top of the baseline
	call EncScript_Regidrago_ClampEnergy

EncScript_Regidrago_TurnClose_Hunger:
	@ --- Hunger: no damage moved either way for 3+ turns straight ---
	encjumpifvar CMP_GREATER_THAN, 8, 0, EncScript_Regidrago_TurnClose_ResetHunger
	encjumpifvar CMP_GREATER_THAN, 9, 0, EncScript_Regidrago_TurnClose_ResetHunger
	encaddvar 6, 1
	goto EncScript_Regidrago_TurnClose_HungerApply
EncScript_Regidrago_TurnClose_ResetHunger:
	encsetvar 6, 0
EncScript_Regidrago_TurnClose_HungerApply:
	encjumpifvar CMP_GREATER_THAN, 6, 2, EncScript_Regidrago_TurnClose_HungerOn
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Regidrago_TurnClose_Orb
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -6    @ absolute neutral
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -6  @ absolute neutral
	goto EncScript_Regidrago_TurnClose_Orb
EncScript_Regidrago_TurnClose_HungerOn:
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -5    @ absolute +1 stage
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -5  @ absolute +1 stage
	printstring STRINGID_ENCREGIDRAGOHUNGER
	waitmessage B_WAIT_TIME_LONG

EncScript_Regidrago_TurnClose_Orb:
	@ --- Orb: destroyed by a big hit this turn, consumed by timeout, or left alone ---
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Regidrago_TurnClose_Maw
	encjumpifvar CMP_GREATER_THAN, 8, 19, EncScript_Regidrago_TurnClose_OrbDestroyed
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Regidrago_TurnClose_OrbConsumed
	goto EncScript_Regidrago_TurnClose_Maw
EncScript_Regidrago_TurnClose_OrbDestroyed:
	encsetvar 2, 0
	encsetvar 12, 0   @ destroying an Orb also breaks Energy Theft's lock
	encjumpifvar CMP_LESS_THAN, 0, 5, EncScript_Regidrago_TurnClose_OrbDestroyedFloor
	encsubvar 0, 5
	goto EncScript_Regidrago_TurnClose_Maw
EncScript_Regidrago_TurnClose_OrbDestroyedFloor:
	encsetvar 0, 0
	goto EncScript_Regidrago_TurnClose_Maw
EncScript_Regidrago_TurnClose_OrbConsumed:
	encsetvar 2, 0
	encaddvar 0, 15
	call EncScript_Regidrago_ClampEnergy
	printstring STRINGID_ENCREGIDRAGOORBCONSUMED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL

EncScript_Regidrago_TurnClose_Maw:
	@ --- Dragon Maw: interrupted by a big hit this turn, else counts down and resolves ---
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Regidrago_TurnClose_TheftOrTier
	encjumpifvar CMP_GREATER_THAN, 8, 19, EncScript_Regidrago_TurnClose_MawInterrupt
	encsubvar 5, 1
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Regidrago_TurnClose_TheftOrTier
	encsetvar 4, 0
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCREGIDRAGOMAWHIT
	waitmessage B_WAIT_TIME_LONG
	encaddvar 0, 12
	call EncScript_Regidrago_ClampEnergy
	goto EncScript_Regidrago_TurnClose_TheftOrTier
EncScript_Regidrago_TurnClose_MawInterrupt:
	encsetvar 4, 0
	encjumpifvar CMP_LESS_THAN, 0, 5, EncScript_Regidrago_TurnClose_MawInterruptFloor
	encsubvar 0, 5
	goto EncScript_Regidrago_TurnClose_MawInterruptDone
EncScript_Regidrago_TurnClose_MawInterruptFloor:
	encsetvar 0, 0
EncScript_Regidrago_TurnClose_MawInterruptDone:
	printstring STRINGID_ENCREGIDRAGOMAWINTERRUPT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED

EncScript_Regidrago_TurnClose_TheftOrTier:
	@ --- Energy Theft: Phase 1+ only, ticks until an Orb kill breaks the lock ---
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Regidrago_TurnClose_TierAnnounce
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Regidrago_TurnClose_TheftStart
	goto EncScript_Regidrago_TurnClose_TheftDrain
EncScript_Regidrago_TurnClose_TheftStart:
	encsetvar 12, 1
	printstring STRINGID_ENCREGIDRAGOTHEFTSTART
	waitmessage B_WAIT_TIME_LONG
EncScript_Regidrago_TurnClose_TheftDrain:
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	goto EncScript_Regidrago_TurnClose_TierAnnounce

@ Overload's flat drain replaces every Phase 0/1 subsystem above - none of it applies once
@ Overloaded is set, so this branches straight from the top of the script.
EncScript_Regidrago_TurnClose_Drain:
	encjumpifvar CMP_LESS_THAN, 0, 8, EncScript_Regidrago_TurnClose_DrainFloor
	encsubvar 0, 8
	goto EncScript_Regidrago_TurnClose_EndTurn
EncScript_Regidrago_TurnClose_DrainFloor:
	encsetvar 0, 0
	goto EncScript_Regidrago_TurnClose_EndTurn

@ --- Energy tier callouts. Cascades low-to-high so a turn that jumps several bands in one go
@ still announces and applies each one it crossed. LastTier only ever climbs (guideline-7-safe
@ minor simplification: a tier's buff, once granted, is not stripped by a later Energy dip). ---
EncScript_Regidrago_TurnClose_TierAnnounce:
	encjumpifvar CMP_GREATER_THAN, 11, 24, EncScript_Regidrago_TurnClose_Tier50Check
	encjumpifvar CMP_LESS_THAN, 0, 25, EncScript_Regidrago_TurnClose_EndTurn
	encsetvar 11, 25
	printstring STRINGID_ENCREGIDRAGOTIER25
	waitmessage B_WAIT_TIME_LONG
EncScript_Regidrago_TurnClose_Tier50Check:
	encjumpifvar CMP_GREATER_THAN, 11, 49, EncScript_Regidrago_TurnClose_Tier75Check
	encjumpifvar CMP_LESS_THAN, 0, 50, EncScript_Regidrago_TurnClose_EndTurn
	encsetvar 11, 50
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -5   @ absolute +1 stage
	printstring STRINGID_ENCREGIDRAGOTIER50
	waitmessage B_WAIT_TIME_LONG
EncScript_Regidrago_TurnClose_Tier75Check:
	encjumpifvar CMP_GREATER_THAN, 11, 74, EncScript_Regidrago_TurnClose_Tier100Check
	encjumpifvar CMP_LESS_THAN, 0, 75, EncScript_Regidrago_TurnClose_EndTurn
	encsetvar 11, 75
	encsetcrit ENC_TARGET_BOSS, TRUE
	printstring STRINGID_ENCREGIDRAGOTIER75
	waitmessage B_WAIT_TIME_LONG
EncScript_Regidrago_TurnClose_Tier100Check:
	encjumpifvar CMP_GREATER_THAN, 11, 99, EncScript_Regidrago_TurnClose_EndTurn
	encjumpifvar CMP_LESS_THAN, 0, 100, EncScript_Regidrago_TurnClose_EndTurn
	encsetvar 11, 100
	printstring STRINGID_ENCREGIDRAGOTIER100
	waitmessage B_WAIT_TIME_LONG

EncScript_Regidrago_TurnClose_EndTurn:
	encsetvar 7, 0   @ TurnGuard closes
	return
