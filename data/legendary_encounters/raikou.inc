// Raikou, "The Hunting Thunder". Var indices, pinned by
// the always-true Conditions on EncScript_Raikou_Intro:
// 0 Phase (0 prowl / 1 hunt / 2 lightning incarnate / 3 weakened), 1 Velocity (0-5), 2 Mark
// (0 none, else 3->2->1 on the current quarry), 3 MarkCool, 4 Dash (0 none / 2 telegraphed /
// 1 dashing this turn), 5 Instability (0-4), 6 Soft (turns of soft-guard window left), 7 LastGuard
// (reduction last ANNOUNCED), 8 MoveSeen, 9 BossActed, 10 TurnGuard, 11 PMove (0 none / 1 status /
// 2 attacked), 12 BMoveGuard, 13 SwitchGuard, 14 FaintGuard.
//
// Velocity is tempo, not charge: it rises when Raikou acts first and when it knocks something out,
// and falls whenever the initiative is taken from it. Its guard runs UPWARD with Velocity
// (78/84/89/93), so the fight is "never let it get going" - and every turn spent slowing it is a
// turn not spent damaging it. Two windows genuinely open its guard: breaking the Mark (70 for one
// turn) and the final phase's wild discharge (55 for two). Velocity moves a raw Speed stat, never a
// stage, so the "it is slowed" drain reads only the player's own speed control.

// --- Shared subroutines (call/return) ---

// Sole owner of the guard ladder AND its callout. Returns without touching anything while a soft
// window owns the reduction, or in Weakened. Otherwise it writes the tier for the current Velocity
// and compares it against the last ANNOUNCED value, printing on every change in either direction -
// there is no UI for damage reduction, so a single showing would be trivially missed.
EncScript_Raikou_ApplyTier:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Raikou_ApplyTier_Done          @ weakened owns its own guard
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Raikou_ApplyTier_Done   @ a window owns the guard
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Raikou_ApplyTier_T93
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Raikou_ApplyTier_T89
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Raikou_ApplyTier_T84
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encjumpifvar CMP_EQUAL, 7, 78, EncScript_Raikou_ApplyTier_Done
	encjumpifvar CMP_LESS_THAN, 7, 78, EncScript_Raikou_ApplyTier_T78_Rose
	encsetvar 7, 78
	goto EncScript_Raikou_ApplyTier_Fell
EncScript_Raikou_ApplyTier_T78_Rose:
	encsetvar 7, 78
	goto EncScript_Raikou_ApplyTier_Rose
EncScript_Raikou_ApplyTier_T84:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	encjumpifvar CMP_EQUAL, 7, 84, EncScript_Raikou_ApplyTier_Done
	encjumpifvar CMP_LESS_THAN, 7, 84, EncScript_Raikou_ApplyTier_T84_Rose
	encsetvar 7, 84
	goto EncScript_Raikou_ApplyTier_Fell
EncScript_Raikou_ApplyTier_T84_Rose:
	encsetvar 7, 84
	goto EncScript_Raikou_ApplyTier_Rose
EncScript_Raikou_ApplyTier_T89:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encjumpifvar CMP_EQUAL, 7, 89, EncScript_Raikou_ApplyTier_Done
	encjumpifvar CMP_LESS_THAN, 7, 89, EncScript_Raikou_ApplyTier_T89_Rose
	encsetvar 7, 89
	goto EncScript_Raikou_ApplyTier_Fell
EncScript_Raikou_ApplyTier_T89_Rose:
	encsetvar 7, 89
	goto EncScript_Raikou_ApplyTier_Rose
EncScript_Raikou_ApplyTier_T93:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encjumpifvar CMP_EQUAL, 7, 93, EncScript_Raikou_ApplyTier_Done
	encsetvar 7, 93
EncScript_Raikou_ApplyTier_Rose:
	printstring STRINGID_ENCRAIKOUBLURS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Raikou_ApplyTier_Fell:
	printstring STRINGID_ENCRAIKOUSLOWS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Raikou_ApplyTier_Done:
	return

