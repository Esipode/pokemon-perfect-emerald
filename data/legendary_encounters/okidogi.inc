@ Okidogi, "The Retaliatory Brawler". Variable map (see okidogi.encounter's header for the full
@ rationale): 0 Rage, 1 Phase, 2 HitGuard, 3 AtkGuard, 4 ComboStep, 5 BerserkFailGuard, 6 TurnGuard,
@ 7 ToxicGuard, 8 ResentmentArmed, 9 Mark.

EncScript_Okidogi_Intro::
	return

@ Single writer for Okidogi's own Attack stage: RAGE 1-2 is +1, RAGE 3+ is +2 (guideline 7's cap).
@ Pushed to -12 then rebuilt from neutral every call, so a Haze or a switch can never leave it out
@ of sync with the current RAGE value.
EncScript_Okidogi_ApplyRageAtk:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Okidogi_ApplyRageAtk_Tier2
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Okidogi_ApplyRageAtk_Done
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	return
EncScript_Okidogi_ApplyRageAtk_Tier2:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 8
	return
EncScript_Okidogi_ApplyRageAtk_Done:
	return

@ The once-per-turn scaffold. Also takes the turn-open HP snapshot every RageOnAttack call reads.
EncScript_Okidogi_TurnOpen::
	encsetvar 2, 0
	encsetvar 3, 0
	encsetvar 5, 0
	encsetvar 7, 0
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_SET
	encsetvar 6, 1
	return

EncScript_Okidogi_TurnClose::
	encsetvar 6, 0
	return

@ RAGE ON HIT. Multi-hit moves only grant +1 per turn (HitGuard), not per strike.
EncScript_Okidogi_RageOnHit::
	encsetvar 2, 1
	encjumpifvar CMP_EQUAL, 0, 6, EncScript_Okidogi_RageOnHit_Done
	encaddvar 0, 1
EncScript_Okidogi_RageOnHit_Done:
	call EncScript_Okidogi_ApplyRageAtk
	return

@ RAGE ON ATTACK. The single point that reacts to "Okidogi's own move resolved" (hit or miss - the
@ checkpoint fires either way). AtkGuard is closed first, before anything optional runs, so every
@ path out of this script leaves the latch shut. Handles the Rage-1 cooldown, Brutal Combo
@ progression, the Resentment/Combo-step-1 bonus payout, and (Berserker only) the failed-attack
@ bait and Collision Course finisher - all off the damage-since-mark read in Var(9), since
@ Event.Cause/OldValue/NewValue can't tell a landed hit from a whiff at this checkpoint.
EncScript_Okidogi_RageOnAttack::
	encsetvar 3, 1
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_DAMAGE, EncScript_Okidogi_RageOnAttack_PhaseSplit
EncScript_Okidogi_RageOnAttack_PhaseSplit:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Okidogi_RageOnAttack_Berserker
	@ Normal/Unchained: landing its own hit calms it down
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Okidogi_RageOnAttack_SkipCooldown
	encsubvar 0, 1
	call EncScript_Okidogi_ApplyRageAtk
EncScript_Okidogi_RageOnAttack_SkipCooldown:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Okidogi_RageOnAttack_Unchained
	goto EncScript_Okidogi_RageOnAttack_Resentment

@ Berserker's RAGE math is entirely the fail-drain / hit-at-6-reset pair below - a landed hit below
@ RAGE 6 leaves it untouched, climbing back up only through Resentment/Toxic Feed until it refills.
EncScript_Okidogi_RageOnAttack_Berserker:
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Okidogi_RageOnAttack_BerserkHit
	encsetvar 5, 1
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Okidogi_RageOnAttack_BerserkFailFloor
	encsubvar 0, 2
	goto EncScript_Okidogi_RageOnAttack_BerserkFailDone
EncScript_Okidogi_RageOnAttack_BerserkFailFloor:
	encsetvar 0, 0
