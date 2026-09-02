// Ho-Oh, "The Rainbow Above the Ashes". Var indices, pinned
// by the always-true Conditions on EncScript_HoOh_Intro:
// 0 Phase (0 The Rainbow Pokemon / 1 The Rainbow / 2 Divine Rebirth / 3 Ashes), 1 Flame (0-8),
// 2 Trial (judgment countdown, 4 -> 0), 3 Flights (player switches since the last judgment),
// 4 Fallen (player Pokemon lost since the last judgment), 5 Chosen (active blessing, 0 none),
// 6 Grace (blessing timer, 3 -> 0), 7 Rite (Divine Rebirth cycle, 4 -> 1), 8 LastGuard (reduction
// last ANNOUNCED), 9 LastFlame (flame tier last announced), 10 TurnGuard, 11 Rise (per-turn
// flame-income cap shared by the Fire-move and faint triggers), 12 Wake (per-turn switch-in
// guard), 13 HpMark (boss HP percentage at turn open; ENC_SNAP_DAMAGE turns it into "lost this
// turn"), 14 Prev (ApplyGuard's before-value scratch, and encrewindhp's destination scratch),
// 15 Toll (per-turn re-entry guard for the faint trigger).
//
// Sacred Flame is Ho-Oh's damage reduction, its heal budget and its second life at once, and every
// spend is something it does FOR the player: mending when hit hard, purifying when statused,
// blessing, reviving, rekindling the sun. Income is the Rainbow's sunlight, its own Sacred Fire,
// and the two judgments that punish bad play. At 0 HP the flame decides the fight: 4 or more and
// Ho-Oh is reborn into a whole extra phase, 3 or less and it guts out into the catch window.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout, reached only through Reckon. Prev holds the
// value last announced so each leaf can write LastGuard before comparing; there is no UI for damage
// reduction, so a single showing would be trivially missed and the line has to re-fire on every real
// change. The bands are the same ones FlameTier announces, so a guard move and a flame-tier move
// always arrive together.
EncScript_HoOh_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_HoOh_ApplyGuard_Done   @ weakened owns its own guard
	enccopyvar 14, 8
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_HoOh_Guard80           @ Divine Rebirth: flame locked at 0
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_HoOh_ApplyGuard_P1
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_HoOh_Guard88
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_HoOh_Guard84
	goto EncScript_HoOh_Guard80
EncScript_HoOh_ApplyGuard_P1:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_HoOh_Guard90
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_HoOh_Guard86
	goto EncScript_HoOh_Guard82

EncScript_HoOh_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 8, 80
	encjumpifvar CMP_EQUAL, 14, 80, EncScript_HoOh_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 14, 80, EncScript_HoOh_ApplyGuard_Rise
	goto EncScript_HoOh_ApplyGuard_Fall
EncScript_HoOh_Guard82:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetvar 8, 82
	encjumpifvar CMP_EQUAL, 14, 82, EncScript_HoOh_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 14, 82, EncScript_HoOh_ApplyGuard_Rise
	goto EncScript_HoOh_ApplyGuard_Fall
EncScript_HoOh_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 8, 84
	encjumpifvar CMP_EQUAL, 14, 84, EncScript_HoOh_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 14, 84, EncScript_HoOh_ApplyGuard_Rise
	goto EncScript_HoOh_ApplyGuard_Fall
EncScript_HoOh_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 8, 86
	encjumpifvar CMP_EQUAL, 14, 86, EncScript_HoOh_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 14, 86, EncScript_HoOh_ApplyGuard_Rise
	goto EncScript_HoOh_ApplyGuard_Fall
EncScript_HoOh_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 8, 88
	encjumpifvar CMP_EQUAL, 14, 88, EncScript_HoOh_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 14, 88, EncScript_HoOh_ApplyGuard_Rise
	goto EncScript_HoOh_ApplyGuard_Fall
