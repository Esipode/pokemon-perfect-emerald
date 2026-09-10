LilycoveCity_TeraEmporiumBattleRoom_MapScripts::
	.byte 0

@ Stage 6 placeholder. The challenger object is hidden by
@ FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN until a reward is selected; Stage 8 adds
@ the trainerbattle TRAINER_EMPORIUM invocation and the reward payout here.
LilycoveCity_TeraEmporiumBattleRoom_EventScript_Challenger::
	lock
	faceplayer
	msgbox LilycoveCity_TeraEmporiumBattleRoom_Text_ChallengerPlaceholder, MSGBOX_DEFAULT
	release
	end

LilycoveCity_TeraEmporiumBattleRoom_Text_ChallengerPlaceholder:
	.string "Let's battle!\p"
	.string "(The Emporium battle arrives in a\n"
	.string "later update.)$"
