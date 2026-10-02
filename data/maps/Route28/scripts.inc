Route28_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route28_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Route28_OnFrame
	.byte 0

Route28_OnFrame::
	map_script_2 VAR_TEMP_0, 0, Route28_EventScript_CheckAccess
	.2byte 0

@ Route 28 / Mt. Silver open after the Johto League and all eight Kanto badges.
Route28_EventScript_CheckAccess::
	setvar VAR_TEMP_0, 1
	goto_if_unset FLAG_JOHTO_CHAMPION, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE01_GET_FRLG, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE02_GET_FRLG, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE03_GET_FRLG, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE04_GET_FRLG, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE05_GET_FRLG, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE06_GET_FRLG, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE07_GET_FRLG, Route28_EventScript_AccessDenied
	goto_if_unset FLAG_BADGE08_GET_FRLG, Route28_EventScript_AccessDenied
	end

Route28_EventScript_AccessDenied::
	lockall
	msgbox Route28_Text_AccessDenied, MSGBOX_DEFAULT
	closemessage
	warp MAP_RECEPTION_GATE, 3
	waitstate
	end

Route28_Text_AccessDenied:
	.string "A fierce blizzard blocks the way\n"
	.string "ahead.\p"
	.string "Only a trainer who has conquered\n"
	.string "the JOHTO LEAGUE and holds all\l"
	.string "eight KANTO BADGES may pass.$"

Route28_OnTransition: 
	end

Route28_EventScript_Sign::
	msgbox Route28_Text_Sign, MSGBOX_SIGN
	end

Route28_EventScript_Item_Flamethrower::
    finditem ITEM_TM_BLAZE_KICK
    end

Route28_Text_Sign:
	.string "ROUTE 28$"

Route28_EventScript_Trigger_Scientist::
	lock
	clearflag FLAG_HIDE_ROUTE28_SCIENTIST
	addobject LOCALID_ROUTE28_SCIENTIST
	special SpawnCameraObject
	applymovement2 OBJ_EVENT_ID_CAMERA, Route28_Movement_CameraPanDown
	waitmovement OBJ_EVENT_ID_CAMERA
	playse SE_POKENAV_CALL
	waitse
	msgbox Route28_Text_Scientist, MSGBOX_DEFAULT
	playse SE_POKENAV_HANG_UP
	msgbox Route28_Text_Scientist2, MSGBOX_DEFAULT
	closemessage
	applymovement2 LOCALID_ROUTE28_SCIENTIST, Route28_Movement_ScientistLeave
	waitmovement LOCALID_ROUTE28_SCIENTIST
	applymovement2 OBJ_EVENT_ID_CAMERA, Route28_Movement_CameraPanUp
	waitmovement OBJ_EVENT_ID_CAMERA
	special RemoveCameraObject
	setvar VAR_ROUTE28_SCIENTIST, 2
	setflag FLAG_HIDE_ROUTE28_SCIENTIST
	removeobject LOCALID_ROUTE28_SCIENTIST
	release
	end

Route28_Movement_CameraPanDown:
	walk_down
	walk_down
	walk_down
	walk_diag_southwest
	walk_diag_southwest
	step_end

Route28_Movement_CameraPanUp:
	walk_diag_northeast
	walk_diag_northeast
	walk_up
	walk_up
	walk_up
	step_end

Route28_Movement_ScientistLeave:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

Route28_Text_Scientist:
	.string "Hiya, Boss.\p"
	.string "Yes, I've found several artifacts,\n"
	.string "but no sign of the AZURE FLUTE.\p"
	.string "It may be inside the MOUNTAIN, but\n"
	.string "the wild POKéMON here are strong,\l"
	.string "and my team needs some rest...\p"
	.string "..................\p"
	.string "You're breaking up...\l"
	.string "Boss....?\p"
	.string "I'm going to the POKéMON CENTER.\p"
	.string "I'll call back if I find the FLUTE.$"

Route28_Text_Scientist2:
	.string ".......CLICK\p"
	.string "The signal in SINJOH sucks...$"