EncScript_HoOh_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 8, 90
	encjumpifvar CMP_EQUAL, 14, 90, EncScript_HoOh_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 14, 90, EncScript_HoOh_ApplyGuard_Rise
	goto EncScript_HoOh_ApplyGuard_Fall

EncScript_HoOh_ApplyGuard_Rise:
	printstring STRINGID_ENCHOOHGUARDRISE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_HoOh_ApplyGuard_Fall:
	printstring STRINGID_ENCHOOHGUARDFALL
	waitmessage B_WAIT_TIME_SHORT
EncScript_HoOh_ApplyGuard_Done:
	return

// The player's only window onto Sacred Flame - the number itself is never shown. Reached only
// through Reckon. Latched on LastFlame so it prints only when the tier actually moves;
// four tiers rather than nine keeps the fight from narrating every point. Silent from Divine
// Rebirth onward, where the flame is gone for good and the banners say so instead.
EncScript_HoOh_FlameTier:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_HoOh_FlameTier_Done
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_HoOh_FlameTier_T0
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_HoOh_FlameTier_T3
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_HoOh_FlameTier_T2
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_HoOh_FlameTier_Done
	encsetvar 9, 1
	printstring STRINGID_ENCHOOHFLAMELOW
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_HoOh_FlameTier_T0:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_HoOh_FlameTier_Done
	encsetvar 9, 0
	printstring STRINGID_ENCHOOHFLAMEOUT
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_HoOh_FlameTier_T2:
	encjumpifvar CMP_EQUAL, 9, 2, EncScript_HoOh_FlameTier_Done
	encsetvar 9, 2
	printstring STRINGID_ENCHOOHFLAMESTEADY
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_HoOh_FlameTier_T3:
	encjumpifvar CMP_EQUAL, 9, 3, EncScript_HoOh_FlameTier_Done
	encsetvar 9, 3
	printstring STRINGID_ENCHOOHFLAMEHIGH
	waitmessage B_WAIT_TIME_SHORT
EncScript_HoOh_FlameTier_Done:
	return

// Clamped 0-8, and inert from Divine Rebirth onward - the rebirth spends the flame permanently, so
// every income and spend has to stop dead there. Both are SILENT and neither touches the damage
// reduction; that is Reckon's job, below.
EncScript_HoOh_GainFlame:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_HoOh_GainFlame_Done
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_HoOh_GainFlame_Done
	encaddvar 1, 1
EncScript_HoOh_GainFlame_Done:
	return

EncScript_HoOh_SpendFlame:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_HoOh_SpendFlame_Done
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_HoOh_SpendFlame_Done
	encsubvar 1, 1
EncScript_HoOh_SpendFlame_Done:
	return

// The fight's only announcement point, and the only place the guard is recomputed. Flame moves
// silently wherever it moves - a Fire move at OnMoveEnd, a faint, a verdict, a mend - and is
// reckoned up exactly twice a turn, at the open and at the close.
//
// This is what keeps the two latched lines from repeating inside one checkpoint. Announcing at each
// movement latches per CALL, not per checkpoint, so a script that spends two flame across a band
// boundary, or a verdict that gains one before TurnClose gains another, prints the same line twice
// in a row. Reckoning once collapses a whole dispatch's movement into a single net reading, and it
// also means the guard the player was told about is the guard they face for the rest of the turn.
// The tier line runs before the guard line so the player reads cause then effect.
EncScript_HoOh_Reckon:
	call EncScript_HoOh_FlameTier
	call EncScript_HoOh_ApplyGuard
	return

// Ordinary weather, deliberately: the player can overwrite it with any weather move, which cuts the
// Rainbow's flame income. Ho-Oh buys it back at the next turn open for one flame, so blowing the
// sun out costs the player a turn and Ho-Oh a piece of its resurrection. Under sun Weather Ball is
// a 100 BP Fire move instead of a 50 BP Normal one, which is the escalation the player feels first.
EncScript_HoOh_ApplySun:
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return

