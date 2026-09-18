@ Ogerpon, "The Masked Guardian". See src/data/legendary_encounters/ogerpon.encounter for the
@ full design comment and the variable index -> name map (MaskEnergy=0, Pillars=1,
@ PhysicalPressure=2, UsedWellspring=3, UsedHearthflame=4, UsedCornerstone=5, UsedCount=6,
@ BreakSnapshot=7, Scratch=8, BreakWindow=9, Phase=10, CycleTimer=11, CycleSpeed=12,
@ MoveHandled=13, TurnGuard=14, ActiveMask=15).

EncScript_Ogerpon_Intro::
	return

@ ---------------------------------------------------------------------------------------------
@ Shared subroutines (call/return only - never registered as a Script:, so single ':' labels).
@ ---------------------------------------------------------------------------------------------

@ The one writer for Ogerpon's damage reduction: 94 while a Cornerstone shield charge is up,
@ 90 otherwise. Called after anything that changes ActiveMask or Pillars.
EncScript_Ogerpon_ApplyReduction:
	encjumpifvar CMP_NOT_EQUAL, 15, 3, ogerpon_reduction_base   @ ActiveMask != Cornerstone
	encjumpifvar CMP_EQUAL, 1, 0, ogerpon_reduction_base        @ Pillars == 0
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
ogerpon_reduction_base:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return

@ +1 Mask Energy on a super-effective hit taken, regardless of mask. gMoveResultFlags is read via
@ the ordinary jumpifmoveresultflags opcode against gBattlerTarget - valid here because OnMoveEnd
@ is move-scoped and the boss is the target of the hit that opened this dispatch.
EncScript_Ogerpon_SEBonus:
	jumpifmoveresultflags MOVE_RESULT_SUPER_EFFECTIVE, ogerpon_se_apply
	return
ogerpon_se_apply:
	encaddvar 0, 1   @ MaskEnergy
	return

@ Cornerstone's shield: spends one Pillar charge per hit landed while Cornerstone is worn; the
@ charge that empties it fires a shockwave back at the player and drops the reduction band.
EncScript_Ogerpon_PillarTick:
	encjumpifvar CMP_NOT_EQUAL, 15, 3, ogerpon_pillar_done   @ ActiveMask != Cornerstone
	encjumpifvar CMP_EQUAL, 1, 0, ogerpon_pillar_done        @ already broken
	encsubvar 1, 1                                            @ Pillars -= 1
	encjumpifvar CMP_NOT_EQUAL, 1, 0, ogerpon_pillar_apply
	printstring STRINGID_ENCOGERPONPILLARSSHATTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
ogerpon_pillar_apply:
	call EncScript_Ogerpon_ApplyReduction
ogerpon_pillar_done:
	return

@ The break window: within BreakWindow turns of a mask going up, ENC_SNAP_DAMAGE-worth of 5% of
@ the boss's max HP cracks it back to Teal. Scratch is a disposable copy of BreakSnapshot because
@ ENC_SNAP_DAMAGE overwrites the variable it's given.
EncScript_Ogerpon_BreakCheck:
	encjumpifvar CMP_EQUAL, 9, 0, ogerpon_break_done    @ BreakWindow == 0
	encjumpifvar CMP_EQUAL, 15, 0, ogerpon_break_done   @ ActiveMask == Teal, nothing to crack
	enccopyvar 8, 7                                      @ Scratch = BreakSnapshot
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_DAMAGE, ogerpon_break_done
	encjumpifvar CMP_LESS_THAN, 8, 5, ogerpon_break_done
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_TEAL, ogerpon_break_applied, B_ANIM_FORM_CHANGE
ogerpon_break_applied:
	encsetvar 15, 0   @ ActiveMask = Teal
	encsetvar 0, 0    @ MaskEnergy = 0
	encsetvar 9, 0    @ BreakWindow = 0
	encsetvar 1, 0    @ Pillars = 0
	call EncScript_Ogerpon_ApplyReduction
	printstring STRINGID_ENCOGERPONMASKCRACKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encjumpifvar CMP_NOT_EQUAL, 10, 1, ogerpon_break_done   @ Phase != Guardian's Resolve
	encsetvar 11, 0                                          @ CycleTimer = 0, skip to next step