// Velocity +1 (cap 5) and the raw Speed that rides with it. +12% up against -11% down means a full
// up-and-down cycle is very nearly neutral (1.12 * 0.89 = 0.9968).
EncScript_Raikou_GainVelocity:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Raikou_GainVelocity_Done   @ weakened: the hunt is over
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Raikou_GainVelocity_Done
	encaddvar 1, 1
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 12, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Raikou_GainVelocity_Full
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Raikou_GainVelocity_Blur
	printstring STRINGID_ENCRAIKOUQUICKENS
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Raikou_GainVelocity_Tier
EncScript_Raikou_GainVelocity_Blur:
	printstring STRINGID_ENCRAIKOUBECOMESABLUR
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_AGILITY
	waitanimation
	goto EncScript_Raikou_GainVelocity_Tier
EncScript_Raikou_GainVelocity_Full:
	printstring STRINGID_ENCRAIKOUFULLSTRIDE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EXTREME_SPEED
	waitanimation
EncScript_Raikou_GainVelocity_Tier:
	call EncScript_Raikou_ApplyTier
EncScript_Raikou_GainVelocity_Done:
	return

// Velocity -1, and the only place the phase-aware floor lives: 0 while prowling, 2 from the Hunt on -
// once it has roared it can never be fully grounded again.
EncScript_Raikou_LoseVelocity1:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Raikou_LoseVelocity1_Done
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Raikou_LoseVelocity1_Done   @ phase 2+: locked at full stride
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Raikou_LoseVelocity1_Do    @ prowling: the floor is 0
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Raikou_LoseVelocity1_Done   @ the Hunt's floor is 2
EncScript_Raikou_LoseVelocity1_Do:
	encsubvar 1, 1
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, -11, ENC_AMOUNT_PERCENT
	call EncScript_Raikou_ApplyTier
EncScript_Raikou_LoseVelocity1_Done:
	return

// Every "big" lever routes through here so the floor logic stays in exactly one place.
EncScript_Raikou_LoseVelocity2:
	call EncScript_Raikou_LoseVelocity1
	call EncScript_Raikou_LoseVelocity1
	return

// Leer's animation is a short glint on the target - the closest existing asset to "its eyes lock on".
EncScript_Raikou_LayMark:
	encsetvar 2, 3   @ Mark: TurnClose steps it 3->2->1, TurnOpen resolves it at 1
	printstring STRINGID_ENCRAIKOUMARKS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_LEER
	waitanimation
	return

// --- Trigger scripts ---

// LastGuard is seeded to the Properties reduction so the first tier change reads as a change.
EncScript_Raikou_Intro::
	encsetvar 7, 78
	printstring STRINGID_ENCRAIKOUINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCRAIKOUCIRCLES
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Then, in order: clear the
// per-turn guards, execute a matured Dash, shed sleep/freeze, drain for a negative Speed stage,
// resolve or lay the Mark, and roll the storm.
EncScript_Raikou_TurnOpen::
	flushtextbox
	encsetvar 10, 1   @ TurnGuard
	encsetvar 8, 0    @ MoveSeen
	encsetvar 9, 0    @ BossActed
	encsetvar 11, 0   @ PMove
	encsetvar 12, 0   @ BMoveGuard
	encsetvar 13, 0   @ SwitchGuard
	encsetvar 14, 0   @ FaintGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Raikou_TurnOpen_Done   @ weakened: inert
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Raikou_TurnOpen_Status
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCRAIKOUVANISHES
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_AGILITY
	waitanimation
EncScript_Raikou_TurnOpen_Status:
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Raikou_TurnOpen_Shed
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Raikou_TurnOpen_Shed
	goto EncScript_Raikou_TurnOpen_Slowed
EncScript_Raikou_TurnOpen_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCRAIKOUTEARSFREE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_THUNDER_SHOCK
	waitanimation
	call EncScript_Raikou_LoseVelocity2
EncScript_Raikou_TurnOpen_Slowed:
	jumpifstat BS_OPPONENT1, CMP_LESS_THAN, STAT_SPEED, DEFAULT_STAT_STAGE, EncScript_Raikou_TurnOpen_Drain
	goto EncScript_Raikou_TurnOpen_Mark