// Drops a blessing WITHOUT mirroring its stat stages back out, and prints nothing - the caller
// supplies its own line. For the two paths where the stages are already gone on their own: switching
// out discards them, and an Impure verdict Hazes them away. Subtracting them again there would put
// the player BELOW neutral for a boon they no longer hold. Blessing 4's Safeguard is not a stat
// stage and survives both, so it is stripped here either way - otherwise switching out of Purity
// would leave the player a permanent Safeguard. encclearsidestatus rather than encclearscreens, so
// the player's own screens are not collateral.
EncScript_HoOh_ShedBlessing:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_HoOh_ShedBlessing_Done
	encjumpifvar CMP_NOT_EQUAL, 5, 4, EncScript_HoOh_ShedBlessing_Clear
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SAFEGUARD
EncScript_HoOh_ShedBlessing_Clear:
	encsetvar 5, 0   @ Chosen
	encsetvar 6, 0   @ Grace
EncScript_HoOh_ShedBlessing_Done:
	return

// The ordinary end of a blessing: mirror the granted stages back out, then shed the rest. Called
// when Grace runs out, when a fresh blessing replaces it, and at the rebirth gate.
EncScript_HoOh_EndBlessing:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_HoOh_EndBlessing_Done
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_HoOh_EndBlessing_Swift
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_HoOh_EndBlessing_Valor
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_HoOh_EndBlessing_Aegis
	goto EncScript_HoOh_EndBlessing_Show
EncScript_HoOh_EndBlessing_Swift:
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -2
	goto EncScript_HoOh_EndBlessing_Show
EncScript_HoOh_EndBlessing_Valor:
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, -1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, -1
	goto EncScript_HoOh_EndBlessing_Show
EncScript_HoOh_EndBlessing_Aegis:
	encchangestat ENC_TARGET_ALL_FOES, STAT_DEF, -1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPDEF, -1
EncScript_HoOh_EndBlessing_Show:
	call EncScript_HoOh_ShedBlessing
	printstring STRINGID_ENCHOOHBLESSENDS
	waitmessage B_WAIT_TIME_SHORT
EncScript_HoOh_EndBlessing_Done:
	return

// Shared verdict tail: re-arm the trial and forget everything it was weighing.
EncScript_HoOh_JudgeDone:
	encsetvar 2, 4   @ Trial
	encsetvar 3, 0   @ Flights
	encsetvar 4, 0   @ Fallen
	return

// --- Trigger scripts ---

// Flame starts at 3, so the fight opens mid-ladder and can visibly move either way from turn one.
// LastGuard is seeded to the Properties reduction and LastFlame to the matching tier, so the first
// real movement in either reads as a change. The second line is the whole hint the fight rests on:
// Ho-Oh's fire is something it SPENDS.
EncScript_HoOh_Intro::
	encsetvar 1, 3     @ Flame
	encsetvar 2, 4     @ Trial
	encsetvar 8, 87    @ LastGuard
	encsetvar 9, 2     @ LastFlame: the 2-4 tier
	encsetvar 13, 100  @ HpMark
	printstring STRINGID_ENCHOOHINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCHOOHFIRE
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Owns the trial countdown
// (ticked here, resolved at OnTurnEnd, so a real turn passes between a tick and its verdict), the
// rekindle, and the two standing flame cues.
EncScript_HoOh_TurnOpen::
	flushtextbox
	encsetvar 10, 1   @ TurnGuard
	encsetvar 11, 0   @ Rise
	encsetvar 15, 0   @ Toll
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_HoOh_TurnOpen_Done   @ weakened: inert
	encsnapshothp ENC_TARGET_BOSS, 13, ENC_SNAP_SET
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_HoOh_TurnOpen_Rite
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_HoOh_TurnOpen_Rekindle
	encsubvar 2, 1
	@ The rainbow's sunlight is the flame's only standing income, so Ho-Oh buys it back the moment
	@ the player washes it away. jumpifhalfword rather than jumpifweatheraffected: the latter reads
	@ gBattlerAttacker, which is stale at a checkpoint.
