MagmaHideout_2F_3R_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, MagmaHideout_2F_3R_OnLoad
	.byte 0

MagmaHideout_2F_3R_OnLoad:
	@ Heatran quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 9
	setvar VAR_0x8009, 80
	setvar VAR_0x800A, 600
	call LegendaryEncounter_EventScript_HeatranQuake
	end

