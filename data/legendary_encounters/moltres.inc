// Moltres, "The Everlasting Flame". Var indices, pinned by
// the always-true Conditions on EncScript_Moltres_Intro:
// 0 Phase (0 first life / 1 reborn / 2 weakened), 1 Intensity (0-5), 2 Fed, 3 Chant,
// 4 TurnGuard, 5 HitGuard, 6 BossGuard.
//
// Flame Intensity is the whole fight. It drives the guard ladder (ApplyTier), which FALLS as
// Intensity rises - the inverse of Zapdos. It rises from a move damaging Moltres (TookDamage), a
// Fire move hitting it (FireHit, which also heals it), Moltres using a Fire move (BossFire), and a
// shed sleep/freeze (TurnOpen); it decays by 1 at end of turn (TurnClose) only if Moltres took no
// damage and only while Intensity <= 3. At Intensity 4 the fire self-sustains: permanent sun, 5%
// self-damage per turn, and it never decays again. When Moltres would be KO'd the Survive clamp
// leaves it at 1 HP and Rebirth fires once - HP restored, Intensity floored at 2, sun and aura
// permanent. Weakened (10% HP, reborn) drops every guard and opens the only catch window.

// --- Shared subroutines (call/return) ---

// Sole owner of the Intensity-driven guard ladder, and silent. The weakened phase owns its own
// reduction, so this returns untouched there. The ladder is strictly monotonic, so the caller
// (GainIntensity / LoseIntensity) prints the directional line - this never does.
EncScript_Moltres_ApplyTier:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Moltres_ApplyTier_Done
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Moltres_ApplyTier_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Moltres_ApplyTier_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Moltres_ApplyTier_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Moltres_ApplyTier_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Moltres_ApplyTier_4
	encsetdamagereduction ENC_TARGET_BOSS, 64   @ Intensity 5
	return
EncScript_Moltres_ApplyTier_0:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Moltres_ApplyTier_1:
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return
EncScript_Moltres_ApplyTier_2:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Moltres_ApplyTier_3:
	encsetdamagereduction ENC_TARGET_BOSS, 79
	return
EncScript_Moltres_ApplyTier_4:
	encsetdamagereduction ENC_TARGET_BOSS, 72
EncScript_Moltres_ApplyTier_Done:
	return

// Intensity +1 (capped at 4 in the first life, 5 once reborn), one Sp. Atk stage, the new tier, the
// "guard slackens" line every time, then the tier's own flavour. Reaching 4 lights the permanent
// sun and starts the self-immolation clock.
EncScript_Moltres_GainIntensity:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Moltres_GainIntensity_RebornCap
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Moltres_GainIntensity_Done
	goto EncScript_Moltres_GainIntensity_Do
EncScript_Moltres_GainIntensity_RebornCap:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Moltres_GainIntensity_Done
EncScript_Moltres_GainIntensity_Do:
	encaddvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	call EncScript_Moltres_ApplyTier
	printstring STRINGID_ENCMOLTRESGUARDSLACKENS
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Moltres_GainIntensity_T1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Moltres_GainIntensity_T2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Moltres_GainIntensity_T3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Moltres_GainIntensity_T4
	printstring STRINGID_ENCMOLTRESEVERLASTING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return
EncScript_Moltres_GainIntensity_T1:
	printstring STRINGID_ENCMOLTRESEMBERS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return
EncScript_Moltres_GainIntensity_T2:
	printstring STRINGID_ENCMOLTRESROARS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Moltres_GainIntensity_T3:
	printstring STRINGID_ENCMOLTRESAURA
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return
EncScript_Moltres_GainIntensity_T4:
	printstring STRINGID_ENCMOLTRESENGULFED
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	waitanimation
	printstring STRINGID_ENCMOLTRESBURNSITSELF
	waitmessage B_WAIT_TIME_LONG
EncScript_Moltres_GainIntensity_Done:
	return

// Intensity -1, giving back one Sp. Atk stage, the new tier, and the "guard hardens" line. The
// floor is 0 in the first life, 2 once reborn. Never called at Intensity 4+ (TurnClose stops
// decaying there), so this never removes the sun - only Weakened does.
EncScript_Moltres_LoseIntensity:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Moltres_LoseIntensity_RebornFloor
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Moltres_LoseIntensity_Done
	goto EncScript_Moltres_LoseIntensity_Do
EncScript_Moltres_LoseIntensity_RebornFloor:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Moltres_LoseIntensity_Done
EncScript_Moltres_LoseIntensity_Do:
	encsubvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	call EncScript_Moltres_ApplyTier
	printstring STRINGID_ENCMOLTRESBANKS
	waitmessage B_WAIT_TIME_SHORT
	printstring STRINGID_ENCMOLTRESGUARDHARDENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Moltres_LoseIntensity_Done:
	return

// --- Trigger scripts ---

EncScript_Moltres_Intro::
	printstring STRINGID_ENCMOLTRESINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Runs once at the top of every turn (TurnGuard gate): clears the per-turn re-entry guards, then
// - while the fire mechanics are live - sheds sleep/freeze, which STOKES Intensity rather than
// costing it (the deliberate inversion of Zapdos).
EncScript_Moltres_TurnOpen::
	encsetvar 4, 1   @ TurnGuard
	encsetvar 5, 0   @ HitGuard
	encsetvar 6, 0   @ BossGuard
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Moltres_TurnOpen_Done   @ weakened: fire mechanics are over
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Moltres_TurnOpen_Shed
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Moltres_TurnOpen_Shed
	goto EncScript_Moltres_TurnOpen_Done
