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

// ---------------------------------------------------------------------------------------------
// Celebi, "The Guardian of Time" (src/data/battle_encounters.encounter). Var indices, pinned by
// the always-true Conditions on EncScript_Celebi_Intro:
// 0 Phase (0 Guardian of Time / 1 Future Sight / 2 Temporal Collapse / 3 Weakened),
// 1 Echo (recorded HP percentage - the rewind target; 101 means nothing recorded yet),
// 2 Cycle (0 idle, else 3->2->1 resolved at 1, 4 = spent sentinel), 3 Window (0 closed, 1 open,
// 2 anchored this turn), 4 Anchored (0-5), 5 Pred (0 clouded / 1 physical / 2 special / 3 status /
// 4 consumed / 5 unread), 6 Broken (turns of open guard left), 7 LastGuard (reduction last
// ANNOUNCED - the re-fire latch), 8 TurnGuard, 9 PEcho (player HP percentage at record time),
// 10 Delta (scratch: player recovery since PEcho), 11 Collapsed, 12 Prev (ApplyGuard's before-value
// scratch), 13 Breach (which opening is live: 1 broken future, 2 failed collapse).
//
// Everything hangs off Echo. A record window opens, Celebi snapshots its HP, and two turns later
// TIME REWIND snaps its HP back to that number - so damage is not permanent until it is recorded.
// The snapshot is monotone downward, and re-taken whenever the player lands a hit during the window
// (an ANCHOR), which is what makes "when does my damage count?" a question the player controls
// rather than one the script answers. Phase 1 surfaces the AI's real move prediction and pays the
// player for defying it; phase 2 shortens the cycle and ends in TIME COLLAPSE, which the anchors
// banked earlier are what survive.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. Prev holds the value last announced so each
// leaf can write LastGuard before comparing; there is no UI for damage reduction, so a single
// showing would be trivially missed and the line has to re-fire on every real change.
EncScript_Celebi_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Celebi_ApplyGuard_Done   @ weakened owns its own guard
	enccopyvar 12, 7
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Celebi_Guard_Open
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Celebi_Guard92
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Celebi_Guard88
	goto EncScript_Celebi_Guard84
// A failed TIME COLLAPSE opens Celebi further than a broken prediction does - it is the payoff for
// a whole fight's worth of anchoring, not a single good read.
EncScript_Celebi_Guard_Open:
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Celebi_Guard72
	goto EncScript_Celebi_Guard78

EncScript_Celebi_Guard72:
	encsetdamagereduction ENC_TARGET_BOSS, 72
	encsetvar 7, 72
	encjumpifvar CMP_EQUAL, 12, 72, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 72, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 7, 78
	encjumpifvar CMP_EQUAL, 12, 78, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 78, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 7, 84
	encjumpifvar CMP_EQUAL, 12, 84, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 84, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 7, 88
	encjumpifvar CMP_EQUAL, 12, 88, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 88, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 7, 92
	encjumpifvar CMP_EQUAL, 12, 92, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 92, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin

EncScript_Celebi_ApplyGuard_Thicken:
	printstring STRINGID_ENCCELEBITHICKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Celebi_ApplyGuard_Thin:
	printstring STRINGID_ENCCELEBITHINS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Celebi_ApplyGuard_Done:
	return

// Opens a record window. ENC_SNAP_LOWEST is what makes Echo a monotone floor: the mark only ever
// moves down, so damage dealt while the window is open is damage the rewind can never take back.
// PEcho is the player's own HP at the same instant, which is the reference Paradox measures
// recovery against.
EncScript_Celebi_Record:
	encsnapshothp ENC_TARGET_BOSS, 1, ENC_SNAP_LOWEST
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_SET
	encsetvar 3, 1   @ Window open
	printstring STRINGID_ENCCELEBIRECORDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	return

// TIME REWIND. Celebi's HP is moved back TO the recorded percentage - which heals it in the usual
// case and cuts it back down if it has climbed above the mark since. Stat stages revert on both
// sides and any status inflicted since the echo is gone with it: the restore is the whole battle
// state Celebi can reach, not just its own health bar.
EncScript_Celebi_DoRewind:
	printstring STRINGID_ENCCELEBIREWIND
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encrewindhp ENC_TARGET_BOSS, 1
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	normalisebuffs
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCCELEBIRESTORED
	waitmessage B_WAIT_TIME_SHORT
	return

// TEMPORAL PARADOX - the phase-1+ alternative that REPLACES a rewind rather than adding to it. The
// player's damage survives, but whatever they did during the cycle comes back around. Three
// branches in priority order: a recovery Celebi copies, an ascent Celebi copies, or the blow
// returning out of the past. Re-running the player's actual move isn't reachable from a script
// (that is an EFFECT_INSTRUCT problem), so the outcome is what repeats.
EncScript_Celebi_DoParadox:
	printstring STRINGID_ENCCELEBIPARADOX
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	@ Delta = how much the player has healed since the echo. ENC_SNAP_RECOVERY does the var-to-var
	@ subtraction a script can't express, and jumps out when there was none.
	enccopyvar 10, 9
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 10, ENC_SNAP_RECOVERY, EncScript_Celebi_DoParadox_Stats
	encjumpifvar CMP_LESS_THAN, 10, 5, EncScript_Celebi_DoParadox_Stats
	enchangehp ENC_TARGET_BOSS, 10, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	printstring STRINGID_ENCCELEBIPARADOXHEAL
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Celebi_DoParadox_Stats:
	jumpifstat BS_PLAYER1, CMP_GREATER_THAN, STAT_ATK, DEFAULT_STAT_STAGE, EncScript_Celebi_DoParadox_Copy
	jumpifstat BS_PLAYER1, CMP_GREATER_THAN, STAT_SPATK, DEFAULT_STAT_STAGE, EncScript_Celebi_DoParadox_Copy
	jumpifstat BS_PLAYER1, CMP_GREATER_THAN, STAT_SPEED, DEFAULT_STAT_STAGE, EncScript_Celebi_DoParadox_Copy
	goto EncScript_Celebi_DoParadox_Blow
EncScript_Celebi_DoParadox_Copy:
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCCELEBIPARADOXSTAT
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Celebi_DoParadox_Blow:
	printstring STRINGID_ENCCELEBIPARADOXBLOW
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return

// Temporal Collapse sheds status on its own - locking Celebi down is temporary by construction
// anyway, since every rewind cures it. Silent no-op when it has none.
EncScript_Celebi_Shed:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Celebi_Shed_Cure
	return
EncScript_Celebi_Shed_Cure:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCCELEBISHEDS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// --- Trigger scripts ---

// Echo starts at 101 rather than 0 on purpose: ENC_SNAP_LOWEST only writes a value lower than what
// the var holds, so the first snapshot has to find something above any real percentage. LastGuard
// is seeded to the Properties reduction so the first real change reads as a change. The window
// itself is left to turn 1's OpenWindow, so the player can act inside the first record turn.
EncScript_Celebi_Intro::
	encsetvar 1, 101
	encsetvar 7, 86
	encsetvar 5, 5    @ Pred: unread
	printstring STRINGID_ENCCELEBIINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCCELEBIWATCHES
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn
// scratch, re-arms the prediction, and keeps the open-window callout running.
EncScript_Celebi_TurnOpen::
	flushtextbox
	encsetvar 8, 1   @ TurnGuard
	encsetvar 3, 0   @ Window closed
	encjumpifvar CMP_NOT_EQUAL, 2, 4, EncScript_Celebi_TurnOpen_Phase
	encsetvar 2, 0   @ a spent cycle clears here, so a rewind turn and a record turn are never one turn
EncScript_Celebi_TurnOpen_Phase:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Celebi_TurnOpen_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Celebi_TurnOpen_Broken
	encsetvar 5, 5   @ Pred: unread, so Foresee runs this turn
EncScript_Celebi_TurnOpen_Broken:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Celebi_TurnOpen_Shed
	printstring STRINGID_ENCCELEBIUNSETTLED
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Celebi_TurnOpen_Done
EncScript_Celebi_TurnOpen_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Celebi_TurnOpen_Done
	call EncScript_Celebi_Shed
EncScript_Celebi_TurnOpen_Done:
	return

// The rewind resolves the countdown TurnClose stepped, one checkpoint later, so a real turn always
// passes between a tick and the snap-back. It resolves to the spent sentinel 4 first: every trigger
// is re-evaluated after each script, so writing 0 here would re-arm OpenWindow in this same
// dispatch and record the post-rewind HP as a brand new window.
EncScript_Celebi_Rewind::
	flushtextbox
	encsetvar 2, 4
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Celebi_Rewind_Plain
	encjumpifchance 35, EncScript_Celebi_Rewind_Paradox
EncScript_Celebi_Rewind_Plain:
	call EncScript_Celebi_DoRewind
	return
EncScript_Celebi_Rewind_Paradox:
	call EncScript_Celebi_DoParadox
	return

// Behind the rewind at priority 30, so on a turn that does both the snapshot sees the post-rewind
// value rather than the damage the rewind is about to erase.
EncScript_Celebi_OpenWindow::
	flushtextbox
	call EncScript_Celebi_Record
	encsetvar 2, 3
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Celebi_OpenWindow_Done
	encsetvar 2, 2   @ Temporal Collapse: the cycle comes round faster
EncScript_Celebi_OpenWindow_Done:
	return

// The prediction, surfaced. encstoreprediction reads the AI's real AI_FLAG_PREDICT_MOVE answer for
// the player, which is computed during action selection - so this is a genuine read-ahead, not a
// scripted fake, and the player can invalidate it by choosing differently. Writing Pred is also
// this trigger's own self-disabling gate.
EncScript_Celebi_Foresee::
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 5
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Celebi_Foresee_Phys
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Celebi_Foresee_Spec
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Celebi_Foresee_Status
	printstring STRINGID_ENCCELEBIFORESEENONE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Celebi_Foresee_Phys:
	printstring STRINGID_ENCCELEBIFORESEEPHYS
	goto EncScript_Celebi_Foresee_Show
EncScript_Celebi_Foresee_Spec:
	printstring STRINGID_ENCCELEBIFORESEESPEC
	goto EncScript_Celebi_Foresee_Show
EncScript_Celebi_Foresee_Status:
	printstring STRINGID_ENCCELEBIFORESEESTAT
EncScript_Celebi_Foresee_Show:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	return

// Damaging Celebi while the window is open re-takes the mark at the lower value, which is what
// permanently banks the damage. encsnapshothp's failInstr does the whole test: if the mark didn't
// move, the hit did nothing worth recording (a miss, an immunity, a status move) and no anchor is
// claimed. Window 2 is the consumed value its own trigger condition rejects - one attempt per turn,
// as an event condition stays true for the whole dispatch.
EncScript_Celebi_Anchor::
	encsetvar 3, 2
	encsnapshothp ENC_TARGET_BOSS, 1, ENC_SNAP_LOWEST, EncScript_Celebi_Anchor_Done
	encjumpifvar CMP_GREATER_THAN, 4, 4, EncScript_Celebi_Anchor_Show
	encaddvar 4, 1
EncScript_Celebi_Anchor_Show:
	printstring STRINGID_ENCCELEBIANCHOR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
EncScript_Celebi_Anchor_Done:
	return

// Shared by the three matched-category triggers. The foreseen future arrives sooner as well as
// hitting: a correct read pulls the next rewind a turn closer.
EncScript_Celebi_PredHeld::
	encsetvar 5, 4   @ Pred consumed
	printstring STRINGID_ENCCELEBIPREDHELD
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Celebi_PredHeld_Done
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Celebi_PredHeld_Done   @ the spent sentinel isn't a countdown
	encsubvar 2, 1
EncScript_Celebi_PredHeld_Done:
	return

// The catch-all at priority 30: reached only when none of the three matched triggers claimed the
// dispatch, i.e. the player played a category Celebi did not foresee. The pending rewind is
// cancelled outright and the guard falls for two turns - this is what makes reading the prediction
// worth doing rather than an announcement to ignore.
EncScript_Celebi_PredBroken::
	encsetvar 5, 4   @ Pred consumed
	printstring STRINGID_ENCCELEBIPREDBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 6, 2    @ Broken: two turns of open guard
	encsetvar 13, 1   @ Breach: broken future
	encsetvar 2, 4    @ the pending rewind is cancelled
	call EncScript_Celebi_ApplyGuard
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown in one place, and the phase-2 end-of-turn shed.
EncScript_Celebi_TurnClose::
	encsetvar 8, 0   @ TurnGuard
	encsetvar 3, 0   @ Window closed
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Celebi_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Celebi_TurnClose_Broken
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Celebi_TurnClose_Broken   @ 4 is the spent sentinel
	encsubvar 2, 1
EncScript_Celebi_TurnClose_Broken:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Celebi_TurnClose_Shed
	encsubvar 6, 1
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Celebi_TurnClose_Done
	encsetvar 13, 0   @ Breach cleared: ApplyGuard re-announces the guard closing again
	call EncScript_Celebi_ApplyGuard
	goto EncScript_Celebi_TurnClose_Done
EncScript_Celebi_TurnClose_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Celebi_TurnClose_Done
	call EncScript_Celebi_Shed
EncScript_Celebi_TurnClose_Done:
	return

// Phase 1 at 60% HP. Celebi stops reacting and starts reading ahead: from here every turn opens
// with a stated prediction, and the fight gains a second layer the player can play against.
EncScript_Celebi_FutureSight::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCCELEBIFUTURESIGHT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 5, 5   @ Pred: unread, so the foresight starts next turn
	call EncScript_Celebi_ApplyGuard
	return

// Phase 2 at 25% HP. The cycle shortens, the past bleeds back onto the field as a weather that is
// no longer falling anywhere else, and Survive guarantees the TIME COLLAPSE beat is reachable.
EncScript_Celebi_Collapse::
	encsetvar 0, 2   @ Phase 2
	printstring STRINGID_ENCCELEBICOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetweather BATTLE_WEATHER_RAIN
	playanimation BS_PLAYER1, B_ANIM_RAIN_CONTINUES
	printstring STRINGID_ENCCELEBIWEATHER
	waitmessage B_WAIT_TIME_LONG
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encjumpifvar CMP_NOT_EQUAL, 2, 3, EncScript_Celebi_Collapse_Guard
	encsetvar 2, 2   @ shorten a cycle already in flight
EncScript_Celebi_Collapse_Guard:
	call EncScript_Celebi_ApplyGuard
	return

// TIME COLLAPSE at 13% HP - one final rewind, and the anchors banked all fight are what decides how
// much of it lands. Collapsed is set in every branch, which is what unlocks the weakened trigger:
// the finale always plays before the catch window opens, even when one hit crosses both thresholds.
EncScript_Celebi_TimeCollapse::
	encsetvar 11, 1   @ Collapsed
	printstring STRINGID_ENCCELEBITIMECOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 2, 4    @ nothing else resolves this turn
	encjumpifvar CMP_GREATER_THAN, 4, 2, EncScript_Celebi_TimeCollapse_Held
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Celebi_TimeCollapse_Partial
	printstring STRINGID_ENCCELEBICOLLAPSEFULL
	waitmessage B_WAIT_TIME_LONG
	encrewindhp ENC_TARGET_BOSS, 1
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	normalisebuffs
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return
EncScript_Celebi_TimeCollapse_Partial:
	printstring STRINGID_ENCCELEBICOLLAPSEPART
	waitmessage B_WAIT_TIME_LONG
	encrewindhp ENC_TARGET_BOSS, 1
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	normalisebuffs
	encsetvar 6, 1    @ Broken: one turn of open guard
	encsetvar 13, 1
	call EncScript_Celebi_ApplyGuard
	return
// Three or more anchors and the collapse finds nothing to reach past. No heal, and Celebi is left
// wide open for three turns - the mechanic it introduced on turn one is what beats it.
EncScript_Celebi_TimeCollapse_Held:
	printstring STRINGID_ENCCELEBICOLLAPSEHELD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encsetvar 6, 3    @ Broken: three turns of open guard
	encsetvar 13, 2   @ Breach: the collapse failed
	call EncScript_Celebi_ApplyGuard
	return

// Below 10% once the collapse has played. Everything off, and Survive released so the catch window
// is a real one. CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the window.
EncScript_Celebi_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 2, 0   @ Cycle
	encsetvar 3, 0   @ Window
	encsetvar 5, 5   @ Pred
	encsetvar 6, 0   @ Broken
	encsetvar 13, 0  @ Breach
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	removeweather
	encsetdamagereduction ENC_TARGET_BOSS, 75
	encsetvar 7, 75
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCCELEBIWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// ---------------------------------------------------------------------------------------------
// Lugia, "The Storm Beneath the Sea" (src/data/battle_encounters.encounter). Var indices, pinned by
// the always-true Conditions on EncScript_Lugia_Intro:
// 0 Phase (0 Guardian of the Sea / 1 The Sea Erupts / 2 Ocean's Wrath / 3 Weakened), 1 Storm (0-5),
// 2 Strain (0-6), 3 Eye (0 closed, else 2->1 countdown), 4 Hold (Maelstrom cycle timer; 0 idle,
// 3 surface, 2 deep sea, 1 strike), 5 Break (turns of collapsed guard left), 6 LastGuard (reduction
// last ANNOUNCED), 7 LastStorm (storm tier last APPLIED), 8 TurnGuard, 9 Rise (per-turn guard shared
// by the two storm-rise triggers), 10 Vent, 11 Swell, 12 HpMark (boss HP percentage at turn open;
// ENC_SNAP_DAMAGE turns it into "lost this turn"), 13 Prev (ApplyGuard's before-value scratch).
//
// Storm is a ladder of real battle weather that Lugia climbs itself by attacking, and the fight is
// built on an inversion: calm Lugia is SOFTER than storming Lugia but never tires, while storming
// Lugia is nearly untouchable and chips the field every turn - and accrues Strain. Capping Strain
// collapses the guard to 60 for two turns, which is the only place real damage happens. The
// Maelstrom cycle closes on its own into a restful Eye that sheds Strain, so the player is racing
// the cycle, not surviving it. Turtling through a Maelstrom is survivable and completely useless.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. Prev holds the value last announced so each
// leaf can write LastGuard before comparing; there is no UI for damage reduction, so a single
// showing would be trivially missed and the line has to re-fire on every real change.
EncScript_Lugia_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Lugia_ApplyGuard_Done   @ weakened owns its own guard
	enccopyvar 13, 6
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Lugia_Guard70    @ Strain collapse
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Lugia_Guard89    @ the Eye is a door, not a window
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Lugia_Guard_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Lugia_Guard_P1
	@ Phase 0 caps the ladder at Storm 3, so it only has the two rungs.
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Lugia_Guard86
	goto EncScript_Lugia_Guard80
EncScript_Lugia_Guard_P1:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Lugia_Guard90
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Guard88
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Lugia_Guard84
	goto EncScript_Lugia_Guard78
EncScript_Lugia_Guard_P2:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Lugia_Guard89
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Guard87
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Lugia_Guard82
	goto EncScript_Lugia_Guard74

EncScript_Lugia_Guard70:
	encsetdamagereduction ENC_TARGET_BOSS, 70
	encsetvar 6, 70
	encjumpifvar CMP_EQUAL, 13, 70, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 70, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard74:
	encsetdamagereduction ENC_TARGET_BOSS, 74
	encsetvar 6, 74
	encjumpifvar CMP_EQUAL, 13, 74, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 74, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 6, 78
	encjumpifvar CMP_EQUAL, 13, 78, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 78, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 6, 80
	encjumpifvar CMP_EQUAL, 13, 80, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 80, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard82:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetvar 6, 82
	encjumpifvar CMP_EQUAL, 13, 82, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 82, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 6, 84
	encjumpifvar CMP_EQUAL, 13, 84, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 84, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 6, 86
	encjumpifvar CMP_EQUAL, 13, 86, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 86, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard87:
	encsetdamagereduction ENC_TARGET_BOSS, 87
	encsetvar 6, 87
	encjumpifvar CMP_EQUAL, 13, 87, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 87, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 6, 88
	encjumpifvar CMP_EQUAL, 13, 88, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 88, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	encsetvar 6, 89
	encjumpifvar CMP_EQUAL, 13, 89, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 89, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken
EncScript_Lugia_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 6, 90
	encjumpifvar CMP_EQUAL, 13, 90, EncScript_Lugia_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 13, 90, EncScript_Lugia_ApplyGuard_Thicken
	goto EncScript_Lugia_ApplyGuard_Slacken

EncScript_Lugia_ApplyGuard_Thicken:
	printstring STRINGID_ENCLUGIATHICKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Lugia_ApplyGuard_Slacken:
	printstring STRINGID_ENCLUGIASLACKENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Lugia_ApplyGuard_Done:
	return

// The weather half of the ladder, latched on LastStorm so it is safe to call unconditionally - a
// tier that has not moved returns immediately, which is what lets the player's own weather survive
// a quiet turn. The removeweather first is load-bearing: TryChangeBattleWeather refuses any change
// while primal weather is up, so stepping DOWN from Tempest or Maelstrom is impossible without
// clearing first. RemoveAllWeather does clear primal, and prints nothing on its own.
EncScript_Lugia_ApplyStorm:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Lugia_Storm0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Lugia_Storm1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Lugia_Storm2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Lugia_Storm3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Storm4
	goto EncScript_Lugia_Storm5

EncScript_Lugia_Storm0:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetvar 7, 0
	printstring STRINGID_ENCLUGIACALM
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_ApplyStorm_Guard
EncScript_Lugia_Storm1:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetvar 7, 1
	printstring STRINGID_ENCLUGIABREEZE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_ApplyStorm_Guard
	@ From here the weather does the work no scripted modifier has to: rain boosts Lugia's Hydro Pump
	@ and makes its Thunder unmissable.
EncScript_Lugia_Storm2:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_RAIN, 0
	encsetvar 7, 2
	printstring STRINGID_ENCLUGIARAIN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	goto EncScript_Lugia_ApplyStorm_Guard
