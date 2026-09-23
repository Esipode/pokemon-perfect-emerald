CaveOfOrigin_B1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, CaveOfOrigin_B1F_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, CaveOfOrigin_B1F_OnLoad
	.byte 0

CaveOfOrigin_B1F_OnTransition:
	updatelegendaryvisibility SPECIES_RESHIRAM, FLAG_HIDE_RESHIRAM
	updatelegendaryvisibility SPECIES_ZEKROM, FLAG_HIDE_ZEKROM
	end

CaveOfOrigin_B1F_OnLoad:
	call_if_unset FLAG_WALLACE_GOES_TO_SKY_PILLAR, CaveOfOrigin_B1F_ShowWallace
	call CaveOfOrigin_B1F_ReshiramLoad
	call CaveOfOrigin_B1F_ZekromLoad
	end

@ Wallace is hidden only while a legendary is active; once he has left for the Sky Pillar he stays hidden
CaveOfOrigin_B1F_ShowWallace:
	clearflag FLAG_HIDE_CAVE_OF_ORIGIN_B1F_WALLACE
	return

CaveOfOrigin_B1F_ReshiramLoad:
	setvar VAR_0x8004, SPECIES_RESHIRAM
	setvar VAR_0x8005, LAYOUT_CAVE_OF_ORIGIN_B1F
	setvar VAR_0x8006, WEEKDAY_WED
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, CaveOfOrigin_B1F_HideReshiramArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_MORNING, CaveOfOrigin_B1F_HideReshiramArena
	checkpartytype TYPE_FIRE
	goto_if_eq VAR_RESULT, FALSE, CaveOfOrigin_B1F_HideReshiramArena
	goto CaveOfOrigin_B1F_ShowReshiramArena

CaveOfOrigin_B1F_Reshiram_Encounter::
	goto LegendaryEncounter_EventScript_Reshiram

CaveOfOrigin_B1F_HideReshiramArena:
	setflag FLAG_HIDE_RESHIRAM
	return

CaveOfOrigin_B1F_ShowReshiramArena:
	setflag FLAG_HIDE_CAVE_OF_ORIGIN_B1F_WALLACE
	setweather WEATHER_NONE
	@ TODO(user): tune opacity, pulse range and period
	overlaycreate RGB(31, 31, 31), 8, OVERLAY_LAYER_WORLD, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_0
	overlaypulse VAR_TEMP_0, 2, 6, 120

	setmetatilewithelevation 8, 5, 0x396, FALSE, 0
	setmetatilewithelevation 9, 5, 0x396, FALSE, 0
	setmetatilewithelevation 10, 5, 0x396, FALSE, 0

	setmetatilewithelevation 9, 6, 0x31B, FALSE, 2

	setmetatilewithelevation 9, 9, 0x31B, FALSE, 2

	return

CaveOfOrigin_B1F_ZekromLoad:
	setvar VAR_0x8004, SPECIES_ZEKROM
	setvar VAR_0x8005, LAYOUT_CAVE_OF_ORIGIN_B1F
	setvar VAR_0x8006, WEEKDAY_WED
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, CaveOfOrigin_B1F_HideZekromArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_EVENING, CaveOfOrigin_B1F_HideZekromArena
	checkpartytype TYPE_ELECTRIC
	goto_if_eq VAR_RESULT, FALSE, CaveOfOrigin_B1F_HideZekromArena
	goto CaveOfOrigin_B1F_ShowZekromArena

CaveOfOrigin_B1F_Zekrom_Encounter::
	goto LegendaryEncounter_EventScript_Zekrom

CaveOfOrigin_B1F_HideZekromArena:
	setflag FLAG_HIDE_ZEKROM
	return

CaveOfOrigin_B1F_ShowZekromArena:
	setflag FLAG_HIDE_CAVE_OF_ORIGIN_B1F_WALLACE
	setweather WEATHER_NONE
	@ TODO(user): tune opacity, pulse range and period
	overlaycreate RGB(0, 0, 0), 8, OVERLAY_LAYER_WORLD, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_0
	overlaypulse VAR_TEMP_0, 2, 6, 120

	setmetatilewithelevation 8, 5, 0x396, FALSE, 0
	setmetatilewithelevation 9, 5, 0x396, FALSE, 0
	setmetatilewithelevation 10, 5, 0x396, FALSE, 0

	setmetatilewithelevation 9, 6, 0x31B, FALSE, 2

	setmetatilewithelevation 9, 9, 0x31B, FALSE, 2
	return

