// Mesprit, "The Being of Emotion". Var indices, pinned by the
// always-true Conditions on EncScript_Mesprit_Intro:
// 0 Phase (0 The Being of Emotion / 1 Emotional Instability / 2 Empathy / 3 Weakened),
// 1 Mood (0 CALM / 1 JOY / 2 ANGER / 3 FEAR / 4 SADNESS), 2 Extreme (0 normal / 1 extreme),
// 3 Announced (Mood + 5 * Extreme - the cue's re-fire latch; 255 = forced stale), 4 Clock (turns
// spent in the current mood), 5 Pull (the mood being provoked), 6 PullCount (consecutive turns that
// pull has landed), 7 TurnGuard, 8 HpMark (boss HP% at turn open; ENC_SNAP_DAMAGE turns it into
// "lost this turn"), 9 PlayerHpMark (ENC_SNAP_RECOVERY turns it into "healed this turn"),
// 10 Impression (0-8, starts 4), 11 Provoke (this turn's strongest pull, raised monotonically and
// reset at turn open), 12 StatusGuard, 13 Contagion (Mood + 1 of what stands on the player, 0 none).
//
// The fight is a feedback loop. Every turn files a pull from what the player did; the strongest
// pull wins and, once it repeats, commits a mood. Each mood owns Mesprit's guard, its stances, its
// slot-3 move and the contagion it lays on the player - so ANGER is the only mood the player can
// really hurt it in and the mood that hurts them most, and FEAR is the mood their attacks stop
// working in. A mood nobody disturbs festers into an extreme that persists until the player's own
// behaviour moves it. At 20% Mesprit stops reacting and keeps whatever it learned.

// --- Shared subroutines (call/return) ---

// Sole owner of Mesprit's guard, stances, slot-3 move and the contagion. Branches on Mood and
// Extreme to one of nine bodies, each of which falls through to the cue.
EncScript_Mesprit_ApplyMood:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Mesprit_ApplyMood_Extreme
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_EnterSadness
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_EnterFear
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_EnterAnger
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mesprit_EnterJoy
	goto EncScript_Mesprit_EnterCalm
EncScript_Mesprit_ApplyMood_Extreme:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_EnterDespair
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_EnterPanic
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_EnterRage
	goto EncScript_Mesprit_EnterOverjoyed

// The stances are an ABSOLUTE set, not a delta: encchangestat clamps at the stage bounds, so -12
// lands on the floor from wherever a stage had drifted and the +N that follows walks it back up to
// an exact value. That is what makes ApplyMood idempotent and lets the mood machine move in any
// direction without tracking what it added. Safe because nothing in Mesprit's moveset writes its
// own stat stages.
EncScript_Mesprit_NeutralStance:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	return

// CALM. The unremarkable mood, and the guideline baseline guard. Swift never misses and never
// impresses - a neutral mood reads as a neutral move.
EncScript_Mesprit_EnterCalm:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	call EncScript_Mesprit_NeutralStance
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SWIFT
	call EncScript_Mesprit_ClearContagion
	goto EncScript_Mesprit_Cue

// JOY. It thrives: Draining Kiss heals it, and the contagion is a gift the player did not ask for.
EncScript_Mesprit_EnterJoy:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encsetmove ENC_TARGET_BOSS, 3, MOVE_DRAINING_KISS
	call EncScript_Mesprit_ContagionJoy
	goto EncScript_Mesprit_Cue

// ANGER. The mood the player can actually hurt it in - the guard drops to 85 and its own Defense
// falls with it - and the mood that hurts them most. Double-Edge's recoil is the rage costing it
// something.
EncScript_Mesprit_EnterAnger:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encsetmove ENC_TARGET_BOSS, 3, MOVE_DOUBLE_EDGE
	call EncScript_Mesprit_ContagionAnger
	goto EncScript_Mesprit_Cue

// FEAR. The spec wanted it to flee; a wild legendary that flees ends the encounter, so it hardens
// instead. Guard 94 is what makes frightening it the thing that stops the player frightening it
// again - the same raw hit reads as a third of what it did through ANGER.
EncScript_Mesprit_EnterFear:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	encsetmove ENC_TARGET_BOSS, 3, MOVE_PROTECT
	call EncScript_Mesprit_ContagionFear
	goto EncScript_Mesprit_Cue

