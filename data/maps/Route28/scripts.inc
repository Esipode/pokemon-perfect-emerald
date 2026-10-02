Route28_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route28_OnTransition
	.byte 0

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

