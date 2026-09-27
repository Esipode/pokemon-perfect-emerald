InfinityCave_Entrance_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, InfinityCave_Entrance_OnTransition
	.byte 0

@ Reaching the lobby always means the descent is over, however it ended.
InfinityCave_Entrance_OnTransition:
	special InfCave_EndRunQuit
	end

InfinityCave_Entrance_EventScript_Attendant::
	lock
	faceplayer
	goto_if_unset FLAG_SYS_GAME_CLEAR, InfinityCave_Entrance_EventScript_NotChampion
	msgbox InfinityCave_Entrance_Text_Explain, MSGBOX_DEFAULT
	msgbox InfinityCave_Entrance_Text_AskDescend, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, InfinityCave_Entrance_EventScript_Decline
	msgbox InfinityCave_Entrance_Text_GoodLuck, MSGBOX_DEFAULT
	closemessage
	special InfCave_StartRun
	special InfCave_AdvanceDepth
	warp MAP_INFINITY_CAVE_ROOM, 3, 3
	waitstate
	release
	end

InfinityCave_Entrance_EventScript_NotChampion::
	msgbox InfinityCave_Entrance_Text_NotChampion, MSGBOX_DEFAULT
	release
	end

InfinityCave_Entrance_EventScript_Decline::
	msgbox InfinityCave_Entrance_Text_Decline, MSGBOX_DEFAULT
	release
	end

InfinityCave_Entrance_Text_Explain:
	.string "The cave behind me has no bottom.\p"
	.string "Every chamber down there is different,\n"
	.string "and every one of them is somebody's\l"
	.string "battleground.\p"
	.string "Go down far enough and you'll come\n"
	.string "back with SHARDS. Lose, and the cave\l"
	.string "spits you right back out here.$"

InfinityCave_Entrance_Text_AskDescend:
	.string "Want to start a descent?$"

InfinityCave_Entrance_Text_GoodLuck:
	.string "Then down you go.\n"
	.string "See how deep you can get!$"

InfinityCave_Entrance_Text_Decline:
	.string "Come back when you're ready.$"

InfinityCave_Entrance_Text_NotChampion:
	.string "This cave only opens for a CHAMPION.$"
