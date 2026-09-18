// Dialga, "The Temporal Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Dialga_Intro:
// 0 Phase (0 The Flow of Time / 1 Temporal Acceleration / 2 Origin Forme / 3 The Convergence),
// 1 Charge (0-5), 2 LastCharge (the last ANNOUNCED level), 3 Clock, 4 Mark, 5 Dealt (scratch),
// 6 TurnGuard, 7 Hit, 8 Flight (ledger), 9 Patience (ledger), 10 EventGuard, 11 Fracture
// (0 none / 1 RECOIL / 2 LOCK / 3 CLOSING), 12 Aged, 13 TurnMark (RECOIL scratch), 14 EchoPending.
//
// Celebi's rewind is a surprise you try to anchor against. Dialga's is a PUBLISHED deadline: it
// states the price and the clock before the interval begins, and prints a countdown line every turn
// one is live. Nothing here is a new command - the whole fight is encsnapshothp, encrewindhp and the
// existing enc* set.
//
// Animation note: gBattlerAttacker and gBattleScripting.animTurn are both stale at OnTurnStart and
// OnTurnEnd, so anything direction-sensitive uses playanimation with an explicit battler rather than
// playmoveanimation. That also rules out MOVE_FUTURE_SIGHT, whose animation is a choosetwoturnanim
// that picks its arm off the stale turn counter.

// --- Shared subroutines (call/return) ---

// The guard, by Charge. Written absolutely, so the meter falling undoes the meter rising with no
// separate teardown path.
EncScript_Dialga_ApplyGuard:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Dialga_ApplyGuard_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Dialga_ApplyGuard_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Dialga_ApplyGuard_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Dialga_ApplyGuard_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Dialga_ApplyGuard_4
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Dialga_ApplyGuard_0:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Dialga_ApplyGuard_1:
	encsetdamagereduction ENC_TARGET_BOSS, 85
	return
EncScript_Dialga_ApplyGuard_2:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Dialga_ApplyGuard_3:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Dialga_ApplyGuard_4:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return

// THE CALLOUT. The guard is invisible, so every real move of it has to speak. LastCharge holds the
// last ANNOUNCED level; each arm compares it against that arm's own literal, which is the only way
// to express a var-to-var equality when every comparison takes a literal. It fires in BOTH
// directions, so a hold that drops the Charge speaks as loudly as a collapse that raises it.
// LastCharge is written last, alongside the guard, so a level that did not move stays silent.
EncScript_Dialga_Announce:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Dialga_Announce_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Dialga_Announce_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Dialga_Announce_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Dialga_Announce_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Dialga_Announce_4
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Dialga_Announce_Done
	printstring STRINGID_ENCDIALGACHARGE5
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	goto EncScript_Dialga_Announce_Apply
EncScript_Dialga_Announce_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Dialga_Announce_Done
	printstring STRINGID_ENCDIALGACHARGE0
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Dialga_Announce_Apply
EncScript_Dialga_Announce_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Dialga_Announce_Done
	printstring STRINGID_ENCDIALGACHARGE1
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Dialga_Announce_Apply
EncScript_Dialga_Announce_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Dialga_Announce_Done
	printstring STRINGID_ENCDIALGACHARGE2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Dialga_Announce_Apply
EncScript_Dialga_Announce_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Dialga_Announce_Done
	printstring STRINGID_ENCDIALGACHARGE3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	goto EncScript_Dialga_Announce_Apply
EncScript_Dialga_Announce_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Dialga_Announce_Done
	printstring STRINGID_ENCDIALGACHARGE4
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Dialga_Announce_Apply:
	call EncScript_Dialga_ApplyGuard
	enccopyvar 2, 1     @ LastCharge = Charge
EncScript_Dialga_Announce_Done:
	return

// Charge +1, capped at 5, then announced.
EncScript_Dialga_RaiseCharge:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Dialga_RaiseCharge_Done
	encaddvar 1, 1
	call EncScript_Dialga_Announce
EncScript_Dialga_RaiseCharge_Done:
	return

// Charge -1, floored by the phase (Phase 0 floors at 0, Phase 1 at 2, Phase 2 pins at 5), then
// announced. This is the only lever the player has on the meter, and it is the same action the
// fight is already asking for: hit the quota.
EncScript_Dialga_LowerCharge:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Dialga_LowerCharge_Done   @ Phase 2+: pinned
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Dialga_LowerCharge_Ph1
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Dialga_LowerCharge_Done
	goto EncScript_Dialga_LowerCharge_Do
