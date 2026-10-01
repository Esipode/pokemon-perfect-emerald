SafariZone_SecretHouse_Frlg_MapScripts::
	.byte 0

SafariZone_SecretHouse_EventScript_Attendant::
	lock
	faceplayer
	goto_if_set FLAG_GOT_HM03, SafariZone_SecretHouse_EventScript_ExplainSurf
	msgbox SafariZone_SecretHouse_Text_CongratsYouveWon
	checkitemspace ITEM_TM_MUDDY_WATER
	goto_if_eq VAR_RESULT, FALSE, SafariZone_SecretHouse_EventScript_NoRoomForHM03
	giveitem_msg SafariZone_SecretHouse_Text_ReceivedPrizeFromAttendant, ITEM_TM_MUDDY_WATER
	msgbox SafariZone_SecretHouse_Text_ExplainPrize
	setflag FLAG_GOT_HM03
	release
	end

SafariZone_SecretHouse_EventScript_NoRoomForHM03::
	msgbox SafariZone_SecretHouse_Text_DontHaveRoomForPrize
	release
	end

SafariZone_SecretHouse_EventScript_ExplainSurf::
	msgbox SafariZone_SecretHouse_Text_ExplainPrize
	release
	end

SafariZone_SecretHouse_Text_CongratsYouveWon::
	.string "Ah! Finally!\p"
	.string "You're the first person to reach\n"
	.string "the SECRET HOUSE!\p"
	.string "Although I made a campaign\n"
	.string "for our grand opening,\p"
	.string "I was getting worried that no one\n"
	.string "would win our campaign prize.\p"
	.string "Congratulations!\n"
	.string "You have won!$"

SafariZone_SecretHouse_Text_ReceivedPrizeFromAttendant::
	.string "{PLAYER} received TM131\n"
	.string "from the attendant!$"

SafariZone_SecretHouse_Text_ExplainPrize::
	.string "TM131 contains MUDDY WATER.\n"
	.string "It may lower the foe's accuracy.\p"
	.string "You're super lucky for winning this\n"
	.string "fabulous prize!$"

SafariZone_SecretHouse_Text_DontHaveRoomForPrize::
	.string "You don't have room for this\n"
	.string "fabulous prize!$"