// SADNESS. Its own offense drops with the player's - a sad Mesprit stops pressuring you, which is
// the opportunity to heal delivered honestly rather than as a healing-rate multiplier the engine
// does not have.
EncScript_Mesprit_EnterSadness:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encsetmove ENC_TARGET_BOSS, 3, MOVE_CHARM
	call EncScript_Mesprit_ContagionSadness
	goto EncScript_Mesprit_Cue

// OVERJOYED. It out-heals the player. The escape is to stop being gentle.
EncScript_Mesprit_EnterOverjoyed:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encsetmove ENC_TARGET_BOSS, 3, MOVE_DRAINING_KISS
	call EncScript_Mesprit_ContagionJoy
	goto EncScript_Mesprit_Cue

// RAGE. The guard collapses to 80 - the softest Mesprit ever is - and it costs the player 10% of a
// bar every turn to stand there. The escape is to stop hitting it hard.
EncScript_Mesprit_EnterRage:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 3
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 3
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encsetmove ENC_TARGET_BOSS, 3, MOVE_DOUBLE_EDGE
	call EncScript_Mesprit_ContagionAnger
	goto EncScript_Mesprit_Cue

// PANIC. Guard 96 and a coin-flip Protect on top. Nothing the player does lands; the escape is a
// soft turn, which reads as CALM or JOY.
EncScript_Mesprit_EnterPanic:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 3
	encsetmove ENC_TARGET_BOSS, 3, MOVE_PROTECT
	call EncScript_Mesprit_ContagionFear
	goto EncScript_Mesprit_Cue

// DESPAIR. It drains the player and heals itself for it, and washes status off both sides - the
// spec's own SADNESS behaviour, and the reason a status lock cannot hold here.
EncScript_Mesprit_EnterDespair:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	call EncScript_Mesprit_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encsetmove ENC_TARGET_BOSS, 3, MOVE_CHARM
	call EncScript_Mesprit_ContagionSadness
	goto EncScript_Mesprit_Cue

// The mood line, latched on Var(Announced) against Mood + 5 * Extreme, so the cue fires on every
// real change in either direction - into a mood, into its extreme, and back out - and never repeats
// an unchanged state. The fight's mechanics are invisible numbers; this is the channel that names
// them without spelling out what they do.
EncScript_Mesprit_Cue:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Mesprit_Cue_Extreme
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_Cue_Sadness
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_Cue_Fear
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_Cue_Anger
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mesprit_Cue_Joy
	goto EncScript_Mesprit_Cue_Calm
EncScript_Mesprit_Cue_Extreme:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_Cue_Despair
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_Cue_Panic
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_Cue_Rage
	goto EncScript_Mesprit_Cue_Overjoyed
EncScript_Mesprit_Cue_Calm:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Mesprit_Cue_Done
	encsetvar 3, 0    @ Announced
	printstring STRINGID_ENCMESPRITCALM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return
EncScript_Mesprit_Cue_Joy:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Mesprit_Cue_Done
	encsetvar 3, 1
	printstring STRINGID_ENCMESPRITJOY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAINBOW
	return
EncScript_Mesprit_Cue_Anger:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Mesprit_Cue_Done
	encsetvar 3, 2
	printstring STRINGID_ENCMESPRITANGER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return
EncScript_Mesprit_Cue_Fear:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Mesprit_Cue_Done
	encsetvar 3, 3
	printstring STRINGID_ENCMESPRITFEAR
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_HARDEN
	return
EncScript_Mesprit_Cue_Sadness:
	encjumpifvar CMP_EQUAL, 3, 4, EncScript_Mesprit_Cue_Done
	encsetvar 3, 4
	printstring STRINGID_ENCMESPRITSADNESS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	return
EncScript_Mesprit_Cue_Overjoyed:
	encjumpifvar CMP_EQUAL, 3, 6, EncScript_Mesprit_Cue_Done
	encsetvar 3, 6
	printstring STRINGID_ENCMESPRITOVERJOYED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	return
