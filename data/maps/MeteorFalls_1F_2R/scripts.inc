MeteorFalls_1F_2R_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MeteorFalls_1F_2R_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, MeteorFalls_1F_2R_OnLoad
	.byte 0

MeteorFalls_1F_2R_OnTransition:
	updatelegendaryvisibility SPECIES_PALKIA, FLAG_HIDE_PALKIA
	end

MeteorFalls_1F_2R_OnLoad:
	@ Palkia spawn gate: post-game, not already owned
	setvar VAR_0x8004, SPECIES_PALKIA
	setvar VAR_0x8005, LAYOUT_METEOR_FALLS_1F_2R
	setvar VAR_0x8006, WEEKDAY_SUN
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, MeteorFalls_1F_2R_HidePalkiaArena
	goto MeteorFalls_1F_2R_ShowPalkiaArena

MeteorFalls_1F_2R_Palkia_Encounter::
	goto LegendaryEncounter_EventScript_Palkia

MeteorFalls_1F_2R_HidePalkiaArena:
	setflag FLAG_HIDE_PALKIA
	clearflag FLAG_HIDE_METEOR_FALLS_1F_2R_NPCS
	end

MeteorFalls_1F_2R_ShowPalkiaArena:
	setflag FLAG_HIDE_METEOR_FALLS_1F_2R_NPCS
	overlaycreate RGB(28, 10, 20), 5, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	@ Violent quake in bursts: 50 frames decaying to rest, then 300 frames of calm
	screenfxshake VAR_TEMP_8, 16, 24, SCREENFX_AXIS_X | SCREENFX_AXIS_Y, 0
	screenfxpulse VAR_TEMP_8, 50, 300

	setmetatilewithelevation 13, 12, 0x201, FALSE, 3
	setmetatilewithelevation 14, 12, 0x201, FALSE, 3
	setmetatilewithelevation 15, 12, 0x201, FALSE, 3
	setmetatilewithelevation 16, 12, 0x201, FALSE, 3
	setmetatilewithelevation 17, 12, 0x201, FALSE, 3
	setmetatilewithelevation 18, 12, 0x201, FALSE, 3
	setmetatilewithelevation 19, 12, 0x201, FALSE, 3
	setmetatilewithelevation 20, 12, 0x201, FALSE, 3
	setmetatilewithelevation 21, 12, 0x201, FALSE, 3
	setmetatilewithelevation 22, 12, 0x201, FALSE, 3
	setmetatilewithelevation 23, 12, 0x201, FALSE, 3

	setmetatilewithelevation 13, 13, 0x201, FALSE, 3
	setmetatilewithelevation 14, 13, 0x201, FALSE, 3
	setmetatilewithelevation 15, 13, 0x201, FALSE, 3
	setmetatilewithelevation 16, 13, 0x201, FALSE, 3
	setmetatilewithelevation 17, 13, 0x201, FALSE, 3
	setmetatilewithelevation 18, 13, 0x201, FALSE, 3
	setmetatilewithelevation 19, 13, 0x201, FALSE, 3
	setmetatilewithelevation 20, 13, 0x201, FALSE, 3
	setmetatilewithelevation 21, 13, 0x201, FALSE, 3
	setmetatilewithelevation 22, 13, 0x201, FALSE, 3
	setmetatilewithelevation 23, 13, 0x201, FALSE, 3

	setmetatilewithelevation 11, 14, 0x201, FALSE, 3
	setmetatilewithelevation 12, 14, 0x201, FALSE, 3
	setmetatilewithelevation 13, 14, 0x201, FALSE, 3
	setmetatilewithelevation 14, 14, 0x201, FALSE, 3
	setmetatilewithelevation 15, 14, 0x201, FALSE, 3
	setmetatilewithelevation 16, 14, 0x201, FALSE, 3
	setmetatilewithelevation 17, 14, 0x201, FALSE, 3
	setmetatilewithelevation 18, 14, 0x201, FALSE, 3
	setmetatilewithelevation 19, 14, 0x201, FALSE, 3
	setmetatilewithelevation 20, 14, 0x201, FALSE, 3
	setmetatilewithelevation 21, 14, 0x201, FALSE, 3
	setmetatilewithelevation 22, 14, 0x201, FALSE, 3
	setmetatilewithelevation 23, 14, 0x201, FALSE, 3

	setmetatilewithelevation 11, 15, 0x201, FALSE, 3
	setmetatilewithelevation 12, 15, 0x201, FALSE, 3
	setmetatilewithelevation 13, 15, 0x201, FALSE, 3
	setmetatilewithelevation 14, 15, 0x201, FALSE, 3
	setmetatilewithelevation 15, 15, 0x201, FALSE, 3
	setmetatilewithelevation 16, 15, 0x201, FALSE, 3
	setmetatilewithelevation 17, 15, 0x201, FALSE, 3
	setmetatilewithelevation 18, 15, 0x201, FALSE, 3
	
	setmetatilewithelevation 15, 16, 0x201, FALSE, 3
	setmetatilewithelevation 16, 16, 0x201, FALSE, 3
	setmetatilewithelevation 17, 16, 0x201, FALSE, 3
	setmetatilewithelevation 18, 16, 0x201, FALSE, 3
	setmetatilewithelevation 19, 16, 0x201, FALSE, 3
	setmetatilewithelevation 20, 16, 0x201, FALSE, 3
	setmetatilewithelevation 21, 16, 0x201, FALSE, 3
	setmetatilewithelevation 22, 16, 0x201, FALSE, 3
	setmetatilewithelevation 23, 16, 0x201, FALSE, 3
	setmetatilewithelevation 24, 16, 0x201, FALSE, 3
	setmetatilewithelevation 25, 16, 0x201, FALSE, 3
	
	setmetatilewithelevation 12, 17, 0x201, FALSE, 3
	setmetatilewithelevation 13, 17, 0x201, FALSE, 3
	setmetatilewithelevation 14, 17, 0x201, FALSE, 3
	setmetatilewithelevation 15, 17, 0x201, FALSE, 3
	setmetatilewithelevation 16, 17, 0x201, FALSE, 3
	setmetatilewithelevation 17, 17, 0x201, FALSE, 3
	setmetatilewithelevation 18, 17, 0x201, FALSE, 3
	setmetatilewithelevation 19, 17, 0x201, FALSE, 3
	setmetatilewithelevation 20, 17, 0x201, FALSE, 3
	setmetatilewithelevation 21, 17, 0x201, FALSE, 3
	setmetatilewithelevation 22, 17, 0x201, FALSE, 3
	setmetatilewithelevation 23, 17, 0x201, FALSE, 3
	setmetatilewithelevation 24, 17, 0x201, FALSE, 3
	setmetatilewithelevation 25, 17, 0x201, FALSE, 3

	setmetatilewithelevation 10, 18, 0x201, FALSE, 3
	setmetatilewithelevation 11, 18, 0x201, FALSE, 3
	setmetatilewithelevation 12, 18, 0x201, FALSE, 3
	setmetatilewithelevation 13, 18, 0x201, FALSE, 3
	setmetatilewithelevation 14, 18, 0x201, FALSE, 3
	setmetatilewithelevation 15, 18, 0x201, FALSE, 3
	setmetatilewithelevation 16, 18, 0x201, FALSE, 3
	setmetatilewithelevation 17, 18, 0x201, FALSE, 3
	setmetatilewithelevation 18, 18, 0x201, FALSE, 3

	setmetatilewithelevation 10, 19, 0x201, FALSE, 3
	setmetatilewithelevation 11, 19, 0x201, FALSE, 3
	setmetatilewithelevation 16, 19, 0x201, FALSE, 3

	setmetatilewithelevation 10, 20, 0x201, FALSE, 3
	setmetatilewithelevation 11, 20, 0x201, FALSE, 3
	setmetatilewithelevation 19, 20, 0x201, FALSE, 3
	end

