SeafloorCavern_Room6_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SeafloorCavern_Room6_OnLoad
	.byte 0

SeafloorCavern_Room6_OnLoad:
	@ Volcanion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 11
	setvar VAR_0x8009, 85
	setvar VAR_0x800A, 480
	call LegendaryEncounter_EventScript_VolcanionQuake
	end
