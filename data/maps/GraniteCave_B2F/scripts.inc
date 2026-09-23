GraniteCave_B2F_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, GraniteCave_B2F_OnLoad
	.byte 0

GraniteCave_B2F_OnLoad:
	@ Cobalion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 6
	setvar VAR_0x8009, 70
	setvar VAR_0x800A, 900
	call LegendaryEncounter_EventScript_CobalionQuake
	end

