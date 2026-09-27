@ Shared Infinity Cave scripts. Map-local lobby scripts live in
@ data/maps/InfinityCave_Entrance/scripts.inc.

@ Carves the cave mouth into EverGrandeCity at (24, 43). Called from that map's
@ ON_RESUME once FLAG_SYS_GAME_CLEAR is set, so the entrance does not exist
@ before the Hall of Fame. The tiles are the generic General-tileset cave mouth;
@ swap them for EverGrande-specific art once it is drawn.
InfinityCave_EventScript_OpenEntrance::
	setmetatilewithelevation 24, 42, METATILE_General_CaveEntrance_Top, TRUE, 0
	setmetatilewithelevation 24, 43, METATILE_General_CaveEntrance_Bottom, FALSE, 0
	return

@ The exit crystal the generator drops on every room's exit pad. Descending
@ advances the depth, which re-rolls the room seed, then warps back into the same
@ map so the next room is generated. The room map has no warp of its own, so this
@ is also the way out: the lobby's ON_TRANSITION closes the run.
InfinityCave_EventScript_ExitCrystal::
	lock
	faceplayer
	msgbox InfinityCave_Text_CrystalAskDescend, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, InfinityCave_EventScript_CrystalAskLeave
	closemessage
	special InfCave_AdvanceDepth
	warp MAP_INFINITY_CAVE_ROOM, 0, 0
	waitstate
	release
	end

InfinityCave_EventScript_CrystalAskLeave::
	msgbox InfinityCave_Text_CrystalAskLeave, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, InfinityCave_EventScript_CrystalDone
	closemessage
	warp MAP_INFINITY_CAVE_ENTRANCE, 0
	waitstate
	release
	end

InfinityCave_EventScript_CrystalDone::
	release
	end

InfinityCave_Text_CrystalAskDescend:
	.string "A crystal pulses in the dark, and the\n"
	.string "floor below it has fallen away.\p"
	.string "Go deeper?$"

InfinityCave_Text_CrystalAskLeave:
	.string "Leave the cave instead?$"