CaveOfOrigin_B1F_EventScript_Wallace::
	lock
	faceplayer
	msgbox CaveOfOrigin_B1F_Text_WallaceStory, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	delay 60
	playse SE_PIN
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_Delay48
	waitmovement 0
	delay 30
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_FacePlayer
	waitmovement 0
	message CaveOfOrigin_B1F_Text_WhereIsRayquaza
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_WheresRayquaza::
	multichoice 0, 0, MULTI_WHERES_RAYQUAZA, FALSE
	switch VAR_RESULT
	case 0, CaveOfOrigin_B1F_EventScript_AtCaveOfOrigin
	case 1, CaveOfOrigin_B1F_EventScript_AtMtPyre
	case 2, CaveOfOrigin_B1F_EventScript_AtSkyPillar
	case 3, CaveOfOrigin_B1F_EventScript_DontRemember
	goto CaveOfOrigin_B1F_EventScript_DontRemember
	end

CaveOfOrigin_B1F_EventScript_AtCaveOfOrigin::
	message CaveOfOrigin_B1F_Text_ButWereInCaveOfOrigin
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_AtMtPyre::
	message CaveOfOrigin_B1F_Text_OldLadyDidntMentionThat
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_DontRemember::
	message CaveOfOrigin_B1F_Text_CantYouRememberSomehow
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_AtSkyPillar::
	msgbox CaveOfOrigin_B1F_Text_WellHeadToSkyPillar, MSGBOX_DEFAULT
	closemessage
	playse SE_EXIT
	fadescreenspeed FADE_TO_BLACK, 4
	setflag FLAG_WALLACE_GOES_TO_SKY_PILLAR
	setvar VAR_SOOTOPOLIS_CITY_STATE, 3
	removeobject LOCALID_CAVE_OF_ORIGIN_WALLACE
	clearflag FLAG_HIDE_SKY_PILLAR_WALLACE
	fadescreen FADE_FROM_BLACK
	release
	end

CaveOfOrigin_B1F_Text_WallaceStory:
	.string "Ah, so you are {PLAYER}{KUN}?\n"
	.string "I've heard tales of your exploits.\p"
	.string "My name is WALLACE.\p"
	.string "I was once the GYM LEADER of\n"
	.string "SOOTOPOLIS, but something came up.\p"
	.string "So now, I've entrusted my mentor JUAN\n"
	.string "with the GYM's operation.\p"
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "GROUDON and KYOGRE, the two POKéMON\n"
	.string "wreaking havoc here, are considered\l"
	.string "to be super-ancient POKéMON.\p"
	.string "But there aren't just two super-\n"
	.string "ancient POKéMON.\p"
	.string "There is one more somewhere.\p"
	.string "Somewhere, there is a super-\n"
	.string "ancient POKéMON named RAYQUAZA.\p"
	.string "It's said that it was RAYQUAZA that\n"
	.string "becalmed the two combatants in\l"
	.string "the distant past.\p"
	.string "But even I have no clue as to\n"
	.string "RAYQUAZA's whereabouts…$"

CaveOfOrigin_B1F_Text_WhereIsRayquaza:
	.string "WALLACE: {PLAYER}{KUN}, do you perhaps\n"
	.string "know where RAYQUAZA is now?\p"
	.string "If you do, please tell me.$"

CaveOfOrigin_B1F_Text_ButWereInCaveOfOrigin:
	.string "WALLACE: The CAVE OF ORIGIN?\p"
	.string "But that's right here!\n"
	.string "I need you to do better than that!\p"
	.string "Please, I need you to think about\n"
	.string "where RAYQUAZA might be right now.$"

CaveOfOrigin_B1F_Text_OldLadyDidntMentionThat:
	.string "WALLACE: MT. PYRE?\p"
	.string "But when I met the old lady there\n"
	.string "earlier, she made no mention of it.\p"
	.string "I very much doubt that the old lady\n"
	.string "would try to hide something from me…\p"
	.string "{PLAYER}{KUN}, could you think about this\n"
	.string "more carefully for me?$"

CaveOfOrigin_B1F_Text_CantYouRememberSomehow:
	.string "WALLACE: Huh? You don't remember?\n"
	.string "Hmm… That's a problem…\p"
	.string "Can't you remember somehow?$"

CaveOfOrigin_B1F_Text_WellHeadToSkyPillar:
	.string "WALLACE: The SKY PILLAR?\p"
	.string "That's it!\n"
	.string "It must be the SKY PILLAR!\p"
	.string "{PLAYER}{KUN}, there's not a moment to lose!\n"
	.string "We'll head to the SKY PILLAR right away!$"

