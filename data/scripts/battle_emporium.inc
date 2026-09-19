@ Battle Emporium — shared instructor flow: reward menu, opponent roll, hand-off,
@ battle outcome and lobby reward payout. Per-building lobby scripts set VAR_0x8004
@ to the enum EmporiumId, show the one-time intro, then jump to
@ Emporium_EventScript_OpenMenu. VAR_0x8004 stays set for the whole interaction;
@ VAR_EMPORIUM_ID / VAR_EMPORIUM_REWARD are written only once the player commits,
@ and the redirect (gEmporiumBattleActive) is armed by EmporiumRollChallenger.
@
@ All three lobbies share the same geometry: instructor object is local id 1 at
@ (7,10), approached from the south; the north door to the battle room is warp 2 at
@ (7,4). All three battle rooms: challenger object is local id 1 at (11,2); the
@ exit door back to the lobby is warp 0 at (2,8). The movement templates below
@ assume that layout, and assume the (7,4) lobby tile has a door metatile
@ behaviour so the warp-exit step drops the returning player on (7,5).

.set LOCALID_EMPORIUM_INSTRUCTOR, 1

Emporium_EventScript_OpenMenu::
	special EmporiumMenu_BuildList
	goto_if_eq VAR_RESULT, 0, Emporium_EventScript_NoRewardsAvailable
	message Emporium_Text_WhichPrize
	waitmessage
	dynmultistack 0, 0, FALSE, EMPORIUM_MENU_PAGE_SIZE, FALSE, 0, DYN_MULTICHOICE_CB_SHOW_ITEM
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Emporium_EventScript_Goodbye
	special EmporiumMenu_CommitReward
	goto_if_eq VAR_RESULT, FALSE, Emporium_EventScript_OpenMenu
	special EmporiumMenu_BufferConfirm
	msgbox Emporium_Text_Confirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Emporium_EventScript_OpenMenu
	checkmoney EMPORIUM_ENTRY_FEE
	goto_if_eq VAR_RESULT, FALSE, Emporium_EventScript_CantAffordFee
	showmoneybox 0, 0
	removemoney EMPORIUM_ENTRY_FEE
	updatemoneybox
	playse SE_SHOP
	msgbox Emporium_Text_FeePaid, MSGBOX_DEFAULT
	hidemoneybox
	closemessage
	copyvar VAR_EMPORIUM_ID, VAR_0x8004
	special EmporiumRollChallenger
	special EmporiumShowChallenger
	applymovement LOCALID_EMPORIUM_INSTRUCTOR, Emporium_Movement_InstructorStepAside
	waitmovement 0
	setobjectxyperm LOCALID_EMPORIUM_INSTRUCTOR, 8, 9
	setobjectmovementtype LOCALID_EMPORIUM_INSTRUCTOR, MOVEMENT_TYPE_FACE_LEFT
	goto Emporium_EventScript_WalkPlayerToChallengerRoom

@ Shared tail: walk the player through the north door and warp to the pending
@ challenge's battle room. Entered from Emporium_EventScript_OpenMenu (first
@ attempt) and Emporium_EventScript_LobbyRecovery (retry after a loss). The
@ instructor is already stepped aside on both paths.
Emporium_EventScript_WalkPlayerToChallengerRoom:
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerToBattleRoom
	waitmovement 0
	opendoor 7, 4
	waitdooranim
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerEnterDoor
	waitmovement 0
	closedoor 7, 4
	waitdooranim
	switch VAR_EMPORIUM_ID
	case EMPORIUM_ZMOVE, Emporium_EventScript_WarpZMoveRoom
	case EMPORIUM_MEGA, Emporium_EventScript_WarpMegaRoom
	case EMPORIUM_TERA, Emporium_EventScript_WarpTeraRoom
	end

Emporium_EventScript_WarpZMoveRoom:
	warp MAP_MAUVILLE_CITY_ZMOVE_EMPORIUM_BATTLE_ROOM, 0
	waitstate
	end

Emporium_EventScript_WarpMegaRoom:
	warp MAP_ROUTE123_MEGA_EMPORIUM_BATTLE_ROOM, 0
	waitstate
	end

Emporium_EventScript_WarpTeraRoom:
	warp MAP_LILYCOVE_CITY_TERA_EMPORIUM_BATTLE_ROOM, 0
	waitstate
	end

Emporium_EventScript_CantAffordFee::
	msgbox Emporium_Text_CantAffordFee, MSGBOX_DEFAULT
	goto Emporium_EventScript_OpenMenu

