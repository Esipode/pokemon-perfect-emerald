#include "config/battle.h"
#include "constants/global.h"
#include "constants/battle.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_string_ids.h"
#include "constants/battle_encounter.h"
#include "constants/battle_anim.h"
#include "constants/pokemon.h"
	.include "asm/macros.inc"
	.include "asm/macros/battle_script.inc"
	.include "constants/constants.inc"

	.section script_data, "aw", %progbits

// Engine-owned shim: called via BattleScriptExecute from a non-script callback, with the
// encounter script itself invoked via BattleScriptCall. All encounter scripts end with
// `return`, which unwinds here; this is the only encounter script allowed to end2.
BattleScript_EncounterCheckpointEnd2::
	end2

EncScript_TestBattleStart::
	printstring STRINGID_EMPTYSTRING3
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TestTurnEnd::
	printstring STRINGID_EMPTYSTRING3
	waitmessage B_WAIT_TIME_LONG
	return

// Shared by tests for checkpoints that don't otherwise need a dedicated script (Stage 09+).
EncScript_TestGeneric::
	printstring STRINGID_EMPTYSTRING3
	waitmessage B_WAIT_TIME_LONG
	return

// Exercise the sENCOUNTER_VAR addressing convention (Stage 14). Var 0 is whatever the test is
// setting/branching/accumulating on; var 1 records a branch outcome. See test/battle/encounter/
// variables.c for how each script is used.

EncScript_TestSetVar::
	encsetvar 0, 1
	return

EncScript_TestAddVar::
	encaddvar 0, 1
	return

EncScript_TestSeedVarHigh::
	encsetvar 0, 5
	return

EncScript_TestSeedVarLow::
	encsetvar 0, 2
	return

EncScript_TestBranch::
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_TestBranch_High
	encsetvar 1, 0
	return
EncScript_TestBranch_High:
	encsetvar 1, 1
	return

EncScript_TestCallSub::
	call EncScript_TestSubroutine
	encsetvar 1, 1   @ proves control returned here, not just that the subroutine ran
	return
EncScript_TestSubroutine:
	encsetvar 0, 1
	return

// Stage 15 example encounters (outline Sec33/Sec34), built through the authoring path only - no
// encounter-specific C beyond the reusable commands in asm/macros/battle_script.inc. Var 0 in each
// is the encounter's own phase/state flag, per src/data/battle_encounters.encounter.

// outline Sec33: a boss that grows a defensive barrier the first time it drops to half HP.
// "ADD_BARRIER" isn't a new mechanic (architecture doc Sec1.2) - it's Defense/Sp. Def stat stages.
EncScript_LegendaryBarrier_PhaseTransition::
	encsetvar 0, 2   @ phase = 2
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCLEGENDARYGATHERSSTRENGTH
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 3
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 3
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT   @ percent, not raw HP: the boss's level (and so its max HP) comes from the Level: property
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL   @ stand-in glow - no general-purpose "shield" anim exists outside move-specific ones
	printstring STRINGID_ENCMYSTERIOUSBARRIERSURROUNDS
	waitmessage B_WAIT_TIME_LONG
	return

// Weakened phase: the boss's damage reduction and its immunities drop away, and the ball block the
// Properties: block set at battle start is lifted so the player can attempt the catch.
EncScript_LegendaryBarrier_Weakened::
	encsetvar 0, 3   @ phase = 3
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -25, ENC_AMOUNT_PERCENT
	encsetballs ENC_BALLS_ALLOWED
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCLEGENDARYWEAKENED
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	return

// outline Sec34: a trainer who Mega Evolves their boss once, on a fixed turn, outside the normal
// player-facing gimmick flow.
EncScript_TrainerMega_Reveal::
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCTRAINERPUSHEDTHISFAR
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTRAINERSHOWTRUEPOWER
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	encmegaevolve ENC_TARGET_BOSS, EncScript_TrainerMega_Reveal_Done
	encsetvar 0, 1   @ mega_evolved = true
EncScript_TrainerMega_Reveal_Done:
	return

// Stage 15 command tests (test/battle/encounter/commands.c). Each exercises one command in
// isolation through the real interpreter, the same way Stage 14's variables scripts do.

EncScript_TestChangeHpDamage::
	enchangehp ENC_TARGET_BOSS, -30
	return

EncScript_TestChangeHpHeal::
	enchangehp ENC_TARGET_BOSS, 20
	return

EncScript_TestChangeHpAllFoes::
	enchangehp ENC_TARGET_ALL_FOES, -15
	return

EncScript_TestChangeStat::
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 2
	return

EncScript_TestChangeStatAllFoes::
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, -1
	return

EncScript_TestMegaEvolve::
	encmegaevolve ENC_TARGET_BOSS, EncScript_TestMegaEvolve_Done
	encsetvar 0, 1
EncScript_TestMegaEvolve_Done:
	return

// Stage 16 example encounter: a boss whose escalation is spread across three checkpoints instead
// of one, each phase gated by the same Var(Phase) the previous phase's script advances.

EncScript_StormHerald_Intro::
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCSTORMHERALDINTRO
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	return

// Phase 1 -> 2: first time the boss drops to half HP, it heals a portion back and its Sp. Atk rises.
EncScript_StormHerald_Surge::
	encsetvar 0, 2   @ phase = 2
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCSTORMHERALDSURGE
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	enchangehp ENC_TARGET_BOSS, 60
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// Phase 2 -> 3: once the surge has happened and HP falls further, a desperation Attack/Speed boost.
EncScript_StormHerald_Desperation::
	encsetvar 0, 3   @ phase = 3
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCSTORMHERALDDESPERATION
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// Articuno, "The Frozen Battlefield" (src/data/battle_encounters.encounter). Var indices, pinned by
// the always-true Conditions on EncScript_Articuno_Intro:
// 0 Phase, 1 Barrier, 2 Plummet, 3 Frost, 4 Chill, 5 FrostLock, 6 Answered, 7 LastBlunt,
// 8 IceGuard, 9 FireGuard, 10 SwitchGuard. The three *Guard vars are set to 1 the first time their
// OnMoveEnd/OnSwitchIn trigger fires and cleared by EncScript_Articuno_TurnReset at end of turn,
// so an Event condition that stays true through the re-evaluation loop can't fire twice per event.
//
// The core loop: Articuno's Ice moves raise Frost (EncScript_Articuno_IceMove); the player's Fire
// moves lower it (EncScript_Articuno_FireResponse); while Frost >= 1 the Ice Barrier regrows every
// turn (EncScript_Articuno_BarrierRaise), at a strength that scales with Frost, until a damaging
// hit shatters it (EncScript_Articuno_BarrierShatter), unleashing a shockwave scaled by the
// current Frost level.
// Absolute Zero (EncScript_Articuno_AbsoluteZero) locks Frost in place - freezing the barrier at
// its current strength for the rest of the fight - and repurposes the Fire answer into resolving a
// recurring "temperature plummets" timer instead.

EncScript_Articuno_Intro::
	printstring STRINGID_ENCARTICUNOINTRO
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Articuno_PhaseFrozenField::
	encsetvar 0, 1   @ Phase = 1 (Frozen Battlefield)
	printstring STRINGID_ENCARTICUNOPHASE1
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	return

EncScript_Articuno_AbsoluteZero::
	encsetvar 0, 2   @ Phase = 2 (Absolute Zero)
	encsetvar 5, 1   @ FrostLock = true
	printstring STRINGID_ENCARTICUNOABSOLUTEZERO
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SNOW, 0
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 1
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 10, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 10, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_SHEER_COLD
	waitanimation
	@ The barrier is now fixed at whatever strength the frozen Frost level supports.
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Articuno_AbsoluteZero_SealNone
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Articuno_AbsoluteZero_SealThin
	printstring STRINGID_ENCARTICUNOBARRIERSEALEDSOLID
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Articuno_AbsoluteZero_SealThin:
	printstring STRINGID_ENCARTICUNOBARRIERSEALEDTHIN
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Articuno_AbsoluteZero_SealNone:
	printstring STRINGID_ENCARTICUNOBARRIERSEALEDNONE
	waitmessage B_WAIT_TIME_LONG
	return

// Fires when the player lands a damaging hit on Articuno while the barrier is up (Var(Barrier)==1
// is the trigger's own guard). Restores the phase-appropriate base reduction, then unleashes a
// shockwave scaled by the current Frost level before it's spent decrementing on the way out.
EncScript_Articuno_BarrierShatter::
	encsetvar 1, 0   @ Barrier = false
	printstring STRINGID_ENCARTICUNOBARRIERSHATTERS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_ICICLE_CRASH
	waitanimation
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Articuno_BarrierShatter_Weakened
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Articuno_BarrierShatter_Locked
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encjumpifvar CMP_EQUAL, 7, 70, EncScript_Articuno_BarrierShatter_Shockwave
	encsetvar 7, 70   @ LastBlunt: re-announce the absorption whenever the barrier's tier changes
	goto EncScript_Articuno_BarrierShatter_Blunted
EncScript_Articuno_BarrierShatter_Locked:
	@ Frost is frozen here (Frozen Domain / Absolute Zero); the between-barrier floor tracks the
	@ level it locked at, so a diligent player who capped Frost low keeps Articuno soft.
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Articuno_BarrierShatter_LockedThin
	encsetdamagereduction ENC_TARGET_BOSS, 85
	goto EncScript_Articuno_BarrierShatter_LockedAnnounce
EncScript_Articuno_BarrierShatter_LockedThin:
	encsetdamagereduction ENC_TARGET_BOSS, 80
EncScript_Articuno_BarrierShatter_LockedAnnounce:
	encjumpifvar CMP_EQUAL, 7, 85, EncScript_Articuno_BarrierShatter_Shockwave
	encsetvar 7, 85
EncScript_Articuno_BarrierShatter_Blunted:
	printstring STRINGID_ENCARTICUNOBARRIERBLUNTED
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Articuno_BarrierShatter_Shockwave
EncScript_Articuno_BarrierShatter_Weakened:
	encsetdamagereduction ENC_TARGET_BOSS, 0
EncScript_Articuno_BarrierShatter_Shockwave:
	printstring STRINGID_ENCARTICUNOSHOCKWAVE
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Articuno_BarrierShatter_Shock0
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Articuno_BarrierShatter_Shock1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Articuno_BarrierShatter_Shock2
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Articuno_BarrierShatter_Shock3
	enchangehp ENC_TARGET_ALL_FOES, -15, ENC_AMOUNT_PERCENT   @ Frost 4
	return
EncScript_Articuno_BarrierShatter_Shock0:
	enchangehp ENC_TARGET_ALL_FOES, -3, ENC_AMOUNT_PERCENT
	return
EncScript_Articuno_BarrierShatter_Shock1:
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Articuno_BarrierShatter_Shock2:
	enchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Articuno_BarrierShatter_Shock3:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return

// Fires whenever Articuno itself uses an Ice move (Event.MoveType == TYPE_ICE). While Frost is
// locked, only Absolute Zero's plummet timer still listens to it; otherwise it raises Frost and
// applies that tier's effect, capping at 4 (Frozen Domain).
EncScript_Articuno_IceMove::
	encsetvar 8, 1   @ IceGuard: one shot per Ice move; TurnReset clears it
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Articuno_IceMove_Locked
	encjumpifvar CMP_LESS_THAN, 3, 4, EncScript_Articuno_IceMove_Raise
	return   @ Frost already at cap; FrostLock should have engaged by now
EncScript_Articuno_IceMove_Locked:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Articuno_IceMove_NoOp   @ only Absolute Zero re-arms the timer
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Articuno_IceMove_NoOp   @ timer already armed or resolving
	encsetvar 2, 2   @ Plummet = armed
	encsetvar 6, 0   @ Answered = false
	printstring STRINGID_ENCARTICUNOTEMPPLUMMETS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_HAZE
	waitanimation
	return
EncScript_Articuno_IceMove_NoOp:
	return
EncScript_Articuno_IceMove_Raise:
	encaddvar 3, 1   @ Frost += 1
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Articuno_IceMove_Frost1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Articuno_IceMove_Frost2
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Articuno_IceMove_Frost3
	goto EncScript_Articuno_IceMove_Frost4
EncScript_Articuno_IceMove_Frost1:
	encsetweather BATTLE_WEATHER_SNOW, 0
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	printstring STRINGID_ENCARTICUNOFROST1
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Articuno_IceMove_Frost2:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Articuno_IceMove_Frost2Done   @ Chill already applied
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encsetvar 4, 1   @ Chill = true
	printstring STRINGID_ENCARTICUNOFROST2
	waitmessage B_WAIT_TIME_SHORT