EncScript_Mesprit_Cue_Rage:
	encjumpifvar CMP_EQUAL, 3, 7, EncScript_Mesprit_Cue_Done
	encsetvar 3, 7
	printstring STRINGID_ENCMESPRITRAGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	return
EncScript_Mesprit_Cue_Panic:
	encjumpifvar CMP_EQUAL, 3, 8, EncScript_Mesprit_Cue_Done
	encsetvar 3, 8
	printstring STRINGID_ENCMESPRITPANIC
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	return
EncScript_Mesprit_Cue_Despair:
	encjumpifvar CMP_EQUAL, 3, 9, EncScript_Mesprit_Cue_Done
	encsetvar 3, 9
	printstring STRINGID_ENCMESPRITDESPAIR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
EncScript_Mesprit_Cue_Done:
	return

// The contagion is a DELTA, undone from Var(Contagion) on every change - wiping the player's stages
// absolutely would destroy their own setup, which is not what the spec asks for. Stage clamping can
// make an undo inexact at the bounds; with deltas of 1-2 that is an edge case and always lands in
// the player's favour. The player can also clear it outright by switching, which is itself a JOY
// pull - so the escape has a price. Each routine leaves an already-standing contagion alone, which
// is what keeps a mood going extreme from re-printing its own contagion lines.
EncScript_Mesprit_ContagionJoy:
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Mesprit_Contagion_Done
	call EncScript_Mesprit_ClearContagion
	encsetvar 13, 2    @ Contagion: JOY
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, 1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, 1
	call EncScript_Mesprit_ContagionHeader
	printstring STRINGID_ENCMESPRITCONTJOY
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Mesprit_ContagionAnger:
	encjumpifvar CMP_EQUAL, 13, 3, EncScript_Mesprit_Contagion_Done
	call EncScript_Mesprit_ClearContagion
	encsetvar 13, 3    @ Contagion: ANGER
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, 2
	encchangestat ENC_TARGET_ALL_FOES, STAT_ACC, -1
	call EncScript_Mesprit_ContagionHeader
	printstring STRINGID_ENCMESPRITCONTANGER
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Mesprit_ContagionFear:
	encjumpifvar CMP_EQUAL, 13, 4, EncScript_Mesprit_Contagion_Done
	call EncScript_Mesprit_ClearContagion
	encsetvar 13, 4    @ Contagion: FEAR
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -2
	call EncScript_Mesprit_ContagionHeader
	printstring STRINGID_ENCMESPRITCONTFEAR
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Mesprit_ContagionSadness:
	encjumpifvar CMP_EQUAL, 13, 5, EncScript_Mesprit_Contagion_Done
	call EncScript_Mesprit_ClearContagion
	encsetvar 13, 5    @ Contagion: SADNESS
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, -1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, -1
	call EncScript_Mesprit_ContagionHeader
	printstring STRINGID_ENCMESPRITCONTSADNESS
	waitmessage B_WAIT_TIME_LONG
EncScript_Mesprit_Contagion_Done:
	return

EncScript_Mesprit_ContagionHeader:
	printstring STRINGID_ENCMESPRITWASHESOVER
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_FORM_CHANGE
	return

// The exact inverse of whatever Var(Contagion) says is standing. Silent when there is nothing.
EncScript_Mesprit_ClearContagion:
	encjumpifvar CMP_EQUAL, 13, 5, EncScript_Mesprit_ClearCont_Sadness
	encjumpifvar CMP_EQUAL, 13, 4, EncScript_Mesprit_ClearCont_Fear
	encjumpifvar CMP_EQUAL, 13, 3, EncScript_Mesprit_ClearCont_Anger
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Mesprit_ClearCont_Joy
	return
EncScript_Mesprit_ClearCont_Joy:
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, -1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, -1
	goto EncScript_Mesprit_ClearCont_Done
EncScript_Mesprit_ClearCont_Anger:
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, -2
	encchangestat ENC_TARGET_ALL_FOES, STAT_ACC, 1
	goto EncScript_Mesprit_ClearCont_Done
EncScript_Mesprit_ClearCont_Fear:
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, 2
	goto EncScript_Mesprit_ClearCont_Done