EncScript_HoOh_TurnOpen_Rekindle:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_HoOh_TurnOpen_Reckon
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_HoOh_TurnOpen_Reckon
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_SUN, EncScript_HoOh_TurnOpen_DoRekindle
	goto EncScript_HoOh_TurnOpen_Reckon
EncScript_HoOh_TurnOpen_DoRekindle:
	printstring STRINGID_ENCHOOHREKINDLE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_HoOh_ApplySun
	call EncScript_HoOh_SpendFlame
	@ The turn's opening reckoning: this is where a rekindle's spend is announced, and where the guard
	@ the player faces for the rest of the turn is set.
EncScript_HoOh_TurnOpen_Reckon:
	call EncScript_HoOh_Reckon
	@ Standing cues at the two ends of the ladder. The latched tier line covers movement; these two
	@ keep the state readable for a player who joined the fight mid-phase or missed the transition.
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_HoOh_TurnOpen_Ember
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_HoOh_TurnOpen_Blaze
	goto EncScript_HoOh_TurnOpen_Trial
EncScript_HoOh_TurnOpen_Ember:
	printstring STRINGID_ENCHOOHEMBER
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_HoOh_TurnOpen_Trial
EncScript_HoOh_TurnOpen_Blaze:
	printstring STRINGID_ENCHOOHBLAZE
	waitmessage B_WAIT_TIME_SHORT
EncScript_HoOh_TurnOpen_Trial:
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_HoOh_TurnOpen_Done
	printstring STRINGID_ENCHOOHTRIALNEAR
	waitmessage B_WAIT_TIME_SHORT
EncScript_HoOh_TurnOpen_Done:
	return
	@ Divine Rebirth: Rite is stepped at TurnClose and resolved here, one checkpoint later. Living
	@ inside TurnOpen rather than in level triggers of its own means it inherits TurnGuard's
	@ once-per-turn guarantee and cannot re-select.
EncScript_HoOh_TurnOpen_Rite:
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_HoOh_Rite_SacredFlame
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_HoOh_Rite_Purification
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_HoOh_Rite_Judgment
	goto EncScript_HoOh_Rite_Phoenix

// Rite 4. RAW stats, not stages, specifically so Rite 3's own Purification does not undo them:
// Ho-Oh's gains compound across cycles while the player's are wiped every fourth turn. That
// asymmetry is why the phase has to be closed out rather than stalled.
EncScript_HoOh_Rite_SacredFlame:
	printstring STRINGID_ENCHOOHRITEFLAME
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 10, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 10, ENC_AMOUNT_PERCENT
	return

// Rite 3. normalisebuffs Hazes every battler, Ho-Oh included - thematically exact for a
// purification, and free to it, since its own gains are raw stat values.
EncScript_HoOh_Rite_Purification:
	printstring STRINGID_ENCHOOHRITEPURIFY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
	normalisebuffs
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_HoOh_Rite_Purification_Done
EncScript_HoOh_Rite_Purification_Done:
	return

// Rite 2 and Rite 1. Scripted enchangehp rather than moves, so Protect and screens do not stop
// them. No self-recoil on the Phoenix: scripted damage bypasses the Survive clamp, so a recoil tick
// could faint Ho-Oh outright and skip the catch window.
EncScript_HoOh_Rite_Judgment:
	printstring STRINGID_ENCHOOHRITEJUDGMENT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EXTRASENSORY
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return

EncScript_HoOh_Rite_Phoenix:
	printstring STRINGID_ENCHOOHRITEPHOENIX
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SKY_ATTACK
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return

