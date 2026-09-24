NewMauville_Inside_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, NewMauville_Inside_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, NewMauville_Inside_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, NewMauville_Inside_OnLoad
	.byte 0

NewMauville_Inside_OnResume:
	call_if_eq VAR_TEMP_1, 1, NewMauville_Inside_EventScript_SetBarrierStateBlueButton
	call_if_eq VAR_TEMP_2, 1, NewMauville_Inside_EventScript_SetBarrierStateGreenButton
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, NewMauville_Inside_EventScript_TryRemoveVoltorb
	end

NewMauville_Inside_EventScript_TryRemoveVoltorb::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject VAR_LAST_TALKED
	return

NewMauville_Inside_OnTransition:
	setvar VAR_TEMP_1, 0
	setvar VAR_TEMP_2, 0
	call_if_unset FLAG_DEFEATED_VOLTORB_1_NEW_MAUVILLE, NewMauville_Inside_EventScript_ShowVoltorb1
	call_if_unset FLAG_DEFEATED_VOLTORB_2_NEW_MAUVILLE, NewMauville_Inside_EventScript_ShowVoltorb2
	call_if_unset FLAG_DEFEATED_VOLTORB_3_NEW_MAUVILLE, NewMauville_Inside_EventScript_ShowVoltorb3
	updatelegendaryvisibility SPECIES_ZERAORA, FLAG_HIDE_ZERAORA
	end

NewMauville_Inside_EventScript_ShowVoltorb1::
	clearflag FLAG_HIDE_NEW_MAUVILLE_VOLTORB_1
	return

NewMauville_Inside_EventScript_ShowVoltorb2::
	clearflag FLAG_HIDE_NEW_MAUVILLE_VOLTORB_2
	return

NewMauville_Inside_EventScript_ShowVoltorb3::
	clearflag FLAG_HIDE_NEW_MAUVILLE_VOLTORB_3
	return

NewMauville_Inside_OnLoad:
	call_if_eq VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_SetGeneratorOffMetatiles
	@ Zeraora spawn gate: post-game, not already owned, Saturday, daytime, generator quest complete
	setvar VAR_0x8004, SPECIES_ZERAORA
	setvar VAR_0x8005, LAYOUT_NEW_MAUVILLE_INSIDE
	setvar VAR_0x8006, WEEKDAY_SAT
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, NewMauville_Inside_HideZeraoraArena
	goto_if_ne VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_HideZeraoraArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_DAY, NewMauville_Inside_HideZeraoraArena
	goto NewMauville_Inside_ShowZeraoraArena

NewMauville_Inside_Zeraora_Encounter::
	goto LegendaryEncounter_EventScript_Zeraora

NewMauville_Inside_HideZeraoraArena:
	setflag FLAG_HIDE_ZERAORA
	end

NewMauville_Inside_ShowZeraoraArena:
	@ Wild encounters stay off while Zeraora is present (temp flag clears on map load)
	setflag FLAG_TEMP_NO_WILD_ENCOUNTERS
	@ Bolt: yellow flash (opacity 8) fades with the shake over 20 frames, thunder on the same frame,
	@ then 420 frames of calm. The shake's pulse window drives the flash, so they cannot drift apart.
	overlaycreate RGB(31, 30, 4), 0, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	screenfxshake VAR_TEMP_8, 6, 16, SCREENFX_AXIS_X | SCREENFX_AXIS_Y, 0
	screenfxpulse VAR_TEMP_8, 20, 420, SE_THUNDER
	screenfxpulseflash VAR_TEMP_8, VAR_TEMP_5, 8
	end

NewMauville_Inside_EventScript_BlueButton::
	lockall
	setvar VAR_TEMP_1, 1
	setvar VAR_TEMP_2, 0
	playse SE_PIN
	call NewMauville_Inside_EventScript_SetBarrierStateBlueButton
	special DrawWholeMapView
	releaseall
	end

NewMauville_Inside_EventScript_GreenButton::
	lockall
	setvar VAR_TEMP_1, 0
	setvar VAR_TEMP_2, 1
	playse SE_PIN
	call NewMauville_Inside_EventScript_SetBarrierStateGreenButton
	special DrawWholeMapView
	releaseall
	end

NewMauville_Inside_EventScript_SetBarrierStateBlueButton::
	setmetatile 23, 34, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 23, 35, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 23, 36, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 23, 37, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 10, 16, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 10, 17, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 10, 18, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 10, 19, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 10, 0, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 10, 1, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 10, 2, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 10, 3, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 37, 33, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 37, 34, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 37, 35, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 37, 36, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 28, 22, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 28, 23, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 28, 24, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 28, 25, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 10, 24, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 10, 25, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 10, 26, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 10, 27, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 21, 2, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 21, 3, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 21, 4, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 21, 5, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 6, 11, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 13, 10, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 16, 22, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 4, 26, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 30, 38, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 2, 11, METATILE_BikeShop_Button_Green, FALSE
	setmetatile 17, 10, METATILE_BikeShop_Button_Green, FALSE
	setmetatile 25, 18, METATILE_BikeShop_Button_Green, FALSE
	setmetatile 18, 36, METATILE_BikeShop_Button_Green, FALSE
	return

