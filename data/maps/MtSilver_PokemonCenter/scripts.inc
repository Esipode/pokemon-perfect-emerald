MtSilver_PokemonCenter_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtSilver_PokemonCenter_OnTransition
	.byte 0

MtSilver_PokemonCenter_OnTransition::
	setrespawn HEAL_LOCATION_MT_SILVER
	end

MtSilver_PokemonCenter_EventScript_Meara::
	lock
	faceplayer
	msgbox MtSilver_PokemonCenter_Text_Meara1, MSGBOX_DEFAULT
	closemessage
	release
	end

MtSilver_PokemonCenter_Text_Meara1:
	.string "I'm a researcher from... Uhh...\n"
	.string "the PEWTER CITY museum!\p"
	.string "An unusual amount of artifacts have\n"
	.string "been discovered in this area, so\l"
	.string "I've come here to investigate.\p"
	.string "Be careful out there. The mountain\n"
	.string "is full of strong POKéMON.$"
