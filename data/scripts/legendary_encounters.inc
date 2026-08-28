// Shared home for every legendary encounter's overworld trigger script. A map just needs to
// `call`/`goto` the entry point for its own legendary; the encounter's actual behavior lives in
// src/data/battle_encounters.encounter and data/battle_scripts_encounters.s.

LegendaryEncounter_EventScript_Articuno::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ARTICUNO, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ARTICUNO
	setwildbattle SPECIES_ARTICUNO, 50        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Zapdos::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZAPDOS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ZAPDOS
	setwildbattle SPECIES_ZAPDOS, 50        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Moltres::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MOLTRES, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_MOLTRES
	setwildbattle SPECIES_MOLTRES, 50        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Mewtwo::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MEWTWO, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_MEWTWO
	setwildbattle SPECIES_MEWTWO, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Mew::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MEW, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_MEW
	setwildbattle SPECIES_MEW, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Raikou::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_RAIKOU, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_RAIKOU
	setwildbattle SPECIES_RAIKOU, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

@ Shared post-battle tail. Only a caught legendary is removed from the overworld; fainting or
@ fleeing leaves it in place so the fight can be retried. Each host map's ON_TRANSITION runs
@ `updatelegendaryvisibility`, which brings the object back if the player later releases the mon.
LegendaryEncounter_EventScript_FinishBattle::
	specialvar VAR_RESULT, GetBattleOutcome
	call_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_RemoveCaughtLegendary
	releaseall
	end

LegendaryEncounter_EventScript_RemoveCaughtLegendary::
	removeobject VAR_LAST_TALKED
	return
