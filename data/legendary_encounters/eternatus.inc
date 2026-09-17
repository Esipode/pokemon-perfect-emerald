// Eternatus, "The Gigantic Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Eternatus_Intro:
// 0 Energy (0-100, forced to 100 entering Phase 2), 1 Phase (0 Alien Core / 1 Eternamax /
// 2 Darkest Day / 3 Spent), 2 NodeState (0 none / 1 live), 3 NodeTimer, 4 CannonState
// (0 none / 1 charging), 5 CannonTimer, 6 CoreState (0 closed / 1 open, Phase 2 only), 7 CoreTimer,
// 8 LastTier (the last ANNOUNCED Energy band: 0 none / 1 Distortion / 2 Unstable / 3 Surge),
// 9 TurnGuard, 10 HandledHit (OnMoveEnd guard: the player's move hit the boss), 11 HandledAtk
// (OnMoveEnd guard: the boss's move resolved).
//
// ENERGY NEVER STOPS CLIMBING (+1/turn, +3/boss-attack, +15 a Node detonation, +20 a Cannon hit)
// and only ever bleeds when the player interrupts something (-20 a Cannon interrupt, -25 a Core
// hit). Its band gates the crit boost only - DamageReduction is a flat 90 baseline the whole fight,
// dropped to 60 only inside a Core window. EncScript_Eternatus_ApplyTierEffects is the single writer
// of both, so a Core window closing always restores exactly what the band would have set anyway.
//
// A PLAYER HIT RESOLVES AGAINST WHICHEVER STATE IS ACTIVE, Core first, then Cannon, then Node -
// they are never simultaneously attackable, so this ordering is just a priority among mutually
// exclusive live states, not a real conflict.
//
// EVERY OnMoveEnd TRIGGER IS SELF-DISABLING through the variable its own script sets as its first
// line, so every branch closes the latch before it does anything else.
//
// Split-countdown rule: NodeTimer/CannonTimer/CoreTimer tick at OnTurnStart and resolve at
// OnTurnEnd, on two different checkpoints, so a timer never chases itself to zero in one dispatch.
//
// Stat stages are written absolutely (push to the raw ceiling with +12, then one corrective delta),
// the Rayquaza idiom, so Dynamax Resonance and the Eternamax stat push land exactly on +2 no matter
// what a Haze or a switch did first.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of both DamageReduction and the crit boost. A Core window overrides the
// reduction to 60; everywhere else it's the flat 90 baseline. Crit turns on at Energy 50+
// (Unstable/Surge) regardless of Core state - the two are independent axes.
EncScript_Eternatus_ApplyTierEffects:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Eternatus_ApplyTierEffects_CoreOpen
	encsetdamagereduction ENC_TARGET_BOSS, 96
	goto EncScript_Eternatus_ApplyTierEffects_Crit
EncScript_Eternatus_ApplyTierEffects_CoreOpen:
	encsetdamagereduction ENC_TARGET_BOSS, 90
EncScript_Eternatus_ApplyTierEffects_Crit:
	encjumpifvar CMP_LESS_THAN, 0, 50, EncScript_Eternatus_ApplyTierEffects_NoCrit
	encsetcrit ENC_TARGET_BOSS, TRUE
	return
EncScript_Eternatus_ApplyTierEffects_NoCrit:
	encsetcrit ENC_TARGET_BOSS, FALSE
	return

// THE BAND CALLOUT (guideline 14). LastTier holds the last ANNOUNCED band; each arm compares it
// against its own literal and stays quiet if that's what was last said, so the line prints only on
// a real crossing - in either direction. Band 0 (under 25) is always silent; there's nothing to
// warn about yet.
EncScript_Eternatus_Band:
	encjumpifvar CMP_LESS_THAN, 0, 25, EncScript_Eternatus_Band_None
	encjumpifvar CMP_LESS_THAN, 0, 50, EncScript_Eternatus_Band_Distortion
	encjumpifvar CMP_LESS_THAN, 0, 75, EncScript_Eternatus_Band_Unstable
	encjumpifvar CMP_EQUAL, 8, 3, EncScript_Eternatus_Band_Done
	encsetvar 8, 3
	printstring STRINGID_ENCETERNATUSTIER3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Eternatus_Band_Unstable:
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Eternatus_Band_Done
	encsetvar 8, 2
	printstring STRINGID_ENCETERNATUSTIER2
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Eternatus_Band_Distortion:
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Eternatus_Band_Done
	encsetvar 8, 1
	printstring STRINGID_ENCETERNATUSTIER1
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Eternatus_Band_None:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Eternatus_Band_Done
	encsetvar 8, 0
EncScript_Eternatus_Band_Done:
	return

// Floors Energy at 100. Called after every addition; silent.
EncScript_Eternatus_EnergyCap:
	encjumpifvar CMP_GREATER_THAN, 0, 100, EncScript_Eternatus_EnergyCap_Clamp
	return
EncScript_Eternatus_EnergyCap_Clamp:
	encsetvar 0, 100
	return

