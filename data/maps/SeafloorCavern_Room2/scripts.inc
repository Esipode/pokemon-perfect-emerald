SeafloorCavern_Room2_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SeafloorCavern_Room2_OnLoad
	.byte 0

SeafloorCavern_Room2_OnLoad:
	@ Volcanion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 5
	setvar VAR_0x8009, 65
	setvar VAR_0x800A, 1050
	call LegendaryEncounter_EventScript_VolcanionQuake
	end
