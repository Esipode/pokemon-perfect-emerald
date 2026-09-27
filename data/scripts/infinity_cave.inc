@ Shared Infinity Cave scripts. Map-local lobby scripts live in
@ data/maps/InfinityCave_Entrance/scripts.inc.

@ Carves the cave mouth into EverGrandeCity at (24, 43). Called from that map's
@ ON_RESUME once FLAG_SYS_GAME_CLEAR is set, so the entrance does not exist
@ before the Hall of Fame. The tiles are the generic General-tileset cave mouth;
@ swap them for EverGrande-specific art once it is drawn.
InfinityCave_EventScript_OpenEntrance::
	setmetatilewithelevation 24, 42, METATILE_General_CaveEntrance_Top, TRUE, 0
	setmetatilewithelevation 24, 43, METATILE_General_CaveEntrance_Bottom, FALSE, 0
	return

@ The descent ladder the generator draws on every room's exit pad. It is a step
@ trigger, not an object: TryStartInfCaveExitScript (src/field_control_avatar.c)
@ starts this when the player walks onto that tile. Descending advances the depth,
@ which re-rolls the room seed, then warps back into the same map so the next room
@ is generated. The room map has no warp of its own, so this is also the way out:
@ the lobby's ON_TRANSITION closes the run.
InfinityCave_EventScript_ExitLadder::
	lockall
	specialvar VAR_RESULT, InfCave_IsExitLocked
	goto_if_eq VAR_RESULT, TRUE, InfinityCave_EventScript_LadderSealed
	msgbox InfinityCave_Text_LadderAskDescend, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, InfinityCave_EventScript_LadderAskLeave
	closemessage
	special InfCave_AdvanceDepth
	warp MAP_INFINITY_CAVE_ROOM, 0, 0
	waitstate
	releaseall
	end

InfinityCave_EventScript_LadderAskLeave::
	msgbox InfinityCave_Text_LadderAskLeave, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, InfinityCave_EventScript_LadderDone
	closemessage
	warp MAP_INFINITY_CAVE_ENTRANCE, 0
	waitstate
	releaseall
	end

InfinityCave_EventScript_LadderDone::
	releaseall
	end

@ A boss room's ladder refuses both descending and leaving while the boss is
@ standing, so the arena fight is the only way on.
InfinityCave_EventScript_LadderSealed::
	msgbox InfinityCave_Text_LadderSealed, MSGBOX_DEFAULT
	releaseall
	end

InfinityCave_Text_LadderAskDescend:
	.string "A ladder drops away into the dark\n"
	.string "below.\p"
	.string "Go deeper?$"

InfinityCave_Text_LadderAskLeave:
	.string "Leave the cave instead?$"

InfinityCave_Text_LadderSealed:
	.string "Rubble chokes the ladder shaft.\p"
	.string "Something in this chamber still holds\n"
	.string "this floor shut.$"

@ The rolled trainers the generator stands on the room's floor. Their templates
@ carry TRAINER_TYPE_NORMAL and a sight range, so the challenge is sight-based;
@ trainerbattle takes a literal id and the approach code reads that id back out of
@ the script, so every slot needs its own copy. Every slot shares one payout
@ script: the shards a win pays come from the room, not the slot. A loss never
@ reaches here - B_FLAG_NO_WHITEOUT is armed when the room generates, and
@ src/battle_setup.c sends a cave defeat straight back to the lobby.
InfinityCave_EventScript_Trainer0::
	trainerbattle_single TRAINER_INFCAVE_0, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_EventScript_Trainer1::
	trainerbattle_single TRAINER_INFCAVE_1, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_EventScript_Trainer2::
	trainerbattle_single TRAINER_INFCAVE_2, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_EventScript_Trainer3::
	trainerbattle_single TRAINER_INFCAVE_3, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_EventScript_Trainer4::
	trainerbattle_single TRAINER_INFCAVE_4, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_EventScript_Trainer5::
	trainerbattle_single TRAINER_INFCAVE_5, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_EventScript_Trainer6::
	trainerbattle_single TRAINER_INFCAVE_6, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_EventScript_Trainer7::
	trainerbattle_single TRAINER_INFCAVE_7, InfinityCave_Text_TrainerIntro, InfinityCave_Text_TrainerDefeat, InfinityCave_EventScript_TrainerPayout
	end

