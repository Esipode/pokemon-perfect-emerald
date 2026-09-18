// Uxie, "The Being of Knowledge". Var indices, pinned by the always-true Conditions on
// EncScript_Uxie_Intro:
// 0 Phase (0 Study / 1 Complete Understanding / 2 Omniscience / 3 Weakened), 1 Knowledge (0-5, the
// meter), 2 Read (0 none / 1-3 the read / 11-13 vowed YES on that category / 21-23 vowed NO),
// 3 Tier (rung last APPLIED - ApplyKnowledge's idempotence latch and the rung line's re-fire latch),
// 4 Load (adaptations held, 0-4 - written ONLY by encadapt/encpurgeadapt), 5 Result (encadapt's
// ENC_ADAPT_RESULT_* outcome), 6/7/8 SeenPhys/SeenSpec/SeenStat (lifetime reserve-card latches),
// 9 Streak (consecutive defied reads in Omniscience), 10 Collapse (collapse-window countdown),
// 11 TestClock (turns until the next Test), 12 TurnGuard, 13 MoveGuard (the verdict's per-turn
// re-entry guard; doubles as "the player acted this turn"), 14 FaintGuard, 15 Rising (scratch:
// which direction the rung ladder just moved, for the animation).
//
// The fight is one question asked every turn: did Uxie call the player's move before they made it?
// The read is the engine's real AI_FLAG_PREDICT_MOVE answer, announced out loud at turn open and
// graded at OnMoveEnd. Foreseen raises KNOWLEDGE and lets Uxie file the type it was hit with as a
// resistance; defied costs it a rung and its newest adaptation. Knowledge is the only thing that
// lets Uxie hold a board at all - nothing at 0, four types at 70% at 5 - so the player never touches
// the board directly, they decide whether Uxie is allowed one. The rule they are meant to find,
// untold, is that Uxie expects their BEST move, so winning means not using it.

// --- Shared subroutines (call/return) ---

// Sole owner of Uxie's SpAtk and Speed, and of the rung line. An ABSOLUTE set, not a delta -
// encchangestat clamps at the stage bounds, so -12 lands on the floor from wherever the stage had
// drifted and +N walks back up to an exact value. That is what makes this idempotent and what lets
// the ladder move in both directions without tracking what it added.
// Offense and speed, never bulk: Uxie's 130/130 defenses are already a wall, so more would be
// invisible. Knowledge buys it PRECISION - it moves first and it hits harder.
// Each rung latches on Var(Tier) against its own literal - there is no var-to-var comparison - and
// compares Tier to that same literal once more to know which way the ladder moved, so a rung that
// was climbed to and one that was fallen back to never play the same animation.
EncScript_Uxie_ApplyKnowledge:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Uxie_Rung_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Uxie_Rung_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Uxie_Rung_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Uxie_Rung_Two
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Uxie_Rung_One
EncScript_Uxie_Rung_Zero:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Uxie_Rung_Done
	encsetvar 15, 0   @ Rising: nothing is below 0, so this is always a fall
	encsetvar 3, 0    @ Tier
	call EncScript_Uxie_NeutralStats
	printstring STRINGID_ENCUXIEK0
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Uxie_Rung_Anim
EncScript_Uxie_Rung_One:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Uxie_Rung_Done
	encsetvar 15, 1   @ Rising: climbed to this rung...
	encjumpifvar CMP_LESS_THAN, 3, 1, EncScript_Uxie_Rung_One_Apply
	encsetvar 15, 0   @ ...unless Tier was above it, in which case this is a fall
EncScript_Uxie_Rung_One_Apply:
	encsetvar 3, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 7
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	printstring STRINGID_ENCUXIEK1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Uxie_Rung_Anim
EncScript_Uxie_Rung_Two:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Uxie_Rung_Done
	encsetvar 15, 1   @ Rising: climbed to this rung...
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Uxie_Rung_Two_Apply
	encsetvar 15, 0   @ ...unless Tier was above it, in which case this is a fall
EncScript_Uxie_Rung_Two_Apply:
	encsetvar 3, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 7
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 7
	printstring STRINGID_ENCUXIEK2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Uxie_Rung_Anim
EncScript_Uxie_Rung_Three:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Uxie_Rung_Done
	encsetvar 15, 1   @ Rising: climbed to this rung...
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Uxie_Rung_Three_Apply
	encsetvar 15, 0   @ ...unless Tier was above it, in which case this is a fall