EncScript_Mesprit_ClearCont_Sadness:
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, 1
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, 1
EncScript_Mesprit_ClearCont_Done:
	encsetvar 13, 0    @ Contagion: nothing standing
	printstring STRINGID_ENCMESPRITCONTFADES
	waitmessage B_WAIT_TIME_SHORT
	return

// Anti-cheese, half one: entering a new mood washes Mesprit's own status off. Statusing it is a
// legitimate move here - it is the SADNESS pull - so a flat immunity would take away a real option;
// this just stops it being a lock, since a mood changes at most every two turns in Phase 0 and every
// turn from Phase 1. StatusGuard is the "is there anything to clear" test: the ProvokeStatus trigger
// arms it at the first end of turn a status stands, and StatusCleared hands it back.
// curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at a turn checkpoint.
EncScript_Mesprit_ClearOwnStatus:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Mesprit_ClearOwnStatus_Done
	encsetvar 12, 0    @ StatusGuard
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCMESPRITSHIFTSOFF
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
EncScript_Mesprit_ClearOwnStatus_Done:
	return

// The five commit entry points. The engine compares vars against literals only, so each caller
// names its own mood. A mood that is already current is left alone - its clock keeps running, which
// is what eventually festers it.
EncScript_Mesprit_ToCalm:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mesprit_ToMood_Same
	encsetvar 1, 0    @ Mood
	goto EncScript_Mesprit_MoodChanged
EncScript_Mesprit_ToJoy:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mesprit_ToMood_Same
	encsetvar 1, 1
	goto EncScript_Mesprit_MoodChanged
EncScript_Mesprit_ToAnger:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_ToMood_Same
	encsetvar 1, 2
	goto EncScript_Mesprit_MoodChanged
EncScript_Mesprit_ToFear:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_ToMood_Same
	encsetvar 1, 3
	goto EncScript_Mesprit_MoodChanged
EncScript_Mesprit_ToSadness:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_ToMood_Same
	encsetvar 1, 4
EncScript_Mesprit_MoodChanged:
	encsetvar 2, 0    @ Extreme: a new mood is never born extreme
	encsetvar 4, 0    @ Clock
	call EncScript_Mesprit_ClearOwnStatus
	call EncScript_Mesprit_ApplyMood
EncScript_Mesprit_ToMood_Same:
	return

// EMOTIONAL OVERLOAD. Reached from TurnClose when the fester clock runs out. CALM has no extreme -
// an undisturbed calm drifts into JOY instead, which is the spec's own Calm -> Joy -> Overjoyed
// progression. Everything else goes extreme and STAYS extreme, re-firing a smaller surge every turn
// until the player's behaviour moves the mood; an overload that reset itself could be waited out,
// and the player would never have to learn the mood table.
EncScript_Mesprit_GoExtreme:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Mesprit_GoExtreme_Calm
	encsetvar 2, 1    @ Extreme
	encsetvar 4, 0    @ Clock
	printstring STRINGID_ENCMESPRITOVERWHELMING
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Mesprit_ClearOwnStatus
	call EncScript_Mesprit_ApplyMood
	call EncScript_Mesprit_Surge
	return
EncScript_Mesprit_GoExtreme_Calm:
	printstring STRINGID_ENCMESPRITSTIRS
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1    @ Mood: JOY
	goto EncScript_Mesprit_MoodChanged

// The one-shot surge, fired the moment a mood goes extreme. Twice the recurring figure, because it
// is the beat that has to land.
EncScript_Mesprit_Surge:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_Surge_Despair
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_Surge_Panic
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_Surge_Rage
EncScript_Mesprit_Surge_Overjoyed:
	printstring STRINGID_ENCMESPRITTHRIVES
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	enchangehp ENC_TARGET_BOSS, 16, ENC_AMOUNT_PERCENT
	return
EncScript_Mesprit_Surge_Rage:
	printstring STRINGID_ENCMESPRITLASHES
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_EXTRASENSORY
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	return
// PANIC's one-shot cannot be a Protect - gProtectStructs is cleared after end-of-turn effects, so
// encsetprotect only means anything at OnTurnStart, which is where the recurring half runs.
EncScript_Mesprit_Surge_Panic:
	printstring STRINGID_ENCMESPRITCOWERS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_HARDEN
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -2
	return
