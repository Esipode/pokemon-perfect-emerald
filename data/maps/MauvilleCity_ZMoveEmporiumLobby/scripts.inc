MauvilleCity_ZMoveEmporiumLobby_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MauvilleCity_ZMoveEmporiumLobby_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, MauvilleCity_ZMoveEmporiumLobby_OnFrame
	.byte 0

@ Defensive teardown: entering the lobby always means no challenge is in progress,
@ so disarm the TRAINER_EMPORIUM redirect and re-hide the challenger. Keeps the
@ pending reward when the player is walking back in from a won battle (see
@ EmporiumLobbyOnTransition). Backstop for a crash or white-out too.
MauvilleCity_ZMoveEmporiumLobby_OnTransition:
	callnative EmporiumLobbyOnTransition
	setflag FLAG_TEMP_HIDE_FOLLOWER @ The payout and recovery scenes script the player's steps
	setflag FLAG_EMPORIUM_ZMOVE_CHALLENGER_HIDDEN
	call Emporium_EventScript_LobbyRestoreInstructorAside
	end

@ Post-battle scene: fires only when the player has just returned from the battle
@ room (VAR_EMPORIUM_RESULT set). Shared with the other two lobbies.
MauvilleCity_ZMoveEmporiumLobby_OnFrame:
	map_script_2 VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_WON, Emporium_EventScript_LobbyReward
	map_script_2 VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_LOST, Emporium_EventScript_LobbyRecovery
	.2byte 0

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
