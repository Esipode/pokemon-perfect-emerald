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
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_MEWTWONITE_X
	giveitem ITEM_MEWTWONITE_Y
	releaseall
	end

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
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_MEWNIUM_Z
	releaseall
	end

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
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_BLUE_ORB
	releaseall
	end

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
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_RED_ORB
	releaseall
	end

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
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_FLAME_PLATE
	giveitem ITEM_SPLASH_PLATE
	giveitem ITEM_ZAP_PLATE
	giveitem ITEM_MEADOW_PLATE
	giveitem ITEM_ICICLE_PLATE
	giveitem ITEM_FIST_PLATE
	giveitem ITEM_TOXIC_PLATE
	giveitem ITEM_EARTH_PLATE
	giveitem ITEM_SKY_PLATE
	giveitem ITEM_MIND_PLATE
	giveitem ITEM_INSECT_PLATE
	giveitem ITEM_STONE_PLATE
	giveitem ITEM_SPOOKY_PLATE
	giveitem ITEM_DRACO_PLATE
	giveitem ITEM_DREAD_PLATE
	giveitem ITEM_IRON_PLATE
	giveitem ITEM_PIXIE_PLATE
	releaseall
	end

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

LegendaryEncounter_EventScript_Rotom::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ROTOM, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ROTOM
	setwildbattle SPECIES_ROTOM, 50         @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_ROTOM_CATALOG
	releaseall
	end

LegendaryEncounter_EventScript_Shaymin::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_SHAYMIN, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_SHAYMIN
	setwildbattle SPECIES_SHAYMIN, 50       @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_GRACIDEA
	releaseall
	end

LegendaryEncounter_EventScript_Heatran::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_HEATRAN, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_HEATRAN
	setwildbattle SPECIES_HEATRAN, 50       @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_HEATRANITE
	releaseall
	end

LegendaryEncounter_EventScript_Manaphy::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MANAPHY, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_MANAPHY
	setwildbattle SPECIES_MANAPHY, 50       @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Darkrai::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_DARKRAI, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_DARKRAI
	setwildbattle SPECIES_DARKRAI, 50       @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_DARKRANITE
	releaseall
	end

LegendaryEncounter_EventScript_Cresselia::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_CRESSELIA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_CRESSELIA
	setwildbattle SPECIES_CRESSELIA, 50     @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Giratina::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_GIRATINA_ALTERED, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_GIRATINA
	setwildbattle SPECIES_GIRATINA_ALTERED, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_GRISEOUS_ORB
	giveitem ITEM_GRISEOUS_CORE
	releaseall
	end

LegendaryEncounter_EventScript_Dialga::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_DIALGA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_DIALGA
	setwildbattle SPECIES_DIALGA, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_ADAMANT_CRYSTAL
	releaseall
	end

LegendaryEncounter_EventScript_Palkia::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_PALKIA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_PALKIA
	setwildbattle SPECIES_PALKIA, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_LUSTROUS_GLOBE
	releaseall
	end

LegendaryEncounter_EventScript_Cobalion::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_COBALION, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_COBALION
	setwildbattle SPECIES_COBALION, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Terrakion::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_TERRAKION, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_TERRAKION
	setwildbattle SPECIES_TERRAKION, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Virizion::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_VIRIZION, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_VIRIZION
	setwildbattle SPECIES_VIRIZION, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Genesect::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_GENESECT, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_GENESECT
	setwildbattle SPECIES_GENESECT, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_DOUSE_DRIVE
	giveitem ITEM_SHOCK_DRIVE
	giveitem ITEM_BURN_DRIVE
	giveitem ITEM_CHILL_DRIVE
	releaseall
	end

LegendaryEncounter_EventScript_Keldeo::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_KELDEO, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_KELDEO
	setwildbattle SPECIES_KELDEO, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Kyurem::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_KYUREM, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_KYUREM
	setwildbattle SPECIES_KYUREM, 50   @ level is overridden by Level: LevelCap
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

LegendaryEncounter_EventScript_Landorus::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_LANDORUS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_LANDORUS
	setwildbattle SPECIES_LANDORUS, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, LegendaryEncounter_EventScript_FinishBattle
	call LegendaryEncounter_EventScript_RemoveCaughtLegendary
	giveitem ITEM_REVEAL_GLASS
	releaseall
	end

LegendaryEncounter_EventScript_Thundurus::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_THUNDURUS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_THUNDURUS
	setwildbattle SPECIES_THUNDURUS, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Meloetta::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MELOETTA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_MELOETTA
	setwildbattle SPECIES_MELOETTA, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Tornadus::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_TORNADUS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_TORNADUS
	setwildbattle SPECIES_TORNADUS, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Reshiram::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_RESHIRAM, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_RESHIRAM
	setwildbattle SPECIES_RESHIRAM, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Zekrom::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZEKROM, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ZEKROM
	setwildbattle SPECIES_ZEKROM, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Diancie::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_DIANCIE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_DIANCIE
	setwildbattle SPECIES_DIANCIE, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Hoopa::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_HOOPA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_HOOPA
	setwildbattle SPECIES_HOOPA, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Volcanion::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_VOLCANION, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_VOLCANION
	setwildbattle SPECIES_VOLCANION, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Zygarde::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZYGARDE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ZYGARDE
	setwildbattle SPECIES_ZYGARDE, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Xerneas::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_XERNEAS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_XERNEAS
	setwildbattle SPECIES_XERNEAS, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Yveltal::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_YVELTAL, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_YVELTAL
	setwildbattle SPECIES_YVELTAL, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Necrozma::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_NECROZMA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_NECROZMA
	setwildbattle SPECIES_NECROZMA, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Zeraora::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZERAORA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_ZERAORA
	setwildbattle SPECIES_ZERAORA, 50, ITEM_ZERAORITE   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_TapuKoko::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_TAPU_KOKO, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_TAPU_KOKO
	setwildbattle SPECIES_TAPU_KOKO, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_Magearna::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MAGEARNA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_MAGEARNA
	setwildbattle SPECIES_MAGEARNA, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_TapuLele::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_TAPU_LELE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_TAPU_LELE
	setwildbattle SPECIES_TAPU_LELE, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_TapuBulu::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_TAPU_BULU, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_TAPU_BULU
	setwildbattle SPECIES_TAPU_BULU, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_TapuFini::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_TAPU_FINI, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_TAPU_FINI
	setwildbattle SPECIES_TAPU_FINI, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle

LegendaryEncounter_EventScript_GalarianArticuno::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ARTICUNO_GALAR, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setbattleencounter ENCOUNTER_GALARIAN_ARTICUNO
	setwildbattle SPECIES_ARTICUNO_GALAR, 50   @ level is overridden by Level: LevelCap
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto LegendaryEncounter_EventScript_FinishBattle
