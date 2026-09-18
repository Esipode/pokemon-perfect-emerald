// Koraidon, "The Winged King Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Koraidon_Intro:
// 0 Phase (0/1/2), 1 Momentum (0-6), 2 SunTier (0 none/1 Primal/2 Intense/3 Ancient),
// 3 LastPlayerMoveType, 4 RepeatTypeCount (capped display at 2), 5 InstinctTelegraph
// (0 none/1 brace/2 pursue/3 rhythm), 6 RoarState, 7 CollisionState, 8 MomentumGuard,
// 9 InstinctGuard, 10 SwitchGuard, 11 TurnGuard, 12 LastAnnouncedTier, 13 Scratch.
//
// MOMENTUM IS GUARDED, NOT CLAMPED. Every mutation goes through _MomentumUp/_MomentumDown, which
// test the bound before touching the byte - Momentum is unsigned, so an unguarded subvar at 0
// would wrap to 255 rather than floor. That is the "one writer per number" idiom for a bounded
// counter (compare Reshiram's RevDown).
//
// INSTINCT NEEDS A TYPE-MATCH THAT NO CONDITION CAN EXPRESS. Conditions and encjumpifvar only
// ever compare a variable against a literal, never another variable (SPEC's own gotcha list), so
// "was that the same type as last turn" can't be one comparison. encstoretype (new this fight,
// mirrors encstoreprediction) writes the current move's base type into Scratch; the dispatch in
// _InstinctRead then jumps on Scratch's value and, in each arm, tests LastPlayerMoveType against
// that SAME literal - 18 real attacking types, TYPE_MYSTERY/TYPE_STELLAR/TYPE_NONE fall through to
// "no match" since no move in this project carries them as a base type worth tracking.
//
// EVERY OnMoveEnd/OnSwitchIn TRIGGER WITHOUT Once/OnEnter CLOSES ITS OWN LATCH on every path, set
// as the first line of its script, per the runaway-trigger-chain rule.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so playanimation always
// takes an explicit battler here, never playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE ONLY WRITERS of Momentum. Guard-then-mutate instead of clamp-after, because Momentum is an
// unsigned byte and a bare subvar at 0 wraps to 255 rather than floors. Silent; every caller
// supplies its own line.
EncScript_Koraidon_MomentumUp:
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Koraidon_MomentumUp_Done
	encaddvar 1, 1
EncScript_Koraidon_MomentumUp_Done:
	return

EncScript_Koraidon_MomentumDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Koraidon_MomentumDown_Done
	encsubvar 1, 1
EncScript_Koraidon_MomentumDown_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Koraidon_Intro::
	printstring STRINGID_ENCKORAIDONINTRO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	encsetweather BATTLE_WEATHER_SUN
	encsetvar 2, 1      @ SunTier: Primal
	encsetvar 12, 1     @ LastAnnouncedTier
	return

// Top of every turn. TurnGuard is set immediately so no branch out can leave it re-armed; the
// other per-turn latches (MomentumGuard/InstinctGuard/SwitchGuard) clear here too, per the
// authoring.md once-per-turn scaffold.
EncScript_Koraidon_TurnGuardSet::
	encsetvar 11, 1     @ TurnGuard
	encsetvar 8, 0      @ MomentumGuard
	encsetvar 9, 0      @ InstinctGuard
	encsetvar 10, 0     @ SwitchGuard
	return

EncScript_Koraidon_TurnGuardClear::
	encsetvar 11, 0
	return

// Koraidon's own move landed. Silent - Momentum's state is only surfaced through what it unlocks.
EncScript_Koraidon_MomentumGain::
	encsetvar 8, 1      @ MomentumGuard: closes immediately
	call EncScript_Koraidon_MomentumUp
	return