ogerpon_break_done:
	return

@ The per-mask signature payload, Phase 0 only. Fires once Mask Energy reaches 3 while masked.
EncScript_Ogerpon_PayloadCheck:
	encjumpifvar CMP_EQUAL, 15, 0, ogerpon_payload_done   @ ActiveMask == Teal
	encjumpifvar CMP_LESS_THAN, 0, 3, ogerpon_payload_done   @ MaskEnergy < 3
	encjumpifvar CMP_EQUAL, 15, 1, ogerpon_payload_wellspring
	encjumpifvar CMP_EQUAL, 15, 2, ogerpon_payload_hearthflame
	goto ogerpon_payload_cornerstone

ogerpon_payload_wellspring:
	printstring STRINGID_ENCOGERPONWELLSPRINGSURGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	encsetvar 0, 0   @ MaskEnergy = 0
	encjumpifvar CMP_NOT_EQUAL, 3, 0, ogerpon_payload_done   @ UsedWellspring already set
	encsetvar 3, 1
	encaddvar 6, 1   @ UsedCount += 1
	goto ogerpon_payload_done

ogerpon_payload_hearthflame:
	printstring STRINGID_ENCOGERPONFLAMESOUT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encsetvar 0, 0   @ MaskEnergy = 0
	encjumpifvar CMP_NOT_EQUAL, 4, 0, ogerpon_payload_done   @ UsedHearthflame already set
	encsetvar 4, 1
	encaddvar 6, 1
	goto ogerpon_payload_done

ogerpon_payload_cornerstone:
	printstring STRINGID_ENCOGERPONCORNERSTONEHOLDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetvar 1, 2   @ Pillars = 2 (refill)
	call EncScript_Ogerpon_ApplyReduction
	encsetvar 0, 0   @ MaskEnergy = 0
	encjumpifvar CMP_NOT_EQUAL, 5, 0, ogerpon_payload_done   @ UsedCornerstone already set
	encsetvar 5, 1
	encaddvar 6, 1

ogerpon_payload_done:
	return

