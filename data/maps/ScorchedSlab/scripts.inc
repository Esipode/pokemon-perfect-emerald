ScorchedSlab_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ScorchedSlab_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ScorchedSlab_OnLoad
	.byte 0

ScorchedSlab_OnTransition:
	setflag FLAG_LANDMARK_SCORCHED_SLAB
	updatelegendaryvisibility SPECIES_UXIE, FLAG_HIDE_UXIE
	updatelegendaryvisibility SPECIES_MESPRIT, FLAG_HIDE_MESPRIT
	updatelegendaryvisibility SPECIES_AZELF, FLAG_HIDE_AZELF
	end

ScorchedSlab_OnLoad:
	@ Lake trio spawn gate: post-game, Monday only, not already owned. One member spawns per day,
	@ picked uniformly from the daily seed among the unowned. The pick is kept in FLAG_DAILY_LAKE_TRIO_*.
	goto_if_set FLAG_DAILY_LAKE_TRIO_UXIE, ScorchedSlab_ShowTrioPick
	goto_if_set FLAG_DAILY_LAKE_TRIO_MESPRIT, ScorchedSlab_ShowTrioPick
	goto_if_set FLAG_DAILY_LAKE_TRIO_AZELF, ScorchedSlab_ShowTrioPick
	setvar VAR_0x8008, 0
	setvar VAR_0x8004, SPECIES_UXIE
	call ScorchedSlab_EventScript_RollTrioMember
	call_if_eq VAR_RESULT, TRUE, ScorchedSlab_EventScript_PickUxie
	setvar VAR_0x8004, SPECIES_MESPRIT
	call ScorchedSlab_EventScript_RollTrioMember
	call_if_eq VAR_RESULT, TRUE, ScorchedSlab_EventScript_PickMesprit
	setvar VAR_0x8004, SPECIES_AZELF
	call ScorchedSlab_EventScript_RollTrioMember
	call_if_eq VAR_RESULT, TRUE, ScorchedSlab_EventScript_PickAzelf
ScorchedSlab_ShowTrioPick:
	call_if_unset FLAG_DAILY_LAKE_TRIO_UXIE, ScorchedSlab_HideUxie
	call_if_unset FLAG_DAILY_LAKE_TRIO_MESPRIT, ScorchedSlab_HideMesprit
	call_if_unset FLAG_DAILY_LAKE_TRIO_AZELF, ScorchedSlab_HideAzelf
	end

@ VAR_0x8004 = species, VAR_0x8008 = eligible members so far. Result TRUE = this member is the current pick.
@ Reservoir sampling: the nth eligible member takes over the pick with 1/n, so each ends up equally likely.
ScorchedSlab_EventScript_RollTrioMember:
	copyvar VAR_0x800A, VAR_0x8004
	setvar VAR_0x8005, LAYOUT_SCORCHED_SLAB
	setvar VAR_0x8006, WEEKDAY_MON
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, TRUE, ScorchedSlab_EventScript_RollEligibleTrioMember
	return

ScorchedSlab_EventScript_RollEligibleTrioMember:
	addvar VAR_0x8008, 1
	setvar VAR_0x8009, 100
	call_if_eq VAR_0x8008, 2, ScorchedSlab_EventScript_SetRoll50
	call_if_eq VAR_0x8008, 3, ScorchedSlab_EventScript_SetRoll33
	checkdailyseedchance VAR_0x8009, VAR_0x800A
	return

ScorchedSlab_EventScript_SetRoll50:
	setvar VAR_0x8009, 50
	return

ScorchedSlab_EventScript_SetRoll33:
	setvar VAR_0x8009, 33
	return

ScorchedSlab_EventScript_PickUxie:
	setflag FLAG_DAILY_LAKE_TRIO_UXIE
	clearflag FLAG_DAILY_LAKE_TRIO_MESPRIT
	clearflag FLAG_DAILY_LAKE_TRIO_AZELF
	return

ScorchedSlab_EventScript_PickMesprit:
	clearflag FLAG_DAILY_LAKE_TRIO_UXIE
	setflag FLAG_DAILY_LAKE_TRIO_MESPRIT
	clearflag FLAG_DAILY_LAKE_TRIO_AZELF
	return

ScorchedSlab_EventScript_PickAzelf:
	clearflag FLAG_DAILY_LAKE_TRIO_UXIE
	clearflag FLAG_DAILY_LAKE_TRIO_MESPRIT
	setflag FLAG_DAILY_LAKE_TRIO_AZELF
	return

ScorchedSlab_Uxie_Encounter::
	goto LegendaryEncounter_EventScript_Uxie

ScorchedSlab_Mesprit_Encounter::
	goto LegendaryEncounter_EventScript_Mesprit

ScorchedSlab_Azelf_Encounter::
	goto LegendaryEncounter_EventScript_Azelf

ScorchedSlab_HideUxie:
	setflag FLAG_HIDE_UXIE
	return

ScorchedSlab_HideMesprit:
	setflag FLAG_HIDE_MESPRIT
	return

ScorchedSlab_HideAzelf:
	setflag FLAG_HIDE_AZELF
	return