EncScript_Mesprit_Surge_Despair:
	printstring STRINGID_ENCMESPRITDRAINS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_LEECH_SEED_DRAIN
	enchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	enchangehp ENC_TARGET_BOSS, 16, ENC_AMOUNT_PERCENT
	goto EncScript_Mesprit_SorrowClears

// The standing cost of an extreme, run at every turn open for as long as it holds. Each one is
// unaffordable in a different way, and the escape from each is a different behaviour - which is what
// forces the player to learn the mood table rather than ride one strategy.
EncScript_Mesprit_RecurringSurge:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_Recurring_Despair
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_Recurring_Panic
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_Recurring_Rage
EncScript_Mesprit_Recurring_Overjoyed:
	printstring STRINGID_ENCMESPRITTHRIVES
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	return
EncScript_Mesprit_Recurring_Rage:
	printstring STRINGID_ENCMESPRITLASHES
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_EXTRASENSORY
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	return
// The coin flip is the whole of PANIC's cost: on top of a 96 guard, half the player's turns simply
// do not happen. encsetprotect at OnTurnStart covers the whole turn and expires on its own.
EncScript_Mesprit_Recurring_Panic:
	encjumpifchance 50, EncScript_Mesprit_Recurring_Panic_Hide
	printstring STRINGID_ENCMESPRITCOWERS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mesprit_Recurring_Panic_Hide:
	printstring STRINGID_ENCMESPRITHIDES
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_PROTECT
	encsetprotect ENC_TARGET_BOSS
	return
EncScript_Mesprit_Recurring_Despair:
	printstring STRINGID_ENCMESPRITDRAINS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_LEECH_SEED_DRAIN
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	enchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT

// Anti-cheese, half two: DESPAIR washes status off BOTH sides every turn it holds. It is the spec's
// own SADNESS behaviour, it is a real cost to the player, and it is why no status can hold Mesprit
// for long even in Phase 2, where moods no longer change.
EncScript_Mesprit_SorrowClears:
	encsetvar 12, 0    @ StatusGuard
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	curestatus BS_PLAYER1
	updatestatusicon BS_PLAYER1
	printstring STRINGID_ENCMESPRITSORROWCLEARS
	waitmessage B_WAIT_TIME_LONG
	return

// Guideline 14's standing channel: a player who missed the mood line is told, every turn, that a
// state is running. CALM waits until its clock has actually started to fester - saying "nothing is
// happening" on turn one is noise, and by turn two it is a warning.
EncScript_Mesprit_Heartbeat:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Mesprit_Heartbeat_Sadness
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Mesprit_Heartbeat_Fear
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Mesprit_Heartbeat_Anger
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Mesprit_Heartbeat_Joy
	encjumpifvar CMP_LESS_THAN, 4, 2, EncScript_Mesprit_Heartbeat_Done
	printstring STRINGID_ENCMESPRITCALMHOLDS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mesprit_Heartbeat_Joy:
	printstring STRINGID_ENCMESPRITJOYHOLDS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mesprit_Heartbeat_Anger:
	printstring STRINGID_ENCMESPRITANGERHOLDS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mesprit_Heartbeat_Fear:
	printstring STRINGID_ENCMESPRITFEARHOLDS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Mesprit_Heartbeat_Sadness:
	printstring STRINGID_ENCMESPRITSADNESSHOLDS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Mesprit_Heartbeat_Done:
	return

// Impression, the Empathy scale. Clamped at both ends in one place so nothing else has to.
EncScript_Mesprit_RaiseImpression:
	encjumpifvar CMP_GREATER_THAN, 10, 7, EncScript_Mesprit_Impression_Done
	encaddvar 10, 1
	return
EncScript_Mesprit_LowerImpression:
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Mesprit_Impression_Done
	encsubvar 10, 1
EncScript_Mesprit_Impression_Done:
	return

// The commit counter. A pull has to repeat before it moves the mood - twice in Phase 0, once from
// Phase 1. PullCount landing back on 0 is the "committed" signal the callers read; there is no
// return value in a battle script, so the counter being consumed is the message.
EncScript_Mesprit_PullTick:
	encaddvar 6, 1    @ PullCount
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Mesprit_PullTick_Commit
	encjumpifvar CMP_LESS_THAN, 6, 2, EncScript_Mesprit_PullTick_Done
