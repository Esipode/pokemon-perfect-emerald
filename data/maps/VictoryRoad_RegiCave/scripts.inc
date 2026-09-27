VictoryRoad_RegiCave_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, VictoryRoad_RegiCave_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, VictoryRoad_RegiCave_OnLoad
	.byte 0

VictoryRoad_RegiCave_OnTransition:
	updatelegendaryvisibility SPECIES_REGIELEKI, FLAG_HIDE_REGIELEKI
	setvar VAR_REGIELEKI_CHARGE, 0
	end

@ Entrance metatiles start sealed every load until the puzzle flag is set, then stay open for good.
VictoryRoad_RegiCave_OnLoad:
	call_if_unset FLAG_REGIELEKI_PUZZLE_COMPLETED, VictoryRoad_RegiCave_EventScript_HideRegiEntrance
	setvar VAR_0x8004, SPECIES_REGIELEKI
	setvar VAR_0x8005, LAYOUT_VICTORY_ROAD_REGI_CAVE
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, VictoryRoad_RegiCave_HideRegielekiArena
	goto_if_unset FLAG_REGIELEKI_PUZZLE_COMPLETED, VictoryRoad_RegiCave_HideRegielekiArena
	end

VictoryRoad_RegiCave_Regieleki_Encounter::
	goto LegendaryEncounter_EventScript_Regieleki

VictoryRoad_RegiCave_HideRegielekiArena:
	setflag FLAG_HIDE_REGIELEKI
	end

VictoryRoad_RegiCave_EventScript_HideRegiEntrance::
	setmetatile 7, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 8, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 9, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 9, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	return

@ Puzzle: 50 consecutive steps on the top row (Y=21). Row 21 tiles count a step; row 22 tiles,
@ the only way off the row, reset the count.
VictoryRoad_RegiCave_EventScript_ChargeStep::
	goto_if_set FLAG_REGIELEKI_PUZZLE_COMPLETED, VictoryRoad_RegiCave_EventScript_ChargeDone
	addvar VAR_REGIELEKI_CHARGE, 1
	goto_if_lt VAR_REGIELEKI_CHARGE, 50, VictoryRoad_RegiCave_EventScript_ChargeDone
	lockall
	setflag FLAG_REGIELEKI_PUZZLE_COMPLETED
	clearflag FLAG_HIDE_REGIELEKI
	showobjectat 1, MAP_VICTORY_ROAD_REGI_CAVE
	goto VictoryRoad_RegiCave_EventScript_OpenRegiEntrance

VictoryRoad_RegiCave_EventScript_ChargeReset::
	setvar VAR_REGIELEKI_CHARGE, 0
	end

VictoryRoad_RegiCave_EventScript_ChargeDone::
	end

VictoryRoad_RegiCave_EventScript_Braille::
	lockall
	goto_if_set FLAG_REGIELEKI_PUZZLE_COMPLETED, VictoryRoad_RegiCave_EventScript_BigHoleInWall
	braillemsgbox VictoryRoad_RegiCave_Braille_WalkTheWall
	releaseall
	end

VictoryRoad_RegiCave_EventScript_BigHoleInWall::
	msgbox gText_BigHoleInTheWall, MSGBOX_DEFAULT
	releaseall
	end

VictoryRoad_RegiCave_EventScript_OpenRegiEntrance::
	setmetatile 7, 19, METATILE_Cave_SealedChamberEntrance_TopLeft, TRUE
	setmetatile 8, 19, METATILE_Cave_SealedChamberEntrance_TopMid, TRUE
	setmetatile 9, 19, METATILE_Cave_SealedChamberEntrance_TopRight, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberEntrance_BottomLeft, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberEntrance_BottomMid, FALSE
	setmetatile 9, 20, METATILE_Cave_SealedChamberEntrance_BottomRight, TRUE
	special DrawWholeMapView
	playse SE_BANG
	releaseall
	end
