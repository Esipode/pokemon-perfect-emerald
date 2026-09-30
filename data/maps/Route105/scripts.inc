Route105_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, Route105_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, Route105_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Route105_OnFrame
	.byte 0

Route105_OnLoad:
	call_if_unset FLAG_REGI_DOORS_OPENED, Route105_CloseRegiEntrance
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_105_NORTH, AbnormalWeather_EventScript_PlaceTilesRoute105North
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_105_SOUTH, AbnormalWeather_EventScript_PlaceTilesRoute105South
	setvar VAR_0x8004, SPECIES_CHIEN_PAO
	setvar VAR_0x8005, LAYOUT_ROUTE105
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	@ special CheckValidLegendaryEncounter
	@ goto_if_eq VAR_RESULT, FALSE, Route105_HideChienPaoArena
	@ checkspeciesowned SPECIES_REGICE
	@ goto_if_eq VAR_RESULT, FALSE, Route105_HideChienPaoArena
	@ getmapweather VAR_0x8008
	@ goto_if_ne VAR_0x8008, WEATHER_SNOW, Route105_HideChienPaoArena
	goto Route105_ShowChienPaoArena

Route105_ChienPao_Encounter::
	goto LegendaryEncounter_EventScript_ChienPao

Route105_HideChienPaoArena:
	setflag FLAG_HIDE_CHIEN_PAO
	end

Route105_ShowChienPaoArena:
	setmetatilewithelevation 5, 27, 0x1AD, FALSE, 1
	setmetatilewithelevation 6, 27, 0x1AE, FALSE, 1
	setmetatilewithelevation 7, 27, 0x1AE, FALSE, 1
	setmetatilewithelevation 8, 27, 0x1AE, FALSE, 1
	setmetatilewithelevation 9, 27, 0x1AE, FALSE, 1
	setmetatilewithelevation 10, 27, 0x1AE, FALSE, 1
	setmetatilewithelevation 11, 27, 0x1AF, FALSE, 1

	setmetatilewithelevation 5, 28, 0x1B5, FALSE, 1
	setmetatilewithelevation 6, 28, 0x19E, FALSE, 3
	setmetatilewithelevation 7, 28, 0x19E, FALSE, 3
	setmetatilewithelevation 8, 28, 0x19E, FALSE, 3
	setmetatilewithelevation 9, 28, 0x19E, FALSE, 3
	setmetatilewithelevation 10, 28, 0x19E, FALSE, 3
	setmetatilewithelevation 11, 28, 0x1B7, FALSE, 1

	setmetatilewithelevation 5, 29, 0x1B5, FALSE, 1
	setmetatilewithelevation 6, 29, 0x19E, FALSE, 3
	setmetatilewithelevation 7, 29, 0x19E, FALSE, 3
	setmetatilewithelevation 8, 29, 0x19E, FALSE, 3
	setmetatilewithelevation 9, 29, 0x19E, FALSE, 3
	setmetatilewithelevation 10, 29, 0x19E, FALSE, 3
	setmetatilewithelevation 11, 29, 0x1B7, FALSE, 1

	setmetatilewithelevation 5, 30, 0x1B5, FALSE, 1
	setmetatilewithelevation 6, 30, 0x19E, FALSE, 3
	setmetatilewithelevation 7, 30, 0x19E, FALSE, 3
	setmetatilewithelevation 8, 30, 0x19E, FALSE, 3
	setmetatilewithelevation 9, 30, 0x19E, FALSE, 3
	setmetatilewithelevation 10, 30, 0x19E, FALSE, 3
	setmetatilewithelevation 11, 30, 0x1B7, FALSE, 1

	setmetatilewithelevation 5, 31, 0x1BD, FALSE, 1
	setmetatilewithelevation 6, 31, 0x1BE, FALSE, 1
	setmetatilewithelevation 7, 31, 0x1BE, FALSE, 1
	setmetatilewithelevation 8, 31, 0x1BE, FALSE, 1
	setmetatilewithelevation 9, 31, 0x1BE, FALSE, 1
	setmetatilewithelevation 10, 31, 0x1BE, FALSE, 1
	setmetatilewithelevation 11, 31, 0x1BF, FALSE, 1

	end

Route105_CloseRegiEntrance::
	setmetatile 9, 19, METATILE_General_RockWall_RockBase, TRUE
	setmetatile 9, 20, METATILE_General_RockWall_SandBase, TRUE
	return

Route105_OnTransition:
	updatelegendaryvisibility SPECIES_CHIEN_PAO, FLAG_HIDE_CHIEN_PAO
	call_if_eq VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_HideMapNamePopup
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_105_NORTH, AbnormalWeather_StartKyogreWeather
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_105_SOUTH, AbnormalWeather_StartKyogreWeather
	end

Route105_OnFrame:
	map_script_2 VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_EndEventAndCleanup_1
	.2byte 0

Route105_EventScript_Foster::
	trainerbattle_single TRAINER_FOSTER, Route105_Text_FosterIntro, Route105_Text_FosterDefeated
	msgbox Route105_Text_FosterPostBattle, MSGBOX_AUTOCLOSE
	end

Route105_EventScript_Luis::
	trainerbattle_single TRAINER_LUIS, Route105_Text_LuisIntro, Route105_Text_LuisDefeated
	msgbox Route105_Text_LuisPostBattle, MSGBOX_AUTOCLOSE
	end

Route105_EventScript_Dominik::
	trainerbattle_single TRAINER_DOMINIK, Route105_Text_DominikIntro, Route105_Text_DominikDefeated
	msgbox Route105_Text_DominikPostBattle, MSGBOX_AUTOCLOSE
	end

Route105_EventScript_Beverly::
	trainerbattle_single TRAINER_BEVERLY, Route105_Text_BeverlyIntro, Route105_Text_BeverlyDefeated
	msgbox Route105_Text_PostBattle, MSGBOX_AUTOCLOSE
	end

Route105_EventScript_Imani::
	trainerbattle_single TRAINER_IMANI, Route105_Text_ImaniIntro, Route105_Text_ImaniDefeated
	msgbox Route105_Text_ImaniPostBattle, MSGBOX_AUTOCLOSE
	end

Route105_EventScript_Josue::
	trainerbattle_single TRAINER_JOSUE, Route105_Text_JosueIntro, Route105_Text_JosueDefeated
	msgbox Route105_Text_JosuePostBattle, MSGBOX_AUTOCLOSE
	end

Route105_EventScript_Andres::
	trainerbattle_single TRAINER_ANDRES_1, Route105_Text_AndresIntro, Route105_Text_AndresDefeated
	msgbox Route105_Text_AndresPostBattle, MSGBOX_DEFAULT
	release
	end

Route104_Text_DadPhoneCall:
	.string "… … … … … …\n"
	.string "… … … … … Beep!\p"
	.string "DAD: Oh, {PLAYER}?\p"
	.string "… … … … … …\n"
	.string "Where are you now?\l"
	.string "It sounds windy wherever you are.\p"
	.string "MOM gave me your number, so I\n"
	.string "decided to give you a call.\p"
	.string "It sounds like you're doing fine,\n"
	.string "so that's fine with me.\p"
	.string "You take care now.\p"
	.string "… … … … … …\n"
	.string "… … … … … Click!$"

