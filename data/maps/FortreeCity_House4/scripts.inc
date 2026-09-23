FortreeCity_House4_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, FortreeCity_House4_OnLoad
	.byte 0

FortreeCity_House4_OnLoad:
	@ Rotom TV chain, stage 2 of 9: postgame, not owned, previous stage cleared
	setvar VAR_0x8004, SPECIES_ROTOM
	setvar VAR_0x8005, LAYOUT_FORTREE_CITY_HOUSE2
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, FortreeCity_House4_HideRotomTV
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 1, FortreeCity_House4_HideRotomTV
	goto FortreeCity_House4_ShowRotomTV

FortreeCity_House4_Rotom_Encounter::
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 1, FortreeCity_House4_EventScript_RotomTVNothing
	lockall
	msgbox FortreeCity_House4_Text_RotomInterference, MSGBOX_DEFAULT
	addvar VAR_ROTOM_TV_PROGRESS, 1
	special TurnOffTVScreen
	releaseall
	end

FortreeCity_House4_EventScript_RotomTVNothing:
	end

FortreeCity_House4_HideRotomTV:
	end

FortreeCity_House4_ShowRotomTV:
	special TurnOnTVScreen
	end

FortreeCity_House4_Text_RotomInterference:
	.string "BZZT!\pThe interference on the TV\n"
	.string "suddenly vanished...$"

FortreeCity_House4_EventScript_Woman::
	msgbox FortreeCity_House4_Text_BringsWorldCloserTogether, MSGBOX_NPC
	end

FortreeCity_House4_EventScript_Boy::
	lockall
	goto_if_set FLAG_RECEIVED_MENTAL_HERB, FortreeCity_House4_EventScript_ReceivedMentalHerb
	goto_if_set FLAG_WINGULL_DELIVERED_MAIL, FortreeCity_House4_EventScript_WingullReturned
	goto_if_set FLAG_WINGULL_SENT_ON_ERRAND, FortreeCity_House4_EventScript_WingullOnErrand
	msgbox FortreeCity_House4_Text_GoBirdPokemon, MSGBOX_DEFAULT
	closemessage
	setflag FLAG_WINGULL_SENT_ON_ERRAND
	clearflag FLAG_HIDE_MOSSDEEP_CITY_HOUSE_2_WINGULL
	applymovement LOCALID_FORTREE_HOUSE_WINGULL, FortreeCity_House4_Movement_WingullExit
	waitmovement 0
	removeobject LOCALID_FORTREE_HOUSE_WINGULL
	releaseall
	end

FortreeCity_House4_EventScript_WingullOnErrand::
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox FortreeCity_House4_Text_AskedWingullToRunErrand, MSGBOX_DEFAULT
	releaseall
	end

FortreeCity_House4_EventScript_WingullReturned::
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox FortreeCity_House4_Text_WelcomeWingullTakeMentalHerb, MSGBOX_DEFAULT
	giveitem ITEM_MENTAL_HERB
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_MENTAL_HERB
	releaseall
	end

FortreeCity_House4_EventScript_ReceivedMentalHerb::
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox FortreeCity_House4_Text_FriendsFarAwayThanksToWingull, MSGBOX_DEFAULT
	releaseall
	end

FortreeCity_House4_Movement_WingullExit:
	walk_fast_down
	walk_fast_down
	walk_fast_right
	walk_in_place_faster_down
	delay_8
	step_end

FortreeCity_House4_EventScript_Wingull::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_WINGULL, CRY_MODE_NORMAL
	msgbox FortreeCity_House4_Text_Wingull, MSGBOX_DEFAULT
	waitmoncry
	release
	end

FortreeCity_House4_Text_BringsWorldCloserTogether:
	.string "By being together with POKéMON,\n"
	.string "people make more and more friends.\p"
	.string "And that brings the world closer\n"
	.string "together. I think it's wonderful!$"

FortreeCity_House4_Text_GoBirdPokemon:
	.string "There!\n"
	.string "Go, BIRD POKéMON!$"

FortreeCity_House4_Text_AskedWingullToRunErrand:
	.string "Heheh, I asked my WINGULL to run\n"
	.string "an errand for me.$"

FortreeCity_House4_Text_WelcomeWingullTakeMentalHerb:
	.string "Good!\n"
	.string "Welcome back, WINGULL!\p"
	.string "Huh? What is this?\n"
	.string "What is it holding?\p"
	.string "A MENTAL HERB?\n"
	.string "It must have picked it up somewhere.\p"
	.string "But I'm not a TRAINER, so you can\n"
	.string "have it.$"

FortreeCity_House4_Text_FriendsFarAwayThanksToWingull:
	.string "Thanks to my WINGULL, I have friends\n"
	.string "who live far away.$"

FortreeCity_House4_Text_Wingull:
	.string "WINGULL: Pihyoh!$"

