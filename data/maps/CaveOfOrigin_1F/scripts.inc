CaveOfOrigin_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, CaveOfOrigin_1F_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, CaveOfOrigin_1F_OnLoad
	.byte 0

CaveOfOrigin_1F_OnTransition:
	call_if_set FLAG_UNUSED_RS_LEGENDARY_BATTLE_DONE, CaveOfOrigin_EventScript_DisableTriggers
	updatelegendaryvisibility SPECIES_DIALGA, FLAG_HIDE_DIALGA
	end

CaveOfOrigin_1F_OnLoad:
	@ Dialga spawn gate: post-game, not already owned, Tuesday
	setvar VAR_0x8004, SPECIES_DIALGA
	setvar VAR_0x8005, LAYOUT_CAVE_OF_ORIGIN_1F
	setvar VAR_0x8006, WEEKDAY_TUE
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, CaveOfOrigin_1F_HideDialgaArena
	goto CaveOfOrigin_1F_ShowDialgaArena

CaveOfOrigin_1F_Dialga_Encounter::
	goto LegendaryEncounter_EventScript_Dialga

CaveOfOrigin_1F_HideDialgaArena:
	setflag FLAG_HIDE_DIALGA
	end

CaveOfOrigin_1F_ShowDialgaArena:
	overlaycreate RGB(8, 14, 28), 5, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	@ Violent quake in bursts: 50 frames decaying to rest, then 300 frames of calm
	screenfxshake VAR_TEMP_8, 16, 24, SCREENFX_AXIS_X | SCREENFX_AXIS_Y, 0
	screenfxpulse VAR_TEMP_8, 50, 300

	setmetatilewithelevation 14, 5, 0x31B, FALSE, 3

	setmetatilewithelevation 14, 7, 0x31B, FALSE, 3
	setmetatilewithelevation 15, 7, 0x31B, FALSE, 3
	setmetatilewithelevation 16, 7, 0x31B, FALSE, 3
	setmetatilewithelevation 17, 7, 0x320, TRUE, 0

	setmetatilewithelevation 10, 8, 0x31B, FALSE, 3
	setmetatilewithelevation 11, 8, 0x31B, FALSE, 3
	setmetatilewithelevation 12, 8, 0x31B, FALSE, 3
	setmetatilewithelevation 13, 8, 0x31B, FALSE, 3
	setmetatilewithelevation 14, 8, 0x31B, FALSE, 3
	setmetatilewithelevation 15, 8, 0x31B, FALSE, 3
	setmetatilewithelevation 16, 8, 0x31B, FALSE, 3
	setmetatilewithelevation 17, 8, 0x320, TRUE, 0

	setmetatilewithelevation 9, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 10, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 11, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 12, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 13, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 14, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 15, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 16, 9, 0x31B, FALSE, 3
	setmetatilewithelevation 17, 9, 0x320, TRUE, 0

	setmetatilewithelevation 8, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 9, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 10, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 11, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 12, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 13, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 14, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 15, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 16, 10, 0x31B, FALSE, 3
	setmetatilewithelevation 17, 10, 0x320, TRUE, 0

	setmetatilewithelevation 8, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 9, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 10, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 11, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 12, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 13, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 14, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 15, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 16, 11, 0x31B, FALSE, 3
	setmetatilewithelevation 17, 11, 0x328, TRUE, 0
	setmetatilewithelevation 18, 11, 0x31D, TRUE, 0

	setmetatilewithelevation 8, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 9, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 10, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 11, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 12, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 13, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 14, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 15, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 16, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 17, 12, 0x31B, FALSE, 3
	setmetatilewithelevation 18, 12, 0x320, TRUE, 0

	setmetatilewithelevation 8, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 9, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 10, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 11, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 12, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 13, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 14, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 15, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 16, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 17, 13, 0x31B, FALSE, 3
	setmetatilewithelevation 18, 13, 0x320, TRUE, 0

	setmetatilewithelevation 10, 14, 0x31B, FALSE, 3
	setmetatilewithelevation 11, 14, 0x31B, FALSE, 3
	setmetatilewithelevation 12, 14, 0x31B, FALSE, 3
	setmetatilewithelevation 13, 14, 0x31B, FALSE, 3
	setmetatilewithelevation 14, 14, 0x31B, FALSE, 3

	end