EncScript_Lugia_Storm3:
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_RAIN, 0
	encsetvar 7, 3
	printstring STRINGID_ENCLUGIASTORM
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	goto EncScript_Lugia_ApplyStorm_Guard
	@ Strong Winds clamps any >= 2x multiplier against a Flying defender back to neutral, so Tempest
	@ cancels exactly Lugia's Flying weaknesses - Electric, Ice and Rock stop being super effective -
	@ and leaves Ghost and Dark still hitting for 2x through its Psychic half. The player's coverage
	@ is rearranged, not walled. It is also primal-class, so the vent door shuts here.
EncScript_Lugia_Storm4:
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_STRONG_WINDS, 0
	encsetvar 7, 4
	printstring STRINGID_ENCLUGIATEMPEST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	goto EncScript_Lugia_ApplyStorm_Guard
EncScript_Lugia_Storm5:
	encjumpifvar CMP_EQUAL, 7, 5, EncScript_Lugia_ApplyStorm_Done
	removeweather
	encsetweather BATTLE_WEATHER_RAIN_PRIMAL, 0
	encsetvar 7, 5
	printstring STRINGID_ENCLUGIASEAWEATHER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
EncScript_Lugia_ApplyStorm_Guard:
	call EncScript_Lugia_ApplyGuard
EncScript_Lugia_ApplyStorm_Done:
	return

// Clamped +1 against the phase cap - phase 0 stops at Storm 3, so the Tempest and the Maelstrom are
// things The Sea Erupts unlocks rather than things turn three can hand out.
EncScript_Lugia_RaiseStorm:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Lugia_RaiseStorm_Early
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Lugia_RaiseStorm_Done
	goto EncScript_Lugia_RaiseStorm_Rise
EncScript_Lugia_RaiseStorm_Early:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Lugia_RaiseStorm_Done
EncScript_Lugia_RaiseStorm_Rise:
	encaddvar 1, 1
	call EncScript_Lugia_ApplyStorm
EncScript_Lugia_RaiseStorm_Done:
	return

// Clamped 0-6. Silent: the recurring "its wings falter" cue lives in TurnOpen, so the player gets
// one reading a turn instead of one per point.
EncScript_Lugia_GainStrain:
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Lugia_GainStrain_Done
	encaddvar 2, 1
EncScript_Lugia_GainStrain_Done:
	return

EncScript_Lugia_LoseStrain:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Lugia_LoseStrain_Done
	encsubvar 2, 1
EncScript_Lugia_LoseStrain_Done:
	return

// The end-of-turn field chip, called from TurnClose. Scripted enchangehp rather than a move, so
// Protect and screens do not stop it. ENC_TARGET_ALL_FOES rather than a single slot: doubles-safe
// for free, and it skips a fainted member silently. A Water-type foe is spared outright, which is
// the one clean answer to the Maelstrom that does not cost the player their storm progress.
EncScript_Lugia_Maelstrom:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Lugia_Maelstrom_Done
	jumpiftype BS_PLAYER1, TYPE_WATER, EncScript_Lugia_Maelstrom_Spared
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Lugia_Maelstrom_Chip5
	jumpiftype BS_PLAYER1, TYPE_FLYING, EncScript_Lugia_Maelstrom_Flying
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Maelstrom_Chip7
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_Show
	@ Gravity is the only real grounding field state and it would ground Lugia too, opening it to
	@ Ground moves. A Flying-type foe torn out of the air by the wind is the same fiction, one
	@ jumpiftype deep, and it costs the encounter nothing.
EncScript_Lugia_Maelstrom_Flying:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Lugia_Maelstrom_Chip11
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_ShowFlying
EncScript_Lugia_Maelstrom_Chip11:
	enchangehp ENC_TARGET_ALL_FOES, -7, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_ShowFlying
EncScript_Lugia_Maelstrom_Chip7:
	enchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	goto EncScript_Lugia_Maelstrom_Show
EncScript_Lugia_Maelstrom_Chip5:
	enchangehp ENC_TARGET_ALL_FOES, -3, ENC_AMOUNT_PERCENT
EncScript_Lugia_Maelstrom_Show:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCLUGIACHIP
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_Maelstrom_Acc
EncScript_Lugia_Maelstrom_ShowFlying:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCLUGIACHIPFLYING
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_Maelstrom_Acc
EncScript_Lugia_Maelstrom_Spared:
	printstring STRINGID_ENCLUGIACHIPSPARED
	waitmessage B_WAIT_TIME_SHORT
	@ Accuracy erosion is Maelstrom-only, and it is answerable - Haze, Mist, or riding the cycle out
	@ to the Eye, which hands the stages back with a switch.
EncScript_Lugia_Maelstrom_Acc:
	encjumpifvar CMP_NOT_EQUAL, 1, 5, EncScript_Lugia_Maelstrom_Done
	encchangestat ENC_TARGET_ALL_FOES, STAT_ACC, -1
	printstring STRINGID_ENCLUGIAERODES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Lugia_Maelstrom_Done:
	return

// The payoff. Sixty is the softest state in the fight by a wide margin - roughly 2.9x the damage
// the player gets through a Tempest guard of 92 - and it is only reachable by attacking hard in the
// worst conditions the fight offers. ApplyStorm clears the weather on the way down; the explicit
// ApplyGuard after it covers the case where the tier did not actually move, and is silent when the
// tail call already announced the 70.
EncScript_Lugia_Collapse:
	printstring STRINGID_ENCLUGIACOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1   @ Storm knocked back to Breeze
	encsetvar 2, 0   @ Strain
	encsetvar 3, 0   @ Eye
	encsetvar 4, 0   @ Hold
	encsetvar 5, 2   @ Break: two full turns of collapsed guard
	enchangehp ENC_TARGET_BOSS, -7, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	call EncScript_Lugia_ApplyStorm
	call EncScript_Lugia_ApplyGuard
	return

// The conditional status immunity: while Storm >= 2 the sea will not let Lugia rest, so sleep and
// freeze are shed at the top of every turn - but below Storm 2 they stick. Sleep-locking Lugia is
// possible, and the price is venting the storm first, which is exactly the trade the fight is about.
// curestatus clears everything, so sleeping a storming Lugia also throws away the player's Toxic.
EncScript_Lugia_ShedStatus:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Lugia_ShedStatus_Done
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Lugia_ShedStatus_Cure
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Lugia_ShedStatus_Cure
	goto EncScript_Lugia_ShedStatus_Done
EncScript_Lugia_ShedStatus_Cure:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCLUGIASHEDS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Lugia_ShedStatus_Done:
	return

// --- Trigger scripts ---

// LastGuard is seeded to the Properties reduction so the first real change reads as a change. The
// second line is the hint that the weather is a second health bar, and that Lugia owns it.
EncScript_Lugia_Intro::
	encsetvar 6, 86    @ LastGuard
	encsetvar 7, 0     @ LastStorm
	encsetvar 12, 100  @ HpMark
	printstring STRINGID_ENCLUGIAINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCLUGIASKY
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn
// re-entry guards, marks the boss HP the Strain race is measured against, then keeps the three
// recurring callouts running - the collapsed guard, the Eye, and the standing Strain cue.
EncScript_Lugia_TurnOpen::
	flushtextbox
	encsetvar 8, 1    @ TurnGuard
	encsetvar 9, 0    @ Rise
	encsetvar 10, 0   @ Vent
	encsetvar 14, 0   @ Cycle
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Lugia_TurnOpen_Done   @ weakened: inert
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_SET
	call EncScript_Lugia_ShedStatus
	call EncScript_Lugia_ApplyStorm
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Lugia_TurnOpen_Eye
	printstring STRINGID_ENCLUGIAUNSTEADY
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_TurnOpen_Done
EncScript_Lugia_TurnOpen_Eye:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Lugia_TurnOpen_Strain
	printstring STRINGID_ENCLUGIAEYECALM
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_TurnOpen_Done
EncScript_Lugia_TurnOpen_Strain:
	encjumpifvar CMP_GREATER_THAN, 2, 4, EncScript_Lugia_TurnOpen_StrainHigh
	encjumpifvar CMP_LESS_THAN, 2, 3, EncScript_Lugia_TurnOpen_Done
	printstring STRINGID_ENCLUGIASTRAIN
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Lugia_TurnOpen_Done
EncScript_Lugia_TurnOpen_StrainHigh:
	printstring STRINGID_ENCLUGIASTRAINHIGH
	waitmessage B_WAIT_TIME_SHORT
EncScript_Lugia_TurnOpen_Done:
	return

// Hold 2. encsetprotect gives Lugia a real Protect, so Feint, never-miss moves and contact
// punishment all resolve exactly as they would against one, and it expires on its own. Lugia still
// attacks this turn: it is pure pressure, and the player's job is to heal, switch or set up before
// the strike lands next turn.
EncScript_Lugia_DeepSea::
	flushtextbox
	encsetvar 14, 1   @ Cycle
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCLUGIADIVE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_DIVE
	printstring STRINGID_ENCLUGIACHURN
	waitmessage B_WAIT_TIME_LONG
	return

// Hold 1. Scripted damage rather than a move, so Protect and screens do not stop it either.
EncScript_Lugia_Strike::
	flushtextbox
	encsetvar 14, 1   @ Cycle
	printstring STRINGID_ENCLUGIASTRIKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SURF
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	call EncScript_Lugia_RaiseStorm
	call EncScript_Lugia_GainStrain
	return

// Aeroblast is the ladder's biggest single push. Rise is the shared per-turn guard, so a turn that
// climbs two rungs cannot also climb a third off the surge trigger.
EncScript_Lugia_StormAeroblast::
	encsetvar 9, 1   @ Rise
	call EncScript_Lugia_RaiseStorm
	call EncScript_Lugia_RaiseStorm
	return

// Hydro Pump and Thunder each feed the storm one rung. Psychic feeds it nothing, which is the free
// turn the ladder occasionally hands back.
EncScript_Lugia_StormSurge::
	encsetvar 9, 1   @ Rise
	call EncScript_Lugia_RaiseStorm
	return

// The panic button. Venting from Storm 2-3 always lands at 0-1, which is no weather at all - so it
// washes the player's own weather away in the same breath and costs every point of progress toward
// a Strain collapse. It buys safety, and that is all it buys.
EncScript_Lugia_Vent::
	encsetvar 10, 1   @ Vent
	printstring STRINGID_ENCLUGIAVENT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_DEFOG
	encsubvar 1, 2
	call EncScript_Lugia_ApplyStorm
	return

// The Maelstrom's answer to pivot stalling.
EncScript_Lugia_Undertow::
	encsetvar 11, 1   @ Swell
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	printstring STRINGID_ENCLUGIAUNDERTOW
	waitmessage B_WAIT_TIME_SHORT
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown and the whole Strain economy, in an order that matters: Break steps down FIRST so a
// collapse armed further down this same script gets both its turns, and the Eye is checked before
// the Hold so the two halves of the cycle can never run in one dispatch.
EncScript_Lugia_TurnClose::
	encsetvar 8, 0    @ TurnGuard
	encsetvar 11, 0   @ Swell
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Lugia_TurnClose_Phase
	encsubvar 5, 1
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Lugia_TurnClose_Phase
	call EncScript_Lugia_ApplyGuard   @ the guard coming back up re-announces itself
EncScript_Lugia_TurnClose_Phase:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Lugia_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Lugia_TurnClose_Hold
	@ The Eye: no chip, no Strain gain, a small mend and a 94 guard. It is a heal-and-reposition
	@ window for both sides, deliberately too hard to be farmed as a damage window.
	enchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCLUGIAEYEMENDS
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Lugia_LoseStrain
	call EncScript_Lugia_LoseStrain
	encsubvar 3, 1
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Lugia_TurnClose_Done
	printstring STRINGID_ENCLUGIAEYECLOSES
	waitmessage B_WAIT_TIME_LONG
	@ Spending the Eye on a weather move is the reward: the storm comes back one rung instead of
	@ three or four, because the sky is already answering to somebody else.
	jumpifhalfword CMP_NOT_EQUAL, gBattleWeather, B_WEATHER_NONE, EncScript_Lugia_TurnClose_EyeHeld
	encsetvar 1, 3
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Lugia_TurnClose_EyeApply
	encsetvar 1, 4
	goto EncScript_Lugia_TurnClose_EyeApply
EncScript_Lugia_TurnClose_EyeHeld:
	encsetvar 1, 1
EncScript_Lugia_TurnClose_EyeApply:
	call EncScript_Lugia_ApplyStorm
	goto EncScript_Lugia_TurnClose_Done
EncScript_Lugia_TurnClose_Hold:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Lugia_TurnClose_Chip
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Lugia_TurnClose_Chip
	printstring STRINGID_ENCLUGIAEYEOPENS
	waitmessage B_WAIT_TIME_LONG
	encsetvar 3, 2   @ Eye
	encsetvar 1, 0   @ Storm: the cycle ends on Lugia's terms, not the player's
	call EncScript_Lugia_ApplyStorm
	goto EncScript_Lugia_TurnClose_Done
EncScript_Lugia_TurnClose_Chip:
	call EncScript_Lugia_Maelstrom
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Lugia_TurnClose_Cap
	call EncScript_Lugia_GainStrain
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Lugia_TurnClose_Heavy
	call EncScript_Lugia_GainStrain
	@ The race itself: ENC_SNAP_DAMAGE turns HpMark from "boss HP at turn open" into "percentage lost
	@ this turn", which is the only way to ask that question against a LevelCap boss whose max HP is
	@ unknown at authoring time. The failInstr fires when nothing was lost.
EncScript_Lugia_TurnClose_Heavy:
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE, EncScript_Lugia_TurnClose_Cap
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Lugia_TurnClose_HeavyP2
	encjumpifvar CMP_LESS_THAN, 12, 6, EncScript_Lugia_TurnClose_Cap
	call EncScript_Lugia_GainStrain
	call EncScript_Lugia_GainStrain
	goto EncScript_Lugia_TurnClose_Cap
EncScript_Lugia_TurnClose_HeavyP2:
	encjumpifvar CMP_LESS_THAN, 12, 4, EncScript_Lugia_TurnClose_Cap
	call EncScript_Lugia_GainStrain
	call EncScript_Lugia_GainStrain
	call EncScript_Lugia_GainStrain
EncScript_Lugia_TurnClose_Cap:
	encjumpifvar CMP_LESS_THAN, 2, 6, EncScript_Lugia_TurnClose_Arm
	call EncScript_Lugia_Collapse
	goto EncScript_Lugia_TurnClose_Done
	@ Arming the Maelstrom cycle. Checked last so a collapse in this same dispatch has already zeroed
	@ Storm out of range, and gated on Eye == 0 so the cycle cannot re-arm the instant it ends.
EncScript_Lugia_TurnClose_Arm:
	encjumpifvar CMP_NOT_EQUAL, 1, 5, EncScript_Lugia_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Lugia_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Lugia_TurnClose_Done
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Lugia_TurnClose_Done
	encsetvar 4, 3   @ Hold: surface, deep sea, strike
	printstring STRINGID_ENCLUGIAMAELSTROM
	waitmessage B_WAIT_TIME_LONG
EncScript_Lugia_TurnClose_Done:
	return

// Phase 1 at 50% HP. The storm cap comes off, so Tempest and the Maelstrom cycle become reachable,
// and Strain starts accruing - the loop is learned with half the fight still left rather than sprung
// at the end.
EncScript_Lugia_SeaErupts::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCLUGIASEAERUPTS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Lugia_RaiseStorm
	call EncScript_Lugia_ApplyGuard
	return

// Phase 2 at 25% HP. The storm is floored at Tempest, Strain accrues twice as fast, and Survive goes
// on so a 60-guard Break window cannot overshoot the catch window.
EncScript_Lugia_OceansWrath::
	encsetvar 0, 2   @ Phase 2
	encsetsurvive ENC_TARGET_BOSS, TRUE
	printstring STRINGID_ENCLUGIAOCEANSWRATH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Lugia_OceansWrath_Apply
	encsetvar 1, 4
EncScript_Lugia_OceansWrath_Apply:
	call EncScript_Lugia_ApplyStorm
	call EncScript_Lugia_ApplyGuard
	return

// Below 10% in phase 2. Everything off, and Survive released so the catch window is a real one.
// CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the window.
EncScript_Lugia_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Storm
	encsetvar 2, 0   @ Strain
	encsetvar 3, 0   @ Eye
	encsetvar 4, 0   @ Hold
	encsetvar 5, 0   @ Break
	encsetvar 7, 0   @ LastStorm
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	removeweather
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 6, 78
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCLUGIAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// ---------------------------------------------------------------------------------------------
// Ho-Oh, "The Rainbow Above the Ashes" (src/data/battle_encounters.encounter). Var indices, pinned
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

// ---------------------------------------------------------------------------------------------
// Deoxys, "The Alien Organism" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Deoxys_Intro:
// 0 Phase (0 The Organism / 1 Rapid Mutation / 2 Perfect Adaptation / 3 Cellular Collapse),
// 1 Form (0 Normal / 1 Attack / 2 Defense / 3 Speed), 2 Stress (Mutation Stress, 0-10),
// 3 Cooldown (turns until the next reconstruction), 4 Rebuild (0 idle / 3 queued / 1 reconstructing
// / 2 spent), 5 Lockout (Cellular Instability countdown, 3 -> 0), 6 Barrier (Defense Forme barrier,
// 0-3), 7 LastType (last attack bucket: 0 none / 1 Bug / 2 Ghost / 3 Dark / 4 other-physical /
// 5 other-special), 8 Assault (Attack Forme meter, 0-3), 9 LastGuard (reduction last ANNOUNCED),
// 10 TurnGuard, 11 MoveGuard (per-turn OnMoveEnd re-entry guard), 12 HpMark (boss HP percentage at
// the reconstruction open), 13 Scratch, 14 LastStress (stress TIER last announced), 15 Prev
// (ApplyGuard's before-value scratch).
//
// Deoxys reconstructs on a timer, and each reconstruction READS the player and builds the forme
// that answers what it read. Every real change costs it a point of Mutation Stress; at 5 it comes
// apart into CELLULAR INSTABILITY, collapsing to Normal Forme with a cratered guard for three
// turns. So the way to hurt it is to make it keep changing, and the way to make it keep changing is
// to keep changing what you do. Defense Forme's adaptive barrier teaches the same lesson a second
// time: it hardens against whatever kind of attack last landed and collapses the moment you switch.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. The ladder runs BACKWARDS relative to the
// formes' own bulk: Attack Forme's 20/20 defences behind a flat 90 guard would evaporate to one hit
// and Defense Forme's 160/160 would be unkillable, so the guard compensates for the shape rather
// than reinforcing it. Relative damage taken - (1 - guard/100) x (50 / defStat), against Normal
// Forme as 1.00: Attack (96) 1.43, Speed (86) 1.11, Defense (88-94) 0.54 falling to 0.27 at a full
// barrier, Instability (84) 2.29, Phase 3 (80, behind -30% defences) 4.08.
// Prev holds the value last announced so each leaf can write LastGuard before comparing; there is
// no UI for damage reduction, so the line has to re-fire on every real change.
EncScript_Deoxys_ApplyGuard:
	@ A reconstruction in flight owns the guard outright (97, set by RebuildBegin). Without this, a
	@ type-bucket script firing at OnMoveEnd during the telegraph turn would recompute the ladder and
	@ quietly undo the cover. Rebuild 2 (spent) is the resolution itself, and must recompute.
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Deoxys_ApplyGuard_Done
	enccopyvar 15, 9
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_Guard76      @ Phase 3: the collapse owns its guard
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Deoxys_Guard80  @ cellular instability
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Deoxys_Guard92      @ Attack Forme
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Guard82      @ Speed Forme
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_ApplyGuard_Def
	goto EncScript_Deoxys_Guard89                               @ Normal Forme, the reference
EncScript_Deoxys_ApplyGuard_Def:
	encjumpifvar CMP_GREATER_THAN, 6, 2, EncScript_Deoxys_Guard90
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Deoxys_Guard88
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Deoxys_Guard86
	goto EncScript_Deoxys_Guard84

EncScript_Deoxys_Guard76:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encsetvar 9, 76
	encjumpifvar CMP_EQUAL, 15, 76, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 76, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 9, 80
	encjumpifvar CMP_EQUAL, 15, 80, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 80, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard82:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetvar 9, 82
	encjumpifvar CMP_EQUAL, 15, 82, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 82, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 9, 84
	encjumpifvar CMP_EQUAL, 15, 84, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 84, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 9, 86
	encjumpifvar CMP_EQUAL, 15, 86, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 86, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 9, 88
	encjumpifvar CMP_EQUAL, 15, 88, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 88, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	encsetvar 9, 89
	encjumpifvar CMP_EQUAL, 15, 89, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 89, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 9, 90
	encjumpifvar CMP_EQUAL, 15, 90, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 90, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 9, 92
	encjumpifvar CMP_EQUAL, 15, 92, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 92, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall

EncScript_Deoxys_ApplyGuard_Rise:
	printstring STRINGID_ENCDEOXYSGUARDRISE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Deoxys_ApplyGuard_Fall:
	printstring STRINGID_ENCDEOXYSGUARDFALL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_ApplyGuard_Done:
	return

// The player's only window onto Mutation Stress - the number itself is never shown. Three tiers
// (0-3 stable, 4-7 straining, 8-10 critical) rather than eleven keeps the fight from narrating
// every point, and the latch on LastStress means it prints only when the tier actually moves.
// Silent once the cells have collapsed for good.
EncScript_Deoxys_StressTier:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_StressTier_Done
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_StressTier_T2
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Deoxys_StressTier_T1
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_Deoxys_StressTier_Done
	encsetvar 14, 0
	printstring STRINGID_ENCDEOXYSSTRESSSTABLE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Deoxys_StressTier_T1:
	encjumpifvar CMP_EQUAL, 14, 1, EncScript_Deoxys_StressTier_Done
	encsetvar 14, 1
	printstring STRINGID_ENCDEOXYSSTRESSSTRAIN
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Deoxys_StressTier_T2:
	encjumpifvar CMP_EQUAL, 14, 2, EncScript_Deoxys_StressTier_Done
	encsetvar 14, 2
	printstring STRINGID_ENCDEOXYSSTRESSCRITICAL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_StressTier_Done:
	return