EncScript_Mesprit_PullTick_Commit:
	encsetvar 6, 0    @ PullCount: consumed, and the commit signal
EncScript_Mesprit_PullTick_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only Impression needs seeding. Deliberately does NOT
// call ApplyMood: CALM is the neutral state, and announcing it on turn one would spend the calm line
// before it can mean anything.
EncScript_Mesprit_Intro::
	encsetvar 10, 4   @ Impression: the middle of the scale
	printstring STRINGID_ENCMESPRITAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail so no branch out of this script can leave it re-armed.
// Owns both HP marks, the fester clock, and whichever standing channel the state calls for - an
// extreme speaks through its recurring surge, an ordinary mood through its heartbeat.
EncScript_Mesprit_TurnOpen::
	flushtextbox
	encsetvar 7, 1    @ TurnGuard
	encsetvar 11, 0   @ Provoke: the turn's verdict starts empty
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Mesprit_TurnOpen_Done   @ Weakened: inert
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_SET
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Mesprit_TurnOpen_Extreme
	encaddvar 4, 1    @ Clock: another turn spent in this mood
	call EncScript_Mesprit_Heartbeat
	return
EncScript_Mesprit_TurnOpen_Extreme:
	call EncScript_Mesprit_RecurringSurge
EncScript_Mesprit_TurnOpen_Done:
	return

// The turn verdict - the core loop. Provoke is a monotonically-raised priority slot: the OnMoveEnd,
// OnSwitchIn and OnFaint triggers have already written into it during the turn, and this fills in
// the two readings only measurable at the end - how much Mesprit lost, and whether the player
// healed - without ever lowering what is already there. SADNESS > FEAR > ANGER > JOY > CALM falls
// out of that for free, with no ordering logic between the triggers.
// The bars are measured AFTER the mood's guard, which is what makes the loop self-correcting: the
// same raw hit reads as 6% through ANGER's 85 and 2.4% through FEAR's 94, so frightening Mesprit is
// what stops the player frightening it again. encsnapshothp's failLabel handles the zero-damage
// case by falling straight through, so a Protect turn or a setup turn is a JOY/CALM turn rather
// than a silently-ignored one.
EncScript_Mesprit_TurnClose::
	encsetvar 7, 0    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Mesprit_TurnClose_Done   @ Empathy/Weakened: locked
	encsnapshothp ENC_TARGET_BOSS, 8, ENC_SNAP_DAMAGE, EncScript_Mesprit_TurnClose_Heal
	encjumpifvar CMP_GREATER_THAN, 8, 7, EncScript_Mesprit_TurnClose_Fear
	encjumpifvar CMP_GREATER_THAN, 8, 2, EncScript_Mesprit_TurnClose_Anger
	goto EncScript_Mesprit_TurnClose_Heal
EncScript_Mesprit_TurnClose_Fear:
	encjumpifvar CMP_GREATER_THAN, 11, 2, EncScript_Mesprit_TurnClose_Heal
	encsetvar 11, 3   @ Provoke: FEAR
	goto EncScript_Mesprit_TurnClose_Heal
EncScript_Mesprit_TurnClose_Anger:
	encjumpifvar CMP_GREATER_THAN, 11, 1, EncScript_Mesprit_TurnClose_Heal
	encsetvar 11, 2   @ Provoke: ANGER
// Only worth reading while Provoke is still empty - JOY is the lowest level there is, so anything
// already filed outranks it. That gate is also what keeps this off a fainted PLAYER_LEFT: a faint
// raises Provoke to SADNESS, and a single-slot snapshot on an absent battler is an authoring error.
EncScript_Mesprit_TurnClose_Heal:
	encjumpifvar CMP_GREATER_THAN, 11, 0, EncScript_Mesprit_TurnClose_Impression
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_RECOVERY, EncScript_Mesprit_TurnClose_Impression
	encsetvar 11, 1   @ Provoke: JOY
// Impression, the number the whole battle is quietly writing. Never shown, never announced; it is
// read exactly once, at 20%, and it decides what Mesprit keeps.
EncScript_Mesprit_TurnClose_Impression:
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Mesprit_TurnClose_Softer
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Mesprit_TurnClose_Harder
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Mesprit_TurnClose_Harder
	goto EncScript_Mesprit_TurnClose_Pull
