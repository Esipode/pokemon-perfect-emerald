NavelRock_Base_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, NavelRock_Base_Frlg_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, NavelRock_Base_Frlg_OnResume
	.byte 0

NavelRock_Base_Frlg_OnTransition::
	updatelegendaryvisibility SPECIES_LUGIA, FLAG_HIDE_LUGIA
	end

NavelRock_Base_Frlg_OnResume::
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, NavelRock_Base_Frlg_EventScript_TryRemoveLugia
	end

NavelRock_Base_Frlg_EventScript_TryRemoveLugia::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject LOCALID_NAVEL_ROCK_LUGIA
	return

NavelRock_Base_Frlg_EventScript_Lugia::
	lock
	faceplayer
	waitse
	delay 20
	playse SE_THUNDERSTORM_STOP
	setvar VAR_0x8004, 0  @ Vertical pan
	setvar VAR_0x8005, 3  @ Horizontal pan
	setvar VAR_0x8006, 4  @ Num shakes
	setvar VAR_0x8007, 2  @ Shake delay
	special ShakeScreen
	delay 30
	playse SE_THUNDERSTORM_STOP
	setvar VAR_0x8004, 0  @ Vertical pan
	setvar VAR_0x8005, 3  @ Horizontal pan
	setvar VAR_0x8006, 4  @ Num shakes
	setvar VAR_0x8007, 2  @ Shake delay
	special ShakeScreen
	delay 30
	delay 50
	waitse
	playmoncry SPECIES_LUGIA, CRY_MODE_ENCOUNTER
	waitmoncry
	delay 20
	seteventmon SPECIES_LUGIA, 70
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	call_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, NavelRock_Base_Frlg_EventScript_CaughtLugia
	release
	end

@ Only catching Lugia removes it for good; any other outcome leaves it here to be rechallenged.
NavelRock_Base_Frlg_EventScript_CaughtLugia::
	setflag FLAG_FOUGHT_LUGIA
	return