// Clamped at 10, which is the point Instability reads. Announces through the tier latch rather than
// per point, so a gain inside a band is silent.
EncScript_Deoxys_GainStress:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_GainStress_Done
	encaddvar 2, 1
EncScript_Deoxys_GainStress_Done:
	call EncScript_Deoxys_StressTier
	return

// Stress scales the stance PAYLOADS, never the guard - the guard has to stay a pure function of
// forme plus barrier or the LastGuard latch starts announcing moves the line doesn't describe.
// Six bands of literals rather than a loop of small enchangehp calls, which would play six
// health-bar animations for one hit. Stress runs 0-10, so each band is two points wide except the
// top one, which is the ceiling Instability fires at:
//   0-1 / 2-3 / 4-5 / 6-7 / 8-9 / 10
// Tested in descending order with 0-1 as the fall-through, so the ladder needs no equality leaves.
EncScript_Deoxys_AssaultStrike:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_AssaultStrike_S5
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_AssaultStrike_S4
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Deoxys_AssaultStrike_S3
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Deoxys_AssaultStrike_S2
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_Deoxys_AssaultStrike_S1
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S1:
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S2:
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S3:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S4:
	enchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S5:
	enchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	return

EncScript_Deoxys_BlitzChip:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_BlitzChip_S5
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_BlitzChip_S4
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Deoxys_BlitzChip_S3
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Deoxys_BlitzChip_S2
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_Deoxys_BlitzChip_S1
	enchangehp ENC_TARGET_ALL_FOES, -2, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S1:
	enchangehp ENC_TARGET_ALL_FOES, -3, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S2:
	enchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S3:
	enchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S4:
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S5:
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	return


// The reconstruction cadence, in turns of cooldown before the telegraph turn. Phase 0 is 2 (1 for
// Normal Forme, the observing state, which never sits still for long); Phases 1 and 2 are 1, which
// is the structural floor - RebuildBegin telegraphs at one turn open and RebuildResolve fires at
// the next, so a full cycle can never be shorter than two turns. Phase 0's Normal Forme therefore
// already runs at that floor, which is why it has no separate leaf in the late phases.
EncScript_Deoxys_ResetCooldown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Deoxys_ResetCooldown_P0
	encsetvar 3, 1
	return
EncScript_Deoxys_ResetCooldown_P0:
	encsetvar 3, 2
	encjumpifvar CMP_NOT_EQUAL, 1, 0, EncScript_Deoxys_ResetCooldown_Done
	encsetvar 3, 1
EncScript_Deoxys_ResetCooldown_Done:
	return

// The raw defensive floor, and the reason this encounter is fightable at all. Deoxys has 50 base HP
// - about 210 at level 100, against 322 for Ho-Oh, Lugia and Mewtwo - so every hit is a far bigger
// slice of a far smaller bar, and Attack Forme's 20 base Def makes a super-effective STAB hit worth
// SEVEN TIMES its own max HP before reduction. A damage-reduction percentage cannot fix that: the
// last three points of the 0-99 scale are each worth a 25-50% swing, which is not a tunable knob.
// So the bulk is bought in raw stats instead, and the guard ladder above is left free to express
// the differences between the formes on a stable part of its range.
//
// Called EXACTLY ONCE after every encformchange: RecalcBattlerStats rebuilds the battle stats from
// the new species and wipes anything raw applied before it, so this neither accumulates across
// changes nor survives one. A path that did not actually change form must not call it.
// Defense Forme takes a smaller multiplier because its 160 base defences are already doing the job;
// the same one would make a full barrier take 40+ hits to break through.
EncScript_Deoxys_Bulk:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Bulk_Defense
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 180, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 180, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_Bulk_Defense:
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 110, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 110, ENC_AMOUNT_PERCENT
	return

// The shared transformation body. The caller has already written the destination into Form. Strips
// the Tailwind first so it belongs to Speed Forme rather than to the battle, then changes shape,
// pays the stress, applies the forme's entry effect and resets the timer.
EncScript_Deoxys_ApplyForm:
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encsetvar 6, 0    @ Barrier
	encsetvar 8, 0    @ Assault
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Deoxys_Form_Attack
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Form_Defense
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Form_Speed
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_NORMAL, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMNORMAL
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Deoxys_Form_Gain
EncScript_Deoxys_Form_Attack:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_ATTACK, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMATTACK
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Deoxys_Form_Gain
EncScript_Deoxys_Form_Defense:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_DEFENSE, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMDEFENSE
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Deoxys_Form_Gain
EncScript_Deoxys_Form_Speed:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_SPEED, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMSPEED
	waitmessage B_WAIT_TIME_LONG
EncScript_Deoxys_Form_Gain:
	call EncScript_Deoxys_GainStress
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Form_EntrySpeed
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Form_EntryDefense
	enchangehp ENC_TARGET_BOSS, 5, ENC_AMOUNT_PERCENT
	goto EncScript_Deoxys_Form_Cooldown
	@ Permanent - a 0 timer never ticks - and stripped again by the next form change.
EncScript_Deoxys_Form_EntrySpeed:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	goto EncScript_Deoxys_Form_Cooldown
	@ A straining organism arrives already braced.
EncScript_Deoxys_Form_EntryDefense:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_Form_Braced2
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Deoxys_Form_Braced1
	goto EncScript_Deoxys_Form_Cooldown
EncScript_Deoxys_Form_Braced2:
	encsetvar 6, 2
	goto EncScript_Deoxys_Form_Cooldown
EncScript_Deoxys_Form_Braced1:
	encsetvar 6, 1
	@ Every path, the form-change failure branches included, reaches this label: a script that left
	@ without setting Cooldown would be re-selected on the very next pass.
EncScript_Deoxys_Form_Cooldown:
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// The adaptive barrier, shared by all five type-bucket triggers. The caller has written the
// incoming bucket into Scratch. LastType is recorded in EVERY forme, not just Defense - so a player
// who nukes it with one move and then lets it build Defense Forme finds the barrier growing from
// the first turn. MoveGuard makes this once per turn, so a multi-hit move is one adaptation rather
// than five. The five compare leaves exist because encjumpifvar only ever tests a var against a
// literal; there is no var-to-var comparison.
EncScript_Deoxys_Adapt:
	encsetvar 11, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Deoxys_Adapt_Record
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Deoxys_Adapt_Break
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Deoxys_Adapt_C1
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Deoxys_Adapt_C2
	encjumpifvar CMP_EQUAL, 13, 3, EncScript_Deoxys_Adapt_C3
	encjumpifvar CMP_EQUAL, 13, 4, EncScript_Deoxys_Adapt_C4
	encjumpifvar CMP_EQUAL, 7, 5, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C1:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C2:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C3:
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C4:
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_Harden:
	encjumpifvar CMP_GREATER_THAN, 6, 2, EncScript_Deoxys_Adapt_Record
	encaddvar 6, 1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCDEOXYSHARDEN
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Deoxys_Adapt_Record
EncScript_Deoxys_Adapt_Break:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Deoxys_Adapt_Record
	encsetvar 6, 0
	printstring STRINGID_ENCDEOXYSBARRIERBREAK
	waitmessage B_WAIT_TIME_SHORT
	@ The strings never name the type - "that attack" is both better writing and honest about the
	@ five-bucket approximation.
EncScript_Deoxys_Adapt_Record:
	enccopyvar 7, 13
	call EncScript_Deoxys_ApplyGuard
	return

// --- Trigger scripts ---

// Opens in Normal Forme with a four-turn timer. LastGuard is seeded to the Properties reduction so
// the first real move in it reads as a change. The second line is the whole hint the fight rests
// on: it is rebuilding to answer YOU, and every rebuild costs it something.
EncScript_Deoxys_Intro::
	encsetvar 3, 2     @ Cooldown
	encsetvar 9, 93    @ LastGuard
	encsetvar 12, 100  @ HpMark
	@ The opening Normal Forme has never been through a form change, so it needs the floor applied
	@ here; every later forme gets it from its own encformchange.
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSADAPTS
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Owns the status shrug, the
// instability countdown, the reconstruction timer and the two forme stances.
EncScript_Deoxys_TurnOpen::
	flushtextbox
	encsetvar 10, 1   @ TurnGuard
	encsetvar 11, 0   @ MoveGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_TurnOpen_Done   @ collapsed: inert
	@ Sleep and freeze are refused rather than wasted - and each refusal costs a point of stress, so
	@ status is the fight's second lever. Capped at Stress <= 7 on purpose: sleeping it every turn
	@ would otherwise walk the meter to 10 on its own and hand over a free damage window. The last
	@ two points can only come from forcing mutations.
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Deoxys_TurnOpen_Shrug
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Deoxys_TurnOpen_Shrug
	goto EncScript_Deoxys_TurnOpen_Lockout
EncScript_Deoxys_TurnOpen_Shrug:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCDEOXYSSHRUG
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_TurnOpen_Lockout
	call EncScript_Deoxys_GainStress
EncScript_Deoxys_TurnOpen_Lockout:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Deoxys_TurnOpen_Cooldown
	encsubvar 5, 1
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Deoxys_TurnOpen_StillUnstable
	printstring STRINGID_ENCDEOXYSSTABILIZED
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	goto EncScript_Deoxys_TurnOpen_Stance
EncScript_Deoxys_TurnOpen_StillUnstable:
	printstring STRINGID_ENCDEOXYSUNSTABLE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Deoxys_TurnOpen_Tier
	@ Skipped while a reconstruction is in flight, so the telegraph turn does not also tick toward
	@ the next cycle. encsubvar wraps at 0, so the floor is an explicit skip rather than a clamp.
EncScript_Deoxys_TurnOpen_Cooldown:
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Deoxys_TurnOpen_Stance
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Deoxys_TurnOpen_Stance
	encsubvar 3, 1
EncScript_Deoxys_TurnOpen_Stance:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_TurnOpen_Blitz
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Deoxys_TurnOpen_Studies
	goto EncScript_Deoxys_TurnOpen_Tier
	@ The Blitz. OnTurnStart is after turn order is locked but before any action resolves, so this
	@ genuinely lands before the player's move - the spec's "acts multiple times" delivered honestly
	@ rather than faked. Scripted damage, so Protect and screens do not stop it.
EncScript_Deoxys_TurnOpen_Blitz:
	printstring STRINGID_ENCDEOXYSBLITZ
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	call EncScript_Deoxys_BlitzChip
	@ A real Protect, so Feint, never-miss moves and contact punishment all resolve exactly as they
	@ should. gProtectStructs is wiped after end-of-turn effects, so it expires on its own.
	encjumpifchance 25, EncScript_Deoxys_TurnOpen_Evade
	goto EncScript_Deoxys_TurnOpen_Tier
EncScript_Deoxys_TurnOpen_Evade:
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCDEOXYSEVADE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Deoxys_TurnOpen_Tier
EncScript_Deoxys_TurnOpen_Studies:
	printstring STRINGID_ENCDEOXYSSTUDIES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_TurnOpen_Tier:
	call EncScript_Deoxys_StressTier
EncScript_Deoxys_TurnOpen_Done:
	return

// OnTurnEnd, priority 90 - runs after every phase transition, hybrid and type-bucket script at the
// same checkpoint. Owns the Rebuild state machine and the Attack Forme meter.
EncScript_Deoxys_TurnClose::
	encsetvar 10, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_TurnClose_Done   @ collapsed: inert
	@ Rebuild 3 -> 1 -> 2 -> 0. RebuildBegin writes 3 rather than 1 precisely so RebuildResolve,
	@ gated on 1 at the same checkpoint, cannot fire in the same dispatch: priority orders the first
	@ pass only, it is not a guard. Stepping it here means a real turn always passes between the
	@ telegraph and the transformation.
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Deoxys_TurnClose_Queue
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Deoxys_TurnClose_Spent
	goto EncScript_Deoxys_TurnClose_Assault
EncScript_Deoxys_TurnClose_Queue:
	encsetvar 4, 1
	goto EncScript_Deoxys_TurnClose_Assault
EncScript_Deoxys_TurnClose_Spent:
	encsetvar 4, 0
	@ Assault. The reason not to simply park in front of Attack Forme and race it: it is the forme
	@ the player most wants it in, and the one that punishes them hardest for staying there.
	@ playanimation BS_PLAYER1 rather than playmoveanimation - gBattlerAttacker is stale at
	@ OnTurnEnd, so anything direction-sensitive needs an explicit battler.
EncScript_Deoxys_TurnClose_Assault:
	encjumpifvar CMP_NOT_EQUAL, 1, 1, EncScript_Deoxys_TurnClose_Done
	encaddvar 8, 1
	encjumpifvar CMP_LESS_THAN, 8, 3, EncScript_Deoxys_TurnClose_Charging
	encsetvar 8, 0
	printstring STRINGID_ENCDEOXYSASSAULTFIRE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	call EncScript_Deoxys_AssaultStrike
	goto EncScript_Deoxys_TurnClose_Done
EncScript_Deoxys_TurnClose_Charging:
	encjumpifvar CMP_LESS_THAN, 8, 2, EncScript_Deoxys_TurnClose_Done
	printstring STRINGID_ENCDEOXYSASSAULTCHARGE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_TurnClose_Done:
	return

// The reconstruction turn. The player is NOT locked out here - they act normally, and what they do
// is the input the selection ladder reads next turn. That makes the telegraph and the decision the
// same turn, which is what turns baiting into a skill rather than a guess.
// encsetprotect only has an effect at OnTurnStart, which is exactly where this is; the 97 guard is
// belt-and-braces against a Feint or a never-miss move, and is lifted again at resolution.
EncScript_Deoxys_RebuildBegin::
	flushtextbox
	printstring STRINGID_ENCDEOXYSRECONSTRUCT
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSRECONSTRUCTING
	waitmessage B_WAIT_TIME_LONG
	encsetprotect ENC_TARGET_BOSS
	encsetdamagereduction ENC_TARGET_BOSS, 96
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_SET
	encsetvar 4, 3   @ Rebuild: queued; TurnClose steps it to 1
	return

// The selection ladder, walked in order. Every rung is something the player controls:
//   1. your active outspeeds it            -> SPEED FORME  (and then nothing outspeeds 180 base, so
//                                             the rule goes false next cycle and it leaves again)
//   2. it lost 10%+ of max HP that turn    -> DEFENSE FORME (save your biggest hit for the telegraph)
//   3. it is below 50% HP                  -> ATTACK FORME  (the glass cannon that takes 1.8x)
//   4. otherwise                           -> NORMAL FORME  (and Normal recycles fastest)
// Rule 3 rides Var(Phase) rather than a live HP test: Phase 1's threshold IS the 50% mark, so the
// spec's rule and the phase boundary are the same line and one variable serves both.
EncScript_Deoxys_RebuildResolve::
	flushtextbox
	enccomparestat ENC_TARGET_PLAYER_LEFT, ENC_TARGET_BOSS, STAT_SPEED, 13
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Deoxys_Resolve_Speed
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE, EncScript_Deoxys_Resolve_Rule3
	encjumpifvar CMP_GREATER_THAN, 12, 9, EncScript_Deoxys_Resolve_Defense
EncScript_Deoxys_Resolve_Rule3:
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Deoxys_Resolve_Attack
	goto EncScript_Deoxys_Resolve_Normal
EncScript_Deoxys_Resolve_Speed:
	encsetvar 13, 3
	goto EncScript_Deoxys_Resolve_Apply
EncScript_Deoxys_Resolve_Defense:
	encsetvar 13, 2
	goto EncScript_Deoxys_Resolve_Apply
EncScript_Deoxys_Resolve_Attack:
	encsetvar 13, 1
	goto EncScript_Deoxys_Resolve_Apply
EncScript_Deoxys_Resolve_Normal:
	encsetvar 13, 0
	@ Rebuild 2 (spent) is written before anything else can go wrong, so this trigger's own gate is
	@ already false when the re-evaluation pass comes round.
EncScript_Deoxys_Resolve_Apply:
	encsetvar 4, 2
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Deoxys_Resolve_TryNormal
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Deoxys_Resolve_TryAttack
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Deoxys_Resolve_TryDefense
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Resolve_NoChange
	goto EncScript_Deoxys_Resolve_Change
EncScript_Deoxys_Resolve_TryNormal:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Deoxys_Resolve_NoChange
	goto EncScript_Deoxys_Resolve_Change
EncScript_Deoxys_Resolve_TryAttack:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Deoxys_Resolve_NoChange
	goto EncScript_Deoxys_Resolve_Change
EncScript_Deoxys_Resolve_TryDefense:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Resolve_NoChange
EncScript_Deoxys_Resolve_Change:
	enccopyvar 1, 13
	call EncScript_Deoxys_ApplyForm
	return
	@ A read that comes back the same costs nothing, and the line that says so is the whole tutorial
	@ for the mechanic: play the same way twice and it tells you, in as many words, that the cycle
	@ was wasted. ApplyGuard here is what lifts the reconstruction's 97 cover.
EncScript_Deoxys_Resolve_NoChange:
	printstring STRINGID_ENCDEOXYSNOCHANGE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// The one spend, and it is not voluntary. Stress resets rather than latching, so instability
// REPEATS - it is a rhythm the player learns to drive, not a one-time phase. Self-disabling: the
// gate is Stress >= 10 and the first thing this does is zero it.
EncScript_Deoxys_Instability::
	flushtextbox
	encsetvar 2, 0    @ Stress
	encsetvar 3, 0    @ Cooldown
	encsetvar 4, 0    @ Rebuild: pre-empts a reconstruction due the same turn
	encsetvar 5, 3    @ Lockout: three turns unable to rebuild
	encsetvar 6, 0    @ Barrier
	encsetvar 8, 0    @ Assault
	encsetvar 14, 0   @ LastStress: back to the stable tier without re-announcing it
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	printstring STRINGID_ENCDEOXYSINSTABILITY
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Deoxys_Instability_Guard
	encsetvar 1, 0    @ Form: collapses to Normal
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_NORMAL, EncScript_Deoxys_Instability_Guard, B_ANIM_FORM_CHANGE_INSTANT
	@ Only on the branch that actually changed shape - the already-Normal jump and the failure label
	@ both skip it, because neither rebuilt the stats this would be restoring.
	call EncScript_Deoxys_Bulk
EncScript_Deoxys_Instability_Guard:
	call EncScript_Deoxys_ApplyGuard
	return

// Phase 1 at 50%: the cycle tightens from four turns to two, and selection rule 3 comes online.
// An in-flight reconstruction keeps its own timing - only a countdown longer than the new cadence
// is pulled in, so the transition can never lengthen the wait.
EncScript_Deoxys_RapidMutation::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCDEOXYSRAPID
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Deoxys_RapidMutation_Done
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Deoxys_RapidMutation_Done
	call EncScript_Deoxys_ResetCooldown
EncScript_Deoxys_RapidMutation_Done:
	return

// Phase 2 at 25%. The cadence is already 2, so this only opens the two Hybrid Mutation beats.
EncScript_Deoxys_PerfectAdaptation::
	encsetvar 0, 2   @ Phase 2
	printstring STRINGID_ENCDEOXYSPERFECT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Hybrid I - ATTACK + SPEED. Attack Forme's offences behind Speed Forme's turn order.
// The raw Speed is applied AFTER the form change: a form change rebuilds the battle stats from the
// new species and wipes anything raw applied before it. That is the intended lifetime here - the
// hybrid's edge survives until the next ordinary reconstruction, which is exactly how long a
// temporary hybrid should last.
EncScript_Deoxys_HybridOne::
	printstring STRINGID_ENCDEOXYSHYBRID
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSHYBRIDONE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1   @ Form: Attack
	encsetvar 4, 0   @ Rebuild: the hybrid IS the mutation, so it cancels any reconstruction in flight
	encsetvar 6, 0   @ Barrier
	encsetvar 8, 0   @ Assault
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_ATTACK, EncScript_Deoxys_HybridOne_Velocity, B_ANIM_ULTRA_BURST
	enchangehp ENC_TARGET_BOSS, 5, ENC_AMOUNT_PERCENT
EncScript_Deoxys_HybridOne_Velocity:
	call EncScript_Deoxys_Bulk
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 80, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	call EncScript_Deoxys_GainStress
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// Hybrid II - DEFENSE + ATTACK. Defense Forme's bulk with offences that actually threaten, and it
// arrives with two thirds of a barrier already up.
EncScript_Deoxys_HybridTwo::
	printstring STRINGID_ENCDEOXYSHYBRID
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSHYBRIDTWO
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 2   @ Form: Defense
	encsetvar 4, 0   @ Rebuild: cancels any reconstruction in flight
	encsetvar 8, 0   @ Assault
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_DEFENSE, EncScript_Deoxys_HybridTwo_Armor, B_ANIM_ULTRA_BURST
	enchangehp ENC_TARGET_BOSS, 5, ENC_AMOUNT_PERCENT
