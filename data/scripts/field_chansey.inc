@ Field Chansey: heal / PC menu. Visibility is set by FieldChansey_RefreshVisibility.
Common_EventScript_FieldChansey::
	lock
	faceplayer
	playmoncry SPECIES_CHANSEY, CRY_MODE_NORMAL
	msgbox FieldChansey_Text_Greeting, MSGBOX_DEFAULT
	waitmoncry
FieldChansey_EventScript_Menu::
	message FieldChansey_Text_WhatToDo
	waitmessage
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, FieldChansey_Text_MenuHeal, FieldChansey_Text_MenuPC, FieldChansey_Text_MenuCancel
	switch VAR_RESULT
	case 0, FieldChansey_EventScript_Heal
	case 1, FieldChansey_EventScript_PC
	goto FieldChansey_EventScript_Done

FieldChansey_EventScript_Heal::
	fadescreen FADE_TO_BLACK
	playfanfare MUS_HEAL
	special HealPlayerParty
	callnative FieldChansey_RecordHeal
	waitfanfare
	callnative UpdateFollowingPokemon
	fadescreen FADE_FROM_BLACK
	msgbox FieldChansey_Text_Healed, MSGBOX_DEFAULT
	goto FieldChansey_EventScript_Menu

FieldChansey_EventScript_PC::
	playse SE_PC_LOGIN
	msgbox gText_StorageSystemOpened, MSGBOX_DEFAULT
	special ShowPokemonStorageSystemPC
	goto FieldChansey_EventScript_Menu

FieldChansey_EventScript_Done::
	closemessage
	release
	end
