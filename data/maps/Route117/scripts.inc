.set LOCALID_DAYCARE_MON_0, 25
.set LOCALID_DAYCARE_MON_1, 26

Route117_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route117_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route117_OnLoad
	map_script MAP_SCRIPT_ON_RESUME, Route117_OnResume
	map_script MAP_SCRIPT_ON_STEP, Route117_OnStep
	.byte 0

Route117_OnTransition:
	call Route117_EventScript_TryMoveDayCareMan
	@ set flags for OW daycare mons
	setflag FLAG_TEMP_1
	setflag FLAG_TEMP_2
	updatelegendaryvisibility SPECIES_XERNEAS, FLAG_HIDE_XERNEAS
	getdaycaregfx VAR_OBJ_GFX_ID_0, VAR_OBJ_GFX_ID_1, VAR_TEMP_0, VAR_TEMP_1
	switch VAR_RESULT
	case 2, Route117_EventScript_ShowDaycareMon1
	case 1, Route117_EventScript_ShowDaycareMon0
	end

Route117_OnLoad:
	@ Xerneas spawn gate: post-game, not already owned, Thursday morning, no Poison-type in party
	setvar VAR_0x8004, SPECIES_XERNEAS
	setvar VAR_0x8005, LAYOUT_ROUTE117
	setvar VAR_0x8006, WEEKDAY_THU
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Route117_HideXerneasArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_MORNING, Route117_HideXerneasArena
	checkpartytype TYPE_POISON
	goto_if_eq VAR_RESULT, TRUE, Route117_HideXerneasArena
	goto Route117_ShowXerneasArena

Route117_Xerneas_Encounter::
	goto LegendaryEncounter_EventScript_Xerneas

Route117_HideXerneasArena:
	setflag FLAG_HIDE_XERNEAS
	end

