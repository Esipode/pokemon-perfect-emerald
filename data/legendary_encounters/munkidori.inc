@ Munkidori, "The Manipulative Trickster Pokemon". Author variable map (fixed by first-appearance
@ order in munkidori.encounter's OnBattleStart block - referenced here as bare integers):
@ 0 Phase (0 opening / 1 Psychic Control / 2 Mind Break / 3 Spent)
@ 1 Influence (the meter, 0-6)
@ 2 PredictedCat (this turn's encstoreprediction result: 0 none / 1 phys / 2 spec / 3 status)
@ 3 PredictHandled (once-per-turn guard for the prediction hit/miss triggers)
@ 4 TTTimer (Toxic Thought's countdown: 0 inactive, 4-1 counting down)
@ 5 TTStatApplied (guards the turn-1 stat debuff from double-applying or reversing twice)
@ 6 Bluff (0 real tell / 1 fake tell, rolled each turn)
@ 7 BluffHandled (once-per-turn guard for the tell resolution)
@ 8 HijackCat (the category Munkidori calls out: 0 none / 1 phys / 2 spec / 3 status)
@ 9 HijackHandled (once-per-turn guard for the reflect-punish)
@ 10 TurnGuard (the once-per-turn scaffold)
@ 11 Overconfident (0/1, whether its guard is currently down)
@ 12 FinalePlayed (guards the Nasty Plot burst to fire once)

EncScript_Munkidori_Intro::
	printstring STRINGID_ENCMUNKIDORIINTRO
	waitmessage B_WAIT_TIME_LONG
	return

@ The once-per-turn scaffold: rolls this turn's prediction, bluffed tell, and (Phase >= 1) Command
@ Hijack warning, then latches TurnGuard so nothing else at this checkpoint re-runs it.
EncScript_Munkidori_TurnOpen::
	encsetvar 10, 1                @ TurnGuard = 1
	flushtextbox
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 2
	encsetvar 3, 0                 @ PredictHandled = 0
	encjumpifchance 40, EncScript_Munkidori_TurnOpen_BluffFake
	encsetvar 6, 0                 @ Bluff = 0 (real)
	printstring STRINGID_ENCMUNKIDORIBLUFFREAL
	goto EncScript_Munkidori_TurnOpen_BluffDone
EncScript_Munkidori_TurnOpen_BluffFake:
	encsetvar 6, 1
	printstring STRINGID_ENCMUNKIDORIBLUFFFAKE
EncScript_Munkidori_TurnOpen_BluffDone:
	waitmessage B_WAIT_TIME_LONG
	encsetvar 7, 0                 @ BluffHandled = 0
	encjumpifvar ENC_CMP_LT, 0, 1, EncScript_Munkidori_TurnOpen_NoHijack
	encjumpifchance 33, EncScript_Munkidori_TurnOpen_HijackPhys
	encjumpifchance 50, EncScript_Munkidori_TurnOpen_HijackSpec
	encsetvar 8, 3                 @ HijackCat = status
	printstring STRINGID_ENCMUNKIDORIHIJACKWARN
	goto EncScript_Munkidori_TurnOpen_HijackDone
EncScript_Munkidori_TurnOpen_HijackPhys:
	encsetvar 8, 1
	printstring STRINGID_ENCMUNKIDORIHIJACKWARN
	goto EncScript_Munkidori_TurnOpen_HijackDone
EncScript_Munkidori_TurnOpen_HijackSpec:
	encsetvar 8, 2
	printstring STRINGID_ENCMUNKIDORIHIJACKWARN
EncScript_Munkidori_TurnOpen_HijackDone:
	waitmessage B_WAIT_TIME_LONG
	encsetvar 9, 0                 @ HijackHandled = 0
	goto EncScript_Munkidori_TurnOpen_Cue
EncScript_Munkidori_TurnOpen_NoHijack:
	encsetvar 8, 0                 @ HijackCat = none
EncScript_Munkidori_TurnOpen_Cue:
	encjumpifvar ENC_CMP_LT, 1, 4, EncScript_Munkidori_TurnOpen_End
	printstring STRINGID_ENCMUNKIDORIINFLUENCERISING
	waitmessage B_WAIT_TIME_LONG
EncScript_Munkidori_TurnOpen_End:
	return

@ One writer per number (guideline: single call-able subroutine), clamped 0-6 either direction.
EncScript_Munkidori_IncInfluence:
	encjumpifvar ENC_CMP_GE, 1, 6, EncScript_Munkidori_IncInfluence_Skip
	encaddvar 1, 1
EncScript_Munkidori_IncInfluence_Skip:
	return

EncScript_Munkidori_DecInfluence:
	encjumpifvar ENC_CMP_LE, 1, 0, EncScript_Munkidori_DecInfluence_Skip
	encsubvar 1, 1
EncScript_Munkidori_DecInfluence_Skip:
	return

@ Mind Games: the AI's own predicted category landed. Same script for all three category triggers.
EncScript_Munkidori_PredictionHit::
	encsetvar 3, 1                 @ PredictHandled = 1
	call EncScript_Munkidori_IncInfluence
	printstring STRINGID_ENCMUNKIDORIPREDICTIONHIT
	waitmessage B_WAIT_TIME_LONG
	return

@ Toxic Thought lands - the timer itself only ticks in the ledger (OnTurnEnd), never here, so apply
@ and tick stay on separate checkpoints per the countdown-split rule.
EncScript_Munkidori_ToxicThoughtApply::
	encsetvar 4, 4                 @ TTTimer = 4
	encsetvar 5, 0                 @ TTStatApplied = 0
	printstring STRINGID_ENCMUNKIDORITOXICTHOUGHTAPPLIED
	waitmessage B_WAIT_TIME_LONG
	return

@ Command Hijack punish: used the flagged category anyway. Reflected back as a small chip, not a
@ true override (guideline: no pre-execution move-interception hook exists to actually reflect it).
EncScript_Munkidori_HijackPunish::
	encsetvar 9, 1                 @ HijackHandled = 1
	call EncScript_Munkidori_IncInfluence
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	printstring STRINGID_ENCMUNKIDORIHIJACKPUNISH
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Munkidori_Phase2Entry::
	encsetvar 0, 1                 @ Phase = 1 (Psychic Control)
	printstring STRINGID_ENCMUNKIDORIPSYCHICCONTROL
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Munkidori_FinalPhaseEntry::
	encsetvar 0, 2                 @ Phase = 2 (Mind Break)
	printstring STRINGID_ENCMUNKIDORIMINDBREAK
	waitmessage B_WAIT_TIME_LONG
	return

@ Single writer for the Overconfidence stat swing, reading Overconfident and landing DEF/SPDEF
@ absolutely - clamp to the rail first, then apply the delta that lands on the target stage, so a
@ Haze or a switch landing between enter/exit can't leave this out of sync.
EncScript_Munkidori_ApplyOverconfidence:
	encjumpifvar ENC_CMP_EQ, 11, 1, EncScript_Munkidori_ApplyOverconfidence_Down
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -6
	return
EncScript_Munkidori_ApplyOverconfidence_Down:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 4
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 4
	return

EncScript_Munkidori_OverconfidentEnter::
	encsetvar 11, 1
	call EncScript_Munkidori_ApplyOverconfidence
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCMUNKIDORIOVERCONFIDENTENTER
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Munkidori_OverconfidentExit::
	encsetvar 11, 0
	call EncScript_Munkidori_ApplyOverconfidence
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCMUNKIDORIOVERCONFIDENTEXIT
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Munkidori_BluffFellFor::
	encsetvar 7, 1                 @ BluffHandled = 1
	call EncScript_Munkidori_IncInfluence
	printstring STRINGID_ENCMUNKIDORIBLUFFFELLFOR
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Munkidori_BluffReadPast::
	encsetvar 7, 1                 @ BluffHandled = 1
	call EncScript_Munkidori_DecInfluence
	printstring STRINGID_ENCMUNKIDORIBLUFFREADPAST
	waitmessage B_WAIT_TIME_LONG
	return

@ The ledger. Clears TurnGuard as its first line, closing that latch immediately so this trigger
@ can't re-select within the same checkpoint's re-evaluation passes, then resolves the
@ prediction-miss crash and the Toxic Thought countdown - both pure-var checks.
EncScript_Munkidori_TurnClose::
	encsetvar 10, 0                 @ TurnGuard = 0
	encjumpifvar ENC_CMP_EQ, 2, 0, EncScript_Munkidori_TurnClose_TTCheck
	encjumpifvar ENC_CMP_NE, 3, 0, EncScript_Munkidori_TurnClose_TTCheck
	call EncScript_Munkidori_DecInfluence
	printstring STRINGID_ENCMUNKIDORIPREDICTIONMISS
	waitmessage B_WAIT_TIME_LONG
EncScript_Munkidori_TurnClose_TTCheck:
	encjumpifvar ENC_CMP_EQ, 4, 0, EncScript_Munkidori_TurnClose_End
	encsubvar 4, 1                  @ TTTimer -= 1
	encjumpifvar ENC_CMP_EQ, 4, 3, EncScript_Munkidori_TurnClose_TT1
	encjumpifvar ENC_CMP_EQ, 4, 0, EncScript_Munkidori_TurnClose_TT4
	printstring STRINGID_ENCMUNKIDORITOXICTHOUGHTTICK
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Munkidori_TurnClose_End
EncScript_Munkidori_TurnClose_TT1:
	encsetvar 5, 1                   @ TTStatApplied = 1
	encjumpifvar ENC_CMP_EQ, 0, 2, EncScript_Munkidori_TurnClose_TT1Hard
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -1
	goto EncScript_Munkidori_TurnClose_TT1Done
EncScript_Munkidori_TurnClose_TT1Hard:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -2
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -2
EncScript_Munkidori_TurnClose_TT1Done:
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCMUNKIDORITOXICTHOUGHTTURN1
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Munkidori_TurnClose_End
EncScript_Munkidori_TurnClose_TT4:
	encjumpifvar ENC_CMP_EQ, 5, 0, EncScript_Munkidori_TurnClose_TT4Reward
	encjumpifvar ENC_CMP_EQ, 0, 2, EncScript_Munkidori_TurnClose_TT4Hard
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, 1
	goto EncScript_Munkidori_TurnClose_TT4Reward
EncScript_Munkidori_TurnClose_TT4Hard:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, 2
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, 2
EncScript_Munkidori_TurnClose_TT4Reward:
	encsetvar 5, 0                   @ TTStatApplied = 0
	call EncScript_Munkidori_IncInfluence
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCMUNKIDORITOXICTHOUGHTTURN4
	waitmessage B_WAIT_TIME_LONG
EncScript_Munkidori_TurnClose_End:
	return

@ The Nasty Plot burst: Influence maxed a second time, this time in Mind Break. Gathers whatever
@ Influence remains into one fixed 30% hit (guideline 7's rare-explosive ceiling - Influence is
@ always 6 at this trigger, so there is no lower band to scale from), then tears down the fight.
EncScript_Munkidori_Finale::
	encsetvar 12, 1                  @ FinalePlayed = 1
	printstring STRINGID_ENCMUNKIDORIFINALEWINDUP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	encsetvar 1, 0                    @ Influence = 0
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Munkidori_Finale_NoStats
EncScript_Munkidori_Finale_NoStats:
	encsetvar 11, 0                   @ Overconfident = 0
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	printstring STRINGID_ENCMUNKIDORINOWCATCH
	waitmessage B_WAIT_TIME_LONG
	return