EncScript_Articuno_IceMove_Frost2Done:
	return
EncScript_Articuno_IceMove_Frost3:
	printstring STRINGID_ENCARTICUNOFROST3
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Articuno_IceMove_Frost4:
	encsetvar 5, 1   @ FrostLock = true
	printstring STRINGID_ENCARTICUNOFROZENDOMAIN
	waitmessage B_WAIT_TIME_LONG
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 25, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Articuno_IceMove_Frost4BarrierUp   @ barrier already covers this - don't double-apply
	encsetdamagereduction ENC_TARGET_BOSS, 85
EncScript_Articuno_IceMove_Frost4BarrierUp:
	playmoveanimation MOVE_SHEER_COLD
	waitanimation
	return

// Fires whenever the player uses a Fire move, hit or miss, damaging or status - deliberately loose
// (guideline: the player's counterplay lever should feel generous and unambiguous).
EncScript_Articuno_FireResponse::
	encsetvar 9, 1   @ FireGuard: one shot per Fire move; TurnReset clears it
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Articuno_FireResponse_Melt   @ Plummet == resolving: this answers the timer
	encsetvar 6, 1   @ Answered = true
EncScript_Articuno_FireResponse_Melt:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Articuno_FireResponse_Done   @ FrostLock: Fire no longer melts Frost
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Articuno_FireResponse_Done   @ nothing to melt
	encsubvar 3, 1   @ Frost -= 1
	printstring STRINGID_ENCARTICUNOFLAMESPUSHBACK
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_WILL_O_WISP
	waitanimation
	encjumpifvar CMP_NOT_EQUAL, 3, 1, EncScript_Articuno_FireResponse_CheckClear
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Articuno_FireResponse_Done
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 1
	encsetvar 4, 0   @ Chill = false
	printstring STRINGID_ENCARTICUNOCHILLLIFTS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Articuno_FireResponse_CheckClear:
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Articuno_FireResponse_Done
	removeweather
	printstring STRINGID_ENCARTICUNOSNOWCLEARS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Articuno_FireResponse_Done:
	return

// Articuno shrugs off any non-volatile status at the top of its turn - sleep-locking a scripted
// boss is the single biggest trivializer, and this is the encounter's answer to it (other statuses
// are left to stick; see the encounter's design notes for why this one is purged).
EncScript_Articuno_ShakeOff::
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Articuno_ShakeOff_Wake
	return
EncScript_Articuno_ShakeOff_Wake:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCARTICUNOSHRUGSOFFSLEEP
	waitmessage B_WAIT_TIME_SHORT
	return

EncScript_Articuno_PlummetTick::
	encsetvar 2, 1   @ Plummet = resolving
	printstring STRINGID_ENCARTICUNOCOLDDEEPENS
	waitmessage B_WAIT_TIME_SHORT
	return

// While Frost >= 1 and the barrier isn't already up, it regrows every turn - the crux of the loop.
// Barrier strength tracks Frost: Fire pressure that holds Frost low keeps the regrown barrier thin
// and barely above the phase floor. When Frost locks (hits 4, or Absolute Zero begins) it freezes
// at whatever level the player left it, so the barrier keeps regrowing at that fixed strength -
// diligent Fire pressure in phase 1 carries through the rest of the fight.
EncScript_Articuno_BarrierRaise::
	encsetvar 1, 1   @ Barrier = true
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Articuno_BarrierRaise_Frost1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Articuno_BarrierRaise_Frost2
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Articuno_BarrierRaise_Frost3
	encsetdamagereduction ENC_TARGET_BOSS, 95   @ Frost 4 (Frozen Domain)
	goto EncScript_Articuno_BarrierRaise_Solid
EncScript_Articuno_BarrierRaise_Frost3:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	goto EncScript_Articuno_BarrierRaise_Solid
EncScript_Articuno_BarrierRaise_Frost2:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	goto EncScript_Articuno_BarrierRaise_Thin
EncScript_Articuno_BarrierRaise_Frost1:
	encsetdamagereduction ENC_TARGET_BOSS, 84
EncScript_Articuno_BarrierRaise_Thin:
	printstring STRINGID_ENCARTICUNOBARRIERTHIN
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MIST
	waitanimation
	return
EncScript_Articuno_BarrierRaise_Solid:
	printstring STRINGID_ENCARTICUNOBARRIERUP
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MIST
	waitanimation
	return

// A switch-in toll: heavy Frost bites the newcomer directly, and a lingering Chill re-applies its
// Speed drop (stat stages reset on switch, so this is the only way it reaches a fresh Pokemon).
EncScript_Articuno_FrozenGround::
	encsetvar 10, 1   @ SwitchGuard: one shot per switch-in; TurnReset clears it
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Articuno_FrozenGround_CheckChill
	printstring STRINGID_ENCARTICUNOFROZENGROUND
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_SELF, -12, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_HAIL_CONTINUES
EncScript_Articuno_FrozenGround_CheckChill:
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Articuno_FrozenGround_Done
	encchangestat ENC_TARGET_SELF, STAT_SPEED, -1
EncScript_Articuno_FrozenGround_Done:
	return

EncScript_Articuno_Weakened::
	encsetvar 0, 3   @ Phase = 3 (Weakened)
	encsetvar 1, 0   @ Barrier = false
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -1
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCARTICUNOWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Split across two checkpoints on purpose (OnTurnStart ticks, OnTurnEnd resolves) so a real turn
// passes in between instead of the countdown chasing itself through both states in one dispatch.
EncScript_Articuno_PlummetResolve::
	encsetvar 2, 0   @ Plummet = idle
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Articuno_PlummetResolve_Answered
	printstring STRINGID_ENCARTICUNOFIELDFREEZES
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_HAIL_CONTINUES
	return
EncScript_Articuno_PlummetResolve_Answered:
	printstring STRINGID_ENCARTICUNOFLAMESHOLD
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, -5, ENC_AMOUNT_PERCENT
	return

// Clears the per-event guards at end of turn so IceMove/FireResponse/FrozenGround can fire again
// next turn. In a singles fight each side acts once per turn, so "once per turn" == "once per move".
EncScript_Articuno_TurnReset::
	encsetvar 8, 0    @ IceGuard
	encsetvar 9, 0    @ FireGuard
	encsetvar 10, 0   @ SwitchGuard
	return

// Zapdos, "Storm Overload" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Zapdos_Intro:
// 0 Phase, 1 Charge, 2 Overload, 3 Crash, 4 Mark, 5 Answered, 6 Tick, 7 LastGuard,
// 8 TurnGuard, 9 HitGuard, 10 BossGuard, 11 SwitchGuard, 12 TickGuard.
//
// Charge (0-4) is the whole fight: it drives the guard/weather ladder (ApplyTier), Zapdos's Sp. Atk
// (one stage per Charge), and the lightning-strike loop (Storm). It rises from Zapdos's Electric
// moves (BossElectric), the player's Electric moves (ElectricHit), a 25% roll on any other hit
// (AnyHit) and every 3rd turn (TurnOpen); it falls when a Ground move earths it out - by 2
// (GroundHit) - or when a shed sleep/freeze costs it 1 (TurnOpen). At Charge 4 it OVERLOADS
// (EnterOverload): a strike every turn for three turns, then BURNS OUT (Burnout) - Charge to 0,
// self-damage, guard collapses for one turn (CrashEnd). Below 25% HP it locks into permanent
// overload (LastStand); below 10% while there, the storm dies and it becomes catchable (Weakened).

// --- Shared subroutines (call/return; nesting stays <= 2 here) ---

// Sole owner of the Charge-driven guard/weather ladder. Overload, the burnout crash, and the
// Phase 1/2 scripts each own the reduction themselves, so this returns untouched while any of them
// is active. Re-announces the reduction whenever the tier actually changes, in either direction.
EncScript_Zapdos_ApplyTier:
	encjumpifvar CMP_GREATER_THAN, 2, 0, EncScript_Zapdos_ApplyTier_Done   @ Overload owns the tier
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Zapdos_ApplyTier_Done   @ burnout crash owns the tier
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Zapdos_ApplyTier_Done   @ Last Stand / Weakened own the tier
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Zapdos_ApplyTier_Severe
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Zapdos_ApplyTier_Charged
	encsetdamagereduction ENC_TARGET_BOSS, 80
	removeweather
	encjumpifvar CMP_EQUAL, 7, 80, EncScript_Zapdos_ApplyTier_Done
	encjumpifvar CMP_LESS_THAN, 7, 80, EncScript_Zapdos_ApplyTier_CalmHarden
	encsetvar 7, 80
	printstring STRINGID_ENCZAPDOSGUARDSLACKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_CalmHarden:
	encsetvar 7, 80
	printstring STRINGID_ENCZAPDOSGUARDHARDENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_Charged:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	removeweather
	encjumpifvar CMP_EQUAL, 7, 85, EncScript_Zapdos_ApplyTier_Done
	encjumpifvar CMP_LESS_THAN, 7, 85, EncScript_Zapdos_ApplyTier_ChargedHarden
	encsetvar 7, 85
	printstring STRINGID_ENCZAPDOSGUARDSLACKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_ChargedHarden:
	encsetvar 7, 85
	printstring STRINGID_ENCZAPDOSGUARDHARDENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Zapdos_ApplyTier_Severe:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetweather BATTLE_WEATHER_RAIN, 0
	encjumpifvar CMP_EQUAL, 7, 90, EncScript_Zapdos_ApplyTier_Done
	encsetvar 7, 90
	printstring STRINGID_ENCZAPDOSGUARDHARDENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_ApplyTier_Done:
	return

// Charge +1 (capped at 4), one Sp. Atk stage per Charge, then the tier effect for the new value.
// Charge 4 falls through to EnterOverload.
EncScript_Zapdos_GainCharge:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Zapdos_GainCharge_Done   @ already at cap (4 = Overload)
	encaddvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Zapdos_GainCharge_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Zapdos_GainCharge_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Zapdos_GainCharge_3
	goto EncScript_Zapdos_EnterOverload
EncScript_Zapdos_GainCharge_1:
	printstring STRINGID_ENCZAPDOSSPARKS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_CHARGE
	waitanimation
	call EncScript_Zapdos_ApplyTier
	return
EncScript_Zapdos_GainCharge_2:
	printstring STRINGID_ENCZAPDOSCHARGE2
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_ApplyTier
	return
EncScript_Zapdos_GainCharge_3:
	printstring STRINGID_ENCZAPDOSSEVERESTORM
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Zapdos_ApplyTier
	playmoveanimation MOVE_RAIN_DANCE
	waitanimation
EncScript_Zapdos_GainCharge_Done:
	return

// Charge -1 (floored at 0), giving back one Sp. Atk stage, then the tier effect. Used by the
// sleep/freeze shed; the Ground lever uses LoseChargeBig below.
EncScript_Zapdos_LoseCharge:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Zapdos_LoseCharge_Nothing
	encsubvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	printstring STRINGID_ENCZAPDOSDISCHARGES
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	call EncScript_Zapdos_ApplyTier
	return
EncScript_Zapdos_LoseCharge_Nothing:
	printstring STRINGID_ENCZAPDOSNOTHINGTOEARTH
	waitmessage B_WAIT_TIME_SHORT
	return

// Ground move: Charge -2 (floored at 0), Sp. Atk given back to match, one message.
EncScript_Zapdos_LoseChargeBig:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Zapdos_LoseCharge_Nothing
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Zapdos_LoseChargeBig_One
	encsubvar 1, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -2
	goto EncScript_Zapdos_LoseChargeBig_Announce
EncScript_Zapdos_LoseChargeBig_One:
	encsubvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
EncScript_Zapdos_LoseChargeBig_Announce:
	printstring STRINGID_ENCZAPDOSDISCHARGES
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	call EncScript_Zapdos_ApplyTier
	return

// Charge 4: three turns of huge offense and an every-turn strike, on a soft (70) guard.
EncScript_Zapdos_EnterOverload:
	encsetvar 2, 4   @ Overload duration: OverloadTick steps 4->3->2->1 (3 strikes), then Burnout at 1
	encsetweather BATTLE_WEATHER_RAIN, 0
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encsetdamagereduction ENC_TARGET_BOSS, 70
	encsetvar 7, 70
	printstring STRINGID_ENCZAPDOSOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCZAPDOSUNSTABLE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ZAP_CANNON
	waitanimation
	return

// --- Trigger scripts ---

EncScript_Zapdos_Intro::
	encsetvar 7, 80   @ LastGuard: seed to the Properties reduction so the first tier change reads right
	printstring STRINGID_ENCZAPDOSINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Runs once at the top of every turn (TurnGuard gate): clears the per-turn re-entry guards, sheds