// Floored subtractions - u8 Energy would otherwise wrap past 0 on a big interrupt/hit payout.
EncScript_Eternatus_EnergySub20:
	encjumpifvar CMP_LESS_THAN, 0, 20, EncScript_Eternatus_EnergySub20_Floor
	encsubvar 0, 20
	return
EncScript_Eternatus_EnergySub20_Floor:
	encsetvar 0, 0
	return

EncScript_Eternatus_EnergySub25:
	encjumpifvar CMP_LESS_THAN, 0, 25, EncScript_Eternatus_EnergySub25_Floor
	encsubvar 0, 25
	return
EncScript_Eternatus_EnergySub25_Floor:
	encsetvar 0, 0
	return

// --- Trigger entry points ---

// OnBattleStart. Energy starts at 0, so this just seeds the baseline reduction/crit state.
// No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Eternatus_Intro::
	call EncScript_Eternatus_ApplyTierEffects
	return

// Top of every turn. Leads with flushtextbox - nothing here animates before dialogue, but the
// scaffold is unconditional. TurnGuard is set immediately so no branch out can leave it re-armed.
// Node/Cannon/Core timers tick here; _TurnClose resolves them at the opposite checkpoint.
EncScript_Eternatus_TurnOpen::
	flushtextbox
	encsetvar 9, 1     @ TurnGuard
	encaddvar 0, 1     @ Energy: passing turns alone raise it
	call EncScript_Eternatus_EnergyCap
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Eternatus_TurnOpen_Cannon
	encsubvar 3, 1     @ NodeTimer
EncScript_Eternatus_TurnOpen_Cannon:
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Eternatus_TurnOpen_Core
	encsubvar 5, 1     @ CannonTimer
EncScript_Eternatus_TurnOpen_Core:
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_Eternatus_TurnOpen_Done
	encsubvar 7, 1     @ CoreTimer
EncScript_Eternatus_TurnOpen_Done:
	return

// The player's move landed on the boss. HandledHit is set first so every branch below closes the
// latch. Resolves against whichever of Core/Cannon/Node is live; they're never simultaneous.
EncScript_Eternatus_BossHit::
	encsetvar 10, 1    @ HandledHit
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Eternatus_BossHit_Core
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Eternatus_BossHit_Cannon
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Eternatus_BossHit_Node
	return
EncScript_Eternatus_BossHit_Core:
	call EncScript_Eternatus_EnergySub25
	encsetvar 6, 0     @ CoreState closes early
	encsetvar 7, 0     @ CoreTimer
	call EncScript_Eternatus_ApplyTierEffects
	printstring STRINGID_ENCETERNATUSCORESTRUCK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return
EncScript_Eternatus_BossHit_Cannon:
	call EncScript_Eternatus_EnergySub20
	encsetvar 4, 0     @ CannonState
	encsetvar 5, 0     @ CannonTimer
	printstring STRINGID_ENCETERNATUSCANNONINTERRUPTED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return
EncScript_Eternatus_BossHit_Node:
	encsetvar 2, 0     @ NodeState
	encsetvar 3, 0     @ NodeTimer
	printstring STRINGID_ENCETERNATUSNODEDESTROYED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

// The boss's own move resolved - passive Energy feed. HandledAtk closes the latch immediately.
EncScript_Eternatus_BossAttacked::
	encsetvar 11, 1    @ HandledAtk
	encaddvar 0, 3     @ Energy: boss attacked
	call EncScript_Eternatus_EnergyCap
	return

// THE LEDGER, and the only place Node/Cannon/Core resolve. Phase 3 (Spent) skips everything - the
// fight is already over, just waiting on the ball. Node resolves before Cannon before Core, purely
// to keep the branching linear; none of the three read each other's state.
EncScript_Eternatus_TurnClose::
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Eternatus_TurnClose_Quiet
// --- Node ---
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Eternatus_TurnClose_NodeSpawn
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Eternatus_TurnClose_Cannon
	encsetvar 2, 0
	encsetvar 3, 0
	encaddvar 0, 15
	call EncScript_Eternatus_EnergyCap
	printstring STRINGID_ENCETERNATUSNODEABSORBED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -14, ENC_AMOUNT_PERCENT
	goto EncScript_Eternatus_TurnClose_Cannon
EncScript_Eternatus_TurnClose_NodeSpawn:
	encjumpifvar CMP_LESS_THAN, 0, 75, EncScript_Eternatus_TurnClose_NodeRollLow
	encjumpifchance 50, EncScript_Eternatus_TurnClose_NodeForm
	goto EncScript_Eternatus_TurnClose_Cannon
EncScript_Eternatus_TurnClose_NodeRollLow:
	encjumpifchance 30, EncScript_Eternatus_TurnClose_NodeForm
	goto EncScript_Eternatus_TurnClose_Cannon
EncScript_Eternatus_TurnClose_NodeForm:
	encsetvar 2, 1
	encsetvar 3, 3
	printstring STRINGID_ENCETERNATUSNODEFORMS
	waitmessage B_WAIT_TIME_LONG