EncScript_Mesprit_TurnClose_Harder:
	call EncScript_Mesprit_RaiseImpression
	goto EncScript_Mesprit_TurnClose_Pull
EncScript_Mesprit_TurnClose_Softer:
	call EncScript_Mesprit_LowerImpression
// Resolve the pull. A pull that is not the one already running starts its own count; a pull that
// repeats often enough commits the mood. PullCount back at 0 is PullTick's commit signal.
EncScript_Mesprit_TurnClose_Pull:
	encjumpifvar CMP_EQUAL, 11, 4, EncScript_Mesprit_Pull_Sadness
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Mesprit_Pull_Fear
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Mesprit_Pull_Anger
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Mesprit_Pull_Joy
EncScript_Mesprit_Pull_Calm:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Mesprit_Pull_Calm_Tick
	encsetvar 5, 0    @ Pull
	encsetvar 6, 0    @ PullCount
EncScript_Mesprit_Pull_Calm_Tick:
	call EncScript_Mesprit_PullTick
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Mesprit_TurnClose_Fester
	call EncScript_Mesprit_ToCalm
	goto EncScript_Mesprit_TurnClose_Fester
EncScript_Mesprit_Pull_Joy:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Mesprit_Pull_Joy_Tick
	encsetvar 5, 1
	encsetvar 6, 0
EncScript_Mesprit_Pull_Joy_Tick:
	call EncScript_Mesprit_PullTick
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Mesprit_TurnClose_Fester
	call EncScript_Mesprit_ToJoy
	goto EncScript_Mesprit_TurnClose_Fester
EncScript_Mesprit_Pull_Anger:
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Mesprit_Pull_Anger_Tick
	encsetvar 5, 2
	encsetvar 6, 0
EncScript_Mesprit_Pull_Anger_Tick:
	call EncScript_Mesprit_PullTick
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Mesprit_TurnClose_Fester
	call EncScript_Mesprit_ToAnger
	goto EncScript_Mesprit_TurnClose_Fester
EncScript_Mesprit_Pull_Fear:
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Mesprit_Pull_Fear_Tick
	encsetvar 5, 3
	encsetvar 6, 0
EncScript_Mesprit_Pull_Fear_Tick:
	call EncScript_Mesprit_PullTick
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Mesprit_TurnClose_Fester
	call EncScript_Mesprit_ToFear
	goto EncScript_Mesprit_TurnClose_Fester
EncScript_Mesprit_Pull_Sadness:
	encjumpifvar CMP_EQUAL, 5, 4, EncScript_Mesprit_Pull_Sadness_Tick
	encsetvar 5, 4
	encsetvar 6, 0
EncScript_Mesprit_Pull_Sadness_Tick:
	call EncScript_Mesprit_PullTick
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Mesprit_TurnClose_Fester
	call EncScript_Mesprit_ToSadness
// A mood nobody disturbed for long enough. The clock was ticked at turn open and is zeroed by any
// mood change, so reaching the threshold here means the player has spent four turns (three from
// Phase 1) doing the same thing to it.
EncScript_Mesprit_TurnClose_Fester:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Mesprit_TurnClose_Done   @ already extreme
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Mesprit_TurnClose_Fester1
	encjumpifvar CMP_LESS_THAN, 4, 4, EncScript_Mesprit_TurnClose_Done
	goto EncScript_Mesprit_GoExtreme
EncScript_Mesprit_TurnClose_Fester1:
	encjumpifvar CMP_LESS_THAN, 4, 3, EncScript_Mesprit_TurnClose_Done
	goto EncScript_Mesprit_GoExtreme
EncScript_Mesprit_TurnClose_Done:
	return

// The provocations. Each is two lines: raise Provoke to its own level and return. Silent - the mood
// line is the announcement, and printing on every provocation would bury it. Their .encounter
// conditions already gate on Provoke being below their level, so no in-script guard is needed.
EncScript_Mesprit_ProvokeAnger::
	encsetvar 11, 2   @ Provoke: ANGER
	return