NewMauville_Inside_EventScript_SetBarrierStateGreenButton::
	setmetatile 23, 34, METATILE_BikeShop_Barrier_Blue_Top, TRUE
	setmetatile 23, 35, METATILE_BikeShop_Barrier_Blue_TopMid, TRUE
	setmetatile 23, 36, METATILE_BikeShop_Barrier_Blue_BottomMid, TRUE
	setmetatile 23, 37, METATILE_BikeShop_Barrier_Blue_Bottom, TRUE
	setmetatile 10, 16, METATILE_BikeShop_Barrier_Blue_Top, TRUE
	setmetatile 10, 17, METATILE_BikeShop_Barrier_Blue_TopMid, TRUE
	setmetatile 10, 18, METATILE_BikeShop_Barrier_Blue_BottomMid, TRUE
	setmetatile 10, 19, METATILE_BikeShop_Barrier_Blue_Bottom, TRUE
	setmetatile 10, 0, METATILE_BikeShop_Barrier_Blue_Top, TRUE
	setmetatile 10, 1, METATILE_BikeShop_Barrier_Blue_TopMid, TRUE
	setmetatile 10, 2, METATILE_BikeShop_Barrier_Blue_BottomMid, TRUE
	setmetatile 10, 3, METATILE_BikeShop_Barrier_Blue_Bottom, TRUE
	setmetatile 37, 33, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 37, 34, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 37, 35, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 37, 36, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 28, 22, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 28, 23, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 28, 24, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 28, 25, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 10, 24, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 10, 25, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 10, 26, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 10, 27, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 21, 2, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 21, 3, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 21, 4, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 21, 5, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 2, 11, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 17, 10, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 25, 18, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 18, 36, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 6, 11, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 13, 10, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 16, 22, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 4, 26, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 30, 38, METATILE_BikeShop_Button_Blue, FALSE
	return

NewMauville_Inside_EventScript_RedButton::
	lockall
	msgbox NewMauville_Inside_Text_SteppedOnSwitchGeneratorStopped, MSGBOX_DEFAULT
	call NewMauville_Inside_EventScript_SetGeneratorOffMetatiles
	setvar VAR_NEW_MAUVILLE_STATE, 2
	releaseall
	end

NewMauville_Inside_EventScript_SetGeneratorOffMetatiles::
	setmetatile 33, 6, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 32, 2, METATILE_BikeShop_Generator_Off_Tile0, TRUE
	setmetatile 33, 2, METATILE_BikeShop_Generator_Off_Tile1, TRUE
	setmetatile 34, 2, METATILE_BikeShop_Generator_Off_Tile2, TRUE
	setmetatile 35, 2, METATILE_BikeShop_Generator_Off_Tile3, TRUE
	setmetatile 32, 3, METATILE_BikeShop_Generator_Off_Tile4, TRUE
	setmetatile 33, 3, METATILE_BikeShop_Generator_Off_Tile5, TRUE
	setmetatile 34, 3, METATILE_BikeShop_Generator_Off_Tile6, TRUE
	setmetatile 35, 3, METATILE_BikeShop_Generator_Off_Tile7, TRUE
	special DrawWholeMapView
	return

NewMauville_Inside_EventScript_Generator::
	lockall
	goto_if_eq VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_GeneratorOff
	msgbox NewMauville_Inside_Text_GeneratorRadiatingHeat, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_GeneratorOff::
	msgbox NewMauville_Inside_Text_GeneratorQuietedDown, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_Voltorb1::
	lock
	faceplayer
	setwildbattle SPECIES_VOLTORB, 25
	waitse
	playmoncry SPECIES_VOLTORB, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, NewMauville_Inside_EventScript_DefeatedVoltorb1
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, NewMauville_Inside_EventScript_DefeatedVoltorb1
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, NewMauville_Inside_EventScript_DefeatedVoltorb1
	setflag FLAG_DEFEATED_VOLTORB_1_NEW_MAUVILLE
	release
	end

NewMauville_Inside_EventScript_DefeatedVoltorb1::
	setflag FLAG_DEFEATED_VOLTORB_1_NEW_MAUVILLE
	goto Common_EventScript_RemoveStaticPokemon
	end

NewMauville_Inside_EventScript_Voltorb2::
	lock
	faceplayer
	setwildbattle SPECIES_VOLTORB, 25
	waitse
	playmoncry SPECIES_VOLTORB, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, NewMauville_Inside_EventScript_DefeatedVoltorb2
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, NewMauville_Inside_EventScript_DefeatedVoltorb2
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, NewMauville_Inside_EventScript_DefeatedVoltorb2
	setflag FLAG_DEFEATED_VOLTORB_2_NEW_MAUVILLE
	release
	end

NewMauville_Inside_EventScript_DefeatedVoltorb2::
	setflag FLAG_DEFEATED_VOLTORB_2_NEW_MAUVILLE
	goto Common_EventScript_RemoveStaticPokemon
	end

NewMauville_Inside_EventScript_Voltorb3::
	lock
	faceplayer
	setwildbattle SPECIES_VOLTORB, 25
	waitse
	playmoncry SPECIES_VOLTORB, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, NewMauville_Inside_EventScript_DefeatedVoltorb3
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, NewMauville_Inside_EventScript_DefeatedVoltorb3
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, NewMauville_Inside_EventScript_DefeatedVoltorb3
	setflag FLAG_DEFEATED_VOLTORB_3_NEW_MAUVILLE
	release
	end

NewMauville_Inside_EventScript_DefeatedVoltorb3::
	setflag FLAG_DEFEATED_VOLTORB_3_NEW_MAUVILLE
	goto Common_EventScript_RemoveStaticPokemon
	end

NewMauville_Inside_Text_GeneratorRadiatingHeat:
	.string "The generator is radiating heat that\n"
	.string "can be felt even at a distance.\p"
	.string "It looks like it should be turned off\n"
	.string "as soon as possible.$"

NewMauville_Inside_Text_GeneratorQuietedDown:
	.string "The generator quieted down.$"

NewMauville_Inside_Text_SteppedOnSwitchGeneratorStopped:
	.string "{PLAYER} stepped on the switch.\p"
	.string "Click…\p"
	.string "… … … … … … … …\n"
	.string "… … … … … … … …\p"
	.string "The generator appears to have\n"
	.string "stopped…$"
