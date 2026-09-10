LilycoveCity_TeraEmporiumLobby_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LilycoveCity_TeraEmporiumLobby_OnTransition
	.byte 0

@ Defensive teardown: entering the lobby always means no challenge is in progress,
@ so disarm the TRAINER_EMPORIUM redirect and drop any pending selection. Backstop
@ for a crash or white-out that skipped the post-battle ClearEmporiumBattle.
LilycoveCity_TeraEmporiumLobby_OnTransition:
	callnative ClearEmporiumBattle
	setflag FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN
	end

@ Stage 6 placeholder. Stage 7 replaces this with the reward menu + opponent roll.
LilycoveCity_TeraEmporiumLobby_EventScript_Instructor::
	lock
	faceplayer
	checkitem ITEM_TERA_ORB
	goto_if_eq VAR_RESULT, FALSE, LilycoveCity_TeraEmporiumLobby_EventScript_InstructorLocked
	msgbox LilycoveCity_TeraEmporiumLobby_Text_InstructorPlaceholder, MSGBOX_DEFAULT
	release
	end

LilycoveCity_TeraEmporiumLobby_EventScript_InstructorLocked::
	msgbox LilycoveCity_TeraEmporiumLobby_Text_InstructorLocked, MSGBOX_DEFAULT
	release
	end

LilycoveCity_TeraEmporiumLobby_Text_InstructorPlaceholder:
	.string "Welcome to the Tera Emporium.\p"
	.string "Choose a Tera Shard, then defeat the\n"
	.string "Trainer whose ace Terastallizes to\l"
	.string "match it.\p"
	.string "(The selection menu arrives in a\n"
	.string "later update.)$"

LilycoveCity_TeraEmporiumLobby_Text_InstructorLocked:
	.string "Terastallizing needs a Tera Orb.\p"
	.string "Come back once the Mossdeep Gym\n"
	.string "Leader has entrusted you with one.$"
