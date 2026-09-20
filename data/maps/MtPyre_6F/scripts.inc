MtPyre_6F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtPyre_6F_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, MtPyre_6F_OnLoad
	.byte 0

MtPyre_6F_OnTransition:
	updatelegendaryvisibility SPECIES_GIRATINA, FLAG_HIDE_GIRATINA
	end

MtPyre_6F_OnLoad:
	@ Giratina spawn gate: post-game, not already owned, Friday, night
	setvar VAR_0x8004, SPECIES_GIRATINA
	setvar VAR_0x8005, LAYOUT_MT_PYRE_6F
	setvar VAR_0x8006, WEEKDAY_SUN
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, MtPyre_6F_HideGiratinaArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_NIGHT, MtPyre_6F_HideGiratinaArena
	goto MtPyre_6F_ShowGiratinaArena

MtPyre_6F_Giratina_Encounter::
	overlaydestroy VAR_TEMP_5
	goto LegendaryEncounter_EventScript_Giratina

MtPyre_6F_HideGiratinaArena:
	setflag FLAG_HIDE_GIRATINA
	clearflag FLAG_HIDE_MT_PYRE_6F_NPCS
	end

MtPyre_6F_ShowGiratinaArena:
	setflag FLAG_HIDE_MT_PYRE_6F_NPCS
	@ Item ball moved out of spawn range; unlike removeobject this leaves its collected flag alone
	setobjectxyperm LOCALID_MT_PYRE_6F_ITEM_BALL, 1000, 1000
	@ World layer only: BG palettes go black, object palettes (player, Giratina) stay lit
	overlaycreate RGB(0, 0, 0), 16, OVERLAY_LAYER_WORLD, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	end

MtPyre_6F_EventScript_Valerie::
	trainerbattle_single TRAINER_VALERIE_1, MtPyre_6F_Text_ValerieIntro, MtPyre_6F_Text_ValerieDefeat
	msgbox MtPyre_6F_Text_ValeriePostBattle, MSGBOX_DEFAULT
	release
	end

MtPyre_6F_EventScript_Cedric::
	trainerbattle_single TRAINER_CEDRIC, MtPyre_6F_Text_CedricIntro, MtPyre_6F_Text_CedricDefeat
	msgbox MtPyre_6F_Text_CedricPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_6F_Text_ValerieIntro:
	.string "When I'm here…\n"
	.string "A curious power flows into me…$"

MtPyre_6F_Text_ValerieDefeat:
	.string "The power is ebbing away…$"

MtPyre_6F_Text_ValeriePostBattle:
	.string "Perhaps the power is from the spirits\n"
	.string "of POKéMON in fitful sleep here…$"

MtPyre_6F_Text_CedricIntro:
	.string "Have you lost your bearings?\n"
	.string "Have no fear for I am here!$"

MtPyre_6F_Text_CedricDefeat:
	.string "Weren't you lost?$"

MtPyre_6F_Text_CedricPostBattle:
	.string "I had this feeling that a lost TRAINER\n"
	.string "would be panicked and easy to beat.\p"
	.string "It's dirty and I won't try it again…$"