// Koraidon's own move missed or was blocked. Folded into one flat penalty rather than splitting
// a Protect-specific case: nothing in the engine can carry "was it Protect" from a condition into
// this script, and the fight's anti-abuse already answers Protect spam through Momentum denial
// generally (guideline 8), not a harsher miss-specific number.
EncScript_Koraidon_MomentumLoss::
	encsetvar 8, 1
	call EncScript_Koraidon_MomentumDown
	return

// A status lands on Koraidon. OnEnter already keeps this from re-firing every checkpoint the
// status happens to still be up.
EncScript_Koraidon_StatusBreak::
	call EncScript_Koraidon_MomentumDown
	return

// The Sun goes out from under it.
EncScript_Koraidon_SunLost::
	printstring STRINGID_ENCKORAIDONSUNLOST
	waitmessage B_WAIT_TIME_LONG
	encsetvar 2, 0      @ SunTier
	encsetvar 12, 0     @ LastAnnouncedTier
	call EncScript_Koraidon_MomentumDown
	return

// INSTINCT READ. Reads the player's last move: a repeated TYPE telegraphs RHYTHM (Koraidon
// answers in kind), anything else physical telegraphs BRACE. The 18-arm dispatch is the only way
// to compare two variables the engine has (see header comment) - Scratch holds this turn's type,
// each arm re-tests LastPlayerMoveType against the same literal that got it there.
EncScript_Koraidon_InstinctRead::
	encsetvar 9, 1      @ InstinctGuard: closes immediately, every path
	encstoretype 13     @ Scratch = this turn's move type
	encjumpifvar CMP_EQUAL, 13, TYPE_NORMAL, EncScript_Koraidon_InstinctRead_Normal
	encjumpifvar CMP_EQUAL, 13, TYPE_FIGHTING, EncScript_Koraidon_InstinctRead_Fighting
	encjumpifvar CMP_EQUAL, 13, TYPE_FLYING, EncScript_Koraidon_InstinctRead_Flying
	encjumpifvar CMP_EQUAL, 13, TYPE_POISON, EncScript_Koraidon_InstinctRead_Poison
	encjumpifvar CMP_EQUAL, 13, TYPE_GROUND, EncScript_Koraidon_InstinctRead_Ground
	encjumpifvar CMP_EQUAL, 13, TYPE_ROCK, EncScript_Koraidon_InstinctRead_Rock
	encjumpifvar CMP_EQUAL, 13, TYPE_BUG, EncScript_Koraidon_InstinctRead_Bug
	encjumpifvar CMP_EQUAL, 13, TYPE_GHOST, EncScript_Koraidon_InstinctRead_Ghost
	encjumpifvar CMP_EQUAL, 13, TYPE_STEEL, EncScript_Koraidon_InstinctRead_Steel
	encjumpifvar CMP_EQUAL, 13, TYPE_FIRE, EncScript_Koraidon_InstinctRead_Fire
	encjumpifvar CMP_EQUAL, 13, TYPE_WATER, EncScript_Koraidon_InstinctRead_Water
	encjumpifvar CMP_EQUAL, 13, TYPE_GRASS, EncScript_Koraidon_InstinctRead_Grass
	encjumpifvar CMP_EQUAL, 13, TYPE_ELECTRIC, EncScript_Koraidon_InstinctRead_Electric
	encjumpifvar CMP_EQUAL, 13, TYPE_PSYCHIC, EncScript_Koraidon_InstinctRead_Psychic
	encjumpifvar CMP_EQUAL, 13, TYPE_ICE, EncScript_Koraidon_InstinctRead_Ice
	encjumpifvar CMP_EQUAL, 13, TYPE_DRAGON, EncScript_Koraidon_InstinctRead_Dragon
	encjumpifvar CMP_EQUAL, 13, TYPE_DARK, EncScript_Koraidon_InstinctRead_Dark
	encjumpifvar CMP_EQUAL, 13, TYPE_FAIRY, EncScript_Koraidon_InstinctRead_Fairy
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Normal:
	encjumpifvar CMP_EQUAL, 3, TYPE_NORMAL, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Fighting:
	encjumpifvar CMP_EQUAL, 3, TYPE_FIGHTING, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Flying:
	encjumpifvar CMP_EQUAL, 3, TYPE_FLYING, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Poison:
	encjumpifvar CMP_EQUAL, 3, TYPE_POISON, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Ground:
	encjumpifvar CMP_EQUAL, 3, TYPE_GROUND, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Rock:
	encjumpifvar CMP_EQUAL, 3, TYPE_ROCK, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Bug:
	encjumpifvar CMP_EQUAL, 3, TYPE_BUG, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Ghost:
	encjumpifvar CMP_EQUAL, 3, TYPE_GHOST, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Steel:
	encjumpifvar CMP_EQUAL, 3, TYPE_STEEL, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Fire:
	encjumpifvar CMP_EQUAL, 3, TYPE_FIRE, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Water:
	encjumpifvar CMP_EQUAL, 3, TYPE_WATER, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Grass:
	encjumpifvar CMP_EQUAL, 3, TYPE_GRASS, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Electric:
	encjumpifvar CMP_EQUAL, 3, TYPE_ELECTRIC, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Psychic:
	encjumpifvar CMP_EQUAL, 3, TYPE_PSYCHIC, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Ice:
	encjumpifvar CMP_EQUAL, 3, TYPE_ICE, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Dragon:
	encjumpifvar CMP_EQUAL, 3, TYPE_DRAGON, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Dark:
	encjumpifvar CMP_EQUAL, 3, TYPE_DARK, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