// sleep/freeze at the cost of a Charge, and ticks the every-3rd-turn passive charge. The shed and
// the passive tick are storm-phase behaviour; once Weakened the storm is dead and neither runs.
EncScript_Zapdos_TurnOpen::
	encsetvar 8, 1    @ TurnGuard
	encsetvar 9, 0    @ HitGuard
	encsetvar 10, 0   @ BossGuard
	encsetvar 11, 0   @ SwitchGuard
	encsetvar 12, 0   @ TickGuard
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zapdos_TurnOpen_Done   @ Weakened: storm is over
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Zapdos_TurnOpen_Shed
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Zapdos_TurnOpen_Shed
	goto EncScript_Zapdos_TurnOpen_Tick
EncScript_Zapdos_TurnOpen_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCZAPDOSSHAKESITOFF
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_THUNDER_SHOCK
	waitanimation
	encjumpifvar CMP_GREATER_THAN, 2, 0, EncScript_Zapdos_TurnOpen_Tick   @ overloaded: cured, but overload owns the tier
	call EncScript_Zapdos_LoseCharge
EncScript_Zapdos_TurnOpen_Tick:
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Zapdos_TurnOpen_Done   @ passive charge only cycles in the storm phase
	encaddvar 6, 1   @ Tick
	encjumpifvar CMP_LESS_THAN, 6, 3, EncScript_Zapdos_TurnOpen_Done
	encsetvar 6, 0
	printstring STRINGID_ENCZAPDOSSTORMBUILDS
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_GainCharge
EncScript_Zapdos_TurnOpen_Done:
	return

// Overload -> 1 is the sentinel Burnout keys on. Charge back to 0, the +6 from charging unwound,
// weather gone, a chunk of self-damage, and a one-turn 60 guard while Crash is set.
EncScript_Zapdos_Burnout::
	encsetvar 2, 0   @ Overload off
	encsetvar 1, 0   @ Charge 0
	encsetvar 6, 0   @ Tick 0
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6
	removeweather
	encsetdamagereduction ENC_TARGET_BOSS, 60
	encsetvar 7, 60
	encsetvar 3, 1   @ Crash: CrashEnd closes this window at end of turn
	printstring STRINGID_ENCZAPDOSBURNSOUT
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCZAPDOSREELING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EXPLOSION
	waitanimation
	enchangehp ENC_TARGET_BOSS, -10, ENC_AMOUNT_PERCENT
	return

// Resolves a pending strike, then lays the next one while Charge >= 3 and not overloaded. Doing
// both here keeps the cadence continuous and the OnTurnStart budget at 3.
EncScript_Zapdos_Storm::
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Zapdos_Storm_Lay
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Zapdos_Storm_Grounded
	printstring STRINGID_ENCZAPDOSLIGHTNINGSTRIKE
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER
	waitanimation
	goto EncScript_Zapdos_Storm_ClearMark
EncScript_Zapdos_Storm_Grounded:
	printstring STRINGID_ENCZAPDOSBOLTGROUNDED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_Storm_ClearMark:
	encsetvar 4, 0   @ Mark cleared
EncScript_Zapdos_Storm_Lay:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Zapdos_Storm_Done       @ Charge < 3: storm isn't marking
	encjumpifvar CMP_GREATER_THAN, 2, 0, EncScript_Zapdos_Storm_Done    @ overloaded: its own strike covers this
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Zapdos_Storm_Done    @ a mark is already in flight
	encsetvar 4, 2   @ Mark: laid this turn; TurnClose steps it to 1
	encsetvar 5, 0   @ Answered
	printstring STRINGID_ENCZAPDOSMARKS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_LOCK_ON
	waitanimation
EncScript_Zapdos_Storm_Done:
	return

// Below 25% HP: permanent overload, guard back up to 82, no more burnout window to exploit.
EncScript_Zapdos_LastStand::
	encsetvar 0, 1   @ Phase 1 (Last Stand)
	encsetvar 3, 0   @ Crash off
	encsetvar 1, 4   @ Charge 4 (Overload)
	encsetvar 2, 4   @ Overload duration: not stepped down while Phase >= 1
	encsetweather BATTLE_WEATHER_RAIN, 0
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetvar 7, 82
	printstring STRINGID_ENCZAPDOSLASTSTAND
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_THUNDER
	waitanimation
	return

// Below 10% HP while in Last Stand: the storm collapses, every guard and immunity drops, and
// Poke Balls are unblocked. The only catch window in the fight.
EncScript_Zapdos_Weakened::
	encsetvar 0, 2   @ Phase 2 (Weakened)
	encsetvar 2, 0   @ Overload off - a strike resolving after the storm died would be incoherent
	encsetvar 4, 0   @ Mark cleared
	encsetvar 1, 0   @ Charge 0
	encsetvar 3, 0   @ Crash off
	removeweather
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	encsetvar 7, 0   @ LastGuard
	printstring STRINGID_ENCZAPDOSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Zapdos itself uses an Electric move: Charge +1.
EncScript_Zapdos_BossElectric::
	encsetvar 10, 1   @ BossGuard
	call EncScript_Zapdos_GainCharge
	return

// The player's Electric move lands on Zapdos: Charge +1, and it's told outright the first time so
// feeding the boss with your own attack doesn't read as a bug.
EncScript_Zapdos_ElectricHit::
	encsetvar 9, 1   @ HitGuard
	printstring STRINGID_ENCZAPDOSDRINKSITIN
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_GainCharge
	return

// Any other move aimed at Zapdos: 25% chance to feed Charge.
EncScript_Zapdos_AnyHit::
	encsetvar 9, 1   @ HitGuard
	encjumpifchance 25, EncScript_Zapdos_AnyHit_Feed
	return
EncScript_Zapdos_AnyHit_Feed:
	printstring STRINGID_ENCZAPDOSFEEDSONIMPACT
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_GainCharge
	return

// A Ground move hits Zapdos. It can't damage Electric/Flying, so this turn deals no damage - that
// is the cost of the lever. It answers a pending mark, and against a fully overloaded Zapdos it
// forces an immediate discharge (remaining strikes cancelled, burnout pulled forward); otherwise
// it earths off 2 Charge.
EncScript_Zapdos_GroundHit::
	encsetvar 9, 1   @ HitGuard
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Zapdos_GroundHit_Answer
	goto EncScript_Zapdos_GroundHit_Resolve
EncScript_Zapdos_GroundHit_Answer:
	encsetvar 5, 1   @ Answered
EncScript_Zapdos_GroundHit_Resolve:
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Zapdos_GroundHit_Earth   @ Last Stand: overload is permanent
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Zapdos_GroundHit_Earth
	encsetvar 2, 1   @ Overload -> burnout sentinel; Burnout fires next turn start
	printstring STRINGID_ENCZAPDOSEARTHSOUT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MUD_SLAP
	waitanimation
	return
EncScript_Zapdos_GroundHit_Earth:
	call EncScript_Zapdos_LoseChargeBig
	return

// A fresh Pokemon enters: it answers a pending mark, and while Charge >= 3 the charged air tolls it
// for HP - cheaper than eating a strike, but not free.
EncScript_Zapdos_SwitchIn::
	encsetvar 11, 1   @ SwitchGuard
	encjumpifvar CMP_LESS_THAN, 4, 1, EncScript_Zapdos_SwitchIn_Toll
	encsetvar 5, 1   @ Answered
	printstring STRINGID_ENCZAPDOSLOSESITSTARGET
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_SwitchIn_Toll:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Zapdos_SwitchIn_Done
	printstring STRINGID_ENCZAPDOSCHARGEDAIR
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_SELF, -8, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER_SHOCK
	waitanimation
EncScript_Zapdos_SwitchIn_Done:
	return

// Overload strike (OnTurnEnd), then step the counter down - except in Last Stand, where overload
// is permanent. TickGuard stops the step-down from chasing itself through the counter in one pass.
EncScript_Zapdos_OverloadTick::
	encsetvar 12, 1   @ TickGuard
	printstring STRINGID_ENCZAPDOSSTRIKE
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_THUNDER
	waitanimation
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Zapdos_OverloadTick_Done   @ Last Stand: no step-down
	encsubvar 2, 1
EncScript_Zapdos_OverloadTick_Done:
	return

// Closes the one-turn post-burnout soft window: guard back to the Charge-0 tier (60 -> 80).
EncScript_Zapdos_CrashEnd::
	encsetvar 3, 0   @ Crash off
	printstring STRINGID_ENCZAPDOSSTEADIES
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Zapdos_ApplyTier
	return

// OnTurnEnd, priority 90 - runs last. Clears TurnGuard and steps a mark laid this turn (2 -> 1) so
// its warning lasts a full turn before EncScript_Zapdos_Storm resolves it next turn start.
EncScript_Zapdos_TurnClose::
	encsetvar 8, 0   @ TurnGuard
	encjumpifvar CMP_NOT_EQUAL, 4, 2, EncScript_Zapdos_TurnClose_Done
	encsetvar 4, 1
	printstring STRINGID_ENCZAPDOSAIRCRACKLES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Zapdos_TurnClose_Done:
	return

// Moltres, "The Everlasting Flame" (src/data/battle_encounters.encounter). Var indices, pinned by
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
	enchangehp ENC_TARGET_EVENT_TARGET, -7, ENC_AMOUNT_PERCENT
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
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
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
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT   @ Intensity 5
	goto EncScript_Moltres_TurnClose_BurnAnim
EncScript_Moltres_TurnClose_Burn3:
	enchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	goto EncScript_Moltres_TurnClose_BurnAnim
EncScript_Moltres_TurnClose_Burn4:
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
EncScript_Moltres_TurnClose_BurnAnim:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
EncScript_Moltres_TurnClose_Immolate:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Moltres_TurnClose_Decay
	printstring STRINGID_ENCMOLTRESCONSUMESITSELF
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, -5, ENC_AMOUNT_PERCENT
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
	enchangehp ENC_TARGET_BOSS, 55, ENC_AMOUNT_PERCENT
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

// Mewtwo, "The Perfect Weapon" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Mewtwo_Intro:
// 0 Phase (0 analysis / 1 adaptation / 2 perfect adaptation / 3 weakened), 1 Form (0 base / 1 Mega X
// / 2 Mega Y), 2 Lean (0-6), 3 Cooldown, 4 Instability (0-3), 5 Overload, 6 Stance (0-3),
// 7 LastGuard, 8 Chant, 9 TurnGuard, 10 MoveGuard, 11 BossGuard, 12 Power, 13 Scratch.
//
// Lean is a tug-of-war the player moves without meaning to: physical +1, special/status -1. When
// Cooldown expires Mewtwo MIRRORS the winning side - Mega X at Lean >= 5, Mega Y at Lean <= 1 - and
// the new species' stat spread does the countering with no extra scripting. Every form change and
// every shrugged sleep/freeze costs Instability, and Instability drives the guard DOWNWARD
// (ApplyGuard). At Instability 3 it Overloads: back to base Mewtwo, guard 40 for two full turns.
// So the only way to open a window is to keep changing what you do - which is also the counterplay
// to the mirroring. Stance (Force in X, Focus in Y) punishes every third landed hit and resets on
// every form change. Phase 2 makes adaptation compulsory, so the adaptation is what kills it.

// --- Shared subroutines (call/return) ---

// Sole owner of the guard ladder AND its callout. The ladder is not monotonic (Overload craters it
// and then hands it back), so this stores the last ANNOUNCED tier in LastGuard and prints only on a
// change, in either direction. Called from every place either input can move.
EncScript_Mewtwo_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mewtwo_ApplyGuard_Done   @ weakened owns its own guard
	encjumpifvar CMP_GREATER_THAN, 5, 1, EncScript_Mewtwo_ApplyGuard_T3   @ Overload 3 or 2
	encjumpifvar CMP_GREATER_THAN, 4, 1, EncScript_Mewtwo_ApplyGuard_T2   @ Instability 2+
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Mewtwo_ApplyGuard_T1
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 0
	printstring STRINGID_ENCMEWTWOREASSEMBLES
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_ApplyGuard_T1:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 1
	printstring STRINGID_ENCMEWTWOFLICKER
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_ApplyGuard_T2:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 2
	printstring STRINGID_ENCMEWTWORIPPLES
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_ApplyGuard_T3:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Mewtwo_ApplyGuard_Done
	encsetvar 7, 3
	printstring STRINGID_ENCMEWTWOERUPTS
	waitmessage B_WAIT_TIME_LONG
EncScript_Mewtwo_ApplyGuard_Done:
	return