EncScript_Mesprit_ProvokeJoy::
	encsetvar 11, 1   @ Provoke: JOY
	return

EncScript_Mesprit_ProvokeSadness::
	encsetvar 11, 4   @ Provoke: SADNESS
	return

// StatusGuard is spent whether or not the pull lands, because it is also what tells ClearOwnStatus
// there is something to wash off. The Provoke raise is guarded separately so a status landing on a
// turn that already filed SADNESS cannot lower it.
EncScript_Mesprit_ProvokeStatus::
	encsetvar 12, 1   @ StatusGuard
	encjumpifvar CMP_GREATER_THAN, 11, 3, EncScript_Mesprit_ProvokeStatus_Done
	encsetvar 11, 4   @ Provoke: SADNESS
EncScript_Mesprit_ProvokeStatus_Done:
	return

// Silent - nothing has happened the player needs told. It only re-arms the guard, so each SADNESS
// pull costs a fresh application rather than a standing burn.
EncScript_Mesprit_StatusCleared::
	encsetvar 12, 0   @ StatusGuard
	return

// 50% - EMOTIONAL INSTABILITY. One literal: the commit threshold drops from two turns to one, so a
// single turn's conduct now redirects the fight, and the fester clock drops from four to three, so
// standing still is punished faster. PullCount is cleared because nothing carries across the
// boundary - the new threshold gets a clean count.
EncScript_Mesprit_Unstable::
	encsetvar 0, 1    @ Phase
	encsetvar 6, 0    @ PullCount
	printstring STRINGID_ENCMESPRITUNSTABLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return

// 20% - EMPATHY. Mesprit stops reacting and keeps what it learned. Var(Impression) has been running
// unannounced since turn one: every ANGER or FEAR turn pushed it up, every JOY turn pushed it down,
// and the band it landed in picks the mood, locks it, forces it extreme, and traps the player in.
// From here TurnClose returns through its leading branch - no pull is read and no mood moves - and
// what remains is the extreme's recurring surge every turn until the player chips it to 10%.
// Announced is forced stale so the locked mood announces itself on the way in.
EncScript_Mesprit_Empathy::
	encsetvar 0, 2    @ Phase
	encsetvar 3, 255  @ Announced: forced stale
	encsetvar 2, 1    @ Extreme
	encsetvar 4, 0    @ Clock
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCMESPRITEMPATHY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TERA_ACTIVATE
	encjumpifvar CMP_GREATER_THAN, 10, 5, EncScript_Mesprit_Empathy_Anger
	encjumpifvar CMP_LESS_THAN, 10, 3, EncScript_Mesprit_Empathy_Fear
	printstring STRINGID_ENCMESPRITEMPATHYSADNESS
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 4    @ Mood: SADNESS -> DESPAIR
	goto EncScript_Mesprit_Empathy_Lock
EncScript_Mesprit_Empathy_Anger:
	printstring STRINGID_ENCMESPRITEMPATHYANGER
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 2    @ Mood: ANGER -> RAGE
	goto EncScript_Mesprit_Empathy_Lock
EncScript_Mesprit_Empathy_Fear:
	printstring STRINGID_ENCMESPRITEMPATHYFEAR
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 3    @ Mood: FEAR -> PANIC
EncScript_Mesprit_Empathy_Lock:
	call EncScript_Mesprit_ApplyMood
	call EncScript_Mesprit_Surge
	return

// 10% - the catch window, the mirror image of the Properties: block. Guideline 10 exactly: no ball
// can be thrown before this point. Every counter the fight built is zeroed so nothing outlives it,
// the contagion comes off the player, and Phase 3 makes TurnOpen and TurnClose both return through
// their leading branch, so no clock or surge can restart.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Mesprit_Weakened::
	encsetvar 0, 3    @ Phase
	encsetvar 1, 0    @ Mood
	encsetvar 2, 0    @ Extreme
	encsetvar 3, 0    @ Announced
	encsetvar 4, 0    @ Clock
	encsetvar 5, 0    @ Pull
	encsetvar 6, 0    @ PullCount
	encsetvar 11, 0   @ Provoke
	call EncScript_Mesprit_ClearContagion
	call EncScript_Mesprit_NeutralStance
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SWIFT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMESPRITWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
