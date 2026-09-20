Route126_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route126_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route126_OnLoad
	.byte 0

Route126_OnTransition:
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	updatelegendaryvisibility SPECIES_ARCEUS, FLAG_HIDE_ARCEUS
	@ Connection steps keep screen effects but clear their handle vars; stop them so ON_LOAD does not stack copies
	screenfxstopall
	call Route126_EventScript_CheckArceusActive
	call_if_eq VAR_RESULT, TRUE, Route126_EventScript_ArceusActive
	call_if_eq VAR_RESULT, FALSE, Route126_EventScript_ArceusInactive
	end

@ Arceus spawn gate: post-game, not already owned, 2% daily-seed chance. Result in VAR_RESULT
Route126_EventScript_CheckArceusActive:
	setvar VAR_0x8004, SPECIES_ARCEUS
	setvar VAR_0x8005, LAYOUT_ROUTE126
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route126_EventScript_CheckArceusActiveDone
	checkdailyseedchance 2, SPECIES_ARCEUS
Route126_EventScript_CheckArceusActiveDone:
	return

Route126_EventScript_ArceusActive:
	setweather WEATHER_RAIN_THUNDERSTORM
	setflag FLAG_HIDE_ROUTE_126_NPCS
	return

Route126_EventScript_ArceusInactive:
	clearflag FLAG_HIDE_ROUTE_126_NPCS
	return

Route126_OnLoad:
	call Route126_EventScript_CheckArceusActive
	goto_if_eq VAR_RESULT, FALSE, Route126_HideArceusArena
	goto Route126_ShowArceusArena

Route126_Arceus_Encounter::
	screenfxstopall
	overlaydestroy VAR_TEMP_5
	overlaydestroy VAR_TEMP_6
	goto LegendaryEncounter_EventScript_Arceus

Route126_HideArceusArena:
	setflag FLAG_HIDE_ARCEUS
	end

