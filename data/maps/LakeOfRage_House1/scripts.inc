LakeOfRage_House1_MapScripts::
	.byte 0

LakeOfRage_House1_EventScript_HiddenPowerGuy::
	lock
	faceplayer
	goto_if_set FLAG_GOT_HIDDEN_POWER, LakeOfRage_House1_EventScript_HiddenPowerGuyAfter
	msgbox HiddenPowerGuy_Text_Intro, MSGBOX_DEFAULT
	giveitem ITEM_TM_WEATHER_BALL
	msgbox HiddenPowerGuy_Text_Explanation, MSGBOX_DEFAULT
	setflag FLAG_GOT_HIDDEN_POWER
	closemessage
	release
	end

LakeOfRage_House1_EventScript_HiddenPowerGuyAfter::
	msgbox HiddenPowerGuy_Text_Meditating, MSGBOX_DEFAULT
	closemessage
	release
	end

HiddenPowerGuy_Text_Intro:
    .string "…You have strayed far…\p"
    .string "Here I have meditated. Inside me,\n"
    .string "a new power has been awakened.\p"
    .string "Let me share my power with your\n"
    .string "POKéMON. Take this, child.$"

HiddenPowerGuy_Text_Explanation:
    .string "Do you see it? It is WEATHER BALL!\p"
    .string "It gathers the energy of the sky\n"
    .string "into a sphere.\p"
    .string "Remember this: its type and power\n"
    .string "change with the weather.$"

HiddenPowerGuy_Text_Meditating:
    .string "I am meditating…$"
