SeafloorCavern_Room7_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SeafloorCavern_Room7_OnLoad
	.byte 0

SeafloorCavern_Room7_OnLoad:
	@ Volcanion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 12
	setvar VAR_0x8009, 90
	setvar VAR_0x800A, 360
	call LegendaryEncounter_EventScript_VolcanionQuake
	end
