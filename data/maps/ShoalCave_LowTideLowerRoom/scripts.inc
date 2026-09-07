ShoalCave_LowTideLowerRoom_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, ShoalCave_LowTideLowerRoom_OnLoad
	.byte 0

ShoalCave_LowTideLowerRoom_OnLoad:
	// Check if legendary is available
    setvar VAR_0x8004, SPECIES_ARTICUNO
    setvar VAR_0x8005, LAYOUT_SHOAL_CAVE_LOW_TIDE_ICE_ROOM_LAYOUT_ARTICUNO
    setvar VAR_0x8006, 1 @ Monday
    setvar VAR_0x8007, 1
    special TrySetLegendaryTile
	goto_if_eq VAR_RESULT, TRUE, ShoalCave_LowTideLowerRoom_HiddenLadder
	// Keep warp open if returning from legendary map
    setvar VAR_0x8004, MAP_GROUP(MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM_ARTICUNO)
    setvar VAR_0x8005, MAP_NUM(MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM_ARTICUNO)
    setvar VAR_0x8006, 0 // Warp ID
	special IsLastUsedWarp
	goto_if_eq VAR_RESULT, TRUE, ShoalCave_LowTideLowerRoom_HiddenLadder
	call ShoalCave_LowTideLowerRoom_EventScript_SetShoalItemMetatiles
	end

ShoalCave_LowTideLowerRoom_HiddenLadder:
	setmetatile 14, 2, 0x205, FALSE
	return

ShoalCave_LowTideLowerRoom_EventScript_SetShoalItemMetatiles::
	goto_if_set FLAG_RECEIVED_SHOAL_SALT_4, ShoalCave_LowTideLowerRoom_EventScript_SetShoalItemMetatilesEnd
	setmetatile 18, 2, METATILE_Cave_ShoalCave_DirtPile_Large, TRUE
	return

ShoalCave_LowTideLowerRoom_EventScript_SetShoalItemMetatilesEnd::
	return

ShoalCave_LowTideLowerRoom_EventScript_ShoalSalt4::
	lockall
	goto_if_set FLAG_RECEIVED_SHOAL_SALT_4, ShoalCave_LowTideLowerRoom_EventScript_ReceivedShoalSalt
	giveitem ITEM_SHOAL_SALT
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setmetatile 18, 2, METATILE_Cave_ShoalCave_DirtPile_Small, FALSE
	special DrawWholeMapView
	setflag FLAG_RECEIVED_SHOAL_SALT_4
	releaseall
	end

ShoalCave_LowTideLowerRoom_EventScript_ReceivedShoalSalt::
	msgbox ShoalCave_Text_WasShoalSaltNowNothing, MSGBOX_DEFAULT
	releaseall
	end

ShoalCave_LowTideLowerRoom_EventScript_BlackBelt::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_FOCUS_BAND, ShoalCave_LowTideLowerRoom_EventScript_ReceivedFocusBand
	msgbox ShoalCave_LowTideLowerRoom_Text_CanOvercomeColdWithFocus, MSGBOX_DEFAULT
	giveitem ITEM_FOCUS_BAND
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_FOCUS_BAND
	release
	end

ShoalCave_LowTideLowerRoom_EventScript_ReceivedFocusBand::
	msgbox ShoalCave_LowTideLowerRoom_Text_EverythingStartsWithFocus, MSGBOX_DEFAULT
	release
	end

ShoalCave_LowTideLowerRoom_Text_CanOvercomeColdWithFocus:
	.string "The penetrating cold around these\n"
	.string "parts is an impediment to training.\p"
	.string "But with focus, one can overcome!\p"
	.string "With this FOCUS BAND, buckle down and\n"
	.string "withstand the cold!$"

ShoalCave_LowTideLowerRoom_Text_EverythingStartsWithFocus:
	.string "Everything starts with focus!$"

