OldaleTown_House2_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, OldaleTown_House2_OnLoad
	.byte 0

OldaleTown_House2_OnLoad:
	@ Rotom TV chain, stage 6 of 9: postgame, not owned, previous stage cleared
	setvar VAR_0x8004, SPECIES_ROTOM
	setvar VAR_0x8005, LAYOUT_HOUSE2
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, OldaleTown_House2_HideRotomTV
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 5, OldaleTown_House2_HideRotomTV
	goto OldaleTown_House2_ShowRotomTV

OldaleTown_House2_Rotom_Encounter::
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 5, OldaleTown_House2_EventScript_RotomTVNothing
	lockall
	msgbox OldaleTown_House2_Text_RotomInterference, MSGBOX_DEFAULT
	addvar VAR_ROTOM_TV_PROGRESS, 1
	special TurnOffTVScreen
	releaseall
	end

OldaleTown_House2_EventScript_RotomTVNothing:
	end

OldaleTown_House2_HideRotomTV:
	end

OldaleTown_House2_ShowRotomTV:
	special TurnOnTVScreen
	end

OldaleTown_House2_Text_RotomInterference:
	.string "BZZT!\pThe interference on the TV\n"
	.string "suddenly vanished...$"

OldaleTown_House2_EventScript_Woman::
	msgbox OldaleTown_House2_Text_PokemonLevelUp, MSGBOX_NPC
	end

OldaleTown_House2_EventScript_Man::
	msgbox OldaleTown_House2_Text_YoullGoFurtherWithStrongPokemon, MSGBOX_NPC
	end

OldaleTown_House2_Text_PokemonLevelUp:
	.string "When POKéMON battle, they eventually\n"
	.string "level up and become stronger.$"

OldaleTown_House2_Text_YoullGoFurtherWithStrongPokemon:
	.string "If the POKéMON with you become\n"
	.string "stronger, you'll be able to go farther\l"
	.string "away from here.$"