Route117_ShowXerneasArena:
	@ Park every NPC off-map while Xerneas is active. ON_LOAD runs before objects spawn,
	@ so unspawned objects never appear; templates reload on the next map load.
	setobjectxyperm 1, 999, 999
	setobjectxyperm 2, 999, 999
	setobjectxyperm LOCALID_DAYCARE_MAN, 999, 999
	setobjectxyperm 4, 999, 999
	setobjectxyperm 5, 999, 999
	setobjectxyperm 6, 999, 999
	setobjectxyperm 7, 999, 999
	setobjectxyperm 8, 999, 999
	setobjectxyperm 9, 999, 999
	setobjectxyperm 10, 999, 999
	setobjectxyperm 17, 999, 999
	setobjectxyperm 18, 999, 999
	setobjectxyperm 19, 999, 999
	setobjectxyperm 20, 999, 999
	setobjectxyperm 21, 999, 999
	setobjectxyperm 22, 999, 999
	setobjectxyperm 23, 999, 999
	setobjectxyperm 24, 999, 999
	setobjectxyperm LOCALID_DAYCARE_MON_0, 999, 999
	setobjectxyperm LOCALID_DAYCARE_MON_1, 999, 999

	setmetatilewithelevation 39, 8, 0x129, FALSE, 3
	setmetatilewithelevation 40, 8, 0x129, FALSE, 3
	setmetatilewithelevation 41, 8, 0x129, FALSE, 3
	setmetatilewithelevation 42, 8, 0x129, FALSE, 3
	setmetatilewithelevation 43, 8, 0x129, FALSE, 3
	setmetatilewithelevation 44, 8, 0x129, FALSE, 3
	setmetatilewithelevation 45, 8, 0x129, FALSE, 3
	setmetatilewithelevation 46, 8, 0x129, FALSE, 3

	setmetatilewithelevation 38, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 39, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 40, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 41, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 42, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 43, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 44, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 45, 9, 0x20A, FALSE, 3
	setmetatilewithelevation 46, 9, 0x20A, FALSE, 3

	setmetatilewithelevation 37, 10, 0x20A, FALSE, 3
	setmetatilewithelevation 38, 10, 0x001, FALSE, 3
	setmetatilewithelevation 39, 10, 0x001, FALSE, 3
	setmetatilewithelevation 40, 10, 0x001, FALSE, 3
	setmetatilewithelevation 41, 10, 0x001, FALSE, 3
	setmetatilewithelevation 42, 10, 0x001, FALSE, 3
	setmetatilewithelevation 43, 10, 0x001, FALSE, 3
	setmetatilewithelevation 44, 10, 0x001, FALSE, 3
	setmetatilewithelevation 45, 10, 0x001, FALSE, 3
	setmetatilewithelevation 46, 10, 0x001, FALSE, 3
	setmetatilewithelevation 47, 10, 0x20A, FALSE, 3
	setmetatilewithelevation 48, 10, 0x20A, FALSE, 3

	setmetatilewithelevation 36, 11, 0x20A, FALSE, 3
	setmetatilewithelevation 38, 11, 0x001, FALSE, 3
	setmetatilewithelevation 39, 11, 0x001, FALSE, 3
	setmetatilewithelevation 40, 11, 0x001, FALSE, 3
	setmetatilewithelevation 41, 11, 0x001, FALSE, 3
	setmetatilewithelevation 42, 11, 0x001, FALSE, 3
	setmetatilewithelevation 43, 11, 0x001, FALSE, 3
	setmetatilewithelevation 44, 11, 0x001, FALSE, 3
	setmetatilewithelevation 45, 11, 0x001, FALSE, 3
	setmetatilewithelevation 46, 11, 0x001, FALSE, 3
	setmetatilewithelevation 47, 11, 0x001, FALSE, 3
	setmetatilewithelevation 49, 11, 0x20A, FALSE, 3

	setmetatilewithelevation 36, 12, 0x20A, FALSE, 3
	setmetatilewithelevation 38, 12, 0x001, FALSE, 3
	setmetatilewithelevation 39, 12, 0x001, FALSE, 3
	setmetatilewithelevation 40, 12, 0x001, FALSE, 3
	setmetatilewithelevation 41, 12, 0x001, FALSE, 3
	setmetatilewithelevation 42, 12, 0x001, FALSE, 3
	setmetatilewithelevation 43, 12, 0x001, FALSE, 3
	setmetatilewithelevation 44, 12, 0x001, FALSE, 3
	setmetatilewithelevation 45, 12, 0x001, FALSE, 3
	setmetatilewithelevation 46, 12, 0x001, FALSE, 3
	setmetatilewithelevation 47, 12, 0x001, FALSE, 3
	setmetatilewithelevation 49, 12, 0x20A, FALSE, 3

	setmetatilewithelevation 37, 13, 0x20A, FALSE, 3
	setmetatilewithelevation 38, 13, 0x001, FALSE, 3
	setmetatilewithelevation 39, 13, 0x001, FALSE, 3
	setmetatilewithelevation 45, 13, 0x001, FALSE, 3
	setmetatilewithelevation 46, 13, 0x001, FALSE, 3
	setmetatilewithelevation 48, 13, 0x20A, FALSE, 3

	setmetatilewithelevation 37, 14, 0x20A, FALSE, 3
	setmetatilewithelevation 38, 14, 0x001, FALSE, 3
	setmetatilewithelevation 39, 14, 0x001, FALSE, 3
	setmetatilewithelevation 45, 14, 0x001, FALSE, 3
	setmetatilewithelevation 46, 14, 0x001, FALSE, 3
	setmetatilewithelevation 48, 14, 0x20A, FALSE, 3

	setmetatilewithelevation 38, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 39, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 40, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 41, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 42, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 43, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 44, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 45, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 46, 15, 0x20A, FALSE, 3
	setmetatilewithelevation 47, 15, 0x20A, FALSE, 3

	end

Route117_OnResume:
	@ Flash overlay: VAR_TEMP_4 = handle, VAR_TEMP_5 = last flash colour (0 green, 1 pink).
	@ Created here because map-local overlays are destroyed after ON_LOAD; the destroy clears any stale handle.
	@ Intensity scales with distance to Xerneas; each step re-fires the flash (Route117_OnStep).
	goto_if_set FLAG_HIDE_XERNEAS, Route117_OnResumeEnd
	overlaydestroy VAR_TEMP_4
	overlaycreate RGB(8, 31, 12), 0, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_4
	overlayanchorobject VAR_TEMP_4, LOCALID_XERNEAS
	overlayfalloff VAR_TEMP_4, 2, 14, 0, 16