EncScript_Raikou_TurnOpen_Drain:
	printstring STRINGID_ENCRAIKOUCANTFINDFOOTING
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Raikou_LoseVelocity1
EncScript_Raikou_TurnOpen_Mark:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Raikou_TurnOpen_Break
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Raikou_TurnOpen_Storm      @ a mark is still counting down
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Raikou_TurnOpen_Storm   @ still on cooldown
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Raikou_TurnOpen_Lay     @ the Hunt marks unconditionally
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Raikou_TurnOpen_Storm      @ prowling: needs Velocity 3+
EncScript_Raikou_TurnOpen_Lay:
	call EncScript_Raikou_LayMark
	goto EncScript_Raikou_TurnOpen_Storm

// The quarry stood its ground for all three turns: the biggest tempo break in the fight, plus a
// one-turn window at 70. Soft counts open turns: TurnClose ticks it down tonight and restores the
// ladder, so 1 buys exactly this turn.
EncScript_Raikou_TurnOpen_Break:
	encsetvar 2, 0   @ Mark cleared
	printstring STRINGID_ENCRAIKOUQUARRYSTOOD
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCRAIKOUOVERSHOOTS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	encsetvar 6, 1   @ Soft: one full turn of open guard (TurnClose closes it tonight)
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encsetvar 7, 70
	call EncScript_Raikou_LoseVelocity2
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Raikou_TurnOpen_BreakHunt
	encsetvar 3, 2   @ MarkCool: prowling Raikou needs two turns before it re-marks
	goto EncScript_Raikou_TurnOpen_Storm
EncScript_Raikou_TurnOpen_BreakHunt:
	call EncScript_Raikou_LoseVelocity1   @ -3 total once the Hunt has begun
EncScript_Raikou_TurnOpen_Storm:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Raikou_TurnOpen_Done
	encjumpifchance 55, EncScript_Raikou_TurnOpen_StormRoll
	goto EncScript_Raikou_TurnOpen_Done
EncScript_Raikou_TurnOpen_StormRoll:
	printstring STRINGID_ENCRAIKOUSTORMROLLS
	waitmessage B_WAIT_TIME_SHORT
	encjumpifchance 30, EncScript_Raikou_TurnOpen_StormBolt
	encjumpifchance 36, EncScript_Raikou_TurnOpen_StormFeeds     @ ~25% of the whole roll
	encjumpifchance 56, EncScript_Raikou_TurnOpen_StormRain      @ ~25%
	printstring STRINGID_ENCRAIKOUSTATICHANGS                    @ ~20%
	waitmessage B_WAIT_TIME_SHORT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 6, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_CHARGE
	waitanimation
	goto EncScript_Raikou_TurnOpen_Done
EncScript_Raikou_TurnOpen_StormBolt:
	printstring STRINGID_ENCRAIKOUBOLT
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER
	waitanimation
	goto EncScript_Raikou_TurnOpen_Done
EncScript_Raikou_TurnOpen_StormFeeds:
	printstring STRINGID_ENCRAIKOUSTORMFEEDS
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Raikou_GainVelocity
	goto EncScript_Raikou_TurnOpen_Done
EncScript_Raikou_TurnOpen_StormRain:
	printstring STRINGID_ENCRAIKOURAINLASHES
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_ALL_FOES, STAT_ACC, -1
	playmoveanimation MOVE_RAIN_DANCE
	waitanimation
EncScript_Raikou_TurnOpen_Done:
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown: the denied-turn drain, the Dash lifecycle, the Mark tick, MarkCool, the phase-2 arc and
// its Instability climb, the soft-window tick, and arming the next Dash telegraph.
EncScript_Raikou_TurnClose::
	encsetvar 10, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Raikou_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Raikou_TurnClose_Dash
	printstring STRINGID_ENCRAIKOUDENIED
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Raikou_LoseVelocity2
EncScript_Raikou_TurnClose_Dash:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Raikou_TurnClose_DashSpent
	encjumpifvar CMP_NOT_EQUAL, 4, 2, EncScript_Raikou_TurnClose_Mark
	encsetvar 4, 1   @ the telegraph matures; next TurnOpen executes the dash
	goto EncScript_Raikou_TurnClose_Mark