EncScript_Dialga_LowerCharge_Ph1:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Dialga_LowerCharge_Done
EncScript_Dialga_LowerCharge_Do:
	encsubvar 1, 1
	call EncScript_Dialga_Announce
EncScript_Dialga_LowerCharge_Done:
	return

// The clock, by Charge: 4/4/3/3/2/2. Temporal Acceleration is exactly one subtraction - from Phase 1
// every interval is a turn shorter, floored at 2.
EncScript_Dialga_SetClock:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Dialga_SetClock_Fast
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Dialga_SetClock_Mid
	encsetvar 3, 4
	goto EncScript_Dialga_SetClock_Accel
EncScript_Dialga_SetClock_Mid:
	encsetvar 3, 3
	goto EncScript_Dialga_SetClock_Accel
EncScript_Dialga_SetClock_Fast:
	encsetvar 3, 2
EncScript_Dialga_SetClock_Accel:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Dialga_SetClock_Done
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Dialga_SetClock_Done
	encsubvar 3, 1
EncScript_Dialga_SetClock_Done:
	return

// The quota and the clock, stated in prose rather than digits - one string per Charge rung, the
// same shape every other encounter uses for a per-value banner. This is the promise the fight then
// keeps: nothing is ever taken back that the player was not told about in advance.
EncScript_Dialga_StateQuota:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Dialga_StateQuota_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Dialga_StateQuota_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Dialga_StateQuota_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Dialga_StateQuota_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Dialga_StateQuota_4
	printstring STRINGID_ENCDIALGAQUOTA5
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Dialga_StateQuota_0:
	printstring STRINGID_ENCDIALGAQUOTA0
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Dialga_StateQuota_1:
	printstring STRINGID_ENCDIALGAQUOTA1
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Dialga_StateQuota_2:
	printstring STRINGID_ENCDIALGAQUOTA2
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Dialga_StateQuota_3:
	printstring STRINGID_ENCDIALGAQUOTA3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Dialga_StateQuota_4:
	printstring STRINGID_ENCDIALGAQUOTA4
	waitmessage B_WAIT_TIME_LONG
	return

// The recurring line, printed every turn an interval is live. A player who missed the opening banner
// still cannot miss that something is counting down.
EncScript_Dialga_Countdown:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Dialga_Countdown_1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Dialga_Countdown_2
	printstring STRINGID_ENCDIALGACOUNT3
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Dialga_Countdown_2:
	printstring STRINGID_ENCDIALGACOUNT2
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Dialga_Countdown_1:
	printstring STRINGID_ENCDIALGACOUNT1
	waitmessage B_WAIT_TIME_SHORT
	return

// AGE, from Phase 1: every mark wears the player's active Pokemon down.
// encchangestatvalue writes the RAW stat the battler was built with - permanent for that battler,
// invisible on the stat screen, not undone by Haze, and RESET the moment a fresh Pokemon switches
// in. So aging compounds on a Pokemon that stays in (0.85, 0.72, 0.61, ...) and is zero on one that
// does not. Rotating the team is the intended counterplay, which is why Dialga is never given
// encsettrapped before Origin Forme; the price for taking it is the Flight ledger, not a lockout.
// encclearscreens plus removeweather is "durations advance" made concrete - screens, Tailwind,
// Safeguard and weather are exactly the timed effects a player sets down.
// playanimation with an explicit battler rather than playmoveanimation: AGE runs at OnTurnStart,
// where gBattlerAttacker is stale.
EncScript_Dialga_Age:
	encjumpifvar CMP_GREATER_THAN, 12, 200, EncScript_Dialga_Age_Announce
	encaddvar 12, 1
EncScript_Dialga_Age_Announce:
	printstring STRINGID_ENCDIALGAAGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	encchangestatvalue ENC_TARGET_ALL_FOES, STAT_ATK, -15, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_ALL_FOES, STAT_SPATK, -15, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_ALL_FOES, STAT_SPEED, -15, ENC_AMOUNT_PERCENT
	encsetembargo ENC_TARGET_ALL_FOES, 3
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Dialga_Age_NoScreens
	printstring STRINGID_ENCDIALGAAGEWEARS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Dialga_Age_NoScreens:
	removeweather
	encjumpifvar CMP_LESS_THAN, 12, 3, EncScript_Dialga_Age_Done
	printstring STRINGID_ENCDIALGAAGEDEEP
	waitmessage B_WAIT_TIME_SHORT
EncScript_Dialga_Age_Done:
	return

