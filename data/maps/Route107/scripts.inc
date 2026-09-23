Route107_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route107_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route107_OnLoad
	.byte 0

Route107_OnTransition:
	updatelegendaryvisibility SPECIES_THUNDURUS, FLAG_HIDE_THUNDURUS
	end

Route107_OnLoad:
	@ Thundurus spawn gate: post-game, not already owned, thunderstorm weather
	setvar VAR_0x8004, SPECIES_THUNDURUS
	setvar VAR_0x8005, LAYOUT_ROUTE107
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route107_HideThundurusArena
	getmapweather VAR_0x8008
	goto_if_ne VAR_0x8008, WEATHER_RAIN_THUNDERSTORM, Route107_HideThundurusArena
	goto Route107_ShowThundurusArena

Route107_Thundurus_Encounter::
	goto LegendaryEncounter_EventScript_Thundurus

Route107_HideThundurusArena:
	setflag FLAG_HIDE_THUNDURUS
	end

Route107_ShowThundurusArena:
	@ Hides all route NPCs while Thundurus is present
	setflag FLAG_TEMP_7
	@ TODO(user): arena metatiles. One setmetatilewithelevation per tile:
	@   setmetatilewithelevation <x>, <y>, <metatileId>, <impassable>, <elevation>
	end

Route107_EventScript_Darrin::
	trainerbattle_single TRAINER_DARRIN, Route107_Text_DarrinIntro, Route107_Text_DarrinDefeated
	msgbox Route107_Text_DarrinPostBattle, MSGBOX_AUTOCLOSE
	end

Route107_EventScript_Tony::
	trainerbattle_single TRAINER_TONY_1, Route107_Text_TonyIntro, Route107_Text_TonyDefeated
	msgbox Route107_Text_TonyPostBattle, MSGBOX_DEFAULT
	release
	end

Route107_EventScript_Denise::
	trainerbattle_single TRAINER_DENISE, Route107_Text_DeniseIntro, Route107_Text_DeniseDefeated
	msgbox Route107_Text_DenisePostBattle, MSGBOX_AUTOCLOSE
	end

Route107_EventScript_Beth::
	trainerbattle_single TRAINER_BETH, Route107_Text_BethIntro, Route107_Text_BethDefeated
	msgbox Route107_Text_BethPostBattle, MSGBOX_AUTOCLOSE
	end

Route107_EventScript_Lisa::
	trainerbattle_double TRAINER_LISA_AND_RAY, Route107_Text_LisaIntro, Route107_Text_LisaDefeated, Route107_Text_LisaNotEnoughPokemon
	msgbox Route107_Text_LisaPostBattle, MSGBOX_AUTOCLOSE
	end

Route107_EventScript_Ray::
	trainerbattle_double TRAINER_LISA_AND_RAY, Route107_Text_RayIntro, Route107_Text_RayDefeated, Route107_Text_RayNotEnoughPokemon
	msgbox Route107_Text_RayPostBattle, MSGBOX_AUTOCLOSE
	end

Route107_EventScript_Camron::
	trainerbattle_single TRAINER_CAMRON, Route107_Text_CamronIntro, Route107_Text_CamronDefeated
	msgbox Route107_Text_CamronPostBattle, MSGBOX_AUTOCLOSE
	end

