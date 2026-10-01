Route16_NorthEntrance_2F_Frlg_MapScripts::
	.byte 0

Route16_NorthEntrance_2F_EventScript_LittleBoy::
	msgbox Route16_NorthEntrance_2F_Text_OnBikeRideWithGirlfriend, MSGBOX_NPC
	end

Route16_NorthEntrance_2F_EventScript_LittleGirl::
	msgbox Route16_NorthEntrance_2F_Text_RidingTogetherOnNewBikes, MSGBOX_NPC
	end

Route16_NorthEntrance_2F_EventScript_LeftBinoculars::
	msgbox Route16_NorthEntrance_2F_Text_ItsCeladonDeptStore, MSGBOX_SIGN
	end

Route16_NorthEntrance_2F_EventScript_RightBinoculars::
	msgbox Route16_NorthEntrance_2F_Text_LongPathOverWater, MSGBOX_SIGN
	end

Route16_NorthEntrance_2F_EventScript_Aide::
	lock
	faceplayer
	goto_if_set FLAG_GOT_AMULET_COIN_FROM_OAKS_AIDE, Route16_NorthEntrance_2F_EventScript_AlreadyGotAmuletCoin
	msgbox Route16_NorthEntrance_2F_Text_GiveAmuletCoin
	bufferitemname STR_VAR_2, ITEM_AMULET_COIN
	checkitemspace ITEM_AMULET_COIN
	goto_if_eq VAR_RESULT, FALSE, Aide_EventScript_NoRoomForItem
	giveitem_msg Route16_NorthEntrance_2F_Text_ReceivedAmuletCoinFromAide, ITEM_AMULET_COIN
	setflag FLAG_GOT_AMULET_COIN_FROM_OAKS_AIDE
	msgbox Route16_NorthEntrance_2F_Text_ExplainAmuletCoin
	release
	end

Route16_NorthEntrance_2F_EventScript_AlreadyGotAmuletCoin::
	msgbox Route16_NorthEntrance_2F_Text_ExplainAmuletCoin
	release
	end

Route16_NorthEntrance_2F_Text_OnBikeRideWithGirlfriend::
	.string "I'm on a relaxing ride on my new\n"
	.string "bike with my girlfriend.$"

Route16_NorthEntrance_2F_Text_RidingTogetherOnNewBikes::
	.string "We're going riding together on our\n"
	.string "new bikes.$"

Route16_NorthEntrance_2F_Text_ItsCeladonDeptStore::
	.string "Let's see what the binoculars have\n"
	.string "to show…\p"
	.string "It's the CELADON DEPT. STORE!$"

Route16_NorthEntrance_2F_Text_LongPathOverWater::
	.string "Let's see what the binoculars have\n"
	.string "to show…\p"
	.string "There's a long path over water far\n"
	.string "away.$"

Route16_NorthEntrance_2F_Text_GiveAmuletCoin::
	.string "Oh… {PLAYER}!\n"
	.string "I've been looking for you!\p"
	.string "I'm one of PROF. OAK's AIDES.\p"
	.string "He entrusted me with this\n"
	.string "AMULET COIN for you.\p"
	.string "Here you go!$"

Route16_NorthEntrance_2F_Text_ReceivedAmuletCoinFromAide::
	.string "{PLAYER} received the AMULET COIN\n"
	.string "from the AIDE.$"

Route16_NorthEntrance_2F_Text_ExplainAmuletCoin::
	.string "An AMULET COIN is an item to be\n"
	.string "held by a POKéMON.\p"
	.string "If the POKéMON appears in a winning\n"
	.string "battle, you will earn more money.$"