// A form change rebuilds the battle stats from the new species, which wipes every raw stat change
// made since. Re-applies the banked ones - Perfect Adaptation's boost, then one Annihilation stack
// at a time - so "permanent" means permanent. Runs after every encformchange except Weakened's.
EncScript_Mewtwo_RestorePower:
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Mewtwo_RestorePower_Stacks
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
EncScript_Mewtwo_RestorePower_Stacks:
	enccopyvar 13, 12   @ Scratch = Power, so the loop below can consume it without losing Power
EncScript_Mewtwo_RestorePower_Loop:
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Mewtwo_RestorePower_Done
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 12, ENC_AMOUNT_PERCENT
	encsubvar 13, 1
	goto EncScript_Mewtwo_RestorePower_Loop
EncScript_Mewtwo_RestorePower_Done:
	return

// The transformation body, shared by FirstAdapt and Adapt. The caller has already written the
// destination into Form. The failure path lands on _Cooldown, not on the return: Adapt's only gate
// is Cooldown == 0, so a script that returned without setting it would be re-selected immediately.
EncScript_Mewtwo_DoAdapt:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mewtwo_DoAdapt_Y
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO_MEGA_X, EncScript_Mewtwo_DoAdapt_Cooldown
	printstring STRINGID_ENCMEWTWOBECOMESX
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Mewtwo_DoAdapt_After
EncScript_Mewtwo_DoAdapt_Y:
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO_MEGA_Y, EncScript_Mewtwo_DoAdapt_Cooldown
	printstring STRINGID_ENCMEWTWOBECOMESY
	waitmessage B_WAIT_TIME_LONG
EncScript_Mewtwo_DoAdapt_After:
	encsetvar 6, 0   @ Stance - rebuilding the body interrupts whatever it was building
	encsetvar 2, 3   @ Lean back to neutral; the tug-of-war restarts
	encjumpifvar CMP_GREATER_THAN, 4, 2, EncScript_Mewtwo_DoAdapt_Cooldown   @ Instability clamps at 3
	encaddvar 4, 1
EncScript_Mewtwo_DoAdapt_Cooldown:
	encsetvar 3, 3
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Mewtwo_DoAdapt_Finish
	encsetvar 3, 1   @ Phase 2: it re-evaluates every turn and can no longer stop
EncScript_Mewtwo_DoAdapt_Finish:
	call EncScript_Mewtwo_RestorePower
	call EncScript_Mewtwo_ApplyGuard
	return

// Stance +1; at 3 it spends, and which payoff lands is the active form's. Only ever called while
// Form is non-zero, so the X branch is the correct fallthrough.
EncScript_Mewtwo_GainStance:
	encaddvar 6, 1
	encjumpifvar CMP_LESS_THAN, 6, 3, EncScript_Mewtwo_GainStance_Done
	encsetvar 6, 0
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mewtwo_GainStance_Focus
	// Annihilation (Mega X): a chunk off the player's side, and Mewtwo banks another Attack stack.
	// Escalation - the longer a Force meter is left to fill, the harder X hits for the rest of the
	// fight.
	printstring STRINGID_ENCMEWTWOFORCE
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -15, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	// Bank the stack before applying it, so the live stat and Power never disagree at the cap -
	// RestorePower re-applies exactly what Power says after a form change wipes the stat.
	encjumpifvar CMP_GREATER_THAN, 12, 5, EncScript_Mewtwo_GainStance_Done   @ Power stacks cap at 6
	encaddvar 12, 1
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 12, ENC_AMOUNT_PERCENT
	return
EncScript_Mewtwo_GainStance_Focus:
	// Mind Crush (Mega Y): a smaller chunk, plus stat drops. Control rather than escalation.
	printstring STRINGID_ENCMEWTWOFOCUS
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
EncScript_Mewtwo_GainStance_Done:
	return

// Lean -1, floored at 0 (encsubvar wraps, so the floor has to be explicit). Shared by the special
// and status branches - for Mewtwo, a turn spent on setup reads the same as attacking specially.
EncScript_Mewtwo_LeanSpecial:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Mewtwo_LeanSpecial_Done
	encsubvar 2, 1
EncScript_Mewtwo_LeanSpecial_Done:
	return

// --- Trigger scripts ---

// No transformation yet and no trainerslide - this is a wild Pokemon. Lean starts neutral (vars
// zero-initialise, so 3 has to be written here).
EncScript_Mewtwo_Intro::
	encsetvar 2, 3   @ Lean
	printstring STRINGID_ENCMEWTWOINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Runs once at the top of every turn (TurnGuard gate): clears the per-event guards, ticks the
// adaptation cooldown, runs the recurring callout, then shrugs off sleep/freeze. The shrug is the
// one place status HELPS the player - it costs Mewtwo Instability, a third of a damage window.
EncScript_Mewtwo_TurnOpen::
	encsetvar 9, 1    @ TurnGuard
	encsetvar 10, 0   @ MoveGuard
	encsetvar 11, 0   @ BossGuard
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Mewtwo_TurnOpen_Chant
	encsubvar 3, 1    @ Cooldown
EncScript_Mewtwo_TurnOpen_Chant:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Mewtwo_TurnOpen_Status   @ analysis: nothing to comment on
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mewtwo_TurnOpen_Status   @ weakened
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Mewtwo_TurnOpen_Status   @ overloaded: it has its own lines
	encaddvar 8, 1    @ Chant
	encjumpifvar CMP_LESS_THAN, 8, 3, EncScript_Mewtwo_TurnOpen_Status
	encsetvar 8, 0
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Mewtwo_TurnOpen_Watches
	printstring STRINGID_ENCMEWTWOSTRAINS
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Mewtwo_TurnOpen_Status
EncScript_Mewtwo_TurnOpen_Watches:
	printstring STRINGID_ENCMEWTWOWATCHES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mewtwo_TurnOpen_Status:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mewtwo_TurnOpen_Done   @ weakened: status sticks now
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Mewtwo_TurnOpen_Shrug
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Mewtwo_TurnOpen_Shrug
	goto EncScript_Mewtwo_TurnOpen_Done
EncScript_Mewtwo_TurnOpen_Shrug:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCMEWTWOSHRUGS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_GREATER_THAN, 4, 2, EncScript_Mewtwo_TurnOpen_Done   @ Instability clamps at 3
	encaddvar 4, 1
	call EncScript_Mewtwo_ApplyGuard
EncScript_Mewtwo_TurnOpen_Done:
	return

// OnTurnEnd, priority 90 - runs last. Clears TurnGuard and steps the Overload window down, so it
// always resolves at a later checkpoint than the one that opened it.
EncScript_Mewtwo_TurnClose::
	encsetvar 9, 0   @ TurnGuard
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Mewtwo_TurnClose_Done
	encsubvar 5, 1   @ Overload 3 -> 2 -> 1
EncScript_Mewtwo_TurnClose_Done:
	return

// The player attacked physically: Lean climbs toward Mega X, capped at 6.
EncScript_Mewtwo_PlayerPhysical::
	encsetvar 10, 1   @ MoveGuard
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Mewtwo_PlayerPhysical_Done
	encaddvar 2, 1    @ Lean
EncScript_Mewtwo_PlayerPhysical_Done:
	return

// The player attacked specially: Lean falls toward Mega Y.
EncScript_Mewtwo_PlayerSpecial::
	encsetvar 10, 1   @ MoveGuard
	call EncScript_Mewtwo_LeanSpecial
	return

// A status move counts the same way, and Mega Y feeds on it directly - Focus fills off the turns
// the player spends not attacking, which is exactly what Y is meant to punish.
EncScript_Mewtwo_PlayerStatus::
	encsetvar 10, 1   @ MoveGuard
	call EncScript_Mewtwo_LeanSpecial
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Mewtwo_PlayerStatus_Done
	call EncScript_Mewtwo_GainStance
EncScript_Mewtwo_PlayerStatus_Done:
	return

// Mewtwo's own move landed on the player: the active form's Stance meter climbs.
EncScript_Mewtwo_BossHit::
	encsetvar 11, 1   @ BossGuard
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mewtwo_BossHit_Done   @ base Mewtwo has no stance
	call EncScript_Mewtwo_GainStance
EncScript_Mewtwo_BossHit_Done:
	return

// Turn 4: the analysis ends and the first transformation always fires, so the reveal lands as a
// transformation rather than a shrug. Lean >= 4 picks X, everything else picks Y.
EncScript_Mewtwo_FirstAdapt::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCMEWTWOANALYZED
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Mewtwo_FirstAdapt_X
	encsetvar 1, 2   @ Form = Mega Y
	goto EncScript_Mewtwo_FirstAdapt_Do
EncScript_Mewtwo_FirstAdapt_X:
	encsetvar 1, 1   @ Form = Mega X
EncScript_Mewtwo_FirstAdapt_Do:
	call EncScript_Mewtwo_DoAdapt
	return

// The cooldown expired. Phase 1: Lean decides, and the middle of the range is a dead zone it sits
// out quietly. Phase 2: it flips regardless of Lean - the adaptation has stopped being a choice, so
// every expiry is another point of Instability and the damage windows arrive on their own.
EncScript_Mewtwo_Adapt::
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Mewtwo_Adapt_Forced
	encjumpifvar CMP_GREATER_THAN, 2, 4, EncScript_Mewtwo_Adapt_WantX   @ Lean >= 5
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Mewtwo_Adapt_WantY      @ Lean <= 1
	encsetvar 3, 1   @ dead zone: re-check next turn, silently
	return
EncScript_Mewtwo_Adapt_WantX:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mewtwo_Adapt_Hold
	encsetvar 1, 1
	goto EncScript_Mewtwo_Adapt_Do
EncScript_Mewtwo_Adapt_WantY:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mewtwo_Adapt_Hold
	encsetvar 1, 2
	goto EncScript_Mewtwo_Adapt_Do
EncScript_Mewtwo_Adapt_Hold:
	// It is already in the form the player is asking for, so nothing changes and nothing
	// destabilises. The line is the nudge: it considered rebuilding and did not need to.
	encsetvar 3, 2   @ Cooldown
	printstring STRINGID_ENCMEWTWOISEE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mewtwo_Adapt_Forced:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mewtwo_Adapt_ForceY
	encsetvar 1, 1   @ base or Mega Y -> Mega X
	goto EncScript_Mewtwo_Adapt_Do
EncScript_Mewtwo_Adapt_ForceY:
	encsetvar 1, 2
EncScript_Mewtwo_Adapt_Do:
	call EncScript_Mewtwo_DoAdapt
	return

// Instability 3: the body gives. It collapses back to base Mewtwo, takes a chunk out of itself, and
// its guard craters for two full turns. Overload and Instability are written before anything that
// can fail, so this trigger can never re-select itself. ApplyGuard prints the eruption line.
EncScript_Mewtwo_OverloadStart::
	encsetvar 5, 3   @ Overload - two full turns, stepped down by TurnClose
	encsetvar 4, 0   @ Instability
	encsetvar 6, 0   @ Stance
	encsetvar 2, 3   @ Lean
	encsetvar 3, 3   @ Cooldown - it can't adapt its way out of the window
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mewtwo_OverloadStart_Collapse
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO, EncScript_Mewtwo_OverloadStart_Collapse, B_ANIM_ULTRA_BURST
	encsetvar 1, 0   @ Form = base
	call EncScript_Mewtwo_RestorePower
EncScript_Mewtwo_OverloadStart_Collapse:
	call EncScript_Mewtwo_ApplyGuard
	enchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

// Two turns later: the window closes and the guard goes back to whatever the Instability ladder
// says, which ApplyGuard announces.
EncScript_Mewtwo_OverloadEnd::
	encsetvar 5, 0   @ Overload
	call EncScript_Mewtwo_ApplyGuard
	return

// 25% HP: the adaptation stops being a choice. The cooldown drops to a single turn, both offensive
// stats climb, and Adapt starts flipping the form whether Lean asked for it or not.
EncScript_Mewtwo_PerfectAdaptation::
	encsetvar 0, 2   @ Phase 2
	encsetvar 3, 1   @ Cooldown
	encsetvar 6, 0   @ Stance
	printstring STRINGID_ENCMEWTWOPERFECT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
	return

// 10% HP: the only catch window. The revert is what guarantees the player catches a SPECIES_MEWTWO
// rather than a Mega, instead of leaning on the species' FORM_CHANGE_END_BATTLE backstop. Every
// guard drops; the automatic catch-window damage guard takes over from here.
EncScript_Mewtwo_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 4, 0   @ Instability
	encsetvar 5, 0   @ Overload
	encsetvar 6, 0   @ Stance
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mewtwo_Weakened_Guards
	encformchange ENC_TARGET_BOSS, SPECIES_MEWTWO, EncScript_Mewtwo_Weakened_Guards, B_ANIM_FORM_CHANGE
	encsetvar 1, 0   @ Form = base