// --- Cannon (Phase 1+) ---
EncScript_Eternatus_TurnClose_Cannon:
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Eternatus_TurnClose_Core
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Eternatus_TurnClose_CannonSpawn
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Eternatus_TurnClose_Core
	encsetvar 4, 0
	encsetvar 5, 0
	encaddvar 0, 20
	call EncScript_Eternatus_EnergyCap
	printstring STRINGID_ENCETERNATUSCANNONFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Eternatus_TurnClose_Core
EncScript_Eternatus_TurnClose_CannonSpawn:
	encjumpifvar CMP_LESS_THAN, 0, 75, EncScript_Eternatus_TurnClose_CannonRollLow
	encjumpifchance 35, EncScript_Eternatus_TurnClose_CannonBegin
	goto EncScript_Eternatus_TurnClose_Core
EncScript_Eternatus_TurnClose_CannonRollLow:
	encjumpifchance 20, EncScript_Eternatus_TurnClose_CannonBegin
	goto EncScript_Eternatus_TurnClose_Core
EncScript_Eternatus_TurnClose_CannonBegin:
	encsetvar 4, 1
	encsetvar 5, 2
	printstring STRINGID_ENCETERNATUSCANNONCHARGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
// --- Core (Phase 2 only) ---
EncScript_Eternatus_TurnClose_Core:
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Eternatus_TurnClose_Tier
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_Eternatus_TurnClose_CoreSpawn
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Eternatus_TurnClose_Tier
	encsetvar 6, 0     @ closes untouched
	call EncScript_Eternatus_ApplyTierEffects
	goto EncScript_Eternatus_TurnClose_Tier
EncScript_Eternatus_TurnClose_CoreSpawn:
	encjumpifchance 20, EncScript_Eternatus_TurnClose_CoreOpen
	goto EncScript_Eternatus_TurnClose_Tier
EncScript_Eternatus_TurnClose_CoreOpen:
	encsetvar 6, 1
	encsetvar 7, 2
	call EncScript_Eternatus_ApplyTierEffects
	printstring STRINGID_ENCETERNATUSCOREOPENS
	waitmessage B_WAIT_TIME_LONG
// --- Tier band: Distortion chip, then the callout ---
EncScript_Eternatus_TurnClose_Tier:
	encjumpifvar CMP_LESS_THAN, 0, 25, EncScript_Eternatus_TurnClose_Announce
	encjumpifvar CMP_GREATER_THAN, 0, 49, EncScript_Eternatus_TurnClose_Announce
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
EncScript_Eternatus_TurnClose_Announce:
	call EncScript_Eternatus_ApplyTierEffects
	call EncScript_Eternatus_Band
	encjumpifvar CMP_LESS_THAN, 0, 80, EncScript_Eternatus_TurnClose_Quiet
	printstring STRINGID_ENCETERNATUSTIER4
	waitmessage B_WAIT_TIME_SHORT
EncScript_Eternatus_TurnClose_Quiet:
	encsetvar 9, 0     @ TurnGuard
	encsetvar 10, 0    @ HandledHit
	encsetvar 11, 0    @ HandledAtk
	return

// --- Phase transitions ---

// 60% - ETERNAMAX. TIER3's "reality bends" line doubles as the formchange announcement - the same
// surge language fits both. Sp. Atk pushed to the raw ceiling then landed at exactly +2.
EncScript_Eternatus_Eternamax::
	printstring STRINGID_ENCETERNATUSTIER3
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_ETERNATUS_ETERNAMAX, EncScript_Eternatus_Eternamax_Stats, B_ANIM_ULTRA_BURST
EncScript_Eternatus_Eternamax_Stats:
	encsetvar 1, 1     @ Phase
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -4
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Eternatus_ApplyTierEffects
	call EncScript_Eternatus_Band
	return

// 30% - THE DARKEST DAY. Energy is forced to the top band immediately - the whole final phase runs
// hot. Dynamax Resonance is the one-time +2 Attack gift to whoever's on the field (guideline 7 cap).
EncScript_Eternatus_DarkestDay::
	printstring STRINGID_ENCETERNATUSDARKESTDAY
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 2     @ Phase
	encsetvar 0, 100   @ Energy
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -4
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	call EncScript_Eternatus_ApplyTierEffects
	call EncScript_Eternatus_Band
	return

// SPENT, and the catch window (guideline 10). Reached only once (Once, OnEnter at both
// checkpoints), so the first selection disables the other and Phase 3 closes the condition behind
// it. Teardown mirrors the Properties: block plus everything Phase 2 raised.
EncScript_Eternatus_Spent::
	encsetvar 1, 3     @ Phase
	printstring STRINGID_ENCETERNATUSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 0, 0     @ Energy
	encsetvar 2, 0     @ NodeState
	encsetvar 3, 0     @ NodeTimer
	encsetvar 4, 0     @ CannonState
	encsetvar 5, 0     @ CannonTimer
	encsetvar 6, 0     @ CoreState
	encsetvar 7, 0     @ CoreTimer
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Eternatus_Spent_NoStats
EncScript_Eternatus_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	return
