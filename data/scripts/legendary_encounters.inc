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
	releaseall
	end

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
	releaseall
	end