EncScript_Mewtwo_Weakened_Guards:
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMEWTWOWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Mew, "The Genetic Wonder" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Mew_Intro:
// 0 Phase (0 curious / 1 playtime / 2 genesis / 3 weakened), 1 Wonder (0-6), 2 Bored (0-3),
// 3 LastCat (1 physical / 2 special / 3 status / 4 switch), 4-7 SeenPhys/SeenSpec/SeenStatus/
// SeenSwitch, 8 Wait (0 off / 1 armed / 2 waiting / 3 satisfied / 4 delighted / 5 disappointed),
// 9 LastGuard (last ANNOUNCED guard tier 0-3), 10 Chant, 11 TurnGuard, 12 MoveGuard, 13 SwitchGuard,
// 14 ChaosGuard.
//
// The fight punishes a habit, not a resource. Changing what KIND of thing you do - a move category
// different from last turn, or a switch - raises Wonder, which drives Mew's guard DOWNWARD as it
// gets absorbed in watching you (ApplyGuard: 92 -> 86 -> 78 -> 68). Repeating a category raises
// Bored; at 3 it fires one Mischief roll off a fixed table and knocks Wonder back down, so spam
// regresses. From Playtime (60% HP) Mew uses enctransform to become the player's active Pokemon and
// re-copies on every switch. At 30% HP it stops fighting for one turn and asks to be shown something
// (a status move or a switch); either branch enters Genesis - raw offense +20%, Mischief every turn.
// Below 10% it reverts, all guards off, Poke Balls unlocked.

// --- Shared subroutines (call/return) ---

// Sole owner of the guard ladder AND its callout. The ladder is not monotonic (Mischief pushes
// Wonder down), so this stores the last ANNOUNCED tier in LastGuard and prints only on a change, in
// either direction. Returns early in Weakened, which owns its own guard.
EncScript_Mew_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mew_ApplyGuard_Done
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Mew_ApplyGuard_T3   @ Wonder 5-6
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Mew_ApplyGuard_T2   @ Wonder 3-4
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Mew_ApplyGuard_T1   @ Wonder 1-2
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 0
	printstring STRINGID_ENCMEWRETREATS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mew_ApplyGuard_T1:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 1
	printstring STRINGID_ENCMEWCLOSER
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mew_ApplyGuard_T2:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encjumpifvar CMP_EQUAL, 9, 2, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 2
	printstring STRINGID_ENCMEWUNSHIELDED
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mew_ApplyGuard_T3:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encjumpifvar CMP_EQUAL, 9, 3, EncScript_Mew_ApplyGuard_Done
	encsetvar 9, 3
	printstring STRINGID_ENCMEWABSORBED
	waitmessage B_WAIT_TIME_LONG
EncScript_Mew_ApplyGuard_Done:
	return

// Wonder +1 (cap 6), Bored = 0, then the guard ladder. Prints the "eyes light up" line only when
// Wonder actually moved.
EncScript_Mew_GainWonder:
	encsetvar 2, 0   @ Bored
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Mew_GainWonder_Done
	encaddvar 1, 1
	printstring STRINGID_ENCMEWEYESLIGHTUP
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Mew_ApplyGuard
EncScript_Mew_GainWonder_Done:
	return

// Bored +1 (cap 3). At 2 it prints the drift warning, so the player gets a turn's notice before
// Mischief lands.
EncScript_Mew_GainBored:
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Mew_GainBored_Done
	encaddvar 2, 1
	encjumpifvar CMP_NOT_EQUAL, 2, 2, EncScript_Mew_GainBored_Done
	printstring STRINGID_ENCMEWDRIFTS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mew_GainBored_Done:
	return

// The Transform beat. enctransform copies the player's active Pokemon onto Mew (species, stats, stat
// stages, types, ability, moves); HP/level/item/status and the party mon are untouched, so a caught
// Mew is still SPECIES_MEW. Re-copying (a later switch) preserves the first original species for the
// revert. Jumps to _Failed on the legitimate states that block a copy.
EncScript_Mew_Copy:
	printstring STRINGID_ENCMEWCOPIES
	waitmessage B_WAIT_TIME_SHORT
	enctransform ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, EncScript_Mew_Copy_Failed
	printstring STRINGID_ENCMEWWEARSYOURSHAPE
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Mew_Copy_Done   @ Genesis: re-apply the raw boost the copy just wiped
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
EncScript_Mew_Copy_Done:
	return
EncScript_Mew_Copy_Failed:
	printstring STRINGID_ENCMEWCANTQUITEGETIT
	waitmessage B_WAIT_TIME_SHORT
	return

// The Mischief outcomes. Each is silent of the Wonder cost and Bored reset - the dispatcher
// (EncScript_Mew_Mischief) owns those - and just applies its own effect and animation.
EncScript_Mew_Mischief_Copycat:
	printstring STRINGID_ENCMEWCOPYCAT
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

EncScript_Mew_Mischief_Whim:
	printstring STRINGID_ENCMEWSKY
	waitmessage B_WAIT_TIME_SHORT
	encjumpifchance 33, EncScript_Mew_Mischief_Whim_Sun
	encjumpifchance 50, EncScript_Mew_Mischief_Whim_Rain
	encsetweather BATTLE_WEATHER_SANDSTORM, 0
	playanimation BS_PLAYER1, B_ANIM_SANDSTORM_CONTINUES
	return
EncScript_Mew_Mischief_Whim_Sun:
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_PLAYER1, B_ANIM_SUN_CONTINUES
	return
EncScript_Mew_Mischief_Whim_Rain:
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_PLAYER1, B_ANIM_RAIN_CONTINUES
	return

EncScript_Mew_Mischief_Recovery:
	printstring STRINGID_ENCMEWRECOVERS
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, 10, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

EncScript_Mew_Mischief_Tag:
	printstring STRINGID_ENCMEWTAG
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
	return

EncScript_Mew_Mischief_Generosity:
	printstring STRINGID_ENCMEWCLEARSFIELD
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_PLAYER_LEFT, 15, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	removeweather
	return

// --- Trigger scripts ---

// No transformation yet and no trainerslide - this is a wild Pokemon.
EncScript_Mew_Intro::
	printstring STRINGID_ENCMEWINTRO
	waitmessage B_WAIT_TIME_LONG
	return

// Runs once at the top of every turn (TurnGuard gate): clears the per-event guards, then - while the
// mechanic is live - runs the recurring callout every 3rd turn, keyed to whether Wonder is high.
EncScript_Mew_TurnOpen::
	encsetvar 11, 1   @ TurnGuard
	encsetvar 12, 0   @ MoveGuard
	encsetvar 13, 0   @ SwitchGuard
	encsetvar 14, 0   @ ChaosGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mew_TurnOpen_Done   @ weakened: no more callouts
	encaddvar 10, 1   @ Chant
	encjumpifvar CMP_LESS_THAN, 10, 3, EncScript_Mew_TurnOpen_Done
	encsetvar 10, 0
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Mew_TurnOpen_Entranced   @ Wonder >= 3
	printstring STRINGID_ENCMEWWATCHES
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Mew_TurnOpen_Done
EncScript_Mew_TurnOpen_Entranced:
	printstring STRINGID_ENCMEWENTRANCED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mew_TurnOpen_Done:
	return

// OnTurnEnd, priority 90 - runs last. Clears TurnGuard, then in Genesis the disappointed branch's
// recurring chip: 6% off the player's side every turn for the rest of the fight.
EncScript_Mew_TurnClose::
	encsetvar 11, 0   @ TurnGuard
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Mew_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 8, 5, EncScript_Mew_TurnClose_Done
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
EncScript_Mew_TurnClose_Done:
	return

// The player attacked physically. First-time beat: Attack +1 stage. Then the variety read - a
// category different from last turn raises Wonder, a repeat raises Bored.
EncScript_Mew_PlayerPhysical::
	encsetvar 12, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Mew_PlayerPhysical_Variety
	encsetvar 4, 1   @ SeenPhys
	printstring STRINGID_ENCMEWFASCINATED
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
EncScript_Mew_PlayerPhysical_Variety:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Mew_PlayerPhysical_Repeat
	encsetvar 3, 1   @ LastCat = physical
	call EncScript_Mew_GainWonder
	return
EncScript_Mew_PlayerPhysical_Repeat:
	call EncScript_Mew_GainBored
	return

// The player attacked specially. First-time beat: Sp. Atk +1 stage.
EncScript_Mew_PlayerSpecial::
	encsetvar 12, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Mew_PlayerSpecial_Variety
	encsetvar 5, 1   @ SeenSpec
	printstring STRINGID_ENCMEWSTUDIES
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
EncScript_Mew_PlayerSpecial_Variety:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Mew_PlayerSpecial_Repeat
	encsetvar 3, 2   @ LastCat = special
	call EncScript_Mew_GainWonder
	return
EncScript_Mew_PlayerSpecial_Repeat:
	call EncScript_Mew_GainBored
	return

// The player used a status move. First-time beat: Def +1 and Sp. Def +1. Also answers the challenge
// (Wait 2 -> 3) - a status move is one of the two things Mew wants to be shown.
EncScript_Mew_PlayerStatus::
	encsetvar 12, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Mew_PlayerStatus_Variety
	encsetvar 6, 1   @ SeenStatus
	printstring STRINGID_ENCMEWCOPIESTECHNIQUE
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
EncScript_Mew_PlayerStatus_Variety:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Mew_PlayerStatus_Repeat
	encsetvar 3, 3   @ LastCat = status
	call EncScript_Mew_GainWonder
	goto EncScript_Mew_PlayerStatus_Challenge
EncScript_Mew_PlayerStatus_Repeat:
	call EncScript_Mew_GainBored
EncScript_Mew_PlayerStatus_Challenge:
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Mew_PlayerStatus_Satisfy
	return
EncScript_Mew_PlayerStatus_Satisfy:
	encsetvar 8, 3   @ challenge satisfied
	return

// A player Pokemon switched in. First-time beat: Mew heals 8%. A switch always raises Wonder (it is
// variety by definition), answers the challenge, and - from Playtime on - Mew re-copies the new mon.
EncScript_Mew_SwitchIn::
	encsetvar 13, 1   @ SwitchGuard
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Mew_SwitchIn_Variety
	encsetvar 7, 1   @ SeenSwitch
	printstring STRINGID_ENCMEWFLITSBACK
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Mew_SwitchIn_Variety:
	encsetvar 3, 4   @ LastCat = switch
	call EncScript_Mew_GainWonder
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Mew_SwitchIn_Satisfy
	goto EncScript_Mew_SwitchIn_Copy
EncScript_Mew_SwitchIn_Satisfy:
	encsetvar 8, 3   @ challenge satisfied
EncScript_Mew_SwitchIn_Copy:
	encjumpifvar CMP_LESS_THAN, 0, 1, EncScript_Mew_SwitchIn_Done   @ still curious: no transform yet
	call EncScript_Mew_Copy
EncScript_Mew_SwitchIn_Done:
	return

// 60% HP: Playtime. Mew decides the fight is a game and takes on the player's shape. Registered at
// both OnMoveEnd and OnTurnEnd; both gate on Phase == 0, which this overwrites.
EncScript_Mew_Playtime::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCMEWPLAYTIME
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	call EncScript_Mew_Copy
	return

// 30% HP: the challenge. Phase goes to 2 up front so the Weakened trigger is armed even if the
// player bursts Mew through the waiting turn. Wait = 1 arms the state machine; WaitStep picks it up
// next turn.
EncScript_Mew_Challenge::
	encsetvar 0, 2   @ Phase 2
	encsetvar 8, 1   @ Wait = armed
	printstring STRINGID_ENCMEWDEMAND
	waitmessage B_WAIT_TIME_LONG
	return

// OnTurnStart, the turn after the challenge. Mew's offense collapses for exactly this turn (Atk and
// Sp. Atk driven to the floor), which reads as it half-heartedly waiting. Its guard is untouched, so
// the turn is genuinely free.
EncScript_Mew_WaitStep::
	encsetvar 8, 2   @ Wait = waiting
	printstring STRINGID_ENCMEWDEMAND
	waitmessage B_WAIT_TIME_SHORT
	printstring STRINGID_ENCMEWWAITING
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	return

