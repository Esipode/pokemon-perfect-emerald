Route123_MegaEmporiumBattleRoom_MapScripts::
	.byte 0

@ Stage 6 placeholder. The challenger object is hidden by
@ FLAG_EMPORIUM_MEGA_CHALLENGER_HIDDEN until a reward is selected; Stage 8 adds
@ the trainerbattle TRAINER_EMPORIUM invocation and the reward payout here.
Route123_MegaEmporiumBattleRoom_EventScript_Challenger::
	lock
	faceplayer
	msgbox Route123_MegaEmporiumBattleRoom_Text_ChallengerPlaceholder, MSGBOX_DEFAULT
	release
	end

Route123_MegaEmporiumBattleRoom_Text_ChallengerPlaceholder:
	.string "Let's battle!\p"
	.string "(The Emporium battle arrives in a\n"
	.string "later update.)$"