InfinityCave_Text_TrainerIntro:
	.string "Another one, come down to test the\n"
	.string "dark. Show me what you brought!$"

InfinityCave_Text_TrainerDefeat:
	.string "The cave takes its own back.$"

@ Post-battle payout. InfCave_PayBattleShards adds the room's shard reward; no
@ message, the start menu's tracker box carries the running total.
InfinityCave_EventScript_TrainerPayout::
	special InfCave_PayBattleShards
	end

@ The boss a depth divisible by INFCAVE_BOSS_INTERVAL stands on its arena. It is
@ written with TRAINER_TYPE_NONE and no sight range, so it never notices the player
@ and never walks over: the player crosses the arena and talks to it. The room's
@ ladder stays sealed until it is beaten, so the fight cannot be skipped.
@ goto_if_defeated covers talking to a boss already beaten.
@ trainerbattle_no_intro continues this script after a win, and a loss never
@ returns here: it ends the run from src/battle_setup.c.
@
@ Each of the three beats - challenge, defeat, post-battle - rolls one of 12 lines
@ independently. The defeat line has to be a literal pointer, because
@ trainerbattle_no_intro reads it out of the script as it sets the battle up, so
@ that roll picks between 12 copies of the battle call rather than one call with a
@ chosen text.
InfinityCave_EventScript_Boss::
	lock
	goto_if_defeated TRAINER_INFCAVE_BOSS, InfinityCave_EventScript_BossDone
	faceplayer
	applymovement VAR_LAST_TALKED, InfinityCave_Movement_BossNotice
	waitmovement 0
	delay 24
	@ Trace probe for the duplicate-NPC hunt; no-op unless INFCAVE_TRACE is on.
	special InfCave_DebugDumpObjects
	random 12
	switch VAR_RESULT
	case 0, InfinityCave_EventScript_BossChallenge0
	case 1, InfinityCave_EventScript_BossChallenge1
	case 2, InfinityCave_EventScript_BossChallenge2
	case 3, InfinityCave_EventScript_BossChallenge3
	case 4, InfinityCave_EventScript_BossChallenge4
	case 5, InfinityCave_EventScript_BossChallenge5
	case 6, InfinityCave_EventScript_BossChallenge6
	case 7, InfinityCave_EventScript_BossChallenge7
	case 8, InfinityCave_EventScript_BossChallenge8
	case 9, InfinityCave_EventScript_BossChallenge9
	case 10, InfinityCave_EventScript_BossChallenge10
	case 11, InfinityCave_EventScript_BossChallenge11
	goto InfinityCave_EventScript_BossChallenge0

InfinityCave_EventScript_BossChallenge0::
	msgbox InfinityCave_Text_BossChallenge0, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge1::
	msgbox InfinityCave_Text_BossChallenge1, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge2::
	msgbox InfinityCave_Text_BossChallenge2, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge3::
	msgbox InfinityCave_Text_BossChallenge3, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge4::
	msgbox InfinityCave_Text_BossChallenge4, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge5::
	msgbox InfinityCave_Text_BossChallenge5, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge6::
	msgbox InfinityCave_Text_BossChallenge6, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge7::
	msgbox InfinityCave_Text_BossChallenge7, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge8::
	msgbox InfinityCave_Text_BossChallenge8, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge9::
	msgbox InfinityCave_Text_BossChallenge9, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge10::
	msgbox InfinityCave_Text_BossChallenge10, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossChallenge11::
	msgbox InfinityCave_Text_BossChallenge11, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossFight

InfinityCave_EventScript_BossFight::
	closemessage
	random 12
	switch VAR_RESULT
	case 0, InfinityCave_EventScript_BossFight0
	case 1, InfinityCave_EventScript_BossFight1
	case 2, InfinityCave_EventScript_BossFight2
	case 3, InfinityCave_EventScript_BossFight3
	case 4, InfinityCave_EventScript_BossFight4
	case 5, InfinityCave_EventScript_BossFight5
	case 6, InfinityCave_EventScript_BossFight6
	case 7, InfinityCave_EventScript_BossFight7
	case 8, InfinityCave_EventScript_BossFight8
	case 9, InfinityCave_EventScript_BossFight9
	case 10, InfinityCave_EventScript_BossFight10
	case 11, InfinityCave_EventScript_BossFight11
	goto InfinityCave_EventScript_BossFight0

