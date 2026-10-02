SaffronCity_TrainStation_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, SaffronCity_TrainStation_OnFrame
	.byte 0

SaffronCity_TrainStation_OnFrame::
	map_script_2 VAR_TRAIN, 1, SaffronCity_TrainStation_EventScript_ExitTrain
	.2byte 0

SaffronCity_TrainStation_EventScript_Nerd::
	msgbox SaffronCity_TrainStation_Text_GymGuide_ReturnedPart, MSGBOX_NPC
	end
SaffronCity_TrainStation_EventScript_Lass::
	msgbox SaffronCity_TrainStation_Text_Lass, MSGBOX_NPC
	end
SaffronCity_TrainStation_EventScript_Teacher::
	msgbox SaffronCity_TrainStation_Text_Teacher, MSGBOX_NPC
	end

SaffronCity_TrainStation_EventScript_Attendant::
	lock
	faceplayer
	checkitem ITEM_PASS
	goto_if_eq VAR_RESULT, TRUE, SaffronCity_TrainStation_EventScript_BoardTrain
	msgbox SaffronCity_TrainStation_Text_OfficerNoRailPass, MSGBOX_DEFAULT
	closemessage
	release
	end

SaffronCity_TrainStation_EventScript_BoardTrain::
	msgbox SaffronCity_TrainStation_Text_OfficerRightThisWay, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_SAFFRON_CITY_TRAIN_STATION_ATTENDANT, SaffronCity_TrainStation_Movement_Attendant_Move_Up
	waitmovement 0
	applymovement OBJ_EVENT_ID_PLAYER, SaffronCity_TrainStation_Movement_WalktoTrain
	waitmovement 0
	special SpawnCameraObject
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkUp1
	waitmovement 0
	playse SE_EXIT
	waitse
	fadescreen FADE_TO_BLACK

	applymovement OBJ_EVENT_ID_CAMERA, SaffronCity_TrainStation_Movement_CameraPanUp
	waitmovement 0
	applymovement OBJ_EVENT_ID_CAMERA, SaffronCity_TrainStation_Movement_CameraPanRight
	waitmovement 0
	delay 60

	applymovement LOCALID_SAFFRON_CITY_TRAIN_STATION_TRAIN SaffronCity_TrainStation_Movement_SlideLeft
	applymovement OBJ_EVENT_ID_CAMERA, SaffronCity_TrainStation_Movement_SlideLeft
	playbgm MUS_HG_MAGNET_TRAIN, FALSE
	fadescreen FADE_FROM_BLACK
	playse SE_TRUCK_MOVE
	delay 220
	playse SE_TRUCK_STOP
	applymovement LOCALID_SAFFRON_CITY_TRAIN_STATION_TRAIN SaffronCity_TrainStation_Movement_SlideLeft
	applymovement OBJ_EVENT_ID_CAMERA, SaffronCity_TrainStation_Movement_SlideLeft
	fadescreen FADE_TO_BLACK
	waitse
	special RemoveCameraObject
	setvar VAR_TRAIN, 1
	delay 60
	warp MAP_GOLDENROD_CITY_TRAIN_STATION, 19, 16
	waitstate
	delay 60
	end

SaffronCity_TrainStation_EventScript_ExitTrain::
	lock
	applymovement LOCALID_SAFFRON_CITY_TRAIN_STATION_ATTENDANT, SaffronCity_TrainStation_Movement_Attendant_Move_Up
	applymovement OBJ_EVENT_ID_PLAYER, SaffronCity_TrainStation_Movement_ExitTrain
	waitmovement 0
	waitmovement 0
	setvar VAR_TRAIN, 0
	applymovement LOCALID_SAFFRON_CITY_TRAIN_STATION_ATTENDANT, SaffronCity_TrainStation_Movement_Attendant_Move_Down
	waitmovement 0
	release
	end


SaffronCity_TrainStation_Movement_ExitTrain:
	walk_down
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	step_end

SaffronCity_TrainStation_Movement_Attendant_Move_Up:
	face_up
	walk_up
	walk_left
	face_right
	step_end

SaffronCity_TrainStation_Movement_Attendant_Move_Down:
	walk_right
	walk_down
	step_end

SaffronCity_TrainStation_Movement_WalktoTrain:
	walk_up
	walk_up
	walk_up
	walk_right
	walk_right
	walk_right
	walk_up
	walk_up
	step_end

SaffronCity_TrainStation_Movement_CameraPanUp:
	slide_up
	slide_up
	slide_up
	slide_up
	slide_up
	slide_up
	slide_up
	slide_up
	slide_up
	slide_up
	slide_up
	step_end

SaffronCity_TrainStation_Movement_CameraPanRight:
	slide_right
	slide_right
	slide_right
	slide_right
	slide_right
	slide_right
	step_end

SaffronCity_TrainStation_Movement_SlideLeft:
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	slide_left
	step_end

SaffronCity_TrainStation_Text_OfficerRightThisWay:
    .string "May I see your rail PASS, please?\p"
    .string "OK. Right this way, please.$"

SaffronCity_TrainStation_Text_OfficerNoRailPass:
    .string "Sorry. You don't have a rail PASS.$"

SaffronCity_TrainStation_Text_GymGuide_ReturnedPart:
	.string "Whew…\p"
	.string "How many times have I gone back\n"
	.string "and forth between KANTO and JOHTO?$"

SaffronCity_TrainStation_Text_Teacher:
	.string "This MAGNET TRAIN STATION is brand\n"
	.string "new.\p"
	.string "It connects SAFFRON CITY directly to\n"
	.string "GOLDENROD CITY in JOHTO.$"

SaffronCity_TrainStation_Text_Lass:
	.string "Hi. Do you have a rail PASS?\n"
	.string "I have one.\p"
	.string "All the people in SAFFRON who ride\n"
	.string "the MAGNET TRAIN have PASSES.$"
