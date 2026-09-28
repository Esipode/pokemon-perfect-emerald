InfinityCave_Entrance_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, InfinityCave_Entrance_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, InfinityCave_Entrance_OnFrame
	.byte 0

@ Reaching the lobby always means the descent is over, however it ended. A defeat
@ closes the run out before the warp, so InfCave_EndRunQuit raises VAR_TEMP_1 off
@ the payout the records still owe rather than off the run it just ended.
InfinityCave_Entrance_OnTransition:
	special InfCave_EndRunQuit
	end

InfinityCave_Entrance_OnFrame:
	map_script_2 VAR_TEMP_1, 1, InfinityCave_Entrance_EventScript_RunReport
	.2byte 0

@ Run-end payout. Announces the descent - win, loss or walked out, all three land
@ here - and hands over every depth milestone it earned.
InfinityCave_Entrance_EventScript_RunReport::
	lockall
	special InfCave_BufferRunReport
	msgbox InfinityCave_Entrance_Text_RunReport, MSGBOX_DEFAULT
	special InfCave_BufferSalvageReport
	call_if_eq VAR_RESULT, TRUE, InfinityCave_Entrance_EventScript_Salvage
	special InfCave_AreMilestonesOwed
	goto_if_eq VAR_RESULT, FALSE, InfinityCave_Entrance_EventScript_RunReportDone
	msgbox InfinityCave_Entrance_Text_MilestoneIntro, MSGBOX_DEFAULT

@ One item per pass. The cursor only advances once the item was actually taken, so
@ a full Bag and a full PC leave the rest owed for the next visit.
InfinityCave_Entrance_EventScript_MilestoneItem::
	special InfCave_PrepareMilestoneItem
	goto_if_eq VAR_RESULT, FALSE, InfinityCave_Entrance_EventScript_RunReportDone
	callstd STD_OBTAIN_ITEM
	goto_if_eq VAR_0x8007, FALSE, InfinityCave_Entrance_EventScript_RunReportDone
	special InfCave_AdvanceMilestoneItem
	goto InfinityCave_Entrance_EventScript_MilestoneItem

InfinityCave_Entrance_EventScript_RunReportDone::
	special InfCave_ClearRunReport
	closemessage
	releaseall
	end

@ Whatever the descent still had in the run inventory, priced in shards. Skipped
@ when it ended with an empty bag.
InfinityCave_Entrance_EventScript_Salvage::
	msgbox InfinityCave_Entrance_Text_Salvage, MSGBOX_DEFAULT
	return

@ The lobby shop, which spends the same shards the cave merchant does. Its stock
@ is gated on best depth, so the greeting never has to explain a bare shelf: the
@ rows the player has not unlocked are simply not in the list.
InfinityCave_Entrance_EventScript_Shopkeeper::
	lock
	faceplayer
	msgbox InfinityCave_Entrance_Text_ShopGreet, MSGBOX_DEFAULT
	closemessage
	special InfCaveLobby_OpenMart
	waitstate
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
	@ Depth 1 is a chosen node like every other floor, so the first descent goes
	@ through the node screen too. The coordinates are ignored: the generator
	@ places the player itself.
	special InfCave_OpenNodeScreen
	waitstate
	special InfCave_TakeChosenNode
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

InfinityCave_Entrance_Text_RunReport:
	.string "You made it to floor {STR_VAR_1}.\p"
	.string "That run brought back {STR_VAR_2}\n"
	.string "SHARDS. You're holding {STR_VAR_3}.$"

InfinityCave_Entrance_Text_Salvage:
	.string "The supplies you didn't use were\n"
	.string "traded in for {STR_VAR_1} SHARDS.$"

InfinityCave_Entrance_Text_MilestoneIntro:
	.string "And going that deep earns you\n"
	.string "something the surface won't sell.$"

InfinityCave_Entrance_Text_ShopGreet:
	.string "SHARDS only, and I stock by\n"
	.string "reputation.\p"
	.string "Get deeper and I'll find better\n"
	.string "things to sell you.$"
