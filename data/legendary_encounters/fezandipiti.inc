@ Fezandipiti, "The Retainer Pokemon".
@ Var index map (must match src/data/legendary_encounters/fezandipiti.encounter's pinning block):
@ 0 Phase, 1 Glamour, 2 Perfume, 3 Frustration, 4 SpotlightTurns, 5 EnchantTimer,
@ 6 LastReductionAnnounced, 7 TurnGuard, 8 PlayerHpMark, 9 BossHpMark, 10 PeriodicGuard,
@ 11 HitGuard, 12 BreakGuard, 13 ReductionDipTimer.

EncScript_Fezandipiti_Intro::
	printstring STRINGID_ENCFEZANDIPITIINTRO
	waitmessage B_WAIT_TIME_LONG
	encsetvar 6, 90        @ LastReductionAnnounced = baseline, so the first real dip is the one that prints
	return

@ Once-per-turn scaffold: sets TurnGuard, clears the three roll guards for this turn, and marks both
@ sides' HP so BossHitResolved / EnchantBreak can read damage-since-mark later this turn.
EncScript_Fezandipiti_TurnOpen::
	encsetvar 7, 1          @ TurnGuard
	encsetvar 10, 0         @ PeriodicGuard
	encsetvar 11, 0         @ HitGuard
	encsetvar 12, 0         @ BreakGuard
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 8
	encsnapshothp ENC_TARGET_BOSS, 9
	return

@ SPOTLIGHT - pure self-buff, substituting for forced-targeting "stealing the show".
EncScript_Fezandipiti_SpotlightRoll::
	encsetvar 10, 1         @ PeriodicGuard - closed before the roll, so a miss still blocks the other two
	encjumpifchance 35, EncScript_Fezandipiti_SpotlightRoll_Hit
	return
EncScript_Fezandipiti_SpotlightRoll_Hit:
	call EncScript_Fezandipiti_SpotlightOn
	return

EncScript_Fezandipiti_SpotlightOn:
	printstring STRINGID_ENCFEZANDIPITISPOTLIGHTON
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 8        @ absolute +2, survives an intervening Haze
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 7    @ absolute +1
	encsetvar 4, 2           @ SpotlightTurns
	return

EncScript_Fezandipiti_SpotlightOff:
	printstring STRINGID_ENCFEZANDIPITISPOTLIGHTOFF
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6        @ absolute 0
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6    @ absolute 0
	encsetvar 4, 0
	return

@ VANITY - Glamour maxed out cracks its own defenses open.
EncScript_Fezandipiti_Vanity::
	printstring STRINGID_ENCFEZANDIPITIVANITY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 5        @ absolute -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 5      @ absolute -1
	encsetvar 1, 3           @ Glamour drops to 3, not 0 - it doesn't forget being admired
	return

@ Fezandipiti's own move landing on (or whiffing past) the player. Hit stacks Perfume (doubled in
@ Perfect Beauty); a miss clears a stack of Perfume and feeds Glamour instead ("ignored").
EncScript_Fezandipiti_BossHitResolved::
	encsetvar 11, 1          @ HitGuard - closed first; Event.Battler stays true all dispatch
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 8, ENC_SNAP_DAMAGE  @ 8 becomes damage taken since TurnOpen's mark
	encjumpifvar ENC_CMP_EQ, 8, 0, EncScript_Fezandipiti_BossHitResolved_Dodge
@ Hit path
	encjumpifvar ENC_CMP_GE, 0, 1, EncScript_Fezandipiti_BossHitResolved_HitPerfect
	encaddvar 2, 1           @ Perfume + 1 (Debut)
	goto EncScript_Fezandipiti_BossHitResolved_Cap
EncScript_Fezandipiti_BossHitResolved_HitPerfect:
	encaddvar 2, 2           @ Perfume + 2 (Perfect Beauty / Jealous Beauty)
EncScript_Fezandipiti_BossHitResolved_Cap:
	encjumpifvar ENC_CMP_LE, 2, 3, EncScript_Fezandipiti_BossHitResolved_Return
	encsetvar 2, 3           @ clamp Perfume at 3
	return
@ Dodge path
EncScript_Fezandipiti_BossHitResolved_Dodge:
	encjumpifvar ENC_CMP_EQ, 2, 0, EncScript_Fezandipiti_BossHitResolved_DodgeGlamour
	encsubvar 2, 1           @ Perfume - 1 (floor 0, guarded above)
EncScript_Fezandipiti_BossHitResolved_DodgeGlamour:
	encjumpifvar ENC_CMP_LT, 1, 6, EncScript_Fezandipiti_BossHitResolved_DodgeGlamourAdd
	return