Route126_ShowArceusArena:
	@ White and black flashes: two pulses on different periods so they interleave; peak opacity scales with proximity
	overlaycreate RGB(31, 31, 31), 16, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlaypulse VAR_TEMP_5, 0, 16, 48
	overlayanchorobject VAR_TEMP_5, LOCALID_ROUTE126_ARCEUS
	overlayfalloff VAR_TEMP_5, 2, 16, 0, 12
	overlaycreate RGB(0, 0, 0), 16, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_6
	overlaypulse VAR_TEMP_6, 0, 16, 72
	overlayanchorobject VAR_TEMP_6, LOCALID_ROUTE126_ARCEUS
	overlayfalloff VAR_TEMP_6, 2, 16, 0, 8
	@ Earthquake shake and distortion, both stronger with proximity
	screenfxshake VAR_TEMP_8, 16, 24, SCREENFX_AXIS_X | SCREENFX_AXIS_Y, 0
	screenfxanchorobject VAR_TEMP_8, LOCALID_ROUTE126_ARCEUS
	screenfxfalloff VAR_TEMP_8, 2, 16, 0, 10
	screenfxwave VAR_TEMP_9, 16, 75, 0, 0, 1
	screenfxanchorobject VAR_TEMP_9, LOCALID_ROUTE126_ARCEUS
	screenfxfalloff VAR_TEMP_9, 2, 16, 0, 8

	setmetatilewithelevation 38, 40, 0x341, FALSE, 3
	setmetatilewithelevation 39, 40, 0x341, FALSE, 3
	setmetatilewithelevation 40, 40, 0x341, FALSE, 3
	setmetatilewithelevation 41, 40, 0x341, FALSE, 3
	setmetatilewithelevation 42, 40, 0x341, FALSE, 3
	setmetatilewithelevation 43, 40, 0x341, FALSE, 3
	setmetatilewithelevation 44, 40, 0x341, FALSE, 3
	setmetatilewithelevation 45, 40, 0x341, FALSE, 3
	setmetatilewithelevation 46, 40, 0x341, FALSE, 3
	setmetatilewithelevation 47, 40, 0x341, FALSE, 3
	
	setmetatilewithelevation 38, 41, 0x341, FALSE, 3
	setmetatilewithelevation 39, 41, 0x341, FALSE, 3
	setmetatilewithelevation 40, 41, 0x341, FALSE, 3
	setmetatilewithelevation 41, 41, 0x341, FALSE, 3
	setmetatilewithelevation 42, 41, 0x341, FALSE, 3
	setmetatilewithelevation 43, 41, 0x341, FALSE, 3
	setmetatilewithelevation 44, 41, 0x341, FALSE, 3
	setmetatilewithelevation 45, 41, 0x341, FALSE, 3
	setmetatilewithelevation 46, 41, 0x341, FALSE, 3
	setmetatilewithelevation 47, 41, 0x341, FALSE, 3
	
	setmetatilewithelevation 38, 42, 0x341, FALSE, 3
	setmetatilewithelevation 39, 42, 0x341, FALSE, 3
	setmetatilewithelevation 40, 42, 0x341, FALSE, 3
	setmetatilewithelevation 41, 42, 0x341, FALSE, 3
	setmetatilewithelevation 42, 42, 0x341, FALSE, 3
	setmetatilewithelevation 43, 42, 0x341, FALSE, 3
	setmetatilewithelevation 44, 42, 0x341, FALSE, 3
	setmetatilewithelevation 45, 42, 0x341, FALSE, 3
	setmetatilewithelevation 46, 42, 0x341, FALSE, 3
	setmetatilewithelevation 47, 42, 0x341, FALSE, 3
	
	setmetatilewithelevation 38, 43, 0x341, FALSE, 3
	setmetatilewithelevation 39, 43, 0x341, FALSE, 3
	setmetatilewithelevation 40, 43, 0x341, FALSE, 3
	setmetatilewithelevation 41, 43, 0x341, FALSE, 3
	setmetatilewithelevation 42, 43, 0x341, FALSE, 3
	setmetatilewithelevation 43, 43, 0x341, FALSE, 3
	setmetatilewithelevation 44, 43, 0x341, FALSE, 3
	setmetatilewithelevation 45, 43, 0x341, FALSE, 3
	setmetatilewithelevation 46, 43, 0x341, FALSE, 3
	setmetatilewithelevation 47, 43, 0x341, FALSE, 3

	setmetatilewithelevation 38, 44, 0x341, FALSE, 3
	setmetatilewithelevation 39, 44, 0x341, FALSE, 3
	setmetatilewithelevation 40, 44, 0x341, FALSE, 3
	setmetatilewithelevation 41, 44, 0x341, FALSE, 3
	setmetatilewithelevation 42, 44, 0x341, FALSE, 3
	setmetatilewithelevation 43, 44, 0x341, FALSE, 3
	setmetatilewithelevation 44, 44, 0x341, FALSE, 3
	setmetatilewithelevation 45, 44, 0x341, FALSE, 3
	setmetatilewithelevation 46, 44, 0x341, FALSE, 3
	setmetatilewithelevation 47, 44, 0x341, FALSE, 3

	setmetatilewithelevation 38, 45, 0x341, FALSE, 3
	setmetatilewithelevation 39, 45, 0x341, FALSE, 3
	setmetatilewithelevation 40, 45, 0x341, FALSE, 3
	setmetatilewithelevation 41, 45, 0x341, FALSE, 3
	setmetatilewithelevation 42, 45, 0x341, FALSE, 3
	setmetatilewithelevation 43, 45, 0x341, FALSE, 3
	setmetatilewithelevation 44, 45, 0x341, FALSE, 3
	setmetatilewithelevation 45, 45, 0x341, FALSE, 3
	setmetatilewithelevation 46, 45, 0x341, FALSE, 3
	setmetatilewithelevation 47, 45, 0x341, FALSE, 3

	setmetatilewithelevation 38, 46, 0x341, FALSE, 3
	setmetatilewithelevation 39, 46, 0x341, FALSE, 3
	setmetatilewithelevation 40, 46, 0x341, FALSE, 3
	setmetatilewithelevation 41, 46, 0x341, FALSE, 3
	setmetatilewithelevation 42, 46, 0x341, FALSE, 3
	setmetatilewithelevation 43, 46, 0x341, FALSE, 3
	setmetatilewithelevation 44, 46, 0x341, FALSE, 3
	setmetatilewithelevation 45, 46, 0x341, FALSE, 3
	setmetatilewithelevation 46, 46, 0x341, FALSE, 3
	setmetatilewithelevation 47, 46, 0x341, FALSE, 3

	setmetatilewithelevation 38, 47, 0x341, FALSE, 3
	setmetatilewithelevation 39, 47, 0x341, FALSE, 3
	setmetatilewithelevation 40, 47, 0x341, FALSE, 3
	setmetatilewithelevation 41, 47, 0x341, FALSE, 3
	setmetatilewithelevation 42, 47, 0x341, FALSE, 3
	setmetatilewithelevation 43, 47, 0x341, FALSE, 3
	setmetatilewithelevation 44, 47, 0x341, FALSE, 3
	setmetatilewithelevation 45, 47, 0x341, FALSE, 3
	setmetatilewithelevation 46, 47, 0x341, FALSE, 3
	setmetatilewithelevation 47, 47, 0x341, FALSE, 3

	setmetatilewithelevation 38, 48, 0x341, FALSE, 3
	setmetatilewithelevation 39, 48, 0x341, FALSE, 3
	setmetatilewithelevation 40, 48, 0x341, FALSE, 3
	setmetatilewithelevation 41, 48, 0x341, FALSE, 3
	setmetatilewithelevation 42, 48, 0x341, FALSE, 3
	setmetatilewithelevation 43, 48, 0x341, FALSE, 3
	setmetatilewithelevation 44, 48, 0x341, FALSE, 3
	setmetatilewithelevation 45, 48, 0x341, FALSE, 3
	setmetatilewithelevation 46, 48, 0x341, FALSE, 3
	setmetatilewithelevation 47, 48, 0x341, FALSE, 3

	setmetatilewithelevation 38, 49, 0x341, FALSE, 3
	setmetatilewithelevation 39, 49, 0x341, FALSE, 3
	setmetatilewithelevation 40, 49, 0x341, FALSE, 3
	setmetatilewithelevation 41, 49, 0x341, FALSE, 3
	setmetatilewithelevation 42, 49, 0x341, FALSE, 3
	setmetatilewithelevation 43, 49, 0x341, FALSE, 3
	setmetatilewithelevation 44, 49, 0x341, FALSE, 3
	setmetatilewithelevation 45, 49, 0x341, FALSE, 3
	setmetatilewithelevation 46, 49, 0x341, FALSE, 3
	setmetatilewithelevation 47, 49, 0x341, FALSE, 3

	setmetatilewithelevation 42, 53, 0x0AF, FALSE, 3
	setmetatilewithelevation 43, 53, 0x0AF, FALSE, 3
	setmetatilewithelevation 44, 53, 0x0AF, FALSE, 3

	setmetatilewithelevation 42, 55, 0x0AF, FALSE, 3
	setmetatilewithelevation 43, 55, 0x0AF, FALSE, 3
	setmetatilewithelevation 44, 55, 0x0AF, FALSE, 3

	setmetatilewithelevation 42, 57, 0x0AF, FALSE, 3
	setmetatilewithelevation 43, 57, 0x0AF, FALSE, 3
	setmetatilewithelevation 44, 57, 0x0AF, FALSE, 3

	setmetatilewithelevation 42, 59, 0x0AF, FALSE, 3
	setmetatilewithelevation 43, 59, 0x0AF, FALSE, 3
	setmetatilewithelevation 44, 59, 0x0AF, FALSE, 3

	setmetatilewithelevation 42, 60, 0x0AF, FALSE, 3
	setmetatilewithelevation 43, 60, 0x0AF, FALSE, 3
	setmetatilewithelevation 44, 60, 0x0AF, FALSE, 3

	end