Route117_OnResumeEnd:
	end

Route117_OnStep:
	goto_if_set FLAG_HIDE_XERNEAS, Route117_OnStepEnd
	@ Skip the step while the last flash is still fading (15 frames = 250ms)
	overlayisfading VAR_TEMP_4
	goto_if_eq VAR_RESULT, TRUE, Route117_OnStepEnd
	goto_if_eq VAR_TEMP_5, 0, Route117_XerneasFlashPink
	setvar VAR_TEMP_5, 0
	overlaysetcolor VAR_TEMP_4, RGB(8, 31, 12)
	goto Route117_XerneasFlash

Route117_XerneasFlashPink:
	setvar VAR_TEMP_5, 1
	overlaysetcolor VAR_TEMP_4, RGB(31, 14, 24)
Route117_XerneasFlash:
	overlaysetalpha VAR_TEMP_4, 8
	overlayfade VAR_TEMP_4, 0, 30
Route117_OnStepEnd:
	end

Route117_EventScript_ShowDaycareMon1:
	@ set pokemon movement type based on compatibility
	special SetDaycareCompatibilityString
	call_if_eq VAR_RESULT, PARENTS_INCOMPATIBLE, Route117_EventScript_DaycareMonIncompatible
	@ Don't do anything if PARENTS_LOW_COMPATIBILITY
	call_if_eq VAR_RESULT, PARENTS_MED_COMPATIBILITY, Route117_EventScript_DaycareMonMedCompatibility
	call_if_eq VAR_RESULT, PARENTS_MAX_COMPATIBILITY, Route117_EventScript_DaycareMonMaxCompatibility
	clearflag FLAG_TEMP_2
Route117_EventScript_ShowDaycareMon0:
	clearflag FLAG_TEMP_1
	end

Route117_EventScript_DaycareMonIncompatible: @ mons never face each other
	setobjectmovementtype LOCALID_DAYCARE_MON_0, MOVEMENT_TYPE_FACE_DOWN_UP_AND_LEFT
	setobjectmovementtype LOCALID_DAYCARE_MON_1, MOVEMENT_TYPE_FACE_DOWN_UP_AND_RIGHT
	return

Route117_EventScript_DaycareMonMedCompatibility: @ down and towards each other
	setobjectmovementtype LOCALID_DAYCARE_MON_0, MOVEMENT_TYPE_FACE_DOWN_AND_RIGHT
	setobjectmovementtype LOCALID_DAYCARE_MON_1, MOVEMENT_TYPE_FACE_DOWN_AND_LEFT
	return

Route117_EventScript_DaycareMonMaxCompatibility: @ facing; walk in place
	setobjectmovementtype LOCALID_DAYCARE_MON_0, MOVEMENT_TYPE_WALK_IN_PLACE_RIGHT
	setobjectmovementtype LOCALID_DAYCARE_MON_1, MOVEMENT_TYPE_WALK_IN_PLACE_LEFT
	return

Route117_EventScript_TryMoveDayCareMan::
	goto_if_unset FLAG_PENDING_DAYCARE_EGG, Route117_EventScript_StopMoveDayCareMan
	setobjectxyperm LOCALID_DAYCARE_MAN, 47, 6
Route117_EventScript_StopMoveDayCareMan::
	return

Route117_EventScript_Woman::
	msgbox Route117_Text_ArentTheseFlowersPretty, MSGBOX_NPC
	end

Route117_EventScript_LittleBoy::
	msgbox Route117_Text_AirIsTastyHere, MSGBOX_NPC
	end

Route117_EventScript_Girl::
	msgbox Route117_Text_DayCarePokemonHadNewMove, MSGBOX_NPC
	end

Route117_EventScript_RouteSignVerdanturf::
	msgbox Route117_Text_RouteSignVerdanturf, MSGBOX_SIGN
	end

