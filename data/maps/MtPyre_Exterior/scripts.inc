MtPyre_Exterior_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtPyre_Exterior_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, MtPyre_Exterior_OnLoad
	.byte 0

MtPyre_Exterior_OnTransition:
	updatelegendaryvisibility SPECIES_SPECTRIER, FLAG_HIDE_SPECTRIER
	call MtPyre_Exterior_EventScript_CheckEnterFromSummit
	end

MtPyre_Exterior_OnLoad:
	setvar VAR_0x8004, SPECIES_SPECTRIER
	setvar VAR_0x8005, LAYOUT_MT_PYRE_EXTERIOR
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, MtPyre_Exterior_HideSpectrierArena
	checkspeciesowned SPECIES_CALYREX
	goto_if_eq VAR_RESULT, FALSE, MtPyre_Exterior_HideSpectrierArena
	goto MtPyre_Exterior_ShowSpectrierArena

MtPyre_Exterior_Spectrier_Encounter::
	screenfxripple VAR_TEMP_8, 16, 0, 60
	delay 60
	goto LegendaryEncounter_EventScript_Spectrier

MtPyre_Exterior_HideSpectrierArena:
	setflag FLAG_HIDE_SPECTRIER
	end

MtPyre_Exterior_ShowSpectrierArena:
	overlaycreate RGB(18, 4, 26), 6, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlaypulse VAR_TEMP_5, 3, 8, 180, 0, OVERLAY_PULSE_SINE
	overlayanchorobject VAR_TEMP_5, LOCALID_MT_PYRE_EXTERIOR_SPECTRIER
	overlayfalloff VAR_TEMP_5, 0, 16, 4, 12
	screenfxwave VAR_TEMP_6, 10, 90, 0, 0
	screenfxanchorobject VAR_TEMP_6, LOCALID_MT_PYRE_EXTERIOR_SPECTRIER
	screenfxfalloff VAR_TEMP_6, 0, 16, 6, 16
	end

MtPyre_Exterior_EventScript_CheckEnterFromSummit::
	getplayerxy VAR_TEMP_0, VAR_TEMP_1
	goto_if_lt VAR_TEMP_1, 12, MtPyre_Exterior_EventScript_EnterFromSummit
	return

MtPyre_Exterior_EventScript_EnterFromSummit::
	goto_if_unset FLAG_HIDE_SPECTRIER, MtPyre_Exterior_EventScript_WeatherLocked
	setweather WEATHER_FOG_HORIZONTAL
	return

MtPyre_Exterior_EventScript_FogTrigger::
	goto_if_unset FLAG_HIDE_SPECTRIER, MtPyre_Exterior_EventScript_WeatherLocked
	setweather WEATHER_FOG_HORIZONTAL
	doweather
	end

MtPyre_Exterior_EventScript_SunTrigger::
	goto_if_unset FLAG_HIDE_SPECTRIER, MtPyre_Exterior_EventScript_WeatherLocked
	setweather WEATHER_SUNNY
	doweather
	end

MtPyre_Exterior_EventScript_WeatherLocked:
	return

