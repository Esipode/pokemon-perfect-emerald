GraniteCave_B1F_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, CaveHole_CheckFallDownHole
	map_script MAP_SCRIPT_ON_TRANSITION, CaveHole_FixCrackedGround
	map_script MAP_SCRIPT_ON_RESUME, GraniteCave_B1F_SetHoleWarp
	map_script MAP_SCRIPT_ON_LOAD, GraniteCave_B1F_OnLoad
	.byte 0

GraniteCave_B1F_OnLoad:
	@ Cobalion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 6
	setvar VAR_0x8009, 70
	setvar VAR_0x800A, 900
	call LegendaryEncounter_EventScript_CobalionQuake
	end

GraniteCave_B1F_SetHoleWarp:
	setstepcallback STEP_CB_CRACKED_FLOOR
	setholewarp MAP_GRANITE_CAVE_B2F
	end