@ ---- Challenger battle outcome ----
@ Called inline from each battle room's EnterArena script once the continueScript
@ trainerbattle returns. Records the result in VAR_EMPORIUM_RESULT for the lobby
@ ON_FRAME script, then the room script walks the player out and warps to the lobby.
@ On a loss the party is healed here (facility-style no-white-out path); on a win it
@ is healed by Emporium_EventScript_LobbyReward. Both heals go through
@ HealPlayerParty, so Nuzlocke fainted-mon removal still applies.
@ VAR_EMPORIUM_ID / VAR_EMPORIUM_REWARD survive the warp on both outcomes: a win
@ pays out and clears them; a loss keeps them for Emporium_EventScript_LobbyRecovery
@ until the retry is declined (ClearEmporiumBattle). The redirect is disarmed by
@ EmporiumLobbyOnTransition on either path.

Emporium_EventScript_RecordChallengerOutcome::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, Emporium_EventScript_RecordChallengerWon
	special HealPlayerParty
	setvar VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_LOST
	return

Emporium_EventScript_RecordChallengerWon::
	setvar VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_WON
	return

@ Runs from every lobby's ON_FRAME table (unconditionally on ClearTempFieldEventData,
@ but the vars aren't temp). VAR_EMPORIUM_RESULT is only non-zero right after a
@ finished battle; the scene clears it so it runs once.

Emporium_EventScript_LobbyReward::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerReturnToInstructor
	waitmovement 0
	applymovement LOCALID_EMPORIUM_INSTRUCTOR, Emporium_Movement_InstructorReturn
	waitmovement 0
	setobjectxyperm LOCALID_EMPORIUM_INSTRUCTOR, 8, 9
	setobjectmovementtype LOCALID_EMPORIUM_INSTRUCTOR, MOVEMENT_TYPE_FACE_DOWN
	special HealPlayerParty
	special EmporiumBufferRewardItem
	msgbox Emporium_Text_RewardWon, MSGBOX_DEFAULT
	giveitem VAR_0x8004
	msgbox Emporium_Text_VisitAgain, MSGBOX_DEFAULT
	callnative ClearEmporiumBattle
	setvar VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_NONE
	closemessage
	releaseall
	end

@ Post-loss scene. The party was already healed in Emporium_EventScript_Record
@ ChallengerOutcome. The player is offered a retry of the same challenge; a retry
@ charges the fee again and re-enters the battle room through the shared walk-in
@ tail, with the instructor still stepped aside (LobbyRestoreInstructorAside).
@ Declining or being unable to pay walks the instructor back, says goodbye and
@ clears the pending selection.
Emporium_EventScript_LobbyRecovery::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, Emporium_Movement_PlayerReturnToInstructor
	waitmovement 0
	msgbox Emporium_Text_ChallengerLost, MSGBOX_DEFAULT
	msgbox Emporium_Text_RetryPrompt, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Emporium_EventScript_RetryChallenge
	goto Emporium_EventScript_RecoveryGoodbye

Emporium_EventScript_RetryChallenge:
	setvar VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_NONE
	checkmoney EMPORIUM_ENTRY_FEE
	goto_if_eq VAR_RESULT, FALSE, Emporium_EventScript_RetryCantAffordFee
	showmoneybox 0, 0
	removemoney EMPORIUM_ENTRY_FEE
	updatemoneybox
	playse SE_SHOP
	msgbox Emporium_Text_FeePaid, MSGBOX_DEFAULT
	hidemoneybox
	closemessage
	special EmporiumRollChallenger
	special EmporiumShowChallenger
	goto Emporium_EventScript_WalkPlayerToChallengerRoom

Emporium_EventScript_RetryCantAffordFee:
	msgbox Emporium_Text_CantAffordFee, MSGBOX_DEFAULT
	goto Emporium_EventScript_RecoveryGoodbye

Emporium_EventScript_RecoveryGoodbye:
	applymovement LOCALID_EMPORIUM_INSTRUCTOR, Emporium_Movement_InstructorReturn
	waitmovement 0
	setobjectxyperm LOCALID_EMPORIUM_INSTRUCTOR, 8, 9
	setobjectmovementtype LOCALID_EMPORIUM_INSTRUCTOR, MOVEMENT_TYPE_FACE_DOWN
	msgbox Emporium_Text_VisitAgain, MSGBOX_DEFAULT
	callnative ClearEmporiumBattle
	setvar VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_NONE
	closemessage
	releaseall
	end

@ Movement templates (see the geometry note at the top of this file).

Emporium_Movement_InstructorStepAside:
	walk_up
	walk_right
	face_left
	step_end

@ Stops on (7,5), the tile below the door, so the door animation can play before
@ Emporium_Movement_PlayerEnterDoor steps through it.
Emporium_Movement_PlayerToBattleRoom:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

Emporium_Movement_PlayerEnterDoor:
	walk_up
	set_invisible
	step_end

Emporium_Movement_ChallengerEnterArena:
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	step_end

Emporium_Movement_PlayerEnterArena:
	walk_up
	walk_up
	walk_up
	walk_right
	walk_right
	walk_right
	step_end

@ Reverse of Emporium_Movement_ChallengerEnterArena: the challenger walks back to
@ their spawn tile before the room script removes the object, so the player sees
@ them leave instead of the sprite vanishing in place.
Emporium_Movement_ChallengerExitArena:
	walk_right
	walk_right
	walk_right
	walk_up
	walk_up
	walk_up
	step_end