// THE MARK. Opens an interval: record the HP percentage, shed whatever status Dialga is carrying
// (it returns to the moment before it was afflicted - the fight's own mechanic applied to itself),
// set the clock, and state the price.
EncScript_Dialga_OpenInterval:
	printstring STRINGID_ENCDIALGAMARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Dialga_OpenInterval_Shed
	goto EncScript_Dialga_OpenInterval_Clock
EncScript_Dialga_OpenInterval_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCDIALGASHED
	waitmessage B_WAIT_TIME_LONG
EncScript_Dialga_OpenInterval_Clock:
	call EncScript_Dialga_SetClock
	call EncScript_Dialga_StateQuota
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Dialga_OpenInterval_Done
	call EncScript_Dialga_Age
EncScript_Dialga_OpenInterval_Done:
	return

// THE VERDICT. Mark is copied into the scratch first so ENC_SNAP_DAMAGE can overwrite it with
// "percentage of max HP taken off since the mark" without losing the mark itself - encrewindhp still
// needs it. The snapshot's failLabel is the zero-damage case, which goes straight to the collapse.
// The per-rung branch exists because every comparison takes a literal; the quota is a pure function
// of Charge, so six literal branches express it exactly and no Demand variable is needed.
EncScript_Dialga_Resolve:
	enccopyvar 5, 4
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Dialga_Collapse
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Dialga_Resolve_Q0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Dialga_Resolve_Q1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Dialga_Resolve_Q2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Dialga_Resolve_Q3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Dialga_Resolve_Q4
	encjumpifvar CMP_LESS_THAN, 5, 7, EncScript_Dialga_Collapse
	goto EncScript_Dialga_Hold
EncScript_Dialga_Resolve_Q0:
	encjumpifvar CMP_LESS_THAN, 5, 10, EncScript_Dialga_Collapse
	goto EncScript_Dialga_Hold
EncScript_Dialga_Resolve_Q1:
	encjumpifvar CMP_LESS_THAN, 5, 8, EncScript_Dialga_Collapse
	goto EncScript_Dialga_Hold
EncScript_Dialga_Resolve_Q2:
	encjumpifvar CMP_LESS_THAN, 5, 6, EncScript_Dialga_Collapse
	goto EncScript_Dialga_Hold
EncScript_Dialga_Resolve_Q3:
	encjumpifvar CMP_LESS_THAN, 5, 5, EncScript_Dialga_Collapse
	goto EncScript_Dialga_Hold
EncScript_Dialga_Resolve_Q4:
	encjumpifvar CMP_LESS_THAN, 5, 4, EncScript_Dialga_Collapse
	goto EncScript_Dialga_Hold

// THE TIMELINE HOLDS. The staggered turn is the reward, and it is the most valuable thing in the
// fight: a free turn against a boss whose guard just dropped a rung. Two holds in a row is how a
// player actually wins this. encsetrecharge 2 at OnTurnEnd costs Dialga NEXT turn's action.
EncScript_Dialga_Hold:
	printstring STRINGID_ENCDIALGAHOLD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetrecharge ENC_TARGET_BOSS, 2
	printstring STRINGID_ENCDIALGASTAGGER
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Dialga_LowerCharge
	return

// THE INTERVAL COLLAPSES. encrewindhp moves Dialga's HP TO the marked percentage, and normalisebuffs
// Hazes EVERY battler, Dialga included - the honest reading of "those turns did not happen", and
// what stops the collapse from being pure punishment: a player who spent the interval being set up
// on gets that undone too.
EncScript_Dialga_Collapse:
	printstring STRINGID_ENCDIALGACOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encrewindhp ENC_TARGET_BOSS, 4
	normalisebuffs
	printstring STRINGID_ENCDIALGAUNDONE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Dialga_RaiseCharge
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Dialga_Collapse_Done
	encsetvar 14, 1     @ EchoPending - the two ECHO triggers resolve it later this checkpoint
EncScript_Dialga_Collapse_Done:
	return

// --- TIME FRACTURE: the three futures ---

// The ledger, evaluated once at Origin Forme, in a fixed order, all against literals. The order is
// the design statement: the fight names the thing the player leaned on hardest, and aggression is
// the default because it is the behaviour a damage-reduced boss most reliably produces.
EncScript_Dialga_ChooseFuture:
	encjumpifvar CMP_GREATER_THAN, 8, 2, EncScript_Dialga_ChooseFuture_Lock
	encjumpifvar CMP_GREATER_THAN, 9, 5, EncScript_Dialga_ChooseFuture_Closing
	encsetvar 11, 1
	printstring STRINGID_ENCDIALGAFUTRECOIL
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Dialga_ChooseFuture_Lock:
	encsetvar 11, 2
	printstring STRINGID_ENCDIALGAFUTLOCK
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Dialga_ChooseFuture_Closing:
	encsetvar 11, 3
	printstring STRINGID_ENCDIALGAFUTCLOSING
	waitmessage B_WAIT_TIME_LONG
	return

