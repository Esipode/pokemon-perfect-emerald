GraniteCave_B2F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, GraniteCave_B2F_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, GraniteCave_B2F_OnLoad
	.byte 0

GraniteCave_B2F_OnTransition:
	updatelegendaryvisibility SPECIES_DIANCIE, FLAG_HIDE_DIANCIE
	end

GraniteCave_B2F_OnLoad:
	@ Cobalion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 6
	setvar VAR_0x8009, 70
	setvar VAR_0x800A, 900
	call LegendaryEncounter_EventScript_CobalionQuake
	setvar VAR_0x8004, SPECIES_DIANCIE
	setvar VAR_0x8005, LAYOUT_GRANITE_CAVE_B2F
	setvar VAR_0x8006, WEEKDAY_TUE
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, GraniteCave_B2F_HideDiancieArena
	goto GraniteCave_B2F_ShowDiancieArena

GraniteCave_B2F_Diancie_Encounter::
	goto LegendaryEncounter_EventScript_Diancie

GraniteCave_B2F_HideDiancieArena:
	setflag FLAG_HIDE_DIANCIE
	end

GraniteCave_B2F_ShowDiancieArena:
	@ Light pink tint and wave, both stronger with proximity
	overlaycreate RGB(31, 22, 26), 14, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlayanchorobject VAR_TEMP_5, LOCALID_GRANITE_CAVE_B2F_DIANCIE
	overlayfalloff VAR_TEMP_5, 2, 14, 0, 8
	end