// OnTurnEnd, priority 90 - runs after every phase transition and every verdict at the same
// checkpoint. Owns the reactive spends, and the order is the design: the blessing's price is
// charged before the mend, so a turn that ends a blessing still bills for it.
EncScript_HoOh_TurnClose::
	encsetvar 10, 0   @ TurnGuard
	encsetvar 12, 0   @ Wake
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_HoOh_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_HoOh_TurnClose_Rite
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_HoOh_TurnClose_Mend
	enchangehp ENC_TARGET_ALL_FOES, -7, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCHOOHBLESSPRICE
	waitmessage B_WAIT_TIME_SHORT
	encsubvar 6, 1
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_HoOh_TurnClose_Mend
	call EncScript_HoOh_EndBlessing
	@ Mending is the dominant sink and it is keyed to the player's own aggression: hitting hard
	@ drains the flame but lengthens the fight, chipping gently keeps the fight short and leaves a
	@ rebirth waiting. ENC_SNAP_DAMAGE turns HpMark from "HP at turn open" into "percentage lost
	@ this turn", which is the only way to ask that of a LevelCap boss whose max HP is unknown here.
EncScript_HoOh_TurnClose_Mend:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_HoOh_TurnClose_Purify
	encsnapshothp ENC_TARGET_BOSS, 13, ENC_SNAP_DAMAGE, EncScript_HoOh_TurnClose_Purify
	encjumpifvar CMP_LESS_THAN, 13, 6, EncScript_HoOh_TurnClose_Purify
	enchangehp ENC_TARGET_BOSS, 4, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCHOOHMEND
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_HoOh_SpendFlame
	call EncScript_HoOh_SpendFlame
	@ Purification is what makes statusing Ho-Oh worth doing: every cure is a flame off the rebirth.
	@ It is also a trap with a rhythm - status still ON Ho-Oh when a judgment lands is what makes the
	@ verdict Impure, which Hazes the field and hands the flame straight back.
EncScript_HoOh_TurnClose_Purify:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_HoOh_TurnClose_Income
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_HoOh_TurnClose_DoPurify
	goto EncScript_HoOh_TurnClose_Income
EncScript_HoOh_TurnClose_DoPurify:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCHOOHPURIFY
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_HoOh_SpendFlame
EncScript_HoOh_TurnClose_Income:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_HoOh_TurnClose_Reckon
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_SUN, EncScript_HoOh_TurnClose_Reckon
	call EncScript_HoOh_GainFlame
	@ The turn's closing reckoning, last and once. Everything that moved the flame this turn - a Fire
	@ move, a faint, a verdict, and the three spends above - is announced here as one net reading.
EncScript_HoOh_TurnClose_Reckon:
	call EncScript_HoOh_Reckon
EncScript_HoOh_TurnClose_Done:
	return
	@ 4 -> 3 -> 2 -> 1 -> 4. The gate parks Rite at 1 so this wrap runs on the rebirth turn itself
	@ and the next turn opens on SACRED FLAME. Routed through the reckoning so the guard drop the
	@ rebirth itself caused is announced on the turn it happens.
EncScript_HoOh_TurnClose_Rite:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_HoOh_TurnClose_RiteWrap
	encsubvar 7, 1
	goto EncScript_HoOh_TurnClose_Reckon
EncScript_HoOh_TurnClose_RiteWrap:
	encsetvar 7, 4
	goto EncScript_HoOh_TurnClose_Reckon

// Phase 1 at 50%. Two separate things, and the split is the point. The SUN is ordinary weather the
// player can overwrite, which cuts the flame income - a genuine, repeatable lever. SIDE_STATUS_
// RAINBOW is not weather and encclearscreens cannot touch it (that mask excludes the pledge
// statuses); it doubles Ho-Oh's secondary chances, so Sacred Fire's 50% burn becomes near certain
// and Extrasensory's flinch doubles. The player can dim the rainbow's income, not its danger.
// The guard jump this causes is left to the turn's own reckoning rather than announced here: this
// script can share a checkpoint with a verdict and with TurnClose, and each announcing separately
// is exactly how the same line ends up printed twice.
EncScript_HoOh_TheRainbow::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCHOOHRAINBOW
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAINBOW
	call EncScript_HoOh_ApplySun
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_RAINBOW, 0
	printstring STRINGID_ENCHOOHRAINBOWRULE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_HoOh_GainFlame
	call EncScript_HoOh_GainFlame
	return

