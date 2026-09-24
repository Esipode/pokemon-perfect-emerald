SeafloorCavern_Room5_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SeafloorCavern_Room5_OnLoad
	.byte 0

SeafloorCavern_Room5_OnLoad:
	@ Volcanion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 9
	setvar VAR_0x8009, 80
	setvar VAR_0x800A, 600
	call LegendaryEncounter_EventScript_VolcanionQuake
	end
