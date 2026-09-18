NavelRock_Summit_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, NavelRock_Summit_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, NavelRock_Summit_OnResume
	.byte 0

NavelRock_Summit_OnTransition::
	setvar VAR_TEMP_1, 1
	updatelegendaryvisibility SPECIES_HO_OH, FLAG_HIDE_HO_OH
	call_if_eq VAR_RESULT, FALSE, NavelRock_Summit_EventScript_EnableHoOhTrigger
	end

NavelRock_Summit_EventScript_EnableHoOhTrigger::
	setvar VAR_TEMP_1, 0
	return

NavelRock_Summit_OnResume::
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, NavelRock_Summit_EventScript_TryRemoveHoOh
	end

NavelRock_Summit_EventScript_TryRemoveHoOh::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject LOCALID_NAVEL_ROCK_HO_OH
	return

NavelRock_Summit_EventScript_HoOh::
	lockall
	setvar VAR_TEMP_1, 1
	special SpawnCameraObject
	setvar VAR_0x8004, 3  @ Num loops
	setvar VAR_0x8005, 35 @ Delay
	special LoopWingFlapSE
	applymovement LOCALID_NAVEL_ROCK_HO_OH, Movement_HoOhAppear
	applymovement LOCALID_CAMERA, Movement_CameraPanUp
	waitmovement 0
	delay 50
	setweather 0
	doweather
	waitse
	playmoncry SPECIES_HO_OH, CRY_MODE_ENCOUNTER
	delay 30
	waitmoncry
	delay 60
	setvar VAR_0x8004, 3  @ Num loops
	setvar VAR_0x8005, 35 @ Delay
	special LoopWingFlapSE
	applymovement LOCALID_CAMERA, Movement_CameraPanDown
	applymovement LOCALID_NAVEL_ROCK_HO_OH, Movement_HoOhApproach
	waitmovement 0
	special RemoveCameraObject
	seteventmon SPECIES_HO_OH, 70
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	setvar VAR_LAST_TALKED, LOCALID_NAVEL_ROCK_HO_OH
	specialvar VAR_RESULT, GetBattleOutcome
	call_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, NavelRock_Summit_EventScript_CaughtHoOh
	releaseall
	end

@ Only catching Ho-Oh removes it for good; any other outcome leaves it here to be rechallenged.
NavelRock_Summit_EventScript_CaughtHoOh::
	setflag FLAG_FOUGHT_HO_OH
	return

Movement_CameraPanUp:
	walk_up
	walk_up
	walk_up
	step_end

Movement_CameraPanDown:
	delay_16
	delay_16
	walk_down
	walk_down
	walk_down
	step_end

Movement_HoOhApproach:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_in_place_down
	delay_16
	delay_16
	step_end

Movement_HoOhAppear:
	delay_16
	delay_16
	walk_in_place_down
	walk_in_place_down
	walk_in_place_down
	walk_in_place_down
	step_end
