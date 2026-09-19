MarineCave_End_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, MarineCave_End_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, MarineCave_End_OnTransition
	.byte 0

MarineCave_End_OnResume:
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, MarineCave_End_EventScript_TryRemoveKyogre
	end

MarineCave_End_EventScript_TryRemoveKyogre::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject LOCALID_MARINE_CAVE_KYOGRE
	setvar VAR_SHOULD_END_ABNORMAL_WEATHER, 1
	setflag FLAG_DEFEATED_KYOGRE
	return

MarineCave_End_OnTransition:
	updatelegendaryvisibility SPECIES_KYOGRE, FLAG_HIDE_MARINE_CAVE_KYOGRE
	call_if_eq VAR_RESULT, FALSE, MarineCave_End_EventScript_EnableKyogreTrigger
	end

MarineCave_End_EventScript_EnableKyogreTrigger::
	setvar VAR_TEMP_1, 1
	return

MarineCave_End_EventScript_Kyogre::
	lockall
	applymovement LOCALID_PLAYER, Common_Movement_FaceUp
	waitmovement 0
	applymovement LOCALID_MARINE_CAVE_KYOGRE, MarineCave_End_Movement_KyogreApproach
	waitmovement 0
	waitse
	playmoncry SPECIES_KYOGRE, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setvar VAR_LAST_TALKED, LOCALID_MARINE_CAVE_KYOGRE
	setvar VAR_TEMP_1, 0
	goto LegendaryEncounter_EventScript_Kyogre

MarineCave_End_Movement_KyogreApproach:
	init_affine_anim
	walk_down_start_affine
	delay_16
	delay_16
	walk_down_affine
	delay_16
	delay_16
	walk_down_affine
	step_end

