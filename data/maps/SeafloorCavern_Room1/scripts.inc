SeafloorCavern_Room1_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SeafloorCavern_Room1_OnLoad
	.byte 0

SeafloorCavern_Room1_OnLoad:
	@ Volcanion quakes: shake intensity, active frames, idle frames
	setvar VAR_0x8008, 4
	setvar VAR_0x8009, 60
	setvar VAR_0x800A, 1200
	call LegendaryEncounter_EventScript_VolcanionQuake
	end

SeafloorCavern_Room1_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_1, SeafloorCavern_Room1_Text_Grunt1Intro, SeafloorCavern_Room1_Text_Grunt1Defeat
	msgbox SeafloorCavern_Room1_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room1_EventScript_Grunt2::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_2, SeafloorCavern_Room1_Text_Grunt2Intro, SeafloorCavern_Room1_Text_Grunt2Defeat
	msgbox SeafloorCavern_Room1_Text_Grunt2PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room1_Text_Grunt1Intro:
	.string "We don't need a kid around!\n"
	.string "Go on home already!$"

SeafloorCavern_Room1_Text_Grunt1Defeat:
	.string "I want to go home…$"

SeafloorCavern_Room1_Text_Grunt1PostBattle:
	.string "I want to get a promotion so I can\n"
	.string "boss around the GRUNTS…$"

SeafloorCavern_Room1_Text_Grunt2Intro:
	.string "That submarine… It's tiny inside.\n"
	.string "I'm sore all over!$"

SeafloorCavern_Room1_Text_Grunt2Defeat:
	.string "Losing makes me sore!$"

SeafloorCavern_Room1_Text_Grunt2PostBattle:
	.string "That submarine we jacked, man,\n"
	.string "it's brutal as a ride.\l"
	.string "It's way too tight in there!$"

