Route129_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route129_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route129_OnLoad
	map_script MAP_SCRIPT_ON_RESUME, Route129_OnResume
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Route129_OnFrame
	.byte 0

Route129_OnLoad:
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_129_WEST, AbnormalWeather_EventScript_PlaceTilesRoute129West
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_129_EAST, AbnormalWeather_EventScript_PlaceTilesRoute129East
	@ Necrozma spawn gate: post-game, not already owned, Tuesday, night
	setvar VAR_0x8004, SPECIES_NECROZMA
	setvar VAR_0x8005, LAYOUT_ROUTE129
	setvar VAR_0x8006, WEEKDAY_TUE
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route129_HideNecrozmaArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_NIGHT, Route129_HideNecrozmaArena
	goto Route129_ShowNecrozmaArena

Route129_Necrozma_Encounter::
	goto LegendaryEncounter_EventScript_Necrozma

Route129_HideNecrozmaArena:
	setflag FLAG_HIDE_NECROZMA
	end

Route129_ShowNecrozmaArena:
	setflag FLAG_TEMP_NO_WILD_ENCOUNTERS
	end

Route129_OnResume:
	goto_if_set FLAG_HIDE_NECROZMA, Route129_OnResumeEnd
	removeobject 1
	removeobject 2
	removeobject 3
	removeobject 4
	removeobject 5
	overlaydestroymaplocal
	overlaycreate RGB(0, 0, 0), 16, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlayanchorpos VAR_TEMP_5, 24, 19
	overlayfalloff VAR_TEMP_5, 14, 18, 0, 16
	overlayexemptobject VAR_TEMP_5, LOCALID_ROUTE129_NECROZMA
Route129_OnResumeEnd:
	end

Route129_OnTransition:
	updatelegendaryvisibility SPECIES_NECROZMA, FLAG_HIDE_NECROZMA
	call_if_eq VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_HideMapNamePopup
	call_if_ge VAR_SOOTOPOLIS_CITY_STATE, 4, Route129_EventScript_CheckSetAbnormalWeather
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_129_WEST, AbnormalWeather_StartKyogreWeather
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_129_EAST, AbnormalWeather_StartKyogreWeather
	end

Route129_EventScript_CheckSetAbnormalWeather::
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	return

Route129_OnFrame:
	map_script_2 VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_EndEventAndCleanup_1
	.2byte 0

Route129_EventScript_Chase::
	trainerbattle_single TRAINER_CHASE, Route129_Text_ChaseIntro, Route129_Text_ChaseDefeat
	msgbox Route129_Text_ChasePostBattle, MSGBOX_AUTOCLOSE
	end

Route129_EventScript_Allison::
	trainerbattle_single TRAINER_ALLISON, Route129_Text_AllisonIntro, Route129_Text_AllisonDefeat
	msgbox Route129_Text_AllisonPostBattle, MSGBOX_AUTOCLOSE
	end

Route129_EventScript_Reed::
	trainerbattle_single TRAINER_REED, Route129_Text_ReedIntro, Route129_Text_ReedDefeat
	msgbox Route129_Text_ReedPostBattle, MSGBOX_AUTOCLOSE
	end

Route129_EventScript_Tisha::
	trainerbattle_single TRAINER_TISHA, Route129_Text_TishaIntro, Route129_Text_TishaDefeat
	msgbox Route129_Text_TishaPostBattle, MSGBOX_AUTOCLOSE
	end

Route129_EventScript_Clarence::
	trainerbattle_single TRAINER_CLARENCE, Route129_Text_ClarenceIntro, Route129_Text_ClarenceDefeat
	msgbox Route129_Text_ClarencePostBattle, MSGBOX_AUTOCLOSE
	end

