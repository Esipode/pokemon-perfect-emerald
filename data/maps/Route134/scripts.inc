Route134_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route134_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route134_OnLoad
	map_script MAP_SCRIPT_ON_RESUME, Route134_OnResume
	.byte 0

Route134_OnTransition:
	updatelegendaryvisibility SPECIES_CALYREX, FLAG_HIDE_CALYREX
	end

Route134_OnLoad:
	@ Calyrex spawn gate: post-game, not already owned, 30+ legendary/mythical families caught
	setvar VAR_0x8004, SPECIES_CALYREX
	setvar VAR_0x8005, LAYOUT_ROUTE134
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route134_HideCalyrexArena
	special CountCaughtLegendaryFamilies
	goto_if_lt VAR_RESULT, 30, Route134_HideCalyrexArena
	goto Route134_ShowCalyrexArena

Route134_Calyrex_Encounter::
	goto LegendaryEncounter_EventScript_Calyrex

Route134_HideCalyrexArena:
	setflag FLAG_HIDE_CALYREX
	end

Route134_ShowCalyrexArena:
	@ Alternating green/purple tints: same period, phases offset by half
	overlaycreate RGB(8, 31, 12), 0, OVERLAY_LAYER_WORLD, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_0
	overlaypulse VAR_TEMP_0, 4, 8, 240
	overlaycreate RGB(22, 8, 31), 0, OVERLAY_LAYER_WORLD, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_1
	overlaypulse VAR_TEMP_1, 4, 8, 240, 120
	end

Route134_OnResume:
	setdivewarp MAP_UNDERWATER_ROUTE134, 8, 6
	end

Route134_EventScript_Jack::
	trainerbattle_single TRAINER_JACK, Route134_Text_JackIntro, Route134_Text_JackDefeat
	msgbox Route134_Text_JackPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Laurel::
	trainerbattle_single TRAINER_LAUREL, Route134_Text_LaurelIntro, Route134_Text_LaurelDefeat
	msgbox Route134_Text_LaurelPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Alex::
	trainerbattle_single TRAINER_ALEX, Route134_Text_AlexIntro, Route134_Text_AlexDefeat
	msgbox Route134_Text_AlexPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Aaron::
	trainerbattle_single TRAINER_AARON, Route134_Text_AaronIntro, Route134_Text_AaronDefeat
	msgbox Route134_Text_AaronPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Hitoshi::
	trainerbattle_single TRAINER_HITOSHI, Route134_Text_HitoshiIntro, Route134_Text_HitoshiDefeat
	msgbox Route134_Text_HitoshiPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Hudson::
	trainerbattle_single TRAINER_HUDSON, Route134_Text_HudsonIntro, Route134_Text_HudsonDefeat
	msgbox Route134_Text_HudsonPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Reyna::
	trainerbattle_single TRAINER_REYNA, Route134_Text_ReynaIntro, Route134_Text_ReynaDefeat
	msgbox Route134_Text_ReynaPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Marley::
	trainerbattle_single TRAINER_MARLEY, Route134_Text_MarleyIntro, Route134_Text_MarleyDefeat
	msgbox Route134_Text_MarleyPostBattle, MSGBOX_AUTOCLOSE
	end

Route134_EventScript_Kelvin::
	trainerbattle_single TRAINER_KELVIN, Route134_Text_KelvinIntro, Route134_Text_KelvinDefeat
	msgbox Route134_Text_KelvinPostBattle, MSGBOX_AUTOCLOSE
	end