// The turn-start half of whichever future is live. Called from TurnOpen, which has already run
// flushtextbox. Heal Block is re-armed every turn so it never lapses under the B_HEAL_BLOCK_TIMER
// ceiling; encsettrapped is idempotent, so re-applying it costs nothing and the line stays as the
// recurring callout the mechanic needs.
EncScript_Dialga_FractureOpen:
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Dialga_FractureOpen_Lock
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Dialga_FractureOpen_Closing
	encsnapshothp ENC_TARGET_BOSS, 13, ENC_SNAP_SET     @ RECOIL: this turn's mark
	return
EncScript_Dialga_FractureOpen_Lock:
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCDIALGALOCK
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Dialga_FractureOpen_Closing:
	encsethealblock ENC_TARGET_ALL_FOES, B_HEAL_BLOCK_TIMER
	return

// The turn-end half.
// RECOIL: damage taken is answered with a fixed bite, only on turns Dialga was actually hurt - the
//   glass-cannon answer stops being free.
// LOCK: the exit is closed and the aging that switching used to answer now compounds every turn.
// CLOSING: stalling stops working. Healing does not land, and a turn that ends with no damaging move
//   committed is charged for outright. This is the plan's "no-reward deadline" expressed without a
//   second clock: in Origin Forme the deadline machinery is stopped, so re-opening it for one future
//   would read as a bug rather than as a punishment. The Hit flag the Patience ledger already tracks
//   says exactly the same thing in one line.
EncScript_Dialga_FractureClose:
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Dialga_FractureClose_Lock
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Dialga_FractureClose_Closing
	enccopyvar 5, 13
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Dialga_FractureClose_Done
	printstring STRINGID_ENCDIALGARECOIL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Dialga_FractureClose_Lock:
	call EncScript_Dialga_Age
	return
EncScript_Dialga_FractureClose_Closing:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Dialga_FractureClose_Done
	printstring STRINGID_ENCDIALGACLOSING
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
EncScript_Dialga_FractureClose_Done:
	return

// --- Trigger scripts ---

EncScript_Dialga_Intro::
	printstring STRINGID_ENCDIALGAINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDIALGAINTRO2
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn. Leads with flushtextbox: an OnTurnStart script that animates before printing
// anything renders on top of the still-open action menu. TurnGuard is set immediately after, so no
// branch out of this script can leave it re-armed.
EncScript_Dialga_TurnOpen::
	flushtextbox
	encsetvar 6, 1      @ TurnGuard
	encsetvar 10, 0     @ EventGuard - a new turn re-arms switch/faint accounting
	encsetvar 7, 0      @ Hit
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Dialga_TurnOpen_Done       @ The Convergence: no clock
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Dialga_TurnOpen_Fracture   @ Origin Forme: suspended
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Dialga_TurnOpen_Tick
	call EncScript_Dialga_OpenInterval
	return
EncScript_Dialga_TurnOpen_Tick:
	call EncScript_Dialga_Countdown
	return
EncScript_Dialga_TurnOpen_Fracture:
	call EncScript_Dialga_FractureOpen
EncScript_Dialga_TurnOpen_Done:
	return

// "They committed to attacking." Hit is this trigger's own re-entry guard as well as the Patience
// ledger's input - an Event.* condition stays true for the whole dispatch, so a trigger reacting to
// a move has to disable itself through a variable its own script writes.
EncScript_Dialga_TheyStruck::
	encsetvar 7, 1
	return

// The Flight ledger. Deliberately silent: the ledger is not meant to be readable until TIME FRACTURE
// names it. EventGuard stops a forced replacement after a faint from also being charged as a switch.
EncScript_Dialga_TheyFled::
	encsetvar 10, 1
	encjumpifvar CMP_GREATER_THAN, 8, 200, EncScript_Dialga_TheyFled_Done
	encaddvar 8, 1
EncScript_Dialga_TheyFled_Done:
	return

// A faint feeds the meter directly, in every phase - losing a Pokemon outright is a harsher beat
// than choosing to pull one back, and it is the one thing besides a missed quota that raises Charge.
EncScript_Dialga_TheyFell::
	encsetvar 10, 1
	call EncScript_Dialga_RaiseCharge
	return

