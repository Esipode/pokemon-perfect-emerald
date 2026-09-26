MeteorFalls_RegiCave_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MeteorFalls_RegiCave_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, MeteorFalls_RegiCave_OnLoad
	.byte 0

MeteorFalls_RegiCave_OnTransition:
	updatelegendaryvisibility SPECIES_REGIDRAGO, FLAG_HIDE_REGIDRAGO
	call_if_unset FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_EventScript_ResetAltarStep
	end

MeteorFalls_RegiCave_EventScript_ResetAltarStep::
	setvar VAR_REGIDRAGO_ALTAR_STEP, 0
	return

@ Same idiom as AncientTomb/DesertRuins/IslandCave: the entrance metatiles start sealed every load
@ until the puzzle flag is set, then call_if_unset stops firing and it stays open for good.
MeteorFalls_RegiCave_OnLoad:
	call_if_unset FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_EventScript_HideRegiEntrance
	setvar VAR_0x8004, SPECIES_REGIDRAGO
	setvar VAR_0x8005, LAYOUT_METEOR_FALLS_REGI_CAVE
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, MeteorFalls_RegiCave_HideRegidragoArena
	goto_if_unset FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_HideRegidragoArena
	end

MeteorFalls_RegiCave_Regidrago_Encounter::
	goto LegendaryEncounter_EventScript_Regidrago

MeteorFalls_RegiCave_HideRegidragoArena:
	setflag FLAG_HIDE_REGIDRAGO
	end

@ Entrance is the same 3x2 footprint as AncientTomb: braille cover on the top row, braille-mid on
@ the bottom row, both impassable, until the puzzle is solved.
MeteorFalls_RegiCave_EventScript_HideRegiEntrance::
	setmetatile 7, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 8, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 9, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 9, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	return

@ North-wall braille sign. Gives the order only ("Wyrm, then Drake, then Wyvern, then Dragon") --
@ not which statue is which. Reading a statue's own braille always names it and, if it's the
@ correct next statue, advances the puzzle; matching name to position is the puzzle.
MeteorFalls_RegiCave_EventScript_Braille::
	lockall
	goto_if_set FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_EventScript_BigHoleInWall
	braillemsgbox MeteorFalls_RegiCave_Braille_WakeTheDragon
	releaseall
	end

MeteorFalls_RegiCave_EventScript_BigHoleInWall::
	msgbox gText_BigHoleInTheWall, MSGBOX_DEFAULT
	releaseall
	end

@ Four decorative statues (dragon-lineage names, unrelated to the Regirock/Regice/Registeel
@ puzzles). VAR_REGIDRAGO_ALTAR_STEP holds the next expected altar index (0-3); reading the wrong
@ statue resets it to 0.

MeteorFalls_RegiCave_EventScript_WyrmStatue::
	lockall
	braillemsgbox MeteorFalls_RegiCave_Braille_WyrmAltar
	goto_if_set FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_EventScript_StatueDone
	goto_if_eq VAR_REGIDRAGO_ALTAR_STEP, 0, MeteorFalls_RegiCave_EventScript_WyrmAdvance
	setvar VAR_REGIDRAGO_ALTAR_STEP, 0
	goto MeteorFalls_RegiCave_EventScript_StatueDone

MeteorFalls_RegiCave_EventScript_WyrmAdvance:
	setvar VAR_REGIDRAGO_ALTAR_STEP, 1
	goto MeteorFalls_RegiCave_EventScript_StatueDone

MeteorFalls_RegiCave_EventScript_DrakeStatue::
	lockall
	braillemsgbox MeteorFalls_RegiCave_Braille_DrakeAltar
	goto_if_set FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_EventScript_StatueDone
	goto_if_eq VAR_REGIDRAGO_ALTAR_STEP, 1, MeteorFalls_RegiCave_EventScript_DrakeAdvance
	setvar VAR_REGIDRAGO_ALTAR_STEP, 0
	goto MeteorFalls_RegiCave_EventScript_StatueDone

MeteorFalls_RegiCave_EventScript_DrakeAdvance:
	setvar VAR_REGIDRAGO_ALTAR_STEP, 2
	goto MeteorFalls_RegiCave_EventScript_StatueDone

MeteorFalls_RegiCave_EventScript_WyvernStatue::
	lockall
	braillemsgbox MeteorFalls_RegiCave_Braille_WyvernAltar
	goto_if_set FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_EventScript_StatueDone
	goto_if_eq VAR_REGIDRAGO_ALTAR_STEP, 2, MeteorFalls_RegiCave_EventScript_WyvernAdvance
	setvar VAR_REGIDRAGO_ALTAR_STEP, 0
	goto MeteorFalls_RegiCave_EventScript_StatueDone

MeteorFalls_RegiCave_EventScript_WyvernAdvance:
	setvar VAR_REGIDRAGO_ALTAR_STEP, 3
	goto MeteorFalls_RegiCave_EventScript_StatueDone

MeteorFalls_RegiCave_EventScript_DragonStatue::
	lockall
	braillemsgbox MeteorFalls_RegiCave_Braille_DragonAltar
	goto_if_set FLAG_REGIDRAGO_PUZZLE_COMPLETED, MeteorFalls_RegiCave_EventScript_StatueDone
	goto_if_ne VAR_REGIDRAGO_ALTAR_STEP, 3, MeteorFalls_RegiCave_EventScript_DragonFail
	setflag FLAG_REGIDRAGO_PUZZLE_COMPLETED
	clearflag FLAG_HIDE_REGIDRAGO
	showobjectat 1, MAP_METEOR_FALLS_REGI_CAVE
	goto MeteorFalls_RegiCave_EventScript_OpenRegiEntrance

MeteorFalls_RegiCave_EventScript_DragonFail:
	setvar VAR_REGIDRAGO_ALTAR_STEP, 0
	goto MeteorFalls_RegiCave_EventScript_StatueDone

@ Same reveal as AncientTomb/IslandCave: swap in the open-entrance metatiles, redraw, bang.
MeteorFalls_RegiCave_EventScript_OpenRegiEntrance::
	setmetatile 7, 19, METATILE_Cave_SealedChamberEntrance_TopLeft, TRUE
	setmetatile 8, 19, METATILE_Cave_SealedChamberEntrance_TopMid, TRUE
	setmetatile 9, 19, METATILE_Cave_SealedChamberEntrance_TopRight, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberEntrance_BottomLeft, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberEntrance_BottomMid, FALSE
	setmetatile 9, 20, METATILE_Cave_SealedChamberEntrance_BottomRight, TRUE
	special DrawWholeMapView
	playse SE_BANG
	goto MeteorFalls_RegiCave_EventScript_StatueDone

MeteorFalls_RegiCave_EventScript_StatueDone:
	releaseall
	end