// OnTurnEnd of the waiting turn. Restores offense to neutral (drive to the cap, then back 6 - the
// established "back to neutral" idiom), resolves delighted (Wait 3) vs disappointed, then enters
// Genesis: raw Atk/Sp. Atk +20% and Mischief every turn from here.
EncScript_Mew_WaitResolve::
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Mew_WaitResolve_Weakened   @ Mew got bursted past the challenge
	encjumpifvar CMP_EQUAL, 8, 3, EncScript_Mew_WaitResolve_Delighted
	encsetvar 8, 5   @ disappointed
	printstring STRINGID_ENCMEWDISAPPOINTED
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Mew_WaitResolve_Genesis
EncScript_Mew_WaitResolve_Delighted:
	encsetvar 8, 4   @ delighted
	printstring STRINGID_ENCMEWDELIGHTED
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_PLAYER_LEFT, 30, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	encsetvar 1, 6   @ Wonder maxed; nothing lowers it in Genesis
	call EncScript_Mew_ApplyGuard
EncScript_Mew_WaitResolve_Genesis:
	printstring STRINGID_ENCMEWGENESIS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	waitanimation
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 20, ENC_AMOUNT_PERCENT
	return
EncScript_Mew_WaitResolve_Weakened:
	encsetvar 8, 4   @ close the state machine; Weakened already owns the fight
	return

// OnTurnStart. Fires at Bored 3, or every turn once Genesis is running. Owns ChaosGuard, the Bored
// reset, and - while Phase <= 1 - the Wonder cost of being predictable. Rolls one outcome off the
// fixed table: 20% Copycat, 20% Whim, 20% Recovery, 25% Tag, 15% Generosity, with Transform
// replacing 40% of the Copycat slot once Mew is in Playtime.
EncScript_Mew_Mischief::
	encsetvar 14, 1   @ ChaosGuard
	encsetvar 2, 0    @ Bored
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Mew_Mischief_Roll   @ Genesis: no "bored" line
	printstring STRINGID_ENCMEWBORED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mew_Mischief_Roll:
	encjumpifchance 20, EncScript_Mew_Mischief_SlotCopycat
	encjumpifchance 25, EncScript_Mew_Mischief_SlotWhim
	encjumpifchance 33, EncScript_Mew_Mischief_SlotRecovery
	encjumpifchance 63, EncScript_Mew_Mischief_SlotTag
	call EncScript_Mew_Mischief_Generosity
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotCopycat:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Mew_Mischief_SlotCopycat_Plain
	encjumpifchance 40, EncScript_Mew_Mischief_SlotTransform
EncScript_Mew_Mischief_SlotCopycat_Plain:
	call EncScript_Mew_Mischief_Copycat
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotTransform:
	call EncScript_Mew_Copy
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotWhim:
	call EncScript_Mew_Mischief_Whim
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotRecovery:
	call EncScript_Mew_Mischief_Recovery
	goto EncScript_Mew_Mischief_Cost
EncScript_Mew_Mischief_SlotTag:
	call EncScript_Mew_Mischief_Tag
EncScript_Mew_Mischief_Cost:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Mew_Mischief_Done   @ Genesis: predictability is free
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mew_Mischief_Done
	encsubvar 1, 1   @ Wonder -1
	call EncScript_Mew_ApplyGuard
EncScript_Mew_Mischief_Done:
	return

// 10% HP: Mew tires of the game. It reverts to its own shape, every guard and the survive clamp
// come off (the automatic catch-window guard takes over on the trailing pass), and Poke Balls are
// unblocked. Registered at both checkpoints; gated on Phase == 2 so it can't precede the challenge.
EncScript_Mew_Weakened::
	encsetvar 0, 3   @ Phase 3
	encuntransform ENC_TARGET_BOSS
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMEWWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// ---------------------------------------------------------------------------------------------
// Raikou, "The Hunting Thunder" (src/data/battle_encounters.encounter). Var indices, pinned by
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
	enchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
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
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
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
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
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
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
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
	enchangehp ENC_TARGET_BOSS, -12, ENC_AMOUNT_PERCENT
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

// ---------------------------------------------------------------------------------------------
// Entei, "The Walking Volcano" (src/data/battle_encounters.encounter). Var indices, pinned by
// the always-true Conditions on EncScript_Entei_Intro:
// 0 Phase (0 dormant / 1 eruption cycle / 2 cataclysm / 3 weakened), 1 Pressure (0-5), 2 Fuse
// (0 unlit, else 3->2->1 resolved at 1), 3 Scorch (0-3), 4 Window (turns of open guard left),
// 5 LastGuard (reduction last ANNOUNCED), 6 VentCool, 7 TurnGuard, 8 PMove, 9 BMoveGuard,
// 10 SwitchGuard, 11 FaintGuard.
//
// Pressure never falls on its own - it only rises, and is only spent by erupting. The player does
// not choose whether the volcano goes off, only when, how big, and what they are standing on. The
// guard is a locked door rather than a ladder: flat for the whole phase (90/92/94), and only an
// eruption opens it. Scorch is a switch tax and the eruption's fuel gauge, cooled by Water moves
// and Rain, and locked at 3 once the Cataclysm begins.

// --- Shared subroutines (call/return) ---

// Sole owner of the flat phase guard AND its callout. Returns without touching anything while a
// window owns the reduction, or in Weakened. Otherwise it writes the phase's base value and compares
// it against the last ANNOUNCED value, printing on every change - there is no UI for damage
// reduction, so a single showing would be trivially missed. Coming back from a window (any value
// below 80) reads as the moment closing; anything else is the mountain hardening.
EncScript_Entei_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_ApplyGuard_Done          @ weakened owns its own guard
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Entei_ApplyGuard_Done   @ a window owns the guard
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Entei_ApplyGuard_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Entei_ApplyGuard_P1
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 5, 90, EncScript_Entei_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 5, 80, EncScript_Entei_ApplyGuard_P0_Shut
	encsetvar 5, 90
	goto EncScript_Entei_ApplyGuard_Harden
EncScript_Entei_ApplyGuard_P0_Shut:
	encsetvar 5, 90
	goto EncScript_Entei_ApplyGuard_Shut
EncScript_Entei_ApplyGuard_P1:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encjumpifvar CMP_EQUAL, 5, 92, EncScript_Entei_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 5, 80, EncScript_Entei_ApplyGuard_P1_Shut
	encsetvar 5, 92
	goto EncScript_Entei_ApplyGuard_Harden
EncScript_Entei_ApplyGuard_P1_Shut:
	encsetvar 5, 92
	goto EncScript_Entei_ApplyGuard_Shut
EncScript_Entei_ApplyGuard_P2:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	encjumpifvar CMP_EQUAL, 5, 94, EncScript_Entei_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 5, 80, EncScript_Entei_ApplyGuard_P2_Shut
	encsetvar 5, 94
	goto EncScript_Entei_ApplyGuard_Harden
EncScript_Entei_ApplyGuard_P2_Shut:
	encsetvar 5, 94
EncScript_Entei_ApplyGuard_Shut:
	printstring STRINGID_ENCENTEIRECOVERS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Entei_ApplyGuard_Harden:
	printstring STRINGID_ENCENTEIHARDENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_ApplyGuard_Done:
	return

// The only place Pressure ever rises. A lit fuse locks it out entirely, so the countdown can be
// neither accelerated nor stalled once it starts.
EncScript_Entei_GainPressure:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_GainPressure_Done       @ weakened: the mountain is out
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Entei_GainPressure_Done   @ the fuse is already burning
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Entei_GainPressure_Done
	encaddvar 1, 1
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Entei_GainPressure_Arm
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Entei_GainPressure_Pours
	printstring STRINGID_ENCENTEIGROUNDWARMS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Entei_GainPressure_Pours:
	printstring STRINGID_ENCENTEIHEATPOURS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Entei_GainPressure_Arm:
	call EncScript_Entei_ArmFuse
EncScript_Entei_GainPressure_Done:
	return

// Two full turns of warning, cut to one once the Cataclysm begins. TurnClose steps it 3->2->1,
// TurnStart resolves it at 1.
EncScript_Entei_ArmFuse:
	encsetvar 2, 3
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Entei_ArmFuse_Print
	encsetvar 2, 2
EncScript_Entei_ArmFuse_Print:
	printstring STRINGID_ENCENTEITREMBLES
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MAGNITUDE
	waitanimation
	return

EncScript_Entei_Scorch1:
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Entei_Scorch1_Done
	encaddvar 3, 1
	printstring STRINGID_ENCENTEISCORCHED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_Scorch1_Done:
	return

// The player's half of the field battle. Dead once the Cataclysm locks the ground at 3.
EncScript_Entei_Cool1:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Entei_Cool1_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Entei_Cool1_Done   @ Cataclysm: the field is locked
	encsubvar 3, 1
	printstring STRINGID_ENCENTEICOOLS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_WATER_GUN
	waitanimation
EncScript_Entei_Cool1_Done:
	return

// The eruption body, shared by the normal and the below-15% forms. Percentage HP loss that ignores
// typing entirely, so nothing walls the volcano - only Entei's own moveset. Every eruption slams the
// field back to full Scorch, scours the stat changes off both sides, and throws Entei awake, then
// leaves it wide open for the only real damage window in the fight.
EncScript_Entei_EruptCore:
	encsetvar 2, 0   @ the fuse is spent
	printstring STRINGID_ENCENTEIERUPTS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ERUPTION
	waitanimation
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Entei_EruptCore_S3
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Entei_EruptCore_S2
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Entei_EruptCore_S1
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Entei_EruptCore_After
EncScript_Entei_EruptCore_S1:
	enchangehp ENC_TARGET_ALL_FOES, -25, ENC_AMOUNT_PERCENT
	goto EncScript_Entei_EruptCore_After
EncScript_Entei_EruptCore_S2:
	enchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Entei_EruptCore_After
EncScript_Entei_EruptCore_S3:
	enchangehp ENC_TARGET_ALL_FOES, -35, ENC_AMOUNT_PERCENT
EncScript_Entei_EruptCore_After:
	normalisebuffs
	encsetvar 3, 3
	printstring STRINGID_ENCENTEIFIELDSCORCHED
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1        @ thrown awake by its own blast
	updatestatusicon BS_OPPONENT1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Entei_EruptCore_Cata
	encsetvar 4, 2   @ Window: two full turns of open guard
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 5, 65
	encsetvar 1, 0   @ Pressure floor, Dormant
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Entei_EruptCore_Spent
	encsetvar 1, 2   @ Pressure floor, eruption cycle
	goto EncScript_Entei_EruptCore_Spent
EncScript_Entei_EruptCore_Cata:
	encsetvar 1, 3   @ Pressure floor, Cataclysm
	encsetvar 4, 3   @ three full turns
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 5, 55
EncScript_Entei_EruptCore_Spent:
	printstring STRINGID_ENCENTEISPENT
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger scripts ---

// LastGuard is seeded to the Properties reduction so the first change reads as a change.
EncScript_Entei_Intro::
	encsetvar 5, 90
	printstring STRINGID_ENCENTEIINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCENTEIGROUNDHOT
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn guards,
// lets Rain cool the ground, converts any status Entei is carrying into Pressure, and keeps the
// open-window callout running for as long as the window lasts.
EncScript_Entei_TurnOpen::
	flushtextbox
	encsetvar 7, 1    @ TurnGuard
	encsetvar 8, 0    @ PMove
	encsetvar 9, 0    @ BMoveGuard
	encsetvar 10, 0   @ SwitchGuard
	encsetvar 11, 0   @ FaintGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_TurnOpen_Done   @ weakened: inert
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_RAIN, EncScript_Entei_TurnOpen_Status
	call EncScript_Entei_Cool1
EncScript_Entei_TurnOpen_Status:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Entei_TurnOpen_Statused
	goto EncScript_Entei_TurnOpen_Window
EncScript_Entei_TurnOpen_Statused:
	printstring STRINGID_ENCENTEINOWHERETOGO
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Entei_GainPressure
EncScript_Entei_TurnOpen_Window:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Entei_TurnOpen_Done
	printstring STRINGID_ENCENTEISTILLOPEN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_TurnOpen_Done:
	return

EncScript_Entei_Erupt::
	flushtextbox
	call EncScript_Entei_EruptCore
	enchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCENTEIRECOIL
	waitmessage B_WAIT_TIME_SHORT
	return

// Below 15% the blast costs Entei nothing: enchangehp bypasses the damage-reduction path, so the
// recoil would otherwise faint it before the catch window ever opens.
EncScript_Entei_EruptSafe::
	flushtextbox
	call EncScript_Entei_EruptCore
	return