// The whole fight in one branch. Four or more flame left and Ho-Oh spends it all on coming back;
// three or less and the fire is not there to spend. Draining it is rewarded with a shorter fight
// and an immediate catch window, so the optimal line is also the fast line - and nobody is ever
// locked out of catching it. Prev doubles as encrewindhp's destination percentage here; ApplyGuard
// overwrites it on its next call, which is why it is written immediately before each rewind.
EncScript_HoOh_RebirthGate::
	printstring STRINGID_ENCHOOHGUTTERS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_HoOh_EndBlessing
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_RAINBOW
	removeweather
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_HoOh_RebirthGate_Ashes
	playanimation BS_OPPONENT1, B_ANIM_RAINBOW
	printstring STRINGID_ENCHOOHREBIRTH
	waitmessage B_WAIT_TIME_LONG
	encsetvar 14, 30
	encrewindhp ENC_TARGET_BOSS, 14
	encsetvar 0, 2   @ Phase 2
	encsetvar 1, 0   @ Flame, permanently
	encsetvar 2, 0   @ Trial: no more judgments
	encsetvar 3, 0   @ Flights
	encsetvar 4, 0   @ Fallen
	encsetvar 7, 1   @ Rite: TurnClose wraps this to 4 on the rebirth turn
	printstring STRINGID_ENCHOOHRITEBEGINS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_HoOh_RebirthGate_Ashes:
	printstring STRINGID_ENCHOOHASHES
	waitmessage B_WAIT_TIME_LONG
	encsetvar 14, 8
	encrewindhp ENC_TARGET_BOSS, 14
	goto EncScript_HoOh_Weakened

// Everything off, and Survive released so the catch window is a real one. The Rainbow is dropped
// explicitly - encclearscreens does not cover it, and a guaranteed Sacred Fire burn during the
// catch window would be miserable. CapTypeEffectiveness and FlatToxicDamage stay on as insurance.
EncScript_HoOh_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Flame
	encsetvar 2, 0   @ Trial
	encsetvar 3, 0   @ Flights
	encsetvar 4, 0   @ Fallen
	encsetvar 5, 0   @ Chosen
	encsetvar 6, 0   @ Grace
	encsetvar 7, 0   @ Rite
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_RAINBOW
	removeweather
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_HoOh_Weakened_Guard
EncScript_HoOh_Weakened_Guard:
	encsetdamagereduction ENC_TARGET_BOSS, 74
	encsetvar 8, 74
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCHOOHWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Ho-Oh drawing breath off its own Sacred Fire. Rise is the shared per-turn income guard.
EncScript_HoOh_FlameGain::
	encsetvar 11, 1   @ Rise
	call EncScript_HoOh_GainFlame
	return

// A fallen Pokemon is worth two flame - and it is what buys Mercy at the next judgment, which
// spends three. Losing one is not free, but it is not only a loss either.
// The tally is unconditional (Toll is this trigger's own guard); only the flame is capped by Rise,
// so a Pokemon that faints to Sacred Fire still counts toward Mercy without paying out twice.
EncScript_HoOh_Fallen::
	encsetvar 15, 1   @ Toll
	encaddvar 4, 1    @ Fallen
	encjumpifvar CMP_NOT_EQUAL, 11, 0, EncScript_HoOh_Fallen_Done
	encsetvar 11, 1   @ Rise
	printstring STRINGID_ENCHOOHFALLEN
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_HoOh_GainFlame
	call EncScript_HoOh_GainFlame
EncScript_HoOh_Fallen_Done:
	return

// Tallies the switch for the Defiant verdict, and sheds any active blessing. The shed half is
// self-disabling because its own test reads Grace, which it clears; the tally half rides Wake.
// Switching already discards stat stages, so refusing the boon needs no extra bookkeeping - it
// costs a turn, and that is the whole trade.
EncScript_HoOh_Flight::
	encsetvar 12, 1   @ Wake
	encaddvar 3, 1    @ Flights
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_HoOh_Flight_Done
	call EncScript_HoOh_ShedBlessing
	printstring STRINGID_ENCHOOHBLESSSHED
	waitmessage B_WAIT_TIME_SHORT
