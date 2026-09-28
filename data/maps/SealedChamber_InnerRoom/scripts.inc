SealedChamber_InnerRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SealedChamber_InnerRoom_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, SealedChamber_InnerRoom_OnLoad
	.byte 0

SealedChamber_InnerRoom_OnTransition:
	updatelegendaryvisibility SPECIES_REGIGIGAS, FLAG_HIDE_REGIGIGAS
	end

SealedChamber_InnerRoom_OnLoad:
	@ Regigigas spawn gate: post-game, not already owned, all five Regis caught
	setvar VAR_0x8004, SPECIES_REGIGIGAS
	setvar VAR_0x8005, LAYOUT_SEALED_CHAMBER_INNER_ROOM
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, SealedChamber_InnerRoom_HideRegigigasArena
	checkspeciesowned SPECIES_REGIROCK
	goto_if_eq VAR_RESULT, FALSE, SealedChamber_InnerRoom_HideRegigigasArena
	checkspeciesowned SPECIES_REGICE
	goto_if_eq VAR_RESULT, FALSE, SealedChamber_InnerRoom_HideRegigigasArena
	checkspeciesowned SPECIES_REGISTEEL
	goto_if_eq VAR_RESULT, FALSE, SealedChamber_InnerRoom_HideRegigigasArena
	checkspeciesowned SPECIES_REGIDRAGO
	goto_if_eq VAR_RESULT, FALSE, SealedChamber_InnerRoom_HideRegigigasArena
	checkspeciesowned SPECIES_REGIELEKI
	goto_if_eq VAR_RESULT, FALSE, SealedChamber_InnerRoom_HideRegigigasArena
	goto SealedChamber_InnerRoom_ShowRegigigasArena

SealedChamber_InnerRoom_Regigigas_Encounter::
	goto LegendaryEncounter_EventScript_Regigigas

SealedChamber_InnerRoom_HideRegigigasArena:
	setflag FLAG_HIDE_REGIGIGAS
	end

SealedChamber_InnerRoom_ShowRegigigasArena:
	@ TODO(user): arena metatiles. One setmetatilewithelevation per tile:
	@   setmetatilewithelevation <x>, <y>, <metatileId>, <impassable>, <elevation>
	end

SealedChamber_InnerRoom_EventScript_BrailleBackWall::
	lockall
	braillemsgbox SealedChamber_InnerRoom_Braille_FirstWailordLastRelicanth
	goto_if_set FLAG_REGI_DOORS_OPENED, SealedChamber_InnerRoom_EventScript_NoEffect
	specialvar VAR_RESULT, CheckRelicanthWailord
	goto_if_eq VAR_RESULT, FALSE, SealedChamber_InnerRoom_EventScript_NoEffect
	fadeoutbgm 0
	playse SE_TRUCK_MOVE
	special DoSealedChamberShakingEffect_Long
	delay 40
	special DoSealedChamberShakingEffect_Short
	playse SE_DOOR
	delay 40
	special DoSealedChamberShakingEffect_Short
	playse SE_DOOR
	delay 40
	special DoSealedChamberShakingEffect_Short
	playse SE_DOOR
	delay 40
	msgbox gText_DoorOpenedFarAway, MSGBOX_DEFAULT
	closemessage
	fadeinbgm 0
	setflag FLAG_REGI_DOORS_OPENED
	releaseall
	end

SealedChamber_InnerRoom_EventScript_NoEffect::
	releaseall
	end

SealedChamber_InnerRoom_EventScript_BrailleStoryPart1::
	lockall
	braillemsgbox SealedChamber_InnerRoom_Braille_InThisCaveWeHaveLived
	releaseall
	end

SealedChamber_InnerRoom_EventScript_BrailleStoryPart2::
	lockall
	braillemsgbox SealedChamber_InnerRoom_Braille_WeOweAllToThePokemon
	releaseall
	end

SealedChamber_InnerRoom_EventScript_BrailleStoryPart3::
	lockall
	braillemsgbox SealedChamber_InnerRoom_Braille_ButWeSealedThePokemonAway
	releaseall
	end

SealedChamber_InnerRoom_EventScript_BrailleStoryPart4::
	lockall
	braillemsgbox SealedChamber_InnerRoom_Braille_WeFearedIt
	releaseall
	end

SealedChamber_InnerRoom_EventScript_BrailleStoryPart5::
	lockall
	braillemsgbox SealedChamber_InnerRoom_Braille_ThoseWithCourageHope
	releaseall
	end

SealedChamber_InnerRoom_EventScript_BrailleStoryPart6::
	lockall
	braillemsgbox SealedChamber_InnerRoom_Braille_OpenDoorEternalPokemonWaits
	releaseall
	end