EncScript_Koraidon_InstinctRead_Fairy:
	encjumpifvar CMP_EQUAL, 3, TYPE_FAIRY, EncScript_Koraidon_InstinctRead_Match
	goto EncScript_Koraidon_InstinctRead_NoMatch
// A repeated type: RHYTHM. Koraidon answers in kind - Speed up.
EncScript_Koraidon_InstinctRead_Match:
	enccopyvar 3, 13    @ LastPlayerMoveType = this turn's type
	encjumpifvar CMP_GREATER_THAN, 4, 1, EncScript_Koraidon_InstinctRead_Cap
	encaddvar 4, 1
	goto EncScript_Koraidon_InstinctRead_Set
EncScript_Koraidon_InstinctRead_Cap:
	encsetvar 4, 2
EncScript_Koraidon_InstinctRead_Set:
	encsetvar 5, 3      @ InstinctTelegraph: rhythm
	printstring STRINGID_ENCKORAIDONINSTINCT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	return
// No type-repeat: BRACE. Anything else physical still reads as aggression - Defense up.
EncScript_Koraidon_InstinctRead_NoMatch:
	enccopyvar 3, 13
	encsetvar 4, 0      @ RepeatTypeCount: pattern broken
	encsetvar 5, 1      @ InstinctTelegraph: brace
	printstring STRINGID_ENCKORAIDONINSTINCT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	return

// PURSUE. Somebody walked in - that's information too. Reuses the same INSTINCT line; the
// telegraph value is what actually carries the meaning turn to turn.
EncScript_Koraidon_InstinctPursue::
	encsetvar 10, 1     @ SwitchGuard
	encsetvar 5, 2      @ InstinctTelegraph: pursue
	printstring STRINGID_ENCKORAIDONINSTINCT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// PRIMAL ROAR channels for the rest of the turn. Leads with dialogue (the BG0 gotcha - an
// OnTurnStart script that animates must lead with text or flushtextbox).
EncScript_Koraidon_RoarStart::
	encsetvar 6, 1      @ RoarState
	printstring STRINGID_ENCKORAIDONROARSTART
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

// Landing a hit on Koraidon while it channels breaks the Roar. Costs a real Momentum chunk
// (guideline 8) so the interrupt isn't free.
EncScript_Koraidon_RoarInterrupt::
	encsetvar 6, 0      @ RoarState
	printstring STRINGID_ENCKORAIDONROARINTERRUPT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Koraidon_MomentumDown
	call EncScript_Koraidon_MomentumDown
	return

