SafariZone_North_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SafariZone_North_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, SafariZone_North_OnLoad
	.byte 0

SafariZone_North_OnTransition:
	updatelegendaryvisibility SPECIES_VIRIZION, FLAG_HIDE_VIRIZION
	end

SafariZone_North_OnLoad:
	@ Virizion spawn gate: post-game, not already owned, Fridays, morning
	setvar VAR_0x8004, SPECIES_VIRIZION
	setvar VAR_0x8005, LAYOUT_SAFARI_ZONE_NORTH
	setvar VAR_0x8006, WEEKDAY_FRI
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, SafariZone_North_HideVirizionArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_MORNING, SafariZone_North_HideVirizionArena
	end

SafariZone_North_Virizion_Encounter::
	goto LegendaryEncounter_EventScript_Virizion

SafariZone_North_HideVirizionArena:
	setflag FLAG_HIDE_VIRIZION
	end

SafariZone_North_EventScript_Fisherman::
	msgbox SafariZone_North_Text_Fisherman, MSGBOX_NPC
	end

SafariZone_North_EventScript_Man::
	msgbox SafariZone_North_Text_Man, MSGBOX_NPC
	end

