AbandonedShip_HiddenFloorRooms_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, AbandonedShip_HiddenFloorRooms_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, AbandonedShip_HiddenFloorRooms_OnLoad
	map_script MAP_SCRIPT_ON_FRAME_TABLE, AbandonedShip_HiddenFloorRooms_OnFrame
	.byte 0

AbandonedShip_HiddenFloorRooms_OnTransition:
	updatelegendaryvisibility SPECIES_DARKRAI, FLAG_HIDE_DARKRAI, TRUE
	end

AbandonedShip_HiddenFloorRooms_OnLoad:
	call AbandonedShip_HiddenFloor_EventScript_CheckDarkraiHunt
	goto_if_eq VAR_RESULT, FALSE, AbandonedShip_HiddenFloorRooms_HideDarkraiArena
	goto AbandonedShip_HiddenFloorRooms_ShowDarkraiArena

AbandonedShip_HiddenFloor_EventScript_CheckDarkraiHunt::
	setvar VAR_0x8004, SPECIES_DARKRAI
	setvar VAR_0x8005, LAYOUT_ABANDONED_SHIP_HIDDEN_FLOOR_ROOMS
	setvar VAR_0x8006, WEEKDAY_THU
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, AbandonedShip_HiddenFloor_DarkraiHuntChecked
	gettimeofday
	setvar VAR_RESULT, FALSE
	goto_if_ne VAR_0x8000, TIME_NIGHT, AbandonedShip_HiddenFloor_DarkraiHuntChecked
	setvar VAR_RESULT, TRUE
AbandonedShip_HiddenFloor_DarkraiHuntChecked:
	return

@ Shared by both hidden floor maps. Handles: VAR_TEMP_5 overlay, VAR_TEMP_6 vignette, VAR_TEMP_7 tear, VAR_TEMP_8 wave.
@ The tear band is fixed at screen line 48 (0-159).
AbandonedShip_HiddenFloor_EventScript_StartDarkraiAtmosphere::
	overlaycreate RGB(0, 0, 0), 14, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlaypulse VAR_TEMP_5, 14, 16, 40
	screenfxvignette VAR_TEMP_6, 12, SCREENFX_VIGNETTE_MEDIUM, 0
	screenfxtear VAR_TEMP_7, 8, 0, 48, 0
	screenfxwave VAR_TEMP_8, 8, 90, 0, 0
	return

@ VAR_DARKRAI_HUNT_STAGE counts relocations done. Darkrai relocates on interaction until stage 5, then battles.
AbandonedShip_HiddenFloorRooms_Darkrai_Encounter::
	goto_if_lt VAR_DARKRAI_HUNT_STAGE, 5, AbandonedShip_HiddenFloorRooms_DarkraiRelocate
	goto LegendaryEncounter_EventScript_Darkrai

@ Despawns Darkrai; the next map load places it in the room for the new stage.
AbandonedShip_HiddenFloorRooms_DarkraiRelocate:
	lock
	faceplayer
	waitse
	playmoncry SPECIES_DARKRAI, CRY_MODE_NORMAL
	waitmoncry
	addvar VAR_DARKRAI_HUNT_STAGE, 1
	removeobject LOCALID_ABANDONED_SHIP_DARKRAI
	releaseall
	end

AbandonedShip_HiddenFloorRooms_HideDarkraiArena:
	setflag FLAG_HIDE_DARKRAI
	setvar VAR_DARKRAI_HUNT_STAGE, 0
	end

AbandonedShip_HiddenFloorRooms_ShowDarkraiArena:
	call AbandonedShip_HiddenFloor_EventScript_StartDarkraiAtmosphere
	setflag FLAG_TEMP_DARKRAI_HUNT
	setflag FLAG_TEMP_HIDDEN_ITEMS_DISABLED
	@ Non-zero VAR_TEMP_1 skips the key sparkle hints in OnFrame
	setvar VAR_TEMP_1, 1
	@ Item balls (localIds 1-4) moved out of spawn range; unlike removeobject this leaves their collected flags alone
	setobjectxyperm 1, 1000, 1000
	setobjectxyperm 2, 1000, 1000
	setobjectxyperm 3, 1000, 1000
	setobjectxyperm 4, 1000, 1000
	switch VAR_DARKRAI_HUNT_STAGE
	case 0, AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm1
	case 1, AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm2
	case 2, AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm3
	case 3, AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm4
	case 4, AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm5
	case 5, AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm6
	end

AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm1:
	setobjectxyperm LOCALID_ABANDONED_SHIP_DARKRAI, 6, 4
	end

AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm2:
	setobjectxyperm LOCALID_ABANDONED_SHIP_DARKRAI, 21, 12
	end

AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm3:
	setobjectxyperm LOCALID_ABANDONED_SHIP_DARKRAI, 21, 4
	end

AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm4:
	setobjectxyperm LOCALID_ABANDONED_SHIP_DARKRAI, 6, 12
	end

AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm5:
	setobjectxyperm LOCALID_ABANDONED_SHIP_DARKRAI, 36, 4
	end

AbandonedShip_HiddenFloorRooms_PlaceDarkraiRm6:
	setobjectxyperm LOCALID_ABANDONED_SHIP_DARKRAI, 32, 12
	end

AbandonedShip_HiddenFloorRooms_OnFrame:
	map_script_2 VAR_TEMP_1, 0, AbandonedShip_HiddenFloorRooms_EventScript_DoHiddenItemSparkle
	.2byte 0

