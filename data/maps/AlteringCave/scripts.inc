AlteringCave_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, AlteringCave_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, AlteringCave_OnLoad
	.byte 0

AlteringCave_OnTransition:
	setflag FLAG_LANDMARK_ALTERING_CAVE
	updatelegendaryvisibility SPECIES_FEZANDIPITI, FLAG_HIDE_FEZANDIPITI
	end

AlteringCave_OnLoad:
	setvar VAR_0x8004, SPECIES_FEZANDIPITI
	setvar VAR_0x8005, LAYOUT_ALTERING_CAVE
	setvar VAR_0x8006, WEEKDAY_SAT
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, AlteringCave_HideFezandipitiArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_MORNING, AlteringCave_HideFezandipitiArena
	goto AlteringCave_ShowFezandipitiArena

AlteringCave_Fezandipiti_Encounter::
	goto LegendaryEncounter_EventScript_Fezandipiti

AlteringCave_HideFezandipitiArena:
	setflag FLAG_HIDE_FEZANDIPITI
	end

AlteringCave_ShowFezandipitiArena:
	setflag FLAG_TEMP_NO_WILD_ENCOUNTERS
	goto_if_ne VAR_TEMP_6, 0, AlteringCave_ShowFezandipitiArena_ParticlesDone
	screenfxparticles VAR_TEMP_6, 8, SCREENFX_PARTICLE_MOTE, RGB(31, 18, 24), 3, 0
	screenfxanchorpos VAR_TEMP_6, 5, 13
AlteringCave_ShowFezandipitiArena_ParticlesDone:
	@ TODO(user): arena metatiles. One setmetatilewithelevation per tile:
	@   setmetatilewithelevation <x>, <y>, <metatileId>, <impassable>, <elevation>
	end
