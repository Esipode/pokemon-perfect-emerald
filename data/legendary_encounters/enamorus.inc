@ Enamorus ("The Love-Hate Pokemon")
@ Var indices, fixed by first appearance in enamorus.encounter's always-true block:
@ 0 Phase, 1 AffectLeft, 2 AffectRight, 3 HeartbreakTimerLeft, 4 HeartbreakTimerRight,
@ 5 BondActive, 6 LastAnnouncedReduction, 7 TurnGuard, 8 HeartstormTurnGuard, 9 CharmGuard,
@ 10 BondGuard, 11 SwitchGuard.
@ AffectLeft/Right domain: 0 Neutral, 1 Fond, 2 Enamored, 3 Obsessed, 4 Heartbreak.

EncScript_Enamorus_Intro::
	encsetvar 6, 88     @ LastAnnouncedReduction = the Properties baseline
	printstring STRINGID_ENCENAMORUSAWAKENS
	waitmessage B_WAIT_TIME_LONG
	return

@ Single writer of the boss's live damage reduction: absolute, so a soften then a restore can never
@ leave a stray delta behind. Re-prints only when the value actually changes (guideline 14).
EncSub_Enamorus_ApplyReduction:
	encjumpifvar CMP_EQUAL, 1, 4, .ApplyRed_Soft
	encjumpifvar CMP_EQUAL, 2, 4, .ApplyRed_Soft
	encjumpifvar CMP_EQUAL, 6, 88, .ApplyRed_Done
	encsetvar 6, 88
	encsetdamagereduction ENC_TARGET_BOSS, 92
	printstring STRINGID_ENCENAMORUSGUARDHOLDS
	waitmessage B_WAIT_TIME_LONG
	goto .ApplyRed_Done
.ApplyRed_Soft:
	encjumpifvar CMP_EQUAL, 6, 73, .ApplyRed_Done
	encsetvar 6, 73
	encsetdamagereduction ENC_TARGET_BOSS, 86
	printstring STRINGID_ENCENAMORUSGUARDSOFTENS
	waitmessage B_WAIT_TIME_LONG
.ApplyRed_Done:
	return