@ ---------------------------------------------------------------------------------------------
@ Registered entry points (named in ogerpon.encounter's Script: fields).
@ ---------------------------------------------------------------------------------------------

@ Once-per-turn scaffold, half 1 of 2: ticks the countdowns down, opens the per-hit dispatch guard.
EncScript_Ogerpon_TurnOpen::
	encsetvar 14, 1   @ TurnGuard = 1
	encsetvar 13, 0   @ MoveHandled = 0
	encjumpifvar CMP_EQUAL, 9, 0, ogerpon_turnopen_skipbw   @ BreakWindow == 0
	encsubvar 9, 1
ogerpon_turnopen_skipbw:
	encjumpifvar CMP_NOT_EQUAL, 10, 1, ogerpon_turnopen_skipct   @ Phase != Guardian's Resolve
	encjumpifvar CMP_EQUAL, 11, 0, ogerpon_turnopen_skipct       @ CycleTimer == 0
	encsubvar 11, 1
ogerpon_turnopen_skipct:
	return

@ Once-per-turn scaffold, half 2 of 2: the natural Mask Energy charge, and closing the guard.
EncScript_Ogerpon_TurnClose::
	encjumpifvar CMP_NOT_EQUAL, 10, 0, ogerpon_turnclose_clear   @ Phase != masks-come-and-go
	encaddvar 0, 1   @ MaskEnergy += 1
	printstring STRINGID_ENCOGERPONENERGYGATHERS
	waitmessage B_WAIT_TIME_LONG
ogerpon_turnclose_clear:
	encsetvar 14, 0   @ TurnGuard = 0
	return

@ Phase 0 -> Guardian's Resolve. Forces Wellspring first and starts the cycle clock.
EncScript_Ogerpon_GuardiansResolve::
	encsetvar 10, 1   @ Phase = 1
	printstring STRINGID_ENCOGERPONGUARDIANSRESOLVE
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_WELLSPRING, ogerpon_gr_applied, B_ANIM_FORM_CHANGE
ogerpon_gr_applied:
	encsetvar 15, 1   @ ActiveMask = Wellspring
	printstring STRINGID_ENCOGERPONDONSWELLSPRING
	waitmessage B_WAIT_TIME_LONG
	encsetvar 12, 3   @ CycleSpeed = 3
	encsetvar 11, 3   @ CycleTimer = 3
	encsnapshothp ENC_TARGET_BOSS, 7   @ BreakSnapshot
	encsetvar 9, 2    @ BreakWindow = 2
	call EncScript_Ogerpon_ApplyReduction
	return

@ The forced cycle: Wellspring -> Hearthflame -> Cornerstone -> Teal -> Wellspring ...
EncScript_Ogerpon_CycleAdvance::
	encjumpifvar CMP_EQUAL, 15, 1, ogerpon_cycle_toHearth
	encjumpifvar CMP_EQUAL, 15, 2, ogerpon_cycle_toCorner
	encjumpifvar CMP_EQUAL, 15, 3, ogerpon_cycle_toTeal
	goto ogerpon_cycle_toWellspring   @ ActiveMask == Teal (shouldn't happen mid-cycle, safe fallback)

ogerpon_cycle_toHearth:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_HEARTHFLAME, ogerpon_cycle_applyHearth, B_ANIM_FORM_CHANGE
ogerpon_cycle_applyHearth:
	encsetvar 15, 2
	printstring STRINGID_ENCOGERPONDONSHEARTHFLAME
	waitmessage B_WAIT_TIME_LONG
	encsnapshothp ENC_TARGET_BOSS, 7
	encsetvar 9, 2
	call EncScript_Ogerpon_ApplyReduction
	goto ogerpon_cycle_advance

ogerpon_cycle_toCorner:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_CORNERSTONE, ogerpon_cycle_applyCorner, B_ANIM_FORM_CHANGE
ogerpon_cycle_applyCorner:
	encsetvar 15, 3
	encsetvar 1, 2   @ Pillars = 2
	printstring STRINGID_ENCOGERPONDONSCORNERSTONE
	waitmessage B_WAIT_TIME_LONG
	encsnapshothp ENC_TARGET_BOSS, 7
	encsetvar 9, 2
	call EncScript_Ogerpon_ApplyReduction
	goto ogerpon_cycle_advance

ogerpon_cycle_toTeal:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_TEAL, ogerpon_cycle_applyTeal, B_ANIM_FORM_CHANGE
ogerpon_cycle_applyTeal:
	encsetvar 15, 0
	encsetvar 1, 0     @ Pillars = 0
	encsetvar 9, 0     @ BreakWindow = 0
	call EncScript_Ogerpon_ApplyReduction
	encjumpifvar CMP_EQUAL, 12, 1, ogerpon_cycle_noshrink   @ CycleSpeed == 1 (floor)
	encsubvar 12, 1
ogerpon_cycle_noshrink:
	printstring STRINGID_ENCOGERPONCYCLESHIFTS
	waitmessage B_WAIT_TIME_LONG
	goto ogerpon_cycle_advance

ogerpon_cycle_toWellspring:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_WELLSPRING, ogerpon_cycle_applyWell, B_ANIM_FORM_CHANGE
ogerpon_cycle_applyWell:
	encsetvar 15, 1
	printstring STRINGID_ENCOGERPONDONSWELLSPRING
	waitmessage B_WAIT_TIME_LONG
	encsnapshothp ENC_TARGET_BOSS, 7
	encsetvar 9, 2
	call EncScript_Ogerpon_ApplyReduction

ogerpon_cycle_advance:
	enccopyvar 11, 12   @ CycleTimer = CycleSpeed
	return

@ Per-hit dispatch, Phase 0, a Physical hit landed on the boss.
EncScript_Ogerpon_ResolvePhysical::
	encsetvar 13, 1   @ MoveHandled = 1
	encjumpifvar CMP_EQUAL, 2, 6, ogerpon_rphys_capped   @ PhysicalPressure == 6 (cap)
	encaddvar 2, 1
ogerpon_rphys_capped:
	call EncScript_Ogerpon_SEBonus
	encjumpifvar CMP_EQUAL, 15, 0, ogerpon_rphys_pillar   @ ActiveMask == Teal
	encaddvar 0, 1   @ MaskEnergy += 1 (masked build source)
ogerpon_rphys_pillar:
	call EncScript_Ogerpon_PillarTick
	call EncScript_Ogerpon_BreakCheck
	call EncScript_Ogerpon_PayloadCheck
	return

@ Per-hit dispatch, Phase 0, a Special hit landed on the boss.
EncScript_Ogerpon_ResolveSpecial::
	encsetvar 13, 1
	encjumpifvar CMP_EQUAL, 2, 0, ogerpon_rspec_floored   @ PhysicalPressure == 0 (floor)
	encsubvar 2, 1
ogerpon_rspec_floored:
	call EncScript_Ogerpon_SEBonus
	encjumpifvar CMP_EQUAL, 15, 0, ogerpon_rspec_pillar
	encaddvar 0, 1
ogerpon_rspec_pillar:
	call EncScript_Ogerpon_PillarTick
	call EncScript_Ogerpon_BreakCheck
	call EncScript_Ogerpon_PayloadCheck
	return

@ Per-hit dispatch, Phase 0, a Status hit landed on the boss - no Tendency shift, everything else
@ still runs.
EncScript_Ogerpon_ResolveStatus::
	encsetvar 13, 1
	call EncScript_Ogerpon_SEBonus
	encjumpifvar CMP_EQUAL, 15, 0, ogerpon_rstat_pillar
	encaddvar 0, 1
ogerpon_rstat_pillar:
	call EncScript_Ogerpon_PillarTick
	call EncScript_Ogerpon_BreakCheck
	call EncScript_Ogerpon_PayloadCheck
	return

@ Per-hit dispatch, Guardian's Resolve - break check and the Cornerstone shield only.
EncScript_Ogerpon_ResolvePhase1::
	encsetvar 13, 1
	call EncScript_Ogerpon_PillarTick
	call EncScript_Ogerpon_BreakCheck
	return

@ Reactive pick: Physical-heavy answers with Cornerstone; otherwise whichever of
@ Wellspring/Hearthflame hasn't been shown yet.
EncScript_Ogerpon_ReactivePick::
	encjumpifvar CMP_LESS_THAN, 2, 4, ogerpon_rp_notphysical   @ PhysicalPressure < 4
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_CORNERSTONE, ogerpon_rp_applyCorner, B_ANIM_FORM_CHANGE
ogerpon_rp_applyCorner:
	encsetvar 15, 3
	encsetvar 1, 2   @ Pillars = 2
	printstring STRINGID_ENCOGERPONDONSCORNERSTONE
	waitmessage B_WAIT_TIME_LONG
	goto ogerpon_rp_arm

ogerpon_rp_notphysical:
	encjumpifvar CMP_NOT_EQUAL, 3, 0, ogerpon_rp_hearth   @ UsedWellspring != 0
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_WELLSPRING, ogerpon_rp_applyWell, B_ANIM_FORM_CHANGE
ogerpon_rp_applyWell:
	encsetvar 15, 1
	printstring STRINGID_ENCOGERPONDONSWELLSPRING
	waitmessage B_WAIT_TIME_LONG
	goto ogerpon_rp_arm

ogerpon_rp_hearth:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_HEARTHFLAME, ogerpon_rp_applyHearth, B_ANIM_FORM_CHANGE
ogerpon_rp_applyHearth:
	encsetvar 15, 2
	printstring STRINGID_ENCOGERPONDONSHEARTHFLAME
	waitmessage B_WAIT_TIME_LONG

ogerpon_rp_arm:
	encsetvar 0, 0   @ MaskEnergy = 0
	encsnapshothp ENC_TARGET_BOSS, 7   @ BreakSnapshot
	encsetvar 9, 2   @ BreakWindow = 2
	call EncScript_Ogerpon_ApplyReduction
	return

@ Guardian's Resolve -> Unyielding: the current mask's payload fires once more, Ogerpon settles
@ to Teal, a Masked Strike scaled by masks worn closes the phase, then the catch window opens.
EncScript_Ogerpon_Unyielding::
	encsetvar 10, 2   @ Phase = 2 (Unyielding)
	printstring STRINGID_ENCOGERPONUNYIELDING
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 15, 0, ogerpon_uy_nopayload
	encjumpifvar CMP_EQUAL, 15, 1, ogerpon_uy_wellspring
	encjumpifvar CMP_EQUAL, 15, 2, ogerpon_uy_hearthflame
	goto ogerpon_uy_cornerstone

ogerpon_uy_wellspring:
	printstring STRINGID_ENCOGERPONWELLSPRINGSURGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_NOT_EQUAL, 3, 0, ogerpon_uy_nopayload
	encsetvar 3, 1
	encaddvar 6, 1
	goto ogerpon_uy_nopayload

ogerpon_uy_hearthflame:
	printstring STRINGID_ENCOGERPONFLAMESOUT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_NOT_EQUAL, 4, 0, ogerpon_uy_nopayload
	encsetvar 4, 1
	encaddvar 6, 1
	goto ogerpon_uy_nopayload

ogerpon_uy_cornerstone:
	printstring STRINGID_ENCOGERPONCORNERSTONEHOLDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_NOT_EQUAL, 5, 0, ogerpon_uy_nopayload
	encsetvar 5, 1
	encaddvar 6, 1

ogerpon_uy_nopayload:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_TEAL, ogerpon_uy_appliedTeal, B_ANIM_FORM_CHANGE
ogerpon_uy_appliedTeal:
	encsetvar 15, 0   @ ActiveMask = Teal
	encsetvar 1, 0    @ Pillars = 0
	encsetdamagereduction ENC_TARGET_BOSS, 92
	@ Masked Strike: reform into whichever mask was used most, priority Cornerstone > Hearthflame >
	@ Wellspring, else stay Teal.
	encjumpifvar CMP_NOT_EQUAL, 5, 0, ogerpon_uy_strikeCorner   @ UsedCornerstone != 0
	encjumpifvar CMP_NOT_EQUAL, 4, 0, ogerpon_uy_strikeHearth   @ UsedHearthflame != 0
	encjumpifvar CMP_NOT_EQUAL, 3, 0, ogerpon_uy_strikeWell     @ UsedWellspring != 0
	goto ogerpon_uy_strikeApplied   @ no mask ever used - stays Teal

ogerpon_uy_strikeCorner:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_CORNERSTONE, ogerpon_uy_strikeApplied, B_ANIM_FORM_CHANGE
	goto ogerpon_uy_strikeApplied

ogerpon_uy_strikeHearth:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_HEARTHFLAME, ogerpon_uy_strikeApplied, B_ANIM_FORM_CHANGE
	goto ogerpon_uy_strikeApplied

ogerpon_uy_strikeWell:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_WELLSPRING, ogerpon_uy_strikeApplied, B_ANIM_FORM_CHANGE

ogerpon_uy_strikeApplied:
	printstring STRINGID_ENCOGERPONMASKEDSTRIKE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	@ 12 + 6*UsedCount percent (12/18/24/30), chained as successive hits since encchangehp takes a
	@ literal amount.
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 6, 0, ogerpon_uy_strikeDone   @ UsedCount == 0
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_LESS_THAN, 6, 2, ogerpon_uy_strikeDone
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_LESS_THAN, 6, 3, ogerpon_uy_strikeDone
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
ogerpon_uy_strikeDone:
	encformchange ENC_TARGET_BOSS, SPECIES_OGERPON_TEAL, ogerpon_uy_finalTeal, B_ANIM_FORM_CHANGE
ogerpon_uy_finalTeal:
	encsetvar 15, 0
	printstring STRINGID_ENCOGERPONNOWISTHEMOMENT
	waitmessage B_WAIT_TIME_LONG
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 45
	return
