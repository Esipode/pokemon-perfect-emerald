Route106_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route106_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route106_OnLoad
	.byte 0

Route106_OnTransition:
	updatelegendaryvisibility SPECIES_MANAPHY, FLAG_HIDE_MANAPHY
	end

Route106_OnLoad:
	@ Manaphy spawn gate: post-game, not already owned, rain or thunderstorm
	setvar VAR_0x8004, SPECIES_MANAPHY
	setvar VAR_0x8005, LAYOUT_ROUTE106
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route106_HideManaphyArena
	getmapweather VAR_0x8008
	goto_if_eq VAR_0x8008, WEATHER_RAIN, Route106_ManaphyWeatherOk
	goto_if_ne VAR_0x8008, WEATHER_RAIN_THUNDERSTORM, Route106_HideManaphyArena
Route106_ManaphyWeatherOk:
	goto Route106_ShowManaphyArena

Route106_Manaphy_Encounter::
	goto LegendaryEncounter_EventScript_Manaphy

Route106_HideManaphyArena:
	setflag FLAG_HIDE_MANAPHY
	clearflag FLAG_HIDE_ROUTE_106_NPCS
	end

@ Handles: VAR_TEMP_5 overlay, VAR_TEMP_6 wave. Manaphy is local id 6.
Route106_ShowManaphyArena:
	setflag FLAG_HIDE_ROUTE_106_NPCS

	setmetatilewithelevation 22, 7, 0x1AD, FALSE, 1
	setmetatilewithelevation 23, 7, 0x1AE, FALSE, 1
	setmetatilewithelevation 24, 7, 0x1AE, FALSE, 1
	setmetatilewithelevation 25, 7, 0x1AE, FALSE, 1
	setmetatilewithelevation 26, 7, 0x1AE, FALSE, 1
	setmetatilewithelevation 27, 7, 0x1AE, FALSE, 1
	setmetatilewithelevation 28, 7, 0x1AF, FALSE, 1

	setmetatilewithelevation 22, 8, 0x1B5, FALSE, 1
	setmetatilewithelevation 23, 8, 0x19E, FALSE, 3
	setmetatilewithelevation 24, 8, 0x19E, FALSE, 3
	setmetatilewithelevation 25, 8, 0x19E, FALSE, 3
	setmetatilewithelevation 26, 8, 0x19E, FALSE, 3
	setmetatilewithelevation 27, 8, 0x19E, FALSE, 3
	setmetatilewithelevation 28, 8, 0x1B7, FALSE, 1
	
	setmetatilewithelevation 22, 9, 0x1B5, FALSE, 1
	setmetatilewithelevation 23, 9, 0x19E, FALSE, 3
	setmetatilewithelevation 24, 9, 0x19E, FALSE, 3
	setmetatilewithelevation 25, 9, 0x19E, FALSE, 3
	setmetatilewithelevation 26, 9, 0x19E, FALSE, 3
	setmetatilewithelevation 27, 9, 0x19E, FALSE, 3
	setmetatilewithelevation 28, 9, 0x1B7, FALSE, 1
	
	setmetatilewithelevation 22, 10, 0x1B5, FALSE, 1
	setmetatilewithelevation 23, 10, 0x19E, FALSE, 3
	setmetatilewithelevation 24, 10, 0x19E, FALSE, 3
	setmetatilewithelevation 25, 10, 0x19E, FALSE, 3
	setmetatilewithelevation 26, 10, 0x19E, FALSE, 3
	setmetatilewithelevation 27, 10, 0x19E, FALSE, 3
	setmetatilewithelevation 28, 10, 0x1B7, FALSE, 1
	
	setmetatilewithelevation 22, 11, 0x1BD, FALSE, 1
	setmetatilewithelevation 23, 11, 0x1BE, FALSE, 1
	setmetatilewithelevation 24, 11, 0x1BE, FALSE, 1
	setmetatilewithelevation 25, 11, 0x1BE, FALSE, 1
	setmetatilewithelevation 26, 11, 0x1BE, FALSE, 1
	setmetatilewithelevation 27, 11, 0x1BE, FALSE, 1
	setmetatilewithelevation 28, 11, 0x1BF, FALSE, 1

	end

Route106_EventScript_TrainerTipsSign::
	msgbox Route106_Text_TrainerTips, MSGBOX_SIGN
	end

Route106_EventScript_Douglas::
	trainerbattle_single TRAINER_DOUGLAS, Route106_Text_DouglasIntro, Route106_Text_DouglasDefeated
	msgbox Route106_Text_DouglasPostBattle, MSGBOX_AUTOCLOSE
	end

Route106_EventScript_Kyla::
	trainerbattle_single TRAINER_KYLA, Route106_Text_KylaIntro, Route106_Text_KylaDefeated
	msgbox Route106_Text_KylaPostBattle, MSGBOX_AUTOCLOSE
	end

Route106_EventScript_Elliot::
	trainerbattle_single TRAINER_ELLIOT_1, Route106_Text_ElliotIntro, Route106_Text_ElliotDefeated
	msgbox Route106_Text_ElliotPostBattle, MSGBOX_DEFAULT
	release
	end

Route106_EventScript_Ned::
	trainerbattle_single TRAINER_NED, Route106_Text_NedIntro, Route106_Text_NedDefeated
	msgbox Route106_Text_NedPostBattle, MSGBOX_AUTOCLOSE
	end

Route106_Text_TrainerTips:
	.string "TRAINER TIPS\p"
	.string "Advice on catching POKéMON with a ROD:\n"
	.string "Press the A Button if you get a bite.$"