EncScript_Uxie_Rung_Three_Apply:
	encsetvar 3, 3
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 8
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 7
	printstring STRINGID_ENCUXIEK3
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Uxie_Rung_Anim
EncScript_Uxie_Rung_Four:
	encjumpifvar CMP_EQUAL, 3, 4, EncScript_Uxie_Rung_Done
	encsetvar 15, 1   @ Rising: climbed to this rung...
	encjumpifvar CMP_LESS_THAN, 3, 4, EncScript_Uxie_Rung_Four_Apply
	encsetvar 15, 0   @ ...unless Tier was above it, in which case this is a fall
EncScript_Uxie_Rung_Four_Apply:
	encsetvar 3, 4
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 8
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 8
	printstring STRINGID_ENCUXIEK4
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Uxie_Rung_Anim
// OMNISCIENCE. +3 SpAtk behind base 130 with a 90 flat guard the other way is the whole statement:
// the thing that already knows the answer also hits hardest.
EncScript_Uxie_Rung_Five:
	encjumpifvar CMP_EQUAL, 3, 5, EncScript_Uxie_Rung_Done
	encsetvar 3, 5
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 9
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 8
	printstring STRINGID_ENCUXIEK5
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	return
// Rung 5 is the only rung that can only ever be climbed to, so it carries its own animation and
// never reaches here.
EncScript_Uxie_Rung_Anim:
	encjumpifvar CMP_EQUAL, 15, 1, EncScript_Uxie_Rung_Anim_Up
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	return
EncScript_Uxie_Rung_Anim_Up:
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
EncScript_Uxie_Rung_Done:
	return

EncScript_Uxie_NeutralStats:
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	return

// One rung up, clamped to the phase ceiling - 3 in Study, 5 from Complete Understanding on. Every
// path that raises Knowledge goes through here, so the clamp lives in exactly one place and the
// ladder can never fall out of step with the meter. In Omniscience Knowledge is already pinned at 5,
// so this is a no-op there without needing to say so, and a collapse window shuts it off outright -
// the faint and sleep-refusal levers can still fire in there, and neither should rebuild the meter
// the window exists to have taken away.
EncScript_Uxie_GainOne:
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Uxie_GainOne_Done   @ mid-collapse: it learns nothing
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Uxie_GainOne_Study
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Uxie_GainOne_Done
	goto EncScript_Uxie_GainOne_Add
EncScript_Uxie_GainOne_Study:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Uxie_GainOne_Done
EncScript_Uxie_GainOne_Add:
	encaddvar 1, 1
	call EncScript_Uxie_ApplyKnowledge
EncScript_Uxie_GainOne_Done:
	return

// The verdict's gain. Study pays +1; from Complete Understanding on it pays +2, which is the spec's
// "Knowledge rises faster" as a single branch rather than a second ladder.
EncScript_Uxie_Gain:
	call EncScript_Uxie_GainOne
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Uxie_Gain_Done
	call EncScript_Uxie_GainOne
EncScript_Uxie_Gain_Done:
	return

EncScript_Uxie_LoseOne:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Uxie_LoseOne_Done
	encsubvar 1, 1
	call EncScript_Uxie_ApplyKnowledge
EncScript_Uxie_LoseOne_Done:
	return

// The board write, gated entirely by Knowledge - K0 files nothing at all, which is what makes the
// board a CONSEQUENCE of the prediction duel rather than something the player feeds directly.
// The percent/slot pair is the rung's, picked here rather than stored. encadapt writes Var(Load)
// itself and ignores anything that missed or had no effect, leaving Var(Result) on the sentinel set
// up front - which is why the sentinel is written before the branch and not inside it.
EncScript_Uxie_File:
	encsetvar 5, 255  @ Result: no outcome unless encadapt writes one
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Uxie_File_Done
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Uxie_File_Five
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Uxie_File_Four
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Uxie_File_Three
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Uxie_File_Two
	encadapt ENC_TARGET_BOSS, 40, 1, 4, 5
	goto EncScript_Uxie_File_Report
EncScript_Uxie_File_Two:
	encadapt ENC_TARGET_BOSS, 45, 2, 4, 5
	goto EncScript_Uxie_File_Report