@ CHARM (OnMoveEnd, Enamorus's move connects on the player-left slot). Advances AffectLeft one step,
@ granting the DEF/SPDEF bonus on crossing into Enamored/Obsessed. Obsessed is capped off before
@ Phase 1. Already-Obsessed and already-Heartbroken slots take a small chip instead. Closes
@ CharmGuard on the first line so every path disables the trigger.
EncScript_Enamorus_Charm_Left::
	encsetvar 9, 1
	encjumpifvar CMP_EQUAL, 1, 0, .Charm_L_FromNeutral
	encjumpifvar CMP_EQUAL, 1, 1, .Charm_L_FromFond
	encjumpifvar CMP_EQUAL, 1, 2, .Charm_L_FromEnamored
	goto .Charm_L_NoAdvance
.Charm_L_FromNeutral:
	encsetvar 1, 1
	goto .Charm_L_Announce
.Charm_L_FromFond:
	encsetvar 1, 2
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 1
	goto .Charm_L_Announce
.Charm_L_FromEnamored:
	encjumpifvar CMP_LESS_THAN, 0, 1, .Charm_L_NoAdvance
	encsetvar 1, 3
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 1
	goto .Charm_L_Announce
.Charm_L_NoAdvance:
	encchangehp ENC_TARGET_PLAYER_LEFT, -4, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	waitanimation
	goto .Charm_L_Hesitate
.Charm_L_Announce:
	printstring STRINGID_ENCENAMORUSCHARMSETTLES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	waitanimation
.Charm_L_Hesitate:
	encjumpifvar CMP_LESS_THAN, 1, 2, .Charm_L_Done
	encjumpifchance 30, .Charm_L_DoHesitate
	goto .Charm_L_Done
.Charm_L_DoHesitate:
	encdisablemove ENC_TARGET_PLAYER_LEFT, 2, .Charm_L_Done
	printstring STRINGID_ENCENAMORUSHESITATES
	waitmessage B_WAIT_TIME_LONG
.Charm_L_Done:
	return

EncScript_Enamorus_Charm_Right::
	encsetvar 9, 1
	encjumpifvar CMP_EQUAL, 2, 0, .Charm_R_FromNeutral
	encjumpifvar CMP_EQUAL, 2, 1, .Charm_R_FromFond
	encjumpifvar CMP_EQUAL, 2, 2, .Charm_R_FromEnamored
	goto .Charm_R_NoAdvance
.Charm_R_FromNeutral:
	encsetvar 2, 1
	goto .Charm_R_Announce
.Charm_R_FromFond:
	encsetvar 2, 2
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, 1
	goto .Charm_R_Announce
.Charm_R_FromEnamored:
	encjumpifvar CMP_LESS_THAN, 0, 1, .Charm_R_NoAdvance
	encsetvar 2, 3
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, 1
	goto .Charm_R_Announce
.Charm_R_NoAdvance:
	encchangehp ENC_TARGET_PLAYER_RIGHT, -4, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	waitanimation
	goto .Charm_R_Hesitate
.Charm_R_Announce:
	printstring STRINGID_ENCENAMORUSCHARMSETTLES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	waitanimation
.Charm_R_Hesitate:
	encjumpifvar CMP_LESS_THAN, 2, 2, .Charm_R_Done
	encjumpifchance 30, .Charm_R_DoHesitate
	goto .Charm_R_Done
.Charm_R_DoHesitate:
	encdisablemove ENC_TARGET_PLAYER_RIGHT, 2, .Charm_R_Done
	printstring STRINGID_ENCENAMORUSHESITATES
	waitmessage B_WAIT_TIME_LONG
.Charm_R_Done:
	return

@ HEARTBREAK (OnMoveEnd, the player's move connects on Enamorus while that attacker's slot is
@ Enamored+). Strips the DEF/SPDEF bonus that slot was granted, flips it to Heartbreak, softens
@ Enamorus's guard, and chips Enamorus for a flat 8% max HP.
EncScript_Enamorus_Heartbreak_Left::
	encjumpifvar CMP_EQUAL, 1, 3, .HB_L_FromObsessed
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -1
	goto .HB_L_Flip
.HB_L_FromObsessed:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -2
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -2
.HB_L_Flip:
	encsetvar 1, 4
	encsetvar 3, 3
	call EncSub_Enamorus_ApplyReduction
	encchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	waitanimation
	printstring STRINGID_ENCENAMORUSHEARTBREAK
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Enamorus_Heartbreak_Right::
	encjumpifvar CMP_EQUAL, 2, 3, .HB_R_FromObsessed
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, -1
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, -1
	goto .HB_R_Flip
.HB_R_FromObsessed:
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, -2
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, -2
.HB_R_Flip:
	encsetvar 2, 4
	encsetvar 4, 3
	call EncSub_Enamorus_ApplyReduction
	encchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	waitanimation
	printstring STRINGID_ENCENAMORUSHEARTBREAK
	waitmessage B_WAIT_TIME_LONG
	return

@ Heartbreak timers tick at OnTurnStart (EncScript_Enamorus_TurnOpen) and resolve here at OnTurnEnd,
@ on a different checkpoint so the timer can't chase itself to zero within one dispatch.
EncScript_Enamorus_HeartbreakFades_Left::
	encsetvar 1, 0
	call EncSub_Enamorus_ApplyReduction
	printstring STRINGID_ENCENAMORUSHEARTBREAKFADES
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Enamorus_HeartbreakFades_Right::
	encsetvar 2, 0
	call EncSub_Enamorus_ApplyReduction
	printstring STRINGID_ENCENAMORUSHEARTBREAKFADES
	waitmessage B_WAIT_TIME_LONG
	return

@ LOVERS' BOND (OnMoveEnd, Enamorus's move connects, Phase 1+). Mirrors 6% onto a random bench
@ member of whichever slot got hit. Closes BondGuard on the first line.
EncScript_Enamorus_LoversBond_Left::
	encsetvar 10, 1
	encbenchhp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT, .Bond_L_Done
	printstring STRINGID_ENCENAMORUSBONDMIRROR
	waitmessage B_WAIT_TIME_LONG
.Bond_L_Done:
	return

EncScript_Enamorus_LoversBond_Right::
	encsetvar 10, 1
	encbenchhp ENC_TARGET_PLAYER_RIGHT, -6, ENC_AMOUNT_PERCENT, .Bond_R_Done
	printstring STRINGID_ENCENAMORUSBONDMIRROR
	waitmessage B_WAIT_TIME_LONG
.Bond_R_Done:
	return

@ JEALOUSY (OnSwitchIn). Reads the vacated slot's leftover Affection before overwriting it; Fond+
@ (including a broken Heartbreak) hands the incoming mon an instant Fond. Closes SwitchGuard first.
EncScript_Enamorus_Jealousy_Left::
	encsetvar 11, 1
	encsetvar 3, 0
	encjumpifvar CMP_EQUAL, 1, 0, .Jeal_L_Neutral
	encsetvar 1, 1
	printstring STRINGID_ENCENAMORUSJEALOUSY
	waitmessage B_WAIT_TIME_LONG
	goto .Jeal_L_Reduction
