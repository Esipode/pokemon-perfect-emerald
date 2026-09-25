SafariZone_Northeast_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SafariZone_Northeast_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, SafariZone_Northeast_OnLoad
	.byte 0

SafariZone_Northeast_OnTransition:
	updatelegendaryvisibility SPECIES_TAPU_BULU, FLAG_HIDE_TAPU_BULU
	end

SafariZone_Northeast_OnLoad:
	setvar VAR_0x8004, SPECIES_TAPU_BULU
	setvar VAR_0x8005, LAYOUT_SAFARI_ZONE_NORTHEAST
	setvar VAR_0x8006, WEEKDAY_THU
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, SafariZone_Northeast_HideTapuBuluArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_DAY, SafariZone_Northeast_HideTapuBuluArena
	goto SafariZone_Northeast_ShowTapuBuluArena

SafariZone_Northeast_TapuBulu_Encounter::
	goto LegendaryEncounter_EventScript_TapuBulu

SafariZone_Northeast_HideTapuBuluArena:
	setflag FLAG_HIDE_TAPU_BULU
	end

SafariZone_Northeast_ShowTapuBuluArena:
	screenfxshake VAR_TEMP_8, 6, 40, SCREENFX_AXIS_X | SCREENFX_AXIS_Y, 0
	screenfxanchorobject VAR_TEMP_8, LOCALID_SAFARI_ZONE_NORTHEAST_TAPU_BULU
	screenfxfalloff VAR_TEMP_8, 2, 14, 3, 16
	end

@ Event scripts for SafariZone_Northeast are in SafariZone_South/scripts.inc