Emporium_Movement_PlayerExitArena:
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	step_end

Emporium_Movement_PlayerReturnToInstructor:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	face_up
	step_end

Emporium_Movement_InstructorReturn:
	walk_left
	walk_down
	step_end

@ Called from every lobby's ON_TRANSITION. Object event templates reload from the
@ map header on each map load, so the stepped-aside position is lost on return from
@ the battle room. Re-apply it before the objects spawn (VAR_EMPORIUM_RESULT is
@ non-zero only on that trip) so Emporium_Movement_InstructorReturn starts from
@ the right tile.

Emporium_EventScript_LobbyRestoreInstructorAside::
	goto_if_eq VAR_EMPORIUM_RESULT, EMPORIUM_RESULT_NONE, Emporium_EventScript_LobbyRestoreInstructorDone
	setobjectxyperm LOCALID_EMPORIUM_INSTRUCTOR, 8, 9
	setobjectmovementtype LOCALID_EMPORIUM_INSTRUCTOR, MOVEMENT_TYPE_FACE_LEFT

Emporium_EventScript_LobbyRestoreInstructorDone:
	return

Emporium_EventScript_Goodbye::
	closemessage
	callnative ClearEmporiumBattle
	release
	end

Emporium_EventScript_NoRewardsAvailable::
	callnative ClearEmporiumBattle
	msgbox Emporium_Text_NoRewardsAvailable, MSGBOX_DEFAULT
	release
	end

Emporium_Text_WhichPrize:
	.string "Which prize has caught your eye?$"

Emporium_Text_Confirm:
	.string "The {STR_VAR_1}, then.\p"
	.string "Beat the challenger waiting in back\n"
	.string "and it's yours to keep.\p"
	.string "The entry fee is ¥2,500.\n"
	.string "Ready to battle?$"

Emporium_Text_VisitAgain:
	.string "A pleasure doing business.\p"
	.string "Come back whenever you want\n"
	.string "another shot at the prizes.$"

Emporium_Text_NoRewardsAvailable:
	.string "You already hold every prize this\n"
	.string "Emporium has to offer.\p"
	.string "Come back if you part with one.$"

Emporium_Text_CantAffordFee:
	.string "The entry fee is ¥2,500, and it\n"
	.string "looks like you're a little short.$"

Emporium_Text_FeePaid:
	.string "Thanks! The challenger is warming\n"
	.string "up for you now.$"

@ Challenger battle lines shown by the battle engine. The intro is rolled per
@ attempt: BuildEmporiumTrainer picks the index and EmporiumBufferChallengerIntro
@ copies the line into gStringVar1, which the template below prints. Defeat and
@ victory lines are per building (the room script passes its own).
Emporium_Text_ChallengerIntroTemplate:
	.string "{STR_VAR_1}$"

Emporium_Text_ChallengerIntro1::
	.string "So you're my challenge today?\n"
	.string "Let's see what you've got!$"

Emporium_Text_ChallengerIntro2::
	.string "I've been after a real battle.\n"
	.string "You'll do just fine.$"

Emporium_Text_ChallengerIntro3::
	.string "You want that prize?\n"
	.string "Then get past me first.$"

Emporium_Text_ChallengerIntro4::
	.string "Show or not, I don't do easy\n"
	.string "battles. Brace yourself.$"

Emporium_Text_ChallengerIntro5::
	.string "My team has waited all day.\n"
	.string "Let's not disappoint them.$"

Emporium_Text_ChallengerIntro6::
	.string "Step up. Show me the battle\n"
	.string "you came here for.$"

Emporium_Text_ZMoveChallengerDefeat:
	.string "That Z-Power was yours in the\n"
	.string "end. The crystal is yours.$"

Emporium_Text_ZMoveChallengerVictory:
	.string "The crystal stays here today.\n"
	.string "Come back and try again.$"

Emporium_Text_MegaChallengerDefeat:
	.string "Your bond beat my Mega\n"
	.string "Evolution. Take the stone.$"

Emporium_Text_MegaChallengerVictory:
	.string "Not enough to top a Mega\n"
	.string "today. Come back stronger.$"

Emporium_Text_TeraChallengerDefeat:
	.string "You read the Tera type well.\n"
	.string "The shard is yours.$"

Emporium_Text_TeraChallengerVictory:
	.string "The Tera gamble paid off for\n"
	.string "me today. Try again.$"

Emporium_Text_RewardWon:
	.string "A deal's a deal. Here's the\n"
	.string "{STR_VAR_1} you earned!$"

Emporium_Text_ChallengerLost:
	.string "You and your team have been\n"
	.string "patched up.$"

Emporium_Text_RetryPrompt:
	.string "Care to take on the challenger\n"
	.string "again?\p"
	.string "The entry fee is another ¥2,500.\n"
	.string "Want to try again?$"