EncScript_Deoxys_HybridTwo_Armor:
	call EncScript_Deoxys_Bulk
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 70, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 70, ENC_AMOUNT_PERCENT
	encsetvar 6, 2   @ Barrier
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	call EncScript_Deoxys_GainStress
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// The Final Twist. B_ANIM_FORM_CHANGE_INSTANT rather than the full fanfare: four of the long
// version would take most of a minute, and the instant variant reads as RAPID, which is the point.
// Each change's failure label is the next change's position, so a failure cascades toward Normal
// rather than parking it mid-cycle.
// Ending on Normal Forme is load-bearing: encformchange writes the party Pokemon's species, so the
// forme Deoxys is in when it is caught is the forme the player keeps.
EncScript_Deoxys_Collapse::
	printstring STRINGID_ENCDEOXYSDESTABILIZE
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_ATTACK, EncScript_Deoxys_Collapse_Two, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Two:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_DEFENSE, EncScript_Deoxys_Collapse_Three, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Three:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_SPEED, EncScript_Deoxys_Collapse_Four, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Four:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_NORMAL, EncScript_Deoxys_Collapse_Done, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Done:
	printstring STRINGID_ENCDEOXYSNOADAPT
	waitmessage B_WAIT_TIME_LONG
	@ Phase 3 gates every stance, countdown and trigger that could still fire, so the catch window is
	@ quiet. CapTypeEffectiveness and FlatToxicDamage stay on as insurance; Survive comes off, after
	@ which the automatic catch-window damage guard keeps the catch target alive.
	encsetvar 0, 3    @ Phase 3
	encsetvar 1, 0    @ Form: Normal
	encsetvar 2, 0    @ Stress
	encsetvar 3, 0    @ Cooldown
	encsetvar 4, 0    @ Rebuild
	encsetvar 5, 0    @ Lockout
	encsetvar 6, 0    @ Barrier
	encsetvar 8, 0    @ Assault
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	@ The four-forme cycle above ended on a freshly rebuilt Normal Forme, so the floor goes back on
	@ before the collapse's own cut is taken out of it.
	call EncScript_Deoxys_Bulk
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -10, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, -10, ENC_AMOUNT_PERCENT
	@ Must match ApplyGuard's Phase 3 leaf exactly - the live reduction and the LastGuard latch are
	@ the same state, and a mismatch makes the next callout announce a move that never happened.
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 9, 80
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCDEOXYSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// The five type buckets. Bug, Ghost and Dark are Deoxys' own weaknesses and so what the player will
// actually reach for; the physical/special split on the remainder keeps neutral coverage moves
// meaningfully different from each other. Five triggers is far cheaper than the eighteen a per-type
// bucket would need.
EncScript_Deoxys_HitBug::
	encsetvar 13, 1
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitGhost::
	encsetvar 13, 2
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitDark::
	encsetvar 13, 3
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitOtherPhysical::
	encsetvar 13, 4
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitOtherSpecial::
	encsetvar 13, 5
	call EncScript_Deoxys_Adapt
	return

// ---------------------------------------------------------------------------------------------
// Jirachi, "The Wish Pokemon" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Jirachi_Intro:
// 0 Phase (0 Awakening / 1 after wish 1 / 2 after wish 2 / 3 Miracle / 4 Wish Granted),
// 1 Wishes (0-3, wishes spent), 2 Energy (Wish Energy, 0-3), 3 Heart (the read on the player,
// 1-15, starts 8), 4 GuardBias (guard ladder rung, 0-4, starts 2), 5 Miracle (0 none / 1 Power /
// 2 Life / 3 Stars), 6 LastGuard (reduction last ANNOUNCED, starts 90), 7 LastTier (Heart tier last
// announced: 0 will / 1 balance / 2 force, starts 3 = never announced), 8 TurnGuard, 9 MoveGuard
// (per-turn OnMoveEnd re-entry guard for the Heart tally), 10 Prev (ApplyGuard's before-value
// scratch), 11 Scratch.
//
// Jirachi reacts to HOW the player fights rather than to what they did to it. Every turn it gathers
// Wish Energy; at full energy it makes a wish, and WHICH wish is decided by the Heart meter - a
// hidden read of the player moved up by attacking and down by using status moves. Every wish costs
// it something, and then it asks whether the player wants the same boon: YES shares the wish and it
// pays nothing, NO makes it pay in full. It gets three wishes. The third is the Miracle, the Heart
// picks that one too, and surviving it puts Jirachi in the catch window.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. A live miracle overrides the ladder outright,
// which is why the miracle leaves are tested first. The ladder itself is symmetric around the
// opening 90: Peace wishes push it up (harder to reach), declined Power wishes pull it down.
// Prev holds the value last announced so each leaf can write LastGuard before comparing; there is
// no UI for damage reduction, so the line has to re-fire on every real change.
EncScript_Jirachi_ApplyGuard:
	enccopyvar 10, 6
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Jirachi_Guard80   @ Miracle of Power: a glass cannon
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Jirachi_Guard92   @ Miracle of Life: a wall
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Jirachi_Guard86   @ Miracle of Stars: a clock
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Jirachi_Guard84
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Jirachi_Guard88
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Jirachi_Guard92
	encjumpifvar CMP_GREATER_THAN, 4, 3, EncScript_Jirachi_Guard94
	goto EncScript_Jirachi_Guard90                            @ GuardBias 2, the opening rung

EncScript_Jirachi_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 6, 80
	encjumpifvar CMP_EQUAL, 10, 80, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 80, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 6, 84
	encjumpifvar CMP_EQUAL, 10, 84, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 84, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 6, 86
	encjumpifvar CMP_EQUAL, 10, 86, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 86, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 6, 88
	encjumpifvar CMP_EQUAL, 10, 88, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 88, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 6, 90
	encjumpifvar CMP_EQUAL, 10, 90, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 90, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 6, 92
	encjumpifvar CMP_EQUAL, 10, 92, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 92, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard94:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encsetvar 6, 94
	encjumpifvar CMP_EQUAL, 10, 94, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 94, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall

EncScript_Jirachi_ApplyGuard_Rise:
	printstring STRINGID_ENCJIRACHIGUARDRISE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_ApplyGuard_Fall:
	printstring STRINGID_ENCJIRACHIGUARDFALL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_ApplyGuard_Done:
	return

// The player's only window onto the Heart meter - the number itself is never shown, only the colour
// of the tags. Latched on LastTier so it prints when the tier actually moves; LastTier starts on the
// out-of-range sentinel 3, so the opening evaluation always prints and the mechanic is on screen
// from turn one. This is the whole discovery mechanism: the colour answers the player's own play,
// and then the wish matches the colour.
EncScript_Jirachi_HeartTier:
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Jirachi_HeartTier_Done   @ Miracle locked in: silent
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Jirachi_HeartTier_Force
	encjumpifvar CMP_GREATER_THAN, 3, 5, EncScript_Jirachi_HeartTier_Balance
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Jirachi_HeartTier_Done
	encsetvar 7, 0
	printstring STRINGID_ENCJIRACHIHEARTWILL
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_HeartTier_Balance:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Jirachi_HeartTier_Done
	encsetvar 7, 1
	printstring STRINGID_ENCJIRACHIHEARTBALANCE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_HeartTier_Force:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Jirachi_HeartTier_Done
	encsetvar 7, 2
	printstring STRINGID_ENCJIRACHIHEARTFORCE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_HeartTier_Done:
	return

// The wish clock, recurring rather than latched: energy is the one mechanic the player has to be
// able to count down every single turn. Silent at 0-1 so the early turns are quiet.
EncScript_Jirachi_EnergyCue:
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Jirachi_EnergyCue_Done   @ no wish left to promise
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Jirachi_EnergyCue_Ring
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Jirachi_EnergyCue_Done
	printstring STRINGID_ENCJIRACHITAGSCHIME
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_EnergyCue_Ring:
	printstring STRINGID_ENCJIRACHITAGSRING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_EnergyCue_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so this only has to write the four non-zero defaults.
// The opening HeartTier call puts the tag colour on screen before the player's first move, which is
// what makes the meter learnable rather than invisible.
EncScript_Jirachi_Intro::
	encsetvar 3, 8     @ Heart: dead centre of 1-15
	encsetvar 4, 2     @ GuardBias: the middle rung, matching the Properties reduction
	encsetvar 6, 90    @ LastGuard: seeded so the first real move reads as a change
	encsetvar 7, 3     @ LastTier: out-of-range sentinel, so the first evaluation always prints
	printstring STRINGID_ENCJIRACHIAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	call EncScript_Jirachi_HeartTier
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Owns the per-turn guards,
// the Miracle of Stars chip clock and the recurring miracle line.
EncScript_Jirachi_TurnOpen::
	flushtextbox
	encsetvar 8, 1    @ TurnGuard
	encsetvar 9, 0    @ MoveGuard
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Jirachi_TurnOpen_Done   @ wishes spent: inert
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Jirachi_TurnOpen_Star
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Jirachi_TurnOpen_Burning
	goto EncScript_Jirachi_TurnOpen_Clings
	@ The star clock. Fires here rather than at the turn close so it lands BEFORE the player's
	@ action, and because gBattlerAttacker is deterministically the opponent at OnTurnStart, which is
	@ what makes playmoveanimation animate from Jirachi in the right direction.
	@ ALL_FOES rather than PLAYER_LEFT so the tick is doubles-safe and skips a fainted battler.
EncScript_Jirachi_TurnOpen_Star:
	printstring STRINGID_ENCJIRACHISTARFALLS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_SWIFT
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_TurnOpen_Done
EncScript_Jirachi_TurnOpen_Burning:
	printstring STRINGID_ENCJIRACHIMIRACLEBURNS
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Jirachi_TurnOpen_Done
	@ Survive is on until the Miracle has played, so a player who has already earned the kill needs
	@ telling why it isn't landing. Only at a sliver of HP, so it is a rare line rather than a drone.
EncScript_Jirachi_TurnOpen_Clings:
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_SET
	encjumpifvar CMP_GREATER_THAN, 11, 5, EncScript_Jirachi_TurnOpen_Done
	printstring STRINGID_ENCJIRACHICLINGS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_TurnOpen_Done:
	return

// Wish Energy income, and the sleep inversion that is this encounter's anti-cheese. Jirachi is the
// Pokemon that sleeps for a thousand years: putting it to sleep does not shut it down, it makes it
// DREAM, and dreams are where wishes come from. Nothing is blocked and nothing is silently ignored -
// a player who reaches for the standard boss lock simply finds the Miracle on a two-turn clock, and
// can reverse it by waking it up. Energy is a plain turn counter besides, so Protect/recover
// stalling accelerates the fight into the Miracle rather than avoiding it.
EncScript_Jirachi_TurnClose::
	encsetvar 8, 0    @ TurnGuard
	encsetvar 9, 0    @ MoveGuard
	@ Phase 3 and 4 stop the clock entirely: the third wish is the last one, so an energy cue past
	@ that point would be promising a fourth that never arrives.
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Jirachi_TurnClose_Done
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Jirachi_TurnClose_Cue
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Jirachi_TurnClose_Dream
	encaddvar 2, 1
	goto EncScript_Jirachi_TurnClose_Cue
EncScript_Jirachi_TurnClose_Dream:
	printstring STRINGID_ENCJIRACHIDREAMS
	waitmessage B_WAIT_TIME_SHORT
	encaddvar 2, 2
	encjumpifvar CMP_LESS_THAN, 2, 4, EncScript_Jirachi_TurnClose_Cue
	encsetvar 2, 3
EncScript_Jirachi_TurnClose_Cue:
	call EncScript_Jirachi_EnergyCue
EncScript_Jirachi_TurnClose_Done:
	return

// The Heart tally. Jirachi reads INTENT, not results: a move that missed still counts, because
// swinging and missing is still swinging - and Event.OldValue/NewValue are unreliable after a miss
// anyway. MoveGuard makes a five-hit move worth one point rather than five. The clamps are explicit
// skips because encaddvar/encsubvar do not saturate.
EncScript_Jirachi_HeartForce::
	encsetvar 9, 1    @ MoveGuard
	encjumpifvar CMP_GREATER_THAN, 3, 14, EncScript_Jirachi_HeartForce_Tier
	encaddvar 3, 1
EncScript_Jirachi_HeartForce_Tier:
	call EncScript_Jirachi_HeartTier
	return

EncScript_Jirachi_HeartWill::
	encsetvar 9, 1    @ MoveGuard
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Jirachi_HeartWill_Tier
	encsubvar 3, 1
EncScript_Jirachi_HeartWill_Tier:
	call EncScript_Jirachi_HeartTier
	return

// 30% HP fills the meter outright, so whichever wish is next arrives within a turn of the player
// pushing Jirachi low rather than waiting out the clock. A hurt Jirachi wishes harder, and the
// Miracle lands as a climax rather than on a timer.
EncScript_Jirachi_TagsBlaze::
	encsetvar 2, 3    @ Energy
	printstring STRINGID_ENCJIRACHITAGSBLAZE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Wishes one and two. The tier branch decides WHAT it wishes for; the prompt decides whether the
// player wishes alongside it. The right answer differs per wish, which is what stops the prompt
// collapsing into a default: refusing Peace or Wonder buys a whole free turn, refusing Power buys
// softened defences and a permanent rung off the guard ladder, and accepting anything hands the
// player the same boon at the price of a faster clock.
// Every player-facing effect targets ALL_FOES rather than PLAYER_LEFT. A group target drops a
// fainted battler; a single-slot one asserts on it - and the Wonder falling-star outcome can KO the
// player's active in the middle of this very script, before the prompt is even answered.
EncScript_Jirachi_Wish::
	flushtextbox
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Jirachi_Wish_BannerTwo
	printstring STRINGID_ENCJIRACHIWISHONE
	goto EncScript_Jirachi_Wish_Open
EncScript_Jirachi_Wish_BannerTwo:
	printstring STRINGID_ENCJIRACHIWISHTWO
EncScript_Jirachi_Wish_Open:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Jirachi_Wish_Power
	encjumpifvar CMP_GREATER_THAN, 3, 5, EncScript_Jirachi_Wish_Wonder
	@ WISH OF PEACE - the answer to a player who fights with status moves. It walls up, and the
	@ guard ladder climbs a rung whether or not the player wishes alongside it: the screens are the
	@ visible half of the boon, the ladder is the half they have to infer from the callout.
EncScript_Jirachi_Wish_Peace:
	printstring STRINGID_ENCJIRACHIWISHPEACE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT, 5
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN, 5
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 5
	encjumpifvar CMP_GREATER_THAN, 4, 3, EncScript_Jirachi_Wish_PeaceAsk
	encaddvar 4, 1
EncScript_Jirachi_Wish_PeaceAsk:
	encaskyesno STRINGID_ENCJIRACHILISTENING, EncScript_Jirachi_Wish_PeaceShared
	printstring STRINGID_ENCJIRACHIPEACECOST
	waitmessage B_WAIT_TIME_LONG
	encsetrecharge ENC_TARGET_BOSS, 1
	goto EncScript_Jirachi_Wish_Spend
EncScript_Jirachi_Wish_PeaceShared:
	printstring STRINGID_ENCJIRACHISHAREDPEACE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_WISH_HEAL
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_REFLECT, 5
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_LIGHT_SCREEN, 5
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SAFEGUARD, 5
	goto EncScript_Jirachi_Wish_Share
	@ WISH OF POWER - the answer to a player who only attacks. Refusing it is the aggressive line:
	@ its defences drop two stages AND the guard ladder loses a rung for good.
EncScript_Jirachi_Wish_Power:
	printstring STRINGID_ENCJIRACHIWISHPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encaskyesno STRINGID_ENCJIRACHILISTENING, EncScript_Jirachi_Wish_PowerShared
	printstring STRINGID_ENCJIRACHIPOWERCOST
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Jirachi_Wish_Spend
	encsubvar 4, 1
	goto EncScript_Jirachi_Wish_Spend
EncScript_Jirachi_Wish_PowerShared:
	printstring STRINGID_ENCJIRACHISHAREDPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, 2
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, 2
	goto EncScript_Jirachi_Wish_Share
	@ WISH OF WONDER - the answer to a player who does a bit of everything, and the only wish whose
	@ payload is a roll. Uniform 1-in-4 across the four outcomes (25 / 33 / 50 / fall-through).
EncScript_Jirachi_Wish_Wonder:
	printstring STRINGID_ENCJIRACHIWISHWONDER
	waitmessage B_WAIT_TIME_LONG
	encjumpifchance 25, EncScript_Jirachi_Wonder_Strength
	encjumpifchance 33, EncScript_Jirachi_Wonder_Star
	encjumpifchance 50, EncScript_Jirachi_Wonder_Still
	@ It gains nothing at all. One beat in the fight is purely a gift, and this is it.
	printstring STRINGID_ENCJIRACHIWONDERPLAY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	enchangehp ENC_TARGET_ALL_FOES, 25, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wonder_Ask
EncScript_Jirachi_Wonder_Strength:
	printstring STRINGID_ENCJIRACHIWONDERSTRENGTH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wonder_Ask
EncScript_Jirachi_Wonder_Star:
	printstring STRINGID_ENCJIRACHIWONDERSTAR
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 4
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	playmoveanimation MOVE_SWIFT
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wonder_Ask
	@ Cuts both ways - it wipes the player's setup AND Jirachi's own boosts - which is why this
	@ outcome needs no drawback of its own.
EncScript_Jirachi_Wonder_Still:
	printstring STRINGID_ENCJIRACHIWONDERSTILL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	normalisebuffs
EncScript_Jirachi_Wonder_Ask:
	encaskyesno STRINGID_ENCJIRACHILISTENING, EncScript_Jirachi_Wish_WonderShared
	printstring STRINGID_ENCJIRACHIWONDERCOST
	waitmessage B_WAIT_TIME_LONG
	encsetrecharge ENC_TARGET_BOSS, 1
	goto EncScript_Jirachi_Wish_Spend
EncScript_Jirachi_Wish_WonderShared:
	printstring STRINGID_ENCJIRACHISHAREDWONDER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	enchangehp ENC_TARGET_ALL_FOES, 25, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wish_Share
	@ Refused: the meter empties, so the next wish is a full three turns away.
EncScript_Jirachi_Wish_Spend:
	encsetvar 2, 0
	goto EncScript_Jirachi_Wish_Advance
	@ Shared: the wish cost it nothing and it comes round a turn sooner. The price the player will
	@ not see coming on a first run - accept all three and the Miracle arrives early, against a
	@ Jirachi that never paid for any of them.
EncScript_Jirachi_Wish_Share:
	encsetvar 2, 1
	@ Both spend paths write Energy to a value this trigger's own condition rejects, so it cannot
	@ re-select inside one dispatch. Phase tracks wishes spent, which is what opens the Miracle.
EncScript_Jirachi_Wish_Advance:
	encaddvar 1, 1
	enccopyvar 0, 1
	call EncScript_Jirachi_ApplyGuard
	return

// The third wish, and Jirachi's alone - no prompt. Taking the choice away at the climax is the
// point. The Heart is read one last time, so the fight the player brought is the fight they get:
// attackers race a glass cannon, stallers have to break a wall, and everyone else runs a clock.
EncScript_Jirachi_Miracle::
	flushtextbox
	printstring STRINGID_ENCJIRACHIFINALWISH
	waitmessage B_WAIT_TIME_LONG
	encsetvar 0, 3    @ Phase 3
	encsetvar 1, 3    @ Wishes
	encsetvar 2, 0    @ Energy: this trigger's own gate, closed before anything else can run
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Jirachi_Miracle_Power
	encjumpifvar CMP_GREATER_THAN, 3, 5, EncScript_Jirachi_Miracle_Stars
	@ MIRACLE OF LIFE. The staller's own game handed back to them, and the answer is the setup they
	@ have spent the fight accumulating and now finally have to cash in.
	printstring STRINGID_ENCJIRACHIMIRACLELIFE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 0
	encsetvar 5, 2
	goto EncScript_Jirachi_Miracle_Done
	@ MIRACLE OF POWER. A race - it can end the player in two moves, they can end it in three.
EncScript_Jirachi_Miracle_Power:
	printstring STRINGID_ENCJIRACHIMIRACLEPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 50, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 50, ENC_AMOUNT_PERCENT
	encsetvar 5, 1
	goto EncScript_Jirachi_Miracle_Done
	@ MIRACLE OF STARS. The softest guard of the three, permanent Tailwind, and a star falling on the
	@ player every turn from here (TurnOpen). Finish it or lose.
EncScript_Jirachi_Miracle_Stars:
	printstring STRINGID_ENCJIRACHIMIRACLESTARS
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encsetvar 5, 3
EncScript_Jirachi_Miracle_Done:
	call EncScript_Jirachi_ApplyGuard
	return

// Survive the Miracle and the tags go dark: every encounter rule comes off and the fight becomes an
// ordinary Pokemon battle. Clearing Miracle is load-bearing rather than tidiness - it is what stops
// the star clock chipping the player's Pokemon down while they are trying to land a ball.
// From here the engine's automatic catch-window damage guard keeps the catch target alive.
EncScript_Jirachi_WishGranted::
	encsetvar 0, 4    @ Phase 4
	encsetvar 2, 0    @ Energy
	encsetvar 5, 0    @ Miracle: stops the star clock and every recurring line
	encclearscreens ENC_TARGET_BOSS, EncScript_Jirachi_WishGranted_Release
EncScript_Jirachi_WishGranted_Release:
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	@ The live reduction and the LastGuard latch are the same state; a mismatch would make the next
	@ callout announce a move that never happened.
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 6, 0
	encsetcatchrate 40
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCJIRACHIWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// ---------------------------------------------------------------------------------------------
// Rayquaza, "The Sky Guardian" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Rayquaza_Intro:
// 0 Phase (0 Sky Guardian / 1 Delta Ascension / 2 Atmosphere Breaks / 3 Weakened),
// 1 Alt (0 Grounded, 1 Low, 2 High, 3 Stratospheric), 2 Press (Atmospheric Pressure, 0-5),
// 3 Fall (Skyfall state: 0 idle, 2 gone, 1 descending), 4 Climb (climb cadence countdown),
// 5 Chaos (Phase 2 weather-roll cadence), 6 TurnGuard, 7 Cycle (per-turn guard shared by the three
// altitude/Skyfall OnTurnStart triggers), 8 Guarded (per-turn guard for SkyGuard), 9 Vent (per-turn
// guard shared by the two weather triggers), 10 Shot (per-turn guard for ShootDown), 11 HpMark
// (boss HP% at turn open; ENC_SNAP_DAMAGE turns it into "lost this turn"), 12 LastPress (Pressure
// tier last ANNOUNCED - the re-fire latch), 13 Pred (encstoreprediction scratch).
//
// Rayquaza is a fight over a POSITION rather than a number. Altitude sets how little damage it
// takes and how fast Atmospheric Pressure builds; full Pressure means it vanishes and Skyfall lands
// on the player. Two levers pull it back down - put weather on the field and it must descend to
// tear the atmosphere apart (but it descends angry, and the anger stacks), or land enough damage in
// one turn to shoot it out of the air. Grounded, its clock is stopped and it is finally hittable.
// Mega Evolution at 50% locks the sky with Strong Winds and confiscates the weather lever; the
// collapse at 22% hands weather back, hostile to Rayquaza too.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction. The altitude and phase lines already announce every change to
// it, so unlike Jirachi's ladder this one needs no callout of its own - the player is told the
// moment Rayquaza moves, every time it moves.
// Mega Rayquaza gains 100/100 defences over base 90/90, so its rungs sit 1-2 points lower to keep
// grounded damage roughly flat across the transition. The Phase 2 collapse is a real ~3x swing.
EncScript_Rayquaza_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rayquaza_GuardBroken
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rayquaza_GuardMega
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Rayquaza_Guard87
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Rayquaza_Guard89
	goto EncScript_Rayquaza_Guard92