// Pressure Break. Holding a stat boost while Entei is loaded buys a small, early, weak eruption and
// a one-turn window - the mirror of letting Pressure run to 5 for a devastating one and the biggest
// window in the fight.
EncScript_Entei_Vent::
	flushtextbox
	encsubvar 1, 2
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Entei_Vent_Cool
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Entei_Vent_Cool
	encsetvar 1, 2   @ the eruption cycle's Pressure floor
EncScript_Entei_Vent_Cool:
	encsetvar 6, 3   @ VentCool: it cannot be farmed
	printstring STRINGID_ENCENTEIVENTS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_LAVA_PLUME
	waitanimation
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	normalisebuffs
	printstring STRINGID_ENCENTEISCOURED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Entei_Scorch1
	encsetvar 4, 1   @ Window: this turn only
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 5, 78
	printstring STRINGID_ENCENTEIOFFBALANCE
	waitmessage B_WAIT_TIME_LONG
	return

// Water cools the ground - and still feeds Pressure. Being the cooling lever does not exempt it from
// "Pressure cannot be prevented"; the player is choosing which of the two things they need more.
EncScript_Entei_PlayerWater::
	encsetvar 8, 2   @ PMove
	call EncScript_Entei_Cool1
	call EncScript_Entei_GainPressure
	return

EncScript_Entei_PlayerAttack::
	encsetvar 8, 2   @ PMove
	call EncScript_Entei_GainPressure
	return

// Entei stoking its own fire. OnMoveEnd fires whether or not the move connected - the heat goes into
// the ground either way.
EncScript_Entei_BossFire::
	encsetvar 9, 1   @ BMoveGuard
	call EncScript_Entei_GainPressure
	return

EncScript_Entei_Faint::
	encsetvar 11, 1   @ FaintGuard
	printstring STRINGID_ENCENTEIUNMOVED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Entei_GainPressure
	return

// The transition tax. Scorch never chips on a timer; it charges for pivoting, which is what makes
// letting it climb a real decision rather than a drip of damage.
EncScript_Entei_SwitchIn::
	encsetvar 10, 1   @ SwitchGuard
	printstring STRINGID_ENCENTEISEARS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_WILL_O_WISP
	waitanimation
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Entei_SwitchIn_S3
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Entei_SwitchIn_S2
	enchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	return
EncScript_Entei_SwitchIn_S2:
	enchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Entei_SwitchIn_S3:
	enchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown. The fuse is stepped here and resolved at the next TurnStart, so a real turn always
// passes between a tick and the eruption. Pressure gains come after the step, which is what keeps a
// fuse armed this turn from being stepped in the same breath.
EncScript_Entei_TurnClose::
	encsetvar 7, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Entei_TurnClose_Fuse3
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Entei_TurnClose_Fuse2
	goto EncScript_Entei_TurnClose_Heat
EncScript_Entei_TurnClose_Fuse3:
	encsetvar 2, 2
	printstring STRINGID_ENCENTEIMAGMA
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Entei_TurnClose_Heat
EncScript_Entei_TurnClose_Fuse2:
	encsetvar 2, 1
	printstring STRINGID_ENCENTEISPLITS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MAGNITUDE
	waitanimation
EncScript_Entei_TurnClose_Heat:
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Entei_TurnClose_Climb
	printstring STRINGID_ENCENTEIDRINKSHEAT
	waitmessage B_WAIT_TIME_SHORT
	enchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Entei_GainPressure
EncScript_Entei_TurnClose_Climb:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Entei_TurnClose_Cool
	call EncScript_Entei_GainPressure   @ from the eruption cycle on, Pressure climbs unconditionally
EncScript_Entei_TurnClose_Cool:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Entei_TurnClose_Window
	encsubvar 6, 1
EncScript_Entei_TurnClose_Window:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Entei_TurnClose_Glow
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Entei_TurnClose_Done   @ window still open
	call EncScript_Entei_ApplyGuard   @ closed: announces the guard coming back up
	goto EncScript_Entei_TurnClose_Done
EncScript_Entei_TurnClose_Glow:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Entei_TurnClose_Done
	printstring STRINGID_ENCENTEIGROUNDGLOWS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_TurnClose_Done:
	return

// Phase 1 at 50% HP. Pressure can never fall below 2 again and now climbs every turn on its own, so
// the eruptions come whatever the player does. The guard hardens a notch to pay for the fact that
// the windows are about to arrive more often.
EncScript_Entei_Cycle::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCENTEICYCLE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ERUPTION
	waitanimation
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Entei_Cycle_Guard
	encsetvar 1, 2   @ Pressure floor
	printstring STRINGID_ENCENTEIHEATPOURS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_Cycle_Guard:
	call EncScript_Entei_ApplyGuard
	return

// Phase 2 at 25% HP. The field locks at full Scorch and cooling stops working, the fuse shortens to
// one turn of warning, and every eruption becomes a Cataclysm. A fuse lit under the old rules is
// abandoned - Pressure is left one short of full so it re-arms on the shorter timer instead of
// stalling at 5 with nothing to spend it on.
EncScript_Entei_Cataclysm::
	encsetvar 0, 2   @ Phase 2
	encsetvar 2, 0   @ Fuse
	encsetvar 3, 3   @ Scorch locked
	printstring STRINGID_ENCENTEICATACLYSM
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EARTHQUAKE
	waitanimation
	printstring STRINGID_ENCENTEIFIELDSCORCHED
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Entei_Cataclysm_Rearm
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Entei_Cataclysm_Guard
	encsetvar 1, 3   @ Pressure floor
	goto EncScript_Entei_Cataclysm_Guard
EncScript_Entei_Cataclysm_Rearm:
	encsetvar 1, 4
EncScript_Entei_Cataclysm_Guard:
	call EncScript_Entei_ApplyGuard
	return

// Below 10% in phase 2. Everything off - a lit fuse or a scorched field landing after Entei has
// visibly guttered out would be incoherent, and a team-wide tick during the catch phase is pure
// frustration. CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the catch window.
EncScript_Entei_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Pressure
	encsetvar 2, 0   @ Fuse
	encsetvar 3, 0   @ Scorch
	encsetvar 4, 0   @ Window
	encsetvar 6, 0   @ VentCool
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 5, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCENTEIWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// ---------------------------------------------------------------------------------------------
// Suicune, "The Purifier" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Suicune_Intro:
// 0 Phase (0 Cleansing Current / 1 The Cleansing / 2 Sacred Beast / 3 Weakened), 1 Flow (0-5),
// 2 Rite (0 idle, else 3->2->1 resolved at 1), 3 Draw (0 idle, 2 telegraphed, 1 resolves),
// 4 Break (turns of open guard left), 5 LastGuard (reduction last ANNOUNCED), 6 Cleansed,
// 7 TurnGuard, 8 Shatter, 9 Calm, 10 Breach (1 broken draw / 2 shattered purity), 11 Prev,
// 12 Dirty, 13 Swept (a purification already ran this turn).
//
// Flow is fed ONLY by Suicune finding something on the field to wash away, and a turn that both
// purifies nothing and ends clean drains one back. Every effect the player puts down is a point of
// Flow waiting to be collected, so the whole fight is one question: is this worth giving Suicune
// power? The guard is a ladder rather than a locked door - draining Flow is visible progress on its
// own - and the fight inverts twice: Sacred Water only heals off a CLEAN field, and Sacred Beast is
// untouchable at any Flow above 0.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. Prev holds the value last announced so each
// leaf can write LastGuard before comparing; there is no UI for damage reduction, so a single
// showing would be trivially missed and the line has to re-fire on every real change.
EncScript_Suicune_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Suicune_ApplyGuard_Done   @ weakened owns its own guard
	enccopyvar 11, 5
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Suicune_Guard_Open
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Suicune_Guard_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Suicune_Guard_P1
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Suicune_Guard93
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Guard91
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Guard88
	goto EncScript_Suicune_Guard84
EncScript_Suicune_Guard_P1:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Suicune_Guard94
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Guard92
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Guard90
	goto EncScript_Suicune_Guard86
// Sacred Beast at Flow 0 is deliberately the softest un-broken state in the fight: draining Flow has
// to feel like progress on its own, so the shatter reads as a plan paying off, not a lucky roll.
EncScript_Suicune_Guard_P2:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Suicune_Guard95
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Guard93
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Guard90
	goto EncScript_Suicune_Guard80
EncScript_Suicune_Guard_Open:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Suicune_Guard72
	goto EncScript_Suicune_Guard76

EncScript_Suicune_Guard72:
	encsetdamagereduction ENC_TARGET_BOSS, 72
	encsetvar 5, 72
	encjumpifvar CMP_EQUAL, 11, 72, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 72, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard76:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encsetvar 5, 76
	encjumpifvar CMP_EQUAL, 11, 76, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 76, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 5, 80
	encjumpifvar CMP_EQUAL, 11, 80, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 80, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 5, 84
	encjumpifvar CMP_EQUAL, 11, 84, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 84, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 5, 86
	encjumpifvar CMP_EQUAL, 11, 86, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 86, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 5, 88
	encjumpifvar CMP_EQUAL, 11, 88, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 88, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 5, 90
	encjumpifvar CMP_EQUAL, 11, 90, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 90, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	encsetvar 5, 91
	encjumpifvar CMP_EQUAL, 11, 91, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 91, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 5, 92
	encjumpifvar CMP_EQUAL, 11, 92, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 92, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard93:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	encsetvar 5, 93
	encjumpifvar CMP_EQUAL, 11, 93, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 93, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard94:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encsetvar 5, 94
	encjumpifvar CMP_EQUAL, 11, 94, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 94, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard95:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	encsetvar 5, 95
	encjumpifvar CMP_EQUAL, 11, 95, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 95, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken

EncScript_Suicune_ApplyGuard_Thicken:
	printstring STRINGID_ENCSUICUNETHICKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Suicune_ApplyGuard_Slacken:
	printstring STRINGID_ENCSUICUNESLACKENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Suicune_ApplyGuard_Done:
	return

// Clamped +1, silent. A sweep can pay out up to six points at once, so the callout is a separate
// step rather than one line per point.
EncScript_Suicune_GainFlow:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Suicune_GainFlow_Done
	encaddvar 1, 1
EncScript_Suicune_GainFlow_Done:
	return

// One line per sweep that washed something away, telling the player the resource moved without
// telling them what it is. The Flow 4 line is louder because that is where Suicune starts shedding
// status unprompted; it re-fires every sweep, so the cue keeps running while the mechanic is live.
EncScript_Suicune_FlowCallout:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Suicune_FlowCallout_Surge
	printstring STRINGID_ENCSUICUNEFLOWRISES
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Suicune_FlowCallout_Surge:
	printstring STRINGID_ENCSUICUNEFLOWSURGES
	waitmessage B_WAIT_TIME_SHORT
	return

EncScript_Suicune_LoseFlow:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Suicune_LoseFlow_Done
	encsubvar 1, 1
	printstring STRINGID_ENCSUICUNEFLOWEBBS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Suicune_LoseFlow_Done:
	return

// Stat stages off neutral on either side, into Dirty. Split out of the impurity sweep because the
// purification needs the same answer before deciding whether to Haze.
// The read-only twin of the purification: the same five impurities in the same order, answering into
// Dirty without touching anything. Sacred Water and the Flow decay both hang off this, so the two
// halves of the fight cannot disagree about what "clean" means.
//
// Stat stages are deliberately NOT on the list. They were, on both sides, and it made the player
// audit their own boosts every turn to avoid feeding Flow - too much to track for what it added.
// Suicune's own stages can't be the impurity on their own either: the only reset opcode is
// normalisebuffs, which Hazes every battler, so cleaning a debuff off Suicune would still wipe the
// player's boosts and leave them managing the same thing one step removed.
EncScript_Suicune_Impure:
	encsetvar 12, 1
	jumpifhalfword CMP_NOT_EQUAL, gBattleWeather, B_WEATHER_NONE, EncScript_Suicune_Impure_Done
	jumpifhalfword CMP_COMMON_BITS, gFieldStatuses, STATUS_FIELD_TERRAIN_ANY, EncScript_Suicune_Impure_Done
	jumpifsideaffecting BS_PLAYER1, SIDE_STATUS_BARRIER_ANY, EncScript_Suicune_Impure_Done
	jumpifsideaffecting BS_OPPONENT1, SIDE_STATUS_BARRIER_ANY, EncScript_Suicune_Impure_Done
	trytidyup FALSE, EncScript_Suicune_Impure_Done   @ hazards on either side, or any Substitute
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Suicune_Impure_Done
	encsetvar 12, 0
EncScript_Suicune_Impure_Done:
	return