EncScript_Uxie_File_Three:
	encadapt ENC_TARGET_BOSS, 55, 2, 4, 5
	goto EncScript_Uxie_File_Report
EncScript_Uxie_File_Four:
	encadapt ENC_TARGET_BOSS, 60, 3, 4, 5
	goto EncScript_Uxie_File_Report
EncScript_Uxie_File_Five:
	encadapt ENC_TARGET_BOSS, 70, 4, 4, 5
EncScript_Uxie_File_Report:
	encjumpifvar CMP_EQUAL, 5, ENC_ADAPT_RESULT_EVICTED, EncScript_Uxie_File_Evicted
	encjumpifvar CMP_EQUAL, 5, ENC_ADAPT_RESULT_FILED, EncScript_Uxie_File_Filed
	encjumpifvar CMP_EQUAL, 5, ENC_ADAPT_RESULT_HARDENED, EncScript_Uxie_File_Hardened
	return            @ nothing landed - the command left the board and both vars alone
EncScript_Uxie_File_Filed:
	printstring STRINGID_ENCUXIEFILED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return
EncScript_Uxie_File_Hardened:
	printstring STRINGID_ENCUXIEHARDENED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return
EncScript_Uxie_File_Evicted:
	printstring STRINGID_ENCUXIEEVICTED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
EncScript_Uxie_File_Done:
	return

// The defiance cost's visible half. The line names what fell, which is the whole tutorial for the
// board - in one sentence, and without a word of instruction, it tells the player Uxie HAS a list
// and that they are the one taking things off it. The purge is skipped outright on an empty board so
// a player who has defied every read is never told about a mechanic they have kept switched off.
EncScript_Uxie_Forget:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Uxie_Forget_Done
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_NEWEST, 4
	printstring STRINGID_ENCUXIEFORGETS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
EncScript_Uxie_Forget_Done:
	return

// A defied read. Below Omniscience it costs a rung and the newest adaptation. In Omniscience
// Knowledge is pinned, so defiance buys the player the adaptation and one step of the STREAK
// instead - three in a row is the finale's second route out.
EncScript_Uxie_Lose:
	call EncScript_Uxie_Forget
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Uxie_Lose_Pinned
	call EncScript_Uxie_LoseOne
	return
EncScript_Uxie_Lose_Pinned:
	encaddvar 9, 1    @ Streak
	encjumpifvar CMP_LESS_THAN, 9, 3, EncScript_Uxie_Lose_Done
	call EncScript_Uxie_Collapse
EncScript_Uxie_Lose_Done:
	return

// A broken vow. Twice the ordinary cost, which is what makes lying a WAGER rather than a free lever
// - the player pays for it with their own next action (encsetrecharge 2 at OnMoveEnd lands on next
// turn, with the engine's stock "must recharge!" line).
EncScript_Uxie_LoseTwo:
	call EncScript_Uxie_Forget
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Uxie_Lose_Pinned
	call EncScript_Uxie_LoseOne
	call EncScript_Uxie_LoseOne
	return

// COLLAPSE. Everything Knowledge was holding up goes at once, and for three full turns Uxie takes
// roughly four and a half times the damage it did. The re-form afterwards is a SAFETY VALVE, not a
// rhythm to farm: the 25%-to-10% band is narrow, and a player who cannot close it in one window
// needs a second attempt rather than a deadlock.
EncScript_Uxie_Collapse:
	encsetvar 1, 0    @ Knowledge
	encsetvar 9, 0    @ Streak
	encsetvar 10, 4   @ Collapse: three full turns of window
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_ALL, 4
	encsetdamagereduction ENC_TARGET_BOSS, 75
	printstring STRINGID_ENCUXIECANNOTCOMPREHEND
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	call EncScript_Uxie_ApplyKnowledge
	return

// The reserve card. Uxie has a model of the player, and a damage category it has never once seen is
// the hole in it. Spent the first time each category is used: cheap in Study, where it teaches the
// mechanic at a point where the lesson is affordable, and the instant COLLAPSE in Omniscience.
// The callers keep this out of a collapse window entirely - Uxie is observing nothing in there, so a
// card burned then would buy the player nothing and leave them without it for a second attempt.
EncScript_Uxie_ReserveCard:
	printstring STRINGID_ENCUXIEUNSEEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Uxie_ReserveCard_Collapse
	call EncScript_Uxie_LoseOne
	return