EncScript_Rayquaza_GuardMega:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Rayquaza_Guard85
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Rayquaza_Guard88
	goto EncScript_Rayquaza_Guard91
EncScript_Rayquaza_GuardBroken:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Rayquaza_Guard76
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_78
	goto EncScript_Rayquaza_Guard80
EncScript_Rayquaza_Guard76:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	return
EncScript_78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
EncScript_Rayquaza_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Rayquaza_Guard85:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Rayquaza_Guard87:
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return
EncScript_Rayquaza_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Rayquaza_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Rayquaza_Guard91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Rayquaza_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return

// The tail both levers and the Skyfall landing share. Evasion is SET, not nudged: encchangestat
// clamps at the stage bounds, so -12 then +6 lands on neutral from wherever the stage had drifted -
// immune to Haze, Defog and the player's own accuracy drops.
EncScript_Rayquaza_Descend:
	encsetvar 1, 0    @ Alt: Grounded
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6
	call EncScript_Rayquaza_ApplyGuard
	printstring STRINGID_ENCRAYQUAZAGROUNDED
	waitmessage B_WAIT_TIME_LONG
	return

// The Pressure callout, latched on the tier last announced rather than on the raw value, so it
// fires on every real change and never drones. Skyfall zeroes Press, which drops the latch back to
// tier 0 through the leading branch here - so the whole build-up re-announces on the next cycle
// instead of being a one-shot the player can miss.
EncScript_Rayquaza_PressureCue:
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Rayquaza_PressureCue_Two
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_Rayquaza_PressureCue_One
	encsetvar 12, 0
	return
EncScript_Rayquaza_PressureCue_One:
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Rayquaza_PressureCue_Done
	encsetvar 12, 1
	printstring STRINGID_ENCRAYQUAZAPRESSUREONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Rayquaza_PressureCue_Two:
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Rayquaza_PressureCue_Done
	encsetvar 12, 2
	printstring STRINGID_ENCRAYQUAZAPRESSURETWO
	waitmessage B_WAIT_TIME_SHORT
EncScript_Rayquaza_PressureCue_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the climb counter needs seeding. Starting it at
// 2 means Rayquaza spends two turns on the ground before it first lifts away, which is the window
// the player is meant to learn the fight in. Air Lock announces itself on entry unprompted, and
// that stock line is the first hint that weather is not going to behave.
EncScript_Rayquaza_Intro::
	encsetvar 4, 2    @ Climb: first climb lands on turn 3
	printstring STRINGID_ENCRAYQUAZAAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// It owns the four per-turn re-entry guards outright. TurnClose deliberately clears none of them -
// Var(Shot) gates an OnTurnEnd trigger, and clearing it from the priority-90 close would make that
// trigger eligible again inside the same dispatch.
EncScript_Rayquaza_TurnOpen::
	flushtextbox
	encsetvar 6, 1     @ TurnGuard
	encsetvar 7, 0     @ Cycle
	encsetvar 8, 0     @ Guarded
	encsetvar 9, 0     @ Vent
	encsetvar 10, 0    @ Shot
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_SET     @ the mark ShootDown reads at turn close
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Rayquaza_TurnOpen_Done   @ Weakened: inert
	@ Atmospheric Pressure income. Grounded pays nothing, which is the whole reward for pulling it
	@ down: the Skyfall clock simply stops. Stratospheric pays nothing either - it is already gone.
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Rayquaza_TurnOpen_Low
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Rayquaza_TurnOpen_High
	goto EncScript_Rayquaza_TurnOpen_Cue
EncScript_Rayquaza_TurnOpen_High:
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Rayquaza_TurnOpen_HighFast
EncScript_Rayquaza_TurnOpen_Low:
	encaddvar 2, 1
	goto EncScript_Rayquaza_TurnOpen_Cue
EncScript_Rayquaza_TurnOpen_HighFast:
	encaddvar 2, 2
EncScript_Rayquaza_TurnOpen_Cue:
	call EncScript_Rayquaza_PressureCue
	@ The recurring out-of-reach line, and the sleep line that replaces it. Nothing in this fight is
	@ an ACTION - a sleeping Rayquaza still climbs, still builds Pressure and still lands Skyfall on
	@ your head, because none of that is a move. That is this encounter's anti-cheese, and one line
	@ makes the point without stating it.
	encjumpifvar CMP_LESS_THAN, 1, 1, EncScript_Rayquaza_TurnOpen_Done
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Rayquaza_TurnOpen_Done
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Rayquaza_TurnOpen_Drifts
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Rayquaza_TurnOpen_Done
	printstring STRINGID_ENCRAYQUAZAOUTOFREACH
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Rayquaza_TurnOpen_Done
EncScript_Rayquaza_TurnOpen_Drifts:
	printstring STRINGID_ENCRAYQUAZADRIFTS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Rayquaza_TurnOpen_Done:
	return

// Every countdown lives here, one checkpoint away from the OnTurnStart triggers that resolve them,
// so a real turn always passes between a timer ticking and the beat it arms.
// While Rayquaza is gone nothing else ticks: the Skyfall step is the only state change that turn.
EncScript_Rayquaza_TurnClose::
	encsetvar 6, 0    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Rayquaza_TurnClose_Done
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Rayquaza_TurnClose_Falling
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Rayquaza_TurnClose_Chaos
	encsubvar 4, 1
EncScript_Rayquaza_TurnClose_Chaos:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Rayquaza_TurnClose_Done
	encsubvar 5, 1
	goto EncScript_Rayquaza_TurnClose_Done
	@ 2 -> 1 is what makes SkyfallLand eligible next turn; SkyfallBegin writes 2 and SkyfallLand is
	@ gated on 1, so no dispatch can run both.
EncScript_Rayquaza_TurnClose_Falling:
	encsetvar 3, 1
EncScript_Rayquaza_TurnClose_Done:
	return

// One rung up the ladder. Sets Var(Cycle) because in Delta Ascension the re-arm value is 0 by
// design (it climbs every turn) - without the shared guard this trigger's own condition would still
// be true on re-evaluation and it would climb to the ceiling inside a single dispatch.
EncScript_Rayquaza_Climb::
	encsetvar 7, 1    @ Cycle
	encaddvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 7    @ absolute +1, not a nudge
	call EncScript_Rayquaza_ApplyGuard
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Rayquaza_Climb_High
	printstring STRINGID_ENCRAYQUAZACLIMBSLOW
	goto EncScript_Rayquaza_Climb_Arm
EncScript_Rayquaza_Climb_High:
	printstring STRINGID_ENCRAYQUAZACLIMBSHIGH
EncScript_Rayquaza_Climb_Arm:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	@ Cadence by phase: every 3rd turn as the Sky Guardian, EVERY turn in Delta Ascension, every 2nd
	@ once the atmosphere breaks.
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rayquaza_Climb_ArmFast
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rayquaza_Climb_ArmMid
	encsetvar 4, 2
	return
EncScript_Rayquaza_Climb_ArmMid:
	encsetvar 4, 1
	return
EncScript_Rayquaza_Climb_ArmFast:
	encsetvar 4, 0
	return

// High Rayquaza reads the player and slips the attack it expects. encstoreprediction writes the
// predicted CATEGORY: 0 no read / 1 physical / 2 special / 3 status. A predicted special move is
// never evaded, which is the rule the player can actually find - special attacks always reach it,
// physical ones can be read. With no read at all it falls back to a flat roll so the sky is never
// simply free.
// encsetprotect blocks everything, so a player whose strongest option is physical but who fires a
// special move that turn can still be stopped; the line claims Rayquaza read their INTENT, which is
// exactly what the prediction is.
EncScript_Rayquaza_SkyGuard::
	encsetvar 8, 1    @ Guarded
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 13
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Rayquaza_SkyGuard_Done
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Rayquaza_SkyGuard_Evade
	encjumpifchance 35, EncScript_Rayquaza_SkyGuard_Evade
EncScript_Rayquaza_SkyGuard_Done:
	return
EncScript_Rayquaza_SkyGuard_Evade:
	printstring STRINGID_ENCRAYQUAZAREADSYOU
	waitmessage B_WAIT_TIME_SHORT
	encsetprotect ENC_TARGET_BOSS
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	return

// Skyfall, turn one. encsetprotect is only meaningful at OnTurnStart (gProtectStructs is wiped
// after end-of-turn effects), which is exactly where this runs, and it expires on its own;
// encsetrecharge 1 here costs Rayquaza THIS turn's action. Between them the turn is mutually dead,
// which is the honest expression of "it is not on the field" - no command removes a battler, and
// the one animation that genuinely slides a sprite away has nothing that slides it back.
EncScript_Rayquaza_SkyfallBegin::
	encsetvar 7, 1    @ Cycle
	printstring STRINGID_ENCRAYQUAZASKYFALLWARN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	printstring STRINGID_ENCRAYQUAZAVANISHED
	waitmessage B_WAIT_TIME_LONG
	encsetvar 2, 0    @ Press
	encsetvar 3, 2    @ Fall: gone
	encsetvar 1, 3    @ Alt: Stratospheric
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 7
	call EncScript_Rayquaza_ApplyGuard
	encsetprotect ENC_TARGET_BOSS
	encsetrecharge ENC_TARGET_BOSS, 1
	return

// Skyfall, turn two - the hardest turn in the fight and the best one. Rayquaza is NOT protected and
// NOT recharging on the way down, so its move and the player's both resolve on top of the scripted
// hit; and it ends standing on the ground with its clock at zero. The Skyfall is how you get your
// turn, which is the whole symmetry of the encounter.
// ENC_TARGET_ALL_FOES rather than a single slot: a single-slot target asserts on a fainted battler,
// a group target skips the absent silently.
EncScript_Rayquaza_SkyfallLand::
	encsetvar 7, 1    @ Cycle
	encsetvar 3, 0    @ Fall: idle
	printstring STRINGID_ENCRAYQUAZASKYFALLCOMING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SKY_ATTACK
	printstring STRINGID_ENCRAYQUAZASKYFALL
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Rayquaza_SkyfallLand_Delta
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Rayquaza_SkyfallLand_Late
	enchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Rayquaza_SkyfallLand_Down
EncScript_Rayquaza_SkyfallLand_Delta:
	enchangehp ENC_TARGET_ALL_FOES, -40, ENC_AMOUNT_PERCENT
	goto EncScript_Rayquaza_SkyfallLand_Down
EncScript_Rayquaza_SkyfallLand_Late:
	enchangehp ENC_TARGET_ALL_FOES, -33, ENC_AMOUNT_PERCENT
EncScript_Rayquaza_SkyfallLand_Down:
	call EncScript_Rayquaza_Descend
	return

// Lever 1. Air Lock means the player's weather never DID anything, so setting it is purely a lure -
// and the Air Lock entry message already told them so on turn one. The stat stages are the price
// and they STACK; MAX_STAT_STAGE caps them for free, so nothing needs tracking. Pressure is
// untouched: the reward for grounding it is that the clock stops, which is quieter and stronger
// than any number.
EncScript_Rayquaza_Vent::
	encsetvar 9, 1    @ Vent
	printstring STRINGID_ENCRAYQUAZATEARS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	removeweather
	call EncScript_Rayquaza_Descend
	printstring STRINGID_ENCRAYQUAZAENOUGH
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	return

// Lever 1b. Bring your own Kyogre or Groudon and Rayquaza silences it personally, for double the
// anger. Primordial Sea and Desolate Land re-apply on switch-in, so pivoting the setter back in
// re-triggers this - a learnable trap rather than an exploit, and the only weather the player
// cannot normally lose.
EncScript_Rayquaza_VentPrimal::
	encsetvar 9, 1    @ Vent
	printstring STRINGID_ENCRAYQUAZACONFLICT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	removeweather
	call EncScript_Rayquaza_Descend
	printstring STRINGID_ENCRAYQUAZAENOUGH
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	return

// Lever 2. HpMark was written at turn open with ENC_SNAP_SET; ENC_SNAP_DAMAGE turns it into
// "percentage of max HP lost this turn", cumulative across multi-hits, chip and status.
// The guard ladder makes the threshold self-balancing: 6% through an 87 guard needs a raw hit worth
// about 46% of max HP, through a 92 guard about 75%. It gets harder the higher Rayquaza is, which
// is exactly right.
EncScript_Rayquaza_ShootDown::
	encsetvar 10, 1    @ Shot
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_DAMAGE, EncScript_Rayquaza_ShootDown_None
	encjumpifvar CMP_LESS_THAN, 11, 6, EncScript_Rayquaza_ShootDown_None
	printstring STRINGID_ENCRAYQUAZASHOTDOWN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	call EncScript_Rayquaza_Descend
EncScript_Rayquaza_ShootDown_None:
	return

// Phase 2's broken sky. Uniform 1-in-5 across the five outcomes (20 / 25 / 33 / 50 / fall-through).
// Air Lock left with the Mega, so this is the only phase where weather is mechanically live - and
// Rayquaza is Dragon/Flying with no sand or hail immunity, so the atmosphere finally chips it too.
// Each roll is announced, and each overwrites whatever the player set, which is the emergent way of
// saying "you do not control the sky".
EncScript_Rayquaza_WeatherChaos::
	encsetvar 5, 2    @ Chaos: two-turn cadence
	encjumpifchance 20, EncScript_Rayquaza_Chaos_Rain
	encjumpifchance 25, EncScript_Rayquaza_Chaos_Sun
	encjumpifchance 33, EncScript_Rayquaza_Chaos_Sand
	encjumpifchance 50, EncScript_Rayquaza_Chaos_Hail
	printstring STRINGID_ENCRAYQUAZACHAOSCLEAR
	waitmessage B_WAIT_TIME_SHORT
	removeweather
	return
EncScript_Rayquaza_Chaos_Rain:
	printstring STRINGID_ENCRAYQUAZACHAOSRAIN
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return
EncScript_Rayquaza_Chaos_Sun:
	printstring STRINGID_ENCRAYQUAZACHAOSSUN
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return
EncScript_Rayquaza_Chaos_Sand:
	printstring STRINGID_ENCRAYQUAZACHAOSSAND
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_SANDSTORM, 0
	playanimation BS_OPPONENT1, B_ANIM_SANDSTORM_CONTINUES
	return
EncScript_Rayquaza_Chaos_Hail:
	printstring STRINGID_ENCRAYQUAZACHAOSHAIL
	waitmessage B_WAIT_TIME_SHORT
	encsetweather BATTLE_WEATHER_HAIL, 0
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	return

// 50% - DELTA ASCENSION. The form change does three things for free, and each does real work:
// both species are 105 base HP so the health bar does not jump; the macro's trailing
// switchinabilities fires Delta Stream, whose Strong Winds are primal and so refuse every
// subsequent weather - the player's grounding lever is confiscated on screen by a stock message;
// and Air Lock leaves, which is what makes the Phase 2 collapse land later.
// Strong Winds also neutralises Flying's own weaknesses, so the Ice/Rock/Electric matchups soften
// on top of the guard - CapTypeEffectiveness had already clamped the 4x Ice weakness to 2x, and
// this takes it to 1x. Ice-stacking a Mega Rayquaza is meant to stop working.
// Dragon Ascent is written in only now: knowing it beforehand would have satisfied CanMegaEvolve
// and let the AI spend a turn-1 gimmick, but once the species already IS the Mega the check can no
// longer fire.
EncScript_Rayquaza_DeltaAscension::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCRAYQUAZAABSORBS
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCRAYQUAZADELTAASCENSION
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_RAYQUAZA_MEGA, EncScript_Rayquaza_DeltaAscension_NoForm
	encsetmove ENC_TARGET_BOSS, 0, MOVE_DRAGON_ASCENT
EncScript_Rayquaza_DeltaAscension_NoForm:
	encsetvar 4, 0    @ Climb: every turn from here
	call EncScript_Rayquaza_ApplyGuard
	return

// 22% - THE ATMOSPHERE BREAKS. RemoveAllWeather clears primal weather unconditionally and Delta
// Stream is a switch-in ability with no per-turn re-application, so Strong Winds stay gone.
// Survive is set here so an oversized hit cannot skip the catch window at 10%; it is paired with
// the Immunities: list, which closes the fixed-damage / Perish Song / Destiny Bond holes Survive
// does not cover.
EncScript_Rayquaza_AtmosphereBreaks::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCRAYQUAZACONTROLFAILING
	waitmessage B_WAIT_TIME_LONG
	removeweather
	encsetvar 4, 1    @ Climb: every other turn
	encsetvar 5, 1    @ Chaos: first roll next turn
	encsetsurvive ENC_TARGET_BOSS, TRUE
	call EncScript_Rayquaza_ApplyGuard
	return

// 10% - the catch window. Reverting to base Rayquaza is the guideline cue and housekeeping at once:
// it guarantees the caught Pokemon is a plain Rayquaza whatever the form-change table does, and
// 105 = 105 base HP means the bar still does not move.
// Slot 0 is restored to Extreme Speed BEFORE the revert, and that is load-bearing rather than
// tidiness: a base Rayquaza that merely knows Dragon Ascent satisfies CanMegaEvolve, and the AI
// would Mega Evolve again in the middle of the player's catch attempts.
// Every altitude, Pressure, Skyfall and chaos trigger is gated Var(Phase) <= 2, so the whole
// machine goes quiet here. From this point the engine's automatic catch-window damage guard keeps
// the catch target alive; encsetdamagereduction sets the value that guard will restore, not the
// live one.
EncScript_Rayquaza_Weakened::
	encsetvar 0, 3    @ Phase
	printstring STRINGID_ENCRAYQUAZAGIVESOUT
	waitmessage B_WAIT_TIME_LONG
	encsetmove ENC_TARGET_BOSS, 0, MOVE_EXTREME_SPEED
	encformchange ENC_TARGET_BOSS, SPECIES_RAYQUAZA, EncScript_Rayquaza_Weakened_Release, B_ANIM_FORM_CHANGE
EncScript_Rayquaza_Weakened_Release:
	removeweather
	encsetvar 1, 0    @ Alt
	encsetvar 2, 0    @ Press
	encsetvar 3, 0    @ Fall
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCRAYQUAZAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// ---------------------------------------------------------------------------------------------
// Kyogre, "The Endless Ocean" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Kyogre_Intro:
// 0 Tide (0-5, the water level everything else derives from), 1 Phase (0 Rising Waters / 1 Primal
// Awakening / 2 Great Deluge / 3 Weakened), 2 LastTide (tide tier last ANNOUNCED - the re-fire
// latch), 3 Rise (natural cadence countdown), 4 Mark (Undertow countdown, 0 or 3-1), 5 Switched
// (per-turn guard for the Undertow bail), 6 Deluge (0 idle, 3-1 charging, 5 just resolved, 4 spent),
// 7 Exhaust (exhaustion window countdown), 8 TurnGuard, 9 Cycle (per-turn guard shared by the two
// Deluge clock triggers), 10 Flux (per-turn guard shared by the three move-event tide triggers, in
// BOTH directions), 11 Woke (per-turn guard for the anti-cheese trigger).
//
// Kyogre is a fight over a WATER LEVEL, and unlike every mechanic before it the level moves in both
// directions. The Tide rises on its own, rises when Kyogre lands a Water move, and rises when Fire
// touches it; it falls to Electric and Grass damage, and to holding your ground under the Undertow
// instead of switching away. High Tide raises Kyogre's damage reduction, drowns the player's whole
// side every turn, swamps that side's Speed at Tide 4, and arms the Undertow.
// Only ONE move-event tide change lands per turn (Var(Flux)), in either direction, so losing the
// Speed race is losing the Tide race - and Tide 4 quarters the player's Speed.
// Everything derived from the Tide is RE-DERIVED at TurnOpen rather than accumulated, so a tide
// that runs 0-3-1-4-2 leaves no residue and nothing has to remember how to undo the last tier.

// --- Shared subroutines (call/return) ---

// Sole owner of the tide's damage reduction ladder. TurnOpen skips this entirely while the
// exhaustion window is open, which is what lets the flat 55 stand over whatever the tide is doing.
EncScript_Kyogre_ApplyGuard:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Kyogre_Guard92
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyogre_Guard90
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Kyogre_Guard88
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Kyogre_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Kyogre_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Kyogre_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return

// Every rise in the fight goes through here. encaddvar has no ceiling of its own, so the clamp is a
// guarded branch rather than a bare add.
EncScript_Kyogre_TideUp:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Kyogre_TideUp_Done
	encaddvar 0, 1
	printstring STRINGID_ENCKYOGRETIDERISES
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
EncScript_Kyogre_TideUp_Done:
	return

// Every ebb goes through here. encsubvar on a u8 underflows to 255, so 0 is tested for explicitly;
// the Phase 2 floor of 3 is the second branch - from the Great Deluge on the player can still push
// the water back, just never to calm, and the Deluge check is set at exactly that floor.
EncScript_Kyogre_TideDown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Kyogre_TideDown_Done
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Kyogre_TideDown_Fall
	encjumpifvar CMP_LESS_THAN, 0, 4, EncScript_Kyogre_TideDown_Done
EncScript_Kyogre_TideDown_Fall:
	encsubvar 0, 1
	printstring STRINGID_ENCKYOGRETIDEFALLS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Kyogre_TideDown_Done:
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones. Dropping back to Tide 0 resets the latch
// through the leading branch, so the whole climb re-announces if the water rises again.
EncScript_Kyogre_TideCue:
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Kyogre_TideCue_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyogre_TideCue_Four
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Kyogre_TideCue_Three
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Kyogre_TideCue_Two
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Kyogre_TideCue_One
	encsetvar 2, 0
	return