@ After the below calculation, VAR_TEMP_4 is the room number of the door the player entered
@ Bottom row, left column   (Rm 1)
@ Bottom row, middle column (Rm 2)
@ Bottom row, right column	(Rm 3)
@ Upper row,  left column   (Rm 4)
@ Upper row,  middle column (Rm 5)
@ Upper row,  right column  (Rm 6)
AbandonedShip_HiddenFloorRooms_EventScript_DoHiddenItemSparkle::
	setvar VAR_TEMP_1, 1
	getplayerxy VAR_TEMP_2, VAR_TEMP_3
	setvar VAR_TEMP_4, 1
	call_if_eq VAR_TEMP_2, 21, AbandonedShip_HiddenFloorRooms_EventScript_InMiddleRoomColumn
	call_if_eq VAR_TEMP_2, 36, AbandonedShip_HiddenFloorRooms_EventScript_InRightRoomColumn
	call_if_eq VAR_TEMP_3, 2, AbandonedShip_HiddenFloorRooms_EventScript_InUpperRoomRow
	switch VAR_TEMP_4
	case 1, AbandonedShip_HiddenFloorRooms_EventScript_EnterRm1
	case 2, AbandonedShip_HiddenFloorRooms_EventScript_EnterRm2
	case 3, AbandonedShip_HiddenFloorRooms_EventScript_EnterRm3
	case 4, AbandonedShip_HiddenFloorRooms_EventScript_EnterRm4
	case 5, AbandonedShip_HiddenFloorRooms_EventScript_EnterRm5
	case 6, AbandonedShip_HiddenFloorRooms_EventScript_EnterRm6
	end

AbandonedShip_HiddenFloorRooms_EventScript_InMiddleRoomColumn::
	addvar VAR_TEMP_4, 1
	return

AbandonedShip_HiddenFloorRooms_EventScript_InRightRoomColumn::
	addvar VAR_TEMP_4, 2
	return

AbandonedShip_HiddenFloorRooms_EventScript_InUpperRoomRow::
	addvar VAR_TEMP_4, 3
	return

AbandonedShip_HiddenFloorRooms_EventScript_EnterRm1::
	delay 20
	dofieldeffectsparkle 10, 10, 0
	specialvar VAR_RESULT, FoundAbandonedShipRoom4Key
	call_if_eq VAR_RESULT, FALSE, AbandonedShip_HiddenFloorRooms_EventScript_Rm4KeySparkle
	waitfieldeffect FLDEFF_SPARKLE
	delay 10
	end

AbandonedShip_HiddenFloorRooms_EventScript_EnterRm2::
	end

AbandonedShip_HiddenFloorRooms_EventScript_EnterRm3::
	specialvar VAR_RESULT, FoundAbandonedShipRoom1Key
	goto_if_eq VAR_RESULT, TRUE, AbandonedShip_HiddenFloorRooms_EventScript_Rm3NoSparkle
	delay 20
	call_if_eq VAR_RESULT, FALSE, AbandonedShip_HiddenFloorRooms_EventScript_Rm1KeySparkle
	waitfieldeffect FLDEFF_SPARKLE
	delay 10
	end

AbandonedShip_HiddenFloorRooms_EventScript_Rm3NoSparkle::
	end

AbandonedShip_HiddenFloorRooms_EventScript_EnterRm4::
	delay 20
	dofieldeffectsparkle 8, 5, 0
	dofieldeffectsparkle 11, 3, 0
	specialvar VAR_RESULT, FoundAbandonedShipRoom6Key
	call_if_eq VAR_RESULT, FALSE, AbandonedShip_HiddenFloorRooms_EventScript_Rm6KeySparkle
	waitfieldeffect FLDEFF_SPARKLE
	delay 10
	end

AbandonedShip_HiddenFloorRooms_EventScript_EnterRm5::
	delay 20
	dofieldeffectsparkle 16, 3, 0
	dofieldeffectsparkle 25, 2, 0
	dofieldeffectsparkle 24, 6, 0
	specialvar VAR_RESULT, FoundAbandonedShipRoom2Key
	call_if_eq VAR_RESULT, FALSE, AbandonedShip_HiddenFloorRooms_EventScript_Rm2KeySparkle
	waitfieldeffect FLDEFF_SPARKLE
	delay 10
	end

AbandonedShip_HiddenFloorRooms_EventScript_EnterRm6::
	end

AbandonedShip_HiddenFloorRooms_EventScript_Rm1KeySparkle::
	dofieldeffectsparkle 42, 10, 0
	return

AbandonedShip_HiddenFloorRooms_EventScript_Rm2KeySparkle::
	dofieldeffectsparkle 20, 5, 0
	return

AbandonedShip_HiddenFloorRooms_EventScript_Rm4KeySparkle::
	dofieldeffectsparkle 1, 12, 0
	return

AbandonedShip_HiddenFloorRooms_EventScript_Rm6KeySparkle::
	dofieldeffectsparkle 1, 2, 0
	return

AbandonedShip_HiddenFloorRooms_EventScript_Trash::
	goto_if_set FLAG_TEMP_DARKRAI_HUNT, AbandonedShip_HiddenFloorRooms_EventScript_TrashDisabled
	lockall
	msgbox AbandonedShip_HiddenFloorRooms_Text_BrightShinyTrash, MSGBOX_DEFAULT
	releaseall
	end

AbandonedShip_HiddenFloorRooms_EventScript_TrashDisabled::
	end

AbandonedShip_HiddenFloorRooms_Text_BrightShinyTrash:
	.string "It's bright and shiny!\n"
	.string "But it's just trash…$"

