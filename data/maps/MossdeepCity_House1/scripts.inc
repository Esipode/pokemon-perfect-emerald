MossdeepCity_House1_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, MossdeepCity_House1_OnLoad
	.byte 0

MossdeepCity_House1_OnLoad:
	@ Rotom TV chain, stage 9 of 9: postgame, not owned, previous stage cleared
	setvar VAR_0x8004, SPECIES_ROTOM
	setvar VAR_0x8005, LAYOUT_HOUSE2
	setvar VAR_0x8006, WEEKDAY_ANY
	setvar VAR_0x8007, 1
	special CheckValidLegendaryEncounter
	goto_if_eq VAR_RESULT, FALSE, MossdeepCity_House1_HideRotomTV
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 8, MossdeepCity_House1_HideRotomTV
	goto MossdeepCity_House1_ShowRotomTV

MossdeepCity_House1_Rotom_Encounter::
	goto_if_ne VAR_ROTOM_TV_PROGRESS, 8, MossdeepCity_House1_EventScript_RotomTVNothing
	lockall
	msgbox MossdeepCity_House1_Text_RotomInterference, MSGBOX_DEFAULT
	addvar VAR_ROTOM_TV_PROGRESS, 1
	special TurnOffTVScreen
	releaseall
	end

MossdeepCity_House1_EventScript_RotomTVNothing:
	end

MossdeepCity_House1_HideRotomTV:
	end

MossdeepCity_House1_ShowRotomTV:
	special TurnOnTVScreen
	end

MossdeepCity_House1_Text_RotomInterference:
	.string "BZZT!\pThe interference on the TV\n"
	.string "suddenly vanished...$"

MossdeepCity_House1_EventScript_BlackBelt::
	lock
	faceplayer
	bufferleadmonspeciesname STR_VAR_1
	msgbox MossdeepCity_House1_Text_HmmYourPokemon, MSGBOX_DEFAULT
	specialvar VAR_RESULT, GetPokeblockNameByMonNature
	goto_if_eq VAR_RESULT, 0, MossdeepCity_House1_EventScript_NeutralNature
	msgbox MossdeepCity_House1_Text_ItLikesXPokeblocks, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House1_EventScript_NeutralNature::
	msgbox MossdeepCity_House1_Text_DoesntLikeOrDislikePokeblocks, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House1_EventScript_Woman::
	msgbox MossdeepCity_House1_Text_HusbandCanTellPokeblockMonLikes, MSGBOX_NPC
	end

MossdeepCity_House1_Text_HmmYourPokemon:
	.string "Hmm!\n"
	.string "Your {STR_VAR_1}…$"

MossdeepCity_House1_Text_ItLikesXPokeblocks:
	.string "It likes {STR_VAR_1}S,\n"
	.string "doesn't it?\p"
	.string "No, I'm positive of it! It definitely\n"
	.string "likes {STR_VAR_1}S!$"

MossdeepCity_House1_Text_DoesntLikeOrDislikePokeblocks:
	.string "It doesn't appear to like or dislike\n"
	.string "any {POKEBLOCK}S.$"

MossdeepCity_House1_Text_HusbandCanTellPokeblockMonLikes:
	.string "My husband can tell what kind of\n"
	.string "{POKEBLOCK}S a POKéMON likes at a glance.$"

