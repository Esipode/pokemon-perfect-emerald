CeruleanCave_B1F_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, CeruleanCave_B1F_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, CeruleanCave_B1F_OnTransition
	.byte 0

CeruleanCave_B1F_OnTransition::
	updatelegendaryvisibility SPECIES_MEWTWO, FLAG_HIDE_MEWTWO
	end

CeruleanCave_B1F_OnLoad:
	setvar VAR_0x8004, SPECIES_MEWTWO
	setvar VAR_0x8005, LAYOUT_CERULEAN_CAVE_B1F
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, CeruleanCave_B1F_HideMewtwoArena
	end

CeruleanCave_B1F_Mewtwo_Encounter::
	goto LegendaryEncounter_EventScript_Mewtwo

CeruleanCave_B1F_HideMewtwoArena:
	setflag FLAG_HIDE_MEWTWO
	end
