InfinityCave_Room_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, InfinityCave_Room_OnLoad
	map_script MAP_SCRIPT_ON_FRAME_TABLE, InfinityCave_Room_OnFrame
	.byte 0

@ ON_LOAD, not ON_TRANSITION: the continue-from-save path skips ON_TRANSITION,
@ and the reload inside a room is exactly the case this has to catch.
InfinityCave_Room_OnLoad:
	special InfCave_ValidateRoom
	end

InfinityCave_Room_OnFrame:
	map_script_2 VAR_TEMP_1, 1, InfinityCave_Room_EventScript_Eject
	.2byte 0

@ The run struct no longer describes this room, so the room cannot be played.
@ Close the run and put the player back in the lobby, whose ON_TRANSITION ends
@ the descent again.
InfinityCave_Room_EventScript_Eject::
	lockall
	setvar VAR_TEMP_1, 0
	msgbox InfinityCave_Room_Text_Ejected, MSGBOX_DEFAULT
	closemessage
	special InfCave_EndRunQuit
	warp MAP_INFINITY_CAVE_ENTRANCE, 0
	waitstate
	releaseall
	end

InfinityCave_Room_Text_Ejected:
	.string "The cave shudders, and the chamber\n"
	.string "collapses around you.\p"
	.string "You come to at the cave mouth.$"
