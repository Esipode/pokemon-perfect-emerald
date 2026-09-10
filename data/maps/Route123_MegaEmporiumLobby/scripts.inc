Route123_MegaEmporiumLobby_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route123_MegaEmporiumLobby_OnTransition
	.byte 0

@ Defensive teardown: entering the lobby always means no challenge is in progress,
@ so disarm the TRAINER_EMPORIUM redirect and drop any pending selection. Backstop
@ for a crash or white-out that skipped the post-battle ClearEmporiumBattle.
Route123_MegaEmporiumLobby_OnTransition:
	callnative ClearEmporiumBattle
	setflag FLAG_EMPORIUM_MEGA_CHALLENGER_HIDDEN
	end

@ Stage 6 placeholder. Stage 7 replaces this with the reward menu + opponent roll.
Route123_MegaEmporiumLobby_EventScript_Instructor::
	lock
	faceplayer
	checkitem ITEM_MEGA_RING
	goto_if_eq VAR_RESULT, FALSE, Route123_MegaEmporiumLobby_EventScript_InstructorLocked
	msgbox Route123_MegaEmporiumLobby_Text_InstructorPlaceholder, MSGBOX_DEFAULT
	release
	end

Route123_MegaEmporiumLobby_EventScript_InstructorLocked::
	msgbox Route123_MegaEmporiumLobby_Text_InstructorLocked, MSGBOX_DEFAULT
	release
	end

Route123_MegaEmporiumLobby_Text_InstructorPlaceholder:
	.string "Welcome to the Mega Emporium.\p"
	.string "Choose a Mega Stone, then defeat the\n"
	.string "Trainer whose ace Mega Evolves with\l"
	.string "it.\p"
	.string "(The selection menu arrives in a\n"
	.string "later update.)$"

Route123_MegaEmporiumLobby_Text_InstructorLocked:
	.string "Mega Evolution needs a Mega Ring.\p"
	.string "Come back once the Petalburg Gym\n"
	.string "Leader has handed you one.$"