EncScript_Uxie_ReserveCard_Collapse:
	call EncScript_Uxie_Collapse
	return

// --- The verdict ---

// Reached from all three entry points. Foreseen and VowKept clear the streak: only an unbroken run
// of defiance counts.
EncScript_Uxie_Foreseen:
	encsetvar 9, 0    @ Streak
	printstring STRINGID_ENCUXIEFORESEEN
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Uxie_Gain
	call EncScript_Uxie_File
	return

// A foreseen status move raises Knowledge but files nothing - a resistance filed off a Calm Mind is
// incoherent, and the separate line makes the difference legible rather than looking like a bug.
EncScript_Uxie_ForeseenStat:
	encsetvar 9, 0    @ Streak
	printstring STRINGID_ENCUXIEFORESEENSTAT
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Uxie_Gain
	return

EncScript_Uxie_Defied:
	printstring STRINGID_ENCUXIEDEFIED
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Uxie_Lose
	return

EncScript_Uxie_VowKept:
	encsetvar 9, 0    @ Streak
	printstring STRINGID_ENCUXIEVOWKEPT
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Uxie_Gain
	call EncScript_Uxie_File
	return

EncScript_Uxie_VowKeptStat:
	encsetvar 9, 0    @ Streak
	printstring STRINGID_ENCUXIEVOWKEPT
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Uxie_Gain
	return

EncScript_Uxie_VowBroken:
	printstring STRINGID_ENCUXIEVOWBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	call EncScript_Uxie_LoseTwo
	printstring STRINGID_ENCUXIEBACKLASH
	waitmessage B_WAIT_TIME_LONG
	encsetrecharge ENC_TARGET_PLAYER_LEFT, 2
	return

// No read was taken, or Uxie is mid-collapse. Nothing is graded, and the streak resets - it counts
// consecutive DEFIED reads, and a turn with no read is not one.
EncScript_Uxie_NoVerdict:
	encsetvar 9, 0    @ Streak
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the Test clock needs seeding. Deliberately does
// NOT call ApplyKnowledge: Knowledge 0 is the neutral state and the stages are already there, and
// announcing the K0 rung on turn one would spend "Uxie is reconsidering what it knows" before it can
// mean anything.
EncScript_Uxie_Intro::
	encsetvar 11, 6   @ TestClock: the first Test lands on turn 5
	printstring STRINGID_ENCUXIEAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// or opens a window before printing renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail so no branch out of this script can leave it re-armed.
// The read is taken here because OnTurnStart is the only checkpoint where gAiLogicData->predictedMove
// is meaningful - it is written during action selection, which has just finished.
EncScript_Uxie_TurnOpen::
	flushtextbox
	encsetvar 12, 1   @ TurnGuard
	encsetvar 13, 0   @ MoveGuard
	encsetvar 14, 0   @ FaintGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Uxie_TurnOpen_Done   @ Weakened: inert
	encjumpifvar CMP_GREATER_THAN, 10, 1, EncScript_Uxie_TurnOpen_Tick
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Uxie_TurnOpen_Reeling      @ Reform resolves at OnTurnEnd
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Uxie_TurnOpen_Read   @ Omniscience: no more Tests
	encjumpifvar CMP_LESS_THAN, 11, 2, EncScript_Uxie_TurnOpen_Read
	encsubvar 11, 1   @ TestClock
EncScript_Uxie_TurnOpen_Read:
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 2
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Uxie_TurnOpen_NoRead
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Uxie_TurnOpen_Announce
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Uxie_Test
EncScript_Uxie_TurnOpen_Announce:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Uxie_TurnOpen_Stat
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Uxie_TurnOpen_Spec
	printstring STRINGID_ENCUXIEREADPHYS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Uxie_TurnOpen_Spec:
	printstring STRINGID_ENCUXIEREADSPEC
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Uxie_TurnOpen_Stat:
	printstring STRINGID_ENCUXIEREADSTAT
	waitmessage B_WAIT_TIME_SHORT
	return
// The clock is left where it is on a readless turn, so a Test is postponed rather than wasted.
EncScript_Uxie_TurnOpen_NoRead:
	printstring STRINGID_ENCUXIENOREAD
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Uxie_TurnOpen_Tick:
	encsubvar 10, 1   @ Collapse
EncScript_Uxie_TurnOpen_Reeling:
	encsetvar 2, 0    @ Read: the window takes none
	printstring STRINGID_ENCUXIEREELING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Uxie_TurnOpen_Done:
	return

