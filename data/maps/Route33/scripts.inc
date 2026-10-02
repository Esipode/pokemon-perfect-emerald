Route33_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route33_OnTransition
	.byte 0

Route33_OnTransition: 
	end

Route33_EventScript_HikerAnthony::
	trainerbattle_single TRAINER_ANTHONY_JOHTO, Route33_Text_HikerAnthony2Seen, Route33_Text_HikerAnthony2Beaten, Route33_EventScript_RegisterAnthony
	setvar VAR_RESULT, FALSE
	goto_if_eq VAR_RESULT, TRUE, Route33_EventScript_RematchAnthony
	msgbox Route33_Text_HikerAnthony2After, MSGBOX_AUTOCLOSE
	end

Route33_EventScript_RegisterAnthony::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox Route33_Text_AnthonyRegister, MSGBOX_DEFAULT
	release
	end

Route33_EventScript_RematchAnthony::
	msgbox Route33_Text_AnthonyPostRematch, MSGBOX_AUTOCLOSE
	end

Route33_EventScript_Lass::
	lock
	faceplayer
	msgbox Route33_Text_Lass, MSGBOX_DEFAULT
	closemessage
	release
	end

Route33_Text_HikerAnthony2Seen:
	.string "I came through the tunnel,\n"
	.string "but I still have plenty\l"
	.string "of energy left.$"

Route33_Text_HikerAnthony2Beaten:
	.string "Whoa! You've got more zip\n"
	.string "than me!$"

Route33_Text_HikerAnthony2After:
	.string "We HIKERS are at our best\n"
	.string "in the mountains.$"

Route33_Text_AnthonyRegister:
	.string "We HIKERS are at our best\n"
	.string "in the mountains.$"

Route33_Text_AnthonyRematchIntro:
	.string "I came through the tunnel,\n"
	.string "but I still have plenty\l"
	.string "of energy left.$"

Route33_Text_AnthonyRematchDefeat:
	.string "Whoa! You've got more zip\n"
	.string "than me!$"

Route33_Text_AnthonyPostRematch:
	.string "We HIKERS are at our best\n"
	.string "in the mountains.$"

Route33_Text_HikerAnthonyDunsparce:
	.string "Hey, did you get a DUNSPARCE?\p"
	.string "I caught one too.\p"
	.string "Take a look at it in the light.\n"
	.string "It's got a funny face!$"

Route33_Text_Lass:
	.string "Pant, pant…\p"
	.string "That cave was bigger than\n"
	.string "I'd expected.\p"
	.string "I got too tired exploring\n"
	.string "for MARILL, so I came outside.$"