EncScript_HoOh_Flight_Done:
	return

// The two punishing verdicts are also the two that FEED Ho-Oh, so playing badly is not merely
// punished locally - it actively funds the rebirth.
EncScript_HoOh_JudgeImpure::
	printstring STRINGID_ENCHOOHJUDGES
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCHOOHIMPURE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
	normalisebuffs
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	@ The Haze has already taken the blessing's stages, so shed it rather than mirroring it out.
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_HoOh_JudgeImpure_Flame
	call EncScript_HoOh_ShedBlessing
	printstring STRINGID_ENCHOOHBLESSENDS
	waitmessage B_WAIT_TIME_SHORT
EncScript_HoOh_JudgeImpure_Flame:
	call EncScript_HoOh_GainFlame
	call EncScript_HoOh_JudgeDone
	return

EncScript_HoOh_JudgeDefiant::
	printstring STRINGID_ENCHOOHJUDGES
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCHOOHDEFIANT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	call EncScript_HoOh_GainFlame
	call EncScript_HoOh_JudgeDone
	return

// The most expensive thing Ho-Oh can do, and the only one that touches the player's party rather
// than the battle. Three flame is most of a rebirth.
EncScript_HoOh_JudgeMerciful::
	printstring STRINGID_ENCHOOHJUDGES
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCHOOHMERCIFUL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encrevive ENC_TARGET_ALL_FOES, 50
	printstring STRINGID_ENCHOOHREVIVE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_HoOh_SpendFlame
	call EncScript_HoOh_SpendFlame
	call EncScript_HoOh_SpendFlame
	call EncScript_HoOh_JudgeDone
	return

// A blessing replaces whatever was already burning, so EndBlessing runs first and the old one is
// mirrored out honestly rather than silently overwritten.
EncScript_HoOh_JudgeWorthy::
	printstring STRINGID_ENCHOOHJUDGES
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCHOOHWORTHY
	waitmessage B_WAIT_TIME_LONG
	call EncScript_HoOh_EndBlessing
	encjumpifchance 25, EncScript_HoOh_Bless_Swift
	encjumpifchance 33, EncScript_HoOh_Bless_Valor
	encjumpifchance 50, EncScript_HoOh_Bless_Aegis
	@ With the Rainbow up Sacred Fire burns on essentially every connect, so a standing Safeguard is
	@ the single most valuable thing Ho-Oh can hand over - and the one the fight most wants you to
	@ want. It is stripped again when the blessing ends.
	encsetvar 5, 4
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SAFEGUARD, 0
	curestatus BS_PLAYER1
	updatestatusicon BS_PLAYER1
	printstring STRINGID_ENCHOOHBLESSPURITY
	goto EncScript_HoOh_Bless_Done
EncScript_HoOh_Bless_Swift:
	encsetvar 5, 1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 2
	printstring STRINGID_ENCHOOHBLESSSWIFT
	goto EncScript_HoOh_Bless_Done
EncScript_HoOh_Bless_Valor:
	encsetvar 5, 2
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, 1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, 1
	printstring STRINGID_ENCHOOHBLESSVALOR
	goto EncScript_HoOh_Bless_Done
EncScript_HoOh_Bless_Aegis:
	encsetvar 5, 3
	encchangestat ENC_TARGET_ALL_FOES, STAT_DEF, 1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPDEF, 1
	printstring STRINGID_ENCHOOHBLESSAEGIS
EncScript_HoOh_Bless_Done:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encsetvar 6, 3   @ Grace: three turns, each of which bills the blessed Pokemon 7%
	call EncScript_HoOh_SpendFlame
	call EncScript_HoOh_SpendFlame
	call EncScript_HoOh_JudgeDone
	return