// THE TEST. The read is put to the player as a vow instead of an announcement, and the OnMoveEnd
// verdict then grades WORD against DEED rather than move against prediction. The vow rides in
// Var(Read) as a sentinel offset - +10 vowed yes, +20 vowed no - rather than in a second variable.
// The question is always about the category Uxie actually predicts, so denying it always means
// abandoning the option the engine rates as the player's best.
EncScript_Uxie_Test:
	encsetvar 11, 6   @ TestClock: re-armed
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Uxie_Test_Stat
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Uxie_Test_Spec
	encaskyesno STRINGID_ENCUXIETESTPHYS, EncScript_Uxie_Test_Yes
	goto EncScript_Uxie_Test_No
EncScript_Uxie_Test_Spec:
	encaskyesno STRINGID_ENCUXIETESTSPEC, EncScript_Uxie_Test_Yes
	goto EncScript_Uxie_Test_No
EncScript_Uxie_Test_Stat:
	encaskyesno STRINGID_ENCUXIETESTSTAT, EncScript_Uxie_Test_Yes
EncScript_Uxie_Test_No:
	encaddvar 2, 20   @ Read: vowed NO on this category
	printstring STRINGID_ENCUXIEVOWNO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Uxie_Test_Yes:
	encaddvar 2, 10   @ Read: vowed YES on this category
	printstring STRINGID_ENCUXIEVOWYES
	waitmessage B_WAIT_TIME_SHORT
	return

// The three verdict entry points, one per damage category, because Event.MoveCategory is readable in
// a CONDITION but not from inside a script. Each sets MoveGuard first - an Event.* condition stays
// true for the whole dispatch and a script cannot move it, so an event-reacting trigger has to
// disable itself or the re-evaluation loop re-selects it until the per-checkpoint script cap trips.
// That also makes the verdict exactly once per turn however many strikes a multi-hit move landed.
// Then each spends its own reserve card if this is the first sighting of that category, and runs the
// same six-line branch table on Var(Read) with its own category number substituted:
//   0 nothing read / 10+own vow kept / 20+own vow broken / >20 vow kept / >10 vow broken /
//   ==own foreseen / otherwise defied.
EncScript_Uxie_JudgePhys::
	encsetvar 13, 1   @ MoveGuard
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Uxie_NoVerdict   @ reeling: it observes nothing
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Uxie_JudgePhys_Seen
	encsetvar 6, 1    @ SeenPhys
	call EncScript_Uxie_ReserveCard
EncScript_Uxie_JudgePhys_Seen:
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Uxie_NoVerdict   @ the card just collapsed it
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Uxie_NoVerdict
	encjumpifvar CMP_EQUAL, 2, 11, EncScript_Uxie_VowKept
	encjumpifvar CMP_EQUAL, 2, 21, EncScript_Uxie_VowBroken
	encjumpifvar CMP_GREATER_THAN, 2, 20, EncScript_Uxie_VowKept
	encjumpifvar CMP_GREATER_THAN, 2, 10, EncScript_Uxie_VowBroken
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Uxie_Foreseen
	goto EncScript_Uxie_Defied

EncScript_Uxie_JudgeSpec::
	encsetvar 13, 1   @ MoveGuard
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Uxie_NoVerdict   @ reeling: it observes nothing
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Uxie_JudgeSpec_Seen
	encsetvar 7, 1    @ SeenSpec
	call EncScript_Uxie_ReserveCard
EncScript_Uxie_JudgeSpec_Seen:
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Uxie_NoVerdict   @ the card just collapsed it
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Uxie_NoVerdict
	encjumpifvar CMP_EQUAL, 2, 12, EncScript_Uxie_VowKept
	encjumpifvar CMP_EQUAL, 2, 22, EncScript_Uxie_VowBroken
	encjumpifvar CMP_GREATER_THAN, 2, 20, EncScript_Uxie_VowKept
	encjumpifvar CMP_GREATER_THAN, 2, 10, EncScript_Uxie_VowBroken
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Uxie_Foreseen
	goto EncScript_Uxie_Defied

// The status entry routes its own kept/foreseen outcomes to the no-filing variants.
EncScript_Uxie_JudgeStat::
	encsetvar 13, 1   @ MoveGuard
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Uxie_NoVerdict   @ reeling: it observes nothing
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Uxie_JudgeStat_Seen
	encsetvar 8, 1    @ SeenStat
	call EncScript_Uxie_ReserveCard
