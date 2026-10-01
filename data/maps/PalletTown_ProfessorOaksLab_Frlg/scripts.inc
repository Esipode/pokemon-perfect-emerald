.equ SHOWED_OAK_COMPLETE_DEX, FLAG_TEMP_2

PalletTown_ProfessorOaksLab_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PalletTown_ProfessorOaksLab_OnTransition
	.byte 0

PalletTown_ProfessorOaksLab_OnTransition::
	setflag FLAG_VISITED_OAKS_LAB
	end

PalletTown_ProfessorOaksLab_EventScript_ProfOak::
	lock
	faceplayer
	goto_if_set SHOWED_OAK_COMPLETE_DEX, PalletTown_ProfessorOaksLab_EventScript_OakJustShownCompleteDex
	setvar VAR_0x8004, 0
	goto PalletTown_ProfessorOaksLab_EventScript_RatePokedex
	end

PalletTown_ProfessorOaksLab_EventScript_OakJustShownCompleteDex::
	msgbox PokedexRating_Text_ThankYouMadeDreamReality
	release
	end

PalletTown_ProfessorOaksLab_EventScript_RatePokedex::
	call PokedexRating_EventScript_RateInPerson
	goto_if_set FLAG_OAK_SAW_DEX_COMPLETION, PalletTown_ProfessorOaksLab_EventScript_DexCompleted
	release
	end

PalletTown_ProfessorOaksLab_EventScript_DexCompleted::
	closemessage
	delay 40
	message PokedexRating_Text_Wroooaaarrr
	waitmessage
	call_if_eq VAR_FACING, DIR_NORTH, PalletTown_ProfessorOaksLab_EventScript_OakExcitedNorth
	call_if_eq VAR_FACING, DIR_SOUTH, PalletTown_ProfessorOaksLab_EventScript_OakExcitedSouth
	call_if_eq VAR_FACING, DIR_EAST, PalletTown_ProfessorOaksLab_EventScript_OakExcitedEast
	call_if_eq VAR_FACING, DIR_WEST, PalletTown_ProfessorOaksLab_EventScript_OakExcitedWest
	applymovement LOCALID_OAKS_LAB_PROF_OAK, Common_Movement_FacePlayer
	waitmovement 0
	closemessage
	delay 70
	msgbox PokedexRating_Text_ThankYouMadeDreamReality
	setflag SHOWED_OAK_COMPLETE_DEX
	release
	end

PalletTown_ProfessorOaksLab_EventScript_OakExcitedNorth::
	applymovement LOCALID_OAKS_LAB_PROF_OAK, PalletTown_ProfessorOaksLab_Movement_OakExcitedNorth
	waitmovement 0
	return

PalletTown_ProfessorOaksLab_EventScript_OakExcitedSouth::
	applymovement LOCALID_OAKS_LAB_PROF_OAK, PalletTown_ProfessorOaksLab_Movement_OakExcitedSouth
	waitmovement 0
	return

PalletTown_ProfessorOaksLab_EventScript_OakExcitedEast::
	applymovement LOCALID_OAKS_LAB_PROF_OAK, PalletTown_ProfessorOaksLab_Movement_OakExcitedEast
	waitmovement 0
	return

PalletTown_ProfessorOaksLab_EventScript_OakExcitedWest::
	applymovement LOCALID_OAKS_LAB_PROF_OAK, PalletTown_ProfessorOaksLab_Movement_OakExcitedWest
	waitmovement 0
	return

PalletTown_ProfessorOaksLab_Movement_OakExcitedNorth::
	walk_in_place_down
	walk_in_place_fast_down
	walk_in_place_down
	walk_in_place_down
	walk_in_place_fast_down
	walk_in_place_down
	step_end

PalletTown_ProfessorOaksLab_Movement_OakExcitedSouth::
	walk_in_place_up
	walk_in_place_fast_up
	walk_in_place_up
	walk_in_place_up
	walk_in_place_fast_up
	walk_in_place_up
	step_end

PalletTown_ProfessorOaksLab_Movement_OakExcitedEast::
	walk_in_place_left
	walk_in_place_fast_left
	walk_in_place_left
	walk_in_place_left
	walk_in_place_fast_left
	walk_in_place_left
	step_end

PalletTown_ProfessorOaksLab_Movement_OakExcitedWest::
	walk_in_place_right
	walk_in_place_fast_right
	walk_in_place_right
	walk_in_place_right
	walk_in_place_fast_right
	walk_in_place_right
	step_end