EncScript_Raikou_TurnClose_DashSpent:
	encsetvar 4, 0
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Raikou_TurnClose_Mark   @ the player threw an attack into it
	printstring STRINGID_ENCRAIKOUREAD
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Raikou_LoseVelocity2
EncScript_Raikou_TurnClose_Mark:
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Raikou_TurnClose_Cool
	encsubvar 2, 1
	printstring STRINGID_ENCRAIKOUPRESSES
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER_SHOCK
	waitanimation
EncScript_Raikou_TurnClose_Cool:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Raikou_TurnClose_Arc
	encsubvar 3, 1
EncScript_Raikou_TurnClose_Arc:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Raikou_TurnClose_Soft
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Raikou_TurnClose_Soft   @ mid-discharge: it is not arcing
	printstring STRINGID_ENCRAIKOUARCS
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER
	waitanimation
	encjumpifvar CMP_GREATER_THAN, 5, 3, EncScript_Raikou_TurnClose_Soft   @ already pending; the HP floor is holding it back
	encaddvar 5, 1   @ Instability
	encjumpifvar CMP_NOT_EQUAL, 5, 3, EncScript_Raikou_TurnClose_Soft
	printstring STRINGID_ENCRAIKOUSTRAINING
	waitmessage B_WAIT_TIME_LONG
EncScript_Raikou_TurnClose_Soft:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Raikou_TurnClose_Telegraph
	encsubvar 6, 1
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Raikou_TurnClose_Done   @ window still open; no telegraph either
	call EncScript_Raikou_ApplyTier   @ window closed: announces the guard coming back up
	goto EncScript_Raikou_TurnClose_Done
EncScript_Raikou_TurnClose_Telegraph:
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Raikou_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Raikou_TurnClose_Done
	encsetvar 4, 2   @ Dash telegraphed; TurnClose matures it next turn, TurnOpen executes it after
	printstring STRINGID_ENCRAIKOUCOILS
	waitmessage B_WAIT_TIME_LONG
EncScript_Raikou_TurnClose_Done:
	return

// The tempo brake. Unlike Zapdos's Ground lever this one also deals its damage - Raikou is pure
// Electric, so a Ground move is both its only super-effective matchup and the way to slow it down.
// That is why Aura Sphere is on the moveset: reaching for the lever means bringing a Ground/Rock or
// Ground/Steel body Raikou is built to delete.
EncScript_Raikou_PlayerGround::
	encsetvar 11, 2   @ PMove: an attack
	encsetvar 8, 1    @ MoveSeen
	printstring STRINGID_ENCRAIKOUEARTHED
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Raikou_PlayerGround_Hunt
	call EncScript_Raikou_LoseVelocity2
	return
EncScript_Raikou_PlayerGround_Hunt:
	call EncScript_Raikou_LoseVelocity1   @ the lever weakens once the Hunt has begun
	return

// Attacking Raikou does not feed it; only tempo does. Both of these exist to record who acted first
// and, for the Dash read, what kind of thing the player did.
EncScript_Raikou_PlayerAttack::
	encsetvar 11, 2   @ PMove: an attack
	encsetvar 8, 1    @ MoveSeen
	return

EncScript_Raikou_PlayerStatus::
	encsetvar 11, 1   @ PMove: a status move
	encsetvar 8, 1    @ MoveSeen
	return

// OnMoveEnd fires whether or not the move connected, so a Raikou move that missed still counts as
// having got there first. That is deliberate - the mechanic measures initiative, not contact.
EncScript_Raikou_BossMove::
	encsetvar 12, 1   @ BMoveGuard
	encsetvar 9, 1    @ BossActed
	encjumpifvar CMP_NOT_EQUAL, 8, 0, EncScript_Raikou_BossMove_Done
	encsetvar 8, 1    @ MoveSeen
	printstring STRINGID_ENCRAIKOUOUTPACES
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Raikou_GainVelocity
	return
