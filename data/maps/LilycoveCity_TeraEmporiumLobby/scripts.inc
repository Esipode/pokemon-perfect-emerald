LilycoveCity_TeraEmporiumLobby_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LilycoveCity_TeraEmporiumLobby_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, LilycoveCity_TeraEmporiumLobby_OnFrame
	.byte 0

@ Defensive teardown: entering the lobby always means no challenge is in progress,
@ so disarm the TRAINER_EMPORIUM redirect and re-hide the challenger. Keeps the
@ pending reward when the player is walking back in from a won battle (see
@ EmporiumLobbyOnTransition). Backstop for a crash or white-out too.
LilycoveCity_TeraEmporiumLobby_OnTransition:
	callnative EmporiumLobbyOnTransition
	setflag FLAG_TEMP_HIDE_FOLLOWER @ The payout and recovery scenes script the player's steps
	setflag FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN
	call Emporium_EventScript_LobbyRestoreInstructorAside
	end

@ Post-battle scene: fires only when the player has just returned from the battle
@ room (VAR_EMPORIUM_RESULT set). Shared with the other two lobbies.
LilycoveCity_TeraEmporiumLobby_OnFrame:
	map_script_2 VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_WON, Emporium_EventScript_LobbyReward
	map_script_2 VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_LOST, Emporium_EventScript_LobbyRecovery
	.2byte 0

LilycoveCity_TeraEmporiumLobby_EventScript_Instructor::
	lock
	faceplayer
	checkitem ITEM_TERA_ORB
	goto_if_eq VAR_RESULT, FALSE, LilycoveCity_TeraEmporiumLobby_EventScript_InstructorLocked
	setvar VAR_0x8004, EMPORIUM_TERA
	goto_if_set FLAG_EMPORIUM_TERA_INTRO_DONE, Emporium_EventScript_OpenMenu
	msgbox LilycoveCity_TeraEmporiumLobby_Text_InstructorIntro, MSGBOX_DEFAULT
	setflag FLAG_EMPORIUM_TERA_INTRO_DONE
	goto Emporium_EventScript_OpenMenu

LilycoveCity_TeraEmporiumLobby_EventScript_InstructorLocked::
	msgbox LilycoveCity_TeraEmporiumLobby_Text_InstructorLocked, MSGBOX_DEFAULT
	release
	end

LilycoveCity_TeraEmporiumLobby_Text_InstructorIntro:
	.string "Welcome to the Tera Emporium.\p"
	.string "Choose a Tera Shard, then defeat the\n"
	.string "Trainer whose ace Terastallizes to\l"
	.string "match it.\p"
	.string "Beat them and the shard is yours.$"

LilycoveCity_TeraEmporiumLobby_Text_InstructorLocked:
	.string "Terastallizing needs a Tera Orb.\p"
	.string "Come back once the Mossdeep Gym\n"
	.string "Leader has entrusted you with one.$"