InfinityCave_EventScript_BossFight0::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat0
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight1::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat1
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight2::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat2
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight3::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat3
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight4::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat4
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight5::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat5
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight6::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat6
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight7::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat7
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight8::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat8
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight9::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat9
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight10::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat10
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossFight11::
	trainerbattle_no_intro TRAINER_INFCAVE_BOSS, InfinityCave_Text_BossDefeat11
	goto InfinityCave_EventScript_BossWon

InfinityCave_EventScript_BossWon::
	special InfCave_PayBattleShards
	goto InfinityCave_EventScript_BossDone

@ Also the talk-to path for a boss already beaten, which never pays out.
InfinityCave_EventScript_BossDone::
	random 12
	switch VAR_RESULT
	case 0, InfinityCave_EventScript_BossBeaten0
	case 1, InfinityCave_EventScript_BossBeaten1
	case 2, InfinityCave_EventScript_BossBeaten2
	case 3, InfinityCave_EventScript_BossBeaten3
	case 4, InfinityCave_EventScript_BossBeaten4
	case 5, InfinityCave_EventScript_BossBeaten5
	case 6, InfinityCave_EventScript_BossBeaten6
	case 7, InfinityCave_EventScript_BossBeaten7
	case 8, InfinityCave_EventScript_BossBeaten8
	case 9, InfinityCave_EventScript_BossBeaten9
	case 10, InfinityCave_EventScript_BossBeaten10
	case 11, InfinityCave_EventScript_BossBeaten11
	goto InfinityCave_EventScript_BossBeaten0

InfinityCave_EventScript_BossBeaten0::
	msgbox InfinityCave_Text_BossBeaten0, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten1::
	msgbox InfinityCave_Text_BossBeaten1, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten2::
	msgbox InfinityCave_Text_BossBeaten2, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten3::
	msgbox InfinityCave_Text_BossBeaten3, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten4::
	msgbox InfinityCave_Text_BossBeaten4, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten5::
	msgbox InfinityCave_Text_BossBeaten5, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten6::
	msgbox InfinityCave_Text_BossBeaten6, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten7::
	msgbox InfinityCave_Text_BossBeaten7, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten8::
	msgbox InfinityCave_Text_BossBeaten8, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten9::
	msgbox InfinityCave_Text_BossBeaten9, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten10::
	msgbox InfinityCave_Text_BossBeaten10, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossBeaten11::
	msgbox InfinityCave_Text_BossBeaten11, MSGBOX_DEFAULT
	goto InfinityCave_EventScript_BossRelease

InfinityCave_EventScript_BossRelease::
	closemessage
	release
	end

InfinityCave_Movement_BossNotice:
	emote_double_exclamation_mark
	delay_16
	step_end

@ Every boss is a Hoenn Leader, Elite Four member or Champion, so the lines stay
@ generic: no gender, class or team is named.
InfinityCave_Text_BossChallenge0:
	.string "Oh good, a visitor.\p"
	.string "I came down here for quiet, and\n"
	.string "instead I got you.\p"
	.string "Battle me and we're even.$"

InfinityCave_Text_BossChallenge1:
	.string "Careful, the floor is a little\n"
	.string "crunchy in places.\p"
	.string "Also I'm blocking the ladder.\n"
	.string "Nothing personal.$"

InfinityCave_Text_BossChallenge2:
	.string "Do you have any idea how long\n"
	.string "I've been down here?\p"
	.string "Neither do I. No clocks here.\p"
	.string "Battle me before I forget why I\n"
	.string "came.$"

InfinityCave_Text_BossChallenge3:
	.string "I was promised treasure.\p"
	.string "There is no treasure. There is\n"
	.string "rock, and now there is you.\p"
	.string "You'll do.$"

InfinityCave_Text_BossChallenge4:
	.string "Rule of the cave: whoever is\n"
	.string "standing here gets a battle.\p"
	.string "I don't make the rules. I just\n"
	.string "stand here.$"

InfinityCave_Text_BossChallenge5:
	.string "You came down this far without\n"
	.string "snacks? Amateur.\p"
	.string "Fine. Battle now, snacks later.$"

