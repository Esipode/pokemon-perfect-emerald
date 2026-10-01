MtEmber_Summit_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtEmber_Summit_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, MtEmber_Summit_OnLoad
	.byte 0

MtEmber_Summit_OnTransition::
	updatelegendaryvisibility SPECIES_MOLTRES, FLAG_HIDE_MOLTRES
	end

MtEmber_Summit_OnLoad:
	setvar VAR_0x8004, SPECIES_MOLTRES
	setvar VAR_0x8005, LAYOUT_MT_EMBER_SUMMIT
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, MtEmber_Summit_HideMoltresArena
	end

MtEmber_Summit_Moltres_Encounter::
	goto LegendaryEncounter_EventScript_Moltres

MtEmber_Summit_HideMoltresArena:
	setflag FLAG_HIDE_MOLTRES
	end
