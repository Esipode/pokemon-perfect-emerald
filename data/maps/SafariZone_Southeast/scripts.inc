SafariZone_Southeast_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SafariZone_Southeast_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, SafariZone_Southeast_OnLoad
	.byte 0

SafariZone_Southeast_OnTransition:
	updatelegendaryvisibility SPECIES_ENAMORUS, FLAG_HIDE_ENAMORUS
	end

SafariZone_Southeast_OnLoad:
	setvar VAR_0x8004, SPECIES_ENAMORUS
	setvar VAR_0x8005, LAYOUT_SAFARI_ZONE_SOUTHEAST
	setvar VAR_0x8006, WEEKDAY_FRI
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, SafariZone_Southeast_HideEnamorusArena
	gettimeofday
	goto_if_ne VAR_0x8000, TIME_NIGHT, SafariZone_Southeast_HideEnamorusArena
	checkspeciesowned SPECIES_LANDORUS
	goto_if_eq VAR_RESULT, FALSE, SafariZone_Southeast_HideEnamorusArena
	checkspeciesowned SPECIES_THUNDURUS
	goto_if_eq VAR_RESULT, FALSE, SafariZone_Southeast_HideEnamorusArena
	checkspeciesowned SPECIES_TORNADUS
	goto_if_eq VAR_RESULT, FALSE, SafariZone_Southeast_HideEnamorusArena
	goto SafariZone_Southeast_ShowEnamorusArena

SafariZone_Southeast_Enamorus_Encounter::
	goto LegendaryEncounter_EventScript_Enamorus

SafariZone_Southeast_HideEnamorusArena:
	setflag FLAG_HIDE_ENAMORUS
	end

SafariZone_Southeast_ShowEnamorusArena:
	@ Pink tint strengthens with proximity, sparkles twinkle around Enamorus
	overlaycreate RGB(31, 14, 22), 6, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlayanchorobject VAR_TEMP_5, LOCALID_SAFARI_ZONE_SOUTHEAST_ENAMORUS
	overlayfalloff VAR_TEMP_5, 2, 14, 0, 16
	screenfxparticles VAR_TEMP_6, 8, SCREENFX_PARTICLE_SPARKLE, RGB(31, 20, 26), 3, 0
	screenfxanchorobject VAR_TEMP_6, LOCALID_SAFARI_ZONE_SOUTHEAST_ENAMORUS
	end

@ Event scripts for SafariZone_Southeast are in SafariZone_South/scripts.inc
