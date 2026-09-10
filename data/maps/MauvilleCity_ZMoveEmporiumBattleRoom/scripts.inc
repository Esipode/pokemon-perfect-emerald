MauvilleCity_ZMoveEmporiumBattleRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MauvilleCity_ZMoveEmporiumBattleRoom_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, MauvilleCity_ZMoveEmporiumBattleRoom_OnFrame
	.byte 0

@ Redirect hygiene: a save reloaded inside the battle room clears the EWRAM
@ gEmporiumBattleActive but not VAR_EMPORIUM_* or the shown challenger.
MauvilleCity_ZMoveEmporiumBattleRoom_OnTransition:
	callnative EmporiumBattleRoomOnTransition
	setflag FLAG_TEMP_HIDE_FOLLOWER @ The arena scene scripts the player's steps
	end

@ Walk the player and challenger into the arena on entry, then start the fight.
@ VAR_EMPORIUM_ID is EMPORIUM_NONE only on a stale reload, where the transition
@ script above has already re-hidden the challenger.
MauvilleCity_ZMoveEmporiumBattleRoom_OnFrame:
	map_script_2 VAR_TEMP_0, 0, MauvilleCity_ZMoveEmporiumBattleRoom_EventScript_EnterArena
	.2byte 0

MauvilleCity_ZMoveEmporiumBattleRoom_EventScript_EnterArena::
	setvar VAR_TEMP_0, 1
	goto_if_eq VAR_EMPORIUM_ID, EMPORIUM_NONE, MauvilleCity_ZMoveEmporiumBattleRoom_EventScript_EnterArenaDone
	callnative EmporiumTraceArena
	lockall
	setvar VAR_0x8005, 1
	callnative EmporiumTraceStep
	applymovement LOCALID_ZMOVE_EMPORIUM_CHALLENGER, Emporium_Movement_ChallengerEnterArena
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerEnterArena
	setvar VAR_0x8005, 2
	callnative EmporiumTraceStep
	waitmovement 0
	setvar VAR_0x8005, 3
	callnative EmporiumTraceStep
	goto MauvilleCity_ZMoveEmporiumBattleRoom_EventScript_Challenger

MauvilleCity_ZMoveEmporiumBattleRoom_EventScript_EnterArenaDone:
	end

@ The runtime TRAINER_EMPORIUM fight: skipFlagCheck so it repeats, continueScript
@ so the outcome is handled here. Reached from EnterArena (normal path) or by
@ talking to the challenger (fallback). See data/scripts/battle_emporium.inc for
@ the shared outcome record and the lobby reward payout.
MauvilleCity_ZMoveEmporiumBattleRoom_EventScript_Challenger::
	setvar VAR_0x8005, 4
	callnative EmporiumTraceStep
	callnative EmporiumArmNoWhiteout
	special EmporiumBufferChallengerIntro
	trainerbattle LOCALID_ZMOVE_EMPORIUM_CHALLENGER, TRAINER_EMPORIUM, Emporium_Text_ChallengerIntroTemplate, Emporium_Text_ZMoveChallengerDefeat, NULL, LOCALID_NONE, TRAINER_NONE, NULL, NULL, NULL, Emporium_Text_ZMoveChallengerVictory, NULL, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE
	applymovement LOCALID_ZMOVE_EMPORIUM_CHALLENGER, Emporium_Movement_ChallengerExitArena
	waitmovement 0
	setflag FLAG_EMPORIUM_ZMOVE_CHALLENGER_HIDDEN
	removeobject LOCALID_ZMOVE_EMPORIUM_CHALLENGER
	call Emporium_EventScript_RecordChallengerOutcome
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerExitArena
	waitmovement 0
	warp MAP_MAUVILLE_CITY_ZMOVE_EMPORIUM_LOBBY, 2
	waitstate
	end