// Runs last so it sees the finished turn. TurnGuard/EventGuard are cleared first - this trigger's
// own self-disabling write, and the switch/faint guard alongside it.
// Ticking here and opening at OnTurnStart is what keeps the countdown honest: a timer that both
// ticks and resolves at one checkpoint chases itself through every state in a single dispatch.
EncScript_Dialga_TurnClose::
	encsetvar 6, 0      @ TurnGuard
	encsetvar 10, 0     @ EventGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Dialga_TurnClose_Done
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Dialga_TurnClose_Phase
	encjumpifvar CMP_GREATER_THAN, 9, 200, EncScript_Dialga_TurnClose_Phase
	encaddvar 9, 1      @ Patience
EncScript_Dialga_TurnClose_Phase:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Dialga_TurnClose_Fracture
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Dialga_TurnClose_Done      @ no interval live
	encsubvar 3, 1      @ Clock
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Dialga_TurnClose_Done
	call EncScript_Dialga_Resolve
	return
EncScript_Dialga_TurnClose_Fracture:
	call EncScript_Dialga_FractureClose
EncScript_Dialga_TurnClose_Done:
	return

// THE ECHO. A collapse in Phase 1 or later tears a strike out of the erased interval. Because the
// countdown is public, a player who reads the fight knows which turn is worth spending on Protect -
// which is what the second arm is for. Both arms clear EchoPending so neither can re-select.
EncScript_Dialga_Echo::
	encsetvar 14, 0
	printstring STRINGID_ENCDIALGAECHO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return

EncScript_Dialga_EchoBlocked::
	encsetvar 14, 0
	printstring STRINGID_ENCDIALGAECHO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_PROTECTED_ITSELF
	printstring STRINGID_ENCDIALGAECHOBLOCKED
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	return

// --- Phases ---

// 50% - TEMPORAL ACCELERATION. Three things change from here: clocks are one turn shorter, every
// mark AGEs the player's active Pokemon, and every collapse fires THE ECHO. The live interval is
// abandoned rather than resolved, so the first shorter one opens cleanly next turn.
EncScript_Dialga_Acceleration::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCDIALGAACCEL1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	printstring STRINGID_ENCDIALGAACCEL2
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Dialga_Acceleration_Announce
	encsetvar 1, 2      @ Charge, floored at 2 from here
EncScript_Dialga_Acceleration_Announce:
	call EncScript_Dialga_Announce
	encsetvar 3, 0      @ Clock
	encsetvar 14, 0     @ EchoPending
	return

// 20% - ORIGIN FORME and TIME FRACTURE. SPECIES_DIALGA_ORIGIN keeps Steel/Dragon and moves 20 points
// out of Attack into Sp. Defense, which is a readable change against a special-attacking Dialga's
// own fight: harder to wear down, no harder to survive. The form change keeps HP and the moveset,
// so only the signature move has to be restated - and Roar of Time's own recharge turn is a gift to
// the player, which is the right shape for a phase that is otherwise a difficulty spike.
// The deadline machinery stops here; the battle-long ledger picks the finale instead.
EncScript_Dialga_OriginForme::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCDIALGAORIGIN1
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_DIALGA_ORIGIN, EncScript_Dialga_OriginForme_Body, B_ANIM_FORM_CHANGE
EncScript_Dialga_OriginForme_Body:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_ROAR_OF_TIME
	printstring STRINGID_ENCDIALGAORIGIN2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 5      @ Charge, pinned
	call EncScript_Dialga_Announce
	encsetvar 3, 0      @ Clock
	encsetvar 14, 0     @ EchoPending
	printstring STRINGID_ENCDIALGAFRACTURE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Dialga_ChooseFuture
	return

// 10% - THE CONVERGENCE, and the catch window. Whichever future was live is released first, so the
// player is not still trapped or Heal Blocked while trying to throw. Dialga stays in Origin Forme:
// the player catches the thing they beat.
// The 99 sets the value the automatic catch-window damage guard restores, not the live one - the
// guard pins the reduction at maximum from the moment ENC_BALLS_ALLOWED is set.
EncScript_Dialga_Convergence::
	encsetvar 0, 3      @ Phase
	printstring STRINGID_ENCDIALGACONV1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encsetvar 11, 0     @ Fracture
	encsetvar 3, 0      @ Clock
	encsetvar 14, 0     @ EchoPending
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCDIALGACONV2
	waitmessage B_WAIT_TIME_LONG
	return
