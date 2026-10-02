MtSilver_1F_WaterfallRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtSilver_1F_WaterfallRoom_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, MtSilver_1F_WaterfallRoom_OnWarp
	map_script MAP_SCRIPT_ON_FRAME_TABLE, MtSilver_1F_WaterfallRoom_OnFrame
	map_script MAP_SCRIPT_ON_RESUME, MtSilver_1F_WaterfallRoom_OnResume
	.byte 0

MtSilver_1F_WaterfallRoom_OnResume::
	setescapewarp MAP_MT_SILVER_OUTSIDE, 17, 8
	end

MtSilver_1F_WaterfallRoom_OnWarp::
	.2byte 0

MtSilver_1F_WaterfallRoom_TurnPlayer::
	turnobject LOCALID_PLAYER, DIR_WEST
	end

MtSilver_1F_WaterfallRoom_OnFrame::
	.2byte 0

MtSilver_1F_WaterfallRoom_Scientist_Scene::
	msgbox MtSilver_1F_WaterfallRoom_Text_Scientist, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_MT_SILVER_1F_WATERFALL_ROOM_SCIENTIST, MtSilver_1F_Movement_Scientist
	playse SE_LEDGE
	waitmovement 0
	removeobject LOCALID_MT_SILVER_1F_WATERFALL_ROOM_SCIENTIST
	setflag FLAG_MT_SILVER_1F_HIDE_SCIENTIST
	release
	end

MtSilver_1F_WaterfallRoom_Text_Scientist:
	.string "This is the entrance to the\n"
	.string "unexplored branch... {PAUSE 0x0F}{PAUSE 0x0F}{PAUSE 0x0F}\p"
	.string "Does it feel cold here to you?\p"
	.string "Okay, I'm going to leave now.\p"
	.string "Good luck!$"

MtSilver_1F_Movement_Scientist:
	jump_2_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	step_end

MtSilver_1F_WaterfallRoom_OnTransition: 
	setescapewarp MAP_MT_SILVER_OUTSIDE, 17, 8
	end

MtSilver_1F_WaterfallRoom_EventScript_MaxPotion::
	finditem ITEM_MAX_POTION
	end

MtSilver_1F_WaterfallRoom_EventScript_DireHit::
	finditem ITEM_DIRE_HIT
	end

MtSilver_1F_WaterfallRoom_EventScript_FullRestore::
	finditem ITEM_FULL_RESTORE
	end

MtSilver_1F_WaterfallRoom_EventScript_UltraBall::
	finditem ITEM_ULTRA_BALL
	end

MtSilver_1F_WaterfallRoom_EventScript_SpellTag::
	finditem ITEM_SPELL_TAG
	end
