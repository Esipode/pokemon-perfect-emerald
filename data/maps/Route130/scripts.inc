Route130_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route130_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route130_OnLoad
	.byte 0

Route130_OnTransition:
	updatelegendaryvisibility SPECIES_TERAPAGOS, FLAG_HIDE_TERAPAGOS
	call_if_ge VAR_SOOTOPOLIS_CITY_STATE, 4, Route130_EventScript_CheckSetAbnormalWeather
	specialvar VAR_RESULT, IsMirageIslandPresent
	goto_if_eq VAR_RESULT, TRUE, Route130_EventScript_SetMirageIslandLayout
@ Mirage Island isn't currently visible, hide any objects on the island. Only the first flag is actually associated with anything.
	setflag FLAG_TEMP_HIDE_MIRAGE_ISLAND_BERRY_TREE
	setflag FLAG_TEMP_12
	setflag FLAG_TEMP_13
	setflag FLAG_TEMP_14
	setflag FLAG_TEMP_15
	setflag FLAG_TEMP_16
	setflag FLAG_TEMP_17
	setflag FLAG_TEMP_18
	setflag FLAG_TEMP_19
	setflag FLAG_TEMP_1A
	setflag FLAG_TEMP_1B
	setflag FLAG_TEMP_1C
	setflag FLAG_TEMP_1D
	setflag FLAG_TEMP_1E
	setflag FLAG_TEMP_1F
	setmaplayoutindex LAYOUT_ROUTE130
	end

Route130_EventScript_SetMirageIslandLayout::
	setmaplayoutindex LAYOUT_ROUTE130_MIRAGE_ISLAND
	end

Route130_OnLoad:
	setvar VAR_0x8004, SPECIES_TERAPAGOS
	setvar VAR_0x8005, LAYOUT_ROUTE130_MIRAGE_ISLAND
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route130_HideTerapagosArena
	special CountCaughtLegendaryFamilies
	goto_if_lt VAR_RESULT, 70, Route130_HideTerapagosArena
	goto Route130_ShowTerapagosArena

Route130_Terapagos_Encounter::
	goto LegendaryEncounter_EventScript_Terapagos

Route130_HideTerapagosArena:
	setflag FLAG_HIDE_TERAPAGOS
	end

Route130_ShowTerapagosArena:
	@ Hue cycle: 1 hue step (of 256) every 3 frames
	overlaycreatehue 0, 16, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_0
	overlayhuecycle VAR_TEMP_0, 3, 1
	@ TODO(user): arena metatiles. One setmetatilewithelevation per tile:
	@   setmetatilewithelevation <x>, <y>, <metatileId>, <impassable>, <elevation>
	end

Route130_EventScript_CheckSetAbnormalWeather::
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	return

Route130_EventScript_Rodney::
	trainerbattle_single TRAINER_RODNEY, Route130_Text_RodneyIntro, Route130_Text_RodneyDefeat
	msgbox Route130_Text_RodneyPostBattle, MSGBOX_AUTOCLOSE
	end

Route130_EventScript_Katie::
	trainerbattle_single TRAINER_KATIE, Route130_Text_KatieIntro, Route130_Text_KatieDefeat
	msgbox Route130_Text_KatiePostBattle, MSGBOX_AUTOCLOSE
	end

Route130_EventScript_Santiago::
	trainerbattle_single TRAINER_SANTIAGO, Route130_Text_SantiagoIntro, Route130_Text_SantiagoDefeat
	msgbox Route130_Text_SantiagoPostBattle, MSGBOX_AUTOCLOSE
	end

