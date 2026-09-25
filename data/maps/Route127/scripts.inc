Route127_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route127_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route127_OnLoad
	map_script MAP_SCRIPT_ON_RESUME, Route127_OnResume
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Route127_OnFrame
	.byte 0

Route127_OnTransition:
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	call_if_eq VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_HideMapNamePopup
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_127_NORTH, AbnormalWeather_StartKyogreWeather
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_127_SOUTH, AbnormalWeather_StartKyogreWeather
	updatelegendaryvisibility SPECIES_TAPU_LELE, FLAG_HIDE_TAPU_LELE
	end

Route127_OnLoad:
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_127_NORTH, AbnormalWeather_EventScript_PlaceTilesRoute127North
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_127_SOUTH, AbnormalWeather_EventScript_PlaceTilesRoute127South
	@ Tapu Lele spawn gate: post-game, not already owned, Saturday only
	setvar VAR_0x8004, SPECIES_TAPU_LELE
	setvar VAR_0x8005, LAYOUT_ROUTE127
	setvar VAR_0x8006, WEEKDAY_SAT
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route127_HideTapuLeleArena
	goto Route127_ShowTapuLeleArena

Route127_TapuLele_Encounter::
	screenfxripple VAR_TEMP_7, 16, 0, 60
	delay 60
	goto LegendaryEncounter_EventScript_TapuLele

Route127_HideTapuLeleArena:
	setflag FLAG_HIDE_TAPU_LELE
	end

Route127_ShowTapuLeleArena:
	screenfxwave VAR_TEMP_8, 8, 150, 0, 0
	screenfxanchorobject VAR_TEMP_8, LOCALID_ROUTE127_TAPU_LELE
	screenfxfalloff VAR_TEMP_8, 2, 14, 0, 10
	screenfxwave VAR_TEMP_9, 6, 70, 24, 0, 1
	screenfxanchorobject VAR_TEMP_9, LOCALID_ROUTE127_TAPU_LELE
	screenfxfalloff VAR_TEMP_9, 2, 14, 0, 12
	end

Route127_OnResume:
	goto_if_set FLAG_HIDE_TAPU_LELE, Route127_OnResumeDone
	goto_if_ne VAR_TEMP_5, 0, Route127_OnResumeDone
	overlaycreate RGB(31, 20, 25), 8, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlayanchorobject VAR_TEMP_5, LOCALID_ROUTE127_TAPU_LELE
	overlayfalloff VAR_TEMP_5, 3, 14, 0, 8
Route127_OnResumeDone:
	end

Route127_OnFrame:
	map_script_2 VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_EndEventAndCleanup_1
	.2byte 0

Route127_EventScript_Camden::
	trainerbattle_single TRAINER_CAMDEN, Route127_Text_CamdenIntro, Route127_Text_CamdenDefeat
	msgbox Route127_Text_CamdenPostBattle, MSGBOX_AUTOCLOSE
	end

Route127_EventScript_Donny::
	trainerbattle_single TRAINER_DONNY, Route127_Text_DonnyIntro, Route127_Text_DonnyDefeat
	msgbox Route127_Text_DonnyPostBattle, MSGBOX_AUTOCLOSE
	end

Route127_EventScript_Jonah::
	trainerbattle_single TRAINER_JONAH, Route127_Text_JonahIntro, Route127_Text_JonahDefeat
	msgbox Route127_Text_JonahPostBattle, MSGBOX_AUTOCLOSE
	end

Route127_EventScript_Henry::
	trainerbattle_single TRAINER_HENRY, Route127_Text_HenryIntro, Route127_Text_HenryDefeat
	msgbox Route127_Text_HenryPostBattle, MSGBOX_AUTOCLOSE
	end

Route127_EventScript_Roger::
	trainerbattle_single TRAINER_ROGER, Route127_Text_RogerIntro, Route127_Text_RogerDefeat
	msgbox Route127_Text_RogerPostBattle, MSGBOX_AUTOCLOSE
	end

Route127_EventScript_Aidan::
	trainerbattle_single TRAINER_AIDAN, Route127_Text_AidanIntro, Route127_Text_AidanDefeat
	msgbox Route127_Text_AidanPostBattle, MSGBOX_AUTOCLOSE
	end

Route127_EventScript_Athena::
	trainerbattle_single TRAINER_ATHENA, Route127_Text_AthenaIntro, Route127_Text_AthenaDefeat
	msgbox Route127_Text_AthenaPostBattle, MSGBOX_AUTOCLOSE
	end

Route127_EventScript_Koji::
	trainerbattle_single TRAINER_KOJI_1, Route127_Text_KojiIntro, Route127_Text_KojiDefeat
	msgbox Route127_Text_KojiPostBattle, MSGBOX_DEFAULT
	release
	end

