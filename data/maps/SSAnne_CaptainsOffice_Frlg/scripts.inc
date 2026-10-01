SSAnne_CaptainsOffice_Frlg_MapScripts::
	.byte 0

SSAnne_CaptainsOffice_EventScript_Captain::
	lock
	goto_if_set FLAG_GOT_HM01, SSAnne_CaptainsOffice_EventScript_AlreadyGotReward
	msgbox SSAnne_CaptainsOffice_Text_CaptainIFeelSeasick
	textcolor NPC_TEXT_COLOR_NEUTRAL
	message SSAnne_CaptainsOffice_Text_RubbedCaptainsBack
	waitmessage
	playfanfare MUS_HEAL
	waitfanfare
	call EventScript_RestorePrevTextColor
	delay 50
	applymovement LOCALID_SS_ANNE_CAPTAIN, Common_Movement_FacePlayer
	waitmovement 0
	msgbox SSAnne_CaptainsOffice_Text_ThankYouTakeThis
	giveitem_msg SSAnne_CaptainsOffice_Text_ObtainedRareCandyFromCaptain, ITEM_TM_FLIP_TURN
	setflag FLAG_GOT_HM01
	release
	end

@ Unused
SSAnne_CaptainsOffice_EventScript_NoRoomForCut::
	msgbox SSAnne_CaptainsOffice_Text_YouHaveNoRoomForThis
	closemessage
	applymovement LOCALID_SS_ANNE_CAPTAIN, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

SSAnne_CaptainsOffice_EventScript_AlreadyGotReward::
	applymovement LOCALID_SS_ANNE_CAPTAIN, Common_Movement_FacePlayer
	waitmovement 0
	msgbox SSAnne_CaptainsOffice_Text_FeelingMuchBetter
	release
	end

SSAnne_CaptainsOffice_EventScript_TrashCan::
	msgbox SSAnne_CaptainsOffice_Text_YuckShouldntHaveLooked, MSGBOX_SIGN
	end

SSAnne_CaptainsOffice_EventScript_Book::
	msgbox SSAnne_CaptainsOffice_Text_HowToConquerSeasickness, MSGBOX_SIGN
	end

SSAnne_CaptainsOffice_Text_CaptainIFeelSeasick::
	.string "CAPTAIN: Ooargh…\n"
	.string "I feel hideous…\l"
	.string "Urrp! Seasick…$"

SSAnne_CaptainsOffice_Text_RubbedCaptainsBack::
	.string "{PLAYER} rubbed the CAPTAIN's\n"
	.string "back!\p"
	.string "Rub-rub…\n"
	.string "Rub-rub…$"

SSAnne_CaptainsOffice_Text_ThankYouTakeThis::
	.string "CAPTAIN: Whew! Thank you!\n"
	.string "I'm feeling much better now.\p"
	.string "I'd like to thank you properly.\n"
	.string "Please, take this!$"

SSAnne_CaptainsOffice_Text_ObtainedRareCandyFromCaptain::
	.string "{PLAYER} received TM112\n"
	.string "from the CAPTAIN!$"

SSAnne_CaptainsOffice_Text_FeelingMuchBetter::
	.string "CAPTAIN: …Whew!\p"
	.string "Now that I'm not sick anymore,\n"
	.string "I can enjoy the sea air again.\p"
	.string "The S.S. ANNE is always welcome\n"
	.string "in VERMILION CITY!$"

SSAnne_CaptainsOffice_Text_YouHaveNoRoomForThis::
	.string "Oh, no!\n"
	.string "You have no room for this!$"

SSAnne_CaptainsOffice_Text_YuckShouldntHaveLooked::
	.string "Yuck!\n"
	.string "Shouldn't have looked!$"

SSAnne_CaptainsOffice_Text_HowToConquerSeasickness::
	.string "How to Conquer Seasickness…\n"
	.string "The CAPTAIN's reading this!$"

