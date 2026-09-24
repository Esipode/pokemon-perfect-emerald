NavelRock_Entrance_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, NavelRock_Entrance_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, NavelRock_Entrance_OnLoad
	.byte 0

NavelRock_Entrance_OnTransition:
	updatelegendaryvisibility SPECIES_YVELTAL, FLAG_HIDE_YVELTAL
	end

NavelRock_Entrance_OnLoad:
	@ Yveltal spawn gate: post-game, not already owned, Sunday, night
	setvar VAR_0x8004, SPECIES_YVELTAL
	setvar VAR_0x8005, LAYOUT_NAVEL_ROCK_ENTRANCE
	setvar VAR_0x8006, WEEKDAY_SUN
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, NavelRock_Entrance_HideYveltalArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_NIGHT, NavelRock_Entrance_HideYveltalArena
	goto NavelRock_Entrance_ShowYveltalArena

NavelRock_Entrance_Yveltal_Encounter::
	goto LegendaryEncounter_EventScript_Yveltal

NavelRock_Entrance_HideYveltalArena:
	setflag FLAG_HIDE_YVELTAL
	end

NavelRock_Entrance_ShowYveltalArena:
	overlaycreate RGB(31, 0, 0), 0, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlaypulse VAR_TEMP_5, 0, 6, 240, 0
	overlaycreate RGB(0, 0, 0), 0, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_6
	overlaypulse VAR_TEMP_6, 0, 6, 240, 120
	screenfxwave VAR_TEMP_7, 8, 90, 0, 0
	screenfxanchorpos VAR_TEMP_7, 10, 5
	screenfxfalloff VAR_TEMP_7, 2, 10, 1, 8
	setmetatilewithelevation 10, 5, 0x23F, FALSE, 3
	end