// The Roar completes: Momentum climbs, the Sun burns hotter (tier capped at 3), and the player's
// side is Hazed - the Roar undoes whatever setup they were leaning on.
EncScript_Koraidon_RoarResolve::
	encsetvar 6, 0      @ RoarState
	printstring STRINGID_ENCKORAIDONROARRESOLVE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	call EncScript_Koraidon_MomentumUp
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Koraidon_RoarResolve_Haze
	encaddvar 2, 1      @ SunTier
	enccopyvar 12, 2    @ LastAnnouncedTier: the ROARRESOLVE line above is the announcement
EncScript_Koraidon_RoarResolve_Haze:
	encresetstats ENC_TARGET_PLAYER_LEFT, EncScript_Koraidon_RoarResolve_Done
EncScript_Koraidon_RoarResolve_Done:
	return

// COLLISION charges for the rest of the turn (Momentum 6, Phase 1+ required to start).
EncScript_Koraidon_CollisionStart::
	encsetvar 7, 1      @ CollisionState
	printstring STRINGID_ENCKORAIDONCOLLISIONSTART
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	return

// Protect, or Momentum dropping below 6 before the turn ends, breaks the charge. Also costs a
// real Momentum chunk (guideline 8).
EncScript_Koraidon_CollisionInterrupt::
	encsetvar 7, 0      @ CollisionState
	printstring STRINGID_ENCKORAIDONCOLLISIONINTERRUPT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Koraidon_MomentumDown
	call EncScript_Koraidon_MomentumDown
	return

// Collision lands. A direct percentage hit on the PLAYER's max HP, so it scales with their team
// rather than Koraidon's (guideline 7: 16% baseline scaling with SunTier to a 30% ceiling).
// Phase 2 reflavours it Collision Course with a higher floor, same ceiling.
EncScript_Koraidon_CollisionResolve::
	encsetvar 7, 0      @ CollisionState
	encsetvar 1, 0      @ Momentum: the payload spends it
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Koraidon_CollisionResolve_Course
	printstring STRINGID_ENCKORAIDONCOLLISIONHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Koraidon_CollisionResolve_16
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Koraidon_CollisionResolve_20
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Koraidon_CollisionResolve_24
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
	return
EncScript_Koraidon_CollisionResolve_16:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	return
EncScript_Koraidon_CollisionResolve_20:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	return
EncScript_Koraidon_CollisionResolve_24:
	encchangehp ENC_TARGET_PLAYER_LEFT, -24, ENC_AMOUNT_PERCENT
	return
EncScript_Koraidon_CollisionResolve_Course:
	printstring STRINGID_ENCKORAIDONCOLLISIONCOURSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Koraidon_CollisionResolve_20C
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Koraidon_CollisionResolve_24C
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Koraidon_CollisionResolve_28C
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	return
EncScript_Koraidon_CollisionResolve_20C:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	return
EncScript_Koraidon_CollisionResolve_24C:
	encchangehp ENC_TARGET_PLAYER_LEFT, -24, ENC_AMOUNT_PERCENT
	return
EncScript_Koraidon_CollisionResolve_28C:
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
	return

// --- Phase transitions ---

// 50% - APEX PREDATOR. Collision unlocks from here (Phase >= 1 is one of its own conditions).
EncScript_Koraidon_Phase1::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCKORAIDONPHASE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// 20% - PRIMAL INSTINCT. Collision is reflavoured Collision Course from here (CollisionResolve
// branches on Phase == 2).
EncScript_Koraidon_Phase2::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCKORAIDONPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_ACC, -1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// SPENT, and the catch window (guideline 10). The exact mirror of the Properties: block plus
// everything the fight raised, so nothing survives into the window.
EncScript_Koraidon_Spent::
	encsetvar 0, 3      @ Phase (past the last real one - nothing else reads it again)
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Koraidon_Spent_NoStats
EncScript_Koraidon_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCKORAIDONWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