EncScript_Fezandipiti_BossHitResolved_DodgeGlamourAdd:
	encaddvar 1, 1           @ Glamour + 1 ("ignored")
EncScript_Fezandipiti_BossHitResolved_Return:
	return

@ The player's move landing on Fezandipiti, checked against Poisonous Kiss's break threshold.
EncScript_Fezandipiti_EnchantBreak::
	encsetvar 12, 1          @ BreakGuard - closed first; Event.Battler stays true all dispatch
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_DAMAGE   @ 9 becomes damage Fezandipiti took since TurnOpen's mark
	encjumpifvar ENC_CMP_GE, 9, 8, EncScript_Fezandipiti_EnchantBreak_Hit
	return
EncScript_Fezandipiti_EnchantBreak_Hit:
	printstring STRINGID_ENCFEZANDIPITIKISSBREAK
	waitmessage B_WAIT_TIME_LONG
	encsetvar 5, 0           @ EnchantTimer cleared
	encjumpifvar ENC_CMP_EQ, 1, 0, EncScript_Fezandipiti_EnchantBreak_Return
	encsubvar 1, 1           @ Glamour - 1 (floor 0, guarded above)
EncScript_Fezandipiti_EnchantBreak_Return:
	return

@ BEAUTIFUL DISTRACTION - the player-decision primitive. Focus feeds Glamour outright; Ignore builds
@ Frustration and, outside Jealous Beauty, trips a lash-out at 3.
EncScript_Fezandipiti_BeautifulDistraction::
	encsetvar 10, 1          @ PeriodicGuard - closed first; encaskyesno is the optional payload
	encaskyesno STRINGID_ENCFEZANDIPITIDISTRACTIONPROMPT, EncScript_Fezandipiti_BeautifulDistraction_Focus
@ Ignore path
	encaddvar 3, 1           @ Frustration + 1
	encjumpifvar ENC_CMP_NE, 3, 3, EncScript_Fezandipiti_BeautifulDistraction_Return
	encjumpifvar ENC_CMP_EQ, 0, 2, EncScript_Fezandipiti_BeautifulDistraction_Return  @ Jealous Beauty: GlamourCollapse handles it
	call EncScript_Fezandipiti_FrustrationLashOut
	return
EncScript_Fezandipiti_BeautifulDistraction_Focus:
	printstring STRINGID_ENCFEZANDIPITIDISTRACTIONFOCUS
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar ENC_CMP_LT, 1, 6, EncScript_Fezandipiti_BeautifulDistraction_FocusAdd
	return
EncScript_Fezandipiti_BeautifulDistraction_FocusAdd:
	encaddvar 1, 1           @ Glamour + 1
EncScript_Fezandipiti_BeautifulDistraction_Return:
	return

EncScript_Fezandipiti_FrustrationLashOut:
	printstring STRINGID_ENCFEZANDIPITIFRUSTRATIONLASHOUT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encsetvar 3, 0           @ Frustration reset
	encsetvar 13, 2          @ ReductionDipTimer
	call EncScript_Fezandipiti_AnnounceReduction
	return

@ POISONOUS KISS - Perfect Beauty onward. Marks EnchantTimer; TurnClose ticks its tax.
EncScript_Fezandipiti_PoisonousKissActivate::
	encsetvar 10, 1          @ PeriodicGuard - closed first
	encjumpifchance 40, EncScript_Fezandipiti_PoisonousKissActivate_Hit
	return
EncScript_Fezandipiti_PoisonousKissActivate_Hit:
	printstring STRINGID_ENCFEZANDIPITIKISSMARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetvar 5, 3           @ EnchantTimer
	return

@ JEALOUS BEAUTY: three Ignores in a row now collapse Glamour outright instead of a lash-out.
EncScript_Fezandipiti_GlamourCollapse::
	printstring STRINGID_ENCFEZANDIPITIGLAMOURCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0           @ Glamour
	encsetvar 3, 0           @ Frustration
	encsetvar 13, 2          @ ReductionDipTimer
	call EncScript_Fezandipiti_AnnounceReduction
	return

EncScript_Fezandipiti_PerfumeReset::
	encsetvar 2, 0
	return

EncScript_Fezandipiti_PerfectBeautyTransition::
	encsetvar 0, 1           @ Phase = 1
	printstring STRINGID_ENCFEZANDIPITIPERFECTBEAUTY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

EncScript_Fezandipiti_JealousBeautyTransition::
	encsetvar 0, 2           @ Phase = 2
	printstring STRINGID_ENCFEZANDIPITIJEALOUSBEAUTY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 8      @ absolute +2
	return

