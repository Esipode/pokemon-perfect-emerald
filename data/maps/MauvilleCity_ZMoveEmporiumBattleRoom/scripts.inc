MauvilleCity_ZMoveEmporiumBattleRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MauvilleCity_ZMoveEmporiumBattleRoom_OnTransition
	.byte 0

@ Redirect hygiene: a save reloaded inside the battle room clears the EWRAM
@ gEmporiumBattleActive but not VAR_EMPORIUM_* or the shown challenger.
MauvilleCity_ZMoveEmporiumBattleRoom_OnTransition:
	callnative EmporiumBattleRoomOnTransition
	end

@ The challenger object is hidden by FLAG_EMPORIUM_ZMOVE_CHALLENGER_HIDDEN until
@ the instructor rolls one (EmporiumShowChallenger). Talking to it fights the
@ runtime TRAINER_EMPORIUM: skipFlagCheck so it repeats, continueScript so the
@ outcome is handled here. See data/scripts/battle_emporium.inc for the shared
@ win / loss tails.
MauvilleCity_ZMoveEmporiumBattleRoom_EventScript_Challenger::
	callnative EmporiumArmNoWhiteout
	special EmporiumBufferChallengerIntro
	trainerbattle LOCALID_NONE, TRAINER_EMPORIUM, Emporium_Text_ChallengerIntroTemplate, Emporium_Text_ZMoveChallengerDefeat, NULL, LOCALID_NONE, TRAINER_NONE, NULL, NULL, NULL, Emporium_Text_ZMoveChallengerVictory, NULL, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE
	setflag FLAG_EMPORIUM_ZMOVE_CHALLENGER_HIDDEN
	removeobject LOCALID_ZMOVE_EMPORIUM_CHALLENGER
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, Emporium_EventScript_ChallengerDefeated
	call Emporium_EventScript_ChallengerLostCommon
	warp MAP_MAUVILLE_CITY_ZMOVE_EMPORIUM_LOBBY, 2
	waitstate
	end