InfinityCave_Text_BossChallenge6:
	.string "Shh. Listen.\p"
	.string "...Nothing. Absolutely nothing.\p"
	.string "Great acoustics for a battle,\n"
	.string "though.$"

InfinityCave_Text_BossChallenge7:
	.string "I'd offer you a seat, but this\n"
	.string "cave only has rocks.\p"
	.string "Stand up and battle me instead.$"

InfinityCave_Text_BossChallenge8:
	.string "You're deeper than most people\n"
	.string "get.\p"
	.string "They usually turn back where it\n"
	.string "starts dripping.\p"
	.string "Show me you meant to come.$"

InfinityCave_Text_BossChallenge9:
	.string "Let me guess. You took one wrong\n"
	.string "turn and simply kept going.\p"
	.string "Same. Let's battle about it.$"

InfinityCave_Text_BossChallenge10:
	.string "My team's been bored for hours.\p"
	.string "They've started sorting pebbles\n"
	.string "by size.\p"
	.string "Please. Entertain them.$"

InfinityCave_Text_BossChallenge11:
	.string "A challenger! Excellent.\p"
	.string "I rehearsed a very intimidating\n"
	.string "speech for this moment.\p"
	.string "...I have forgotten all of it.\n"
	.string "Let's just battle.$"

InfinityCave_Text_BossDefeat0:
	.string "Well. That's one way past me.$"

InfinityCave_Text_BossDefeat1:
	.string "Straight through the middle.\n"
	.string "Rude. Effective.$"

InfinityCave_Text_BossDefeat2:
	.string "Take the ladder. Take it. Go on.$"

InfinityCave_Text_BossDefeat3:
	.string "I'm telling everyone the cave\n"
	.string "cheated on my behalf.$"

InfinityCave_Text_BossDefeat4:
	.string "No notes. That was better than\n"
	.string "me, and I was trying.$"

InfinityCave_Text_BossDefeat5:
	.string "I am going to sit on this rock\n"
	.string "for a while.$"

InfinityCave_Text_BossDefeat6:
	.string "Fine! Deeper you go.\p"
	.string "Send a postcard if the mail runs\n"
	.string "down there.$"

InfinityCave_Text_BossDefeat7:
	.string "That is the most fun I've had in\n"
	.string "complete darkness.$"

InfinityCave_Text_BossDefeat8:
	.string "You win. The rock and I will\n"
	.string "recover together.$"

InfinityCave_Text_BossDefeat9:
	.string "My pebbles. All that sorting,\n"
	.string "for nothing.$"

InfinityCave_Text_BossDefeat10:
	.string "That one is going straight into\n"
	.string "the story I tell later.$"

InfinityCave_Text_BossDefeat11:
	.string "I remembered the speech.\p"
	.string "It was 'please don't'.$"

InfinityCave_Text_BossBeaten0:
	.string "Ladder's clear. Mind the third\n"
	.string "rung, it has opinions.$"

InfinityCave_Text_BossBeaten1:
	.string "Still here. Still beaten.\n"
	.string "Kind of you to check.$"

InfinityCave_Text_BossBeaten2:
	.string "Go on down. I'm collecting\n"
	.string "pebbles again.$"

InfinityCave_Text_BossBeaten3:
	.string "If you meet anyone else down\n"
	.string "there, tell them I'm fine.$"

InfinityCave_Text_BossBeaten4:
	.string "I'd wish you luck, but you\n"
	.string "clearly don't need any.$"

InfinityCave_Text_BossBeaten5:
	.string "The dark is deeper than I am.\n"
	.string "Go bother that instead.$"

InfinityCave_Text_BossBeaten6:
	.string "Rematch? Absolutely not.\n"
	.string "I'm on a break.$"

InfinityCave_Text_BossBeaten7:
	.string "Onward, then. I'll hold this\n"
	.string "rock for you.$"

InfinityCave_Text_BossBeaten8:
	.string "You know the way. It's down.\n"
	.string "It is always down.$"

InfinityCave_Text_BossBeaten9:
	.string "I'm staying. Someone has to keep\n"
	.string "the cave company.$"

InfinityCave_Text_BossBeaten10:
	.string "Off you go. Try not to become a\n"
	.string "cautionary tale.$"

InfinityCave_Text_BossBeaten11:
	.string "The next floor is worse, you\n"
	.string "know.\p"
	.string "That wasn't a warning. Mostly.$"