EncScript_Kyogre_TideCue_One:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCKYOGRETIDEONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Kyogre_TideCue_Two:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCKYOGRETIDETWO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Kyogre_TideCue_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 3
	printstring STRINGID_ENCKYOGRETIDETHREE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Kyogre_TideCue_Four:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 4
	printstring STRINGID_ENCKYOGRETIDEFOUR
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Kyogre_TideCue_Five:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Kyogre_TideCue_Done
	encsetvar 2, 5
	printstring STRINGID_ENCKYOGRETIDEFIVE
	waitmessage B_WAIT_TIME_LONG
EncScript_Kyogre_TideCue_Done:
	return

// Drowning chip, from Flood upward. ENC_TARGET_ALL_FOES rather than a single slot: this can KO, and
// a single-slot target asserts on a fainted battler while a group target skips the absent silently.
EncScript_Kyogre_Drown:
	encjumpifvar CMP_LESS_THAN, 0, 3, EncScript_Kyogre_Drown_Done
	printstring STRINGID_ENCKYOGREDROWNING
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_SURF
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Kyogre_Drown_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Kyogre_Drown_Four
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Kyogre_Drown_Four:
	enchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Kyogre_Drown_Five:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Kyogre_Drown_Done:
	return

// The Undertow. Armed from the Primal Reversion on, whenever the water reaches Tide 4.
// Both the marking turn and the two that follow pay the same price: raw Sp. Def erosion, which is
// permanent and survives Haze and switching, on top of the drowning chip already dealt above.
// Surviving all three turns is worth one tide step (EncScript_Kyogre_TurnClose); switching away
// costs one instead, so bailing is a two-step swing against the player.
EncScript_Kyogre_Undertow:
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Kyogre_Undertow_Pull
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Kyogre_Undertow_Done
	encjumpifvar CMP_LESS_THAN, 0, 4, EncScript_Kyogre_Undertow_Done
	encsetvar 4, 3
	printstring STRINGID_ENCKYOGREUNDERTOWMARK
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_WHIRLPOOL
	goto EncScript_Kyogre_Undertow_Drag
EncScript_Kyogre_Undertow_Pull:
	printstring STRINGID_ENCKYOGREUNDERTOWPULL
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_WHIRLPOOL
EncScript_Kyogre_Undertow_Drag:
	encchangestatvalue ENC_TARGET_ALL_FOES, STAT_SPDEF, -12, ENC_AMOUNT_PERCENT
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
EncScript_Kyogre_Undertow_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the cadence counter needs seeding. Rise counts
// DOWN to 0 and rises the tide on the turn it is already 0, so a re-arm of 2 is a rise every third
// turn - two calm turns to read the fight before the water starts moving.
EncScript_Kyogre_Intro::
	encsetvar 3, 2    @ Rise: first natural rise at the end of turn 3
	printstring STRINGID_ENCKYOGREAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// It owns all five per-turn re-entry guards. Clearing them here rather than at TurnClose is what
// keeps the priority-5 anti-cheese trigger and the OnTurnEnd Deluge triggers out of reach of their
// own dispatch - and Var(Switched) has to be cleared before the player's switch action resolves,
// which is after this checkpoint.
// Every effect below is re-derived from Var(Tide) outright, never accumulated.
EncScript_Kyogre_TurnOpen::
	flushtextbox
	encsetvar 8, 1     @ TurnGuard
	encsetvar 9, 0     @ Cycle
	encsetvar 10, 0    @ Flux
	encsetvar 11, 0    @ Woke
	encsetvar 5, 0     @ Switched
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyogre_TurnOpen_Done   @ Weakened: inert
	@ The exhaustion window replaces the whole tide ladder - the flat 55 stands regardless of what
	@ the water is doing, and it is the only real damage window in the last quarter of the fight.
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Kyogre_TurnOpen_Reeling
	call EncScript_Kyogre_ApplyGuard
	goto EncScript_Kyogre_TurnOpen_Swamp
EncScript_Kyogre_TurnOpen_Reeling:
	printstring STRINGID_ENCKYOGREREELING
	waitmessage B_WAIT_TIME_SHORT
	@ The submerged battlefield. Swamp quarters the player's side's Speed
	@ (GetBattlerTotalSpeedStat), which feeds straight back into the Flux race the whole fight turns
	@ on: falling behind makes falling further behind easier. Set or cleared outright every turn, so
	@ it lifts the moment the water drops back below Tide 4.
EncScript_Kyogre_TurnOpen_Swamp:
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_Kyogre_TurnOpen_Swamped
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
	goto EncScript_Kyogre_TurnOpen_Cue
EncScript_Kyogre_TurnOpen_Swamped:
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
EncScript_Kyogre_TurnOpen_Cue:
	call EncScript_Kyogre_TideCue
	call EncScript_Kyogre_Drown
	call EncScript_Kyogre_Undertow
EncScript_Kyogre_TurnOpen_Done:
	return

// Every countdown lives here, one checkpoint away from the OnTurnStart triggers that resolve them,
// so a real turn always passes between a timer ticking and the beat it arms.
EncScript_Kyogre_TurnClose::
	encsetvar 8, 0    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Kyogre_TurnClose_Done   @ Weakened: inert
	@ The natural cadence. It does NOT spend Var(Flux), so from the Primal Reversion on the water
	@ gains a step every turn for free and an ebb only ever buys the player a net zero - which is
	@ why holding the line, rather than draining the sea, is the whole Phase 2 game.
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Kyogre_TurnClose_Rise
	encsubvar 3, 1
	goto EncScript_Kyogre_TurnClose_Mark
EncScript_Kyogre_TurnClose_Rise:
	call EncScript_Kyogre_TideUp
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Kyogre_TurnClose_RiseSlow
	encsetvar 3, 0    @ Primal onward: every turn
	goto EncScript_Kyogre_TurnClose_Mark
EncScript_Kyogre_TurnClose_RiseSlow:
	encsetvar 3, 2    @ Rising Waters: every third turn
	@ The Undertow countdown. Reaching 0 from 1 is the payoff for holding position through all three
	@ marked turns.
EncScript_Kyogre_TurnClose_Mark:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Kyogre_TurnClose_Deluge
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Kyogre_TurnClose_Deluge
	printstring STRINGID_ENCKYOGREUNDERTOWHELD
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyogre_TideDown
	@ The spent-Deluge sentinel, walked down here rather than at TurnOpen so the turn after a
	@ resolution is a guaranteed turn of rest: 5 means "resolved this turn", 4 means "spent", and
	@ only 0 re-arms DelugeBegin. Clearing 4 waits on the exhaustion window running out.
EncScript_Kyogre_TurnClose_Deluge:
	encjumpifvar CMP_EQUAL, 6, 5, EncScript_Kyogre_TurnClose_Spent
	encjumpifvar CMP_NOT_EQUAL, 6, 4, EncScript_Kyogre_TurnClose_Exhaust
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Kyogre_TurnClose_Exhaust
	encsetvar 6, 0
	goto EncScript_Kyogre_TurnClose_Exhaust
EncScript_Kyogre_TurnClose_Spent:
	encsetvar 6, 4
EncScript_Kyogre_TurnClose_Exhaust:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Kyogre_TurnClose_Done
	encsubvar 7, 1
EncScript_Kyogre_TurnClose_Done:
	return

// Anti-cheese. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at
// OnTurnStart - and the water climbs for the trouble, so putting the sea to sleep is not free.
// Leads with flushtextbox for the same reason TurnOpen does: this runs one priority earlier.
EncScript_Kyogre_WillNotBeStilled::
	flushtextbox
	encsetvar 11, 1    @ Woke
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCKYOGRENOTSTILLED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyogre_TideUp
	return

// The three move-event tide triggers, all spending the one shared Var(Flux) charge. Whichever
// resolves first that turn is the only one that lands, in either direction - that cap is the point,
// not an over-shared guard: an Origin Pulse that beats the player's Thunderbolt does not just add a
// step, it locks the player's step out.
EncScript_Kyogre_Ebb::
	encsetvar 10, 1    @ Flux
	call EncScript_Kyogre_TideDown
	return

EncScript_Kyogre_SurgeFire::
	encsetvar 10, 1    @ Flux
	printstring STRINGID_ENCKYOGREANSWERSFLAME
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Kyogre_TideUp
	return

EncScript_Kyogre_SurgeWater::
	encsetvar 10, 1    @ Flux
	call EncScript_Kyogre_TideUp
	return

// Bailing out from under the Undertow. The three turns of Sp. Def erosion already paid are wasted,
// and the water gains the step the player was three turns from taking back.
EncScript_Kyogre_UndertowBail::
	encsetvar 5, 1    @ Switched
	encsetvar 4, 0    @ Mark
	printstring STRINGID_ENCKYOGREUNDERTOWFLED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Kyogre_TideUp
	return

// The Great Deluge, turn one of three. Var(Cycle) stops DelugeTick chasing this straight back down
// in the same dispatch.
EncScript_Kyogre_DelugeBegin::
	encsetvar 9, 1    @ Cycle
	encsetvar 6, 3    @ Deluge
	printstring STRINGID_ENCKYOGREDELUGEBEGINS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return

// Turns two and three, announced every turn so the player can count the wave down and read the
// resolution coming.
EncScript_Kyogre_DelugeTick::
	encsetvar 9, 1    @ Cycle
	encsubvar 6, 1
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Kyogre_DelugeTick_Near
	printstring STRINGID_ENCKYOGREDELUGECOUNT
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return
EncScript_Kyogre_DelugeTick_Near:
	printstring STRINGID_ENCKYOGREDELUGENEAR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	return

// The wave breaks, and it resolves against the Tide rather than against the player's HP - the whole
// endgame is holding the water at the Phase 2 floor for three straight turns.
// Failing that costs 45% of the party's health: a full-health Pokemon lives it, one that has been
// drowning at Tide 5 does not, and that is the lesson.
// Holding it earns the only real damage window in the fight. encsetrecharge 2 at OnTurnEnd costs
// Kyogre its NEXT action (the timer is decremented after this checkpoint) and prints the stock
// "must recharge!" line; Exhaust is seeded at 3 because TurnClose spends one tick this same
// dispatch, leaving two full turns at reduction 76.
EncScript_Kyogre_DelugeResolve::
	encsetvar 6, 5    @ Deluge: resolved this turn
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_Kyogre_DelugeResolve_Hits
	printstring STRINGID_ENCKYOGREDELUGEFAILS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 7, 3    @ Exhaust
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encsetrecharge ENC_TARGET_BOSS, 2
	return
EncScript_Kyogre_DelugeResolve_Hits:
	printstring STRINGID_ENCKYOGREDELUGEHITS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ORIGIN_PULSE
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	encsetvar 0, 5    @ Tide: the sea takes the field
	return

// 50% - PRIMAL REVERSION. The form change carries the mechanic on its own: both forms are 100 base
// HP so the health bar does not jump, and the macro's trailing switchinabilities fires Primordial
// Sea, whose rain cannot be removed and makes Fire moves fail outright. The "Fire raises the tide"
// rule stops being a punishment and becomes a flat denial, which is why EncScript_Kyogre_SurgeFire
// is gated to Phase 0.
// From here the natural cadence goes from one step every three turns to one step every turn -
// everything the player was already doing still works, they just have to do it twice as fast.
EncScript_Kyogre_PrimalReversion::
	encsetvar 1, 1    @ Phase
	printstring STRINGID_ENCKYOGREPRIMAL
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_KYOGRE_PRIMAL, EncScript_Kyogre_PrimalReversion_NoForm, B_ANIM_PRIMAL_REVERSION
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 25, ENC_AMOUNT_PERCENT
EncScript_Kyogre_PrimalReversion_NoForm:
	encsetvar 3, 0    @ Rise: the sea climbs every turn from here
	printstring STRINGID_ENCKYOGREANCIENTPOWER
	waitmessage B_WAIT_TIME_LONG
	return

// 25% - THE GREAT DELUGE. Survive is set here rather than in Properties: so the AI reads Kyogre as
// killable for three quarters of the fight; from this point it guarantees the player actually
// reaches the 10% catch window instead of losing it to one oversized hit during an exhaustion turn.
// The Immunities: list closes the fixed-damage, Perish Song and Destiny Bond holes Survive does not.
// The tide is floored at 3 from here (EncScript_Kyogre_TideDown), and pushed up to that floor now if
// the player held it lower - "permanently high" and "bring the water down to survive" only coexist
// if the floor and the Deluge's own check sit at the same number.
EncScript_Kyogre_GreatDeluge::
	encsetvar 1, 2    @ Phase
	printstring STRINGID_ENCKYOGREBEYONDCONTROL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 15, ENC_AMOUNT_PERCENT
	encsetvar 6, 0    @ Deluge: arms the first charge next turn
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Kyogre_GreatDeluge_Done
	encsetvar 0, 3    @ Tide: floored
EncScript_Kyogre_GreatDeluge_Done:
	return

// 10% - the catch window. Reverting drops Primordial Sea, so the endless sea drains back to ordinary
// weather - the visual confirmation that the mechanic is over - and it guarantees the caught Pokemon
// is a plain Kyogre. removeweather runs after the revert because the macro's switchinabilities fires
// base Kyogre's Drizzle on the way through.
// Clearing every tide effect is trivial precisely because all of them were derived rather than
// accumulated. From this point the engine's automatic catch-window damage guard keeps the catch
// target alive; encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Kyogre_Weakened::
	encsetvar 1, 3    @ Phase
	encformchange ENC_TARGET_BOSS, SPECIES_KYOGRE, EncScript_Kyogre_Weakened_Release, B_ANIM_PRIMAL_REVERSION
EncScript_Kyogre_Weakened_Release:
	removeweather
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
	encsetvar 0, 0    @ Tide
	encsetvar 2, 0    @ LastTide
	encsetvar 4, 0    @ Mark
	encsetvar 6, 0    @ Deluge
	encsetvar 7, 0    @ Exhaust
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 20
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCKYOGREWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// ---------------------------------------------------------------------------------------------
// Groudon, "The Living Continent" (src/data/battle_encounters.encounter). Var indices, pinned by
// the always-true Conditions on EncScript_Groudon_Intro:
// 0 Land (0-5, the landmass everything else derives from), 1 Phase (0 The Land Rises / 1 Primal
// Groudon / 2 Continental Collapse / 3 Weakened), 2 LastLand (landmass tier last ANNOUNCED - the
// re-fire latch), 3 Fuse (quake countdown, 1-4), 4 Grow (natural growth countdown), 5 Collapse
// (Phase 2 collapse cadence), 6 Shift (per-turn guard shared by the three move-event triggers, in
// BOTH directions), 7 Shaken (per-turn guard for the paralysis anti-cheese trigger), 8 TurnGuard,
// 9 Snap (encsnapshothp scratch for the collapse finisher), 10 Sunlit (per-turn guard for the sun
// re-assertion trigger).
//
// Kyogre is a fight over a level you hold; Groudon is a fight over a RATCHET you have to pry back
// down. The Land climbs on its own, climbs when Groudon lands a Ground or Fire move, and climbs
// when Fire touches it. It comes down for exactly one reason: a damaging Water move hitting
// Groudon. Only ONE move-driven change lands per turn (Var(Shift)), in either direction, and
// Groudon's 90 Speed is usually fast enough to spend it first.
// The Land does three things at once: it raises Groudon's damage reduction, it scorches the whole
// player side every turn from Scorched Plateau up, and it sets the QUAKE FUSE - both how hard the
// quake hits and how few turns there are between quakes.
// Everything derived from the Land is RE-DERIVED at TurnOpen rather than accumulated, so a landmass
// that runs 0-3-1-4-2 leaves no residue and nothing has to remember how to undo the last tier.

// --- Shared subroutines (call/return) ---

// Sole owner of the landmass damage reduction ladder. 90 is the guideline baseline and the bottom
// rung; the top rung is reached only by letting the land run away.
EncScript_Groudon_ApplyGuard:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Groudon_Guard95
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_Guard92
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Groudon_Guard88
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Groudon_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Groudon_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Groudon_Guard95:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return

// Every climb in the fight goes through here. encaddvar has no ceiling of its own, so the clamp is
// a guarded branch rather than a bare add.
EncScript_Groudon_LandUp:
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Groudon_LandUp_Done
	encaddvar 0, 1
	printstring STRINGID_ENCGROUDONLANDRISES
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
EncScript_Groudon_LandUp_Done:
	return

// Every erosion goes through here. encsubvar on a u8 underflows to 255, so 0 is tested explicitly.
// Unlike Kyogre's tide there is no phase floor: "Groudon does not easily give territory back" is
// expressed as scarcity of the lever - one Water move per turn, against growth that never stops -
// rather than as a hard bound the player can't push past.
EncScript_Groudon_LandDown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Groudon_LandDown_Done
	encsubvar 0, 1
	printstring STRINGID_ENCGROUDONLANDFALLS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Groudon_LandDown_Done:
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones. Dropping back to Land 0 resets the latch
// through the leading branch, so the whole climb re-announces if the land rises again.
EncScript_Groudon_LandCue:
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Groudon_LandCue_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_LandCue_Four
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Groudon_LandCue_Three
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Groudon_LandCue_Two
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Groudon_LandCue_One
	encsetvar 2, 0
	return
EncScript_Groudon_LandCue_One:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Groudon_LandCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCGROUDONLANDONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Groudon_LandCue_Two:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Groudon_LandCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCGROUDONLANDTWO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Groudon_LandCue_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Groudon_LandCue_Done
	encsetvar 2, 3
	printstring STRINGID_ENCGROUDONLANDTHREE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Groudon_LandCue_Four:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Groudon_LandCue_Done
	encsetvar 2, 4
	printstring STRINGID_ENCGROUDONLANDFOUR
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Groudon_LandCue_Five:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Groudon_LandCue_Done
	encsetvar 2, 5
	printstring STRINGID_ENCGROUDONLANDFIVE
	waitmessage B_WAIT_TIME_LONG
EncScript_Groudon_LandCue_Done:
	return

// The scorch chip, from Scorched Plateau upward. ENC_TARGET_ALL_FOES rather than a single slot:
// this can KO, and a single-slot target asserts on a fainted battler while a group target skips the
// absent silently.
EncScript_Groudon_Scorch:
	encjumpifvar CMP_LESS_THAN, 0, 3, EncScript_Groudon_Scorch_Done
	printstring STRINGID_ENCGROUDONSCORCH
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_LAVA_PLUME
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Groudon_Scorch_Five
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_Scorch_Four
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Groudon_Scorch_Four:
	enchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Groudon_Scorch_Five:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Groudon_Scorch_Done:
	return

// The natural cadence. It does NOT spend Var(Shift), so from the Primal Reversion on the land gains
// a step every turn for free and an erosion only ever buys the player a net zero - which is why
// holding the line, rather than levelling the continent, is the whole second-half game.
// Grow counts DOWN and rises the land on the turn it is already 0, so a re-arm of 2 is a rise every
// third turn and a re-arm of 0 is a rise every turn.
EncScript_Groudon_Growth:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Groudon_Growth_Rise
	encsubvar 4, 1
	return
EncScript_Groudon_Growth_Rise:
	call EncScript_Groudon_LandUp
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Groudon_Growth_Slow
	encsetvar 4, 0    @ Primal onward: every turn
	return
EncScript_Groudon_Growth_Slow:
	encsetvar 4, 2    @ The Land Rises: every third turn
	return

// The quake fuse, ticked here at TurnOpen and resolved at OnTurnEnd, so the tick and the re-arm can
// never collide inside one dispatch and silently shorten every fuse by one.
// The warning fires every single turn - the loudest possible statement that something is coming -
// and the player always gets at least one turn of it at every tier.
EncScript_Groudon_FuseTick:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Groudon_FuseTick_Done   @ inert (Phase 3 reset)
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Groudon_FuseTick_Warn
	encsubvar 3, 1
EncScript_Groudon_FuseTick_Warn:
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Groudon_FuseTick_Far
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Groudon_FuseTick_Near
	printstring STRINGID_ENCGROUDONPREPARING
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Groudon_FuseTick_Near:
	printstring STRINGID_ENCGROUDONCRACK
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Groudon_FuseTick_Far:
	printstring STRINGID_ENCGROUDONTREMBLE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Groudon_FuseTick_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the two countdowns need seeding: the first
// natural rise and the first quake both land on turn 3.
// Drought has already put a five-turn sun up by now; removeweather first is what lets encsetweather
// (which does nothing when that weather is already in force) restate it as permanent.
EncScript_Groudon_Intro::
	encsetvar 3, 4    @ Fuse: first quake at the end of turn 3
	encsetvar 4, 2    @ Grow: first natural rise on turn 3
	printstring STRINGID_ENCGROUDONAWAKENS
	waitmessage B_WAIT_TIME_LONG
	removeweather
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// TurnGuard is set immediately rather than at the tail so no branch out of this script can leave it
// re-armed. Every effect below is re-derived from Var(Land) outright, never accumulated.
EncScript_Groudon_TurnOpen::
	flushtextbox
	encsetvar 8, 1    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Groudon_TurnOpen_Done   @ Weakened: inert
	call EncScript_Groudon_Growth
	call EncScript_Groudon_ApplyGuard
	call EncScript_Groudon_LandCue
	call EncScript_Groudon_Scorch
	call EncScript_Groudon_FuseTick
	@ The collapse cadence, ticked here for the same reason the fuse is - CollapseTick re-arms it at
	@ OnTurnEnd and must not be able to eat its own re-arm.
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Groudon_TurnOpen_Unstable
	encsubvar 5, 1
EncScript_Groudon_TurnOpen_Unstable:
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Groudon_TurnOpen_Done
	printstring STRINGID_ENCGROUDONUNSTABLE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Groudon_TurnOpen_Done:
	return