// The sweep. Walks the five impurities in a fixed order, counting each removal into Cleansed, then
// pays Suicune one point of Flow per thing washed away. The count is left standing afterwards
// because TurnClose reads it to decide whether the turn earned a Flow decay.
EncScript_Suicune_Purify:
	encsetvar 13, 1   @ Swept: one purification per turn, whichever trigger asked for it
	printstring STRINGID_ENCSUICUNEPURIFICATION
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SURF
	waitanimation
	jumpifhalfword CMP_EQUAL, gBattleWeather, B_WEATHER_NONE, EncScript_Suicune_Purify_Terrain
	removeweather
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESWEATHER
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_DEFOG
	waitanimation
EncScript_Suicune_Purify_Terrain:
	jumpifhalfword CMP_NO_COMMON_BITS, gFieldStatuses, STATUS_FIELD_TERRAIN_ANY, EncScript_Suicune_Purify_Screens
	removeterrain
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESTERRAIN
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_DEFOG
	waitanimation
EncScript_Suicune_Purify_Screens:
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Suicune_Purify_Hazards
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESSCREENS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_BRICK_BREAK
	waitanimation
EncScript_Suicune_Purify_Hazards:
	trytidyup FALSE, EncScript_Suicune_Purify_HazardsFound
	goto EncScript_Suicune_Purify_Status
EncScript_Suicune_Purify_HazardsFound:
	encaddvar 6, 1
	@ trytidyup TRUE does not advance while it still has something to clear, so this one instruction
	@ is the whole loop; the engine supplies its own removal and Substitute-fade messaging.
	trytidyup TRUE, EncScript_Suicune_Purify_Status
EncScript_Suicune_Purify_Status:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Suicune_Purify_StatusFound
	goto EncScript_Suicune_Purify_Undertow
EncScript_Suicune_Purify_StatusFound:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESSTATUS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	@ From The Cleansing on, the sweep drags a stage of Speed off the player. This is Suicune's only
	@ hold on the player's stat line now that stages are not impurities, and the drops accumulate -
	@ the sweep's old Haze used to reset them to neutral immediately before reapplying -1, which
	@ pinned the undertow at one stage forever and then paid Suicune a point of Flow next sweep for
	@ the very stage it had just applied itself.
EncScript_Suicune_Purify_Undertow:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Suicune_Purify_Flow
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	printstring STRINGID_ENCSUICUNEUNDERTOW
	waitmessage B_WAIT_TIME_SHORT
	@ One point of Flow per thing washed away, five impurities at most. Unrolled rather than looped so
	@ Cleansed survives for TurnClose, which reads it to decide whether this turn earned a Flow decay.
EncScript_Suicune_Purify_Flow:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Suicune_Purify_Guard
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 2, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 3, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 4, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 5, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
EncScript_Suicune_Purify_Paid:
	call EncScript_Suicune_FlowCallout
EncScript_Suicune_Purify_Guard:
	call EncScript_Suicune_ApplyGuard
	return

// From Flow 4, and unconditionally in Sacred Beast, Suicune sheds status on its own - which is what
// makes locking it down self-defeating rather than impossible. Silent no-op when it has none.
EncScript_Suicune_ShedStatus:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Suicune_ShedStatus_Cure
	return
EncScript_Suicune_ShedStatus_Cure:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCSUICUNESHEDS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// End-of-turn mending, on a percentage so it scales with whatever LevelCap built. The Flow threshold
// drops a rung each phase; Sacred Beast mends every turn regardless.
EncScript_Suicune_Regen:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Suicune_Regen_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Suicune_Regen_P1
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Suicune_Regen_Big
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Suicune_Regen_Small
	return
EncScript_Suicune_Regen_P1:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Regen_Big
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Regen_Small
	return
EncScript_Suicune_Regen_P2:
	enchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	goto EncScript_Suicune_Regen_Show
EncScript_Suicune_Regen_Big:
	enchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	goto EncScript_Suicune_Regen_Show
EncScript_Suicune_Regen_Small:
	enchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
EncScript_Suicune_Regen_Show:
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCSUICUNEMENDS
	waitmessage B_WAIT_TIME_SHORT
	return

// Shared body of the two shatter triggers. Reached by goto, not call, so its return pops the trigger
// dispatch frame like any other trigger script's would.
EncScript_Suicune_ShatterCore:
	printstring STRINGID_ENCSUICUNESHATTERED
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_HYDRO_PUMP
	waitanimation
	encsetvar 1, 0    @ Flow
	encsetvar 3, 0    @ Draw
	encsetvar 4, 2    @ Break: two turns of open guard
	encsetvar 10, 2   @ Breach: shattered purity
	call EncScript_Suicune_ApplyGuard
	return

// --- Trigger scripts ---

// LastGuard is seeded to the Properties reduction so the first real change reads as a change. The
// second line is the hint that the player's own effects, not their attacks, are what Suicune is
// reacting to.
EncScript_Suicune_Intro::
	encsetvar 5, 86
	encsetvar 2, 3   @ Rite armed
	printstring STRINGID_ENCSUICUNEINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCSUICUNEWATCHES
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn scratch
// and guards, sheds status where Flow or the phase says to, and keeps the two recurring callouts
// running - the open window, and the standing "the water is still" cue that Flow is draining.
EncScript_Suicune_TurnOpen::
	flushtextbox
	encsetvar 7, 1    @ TurnGuard
	encsetvar 6, 0    @ Cleansed
	encsetvar 8, 0    @ Shatter
	encsetvar 13, 0   @ Swept
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Suicune_TurnOpen_Phase
	encsetvar 3, 0    @ a spent draw rests one turn before Suicune may draw again
EncScript_Suicune_TurnOpen_Phase:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Suicune_TurnOpen_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Suicune_TurnOpen_Shed
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Suicune_TurnOpen_Break
EncScript_Suicune_TurnOpen_Shed:
	call EncScript_Suicune_ShedStatus
EncScript_Suicune_TurnOpen_Break:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Suicune_TurnOpen_Calm
	printstring STRINGID_ENCSUICUNEUNSTEADY
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Suicune_TurnOpen_Done
EncScript_Suicune_TurnOpen_Calm:
	encjumpifvar CMP_LESS_THAN, 9, 2, EncScript_Suicune_TurnOpen_Done
	printstring STRINGID_ENCSUICUNESTILL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Suicune_TurnOpen_Done:
	return

// The scheduled purification. TurnClose steps Rite 3->2->1 and this resolves it at 1, one checkpoint
// later, so a real turn always passes between a tick and the sweep.
EncScript_Suicune_Rite::
	flushtextbox
	call EncScript_Suicune_Purify
	encsetvar 2, 3
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Suicune_Rite_Done
	encsetvar 2, 2   @ The Cleansing: the rite comes round faster
EncScript_Suicune_Rite_Done:
	return

// Sacred Water, telegraphed a full turn ahead. The field is wiped first so the counterplay is
// legible: anything standing on it next turn is unambiguously the player's own doing.
EncScript_Suicune_SacredDraw::
	flushtextbox
	call EncScript_Suicune_Purify
	encsetvar 3, 2
	printstring STRINGID_ENCSUICUNEDRAWS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_AQUA_RING
	waitanimation
	return

// The inversion: a 10% heal that only lands off a clean field, so on this one turn the player has to
// spend the Flow they have starved all fight. Dirtying it instead breaks the draw wide open.
EncScript_Suicune_SacredResolve::
	flushtextbox
	@ Draw 3 is the "spent this turn" sentinel, cleared by the next TurnOpen. Priority alone would not
	@ hold: every trigger is re-evaluated after each script, so plain 0 would let the Flow-5 draw
	@ re-telegraph in this same dispatch.
	encsetvar 3, 3
	call EncScript_Suicune_Impure
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Suicune_SacredResolve_Break
	printstring STRINGID_ENCSUICUNESACREDWATER
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_BOSS, 10, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return
EncScript_Suicune_SacredResolve_Break:
	printstring STRINGID_ENCSUICUNEBROKEN
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MUDDY_WATER
	waitanimation
	encsetvar 1, 2    @ Flow knocked back
	encsetvar 4, 1    @ Break: this turn only
	encsetvar 10, 1   @ Breach: broken draw
	call EncScript_Suicune_ApplyGuard
	return

// Grass or Electric into a Flow-0 Sacred Beast. Gating the type door behind Flow 0 is what turns
// "bring the counter-type" into the conclusion of the Purity puzzle rather than a checklist item.
EncScript_Suicune_ShatterType::
	encsetvar 8, 1   @ Shatter: one shatter attempt per turn
	goto EncScript_Suicune_ShatterCore

// The lucky second door. The crit test cannot be expressed as a trigger condition, so it is checked
// in-script; a non-crit still consumes the guard rather than letting the trigger re-fire.
EncScript_Suicune_ShatterCrit::
	encsetvar 8, 1
	jumpifnotcriticalhit EncScript_Suicune_ShatterCrit_Done
	goto EncScript_Suicune_ShatterCore
EncScript_Suicune_ShatterCrit_Done:
	return

// Sacred Beast purifies at the end of every turn instead of on the Rite timer. Purify sets Swept,
// which is this trigger's self-disabling gate; TurnOpen re-arms it.
EncScript_Suicune_StillWater::
	call EncScript_Suicune_Purify
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown in one place, then the Flow decay. Gating decay on Cleansed == 0 is load-bearing: a
// purification turn ends with a clean field by definition, and without the guard it would instantly
// refund the Flow it had just paid out.
EncScript_Suicune_TurnClose::
	encsetvar 7, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Suicune_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Suicune_TurnClose_Draw
	encsubvar 2, 1
EncScript_Suicune_TurnClose_Draw:
	encjumpifvar CMP_NOT_EQUAL, 3, 2, EncScript_Suicune_TurnClose_Break   @ 3 is the spent sentinel
	encsubvar 3, 1
EncScript_Suicune_TurnClose_Break:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Suicune_TurnClose_Shed
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Suicune_TurnClose_Shed
	encsetvar 10, 0   @ Breach cleared: ApplyGuard re-announces the guard coming back up
	call EncScript_Suicune_ApplyGuard
EncScript_Suicune_TurnClose_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Suicune_TurnClose_Regen
	call EncScript_Suicune_ShedStatus
EncScript_Suicune_TurnClose_Regen:
	call EncScript_Suicune_Regen
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Suicune_TurnClose_Dirty
	call EncScript_Suicune_Impure
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Suicune_TurnClose_Dirty
	call EncScript_Suicune_LoseFlow
	encjumpifvar CMP_GREATER_THAN, 9, 9, EncScript_Suicune_TurnClose_Calm
	encaddvar 9, 1   @ Calm, capped so a long fight cannot wrap the byte
EncScript_Suicune_TurnClose_Calm:
	call EncScript_Suicune_ApplyGuard
	goto EncScript_Suicune_TurnClose_Done
EncScript_Suicune_TurnClose_Dirty:
	encsetvar 9, 0   @ Calm
EncScript_Suicune_TurnClose_Done:
	return

// Phase 1 at 50% HP. The rite comes round faster and every sweep from here also drags a stage of
// Speed off the player, so a clean, patient game is no longer a free one.
EncScript_Suicune_Cleansing::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCSUICUNECLEANSING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SURF
	waitanimation
	@ The banner beat always plays, but the sweep is skipped if a Rite already swept this turn - the
	@ threshold can be crossed by a move on a turn that opened with a scheduled purification, and
	@ running the sequence twice back to back reads as a bug and takes two stages of Speed.
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Suicune_Cleansing_Rite
	call EncScript_Suicune_Purify
EncScript_Suicune_Cleansing_Rite:
	encsetvar 2, 2
	return

// Phase 2 at 20% HP. Purification moves to the end of every turn and the guard shuts at any Flow
// above 0, which is the moment the player learns the door only opens from empty. Per the spec there
// is no aggressive animation here - it simply stops, and everything goes calm.
EncScript_Suicune_SacredBeast::
	encsetvar 0, 2   @ Phase 2
	encsetvar 2, 0   @ Rite: StillWater takes over
	encsetvar 3, 0   @ Draw
	encsetsurvive ENC_TARGET_BOSS, TRUE
	printstring STRINGID_ENCSUICUNESACREDBEAST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Suicune_ApplyGuard
	return

// Below 10% in phase 2. Everything off, and Survive released so the catch window is a real one.
// CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the window.
EncScript_Suicune_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Flow
	encsetvar 2, 0   @ Rite
	encsetvar 3, 0   @ Draw
	encsetvar 4, 0   @ Break
	encsetvar 9, 0   @ Calm
	encsetvar 10, 0  @ Breach
	encsetvar 13, 0  @ Swept
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 5, 78
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCSUICUNEWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