EncScript_Moltres_TurnOpen_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCMOLTRESBLAZESAWAKE
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Moltres_GainIntensity
EncScript_Moltres_TurnOpen_Done:
	return

// A damaging move lands on Moltres (Event.Cause == MOVE_DAMAGE keeps recoil/drain/ticks out). Marks
// it Fed for the turn, lashes the attacker with the flame aura from Intensity 3 (or always, once
// reborn), then feeds Intensity.
EncScript_Moltres_TookDamage::
	encsetvar 5, 1   @ HitGuard
	encsetvar 2, 1   @ Fed
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Moltres_TookDamage_Aura   @ reborn: aura is always on
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Moltres_TookDamage_Feed
EncScript_Moltres_TookDamage_Aura:
	printstring STRINGID_ENCMOLTRESAURABURNS
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_EVENT_TARGET, -7, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
EncScript_Moltres_TookDamage_Feed:
	call EncScript_Moltres_GainIntensity
	return

// A Fire move hits Moltres: it is absorbed, not survived. Moltres heals and Intensity climbs -
// strictly worse for the player than not attacking. Deliberately does not set Fed and does not
// trigger the aura.
EncScript_Moltres_FireHit::
	encsetvar 5, 1   @ HitGuard
	printstring STRINGID_ENCMOLTRESDRINKSITIN
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Moltres_GainIntensity
	return

// Moltres itself uses a Fire move: Intensity +1.
EncScript_Moltres_BossFire::
	encsetvar 6, 1   @ BossGuard
	call EncScript_Moltres_GainIntensity
	return

// OnTurnEnd, priority 90 - runs last, and does all end-of-turn work in one linear pass: field burn
// (Intensity 3+, spares Fire-types), self-immolation (Intensity 4+), decay (unfed and Intensity
// <= 3), and the recurring callout.
EncScript_Moltres_TurnClose::
	encsetvar 4, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Moltres_TurnClose_Done   @ weakened: nothing below runs
	// Field burn: Intensity 3+, the player's active Pokemon takes chip damage unless it's Fire-type.
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Moltres_TurnClose_Immolate
	jumpiftype BS_PLAYER1, TYPE_FIRE, EncScript_Moltres_TurnClose_Immolate
	printstring STRINGID_ENCMOLTRESFIELDBURNS
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Moltres_TurnClose_Burn3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Moltres_TurnClose_Burn4
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT   @ Intensity 5
	goto EncScript_Moltres_TurnClose_BurnAnim
EncScript_Moltres_TurnClose_Burn3:
	encchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	goto EncScript_Moltres_TurnClose_BurnAnim
EncScript_Moltres_TurnClose_Burn4:
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
EncScript_Moltres_TurnClose_BurnAnim:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
EncScript_Moltres_TurnClose_Immolate:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Moltres_TurnClose_Decay
	printstring STRINGID_ENCMOLTRESCONSUMESITSELF
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_BOSS, -5, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Moltres_TurnClose_Decay:
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Moltres_TurnClose_Chant   @ Fed this turn
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Moltres_TurnClose_Chant   @ Intensity 4+ never decays
	call EncScript_Moltres_LoseIntensity
EncScript_Moltres_TurnClose_Chant:
	encaddvar 3, 1   @ Chant
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Moltres_TurnClose_Reset
	encsetvar 3, 0
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Moltres_TurnClose_Reset
	printstring STRINGID_ENCMOLTRESSTILLBURNING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Moltres_TurnClose_Reset:
	encsetvar 2, 0   @ Fed
EncScript_Moltres_TurnClose_Done:
	return

// Moltres would be KO'd (Survive left it at 1 HP). It falls, the battle goes quiet, then it rises
// from its own ashes - HP restored, Intensity floored at 2, sun and flame aura made permanent,
// Speed up. Fires once; the second death is real.
EncScript_Moltres_Rebirth::
	encsetvar 0, 1   @ Phase 1 (reborn)
	printstring STRINGID_ENCMOLTRESFALLS
	waitmessage B_WAIT_TIME_LONG
	playfaintcry BS_OPPONENT1
	pause 60
	printstring STRINGID_ENCMOLTRESEMBER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_PRIMAL_REVERSION
	waitanimation
	printstring STRINGID_ENCMOLTRESREBIRTH
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_BOSS, 55, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encsetvar 1, 2   @ Intensity 2, and it floors here now
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12    @ push to the cap, then...
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -4    @ ...back down to the Intensity-2 baseline (+2)
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encsetweather BATTLE_WEATHER_SUN, 0
	call EncScript_Moltres_ApplyTier   @ 84
	printstring STRINGID_ENCMOLTRESSCORCHED
	waitmessage B_WAIT_TIME_LONG
	return

// 10% HP while reborn: the everlasting flame dims. Every guard and immunity drops, the survive
// clamp comes off (the automatic catch-window guard takes over on the trailing pass), and Poke
// Balls are unblocked. Phase 2 gates off every fire-mechanic trigger.
EncScript_Moltres_Weakened::
	encsetvar 0, 2   @ Phase 2 (weakened)
	encsetvar 1, 0   @ Intensity 0
	removeweather
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6   @ Sp. Atk back to neutral
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMOLTRESWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
