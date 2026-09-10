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

@ Stage 6 placeholder. Stage 7 replaces this with the reward menu + opponent roll.
MauvilleCity_ZMoveEmporiumLobby_EventScript_Instructor::
	lock
	faceplayer
	checkitem ITEM_Z_POWER_RING
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_ZMoveEmporiumLobby_EventScript_InstructorLocked
	msgbox MauvilleCity_ZMoveEmporiumLobby_Text_InstructorPlaceholder, MSGBOX_DEFAULT
	release
	end

MauvilleCity_ZMoveEmporiumLobby_EventScript_InstructorLocked::
	msgbox MauvilleCity_ZMoveEmporiumLobby_Text_InstructorLocked, MSGBOX_DEFAULT
	release
	end

MauvilleCity_ZMoveEmporiumLobby_Text_InstructorPlaceholder:
	.string "Welcome to the Z-Move Emporium.\p"
	.string "Pick your prize, then prove you can\n"
	.string "wield its power in battle.\p"
	.string "(The selection menu arrives in a\n"
	.string "later update.)$"

MauvilleCity_ZMoveEmporiumLobby_Text_InstructorLocked:
	.string "Z-Moves need a Z-Power Ring.\p"
	.string "Come back once the Mauville Gym\n"
	.string "Leader has handed you one.$"
