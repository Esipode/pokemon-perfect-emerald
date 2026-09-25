Underwater_Route126_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Underwater_Route126_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Underwater_Route126_OnLoad
	.byte 0

Underwater_Route126_OnTransition:
	updatelegendaryvisibility SPECIES_TAPU_FINI, FLAG_HIDE_TAPU_FINI
	end

Underwater_Route126_OnLoad:
	setvar VAR_0x8004, SPECIES_TAPU_FINI
	setvar VAR_0x8005, LAYOUT_UNDERWATER_ROUTE126
	setvar VAR_0x8006, WEEKDAY_SUN
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Underwater_Route126_HideTapuFiniArena
	goto Underwater_Route126_ShowTapuFiniArena

Underwater_Route126_TapuFini_Encounter::
	screenfxripple VAR_TEMP_7, 16, 0, 60
	delay 60
	goto LegendaryEncounter_EventScript_TapuFini

Underwater_Route126_HideTapuFiniArena:
	setflag FLAG_HIDE_TAPU_FINI
	end

Underwater_Route126_ShowTapuFiniArena:
	overlaycreate RGB(31, 16, 24), 12, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlayanchorpos VAR_TEMP_5, 27, 2
	overlayfalloff VAR_TEMP_5, 2, 14, 0, 12
	screenfxwave VAR_TEMP_8, 8, 75, 0, 0
	screenfxanchorpos VAR_TEMP_8, 27, 2
	screenfxfalloff VAR_TEMP_8, 2, 14, 0, 16
	end
