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
	checkmoney EMPORIUM_ENTRY_FEE
	goto_if_eq VAR_RESULT, FALSE, Emporium_EventScript_CantAffordFee
	showmoneybox 0, 0
	removemoney EMPORIUM_ENTRY_FEE
	updatemoneybox
	playse SE_SHOP
	msgbox Emporium_Text_FeePaid, MSGBOX_DEFAULT
	hidemoneybox
	copyvar VAR_EMPORIUM_ID, VAR_0x8004
	special EmporiumRollChallenger
	special EmporiumShowChallenger
	msgbox Emporium_Text_HeadThrough, MSGBOX_DEFAULT
	closemessage
	release
	end

Emporium_EventScript_CantAffordFee::
	msgbox Emporium_Text_CantAffordFee, MSGBOX_DEFAULT
	goto Emporium_EventScript_OpenMenu

@ ---- Stage 8: challenger battle outcome ----
@ Reached inline from each battle room's challenger script (continueScript
@ trainerbattle). On a win the outcome check runs; on a loss it runs only when
@ EmporiumArmNoWhiteout armed B_FLAG_NO_WHITEOUT (i.e. outside Nuzlocke), where
@ the per-room script warps the player back to its lobby.

Emporium_EventScript_ChallengerDefeated::
	special EmporiumBufferRewardItem
	msgbox Emporium_Text_RewardWon, MSGBOX_DEFAULT
	giveitem VAR_0x8004
	callnative ClearEmporiumBattle
	releaseall
	end

Emporium_EventScript_ChallengerLostCommon::
	special HealPlayerParty
	callnative ClearEmporiumBattle
	msgbox Emporium_Text_ChallengerLost, MSGBOX_DEFAULT
	closemessage
	return

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
	.string "The entry fee is ¥2,500.\p"
	.string "Ready to battle?$"

Emporium_Text_ConfirmMega:
	.string "The challenger's ace will Mega Evolve\n"
	.string "with the {STR_VAR_1}.\p"
	.string "That ace is {STR_VAR_2}.\p"
	.string "Win, and the stone is yours.\n"
	.string "The entry fee is ¥2,500.\p"
	.string "Ready to battle?$"

Emporium_Text_ConfirmTera:
	.string "The challenger's ace will Terastallize\n"
	.string "to the {STR_VAR_2} type.\p"
	.string "Win, and the {STR_VAR_1} is yours.\n"
	.string "The entry fee is ¥2,500.\p"
	.string "Ready to battle?$"

Emporium_Text_HeadThrough:
	.string "Then head through to the battle room.\n"
	.string "Your challenger is waiting.$"

Emporium_Text_NoRewardsAvailable:
	.string "You already hold every prize this\n"
	.string "Emporium has to offer.\p"
	.string "Come back if you part with one.$"

Emporium_Text_CantAffordFee:
	.string "The entry fee is ¥2,500, and it\n"
	.string "looks like you're a little short.$"

Emporium_Text_FeePaid:
	.string "Thanks! The challenger is warming\n"
	.string "up for you now.$"

@ Shown by the battle engine as the challenger's lines (intro / their loss /
@ their win). Shared across all three Emporiums; Stage 9 adds per-identity flavour.
Emporium_Text_ChallengerIntro:
	.string "So you're my challenge today?\n"
	.string "Let's see what you've got!$"

Emporium_Text_ChallengerDefeat:
	.string "Beaten fair and square.\n"
	.string "The prize is yours.$"

Emporium_Text_ChallengerVictory:
	.string "Not this time!\n"
	.string "Come back when you're ready.$"

Emporium_Text_RewardWon:
	.string "A deal's a deal. Here's the\n"
	.string "{STR_VAR_1} you earned!$"

Emporium_Text_ChallengerLost:
	.string "You and your team have been\n"
	.string "patched up.\p"
	.string "Talk to me when you'd like to\n"
	.string "try again.$"
