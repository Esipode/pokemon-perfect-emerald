Route121_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route121_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route121_OnLoad
	map_script MAP_SCRIPT_ON_RESUME, Route121_OnResume
	.byte 0

Route121_OnTransition:
	updatelegendaryvisibility SPECIES_OGERPON, FLAG_HIDE_OGERPON
	legendaryhideobjects FLAG_HIDE_OGERPON, LOCALID_ROUTE121_OGERPON
	end

Route121_OnLoad:
	setvar VAR_0x8004, SPECIES_OGERPON
	setvar VAR_0x8005, LAYOUT_ROUTE121
	setvar VAR_0x8006, WEEKDAY_WED
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route121_HideOgerponArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_DAY, Route121_HideOgerponArena
	goto Route121_ShowOgerponArena

Route121_Ogerpon_Encounter::
	goto LegendaryEncounter_EventScript_Ogerpon

Route121_HideOgerponArena:
	setflag FLAG_HIDE_OGERPON
	end

Route121_ShowOgerponArena:
	end

Route121_OnResume:
	goto_if_set FLAG_HIDE_OGERPON, Route121_OnResume_End
	goto_if_ne VAR_TEMP_5, 0, Route121_OnResume_End
	overlaycreate RGB(6, 24, 8), 8, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlaypulse VAR_TEMP_5, 0, 16, 480, 0, OVERLAY_PULSE_SINE_QUARTER
	overlaycreate RGB(28, 6, 6), 8, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_6
	overlaypulse VAR_TEMP_6, 0, 16, 480, 360, OVERLAY_PULSE_SINE_QUARTER
	overlaycreate RGB(6, 12, 28), 8, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_7
	overlaypulse VAR_TEMP_7, 0, 16, 480, 240, OVERLAY_PULSE_SINE_QUARTER
	overlaycreate RGB(22, 12, 4), 8, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_8
	overlaypulse VAR_TEMP_8, 0, 16, 480, 120, OVERLAY_PULSE_SINE_QUARTER
	screenfxparticles VAR_TEMP_9, 10, SCREENFX_PARTICLE_LEAF, SCREENFX_COLOR_DEFAULT, 3, 0
	screenfxanchorpos VAR_TEMP_9, 68, 9
Route121_OnResume_End:
	end

Route121_EventScript_Woman::
	msgbox Route121_Text_AheadLoomsMtPyre, MSGBOX_NPC
	end

Route121_EventScript_MtPyrePierSign::
	msgbox Route121_Text_MtPyrePierSign, MSGBOX_SIGN
	end

Route121_EventScript_SafariZoneSign::
	msgbox Route121_Text_SafariZoneSign, MSGBOX_SIGN
	end

Route121_EventScript_AquaGruntsMoveOut::
	lockall
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	applymovement LOCALID_ROUTE121_GRUNT_2, Common_Movement_WalkInPlaceRight
	waitmovement 0
	msgbox Route121_Text_OkayMoveOutToMtPyre, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ROUTE121_GRUNT_1, Route121_Movement_Grunt1Exit
	applymovement LOCALID_ROUTE121_GRUNT_2, Route121_Movement_Grunt2Exit
	applymovement LOCALID_ROUTE121_GRUNT_3, Route121_Movement_Grunt3Exit
	waitmovement 0
	fadedefaultbgm
	removeobject LOCALID_ROUTE121_GRUNT_1
	removeobject LOCALID_ROUTE121_GRUNT_2
	removeobject LOCALID_ROUTE121_GRUNT_3
	setvar VAR_ROUTE121_STATE, 1
	releaseall
	end

Route121_Movement_Grunt1Exit:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

Route121_Movement_Grunt2Exit:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

Route121_Movement_Grunt3Exit:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

Route121_EventScript_Vanessa::
	trainerbattle_single TRAINER_VANESSA, Route121_Text_VanessaIntro, Route121_Text_VanessaDefeat
	msgbox Route121_Text_VanessaPostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Walter::
	trainerbattle_single TRAINER_WALTER_1, Route121_Text_WalterIntro, Route121_Text_WalterDefeat
	msgbox Route121_Text_WalterPostBattle, MSGBOX_DEFAULT
	release
	end

Route121_EventScript_Tammy::
	trainerbattle_single TRAINER_TAMMY, Route121_Text_TammyIntro, Route121_Text_TammyDefeat
	msgbox Route121_Text_TammyPostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Kate::
	trainerbattle_double TRAINER_KATE_AND_JOY, Route121_Text_KateIntro, Route121_Text_KateDefeat, Route121_Text_KateNotEnoughMons
	msgbox Route121_Text_KatePostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Joy::
	trainerbattle_double TRAINER_KATE_AND_JOY, Route121_Text_JoyIntro, Route121_Text_JoyDefeat, Route121_Text_JoyNotEnoughMons
	msgbox Route121_Text_JoyPostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Jessica::
	trainerbattle_single TRAINER_JESSICA_1, Route121_Text_JessicaIntro, Route121_Text_JessicaDefeat
	msgbox Route121_Text_JessicaPostBattle, MSGBOX_DEFAULT
	release
	end

Route121_EventScript_Cale::
	trainerbattle_single TRAINER_CALE, Route121_Text_CaleIntro, Route121_Text_CaleDefeat
	msgbox Route121_Text_CalePostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Myles::
	trainerbattle_single TRAINER_MYLES, Route121_Text_MylesIntro, Route121_Text_MylesDefeat
	msgbox Route121_Text_MylesPostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Pat::
	trainerbattle_single TRAINER_PAT, Route121_Text_PatIntro, Route121_Text_PatDefeat
	msgbox Route121_Text_PatPostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Marcel::
	trainerbattle_single TRAINER_MARCEL, Route121_Text_MarcelIntro, Route121_Text_MarcelDefeat
	msgbox Route121_Text_MarcelPostBattle, MSGBOX_AUTOCLOSE
	end

Route121_EventScript_Cristin::
	trainerbattle_single TRAINER_CRISTIN_1, Route121_Text_CristinIntro, Route121_Text_CristinDefeat
	msgbox Route121_Text_CristinPostBattle, MSGBOX_DEFAULT
	release
	end

Route121_Text_OkayMoveOutToMtPyre:
	.string "Okay!\n"
	.string "We're to move out to MT. PYRE!$"

Route121_Text_AheadLoomsMtPyre:
	.string "Ahead looms MT. PYRE…\p"
	.string "It is a natural monument to the spirits \n"
	.string "of departed POKéMON…$"

Route121_Text_MtPyrePierSign:
	.string "MT. PYRE PIER\p"
	.string "…The sign is old and worn out.\n"
	.string "The words are barely legible…$"

Route121_Text_SafariZoneSign:
	.string "“Filled with rare POKéMON!”\n"
	.string "SAFARI ZONE$"