@ Unused
PalletTown_ProfessorOaksLab_Movement_Search:
	walk_left
	walk_in_place_faster_down
	delay_8
	walk_in_place_faster_right
	delay_8
	walk_in_place_faster_up
	delay_8
	walk_in_place_faster_left
	delay_8
	walk_in_place_faster_down
	delay_8
	walk_right
	walk_right
	walk_in_place_faster_up
	delay_8
	walk_in_place_faster_left
	delay_8
	walk_in_place_faster_down
	delay_8
	walk_in_place_faster_right
	delay_8
	walk_in_place_faster_up
	delay_8
	walk_left
	step_end

@ Unused
PalletTown_ProfessorOaksLab_Movement_VerticalPacing:
	walk_up
	walk_down
	walk_down
	walk_in_place_down
	walk_up
	delay_16
	delay_16
	delay_16
	step_end

PalletTown_ProfessorOaksLab_EventScript_Aide1::
	lock
	faceplayer
	goto_if_set FLAG_KANTO_CHAMPION, PalletTown_ProfessorOaksLab_EventScript_Aide1GameClear
	msgbox PalletTown_ProfessorOaksLab_Text_StudyAsOaksAide
	release
	end

PalletTown_ProfessorOaksLab_EventScript_Aide1GameClear::
	msgbox PalletTown_ProfessorOaksLab_Text_OakIsGoingToHaveRadioShow
	release
	end

PalletTown_ProfessorOaksLab_EventScript_Aide2::
	lock
	faceplayer
	goto_if_set FLAG_KANTO_CHAMPION, PalletTown_ProfessorOaksLab_EventScript_Aide2GameClear
	msgbox PalletTown_ProfessorOaksLab_Text_StudyAsOaksAide
	release
	end

PalletTown_ProfessorOaksLab_EventScript_Aide2GameClear::
	msgbox PalletTown_ProfessorOaksLab_Text_DaisyWillGroomMons
	release
	end

PalletTown_ProfessorOaksLab_EventScript_Aide3::
	lock
	faceplayer
	msgbox PalletTown_ProfessorOaksLab_Text_OakIsAuthorityOnMons
	release
	end

PalletTown_ProfessorOaksLab_EventScript_Computer::
	msgbox PalletTown_ProfessorOaksLab_Text_EmailMessage, MSGBOX_SIGN
	end

PalletTown_ProfessorOaksLab_EventScript_LeftSign::
	msgbox PalletTown_ProfessorOaksLab_Text_PressStartToOpenMenu, MSGBOX_SIGN
	end

PalletTown_ProfessorOaksLab_EventScript_RightSign::
	msgbox PalletTown_ProfessorOaksLab_Text_AllMonTypesHaveStrongAndWeakPoints, MSGBOX_SIGN
	end
PalletTown_ProfessorOaksLab_Text_PressStartToOpenMenu::
	.string "Press START to open the MENU!$"

PalletTown_ProfessorOaksLab_Text_AllMonTypesHaveStrongAndWeakPoints::
	.string "All POKéMON types have strong and\n"
	.string "weak points against others.$"

PalletTown_ProfessorOaksLab_Text_EmailMessage::
	.string "There's an e-mail message here.\p"
	.string "…\p"
	.string "Finally!\n"
	.string "The ultimate TRAINERS of the\l"
	.string "POKéMON LEAGUE are ready to\l"
	.string "take on all comers!\p"
	.string "Bring your best POKéMON and see\n"
	.string "how you rate as a TRAINER!\p"
	.string "POKéMON LEAGUE HQ\n"
	.string "INDIGO PLATEAU\p"
	.string "PROF. OAK, please visit us!\n"
	.string "…$"

PalletTown_ProfessorOaksLab_Text_StudyAsOaksAide::
	.string "I study POKéMON as PROF. OAK's\n"
	.string "AIDE.$"

PalletTown_ProfessorOaksLab_Text_DaisyWillGroomMons::
	.string "Hi, {PLAYER}. I bet you've become\n"
	.string "good friends with your POKéMON.\p"
	.string "By the way, did you know about\n"
	.string "DAISY?\p"
	.string "If you show DAISY your POKéMON,\n"
	.string "she can tell how much it likes you.\p"
	.string "Occasionally, she will even groom\n"
	.string "a POKéMON for you.\p"
	.string "This is a secret, so please don't\n"
	.string "tell anyone.$"

PalletTown_ProfessorOaksLab_Text_OakIsGoingToHaveRadioShow::
	.string "PROF. OAK is going to have his own\n"
	.string "radio show soon.\p"
	.string "The program will be called PROF.\n"
	.string "OAK's POKéMON SEMINAR.$"

PalletTown_ProfessorOaksLab_Text_OakIsAuthorityOnMons::
	.string "PROF. OAK may not look like much,\n"
	.string "but he's the authority on POKéMON.\p"
	.string "Many POKéMON TRAINERS hold him in\n"
	.string "high regard.$"

