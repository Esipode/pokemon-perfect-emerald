@ Battle Emporium — shared instructor flow: reward menu, ace preview, opponent
@ roll. Per-building lobby scripts set VAR_0x8004 to the enum EmporiumId, show the
@ one-time intro, then jump to Emporium_EventScript_OpenMenu. VAR_0x8004 stays set
@ for the whole interaction; VAR_EMPORIUM_ID / VAR_EMPORIUM_REWARD are written only
@ once the player commits, and the redirect (gEmporiumBattleActive) is armed by
@ EmporiumRollChallenger. The battle itself and the reward payout are Stage 8.

Emporium_EventScript_OpenMenu::
	special EmporiumMenu_BuildList
	goto_if_eq VAR_RESULT, 0, Emporium_EventScript_NoRewardsAvailable
	message Emporium_Text_WhichPrize
	waitmessage
	dynmultistack 0, 0, FALSE, EMPORIUM_MENU_PAGE_SIZE, FALSE, 0, DYN_MULTICHOICE_CB_SHOW_ITEM
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Emporium_EventScript_Goodbye
	special EmporiumMenu_CommitReward
	goto_if_eq VAR_RESULT, FALSE, Emporium_EventScript_OpenMenu
	special EmporiumMenu_BufferConfirm
	goto_if_eq VAR_0x8004, EMPORIUM_MEGA, Emporium_EventScript_ConfirmMega
	goto_if_eq VAR_0x8004, EMPORIUM_TERA, Emporium_EventScript_ConfirmTera
	msgbox Emporium_Text_ConfirmZMove, MSGBOX_YESNO
	goto Emporium_EventScript_ConfirmResult

Emporium_EventScript_ConfirmMega:
	msgbox Emporium_Text_ConfirmMega, MSGBOX_YESNO
	goto Emporium_EventScript_ConfirmResult

Emporium_EventScript_ConfirmTera:
	msgbox Emporium_Text_ConfirmTera, MSGBOX_YESNO

Emporium_EventScript_ConfirmResult:
	goto_if_eq VAR_RESULT, NO, Emporium_EventScript_OpenMenu
	copyvar VAR_EMPORIUM_ID, VAR_0x8004
	special EmporiumRollChallenger
	special EmporiumShowChallenger
	msgbox Emporium_Text_HeadThrough, MSGBOX_DEFAULT
	closemessage
	release
	end

Emporium_EventScript_Goodbye::
	closemessage
	callnative ClearEmporiumBattle
	release
	end

Emporium_EventScript_NoRewardsAvailable::
	callnative ClearEmporiumBattle
	msgbox Emporium_Text_NoRewardsAvailable, MSGBOX_DEFAULT
	release
	end

Emporium_Text_WhichPrize:
	.string "Which prize has caught your eye?$"

Emporium_Text_ConfirmZMove:
	.string "That Z-Crystal is calling for a\n"
	.string "partner in battle.\p"
	.string "The challenger will send out {STR_VAR_2},\n"
	.string "clutching the {STR_VAR_1}.\p"
	.string "Win, and the crystal is yours.\n"
	.string "Ready to battle?$"

Emporium_Text_ConfirmMega:
	.string "The challenger's ace will Mega Evolve\n"
	.string "with the {STR_VAR_1}.\p"
	.string "That ace is {STR_VAR_2}.\p"
	.string "Win, and the stone is yours.\n"
	.string "Ready to battle?$"

Emporium_Text_ConfirmTera:
	.string "The challenger's ace will Terastallize\n"
	.string "to the {STR_VAR_2} type.\p"
	.string "Win, and the {STR_VAR_1} is yours.\n"
	.string "Ready to battle?$"

Emporium_Text_HeadThrough:
	.string "Then head through to the battle room.\n"
	.string "Your challenger is waiting.$"

Emporium_Text_NoRewardsAvailable:
	.string "You already hold every prize this\n"
	.string "Emporium has to offer.\p"
	.string "Come back if you part with one.$"