// TurnClose owns nothing but the per-turn guard resets. Every countdown lives at TurnOpen instead,
// one checkpoint away from the OnTurnEnd resolutions that re-arm them.
EncScript_Groudon_TurnClose::
	encsetvar 8, 0     @ TurnGuard
	encsetvar 6, 0     @ Shift
	encsetvar 7, 0     @ Shaken
	encsetvar 10, 0    @ Sunlit
	return

// Anti-cheese. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at
// OnTurnStart - and the land climbs for the trouble, so paralysing the continent is not free.
// Leads with flushtextbox for the same reason TurnOpen does: this runs two priorities earlier.
EncScript_Groudon_WillNotBeShaken::
	flushtextbox
	encsetvar 7, 1    @ Shaken
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCGROUDONNOTSHAKEN
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Groudon_LandUp
	return

// The land refuses to be watered from the Primal Reversion on. Announced rather than silent so the
// refusal reads as Groudon doing something, instead of as the player's Rain Dance quietly ceasing
// to matter.
EncScript_Groudon_LandWillNotBeWatered::
	flushtextbox
	encsetvar 10, 1    @ Sunlit
	encsetweather BATTLE_WEATHER_SUN, 0
	printstring STRINGID_ENCGROUDONSUNRETURNS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	return

// The three move-event triggers, all spending the one shared Var(Shift) charge. Whichever resolves
// first that turn is the only one that lands, in either direction - that cap is the point, not an
// over-shared guard: a Precipice Blades that beats the player's Surf does not just add a step, it
// locks the player's step out, and the turn nets +2 once natural growth is counted.
EncScript_Groudon_Erosion::
	encsetvar 6, 1    @ Shift
	call EncScript_Groudon_LandDown
	return

EncScript_Groudon_FeedFlame::
	encsetvar 6, 1    @ Shift
	printstring STRINGID_ENCGROUDONFEEDSFLAME
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Groudon_LandUp
	return

EncScript_Groudon_FeedBoss::
	encsetvar 6, 1    @ Shift
	call EncScript_Groudon_LandUp
	return

// The quake. Its magnitude is read off Var(Land) at the moment it lands, and so is its own re-arm -
// higher land means a harder quake AND a shorter fuse, which is the whole reason erosion is worth a
// turn. Re-arms to 4 at Land 0-2, 3 at Land 3-4 and 2 at Land 5, so PRIMAL LAND quakes every turn.
// The Speed drop is a stat STAGE, not a raw cut: a permanent Speed loss compounding every two turns
// for a whole fight is unrecoverable, while a stage drop is something Haze or a switch can answer.
// Every player-facing effect targets ENC_TARGET_ALL_FOES - the damage above it can KO, and a
// single-slot target asserts on a fainted battler.
EncScript_Groudon_QuakeResolve::
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_Groudon_QuakeResolve_Break
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Groudon_QuakeResolve_Major
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Groudon_QuakeResolve_Quake
	printstring STRINGID_ENCGROUDONTREMOR
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_MAGNITUDE
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	encsetvar 3, 4    @ Fuse
	return
EncScript_Groudon_QuakeResolve_Quake:
	printstring STRINGID_ENCGROUDONQUAKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EARTHQUAKE
	enchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Groudon_QuakeResolve_ArmLong
	encsetvar 3, 3    @ Fuse
	return
EncScript_Groudon_QuakeResolve_ArmLong:
	encsetvar 3, 4    @ Fuse
	return
	@ From here up the quake also strips the player's screens - the one command that can take a
	@ Reflect or Light Screen off outside a move. It takes a fail label rather than failing, so the
	@ same command is both "were there any?" and "remove them".
EncScript_Groudon_QuakeResolve_Major:
	printstring STRINGID_ENCGROUDONMAJORQUAKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EARTHQUAKE
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encsetvar 3, 3    @ Fuse
	goto EncScript_Groudon_QuakeResolve_Strip
EncScript_Groudon_QuakeResolve_Break:
	printstring STRINGID_ENCGROUDONBREAK
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FISSURE
	enchangehp ENC_TARGET_ALL_FOES, -28, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encsetvar 3, 2    @ Fuse
EncScript_Groudon_QuakeResolve_Strip:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Groudon_QuakeResolve_Done
	printstring STRINGID_ENCGROUDONSTRIPPED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Groudon_QuakeResolve_Done:
	return

// Phase 2's clock: the continent Groudon spent the fight building caves in on a fixed cadence,
// taking Groudon down with the player's party. Re-arms to 3, so one collapse every two turns.
// The snapshot branch is not optional. enchangehp writes passiveHpUpdate directly rather than going
// through the damage path where the Survive: clamp lives, so an unguarded tick could take Groudon
// to 0, faint it, and end the fight as an ordinary win with no catch window ever opening. Guarding
// it turns that hole into the phase's finisher: the last collapse breaks Groudon deterministically
// at 8%, which is inside the 10% catch window, and EncScript_Groudon_Weakened picks it up in this
// same dispatch.
EncScript_Groudon_CollapseTick::
	printstring STRINGID_ENCGROUDONCOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FISSURE
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_SET
	encjumpifvar CMP_GREATER_THAN, 9, 15, EncScript_Groudon_CollapseTick_Grind
	encsetvar 9, 8
	encrewindhp ENC_TARGET_BOSS, 9
	goto EncScript_Groudon_CollapseTick_Party
EncScript_Groudon_CollapseTick_Grind:
	enchangehp ENC_TARGET_BOSS, -5, ENC_AMOUNT_PERCENT
EncScript_Groudon_CollapseTick_Party:
	enchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	encsetvar 5, 3    @ Collapse
	return

// 50% - PRIMAL REVERSION. SPECIES_GROUDON_PRIMAL is Ground/Fire, so the matchup shifts mid-fight:
// Water goes to 4x and is clamped back to 2x by CapTypeEffectiveness (exactly what that property is
// for), Grass falls to neutral, and Fire becomes resisted - the player's erosion tool stays
// best-in-class while their Grass coverage quietly stops being an answer.
// The Ability: ABILITY_DROUGHT override survives this for free, because RecalcBattlerStats re-
// applies it after every form change. That is the whole reason it is an override: Primal Groudon's
// real Desolate Land would make Water moves fail outright and delete the player's only erosion
// lever at the halfway mark.
// Eruption's power scales with Groudon's remaining HP, so at 50% it is already halved and by Phase 2
// nearly nothing - its offense would collapse exactly when it is supposed to become terrifying.
// Fire Blast is flat, and "it stops radiating and starts firing" is the right read for the form.
// From here the natural cadence goes from one step every three turns to one step every turn.
EncScript_Groudon_PrimalReversion::
	encsetvar 1, 1    @ Phase
	printstring STRINGID_ENCGROUDONPRIMAL
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_GROUDON_PRIMAL, EncScript_Groudon_PrimalReversion_NoForm, B_ANIM_PRIMAL_REVERSION
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 25, ENC_AMOUNT_PERCENT
EncScript_Groudon_PrimalReversion_NoForm:
	encsetmove ENC_TARGET_BOSS, 1, MOVE_FIRE_BLAST
	encsetvar 4, 0    @ Grow: the land climbs every turn from here
	printstring STRINGID_ENCGROUDONANCIENTPOWER
	waitmessage B_WAIT_TIME_LONG
	return

// 25% - CONTINENTAL COLLAPSE. No stat bump: Groudon is not getting stronger here, it is being
// crushed. Survive is set here rather than in Properties: so the AI reads Groudon as killable for
// three quarters of the fight, and from this point it guarantees the player reaches the catch
// window instead of losing it to one oversized hit. The Immunities: list closes the fixed-damage,
// Perish Song and Destiny Bond holes Survive does not.
// The landmass ladder, the natural growth and the quake fuse all keep running underneath, so
// "survive" is not "wait" - at PRIMAL LAND that is 12% scorch plus a 28% quake every turn on top of
// the collapse, and the only way to make it survivable is to keep eroding.
EncScript_Groudon_ContinentalCollapse::
	encsetvar 1, 2    @ Phase
	printstring STRINGID_ENCGROUDONCOLLAPSEBEGINS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FISSURE
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encsetvar 5, 2    @ Collapse: the first collapse lands one turn from now
	return

// 10% - the catch window. The sun going out is the visual confirmation that the mechanic is over,
// and reverting guarantees the caught Pokemon is a plain Groudon. removeweather runs after the
// revert because the macro's trailing switchinabilities fires the Drought override on the way
// through; the sun-re-assertion trigger is gated to Phase <= 2 and so stays out of it from here.
// Clearing every landmass effect is trivial precisely because all of them were derived rather than
// accumulated. From this point the engine's automatic catch-window damage guard keeps the catch
// target alive; encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Groudon_Weakened::
	encsetvar 1, 3    @ Phase
	encformchange ENC_TARGET_BOSS, SPECIES_GROUDON, EncScript_Groudon_Weakened_Release, B_ANIM_PRIMAL_REVERSION
EncScript_Groudon_Weakened_Release:
	removeweather
	encsetvar 0, 0    @ Land
	encsetvar 2, 0    @ LastLand
	encsetvar 3, 0    @ Fuse
	encsetvar 4, 0    @ Grow
	encsetvar 5, 0    @ Collapse
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 20
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCGROUDONWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Regice, "The Frozen Clock" (src/data/battle_encounters.encounter). Var indices, pinned by the
// always-true Conditions on EncScript_Regice_Intro:
// 0 Phase (0 The Frozen Clock / 1 Deep Freeze / 2 Frozen Tomb / 3 Weakened), 1 Chill (0-5),
// 2 LastChill (Chill tier last ANNOUNCED - the re-fire latch), 3 Rise (cadence countdown to the
// next Chill), 4 Tomb (0 open / 1 sealed), 5 Shed ("the cold no longer troubles Regice" has been
// said), 6 TurnGuard, 7 MoveGuard (per-turn guard shared by the two thaw triggers), 8 Rolled
// (per-turn guard for the lost-action roll), 9 Slowed (Speed stages the cold holds on the CURRENT
// player battler), 10 HpMark (boss HP% at turn open), 11 Sky (per-turn guard for the re-freeze).
//
// Articuno's Frost feeds a barrier you shatter and Celebi's Echo rewinds your damage. Regice's
// CHILL eats your TURNS, and it rises on a clock rather than on an action - nothing done to Regice
// slows it down, only the temperature does. Chill 1-2 stiffens both sides honestly; at 3 the field
// starts eating one side's action at random and Regice sheds the cold; at 5 the player loses a turn
// outright and Regice does not. The same value drives the damage reduction, so a stalled fight is a
// fight being lost.
// Four levers bring it down: a Fire move on Regice (-2), a Fighting move (-1), harsh sunlight
// standing at turn close (-1), and a heavy blow of 12% of Regice's HP in one turn (-1). Regice's
// only answer is the sky: it puts its hail back at the next turn open, so a sun cast buys one thaw.
// Every Chill change routes through ChillUp/ChillDown so the Speed stages, the boss mirroring and
// the ledger in Var(Slowed) can never drift out of step with the value itself.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction ladder, a pure function of Chill with no phase term - which is
// what lets the Chill callout double as the guard callout. Stops at 95 rather than the 99 cap:
// Absolute Zero should be frightening, not a window where the player can neither act nor damage.
EncScript_Regice_ApplyGuard:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regice_Guard95
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Regice_Guard94
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Regice_Guard93
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Regice_Guard91
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Regice_Guard89
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Regice_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Regice_Guard91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Regice_Guard93:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Regice_Guard94:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Regice_Guard95:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones. Falling back to Chill 0 resets the latch,
// so the whole build-up re-announces if the cold climbs again. Chill 5 stays on the tier-2 latch -
// Absolute Zero prints its own, louder line.
EncScript_Regice_ChillCue:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Regice_ChillCue_TierTwo
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Regice_ChillCue_TierOne
	encsetvar 2, 0    @ LastChill
	return
EncScript_Regice_ChillCue_TierOne:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regice_ChillCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCREGICECHILLONE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Regice_ChillCue_TierTwo:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Regice_ChillCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCREGICECHILLTWO
	waitmessage B_WAIT_TIME_LONG
EncScript_Regice_ChillCue_Done:
	return

// Called by every path that moves Chill, after ChillUp/ChillDown has run.
EncScript_Regice_ApplyChill:
	call EncScript_Regice_ApplyGuard
	call EncScript_Regice_ChillCue
	return

// One rung colder, and the only place the cold ever rises - so the line here is the guarantee that
// every single change to the guard ladder is announced, with the tier callout layered on top of it.
// Stat STAGES rather than a raw Speed cut, and a delta rather than an absolute set: a delta cannot
// silently eat the player's own Dragon Dance, the stages go away on a switch (the free lever Frozen
// Tomb later confiscates), and Haze clears the cold off both sides at once.
// Regice takes the drop with the field only through Chill 2. Above that it stops feeling the cold
// and the player does not - the fight saying out loud, one rung early, that the symmetry is over.
// Clear Body is the reason this works: encchangestat writes statStages directly rather than going
// through ChangeStatBuffs, so the encounter's own drop lands while every player-side attempt to
// slow Regice bounces off the ability.
EncScript_Regice_ChillUp:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Regice_ChillUp_Done
	encaddvar 1, 1
	printstring STRINGID_ENCREGICEDEEPENS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encaddvar 9, 1    @ Slowed
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Regice_ChillUp_Sheds
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -1
	return
EncScript_Regice_ChillUp_Sheds:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Regice_ChillUp_Done
	encsetvar 5, 1    @ Shed
	printstring STRINGID_ENCREGICESHEDS
	waitmessage B_WAIT_TIME_LONG
EncScript_Regice_ChillUp_Done:
	return

// One rung warmer. encsubvar on a u8 underflows to 255, so the floor is tested explicitly.
// The two sides are given their stage back on separate tests, because they can be out of step:
// Regice only ever took the rungs at Chill 1 and 2, so it only gets one back when the rung being
// undone is one of those, while the player's give-back is metered by Var(Slowed) - which a switch
// zeroes, so a thaw after a switch can never hand out Speed the cold never took.
EncScript_Regice_ChillDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_ChillDown_Done
	encsubvar 1, 1
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Regice_ChillDown_Player
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
EncScript_Regice_ChillDown_Player:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Regice_ChillDown_Done
	encsubvar 9, 1    @ Slowed
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 1
EncScript_Regice_ChillDown_Done:
	return

// The clock. Rise counts down and the cold advances on the turn it is already 0, so a re-arm of 2
// is a rise every third turn and a re-arm of 0 is a rise every turn. The re-arm happens before the
// cap test, so a capped clock still resets rather than sitting at 0 and re-firing.
// Phase 0 caps at 4: the player meets the Speed drop, the random lost actions and all four thaw
// levers before Absolute Zero is ever on the table.
EncScript_Regice_Clock:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Regice_Clock_Advance
	encsubvar 3, 1
	return
EncScript_Regice_Clock_Advance:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regice_Clock_ArmSlow
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Regice_Clock_ArmMid
	encsetvar 3, 0    @ Frozen Tomb: every turn
	goto EncScript_Regice_Clock_Rise
EncScript_Regice_Clock_ArmSlow:
	encsetvar 3, 2    @ The Frozen Clock: every third turn
	goto EncScript_Regice_Clock_Rise
EncScript_Regice_Clock_ArmMid:
	encsetvar 3, 1    @ Deep Freeze: every second turn
EncScript_Regice_Clock_Rise:
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Regice_Clock_Do
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Regice_Clock_Done
EncScript_Regice_Clock_Do:
	call EncScript_Regice_ChillUp
EncScript_Regice_Clock_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the clock needs seeding: the first Chill lands
// at the close of turn 3. The hail is permanent (turns = 0) and one-sided by ordinary type rules -
// Regice is pure Ice, so nothing has to be scripted for it to be the only thing standing in it.
// It is the background clock that says this fight cannot be waited out.
EncScript_Regice_Intro::
	encsetvar 3, 2    @ Rise
	printstring STRINGID_ENCREGICEAWAKENS
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_HAIL, 0
	printstring STRINGID_ENCREGICEHAILFIELD
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu.
// TurnGuard is set immediately rather than at the tail so no branch out of this script can leave it
// re-armed. The HP mark is taken here and read at TurnClose, which is what makes the heavy-blow
// lever mean "this turn" no matter how many hits it took.
// The sleep line is the anti-cheese, stated once a turn and never explained: putting Regice under
// is a legal play that buys nothing, because none of this fight is Regice's action.
EncScript_Regice_TurnOpen::
	flushtextbox
	encsetvar 6, 1     @ TurnGuard
	encsetvar 7, 0     @ MoveGuard: the thaw levers are armed again
	encsetvar 8, 0     @ Rolled: the lost-action roll is armed again
	encsetvar 11, 0    @ Sky: the re-freeze is armed again
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_SET
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Regice_TurnOpen_Done   @ Weakened: inert
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Regice_TurnOpen_Sleeps
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Regice_TurnOpen_Done
	printstring STRINGID_ENCREGICECHILLRECUR
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regice_TurnOpen_Sleeps:
	printstring STRINGID_ENCREGICESLEEPS
	waitmessage B_WAIT_TIME_LONG
EncScript_Regice_TurnOpen_Done:
	return

// Regice puts its own sky back. This is what prices the sun lever: the sun pays out one rung at
// TurnClose and is gone by the time the player acts again, so a cast is worth exactly one thaw and
// no more. No Chill cost for the re-freeze itself - charging for it would make the lever worthless.
EncScript_Regice_Refreeze::
	flushtextbox
	encsetvar 11, 1    @ Sky
	removeweather
	encsetweather BATTLE_WEATHER_HAIL, 0
	printstring STRINGID_ENCREGICEREFREEZE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HAIL_CONTINUES
	return

// ABSOLUTE ZERO. encsetrecharge with 1 at OnTurnStart costs that battler THIS turn's action, which
// is the whole reason this resolves at turn open. Regice is not put on the timer, so the one turn
// the field takes from the player is a turn Regice still gets - the asymmetry the honest, symmetric
// Chill 1-2 was setting up.
// B_ANIM_TRICK_ROOM rather than a weather loop: it is a whole-field warp, which is what this is.
// The 8% heal is the cold preserving what stands in it.
// It then walks back to Chill 2, not 0. The field cannot hold absolute zero, but it does not go
// back to room temperature either - a recoverable position rather than a reset, and in Phase 2 it
// also opens the Tomb for one turn, handing the player their switch window right after the worst
// beat in the fight.
EncScript_Regice_AbsoluteZero::
	flushtextbox
	printstring STRINGID_ENCREGICEABSOLUTEZERO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	statusanimation BS_PLAYER1, STATUS1_FREEZE
	encsetrecharge ENC_TARGET_ALL_FOES, 1
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ApplyChill
	return

// Chill 3-4: the field eats one side's action. At 3 it only happens half the time; at 4 it always
// does, and either side can be the one that loses the turn - Regice is not exempt until Chill 5.
// statusanimation is purely cosmetic (Cmd_statusanimation never writes status1), so the field looks
// like it froze somebody without any status being applied that a Lum Berry could answer.
EncScript_Regice_FrozenAction::
	flushtextbox
	encsetvar 8, 1    @ Rolled
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Regice_FrozenAction_Roll
	encjumpifchance 50, EncScript_Regice_FrozenAction_Roll
	return
EncScript_Regice_FrozenAction_Roll:
	encjumpifchance 50, EncScript_Regice_FrozenAction_Boss
	printstring STRINGID_ENCREGICEFROZENYOU
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_PLAYER1, STATUS1_FREEZE
	encsetrecharge ENC_TARGET_ALL_FOES, 1
	return
EncScript_Regice_FrozenAction_Boss:
	printstring STRINGID_ENCREGICEFROZENBOSS
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_OPPONENT1, STATUS1_FREEZE
	encsetrecharge ENC_TARGET_BOSS, 1
	return

// The two move-driven thaws, sharing the one MoveGuard charge. Fire is worth double, which is what
// makes a Fire attacker the clean answer to the clock - and why Ancient Power is in Regice's
// moveset, so bringing one is not free.
EncScript_Regice_ThawFire::
	encsetvar 7, 1    @ MoveGuard
	printstring STRINGID_ENCREGICETHAWFIRE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ApplyChill
	return

EncScript_Regice_ThawStrike::
	encsetvar 7, 1    @ MoveGuard
	printstring STRINGID_ENCREGICETHAWSTRIKE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regice_ChillDown
	call EncScript_Regice_ApplyChill
	return

// The cold's grip leaves with the mon it was on: stat stages are per-battler and reset on a switch,
// so the ledger has to reset with them or a later thaw would hand the incoming mon free Speed.
EncScript_Regice_ColdLetsGo::
	encsetvar 9, 0    @ Slowed
	return

// TurnClose owns every Chill change that is not move-driven, in a fixed order: the two passive
// thaws first, then the clock. Ticking the clock here and consuming the lost action at TurnOpen is
// the documented split - a timer that ticks and resolves at the same checkpoint chases itself
// through every state in one dispatch and fires instantly.
EncScript_Regice_TurnClose::
	encsetvar 6, 0    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Regice_TurnClose_Done   @ Weakened: inert
	@ Thaw: harsh sunlight standing at the close of the turn. jumpifhalfword rather than
	@ jumpifweatheraffected - the latter reads gBattlerAttacker, which is stale at a checkpoint.
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_SUN, EncScript_Regice_TurnClose_Blow
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_TurnClose_Blow
	printstring STRINGID_ENCREGICETHAWSUN
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	call EncScript_Regice_ChillDown
	@ Thaw: a heavy blow. Measured against the mark TurnOpen took, so it reads "you took 12% off it
	@ this turn" however many hits that took. Deliberately not gated on the physical category
	@ despite the flavour: against 200 base Sp. Def a special attacker needs the lever more, not
	@ less, and gating it would make the lever dead weight for half the rosters that show up.
EncScript_Regice_TurnClose_Blow:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_TurnClose_Clock
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Regice_TurnClose_Clock
	encjumpifvar CMP_LESS_THAN, 10, 12, EncScript_Regice_TurnClose_Clock
	printstring STRINGID_ENCREGICETHAWSTRIKE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regice_ChillDown
