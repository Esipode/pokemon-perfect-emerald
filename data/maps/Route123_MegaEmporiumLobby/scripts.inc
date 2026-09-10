Route123_MegaEmporiumLobby_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route123_MegaEmporiumLobby_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Route123_MegaEmporiumLobby_OnFrame
	.byte 0

@ Defensive teardown: entering the lobby always means no challenge is in progress,
@ so disarm the TRAINER_EMPORIUM redirect and re-hide the challenger. Keeps the
@ pending reward when the player is walking back in from a won battle (see
@ EmporiumLobbyOnTransition). Backstop for a crash or white-out too.
Route123_MegaEmporiumLobby_OnTransition:
	callnative EmporiumLobbyOnTransition
	setflag FLAG_TEMP_HIDE_FOLLOWER @ The payout and recovery scenes script the player's steps
	setflag FLAG_EMPORIUM_MEGA_CHALLENGER_HIDDEN
	call Emporium_EventScript_LobbyRestoreInstructorAside
	end

@ Post-battle scene: fires only when the player has just returned from the battle
@ room (VAR_EMPORIUM_RESULT set). Shared with the other two lobbies.
Route123_MegaEmporiumLobby_OnFrame:
	map_script_2 VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_WON, Emporium_EventScript_LobbyReward
	map_script_2 VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_LOST, Emporium_EventScript_LobbyRecovery
	.2byte 0

Route123_MegaEmporiumLobby_EventScript_Instructor::
	lock
	faceplayer
	checkitem ITEM_MEGA_RING
	goto_if_eq VAR_RESULT, FALSE, Route123_MegaEmporiumLobby_EventScript_InstructorLocked
	setvar VAR_0x8004, EMPORIUM_MEGA
	goto_if_set FLAG_EMPORIUM_MEGA_INTRO_DONE, Emporium_EventScript_OpenMenu
	msgbox Route123_MegaEmporiumLobby_Text_InstructorIntro, MSGBOX_DEFAULT
	setflag FLAG_EMPORIUM_MEGA_INTRO_DONE
	goto Emporium_EventScript_OpenMenu

Route123_MegaEmporiumLobby_EventScript_InstructorLocked::
	msgbox Route123_MegaEmporiumLobby_Text_InstructorLocked, MSGBOX_DEFAULT
	release
	end

Route123_MegaEmporiumLobby_Text_InstructorIntro:
	.string "Welcome to the Mega Emporium.\p"
	.string "Choose a Mega Stone, then defeat the\n"
	.string "Trainer whose ace Mega Evolves with\l"
	.string "it.\p"
	.string "Beat them and the stone is yours.$"

Route123_MegaEmporiumLobby_Text_InstructorLocked:
	.string "Mega Evolution needs a Mega Ring.\p"
	.string "Come back once the Petalburg Gym\n"
	.string "Leader has handed you one.$"