EncScript_Okidogi_RageOnAttack_BerserkFailDone:
	call EncScript_Okidogi_ApplyRageAtk
	printstring STRINGID_ENCOKIDOGIBERSERKFAIL
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Okidogi_RageOnAttack_Resentment
EncScript_Okidogi_RageOnAttack_BerserkHit:
	encjumpifvar CMP_NOT_EQUAL, 0, 6, EncScript_Okidogi_RageOnAttack_Resentment
	encsetvar 0, 0
	call EncScript_Okidogi_ApplyRageAtk
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCOKIDOGICOLLISIONCOURSE
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
	goto EncScript_Okidogi_RageOnAttack_Resentment

EncScript_Okidogi_RageOnAttack_Unchained:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Okidogi_RageOnAttack_ComboStart
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Okidogi_RageOnAttack_ComboStep2
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Okidogi_RageOnAttack_ComboFinish
	goto EncScript_Okidogi_RageOnAttack_Resentment
EncScript_Okidogi_RageOnAttack_ComboStart:
	encsetvar 4, 1
	encsetvar 8, 1
	goto EncScript_Okidogi_RageOnAttack_Resentment
EncScript_Okidogi_RageOnAttack_ComboStep2:
	encsetvar 4, 2
	goto EncScript_Okidogi_RageOnAttack_Resentment
EncScript_Okidogi_RageOnAttack_ComboFinish:
	encsetvar 4, 0
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Okidogi_RageOnAttack_Resentment

@ ResentmentArmed is consumed here regardless of which branch above set it (Resentment itself, or
@ Brutal Combo's step-1 heavy hit reusing the same flag).
EncScript_Okidogi_RageOnAttack_Resentment:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Okidogi_RageOnAttack_Done
	encsetvar 8, 0
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
EncScript_Okidogi_RageOnAttack_Done:
	return

@ RESENTMENT. Okidogi's own Attack stage getting knocked down reads as disrespect.
EncScript_Okidogi_Resentment::
	encjumpifvar CMP_EQUAL, 0, 6, EncScript_Okidogi_Resentment_NoRage
	encaddvar 0, 1
	call EncScript_Okidogi_ApplyRageAtk
EncScript_Okidogi_Resentment_NoRage:
	encsetvar 8, 1
	printstring STRINGID_ENCOKIDOGIRESENTMENT
	waitmessage B_WAIT_TIME_LONG
	return

@ COMBO INTERRUPT. Fires from either the defensive-drop or the status route.
EncScript_Okidogi_ComboInterrupt::
	encsetvar 4, 0
	encjumpifvar CMP_LESS_THAN, 0, 3, EncScript_Okidogi_ComboInterrupt_Floor
	encsubvar 0, 3
	goto EncScript_Okidogi_ComboInterrupt_Done
EncScript_Okidogi_ComboInterrupt_Floor:
	encsetvar 0, 0
EncScript_Okidogi_ComboInterrupt_Done:
	call EncScript_Okidogi_ApplyRageAtk
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCOKIDOGICOMBOINTERRUPT
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Okidogi_Unchained::
	encsetvar 1, 1
	encjumpifvar CMP_LESS_THAN, 0, 5, EncScript_Okidogi_Unchained_Add
	encsetvar 0, 6
	goto EncScript_Okidogi_Unchained_Applied
EncScript_Okidogi_Unchained_Add:
	encaddvar 0, 2
EncScript_Okidogi_Unchained_Applied:
	call EncScript_Okidogi_ApplyRageAtk
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCOKIDOGIUNCHAINED
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Okidogi_Berserker::
	encsetvar 1, 2
	encsetvar 0, 6
	call EncScript_Okidogi_ApplyRageAtk
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCOKIDOGIBERSERKER
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Okidogi_ToxicFeed::
	encsetvar 7, 1
	encaddvar 0, 1
	call EncScript_Okidogi_ApplyRageAtk
	return

EncScript_Okidogi_BallsUnlock::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 60
	printstring STRINGID_ENCOKIDOGIBALLSUNLOCKED
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Okidogi_RageTierLow::
	printstring STRINGID_ENCOKIDOGIRAGERISING
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Okidogi_RageTierHigh::
	printstring STRINGID_ENCOKIDOGIRAGEBOILING
	waitmessage B_WAIT_TIME_LONG
	return
