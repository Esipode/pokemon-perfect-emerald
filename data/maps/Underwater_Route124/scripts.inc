Underwater_Route124_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Underwater_Route124_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Underwater_Route124_OnLoad
	map_script MAP_SCRIPT_ON_RESUME, Underwater_Route124_OnResume
	.byte 0

Underwater_Route124_OnTransition:
	updatelegendaryvisibility SPECIES_ZYGARDE, FLAG_HIDE_ZYGARDE
	end

Underwater_Route124_OnLoad:
	setvar VAR_0x8004, SPECIES_ZYGARDE
	setvar VAR_0x8005, LAYOUT_UNDERWATER_ROUTE124
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, Underwater_Route124_HideZygardeArena
	getcaughtmon SPECIES_XERNEAS
	goto_if_eq VAR_RESULT, FALSE, Underwater_Route124_HideZygardeArena
	getcaughtmon SPECIES_YVELTAL
	goto_if_eq VAR_RESULT, FALSE, Underwater_Route124_HideZygardeArena
	end

Underwater_Route124_Zygarde_Encounter::
	goto LegendaryEncounter_EventScript_Zygarde

Underwater_Route124_HideZygardeArena:
	setflag FLAG_HIDE_ZYGARDE
	end

@ Connection steps destroy map-local overlays after ON_LOAD, so the tint is created here.
@ VAR_TEMP_5 is the guard: it survives battles and is cleared on every map load.
Underwater_Route124_OnResume:
	goto_if_set FLAG_HIDE_ZYGARDE, Underwater_Route124_OnResumeDone
	goto_if_ne VAR_TEMP_5, 0, Underwater_Route124_OnResumeDone
	@ Green tint, pulsing slowly; fades out at 12 tiles from Zygarde
	overlaycreate RGB(6, 28, 10), 12, OVERLAY_LAYER_ALL, OVERLAY_SCOPE_MAP_LOCAL, VAR_TEMP_5
	overlaypulse VAR_TEMP_5, 4, 10, 120
	overlayanchorpos VAR_TEMP_5, 14, 28
	overlayfalloff VAR_TEMP_5, 3, 12, 0, 16
Underwater_Route124_OnResumeDone:
	end

