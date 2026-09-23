SootopolisCity_House7_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SootopolisCity_House7_OnLoad
	.byte 0

SootopolisCity_House7_OnLoad:
	@ Rotom TV chain, stage 7 of 9: postgame, not owned, previous stage cleared
	setvar VAR_0x8004, SPECIES_ROTOM
	setvar VAR_0x8005, LAYOUT_SOOTOPOLIS_CITY_HOUSE1
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, SootopolisCity_House7_HideRotomTV
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 6, SootopolisCity_House7_HideRotomTV
	goto SootopolisCity_House7_ShowRotomTV

SootopolisCity_House7_Rotom_Encounter::
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 6, SootopolisCity_House7_EventScript_RotomTVNothing
	lockall
	msgbox SootopolisCity_House7_Text_RotomInterference, MSGBOX_DEFAULT
	addvar VAR_ROTOM_TV_PROGRESS, 1
	special TurnOffTVScreen
	releaseall
	end

SootopolisCity_House7_EventScript_RotomTVNothing:
	end

SootopolisCity_House7_HideRotomTV:
	end

SootopolisCity_House7_ShowRotomTV:
	special TurnOnTVScreen
	end

SootopolisCity_House7_Text_RotomInterference:
	.string "BZZT!\pThe interference on the TV\n"
	.string "suddenly vanished...$"

SootopolisCity_House7_EventScript_OldMan::
	msgbox SootopolisCity_House7_Text_CityFromEruptedVolcano, MSGBOX_NPC
	end

SootopolisCity_House7_EventScript_PokefanF::
	msgbox SootopolisCity_House7_Text_CaveMadeToKeepSomething, MSGBOX_NPC
	end

SootopolisCity_House7_Text_CityFromEruptedVolcano:
	.string "An underwater volcano erupted and\n"
	.string "forced itself up from the depths.\p"
	.string "Its crater emerged from the sea and\n"
	.string "became filled with rainwater.\p"
	.string "That's how SOOTOPOLIS CITY came into\n"
	.string "being.$"

SootopolisCity_House7_Text_CaveMadeToKeepSomething:
	.string "The cave that links SOOTOPOLIS and\n"
	.string "the outside world…\p"
	.string "It seems as if the cave was made to\n"
	.string "keep something from getting out.\p"
	.string "Or am I just imagining things?$"

