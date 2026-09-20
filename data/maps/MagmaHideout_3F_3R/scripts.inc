MagmaHideout_3F_3R_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, MagmaHideout_3F_3R_OnLoad
	.byte 0

MagmaHideout_3F_3R_OnLoad:
	@ Heatran quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 12
	setvar VAR_0x8009, 90
	setvar VAR_0x800A, 360
	call LegendaryEncounter_EventScript_HeatranQuake
	end

