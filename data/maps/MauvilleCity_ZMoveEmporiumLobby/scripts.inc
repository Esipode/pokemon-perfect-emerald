MauvilleCity_ZMoveEmporiumLobby_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MauvilleCity_ZMoveEmporiumLobby_OnTransition
	.byte 0

@ Defensive teardown: entering the lobby always means no challenge is in progress,
@ so disarm the TRAINER_EMPORIUM redirect and drop any pending selection. Backstop
@ for a crash or white-out that skipped the post-battle ClearEmporiumBattle.
MauvilleCity_ZMoveEmporiumLobby_OnTransition:
	callnative ClearEmporiumBattle
	setflag FLAG_EMPORIUM_ZMOVE_CHALLENGER_HIDDEN
	end

MauvilleCity_ZMoveEmporiumLobby_EventScript_Instructor::
	lock
	faceplayer
	checkitem ITEM_Z_POWER_RING
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_ZMoveEmporiumLobby_EventScript_InstructorLocked
	setvar VAR_0x8004, EMPORIUM_ZMOVE
	goto_if_set FLAG_EMPORIUM_ZMOVE_INTRO_DONE, Emporium_EventScript_OpenMenu
	msgbox MauvilleCity_ZMoveEmporiumLobby_Text_InstructorIntro, MSGBOX_DEFAULT
	setflag FLAG_EMPORIUM_ZMOVE_INTRO_DONE
	goto Emporium_EventScript_OpenMenu

MauvilleCity_ZMoveEmporiumLobby_EventScript_InstructorLocked::
	msgbox MauvilleCity_ZMoveEmporiumLobby_Text_InstructorLocked, MSGBOX_DEFAULT
	release
	end

MauvilleCity_ZMoveEmporiumLobby_Text_InstructorIntro:
	.string "Welcome to the Z-Move Emporium.\p"
	.string "Pick a Z-Crystal, then defeat the\n"
	.string "Trainer whose ace holds its twin and\l"
	.string "can unleash its Z-Power.\p"
	.string "Beat them and the crystal is yours.$"

MauvilleCity_ZMoveEmporiumLobby_Text_InstructorLocked:
	.string "Z-Moves need a Z-Power Ring.\p"
	.string "Come back once the Mauville Gym\n"
	.string "Leader has handed you one.$"
