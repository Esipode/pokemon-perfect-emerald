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
	encchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT   @ percent, not raw HP: the boss's level (and so its max HP) comes from the Level: property
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
	encchangehp ENC_TARGET_BOSS, -30
	return

EncScript_TestChangeHpHeal::
	encchangehp ENC_TARGET_BOSS, 20
	return

EncScript_TestChangeHpAllFoes::
	encchangehp ENC_TARGET_ALL_FOES, -15
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
	encchangehp ENC_TARGET_BOSS, 60
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

// Legendary encounter scripts, one file per boss. Their encounter definitions live alongside them
// in src/data/legendary_encounters/.

	.include "data/legendary_encounters/articuno.inc"
	.include "data/legendary_encounters/zapdos.inc"
	.include "data/legendary_encounters/moltres.inc"
	.include "data/legendary_encounters/mewtwo.inc"
	.include "data/legendary_encounters/mew.inc"
	.include "data/legendary_encounters/raikou.inc"
	.include "data/legendary_encounters/entei.inc"
	.include "data/legendary_encounters/suicune.inc"
	.include "data/legendary_encounters/lugia.inc"
	.include "data/legendary_encounters/ho_oh.inc"
	.include "data/legendary_encounters/celebi.inc"
	.include "data/legendary_encounters/kyogre.inc"
	.include "data/legendary_encounters/groudon.inc"
	.include "data/legendary_encounters/rayquaza.inc"
	.include "data/legendary_encounters/regirock.inc"
	.include "data/legendary_encounters/regice.inc"
	.include "data/legendary_encounters/registeel.inc"
	.include "data/legendary_encounters/arceus.inc"
	.include "data/legendary_encounters/jirachi.inc"
	.include "data/legendary_encounters/deoxys.inc"
	.include "data/legendary_encounters/azelf.inc"
	.include "data/legendary_encounters/uxie.inc"
	.include "data/legendary_encounters/mesprit.inc"
	.include "data/legendary_encounters/regigigas.inc"
	.include "data/legendary_encounters/rotom.inc"
	.include "data/legendary_encounters/shaymin.inc"
	.include "data/legendary_encounters/heatran.inc"
	.include "data/legendary_encounters/manaphy.inc"
	.include "data/legendary_encounters/darkrai.inc"
	.include "data/legendary_encounters/cresselia.inc"
	.include "data/legendary_encounters/giratina.inc"