EncScript_Fezandipiti_BallsUnlock::
	printstring STRINGID_ENCFEZANDIPITIBALLSUNLOCK
	waitmessage B_WAIT_TIME_LONG
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	return

@ Single writer for DamageReduction: reads ReductionDipTimer to decide the target value (80 while
@ dipped, 90 otherwise) and only prints/writes when that target differs from the last announced one.
EncScript_Fezandipiti_AnnounceReduction:
	encjumpifvar ENC_CMP_GT, 13, 0, EncScript_Fezandipiti_AnnounceReduction_Dipped
	encjumpifvar ENC_CMP_EQ, 6, 90, EncScript_Fezandipiti_AnnounceReduction_Skip
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 6, 90
	printstring STRINGID_ENCFEZANDIPITIREDUCTIONCHANGE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Fezandipiti_AnnounceReduction_Dipped:
	encjumpifvar ENC_CMP_EQ, 6, 80, EncScript_Fezandipiti_AnnounceReduction_Skip
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 6, 80
	printstring STRINGID_ENCFEZANDIPITIREDUCTIONCHANGE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Fezandipiti_AnnounceReduction_Skip:
	return

@ THE LEDGER - every OnTurnEnd tick in one script to stay clear of the 8-scripts-per-checkpoint cap.
EncScript_Fezandipiti_TurnClose::
@ Spotlight countdown/expiry
	encjumpifvar ENC_CMP_EQ, 4, 0, EncScript_Fezandipiti_TurnClose_ToxicCharm
	encsubvar 4, 1
	encjumpifvar ENC_CMP_NE, 4, 0, EncScript_Fezandipiti_TurnClose_ToxicCharm
	call EncScript_Fezandipiti_SpotlightOff
EncScript_Fezandipiti_TurnClose_ToxicCharm:
@ Toxic Charm tax - Perfume maxed out
	encjumpifvar ENC_CMP_NE, 2, 3, EncScript_Fezandipiti_TurnClose_GlamourRise
	printstring STRINGID_ENCFEZANDIPITITOXICCHARMTICK
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar ENC_CMP_EQ, 0, 2, EncScript_Fezandipiti_TurnClose_ToxicCharmPhase2
	encchangehp ENC_TARGET_PLAYER_LEFT, -3, ENC_AMOUNT_PERCENT
	goto EncScript_Fezandipiti_TurnClose_GlamourRise
EncScript_Fezandipiti_TurnClose_ToxicCharmPhase2:
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
EncScript_Fezandipiti_TurnClose_GlamourRise:
@ Glamour auto-rise - Perfect Beauty only
	encjumpifvar ENC_CMP_NE, 0, 1, EncScript_Fezandipiti_TurnClose_EnchantTick
	encjumpifvar ENC_CMP_LT, 1, 6, EncScript_Fezandipiti_TurnClose_GlamourRiseAdd
	goto EncScript_Fezandipiti_TurnClose_EnchantTick
EncScript_Fezandipiti_TurnClose_GlamourRiseAdd:
	encaddvar 1, 1
EncScript_Fezandipiti_TurnClose_EnchantTick:
@ Poisonous Kiss tax
	encjumpifvar ENC_CMP_EQ, 5, 0, EncScript_Fezandipiti_TurnClose_ReductionDip
	printstring STRINGID_ENCFEZANDIPITIKISSMARK
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar ENC_CMP_EQ, 0, 2, EncScript_Fezandipiti_TurnClose_EnchantTickPhase2
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
	goto EncScript_Fezandipiti_TurnClose_EnchantTickPerfume
EncScript_Fezandipiti_TurnClose_EnchantTickPhase2:
	encchangehp ENC_TARGET_PLAYER_LEFT, -7, ENC_AMOUNT_PERCENT
EncScript_Fezandipiti_TurnClose_EnchantTickPerfume:
	encjumpifvar ENC_CMP_GE, 2, 3, EncScript_Fezandipiti_TurnClose_EnchantTickTimer
	encaddvar 2, 1           @ the mark itself keeps Perfume topped up
EncScript_Fezandipiti_TurnClose_EnchantTickTimer:
	encsubvar 5, 1           @ EnchantTimer countdown
EncScript_Fezandipiti_TurnClose_ReductionDip:
@ Damage-reduction dip countdown
	encjumpifvar ENC_CMP_EQ, 13, 0, EncScript_Fezandipiti_TurnClose_End
	encsubvar 13, 1
	encjumpifvar ENC_CMP_NE, 13, 0, EncScript_Fezandipiti_TurnClose_End
	call EncScript_Fezandipiti_AnnounceReduction
EncScript_Fezandipiti_TurnClose_End:
	encsetvar 7, 0           @ TurnGuard clear - self-disables this trigger for the rest of the turn
	return
