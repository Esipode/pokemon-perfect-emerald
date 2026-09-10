LilycoveCity_TeraEmporiumBattleRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LilycoveCity_TeraEmporiumBattleRoom_OnTransition
	.byte 0

@ Redirect hygiene: a save reloaded inside the battle room clears the EWRAM
@ gEmporiumBattleActive but not VAR_EMPORIUM_* or the shown challenger.
LilycoveCity_TeraEmporiumBattleRoom_OnTransition:
	callnative EmporiumBattleRoomOnTransition
	end

@ The challenger object is hidden by FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN until
@ the instructor rolls one (EmporiumShowChallenger). Talking to it fights the
@ runtime TRAINER_EMPORIUM: skipFlagCheck so it repeats, continueScript so the
@ outcome is handled here. See data/scripts/battle_emporium.inc for the shared
@ win / loss tails.
LilycoveCity_TeraEmporiumBattleRoom_EventScript_Challenger::
	callnative EmporiumArmNoWhiteout
	trainerbattle LOCALID_NONE, TRAINER_EMPORIUM, Emporium_Text_ChallengerIntro, Emporium_Text_ChallengerDefeat, NULL, LOCALID_NONE, TRAINER_NONE, NULL, NULL, NULL, Emporium_Text_ChallengerVictory, NULL, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE
	setflag FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN
	removeobject LOCALID_TERA_EMPORIUM_CHALLENGER
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, Emporium_EventScript_ChallengerDefeated
	call Emporium_EventScript_ChallengerLostCommon
	warp MAP_LILYCOVE_CITY_TERA_EMPORIUM_LOBBY, 2
	waitstate
	end
