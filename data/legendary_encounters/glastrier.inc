@ Glastrier - The Wild Horse Pokemon. Var indices (fixed by first-appearance order in
@ glastrier.encounter's OnBattleStart block):
@ 0 Phase, 1 Frost, 2 ArmorLayers, 3 FreezeStage, 4 Charge, 5 Unstoppable, 6 FrostGainHandled,
@ 7 FrostTurnFlip, 8 ReminderFlip, 9 LanceTelegraph, 10 LastDRBand, 11 FrostTickGuard,
@ 12 AvalancheChargeUpHandled, 13 AvalancheChargeDownHandled, 14 AvalancheReleaseHandled,
@ 15 LanceBreakHitHandled.

EncScript_Glastrier_Init::
	encsetvar 0, 0    @ Phase
	encsetvar 1, 0    @ Frost
	encsetvar 2, 0    @ ArmorLayers
	encsetvar 3, 0    @ FreezeStage
	encsetvar 4, 0    @ Charge
	encsetvar 5, 0    @ Unstoppable
	encsetvar 6, 0    @ FrostGainHandled
	encsetvar 7, 0    @ FrostTurnFlip
	encsetvar 8, 0    @ ReminderFlip
	encsetvar 9, 0    @ LanceTelegraph
	encsetvar 10, 0   @ LastDRBand
	encsetvar 11, 0   @ FrostTickGuard
	encsetvar 12, 0   @ AvalancheChargeUpHandled
	encsetvar 13, 0   @ AvalancheChargeDownHandled
	encsetvar 14, 0   @ AvalancheReleaseHandled
	encsetvar 15, 0   @ LanceBreakHitHandled
	return

EncScript_Glastrier_FrostGainFromMove::
	encsetvar 6, 1    @ FrostGainHandled
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Glastrier_FrostGainFromMove_Done
	encaddvar 1, 1    @ Frost += 1
EncScript_Glastrier_FrostGainFromMove_Done:
	return

EncScript_Glastrier_FrostGainClear::
	encsetvar 6, 0    @ FrostGainHandled
	return

@ Fires exactly once per real turn (FrostTickGuard, cleared at the following OnTurnEnd - a
@ different checkpoint, so this can't chase itself within one dispatch). FrostTurnFlip then
@ halves the cadence on top of that: gain on one guarded firing, skip the next.
EncScript_Glastrier_FrostTick::
	encsetvar 11, 1   @ FrostTickGuard
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Glastrier_FrostTick_Gain
	encsetvar 7, 0    @ FrostTurnFlip: this was the skip turn
	return
EncScript_Glastrier_FrostTick_Gain:
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Glastrier_FrostTick_DoGain
	encsetvar 7, 1    @ Frost already capped; still flip so the cadence keeps alternating
	return
EncScript_Glastrier_FrostTick_DoGain:
	encaddvar 1, 1    @ Frost += 1
	encsetvar 7, 1    @ FrostTurnFlip
	return

EncScript_Glastrier_FrostTickGuardClear::
	encsetvar 11, 0   @ FrostTickGuard
	return

EncScript_Glastrier_ArmorBand2::
	encsetvar 2, 1    @ ArmorLayers = 1
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 10, 1   @ LastDRBand
	printstring STRINGID_ENCGLASTRIERARMORBAND1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Glastrier_ArmorBand4::
	encsetvar 2, 2    @ ArmorLayers = 2
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 10, 2   @ LastDRBand
	printstring STRINGID_ENCGLASTRIERARMORBAND2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Glastrier_ArmorBand6::
	encsetvar 2, 3    @ ArmorLayers = 3
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encsetvar 10, 3   @ LastDRBand
	printstring STRINGID_ENCGLASTRIERARMORBAND3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_Glastrier_FreezeStage1::
	encsetvar 3, 1    @ FreezeStage = 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -2
	printstring STRINGID_ENCGLASTRIERFREEZESTAGE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	return

EncScript_Glastrier_FreezeStage2::
	encsetvar 3, 2    @ FreezeStage = 2
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	printstring STRINGID_ENCGLASTRIERFREEZESTAGE2
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Glastrier_FreezeStage3::
	encsetvar 3, 3    @ FreezeStage = 3
	encsetstatus ENC_TARGET_PLAYER_LEFT, ENC_STATUS_FREEZE
	printstring STRINGID_ENCGLASTRIERFREEZESTAGE3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return

EncScript_Glastrier_UnstoppableGrant::
	encsetvar 5, 1    @ Unstoppable = 1
	encsetimmunity ENC_TARGET_BOSS, (ENC_IMMUNE_OHKO | ENC_IMMUNE_FIXED_DAMAGE | ENC_IMMUNE_HP_SWAP | ENC_IMMUNE_SHARED_KO | ENC_IMMUNE_FLINCH | ENC_IMMUNE_MAJOR_STATUS)
	printstring STRINGID_ENCGLASTRIERUNSTOPPABLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

EncScript_Glastrier_UnstoppableBreak::
	encsetvar 5, 0    @ Unstoppable = 0
	encsetimmunity ENC_TARGET_BOSS, (ENC_IMMUNE_OHKO | ENC_IMMUNE_FIXED_DAMAGE | ENC_IMMUNE_HP_SWAP | ENC_IMMUNE_SHARED_KO)
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Glastrier_UnstoppableBreak_Floor
	encsubvar 1, 2    @ Frost -= 2
	goto EncScript_Glastrier_UnstoppableBreak_Print
EncScript_Glastrier_UnstoppableBreak_Floor:
	encsetvar 1, 0    @ Frost floored at 0
EncScript_Glastrier_UnstoppableBreak_Print:
	printstring STRINGID_ENCGLASTRIERTHAWED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

EncScript_Glastrier_PhaseTransitionPermafrost::
	encsetvar 0, 1    @ Phase = 1 (Permafrost)
	encsetvar 1, 6    @ Frost forced to max
	encsetdamagereduction ENC_TARGET_BOSS, 96
	encsetvar 10, 3   @ LastDRBand
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, -30, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGLASTRIERPERMAFROST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

EncScript_Glastrier_PhaseTransitionFrozenKing::
	encsetvar 0, 2    @ Phase = 2 (Frozen King)
	encsetvar 9, 0    @ LanceTelegraph
	printstring STRINGID_ENCGLASTRIERFROZENKING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

@ Charge<5 survives a single +1, so without a guard this would reselect and keep incrementing
@ within the same OnTurnEnd dispatch. AvalancheChargeUpHandled is cleared at the following
@ OnTurnStart - a different checkpoint, so it can only fire once per real turn.
EncScript_Glastrier_AvalancheChargeUp::
	encsetvar 12, 1   @ AvalancheChargeUpHandled
	encaddvar 4, 1    @ Charge += 1 (Conditions already guard Charge < 5)
	return

EncScript_Glastrier_AvalancheChargeUpGuardClear::
	encsetvar 12, 0   @ AvalancheChargeUpHandled
	return

@ Event.Battler == Boss stays true for the whole OnMoveEnd dispatch, and Charge > 0 can survive a
@ single -1, so without a guard this would reselect and over-decrement for one hit. Cleared at the
@ following OnTurnEnd.
EncScript_Glastrier_AvalancheChargeDown::
	encsetvar 13, 1   @ AvalancheChargeDownHandled
	encsubvar 4, 1    @ Charge -= 1 (Conditions already guard Charge > 0)
	return

EncScript_Glastrier_AvalancheChargeDownGuardClear::
	encsetvar 13, 0   @ AvalancheChargeDownHandled
	return

@ Bonus damage is 8 + 4*Charge percent, capped at 28% (Charge maxes at 5). Charge has no read-out
@ opcode into encchangehp's amount, so the five possible values are branched explicitly.
@ AvalancheReleaseHandled guards against the same Event.Target-stays-true reselection risk as
@ AvalancheChargeDown above; cleared at the following OnTurnEnd.
EncScript_Glastrier_AvalancheRelease::
	encsetvar 14, 1   @ AvalancheReleaseHandled
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Glastrier_AvalancheRelease_C0
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Glastrier_AvalancheRelease_C1
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Glastrier_AvalancheRelease_C2
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Glastrier_AvalancheRelease_C3
	encjumpifvar CMP_EQUAL, 4, 4, EncScript_Glastrier_AvalancheRelease_C4
	goto EncScript_Glastrier_AvalancheRelease_C5
EncScript_Glastrier_AvalancheRelease_C0:
	encchangehp ENC_TARGET_PLAYER_LEFT, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Glastrier_AvalancheRelease_Print
EncScript_Glastrier_AvalancheRelease_C1:
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	goto EncScript_Glastrier_AvalancheRelease_Print
EncScript_Glastrier_AvalancheRelease_C2:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Glastrier_AvalancheRelease_Print
EncScript_Glastrier_AvalancheRelease_C3:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Glastrier_AvalancheRelease_Print
EncScript_Glastrier_AvalancheRelease_C4:
	encchangehp ENC_TARGET_PLAYER_LEFT, -24, ENC_AMOUNT_PERCENT
	goto EncScript_Glastrier_AvalancheRelease_Print
EncScript_Glastrier_AvalancheRelease_C5:
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
EncScript_Glastrier_AvalancheRelease_Print:
	encsetvar 4, 0    @ Charge = 0
	printstring STRINGID_ENCGLASTRIERAVALANCHESLAM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return

EncScript_Glastrier_AvalancheReleaseGuardClear::
	encsetvar 14, 0   @ AvalancheReleaseHandled
	return

EncScript_Glastrier_LanceTelegraphStart::
	encsetvar 9, 1    @ LanceTelegraph = 1
	encsetvar 4, 0    @ Charge = 0
	printstring STRINGID_ENCGLASTRIERLANCETELEGRAPH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

@ Event.Battler == Boss stays true for the whole dispatch, so this needs the same guard/clear
@ shape as the Avalanche triggers above. Cleared at the following OnTurnEnd.
EncScript_Glastrier_LanceBreakHit::
	encsetvar 15, 1   @ LanceBreakHitHandled
	encaddvar 4, 1    @ Charge += 1 (Conditions already guard Charge < 2)
	return

EncScript_Glastrier_LanceBreakHitGuardClear::
	encsetvar 15, 0   @ LanceBreakHitHandled
	return

EncScript_Glastrier_LanceResolve::
	encjumpifvar CMP_LESS_THAN, 4, 2, EncScript_Glastrier_LanceResolve_Hits
	printstring STRINGID_ENCGLASTRIERLANCEINTERRUPTED
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Glastrier_LanceResolve_End
EncScript_Glastrier_LanceResolve_Hits:
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGLASTRIERLANCEHITS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
EncScript_Glastrier_LanceResolve_End:
	encsetvar 9, 0    @ LanceTelegraph = 0
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Glastrier_LanceResolve_Return
	encsubvar 1, 1    @ Frost -= 1 (the ice begins melting)
EncScript_Glastrier_LanceResolve_Return:
	return

EncScript_Glastrier_CatchWindowOpen::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 45
	printstring STRINGID_ENCGLASTRIERCATCHWINDOW
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Glastrier_ReminderLine::
	encsetvar 8, 1    @ ReminderFlip = 1
	printstring STRINGID_ENCGLASTRIERREMINDER
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Glastrier_ReminderFlipClear::
	encsetvar 8, 0    @ ReminderFlip = 0
	return
