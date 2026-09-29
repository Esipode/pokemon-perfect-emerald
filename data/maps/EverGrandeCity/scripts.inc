EverGrandeCity_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, EverGrandeCity_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, EverGrandeCity_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, EverGrandeCity_OnLoad
	.byte 0

EverGrandeCity_OnResume:
	goto_if_unset FLAG_HIDE_ETERNATUS, EverGrandeCity_OnResumeDone
	call_if_set FLAG_SYS_GAME_CLEAR, InfinityCave_EventScript_OpenEntrance
EverGrandeCity_OnResumeDone:
	end

EverGrandeCity_OnTransition:
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	updatelegendaryvisibility SPECIES_ETERNATUS, FLAG_HIDE_ETERNATUS
	screenfxstopall
	call EverGrandeCity_EventScript_CheckEternatusActive
	call_if_eq VAR_RESULT, TRUE, EverGrandeCity_EventScript_EternatusActive
	end

EverGrandeCity_EventScript_CheckEternatusActive:
	setvar VAR_0x8004, SPECIES_ETERNATUS
	setvar VAR_0x8005, LAYOUT_EVER_GRANDE_CITY
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_EventScript_CheckEternatusActiveDone
	special CountCaughtLegendaryFamilies
	goto_if_lt VAR_RESULT, 70, EverGrandeCity_EventScript_EternatusInactive
	checkdailyseedchance 2, SPECIES_ETERNATUS
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_EventScript_CheckEternatusActiveDone
	@ Arceus takes priority when both roll on the same day
	call Route126_EventScript_CheckArceusActive
	goto_if_eq VAR_RESULT, TRUE, EverGrandeCity_EventScript_EternatusInactive
	setvar VAR_RESULT, TRUE
EverGrandeCity_EventScript_CheckEternatusActiveDone:
	return

EverGrandeCity_EventScript_EternatusInactive:
	setvar VAR_RESULT, FALSE
	return

EverGrandeCity_EventScript_EternatusActive:
	setmaplayoutindex LAYOUT_EVER_GRANDE_CITY_ETERNATUS
	setweather WEATHER_RAIN_THUNDERSTORM
	return

EverGrandeCity_OnLoad:
	call EverGrandeCity_EventScript_CheckEternatusActive
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_HideEternatusArena
	goto EverGrandeCity_ShowEternatusArena

EverGrandeCity_Eternatus_Encounter::
	goto LegendaryEncounter_EventScript_Eternatus

EverGrandeCity_HideEternatusArena:
	setflag FLAG_HIDE_ETERNATUS
	end

EverGrandeCity_ShowEternatusArena:
	@ Blood-red sky tint
	overlaycreate RGB(18, 0, 1), 8, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	@ Lightning: heartbeat wave gives a double flash then a long dark rest
	overlaycreate RGB(31, 31, 31), 16, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_6
	overlaypulse VAR_TEMP_6, 0, 16, 420, 0, OVERLAY_PULSE_HEARTBEAT
	@ Constant tremor: strong near Eternatus, never below the minimum at any distance
	screenfxshake VAR_TEMP_8, 16, 24, SCREENFX_AXIS_X | SCREENFX_AXIS_Y, 0
	screenfxanchorobject VAR_TEMP_8, LOCALID_EVER_GRANDE_CITY_ETERNATUS
	screenfxfalloff VAR_TEMP_8, 2, 20, 6, 16
	@ Pulsing distortion waves, stronger with proximity
	screenfxwave VAR_TEMP_9, 16, 60, 0, 0, 1
	screenfxanchorobject VAR_TEMP_9, LOCALID_EVER_GRANDE_CITY_ETERNATUS
	screenfxfalloff VAR_TEMP_9, 2, 20, 2, 16
	@ Light pillar descending onto Eternatus
	screenfxpillar VAR_TEMP_7, 14, SCREENFX_COLOR_DEFAULT, 90, 0
	screenfxanchorobject VAR_TEMP_7, LOCALID_EVER_GRANDE_CITY_ETERNATUS
	end

EverGrandeCity_EventScript_VictoryRoadSign::
	msgbox EverGrandeCity_Text_EnteringVictoryRoad, MSGBOX_SIGN
	end

EverGrandeCity_EventScript_CitySign::
	msgbox EverGrandeCity_Text_CitySign, MSGBOX_SIGN
	end

EverGrandeCity_EventScript_PokemonLeagueSign::
	msgbox EverGrandeCity_Text_EnteringPokemonLeague, MSGBOX_SIGN
	end

EverGrandeCity_EventScript_SetVisitedEverGrande::
	setflag FLAG_VISITED_EVER_GRANDE_CITY
	setvar VAR_TEMP_1, 1
	end

EverGrandeCity_Text_EnteringVictoryRoad:
	.string "ENTERING VICTORY ROAD$"

EverGrandeCity_Text_EnteringPokemonLeague:
	.string "ENTERING POKéMON LEAGUE\n"
	.string "CENTER GATE$"

EverGrandeCity_Text_CitySign:
	.string "EVER GRANDE CITY\p"
	.string "“The paradise of flowers, the sea,\n"
	.string "and POKéMON.”$"