EncScript_Uxie_JudgeStat_Seen:
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Uxie_NoVerdict   @ the card just collapsed it
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Uxie_NoVerdict
	encjumpifvar CMP_EQUAL, 2, 13, EncScript_Uxie_VowKeptStat
	encjumpifvar CMP_EQUAL, 2, 23, EncScript_Uxie_VowBroken
	encjumpifvar CMP_GREATER_THAN, 2, 20, EncScript_Uxie_VowKeptStat
	encjumpifvar CMP_GREATER_THAN, 2, 10, EncScript_Uxie_VowBroken
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Uxie_ForeseenStat
	goto EncScript_Uxie_Defied

// One of the player's Pokemon has gone down. The loss is data too.
EncScript_Uxie_Enlightened::
	encsetvar 14, 1   @ FaintGuard
	printstring STRINGID_ENCUXIEENLIGHTENED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Uxie_GainOne
	return

// The anti-cheese, and deliberately narrow. Sleep is the one status that would stop the read
// happening at all, so it is the one Uxie throws off - and the attempt teaches it something.
// Burn, paralysis, poison and freeze all stick; it is Uxie's MIND that cannot be dulled, not its
// body, which keeps the trio from reading as the same blanket wall three times.
// curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at a turn checkpoint.
EncScript_Uxie_RefusesSleep::
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCUXIEREFUSESSLEEP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	call EncScript_Uxie_GainOne
	return

// The collapse window closing. Knowledge comes straight back to 5 rather than climbing again - the
// window is a second attempt at the finale, not a second half of the fight.
EncScript_Uxie_Reform::
	encsetvar 10, 0   @ Collapse
	encsetvar 1, 5    @ Knowledge
	encsetvar 9, 0    @ Streak
	encsetdamagereduction ENC_TARGET_BOSS, 90
	printstring STRINGID_ENCUXIEREFORMS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Uxie_ApplyKnowledge
	return

// Checks MoveGuard BEFORE clearing it: a turn in which the player never used a move - a switch, a
// full paralysis, the recharge a broken vow cost them - is not a defiance, so the streak resets.
EncScript_Uxie_TurnClose::
	encjumpifvar CMP_GREATER_THAN, 13, 0, EncScript_Uxie_TurnClose_Acted
	encsetvar 9, 0    @ Streak
EncScript_Uxie_TurnClose_Acted:
	encsetvar 12, 0   @ TurnGuard
	encsetvar 13, 0   @ MoveGuard
	encsetvar 14, 0   @ FaintGuard
	encsetvar 2, 0    @ Read
	return

// 50% - COMPLETE UNDERSTANDING. The ceiling lifts from 3 to 5 and correct reads start paying +2, so
// a player who has been feeding Uxie their best move can be facing a 4-slot 70% board within three
// turns of this line.
EncScript_Uxie_Understanding::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCUXIEUNDERSTANDING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return

// 25% - OMNISCIENCE. Knowledge is pinned at 5 and ordinary defiance no longer lowers it; it only
// strips the newest adaptation and advances the streak. Two routes out, and both exist so that a
// player who spent all three categories early is never locked out of the finale: land a move of a
// category Uxie has never seen, or defy the live read three turns running.
EncScript_Uxie_Omniscience::
	encsetvar 0, 2    @ Phase
	encsetvar 1, 5    @ Knowledge: pinned
	encsetvar 9, 0    @ Streak
	printstring STRINGID_ENCUXIEOMNISCIENCE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Uxie_ApplyKnowledge
	return

// 10% - the catch window, the mirror image of the Properties: block. Guideline 10 exactly: no ball
// can be thrown before this point. Every counter the fight built is zeroed so nothing outlives it,
// and Phase 3 makes TurnOpen return through its leading branch, so no clock can restart.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Uxie_Weakened::
	encsetvar 0, 3    @ Phase
	encsetvar 1, 0    @ Knowledge
	encsetvar 3, 0    @ Tier
	encsetvar 9, 0    @ Streak
	encsetvar 10, 0   @ Collapse
	encsetvar 11, 0   @ TestClock
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_ALL, 4
	call EncScript_Uxie_NeutralStats
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCUXIEWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
