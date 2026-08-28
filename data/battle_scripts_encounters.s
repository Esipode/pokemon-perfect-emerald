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
	playanimation BS_OPPONENT1, B_ANIM_TERA_CHARGE
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