.Jeal_L_Neutral:
	encsetvar 1, 0
.Jeal_L_Reduction:
	call EncSub_Enamorus_ApplyReduction
	return

EncScript_Enamorus_Jealousy_Right::
	encsetvar 11, 1
	encsetvar 4, 0
	encjumpifvar CMP_EQUAL, 2, 0, .Jeal_R_Neutral
	encsetvar 2, 1
	printstring STRINGID_ENCENAMORUSJEALOUSY
	waitmessage B_WAIT_TIME_LONG
	goto .Jeal_R_Reduction
.Jeal_R_Neutral:
	encsetvar 2, 0
.Jeal_R_Reduction:
	call EncSub_Enamorus_ApplyReduction
	return

@ Once-per-turn scaffold: ticks both Heartbreak timers down by one, floored at 0.
EncScript_Enamorus_TurnOpen::
	encsetvar 7, 1
	encjumpifvar CMP_EQUAL, 3, 0, .TurnOpen_SkipL
	encsubvar 3, 1
.TurnOpen_SkipL:
	encjumpifvar CMP_EQUAL, 4, 0, .TurnOpen_SkipR
	encsubvar 4, 1
.TurnOpen_SkipR:
	return

@ Clears every per-event guard so next turn's Charm/Bond/Jealousy can fire again. Self-disables by
@ clearing TurnGuard.
EncScript_Enamorus_TurnClose::
	encsetvar 7, 0
	encsetvar 9, 0
	encsetvar 10, 0
	encsetvar 11, 0
	return

@ Phase 1: Wrath of Love / Therian Forme. Obsessed becomes reachable and Lovers' Bond turns on.
EncScript_Enamorus_Phase2_Therian::
	encsetvar 0, 1
	encsetvar 5, 1
	printstring STRINGID_ENCENAMORUSTHERIANSHIFT
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_ENAMORUS_THERIAN, .Phase2_NoForm, B_ANIM_FORM_CHANGE
.Phase2_NoForm:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_PLAY_ROUGH
	printstring STRINGID_ENCENAMORUSBONDFORMS
	waitmessage B_WAIT_TIME_LONG
	return

@ Phase 2: Heartstorm. Forced per-turn cycling takes over from here.
EncScript_Enamorus_Phase3_Heartstorm::
	encsetvar 0, 2
	printstring STRINGID_ENCENAMORUSHEARTSTORM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	return

@ HEARTSTORM's forced cycle: one active slot's Affection is nudged, or broken straight to
@ Heartbreak, by chance every turn - independent of whether Enamorus's own move connected.
@ OnTurnStart animates, so this leads with flushtextbox.
EncScript_Enamorus_HeartstormPulse::
	encsetvar 8, 1
	flushtextbox
	encjumpifchance 50, .Pulse_Right
	encjumpifvar CMP_EQUAL, 1, 4, .Pulse_Done
	encjumpifchance 40, .Pulse_L_ToHeartbreak
	encjumpifvar CMP_EQUAL, 1, 1, .Pulse_L_F2E
	encjumpifvar CMP_EQUAL, 1, 2, .Pulse_L_E2O
	encjumpifvar CMP_GREATER_THAN, 1, 2, .Pulse_Done
	encsetvar 1, 1
	goto .Pulse_L_NudgeAnnounce
.Pulse_L_F2E:
	encsetvar 1, 2
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 1
	goto .Pulse_L_NudgeAnnounce
.Pulse_L_E2O:
	encsetvar 1, 3
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 1
.Pulse_L_NudgeAnnounce:
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	waitanimation
	printstring STRINGID_ENCENAMORUSCHARMSETTLES
	waitmessage B_WAIT_TIME_LONG
	goto .Pulse_Done
.Pulse_L_ToHeartbreak:
	encjumpifvar CMP_LESS_THAN, 1, 2, .Pulse_Done
	encjumpifvar CMP_EQUAL, 1, 3, .Pulse_L_ToHB_FromObsessed
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -1
	goto .Pulse_L_ToHB_Flip
.Pulse_L_ToHB_FromObsessed:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -2
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -2
.Pulse_L_ToHB_Flip:
	encsetvar 1, 4
	encsetvar 3, 3
	call EncSub_Enamorus_ApplyReduction
	printstring STRINGID_ENCENAMORUSHEARTBREAK
	waitmessage B_WAIT_TIME_LONG
	goto .Pulse_Done
