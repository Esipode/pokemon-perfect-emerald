@ Johto gym trainer flag setters, used by the Johto Gym Leader scripts (VAR_0x8008 = Johto badge 1-8).

Common_EventScript_SetGymTrainers_Johto::
	switch VAR_0x8008
	case 1, VioletCity_Gym_SetGymTrainers_Johto
	case 2, AzaleaTown_Gym_SetGymTrainers_Johto
	case 3, GoldenrodCity_Gym_SetGymTrainers_Johto
	case 4, EcruteakCity_Gym_SetGymTrainers_Johto
	case 5, CianwoodCity_Gym_SetGymTrainers_Johto
	case 6, OlivineCity_Gym_SetGymTrainers_Johto
	case 7, MahoganyTown_Gym_SetGymTrainers_Johto
	case 8, BlackthornCity_Gym_SetGymTrainers_Johto
	end

VioletCity_Gym_SetGymTrainers_Johto::
	settrainerflag TRAINER_ROD_JOHTO
	settrainerflag TRAINER_ABE_JOHTO
	return

AzaleaTown_Gym_SetGymTrainers_Johto::
	settrainerflag TRAINER_BENNY_JOHTO
	settrainerflag TRAINER_AL_JOHTO
	settrainerflag TRAINER_JOSH_JOHTO
	settrainerflag TRAINER_AMY_AND_MAY_JOHTO
	return

GoldenrodCity_Gym_SetGymTrainers_Johto::
	settrainerflag TRAINER_CARRIE_JOHTO
	settrainerflag TRAINER_BRIDGET_JOHTO
	settrainerflag TRAINER_VICTORIA_JOHTO
	settrainerflag TRAINER_SAMANTHA_JOHTO
	return

EcruteakCity_Gym_SetGymTrainers_Johto::
	settrainerflag TRAINER_JEFFREY_JOHTO
	settrainerflag TRAINER_PING_JOHTO
	settrainerflag TRAINER_MARTHA_JOHTO
	settrainerflag TRAINER_GRACE_JOHTO
	return

CianwoodCity_Gym_SetGymTrainers_Johto::
	settrainerflag TRAINER_YOSHI_JOHTO
	settrainerflag TRAINER_LAO_JOHTO
	settrainerflag TRAINER_NOB_JOHTO
	settrainerflag TRAINER_LUNG_JOHTO
	return

OlivineCity_Gym_SetGymTrainers_Johto::
	return

MahoganyTown_Gym_SetGymTrainers_Johto::
	settrainerflag TRAINER_ROXANNE_JOHTO
	settrainerflag TRAINER_RONALD_JOHTO
	settrainerflag TRAINER_CLARISSA_JOHTO
	settrainerflag TRAINER_BRAD_JOHTO
	settrainerflag TRAINER_DOUGLAS_JOHTO
	return

BlackthornCity_Gym_SetGymTrainers_Johto::
	settrainerflag TRAINER_MIKE_JOHTO
	settrainerflag TRAINER_PAUL_JOHTO
	settrainerflag TRAINER_LOLA_JOHTO
	settrainerflag TRAINER_CODY_JOHTO
	settrainerflag TRAINER_FRAN_JOHTO
	return

Common_EventScript_NoOp::
	end

@ Handles VAR_RESULT after a script give-Pokemon command. STR_VAR_1 holds the species name.
Common_EventScript_GiftMon::
	call_if_eq VAR_RESULT, MON_GIVEN_TO_PARTY, Common_EventScript_ReceiveMonParty
	call_if_eq VAR_RESULT, MON_GIVEN_TO_PC, Common_EventScript_ReceiveMonPC
	return

Common_EventScript_ReceiveMonParty::
	playfanfare MUS_OBTAIN_ITEM
	message Common_Text_ReceivedMon
	waitmessage
	waitfanfare
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	call_if_eq VAR_RESULT, TRUE, Common_EventScript_NameReceivedPartyMonFull
	setvar VAR_RESULT, 0
	return

Common_EventScript_ReceiveMonPC::
	playfanfare MUS_OBTAIN_ITEM
	message Common_Text_ReceivedMon
	waitmessage
	waitfanfare
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	call_if_eq VAR_RESULT, TRUE, Common_EventScript_NameReceivedBoxMon
	call Common_EventScript_TransferredToPC
	return

Common_EventScript_NameReceivedPartyMonFull::
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	return

@ Same as Common_EventScript_GiftMon for the givenamedmon gifts, which are never nicknamed or boxed.
Common_EventScript_GiftMonNamed::
	call_if_eq VAR_RESULT, MON_GIVEN_TO_PARTY, Common_EventScript_ReceiveMonPartyNamed
	call_if_eq VAR_RESULT, MON_CANT_GIVE, Common_EventScript_PartyIsFull
	return

Common_EventScript_ReceiveMonPartyNamed::
	playfanfare MUS_OBTAIN_ITEM
	message Common_Text_ReceivedMon
	waitmessage
	waitfanfare
	return

Common_EventScript_PartyIsFull::
	msgbox Common_Text_PartyIsFull, MSGBOX_DEFAULT
	closemessage
	release
	end

Common_Text_ReceivedMon:
	.string "{PLAYER} received {STR_VAR_1}!$"

Common_Text_PartyIsFull:
	.string "Whoa, wait. You can't carry any\n"
	.string "more POKéMON.$"

@ Kurt's fixed-stock Ball shop, used with pokemart.
	.align 2
Johto_Mart_KurtBalls::
	.2byte ITEM_LOVE_BALL
	.2byte ITEM_LURE_BALL
	.2byte ITEM_FRIEND_BALL
	.2byte ITEM_HEAVY_BALL
	.2byte ITEM_MOON_BALL
	.2byte ITEM_FAST_BALL
	.2byte ITEM_LEVEL_BALL
	pokemartlistend
