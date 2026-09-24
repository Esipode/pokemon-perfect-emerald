SeafloorCavern_Room8_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SeafloorCavern_Room8_OnLoad
	.byte 0

SeafloorCavern_Room8_OnLoad:
	@ Volcanion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 14
	setvar VAR_0x8009, 95
	setvar VAR_0x800A, 250
	call LegendaryEncounter_EventScript_VolcanionQuake
	end