.Pulse_Right:
	encjumpifvar CMP_EQUAL, 2, 4, .Pulse_Done
	encjumpifchance 40, .Pulse_R_ToHeartbreak
	encjumpifvar CMP_EQUAL, 2, 1, .Pulse_R_F2E
	encjumpifvar CMP_EQUAL, 2, 2, .Pulse_R_E2O
	encjumpifvar CMP_GREATER_THAN, 2, 2, .Pulse_Done
	encsetvar 2, 1
	goto .Pulse_R_NudgeAnnounce
.Pulse_R_F2E:
	encsetvar 2, 2
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, 1
	goto .Pulse_R_NudgeAnnounce
.Pulse_R_E2O:
	encsetvar 2, 3
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, 1
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, 1
.Pulse_R_NudgeAnnounce:
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	waitanimation
	printstring STRINGID_ENCENAMORUSCHARMSETTLES
	waitmessage B_WAIT_TIME_LONG
	goto .Pulse_Done
.Pulse_R_ToHeartbreak:
	encjumpifvar CMP_LESS_THAN, 2, 2, .Pulse_Done
	encjumpifvar CMP_EQUAL, 2, 3, .Pulse_R_ToHB_FromObsessed
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, -1
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, -1
	goto .Pulse_R_ToHB_Flip
.Pulse_R_ToHB_FromObsessed:
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_DEF, -2
	encchangestat ENC_TARGET_PLAYER_RIGHT, STAT_SPDEF, -2
.Pulse_R_ToHB_Flip:
	encsetvar 2, 4
	encsetvar 4, 3
	call EncSub_Enamorus_ApplyReduction
	printstring STRINGID_ENCENAMORUSHEARTBREAK
	waitmessage B_WAIT_TIME_LONG
.Pulse_Done:
	return

EncScript_Enamorus_HeartstormPulseClear::
	encsetvar 8, 0
	return

@ Phase 3: Spring's Embrace, the catch window. Damage scales with how compromised the team
@ currently is (10% base, +6 per Enamored slot, +12 per Obsessed slot, 30% cap), then every
@ emotional state clears and the ball unlocks.
EncScript_Enamorus_SpringsEmbrace::
	encsetvar 0, 3
	printstring STRINGID_ENCENAMORUSSPRINGSEMBRACE
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 1, 3, .Embrace_LObsessed
	encjumpifvar CMP_EQUAL, 1, 2, .Embrace_LEnamored
	goto .Embrace_LNone
.Embrace_LObsessed:
	encjumpifvar CMP_EQUAL, 2, 3, .Embrace_Damage30
	encjumpifvar CMP_EQUAL, 2, 2, .Embrace_Damage28
	goto .Embrace_Damage22
.Embrace_LEnamored:
	encjumpifvar CMP_EQUAL, 2, 3, .Embrace_Damage28
	encjumpifvar CMP_EQUAL, 2, 2, .Embrace_Damage22
	goto .Embrace_Damage16
.Embrace_LNone:
	encjumpifvar CMP_EQUAL, 2, 3, .Embrace_Damage22
	encjumpifvar CMP_EQUAL, 2, 2, .Embrace_Damage16
	goto .Embrace_Damage10
.Embrace_Damage30:
	encchangehp ENC_TARGET_ALL_ALLIES, -30, ENC_AMOUNT_PERCENT
	goto .Embrace_AfterDamage
.Embrace_Damage28:
	encchangehp ENC_TARGET_ALL_ALLIES, -28, ENC_AMOUNT_PERCENT
	goto .Embrace_AfterDamage
.Embrace_Damage22:
	encchangehp ENC_TARGET_ALL_ALLIES, -22, ENC_AMOUNT_PERCENT
	goto .Embrace_AfterDamage
.Embrace_Damage16:
	encchangehp ENC_TARGET_ALL_ALLIES, -16, ENC_AMOUNT_PERCENT
	goto .Embrace_AfterDamage
.Embrace_Damage10:
	encchangehp ENC_TARGET_ALL_ALLIES, -10, ENC_AMOUNT_PERCENT
.Embrace_AfterDamage:
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	encsetvar 1, 0
	encsetvar 2, 0
	encsetvar 3, 0
	encsetvar 4, 0
	encsetvar 5, 0
	encresetstats ENC_TARGET_ALL_BATTLERS, .Embrace_NoStats
.Embrace_NoStats:
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetvar 6, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCENAMORUSCATCHWINDOW
	waitmessage B_WAIT_TIME_LONG
	return
