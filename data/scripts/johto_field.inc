@ Shared Johto field scripts, texts and movement scripts used by the imported Johto maps.

Common_Movement_WalkUp1:
	walk_up
	step_end

Common_Movement_WalkDown1:
	walk_down
	step_end

Common_Movement_WalkRight1:
	walk_right
	step_end

Common_Movement_WalkLeft1:
	walk_left
	step_end

Common_Movement_JumpUp1:
	jump_up
	step_end

Common_Movement_JumpDown1:
	jump_down
	step_end

Common_Movement_JumpRight1:
	jump_right
	step_end

Common_Movement_JumpLeft1:
	jump_left
	step_end

@ Whirlpool tiles open once the player holds 7 Johto badges (no HM item).
EventScript_Whirlpool::
	lockall
	specialvar VAR_RESULT, GetJohtoBadgeCount
	goto_if_lt VAR_RESULT, 7, EventScript_CantWhirlpool
	playse SE_M_WATERFALL
	goto_if_eq VAR_FACING, DIR_NORTH, EventScript_WhirlpoolGoNorth
	goto_if_eq VAR_FACING, DIR_SOUTH, EventScript_WhirlpoolGoSouth
	goto_if_eq VAR_FACING, DIR_EAST, EventScript_WhirlpoolGoEast
	goto_if_eq VAR_FACING, DIR_WEST, EventScript_WhirlpoolGoWest
	releaseall
	end

EventScript_WhirlpoolGoNorth::
	applymovement OBJ_EVENT_ID_PLAYER, Movement_WhirlpoolNorth
	waitmovement 0
	releaseall
	end

EventScript_WhirlpoolGoSouth::
	applymovement OBJ_EVENT_ID_PLAYER, Movement_WhirlpoolSouth
	waitmovement 0
	releaseall
	end

EventScript_WhirlpoolGoEast::
	applymovement OBJ_EVENT_ID_PLAYER, Movement_WhirlpoolEast
	waitmovement 0
	releaseall
	end

EventScript_WhirlpoolGoWest::
	applymovement OBJ_EVENT_ID_PLAYER, Movement_WhirlpoolWest
	waitmovement 0
	releaseall
	end

EventScript_CantWhirlpool::
	msgbox Text_CantWhirlpool, MSGBOX_DEFAULT
	closemessage
	releaseall
	end

Movement_WhirlpoolNorth:
	slide_up
	slide_up
	slide_up
	step_end

Movement_WhirlpoolSouth:
	slide_down
	slide_down
	slide_down
	step_end

Movement_WhirlpoolEast:
	slide_right
	slide_right
	slide_right
	step_end

Movement_WhirlpoolWest:
	slide_left
	slide_left
	slide_left
	step_end

Text_CantWhirlpool:
	.string "The water is swirling and spinning\n"
	.string "around with a mighty roar.$"

Johto_EventScript_LockedDoor::
	msgbox Johto_Text_LockedDoor, MSGBOX_SIGN
	end

Johto_Text_LockedDoor:
	.string "It's locked…$"

SafariZoneGate_Text_ThatWillBe500Please:
	.string "That will be ¥500, please.$"

RocketHideout_B2F_Text_ArianaMultiDefeat:
	.string "You haven't seen the last of me!$"

RocketHideout_B2F_Text_GruntMultiDefeat:
	.string "I won't accept this!$"

@ Radio Tower takeover: runs once, right after the player holds all of the first seven Johto badges,
@ whichever gym comes last. Called from each of those gym victory scripts.
Johto_EventScript_TryRadioTowerTakeover::
	goto_if_ne VAR_TRIGGER_ELM_ROCKET_CALL, 0, Johto_EventScript_RadioTowerTakeoverNo
	goto_if_unset FLAG_JOHTO_BADGE01_GET, Johto_EventScript_RadioTowerTakeoverNo
	goto_if_unset FLAG_JOHTO_BADGE02_GET, Johto_EventScript_RadioTowerTakeoverNo
	goto_if_unset FLAG_JOHTO_BADGE03_GET, Johto_EventScript_RadioTowerTakeoverNo
	goto_if_unset FLAG_JOHTO_BADGE04_GET, Johto_EventScript_RadioTowerTakeoverNo
	goto_if_unset FLAG_JOHTO_BADGE05_GET, Johto_EventScript_RadioTowerTakeoverNo
	goto_if_unset FLAG_JOHTO_BADGE06_GET, Johto_EventScript_RadioTowerTakeoverNo
	goto_if_unset FLAG_JOHTO_BADGE07_GET, Johto_EventScript_RadioTowerTakeoverNo
	closemessage
	setspeaker Route30_Text_ElmName
	pokenavcall ElmPhoneRadioTowerRocketTakeoverText
	waitmessage
	setvar_min VAR_GOLDENROD_CITY_STATE, 6
	clearflag FLAG_HIDE_GOLDENROD_ROCKETS
	setflag FLAG_HIDE_GOLDENROD_ROCKET_SCOUT
	setflag FLAG_HIDE_GOLDENROD_NPCS
	clearflag FLAG_HIDE_ROCKET_GOLDENROD_RADIOTOWER_1
	setvar VAR_TRIGGER_ELM_ROCKET_CALL, 2
	delay 10
Johto_EventScript_RadioTowerTakeoverNo::
	return

@ VAR_LUGIA_OR_HOOH (1 Lugia, 2 Ho-Oh) is normally set by the Route 39 vision. Ask here when the player never walked that road.
Johto_EventScript_EnsureLegendChoice::
	goto_if_ne VAR_LUGIA_OR_HOOH, 0, Johto_EventScript_EnsureLegendChoiceDone
	msgbox Johto_Text_LegendChoice, MSGBOX_DEFAULT
Johto_EventScript_LegendChoiceMenu::
	multichoice 20, 8, MULTI_GOLDSILVER, FALSE
	switch VAR_RESULT
	case 0, Johto_EventScript_LegendChoiceGold
	case 1, Johto_EventScript_LegendChoiceSilver
	goto Johto_EventScript_LegendChoiceMenu

Johto_EventScript_LegendChoiceGold::
	setvar VAR_LUGIA_OR_HOOH, 2
	return

Johto_EventScript_LegendChoiceSilver::
	setvar VAR_LUGIA_OR_HOOH, 1
	return

Johto_EventScript_EnsureLegendChoiceDone::
	return

Johto_Text_LegendChoice:
	.string "A mysterious presence stirs\n"
	.string "somewhere in JOHTO…\p"
	.string "Is it the color of gold,\n"
	.string "or the color of silver?$"
