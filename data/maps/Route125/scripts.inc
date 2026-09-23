Route125_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route125_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route125_OnLoad
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Route125_OnFrame
	.byte 0

Route125_OnTransition:
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	call_if_eq VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_HideMapNamePopup
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_125_WEST, AbnormalWeather_StartKyogreWeather
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_125_EAST, AbnormalWeather_StartKyogreWeather
	updatelegendaryvisibility SPECIES_KYUREM, FLAG_HIDE_KYUREM
	end

Route125_OnLoad:
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_125_WEST, AbnormalWeather_EventScript_PlaceTilesRoute125West
	call_if_eq VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_ROUTE_125_EAST, AbnormalWeather_EventScript_PlaceTilesRoute125East
	@ Kyurem spawn gate: post-game, not already owned, Sunday, snow
	setvar VAR_0x8004, SPECIES_KYUREM
	setvar VAR_0x8005, LAYOUT_ROUTE125
	setvar VAR_0x8006, WEEKDAY_SUN
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route125_HideKyuremArena
	getmapweather VAR_0x8008
	goto_if_ne VAR_0x8008, WEATHER_SNOW, Route125_HideKyuremArena
	goto Route125_ShowKyuremArena

Route125_Kyurem_Encounter::
	goto LegendaryEncounter_EventScript_Kyurem

Route125_HideKyuremArena:
	setflag FLAG_HIDE_KYUREM
	clearflag FLAG_HIDE_ROUTE_125_NPCS
	end

Route125_ShowKyuremArena:
	setflag FLAG_HIDE_ROUTE_125_NPCS
	@ TODO(user): arena metatiles. One setmetatilewithelevation per tile:
	@   setmetatilewithelevation <x>, <y>, <metatileId>, <impassable>, <elevation>
	end

Route125_OnFrame:
	map_script_2 VAR_SHOULD_END_ABNORMAL_WEATHER, 1, AbnormalWeather_EventScript_EndEventAndCleanup_1
	.2byte 0

Route125_EventScript_Nolen::
	trainerbattle_single TRAINER_NOLEN, Route125_Text_NolenIntro, Route125_Text_NolenDefeat
	msgbox Route125_Text_NolenPostBattle, MSGBOX_AUTOCLOSE
	end

Route125_EventScript_Stan::
	trainerbattle_single TRAINER_STAN, Route125_Text_StanIntro, Route125_Text_StanDefeat
	msgbox Route125_Text_StanPostBattle, MSGBOX_AUTOCLOSE
	end

Route125_EventScript_Tanya::
	trainerbattle_single TRAINER_TANYA, Route125_Text_TanyaIntro, Route125_Text_TanyaDefeat
	msgbox Route125_Text_TanyaPostBattle, MSGBOX_AUTOCLOSE
	end

Route125_EventScript_Sharon::
	trainerbattle_single TRAINER_SHARON, Route125_Text_SharonIntro, Route125_Text_SharonDefeat
	msgbox Route125_Text_SharonPostBattle, MSGBOX_AUTOCLOSE
	end

Route125_EventScript_Ernest::
	trainerbattle_single TRAINER_ERNEST_1, Route125_Text_ErnestIntro, Route125_Text_ErnestDefeat
	msgbox Route125_Text_ErnestPostBattle, MSGBOX_DEFAULT
	release
	end

Route125_EventScript_Kim::
	trainerbattle_double TRAINER_KIM_AND_IRIS, Route125_Text_KimIntro, Route125_Text_KimDefeat, Route125_Text_KimNotEnoughMons
	msgbox Route125_Text_KimPostBattle, MSGBOX_AUTOCLOSE
	end

Route125_EventScript_Iris::
	trainerbattle_double TRAINER_KIM_AND_IRIS, Route125_Text_IrisIntro, Route125_Text_IrisDefeat, Route125_Text_IrisNotEnoughMons
	msgbox Route125_Text_IrisPostBattle, MSGBOX_AUTOCLOSE
	end

Route125_EventScript_Presley::
	trainerbattle_single TRAINER_PRESLEY, Route125_Text_PresleyIntro, Route125_Text_PresleyDefeat
	msgbox Route125_Text_PresleyPostBattle, MSGBOX_AUTOCLOSE
	end

Route125_EventScript_Auron::
	trainerbattle_single TRAINER_AURON, Route125_Text_AuronIntro, Route125_Text_AuronDefeat
	msgbox Route125_Text_AuronPostBattle, MSGBOX_AUTOCLOSE
	end