EncScript_Regice_TurnClose_Clock:
	call EncScript_Regice_Clock
	call EncScript_Regice_ApplyChill
EncScript_Regice_TurnClose_Done:
	return

// The Tomb. encsettrapped writes the same volatile Mean Look sets with Regice as the trapper, so
// Ghost-types walk out for free (B_GHOSTS_ESCAPE >= GEN_6), a Shed Shell still works, and Regice
// fainting releases everything on its own. Sealing costs the player the switch that was the free
// way to shake the Speed stages off - and in Phase 2 the clock re-seals it every turn, so the loop
// to find is: thaw to break out, switch while the window is open, thaw again.
EncScript_Regice_TombSeal::
	encsetvar 4, 1    @ Tomb
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCREGICETOMBSEAL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	return

EncScript_Regice_TombRelease::
	encsetvar 4, 0    @ Tomb
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	printstring STRINGID_ENCREGICETOMBBREAK
	waitmessage B_WAIT_TIME_SHORT
	return

// 50% - DEEP FREEZE. The ladder is the same ladder and the levers are the same levers; the only
// thing that changes is how fast the cold arrives, and that the cap comes off so Absolute Zero is
// finally on the table. Rise is set to 1 rather than left alone so the new cadence is felt at once.
EncScript_Regice_DeepFreeze::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCREGICEDEEPFREEZE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	encsetvar 3, 1    @ Rise: every second turn from here
	return

// 20% - FROZEN TOMB. The clock goes to every turn and Chill 3 now seals the arena. The stat bump is
// there because one more rung of the guard ladder would not be felt at this point, and the phase
// has to land as something Regice DID.
// Survive is set here rather than in Properties: so the AI reads Regice as killable for four fifths
// of the fight, and from this point it guarantees the player reaches the catch window instead of
// losing it to one oversized hit through a band only 10% of max HP wide. The Immunities: list
// closes the fixed-damage, Perish Song and Destiny Bond holes Survive does not.
EncScript_Regice_FrozenTomb::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCREGICEFROZENTOMBPHASE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TURN_TRAP
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encsetvar 3, 0    @ Rise: every turn from here
	return

// 10% - the catch window. Walking Chill down to 0 through the same subroutine that raised it is
// what hands every held Speed stage back on both sides without the script having to remember how
// many there were. From this point the engine's automatic catch-window damage guard keeps the catch
// target alive; encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Regice_Weakened::
	encsetvar 0, 3    @ Phase
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 4, 0    @ Tomb
	encsetvar 3, 0    @ Rise
	encsetvar 2, 0    @ LastChill
EncScript_Regice_Weakened_Thaw:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regice_Weakened_Release
	call EncScript_Regice_ChillDown
	goto EncScript_Regice_Weakened_Thaw
EncScript_Regice_Weakened_Release:
	removeweather
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCREGICEWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// Regirock, "The Ancient Fortress" (src/data/battle_encounters.encounter). Var indices, pinned by
// the always-true Conditions on EncScript_Regirock_Intro:
// 0 Phase (0 The Fortress Rises / 1 Mountain's Wrath / 2 Collapse / 3 Weakened), 1 Fort
// (Fortification 0-5, the spine), 2 LastFort (tier last ANNOUNCED - the re-fire latch), 3 Build
// (rebuild countdown), 4 Building (1 while a rebuild is in progress), 5 BuildMark (HP% mark for the
// rebuild interrupt test), 6 HpMark (HP% mark at turn open, for the quiet-turn test), 7 Fall
// (Rockfall countdown), 8 Collapse (Phase 2 finisher clock), 9 Braced (braced last turn - drives
// the alternating brace cadence AND suppresses quiet growth), 10 Shift (per-turn one-change guard
// shared by the three move-event triggers, in BOTH directions), 11 Shaken (sleep anti-cheese
// guard), 12 TurnGuard.
//
// Groudon is a ratchet you pry back down; Regice eats your turns. Regirock is a WALL you have to
// pick the right tool for, and the wrong tool makes it thicker. Every Fortification effect is
// RE-DERIVED from Var(Fort), never accumulated: the damage reduction is replaced rather than added
// and the three side statuses are cleared and re-raised for the current tier, so a Fortification
// that runs 0-3-1-4-2 leaves no residue and nothing has to remember how to undo a tier.

// --- Shared subroutines (call/return) ---

// Sole owner of everything Fortification means. The fall-through cascade at the bottom is what
// makes the tiers cumulative without restating them: tier 5 drops into Mist, which drops into
// Reflect, which drops into Lucky Chant.
// Baseline 88 is the guideline floor every legendary shares; the top rung is only ever reached by
// letting the wall run away.
EncScript_Regirock_ApplyLayers:
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_LUCKY_CHANT
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regirock_Layers_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Regirock_Layers_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Regirock_Layers_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Regirock_Layers_Two
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Regirock_Layers_One
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Regirock_Layers_One:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Regirock_Layers_Two:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	goto EncScript_Regirock_Layers_Chant
EncScript_Regirock_Layers_Three:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	goto EncScript_Regirock_Layers_Screen
EncScript_Regirock_Layers_Four:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	goto EncScript_Regirock_Layers_Mist
EncScript_Regirock_Layers_Five:
	encsetdamagereduction ENC_TARGET_BOSS, 97
EncScript_Regirock_Layers_Mist:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST, 0
EncScript_Regirock_Layers_Screen:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT, 0
EncScript_Regirock_Layers_Chant:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LUCKY_CHANT, 0
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones on an unchanged tier. Falling back to
// Fort 0 resets the latch through the leading branch, so a wall rebuilt from nothing announces its
// whole climb again.
// Tier 3 gets the Reflect animation because it is the one layer with a screen graphic to show.
EncScript_Regirock_FortCue:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regirock_FortCue_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Regirock_FortCue_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Regirock_FortCue_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Regirock_FortCue_Two
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Regirock_FortCue_One
	encsetvar 2, 0    @ LastFort
	return
EncScript_Regirock_FortCue_One:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Regirock_FortCue_Done
	encsetvar 2, 1
	printstring STRINGID_ENCREGIROCKTIERONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regirock_FortCue_Two:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Regirock_FortCue_Done
	encsetvar 2, 2
	printstring STRINGID_ENCREGIROCKTIERTWO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Regirock_FortCue_Three:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Regirock_FortCue_Done
	encsetvar 2, 3
	printstring STRINGID_ENCREGIROCKTIERTHREE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_REFLECT
	return
EncScript_Regirock_FortCue_Four:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Regirock_FortCue_Done
	encsetvar 2, 4
	printstring STRINGID_ENCREGIROCKTIERFOUR
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Regirock_FortCue_Five:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Regirock_FortCue_Done
	encsetvar 2, 5
	printstring STRINGID_ENCREGIROCKTIERFIVE
	waitmessage B_WAIT_TIME_LONG
EncScript_Regirock_FortCue_Done:
	return

// Called by every path that moves Fortification. TurnOpen calls it once at the end of its pass; the
// move-driven shifts call it themselves so a layer broken mid-turn is felt on the very next hit
// rather than next turn.
EncScript_Regirock_ApplyFort:
	call EncScript_Regirock_ApplyLayers
	call EncScript_Regirock_FortCue
	return

// The only two places Fortification ever moves. encaddvar has no ceiling and encsubvar underflows a
// u8 to 255, so both bounds are tested first - and routing every change through here is what
// guarantees each one is announced.
EncScript_Regirock_FortUp:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Regirock_FortUp_Done
	encaddvar 1, 1
	printstring STRINGID_ENCREGIROCKFORTUP
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_HARDEN
EncScript_Regirock_FortUp_Done:
	return

EncScript_Regirock_FortDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regirock_FortDown_Done
	encsubvar 1, 1
	printstring STRINGID_ENCREGIROCKFORTDOWN
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_ROCK_SMASH
EncScript_Regirock_FortDown_Done:
	return

// The quiet turn: the spec's "several turns pass without it being significantly damaged" kept
// literally. HpMark was set at the previous turn open, so ENC_SNAP_DAMAGE turns it into "percent of
// max HP lost over the last turn"; the command jumps its fail label when nothing was written, which
// is the zero-damage case and routes to the same place a small hit does.
// Skipped after a braced turn - a turn Regirock spent behind Protect is not the player's failure to
// hurt it. The mark is re-taken on every path so the window is always exactly one turn.
// 2% of max HP is a deliberately low bar: the layer punishes a turn spent doing NOTHING to
// Regirock, not a turn that failed to do enough.
EncScript_Regirock_QuietTurn:
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Regirock_QuietTurn_Mark   @ Braced
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_DAMAGE, EncScript_Regirock_QuietTurn_Quiet
	encjumpifvar CMP_GREATER_THAN, 6, 1, EncScript_Regirock_QuietTurn_Mark
EncScript_Regirock_QuietTurn_Quiet:
	printstring STRINGID_ENCREGIROCKQUIET
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Regirock_FortUp
EncScript_Regirock_QuietTurn_Mark:
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET
	return

// Phase 2 only: the mountain sheds a layer by itself every turn and the debris rains on the party,
// pulling against the rebuild clock. That is the spec's "Fortification rapidly fluctuates",
// expressed as two existing clocks pulling opposite ways.
// Refused at Fortification 0 for the same reason Rockfall is - there is nothing left to fall.
EncScript_Regirock_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Regirock_Shed_Done
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regirock_Shed_Done
	call EncScript_Regirock_FortDown
	printstring STRINGID_ENCREGIROCKBREAKAWAY
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_ROCK_THROW
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
EncScript_Regirock_Shed_Done:
	return

// The rebuild clock. Build counts down here and fires at 1 rather than being ticked to 0 and fired
// in the same pass, so the announced cadence is the cadence the player actually feels. 0 is inert.
// encsetrecharge with 1 at OnTurnStart costs Regirock THIS turn's action and the engine prints its
// own "must recharge!" line when the action would have resolved, so the free turn is real and
// visible. The mark taken here is what RebuildResolve measures the interrupt against.
EncScript_Regirock_BuildTick:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Regirock_BuildTick_Done
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Regirock_BuildTick_Begin
	encsubvar 3, 1
	return
EncScript_Regirock_BuildTick_Begin:
	printstring STRINGID_ENCREGIROCKREBUILD
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_POLISH
	encsetrecharge ENC_TARGET_BOSS, 1
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_SET   @ BuildMark
	encsetvar 4, 1    @ Building
EncScript_Regirock_BuildTick_Done:
	return

// Re-arm values are one higher than the gap they produce, because BuildTick fires on 1: 4 is a
// rebuild every fourth turn, 3 every third.
EncScript_Regirock_BuildArm:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regirock_BuildArm_Slow
	encsetvar 3, 3
	return
EncScript_Regirock_BuildArm_Slow:
	encsetvar 3, 4
	return

// The Rockfall and collapse clocks. Both tick here and resolve at OnTurnEnd - the documented split,
// so a re-arm can never be eaten by its own tick.
EncScript_Regirock_FallTick:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Regirock_FallTick_Done
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Regirock_FallTick_Done
	encsubvar 7, 1
EncScript_Regirock_FallTick_Done:
	return

EncScript_Regirock_FallArm:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Regirock_FallArm_Mid
	encsetvar 7, 3    @ Collapse: a Rockfall every second turn
	return
EncScript_Regirock_FallArm_Mid:
	encsetvar 7, 4    @ Mountain's Wrath: every third turn
	return

EncScript_Regirock_CollapseClock:
	encjumpifvar CMP_LESS_THAN, 8, 2, EncScript_Regirock_CollapseClock_Done
	encsubvar 8, 1
EncScript_Regirock_CollapseClock_Done:
	return

// Mountain Form's brace, at Fortification 5 only. encsetprotect at OnTurnStart is the one checkpoint
// where it works - gProtectStructs is cleared after end-of-turn effects, so set here it covers the
// whole turn and expires on its own.
// Var(Braced) is both the alternation flag and the quiet-turn suppressor: a turn that braces sets
// it, the next turn reads it (skipping the quiet growth, since a braced turn is not the player's
// failure) and clears it, so the brace lands every other turn. Falling below Fortification 5 clears
// it through the same tail, so a lost layer can never leave the suppressor stuck on.
EncScript_Regirock_Brace:
	encjumpifvar CMP_NOT_EQUAL, 1, 5, EncScript_Regirock_Brace_Rest
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Regirock_Brace_Rest
	encsetvar 9, 1    @ Braced
	printstring STRINGID_ENCREGIROCKBRACE
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_PROTECT
	encsetprotect ENC_TARGET_BOSS
	return
EncScript_Regirock_Brace_Rest:
	encsetvar 9, 0    @ Braced
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the clocks and the marks need seeding.
// Braced starts at 1 purely to suppress the quiet-turn test on turn 1, which would otherwise see
// zero damage taken before anyone has moved and hand out a free layer; the Fortification 0 tail of
// the brace routine clears it in the same pass.
EncScript_Regirock_Intro::
	encsetvar 3, 4    @ Build: the first reconstruction lands on turn 4
	encsetvar 9, 1    @ Braced
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET   @ HpMark
	printstring STRINGID_ENCREGIROCKAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_STEALTH_ROCK
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail so no branch out of this script can leave it re-armed.
// Order matters. The quiet-turn test reads the mark before anything else can move Regirock's HP;
// the three clocks tick before the ladder is re-derived, so a layer gained or lost by a clock is
// already reflected in the guard the player meets this turn; the brace runs last because it is the
// only step that depends on the final value of Fortification.
// The closing line is the recurring "the mechanic is still on" heartbeat - a player who missed the
// tier callout still sees, every turn, that something is wrong with the wall.
EncScript_Regirock_TurnOpen::
	flushtextbox
	encsetvar 12, 1    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Regirock_TurnOpen_Done   @ Weakened: inert
	call EncScript_Regirock_QuietTurn
	call EncScript_Regirock_Shed
	call EncScript_Regirock_BuildTick
	call EncScript_Regirock_FallTick
	call EncScript_Regirock_CollapseClock
	call EncScript_Regirock_ApplyFort
	call EncScript_Regirock_Brace
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Regirock_TurnOpen_Done
	printstring STRINGID_ENCREGIROCKGRIND
	waitmessage B_WAIT_TIME_SHORT
EncScript_Regirock_TurnOpen_Done:
	return

// TurnClose owns nothing but the per-turn guard resets. Every countdown lives at TurnOpen instead,
// one checkpoint away from the OnTurnEnd resolutions that re-arm them.
EncScript_Regirock_TurnClose::
	encsetvar 12, 0    @ TurnGuard
	encsetvar 10, 0    @ Shift: the one move-driven change is armed again
	encsetvar 11, 0    @ Shaken
	return

// Anti-cheese. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at
// OnTurnStart - and the wall thickens for the trouble, so putting the mountain to sleep is not free.
// Leads with flushtextbox for the same reason TurnOpen does: this runs one priority earlier.
EncScript_Regirock_WillNotSleep::
	flushtextbox
	encsetvar 11, 1    @ Shaken
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCREGIROCKNOSLEEP
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Regirock_FortUp
	call EncScript_Regirock_ApplyFort
	return

// The three shift scripts. Each sets Var(Shift) itself: an Event.* condition stays true for the
// whole dispatch and cannot be moved by the script, so an event-reacting trigger has to disable
// itself or the re-evaluation loop re-selects it until the per-checkpoint script cap trips.
// The break is the fight's safety valve and is deliberately damage-independent - it does not look
// at how much the move did, only at its type.
EncScript_Regirock_Break::
	encsetvar 10, 1    @ Shift
	call EncScript_Regirock_FortDown
	call EncScript_Regirock_ApplyFort
	return

EncScript_Regirock_Build::
	encsetvar 10, 1    @ Shift
	call EncScript_Regirock_FortUp
	call EncScript_Regirock_ApplyFort
	return

EncScript_Regirock_BossBuild::
	encsetvar 10, 1    @ Shift
	call EncScript_Regirock_FortUp
	call EncScript_Regirock_ApplyFort
	return

// The rebuild, resolved. BuildMark holds the HP% taken at turn open, so ENC_SNAP_DAMAGE turns it
// into "percent of max HP lost since the reconstruction began"; the fail label is the zero-damage
// case and routes to the same place a small hit does.
// 3% of max HP through an 88-97% reduction is a bar a real attack clears and a wasted turn does
// not, so interrupting is about SPENDING the free turn rather than about passing a burst check.
// Completing is worth one layer and a 3% heal, so the swing across the interrupt is two layers
// rather than three. The clock is re-armed up front so every path leaves it armed.
EncScript_Regirock_RebuildResolve::
	encsetvar 4, 0    @ Building
	call EncScript_Regirock_BuildArm
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Regirock_RebuildResolve_Whole
	encjumpifvar CMP_LESS_THAN, 5, 3, EncScript_Regirock_RebuildResolve_Whole
	printstring STRINGID_ENCREGIROCKREBUILDBROKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_BLAST
	call EncScript_Regirock_FortDown
	goto EncScript_Regirock_RebuildResolve_Apply
EncScript_Regirock_RebuildResolve_Whole:
	printstring STRINGID_ENCREGIROCKREBUILDDONE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Regirock_FortUp
	enchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Regirock_RebuildResolve_Apply:
	call EncScript_Regirock_ApplyFort
	return

// Rockfall. Regirock breaks off a layer and throws it, so the payload scales with how thick the
// wall was - and at Fortification 0 it is REFUSED and told so. That refusal is the payoff for the
// whole break-the-wall game: a stripped fortress has nothing to throw, and keeping it stripped is
// now offence as well as defence.
// The raw Defense loss is the fight's one accumulating effect, and it is monotone - Rockfall only
// ever fires downward - so the second half genuinely gets easier the more Regirock throws. 3% a
// throw, in proportion to the smaller payload each throw now costs the player.
// ENC_TARGET_ALL_FOES rather than a single slot throughout: Rockfall can KO, and a group target
// skips a fainted battler silently where a single-slot target would assert.
EncScript_Regirock_RockfallResolve::
	call EncScript_Regirock_FallArm
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Regirock_RockfallResolve_Nothing
	printstring STRINGID_ENCREGIROCKROCKFALL
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_SLIDE
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Regirock_RockfallResolve_Huge
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Regirock_RockfallResolve_Big
	enchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	goto EncScript_Regirock_RockfallResolve_Spend
EncScript_Regirock_RockfallResolve_Big:
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	goto EncScript_Regirock_RockfallResolve_Spend
EncScript_Regirock_RockfallResolve_Huge:
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
EncScript_Regirock_RockfallResolve_Spend:
	call EncScript_Regirock_FortDown
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -3, ENC_AMOUNT_PERCENT
	call EncScript_Regirock_ApplyFort
	return
EncScript_Regirock_RockfallResolve_Nothing:
	printstring STRINGID_ENCREGIROCKNOTHINGLEFT
	waitmessage B_WAIT_TIME_SHORT
	return

// 50% - MOUNTAIN'S WRATH. It stops hiding behind the fortress and starts throwing it. Fall is set
// to 2 so the first Rockfall lands at the close of the next turn - one turn of warning after the
// line that announces it. The Attack bump is the fight's one offensive step-up, applied once and
// monotonically, so the second half hits harder even as Rockfall bleeds its Defense away.
EncScript_Regirock_MountainsWrath::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCREGIROCKWRATH
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ANCIENT_POWER
	encsetvar 7, 2    @ Fall
	call EncScript_Regirock_BuildArm
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	return

// 25% - COLLAPSE. A layer falls off by itself every turn and the debris rains on the party, the
// rebuild keeps pulling the other way on its own cadence, Rockfall doubles up, and a fixed clock
// starts running down to the finisher. Fall is set to 1 so a Rockfall lands at the end of this very
// turn - the phase arriving with a hit rather than an announcement.
// Survive is set here rather than in Properties: so the AI reads Regirock as killable for three
// quarters of the fight, and from this point it guarantees the player reaches the catch window
// instead of losing it to one oversized hit through a band only 10% of max HP wide.
EncScript_Regirock_Collapse::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCREGIROCKCOLLAPSING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_THROW
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encsetvar 8, 6    @ Collapse: five turns to MOUNTAIN'S COLLAPSE
	encsetvar 7, 1    @ Fall
	call EncScript_Regirock_BuildArm
	return

// MOUNTAIN'S COLLAPSE. The finisher, and the guarantee that the catch window always opens however
// the damage race went: ENC_AMOUNT_TO_PERCENT is an absolute destination, not a delta.
// encclearscreens takes a fail label rather than failing, so the same command is both "did the
// player have anything up?" and "take it away" - whatever they built goes down with the cavern.
// Every clock is zeroed on the way out and the ladder re-derived from Fort 0, so all three side
// statuses and the whole guard go at once. Weakened (priority 7) wins the next re-evaluation pass
// at this same checkpoint.
EncScript_Regirock_CollapseTick::
	encsetvar 8, 0    @ Collapse: the finisher never repeats
	printstring STRINGID_ENCREGIROCKMOUNTAINFALLS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_WRECKER
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Regirock_CollapseTick_Hit
EncScript_Regirock_CollapseTick_Hit:
	enchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetvar 1, 0    @ Fort
	encsetvar 3, 0    @ Build: nothing rebuilds after the mountain falls
	encsetvar 7, 0    @ Fall
	encsetvar 4, 0    @ Building
	call EncScript_Regirock_ApplyFort
	return

// 10% - the catch window, the mirror image of the Properties: block. Every clock is made inert and
// every layer stripped through the same routine that raised them, so no side status can outlive the
// fight it belonged to. Phase 3 makes TurnOpen return through its leading branch, so nothing can
// restart after this point.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Regirock_Weakened::
	encsetvar 0, 3    @ Phase
	encsetvar 1, 0    @ Fort
	encsetvar 2, 0    @ LastFort
	encsetvar 3, 0    @ Build
	encsetvar 4, 0    @ Building
	encsetvar 7, 0    @ Fall
	encsetvar 8, 0    @ Collapse
	encsetvar 9, 0    @ Braced
	call EncScript_Regirock_ApplyLayers
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 25
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCREGIROCKWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