Route117_EventScript_RouteSignMauville::
	msgbox Route117_Text_RouteSignMauville, MSGBOX_SIGN
	end

Route117_EventScript_DayCareSign::
	msgbox Route117_Text_DayCareSign, MSGBOX_SIGN
	end

Route117_EventScript_Isaac::
	trainerbattle_single TRAINER_ISAAC_1, Route117_Text_IsaacIntro, Route117_Text_IsaacDefeat
	msgbox Route117_Text_IsaacPostBattle, MSGBOX_DEFAULT
	release
	end

Route117_EventScript_Lydia::
	trainerbattle_single TRAINER_LYDIA_1, Route117_Text_LydiaIntro, Route117_Text_LydiaDefeat
	msgbox Route117_Text_LydiaPostBattle, MSGBOX_DEFAULT
	release
	end

Route117_EventScript_Dylan::
	trainerbattle_single TRAINER_DYLAN_1, Route117_Text_DylanIntro, Route117_Text_DylanDefeat
	msgbox Route117_Text_DylanPostBattle, MSGBOX_DEFAULT
	release
	end

Route117_EventScript_Maria::
	trainerbattle_single TRAINER_MARIA_1, Route117_Text_MariaIntro, Route117_Text_MariaDefeat
	msgbox Route117_Text_MariaPostBattle, MSGBOX_DEFAULT
	release
	end

Route117_EventScript_Derek::
	trainerbattle_single TRAINER_DEREK, Route117_Text_DerekIntro, Route117_Text_DerekDefeat
	msgbox Route117_Text_DerekPostBattle, MSGBOX_AUTOCLOSE
	end

Route117_EventScript_Anna::
	trainerbattle_double TRAINER_ANNA_AND_MEG_1, Route117_Text_AnnaIntro, Route117_Text_AnnaDefeat, Route117_Text_AnnaNotEnoughMons
	msgbox Route117_Text_AnnaPostBattle, MSGBOX_DEFAULT
	release
	end

Route117_EventScript_Meg::
	trainerbattle_double TRAINER_ANNA_AND_MEG_1, Route117_Text_MegIntro, Route117_Text_MegDefeat, Route117_Text_MegNotEnoughMons
	msgbox Route117_Text_MegPostBattle, MSGBOX_DEFAULT
	release
	end

Route117_EventScript_Melina::
	trainerbattle_single TRAINER_MELINA, Route117_Text_MelinaIntro, Route117_Text_MelinaDefeat
	msgbox Route117_Text_MelinaPostBattle, MSGBOX_AUTOCLOSE
	end

Route117_EventScript_Brandi::
	trainerbattle_single TRAINER_BRANDI, Route117_Text_BrandiIntro, Route117_Text_BrandiDefeat
	msgbox Route117_Text_BrandiPostBattle, MSGBOX_AUTOCLOSE
	end

Route117_EventScript_Aisha::
	trainerbattle_single TRAINER_AISHA, Route117_Text_AishaIntro, Route117_Text_AishaDefeat
	msgbox Route117_Text_AishaPostBattle, MSGBOX_AUTOCLOSE
	end

Route117_Text_DayCarePokemonHadNewMove:
	.string "I left my POKéMON at the DAY CARE.\p"
	.string "When I got it back, it had a new move\n"
	.string "that I didn't teach it.\l"
	.string "I was really, really surprised.$"

Route117_Text_ArentTheseFlowersPretty:
	.string "What do you think?\n"
	.string "Aren't these flowers pretty?\p"
	.string "I planted them all!$"

Route117_Text_AirIsTastyHere:
	.string "The air is tasty here!$"

Route117_Text_RouteSignVerdanturf:
	.string "ROUTE 117\n"
	.string "{LEFT_ARROW} VERDANTURF TOWN$"

Route117_Text_RouteSignMauville:
	.string "ROUTE 117\n"
	.string "{RIGHT_ARROW} MAUVILLE CITY$"

Route117_Text_DayCareSign:
	.string "POKéMON DAY CARE\n"
	.string "“Let us raise your POKéMON.”$"
