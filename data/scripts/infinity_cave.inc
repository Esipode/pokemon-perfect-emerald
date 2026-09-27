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
