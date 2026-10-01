Route2_EastBuilding_Frlg_MapScripts::
	.byte 0

Route2_EastBuilding_EventScript_Aide::
	lock
	faceplayer
	goto_if_set FLAG_GOT_HM05, Route2_EastBuilding_EventScript_AlreadyGotReward
	msgbox Route2_EastBuilding_Text_GiveRewardForTrainer
	bufferitemname STR_VAR_2, ITEM_TM_STORED_POWER
	checkitemspace ITEM_TM_STORED_POWER
	goto_if_eq VAR_RESULT, FALSE, Aide_EventScript_NoRoomForItem
	giveitem_msg Route2_EastBuilding_Text_ReceivedRewardFromAide, ITEM_TM_STORED_POWER
	setflag FLAG_GOT_HM05
	release
	end

Route2_EastBuilding_EventScript_AlreadyGotReward::
	msgbox Route2_EastBuilding_Text_ExplainReward
	release
	end

Route2_EastBuilding_EventScript_Rocker::
	msgbox Route2_EastBuilding_Text_CanGetThroughRockTunnel, MSGBOX_NPC
	end

Route2_EastBuilding_Text_GiveRewardForTrainer::
	.string "Hi! Remember me?\n"
	.string "I'm one of PROF. OAK's AIDES.\p"
	.string "PROF. OAK heard that a CHAMPION\n"
	.string "from far away was visiting.\p"
	.string "He asked me to hand you a little\n"
	.string "something.$"

Route2_EastBuilding_Text_ReceivedRewardFromAide::
	.string "{PLAYER} received TM101\n"
	.string "from the AIDE.$"

Route2_EastBuilding_Text_ExplainReward::
	.string "TM101 contains STORED POWER.\n"
	.string "It grows stronger with stat boosts.$"

Route2_EastBuilding_Text_CanGetThroughRockTunnel::
	.string "Once a POKéMON learns FLASH, you\n"
	.string "can get through ROCK TUNNEL.$"

