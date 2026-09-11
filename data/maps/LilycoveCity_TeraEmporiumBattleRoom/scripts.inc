LilycoveCity_TeraEmporiumBattleRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LilycoveCity_TeraEmporiumBattleRoom_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, LilycoveCity_TeraEmporiumBattleRoom_OnFrame
	.byte 0

@ Redirect hygiene: a save reloaded inside the battle room clears the EWRAM
@ gEmporiumBattleActive but not VAR_EMPORIUM_* or the shown challenger.
LilycoveCity_TeraEmporiumBattleRoom_OnTransition:
	callnative EmporiumBattleRoomOnTransition
	setflag FLAG_TEMP_HIDE_FOLLOWER @ The arena scene scripts the player's steps
	end

@ Walk the player and challenger into the arena on entry, then start the fight.
@ VAR_EMPORIUM_ID is EMPORIUM_NONE only on a stale reload, where the transition
@ script above has already re-hidden the challenger.
LilycoveCity_TeraEmporiumBattleRoom_OnFrame:
	map_script_2 VAR_TEMP_0, 0, LilycoveCity_TeraEmporiumBattleRoom_EventScript_EnterArena
	.2byte 0

LilycoveCity_TeraEmporiumBattleRoom_EventScript_EnterArena::
	setvar VAR_TEMP_0, 1
	goto_if_eq VAR_EMPORIUM_ID, EMPORIUM_NONE, LilycoveCity_TeraEmporiumBattleRoom_EventScript_EnterArenaDone
	callnative EmporiumTraceArena
	lockall
	setvar VAR_0x8005, 1
	callnative EmporiumTraceStep
	applymovement LOCALID_TERA_EMPORIUM_CHALLENGER, Emporium_Movement_ChallengerEnterArena
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerEnterArena
	setvar VAR_0x8005, 2
	callnative EmporiumTraceStep
	waitmovement 0
	setvar VAR_0x8005, 3
	callnative EmporiumTraceStep
	goto LilycoveCity_TeraEmporiumBattleRoom_EventScript_Challenger

LilycoveCity_TeraEmporiumBattleRoom_EventScript_EnterArenaDone:
	end

@ The runtime TRAINER_EMPORIUM fight: skipFlagCheck so it repeats, continueScript
@ so the outcome is handled here. Reached from EnterArena (normal path) or by
@ talking to the challenger (fallback). See data/scripts/battle_emporium.inc for
@ the shared outcome record and the lobby reward payout.
LilycoveCity_TeraEmporiumBattleRoom_EventScript_Challenger::
	setvar VAR_0x8005, 4
	callnative EmporiumTraceStep
	callnative EmporiumArmNoWhiteout
	special EmporiumBufferChallengerIntro
	trainerbattle LOCALID_TERA_EMPORIUM_CHALLENGER, TRAINER_EMPORIUM, Emporium_Text_ChallengerIntroTemplate, Emporium_Text_TeraChallengerDefeat, NULL, LOCALID_NONE, TRAINER_NONE, NULL, NULL, NULL, Emporium_Text_TeraChallengerVictory, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE
	applymovement LOCALID_TERA_EMPORIUM_CHALLENGER, Emporium_Movement_ChallengerExitArena
	waitmovement 0
	setflag FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN
	removeobject LOCALID_TERA_EMPORIUM_CHALLENGER
	call Emporium_EventScript_RecordChallengerOutcome
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerExitArena
	waitmovement 0
	warp MAP_LILYCOVE_CITY_TERA_EMPORIUM_LOBBY, 2
	waitstate
	end