EncScript_Raikou_BossMove_Done:
	encsetvar 8, 1
	return

EncScript_Raikou_Faint::
	encsetvar 14, 1   @ FaintGuard
	printstring STRINGID_ENCRAIKOURUNSDOWN
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Raikou_GainVelocity
	call EncScript_Raikou_GainVelocity
	return

// Switching feeds the hunt, which is the inversion the fight is built on: the Mark is answered by
// standing your ground, not by running from it. From the Hunt on, the newcomer is struck on arrival.
EncScript_Raikou_SwitchIn::
	encsetvar 13, 1   @ SwitchGuard
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Raikou_SwitchIn_Done
	printstring STRINGID_ENCRAIKOUGIVESCHASE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Raikou_GainVelocity
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Raikou_SwitchIn_Relay
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_EXTREME_SPEED
	waitanimation
EncScript_Raikou_SwitchIn_Relay:
	call EncScript_Raikou_LayMark
EncScript_Raikou_SwitchIn_Done:
	return

// Phase 1 at 50% HP. The Velocity floor rises to 2 - it can never be fully grounded again - the mark
// becomes permanent and the Ground lever weakens, but breaking the mark is now worth 3 Velocity and
// a one-turn open guard. That trade is the player's objective for the rest of the fight.
EncScript_Raikou_Hunt::
	encsetvar 0, 1   @ Phase 1
	encsetvar 3, 0   @ MarkCool cleared: the hunt does not pause
	printstring STRINGID_ENCRAIKOUROAR
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROAR
	waitanimation
	printstring STRINGID_ENCRAIKOUHUNTBEGINS
	waitmessage B_WAIT_TIME_LONG
EncScript_Raikou_Hunt_Floor:
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Raikou_Hunt_Mark
	call EncScript_Raikou_GainVelocity
	goto EncScript_Raikou_Hunt_Floor
EncScript_Raikou_Hunt_Mark:
	encsetvar 2, 0
	call EncScript_Raikou_LayMark
	call EncScript_Raikou_ApplyTier
	return

// Phase 2 at 25% HP. Velocity locks at 5, raw Speed +40%, offense +1 stage each side, and a 10% arc
// on the player's team every turn. Instability then climbs +1 per turn until it discharges - the
// second line is the cue that a window is coming without handing over the timing.
EncScript_Raikou_Incarnate::
	encsetvar 0, 2   @ Phase 2
	encsetvar 1, 5   @ Velocity locked at full stride
	encsetvar 4, 0   @ any pending dash is abandoned
	encsetvar 6, 0   @ Soft
	encsetvar 5, 0   @ Instability
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 40, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	printstring STRINGID_ENCRAIKOUINCARNATE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_DISCHARGE
	waitanimation
	printstring STRINGID_ENCRAIKOUUNSTABLE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Raikou_ApplyTier
	return

// Instability 4: it blows itself off its feet. Two full turns at 55, 12% self-damage, and the cycle
// rebuilds from zero.
EncScript_Raikou_Discharge::
	flushtextbox
	encsetvar 5, 0   @ Instability
	encsetvar 4, 0   @ Dash
	encsetvar 6, 2   @ Soft: two full turns of open guard
	encsetdamagereduction ENC_TARGET_BOSS, 70
	encsetvar 7, 55
	printstring STRINGID_ENCRAIKOUDISCHARGES
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EXPLOSION
	waitanimation
	encchangehp ENC_TARGET_BOSS, -12, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCRAIKOUOPENING
	waitmessage B_WAIT_TIME_LONG
	return

// Below 10% in phase 2. Everything off - a pending mark or a discharge landing after Raikou has
// visibly collapsed would be incoherent, and a team-wide tick during the catch phase is pure
// frustration. CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the catch window.
EncScript_Raikou_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Velocity
	encsetvar 2, 0   @ Mark
	encsetvar 3, 0   @ MarkCool
	encsetvar 4, 0   @ Dash
	encsetvar 5, 0   @ Instability
	encsetvar 6, 0   @ Soft
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetvar 7, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, -50, ENC_AMOUNT_PERCENT
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCRAIKOUWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
