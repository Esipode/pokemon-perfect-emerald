// Shared home for every legendary encounter's overworld trigger script. A map just needs to
// `call`/`goto` the entry point for its own legendary; the encounter's actual behavior lives in
// src/data/legendary_encounters/ and data/legendary_encounters/.

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

LegendaryEncounter_EventScript_Entei::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ENTEI, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ENTEI
	setwildbattle SPECIES_ENTEI, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Suicune::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_SUICUNE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_SUICUNE
	setwildbattle SPECIES_SUICUNE, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Celebi::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_CELEBI, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_CELEBI
	setwildbattle SPECIES_CELEBI, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Lugia::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_LUGIA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_LUGIA
	setwildbattle SPECIES_LUGIA, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_HoOh::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_HO_OH, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_HO_OH
	setwildbattle SPECIES_HO_OH, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Deoxys::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_DEOXYS_NORMAL, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_DEOXYS
	setwildbattle SPECIES_DEOXYS_NORMAL, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Jirachi::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_JIRACHI, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_JIRACHI
	setwildbattle SPECIES_JIRACHI, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Rayquaza::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_RAYQUAZA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_RAYQUAZA
	setwildbattle SPECIES_RAYQUAZA, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Kyogre::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_KYOGRE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_KYOGRE
	setwildbattle SPECIES_KYOGRE, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Groudon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_GROUDON, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_GROUDON
	setwildbattle SPECIES_GROUDON, 70        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Regice::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_REGICE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_REGICE
	setwildbattle SPECIES_REGICE, 50        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Regirock::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_REGIROCK, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_REGIROCK
	setwildbattle SPECIES_REGIROCK, 50      @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Registeel::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_REGISTEEL, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_REGISTEEL
	setwildbattle SPECIES_REGISTEEL, 50     @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Arceus::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ARCEUS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ARCEUS
	setwildbattle SPECIES_ARCEUS, 50        @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Azelf::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_AZELF, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_AZELF
	setwildbattle SPECIES_AZELF, 50         @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Uxie::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_UXIE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_UXIE
	setwildbattle SPECIES_UXIE, 50          @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Mesprit::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MESPRIT, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_MESPRIT
	setwildbattle SPECIES_MESPRIT, 50       @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Regigigas::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_REGIGIGAS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_REGIGIGAS
	setwildbattle SPECIES_REGIGIGAS, 50     @ level is overridden by Level: LevelCap
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
