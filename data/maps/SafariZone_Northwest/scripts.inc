SafariZone_Northwest_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SafariZone_Northwest_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, SafariZone_Northwest_OnLoad
	.byte 0

SafariZone_Northwest_OnTransition:
	updatelegendaryvisibility SPECIES_OKIDOGI, FLAG_HIDE_OKIDOGI
	end

SafariZone_Northwest_OnLoad:
	setvar VAR_0x8004, SPECIES_OKIDOGI
	setvar VAR_0x8005, LAYOUT_SAFARI_ZONE_NORTHWEST
	setvar VAR_0x8006, WEEKDAY_FRI
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, SafariZone_Northwest_HideOkidogiArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_NIGHT, SafariZone_Northwest_HideOkidogiArena
	goto SafariZone_Northwest_ShowOkidogiArena

SafariZone_Northwest_Okidogi_Encounter::
	goto LegendaryEncounter_EventScript_Okidogi

SafariZone_Northwest_HideOkidogiArena:
	setflag FLAG_HIDE_OKIDOGI
	end

SafariZone_Northwest_ShowOkidogiArena:
	setflag FLAG_TEMP_HIDE_SAFARI_NW_NPC
	goto_if_ne VAR_TEMP_0, 0, SafariZone_Northwest_ShowOkidogiArena_ParticlesDone
	screenfxparticles VAR_TEMP_0, 8, SCREENFX_PARTICLE_BUBBLE, RGB(18, 6, 24), 3, 0
	screenfxanchorpos VAR_TEMP_0, 21, 27
SafariZone_Northwest_ShowOkidogiArena_ParticlesDone:
	goto_if_ne VAR_TEMP_1, 0, SafariZone_Northwest_ShowOkidogiArena_ShakeDone
	screenfxshake VAR_TEMP_1, 4, 32, SCREENFX_AXIS_X | SCREENFX_AXIS_Y, 0
	screenfxpulse VAR_TEMP_1, 30, 150
	screenfxanchorpos VAR_TEMP_1, 21, 27
	screenfxfalloff VAR_TEMP_1, 3, 12, 0, 16
SafariZone_Northwest_ShowOkidogiArena_ShakeDone:
	end

SafariZone_Northwest_EventScript_Man::
	msgbox SafariZone_Northwest_Text_Man, MSGBOX_NPC
	end

