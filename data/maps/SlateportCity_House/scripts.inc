SlateportCity_House_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SlateportCity_House_OnLoad
	.byte 0

SlateportCity_House_OnLoad:
	@ Rotom TV chain, stage 1 of 9: postgame, not owned, previous stage cleared
	setvar VAR_0x8004, SPECIES_ROTOM
	setvar VAR_0x8005, LAYOUT_HOUSE2
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_House_HideRotomTV
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 0, SlateportCity_House_HideRotomTV
	goto SlateportCity_House_ShowRotomTV

SlateportCity_House_Rotom_Encounter::
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 0, SlateportCity_House_EventScript_RotomTVNothing
	lockall
	msgbox SlateportCity_House_Text_RotomInterference, MSGBOX_DEFAULT
	addvar VAR_ROTOM_TV_PROGRESS, 1
	special TurnOffTVScreen
	releaseall
	end

SlateportCity_House_EventScript_RotomTVNothing:
	end

SlateportCity_House_HideRotomTV:
	end

SlateportCity_House_ShowRotomTV:
	special TurnOnTVScreen
	end

SlateportCity_House_Text_RotomInterference:
	.string "BZZT!\pThe interference on the TV\n"
	.string "suddenly vanished...$"

SlateportCity_House_EventScript_PokefanM::
	msgbox SlateportCity_House_Text_NatureToDoWithStatGains, MSGBOX_NPC
	end

SlateportCity_House_EventScript_Girl::
	msgbox SlateportCity_House_Text_MustBeGoingToBattleTent, MSGBOX_NPC
	end

SlateportCity_House_Text_NatureToDoWithStatGains:
	.string "My POKéMON has a HASTY nature.\p"
	.string "It has higher SPEED compared to\n"
	.string "my other POKéMON.\p"
	.string "Maybe their nature has something to\n"
	.string "do with the stat gains of POKéMON.$"

SlateportCity_House_Text_MustBeGoingToBattleTent:
	.string "You're a TRAINER, aren't you?\p"
	.string "Since you came to SLATEPORT CITY,\n"
	.string "you must be going to the BATTLE TENT.$"