MeteorFalls_1F_2R_EventScript_Nicolas::
	trainerbattle_single TRAINER_NICOLAS_1, MeteorFalls_1F_2R_Text_NicolasIntro, MeteorFalls_1F_2R_Text_NicolasDefeat
	msgbox MeteorFalls_1F_2R_Text_NicolasPostBattle, MSGBOX_DEFAULT
	release
	end

MeteorFalls_1F_2R_EventScript_John::
	trainerbattle_double TRAINER_JOHN_AND_JAY_1, MeteorFalls_1F_2R_Text_JohnIntro, MeteorFalls_1F_2R_Text_JohnDefeat, MeteorFalls_1F_2R_Text_JohnNotEnoughMons
	msgbox MeteorFalls_1F_2R_Text_JohnPostBattle, MSGBOX_DEFAULT
	release
	end

MeteorFalls_1F_2R_EventScript_Jay::
	trainerbattle_double TRAINER_JOHN_AND_JAY_1, MeteorFalls_1F_2R_Text_JayIntro, MeteorFalls_1F_2R_Text_JayDefeat, MeteorFalls_1F_2R_Text_JayNotEnoughMons
	msgbox MeteorFalls_1F_2R_Text_JayPostBattle, MSGBOX_DEFAULT
	release
	end

MeteorFalls_1F_2R_Text_NicolasIntro:
	.string "This is where we DRAGON users do our\n"
	.string "training.\p"
	.string "The CHAMPION even visits.\n"
	.string "Now do you see how special it is here?$"

MeteorFalls_1F_2R_Text_NicolasDefeat:
	.string "Urgh!\n"
	.string "I didn't expect you to be so strong!$"

MeteorFalls_1F_2R_Text_NicolasPostBattle:
	.string "The road ahead remains long and harsh.\p"
	.string "When will my POKéMON and I become\n"
	.string "the best?$"

MeteorFalls_1F_2R_Text_JohnIntro:
	.string "JOHN: We've always battled POKéMON\n"
	.string "together as a twosome.\l"
	.string "We've confidence in ourselves.$"

MeteorFalls_1F_2R_Text_JohnDefeat:
	.string "JOHN: Oh, my.\n"
	.string "We've lost, dear wife.$"

MeteorFalls_1F_2R_Text_JohnPostBattle:
	.string "JOHN: We've been married for\n"
	.string "fifty years.\p"
	.string "Come to think of it, I've yet to beat\n"
	.string "my dear wife in a battle.$"

MeteorFalls_1F_2R_Text_JohnNotEnoughMons:
	.string "JOHN: Well, well, what a young TRAINER!\p"
	.string "Will you battle with us? If so, you'll\n"
	.string "have to return with more POKéMON.$"

MeteorFalls_1F_2R_Text_JayIntro:
	.string "JAY: We've been married for\n"
	.string "fifty years.\p"
	.string "The bond we share as a couple could\n"
	.string "never be broken.$"

MeteorFalls_1F_2R_Text_JayDefeat:
	.string "JAY: Oh, dear.\n"
	.string "We've lost, my dear husband.$"

MeteorFalls_1F_2R_Text_JayPostBattle:
	.string "JAY: Fifty years of marriage…\p"
	.string "If we ever argued, we always settled\n"
	.string "it with a POKéMON battle…$"

MeteorFalls_1F_2R_Text_JayNotEnoughMons:
	.string "JAY: Well, well, aren't you a young\n"
	.string "TRAINER?\p"
	.string "If you'd care to battle with us, you'll\n"
	.string "have to come back with more POKéMON.$"