Route126_EventScript_Barry::
	trainerbattle_single TRAINER_BARRY, Route126_Text_BarryIntro, Route126_Text_BarryDefeat
	msgbox Route126_Text_BarryPostBattle, MSGBOX_AUTOCLOSE
	end

Route126_EventScript_Dean::
	trainerbattle_single TRAINER_DEAN, Route126_Text_DeanIntro, Route126_Text_DeanDefeat
	msgbox Route126_Text_DeanPostBattle, MSGBOX_AUTOCLOSE
	end

Route126_EventScript_Nikki::
	trainerbattle_single TRAINER_NIKKI, Route126_Text_NikkiIntro, Route126_Text_NikkiDefeat
	msgbox Route126_Text_NikkiPostBattle, MSGBOX_AUTOCLOSE
	end

Route126_EventScript_Brenda::
	trainerbattle_single TRAINER_BRENDA, Route126_Text_BrendaIntro, Route126_Text_BrendaDefeat
	msgbox Route126_Text_BrendaPostBattle, MSGBOX_AUTOCLOSE
	end

Route126_EventScript_Leonardo::
	trainerbattle_single TRAINER_LEONARDO, Route126_Text_LeonardoIntro, Route126_Text_LeonardoDefeat
	msgbox Route126_Text_LeonardoPostBattle, MSGBOX_AUTOCLOSE
	end

Route126_EventScript_Isobel::
	trainerbattle_single TRAINER_ISOBEL, Route126_Text_IsobelIntro, Route126_Text_IsobelDefeat
	msgbox Route126_Text_IsobelPostBattle, MSGBOX_AUTOCLOSE
	end

Route126_EventScript_Sienna::
	trainerbattle_single TRAINER_SIENNA, Route126_Text_SiennaIntro, Route126_Text_SiennaDefeat
	msgbox Route126_Text_SiennaPostBattle, MSGBOX_AUTOCLOSE
	end

Route126_EventScript_Pablo::
	trainerbattle_single TRAINER_PABLO_1, Route126_Text_PabloIntro, Route126_Text_PabloDefeat
	msgbox Route126_Text_PabloPostBattle, MSGBOX_DEFAULT
	release
	end

