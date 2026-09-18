FieryPath_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, FieryPath_OnTransition
	.byte 0

FieryPath_OnTransition:
	call_if_unset FLAG_LANDMARK_FIERY_PATH, FieryPath_EventScript_MoveScottToFallarbor
	updatelegendaryvisibility SPECIES_ENTEI, FLAG_HIDE_ENTEI
	setflag FLAG_LANDMARK_FIERY_PATH
	// Check if legendary is available
    setvar VAR_0x8004, SPECIES_ENTEI
    setvar VAR_0x8005, LAYOUT_FIERY_PATH
    setvar VAR_0x8006, 4 @ Friday
    setvar VAR_0x8007, 1
    special CheckValidLegendaryEncounter
	goto_if_ne VAR_RESULT, TRUE, FieryPath_HideEntei
	end

FieryPath_Entei_Encounter::
	goto LegendaryEncounter_EventScript_Entei

FieryPath_HideEntei:
	setflag FLAG_HIDE_ENTEI
	return

FieryPath_EventScript_MoveScottToFallarbor::
	setflag FLAG_HIDE_VERDANTURF_TOWN_SCOTT
	clearflag FLAG_HIDE_FALLARBOR_TOWN_BATTLE_TENT_SCOTT
	return

