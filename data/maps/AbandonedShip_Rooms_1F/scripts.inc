AbandonedShip_Rooms_1F_MapScripts::
	.byte 0

AbandonedShip_Rooms_1F_EventScript_Gentleman::
	msgbox AbandonedShip_Rooms_1F_Text_TakingALookAround, MSGBOX_NPC
	end

AbandonedShip_Rooms_1F_EventScript_Demetrius::
	trainerbattle_single TRAINER_DEMETRIUS, AbandonedShip_Rooms_1F_Text_DemetriusIntro, AbandonedShip_Rooms_1F_Text_DemetriusDefeat
	msgbox AbandonedShip_Rooms_1F_Text_DemetriusPostBattle, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Rooms_1F_EventScript_Thalia::
	trainerbattle_single TRAINER_THALIA_1, AbandonedShip_Rooms_1F_Text_ThaliaIntro, AbandonedShip_Rooms_1F_Text_ThaliaDefeat
	msgbox AbandonedShip_Rooms_1F_Text_ThaliaPostBattle, MSGBOX_DEFAULT
	release
	end

AbandonedShip_Rooms_1F_Text_TakingALookAround:
	.string "Ships of this sort are rare, so I'm\n"
	.string "taking a look around.\p"
	.string "Hmhm…\n"
	.string "There appear to be other cabins…$"

AbandonedShip_Rooms_1F_Text_ThaliaIntro:
	.string "What on earth would compel you to\n"
	.string "come here? You must be curious!$"

AbandonedShip_Rooms_1F_Text_ThaliaDefeat:
	.string "Not just curious, but also strong…$"

AbandonedShip_Rooms_1F_Text_ThaliaPostBattle:
	.string "The man next door…\p"
	.string "He says he's just sightseeing,\n"
	.string "but I don't know about that.$"

AbandonedShip_Rooms_1F_Text_DemetriusIntro:
	.string "Waaah!\n"
	.string "I've been found! …Huh?$"

AbandonedShip_Rooms_1F_Text_DemetriusDefeat:
	.string "Oh, you're not my mom.$"

AbandonedShip_Rooms_1F_Text_DemetriusPostBattle:
	.string "I'm in trouble with my mom, so I ran.\n"
	.string "Keep it a secret where I am!$"

