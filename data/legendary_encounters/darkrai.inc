// Darkrai, "The Pitch-Black Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Darkrai_Intro:
// 0 Phase (0 Pitch-Black / 1 The Nightmare Deepens / 2 Absolute Nightmare / 3 Weakened),
// 1 Nightmare (0-5), 2 LastNm (the last ANNOUNCED level), 3 Wake (0-3), 4 Clock, 5 TurnGuard,
// 6 MoveGuard (the one-Wakefulness-gain-per-turn latch), 7 LastCat, 8 FaintGuard, 9 Copy (7-4 a copy
// is up, 3-1 cooling down, 0 free), 10 PlayerStatus, 11 Shatter, 12 AskCd, 13 HpMark (255 = no
// FALSE COLLAPSE pending), 14 CatGuard, 15 BossStatus.
//
// Two meters pull against each other and the guard is a direct read of one of them, so "can I hurt
// this thing" and "am I winning the meter war" are the same question. On top of that sits a stream
// of battle text that is not always true - the lies are printstring with no command behind them,
// which is why this encounter needs no new engine work at all. Every one of them opens with
// EncScript_Darkrai_Whisper and nothing else calls it, so the ripple is the fixed tell.

// --- Shared subroutines (call/return) ---

// THE CUE. Cmd_statusanimation never writes status1, so this is a pure signal with no mechanical
// weight - the same trick regice.inc uses for its cosmetic freeze. Learning it is the reward: a
// first run gets fooled, a second reads the ripple and ignores everything after it.
EncScript_Darkrai_Whisper:
	printstring STRINGID_ENCDARKRAIWHISPER
	waitmessage B_WAIT_TIME_SHORT
	volatileanimation BS_OPPONENT1, VOLATILE_NIGHTMARE
	return

// THE SOLE OWNER of the guard and the Nightmare callout. Nothing else in the file writes
// encsetdamagereduction on the boss except EncScript_Darkrai_Shatter's window and the Weakened
// script, which is what makes this safe to call from anywhere and idempotent when the level did not
// move. LastNm holds the last ANNOUNCED level, so crossings speak in both directions and a level
// re-applied without moving stays quiet.
// The Shatter window owns the guard outright while it is open, so this bails first.
EncScript_Darkrai_ApplyNightmare:
	encjumpifvar CMP_GREATER_THAN, 11, 0, EncScript_Darkrai_Nm_Done
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Darkrai_Nm5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Darkrai_Nm4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Darkrai_Nm3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Darkrai_Nm2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Darkrai_Nm1

// Nightmare 0. Below the guideline baseline on purpose: winning the meter war outright is meant to
// feel like the boss stopped being a wall, not like it got slightly easier.
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Darkrai_Nm_Done
	encsetvar 2, 0
	printstring STRINGID_ENCDARKRAINM0
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return

EncScript_Darkrai_Nm1:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Darkrai_Nm_Done
	encsetvar 2, 1
	printstring STRINGID_ENCDARKRAINM1
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Darkrai_Nm2:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Darkrai_Nm_Done
	encsetvar 2, 2
	printstring STRINGID_ENCDARKRAINM2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_SCARED
	return

// From here the lies begin in Phase 0. The guideline baseline of -90% sits across levels 2-3, which
// is where a player working the meter actually holds the fight.
EncScript_Darkrai_Nm3:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Darkrai_Nm_Done
	encsetvar 2, 3
	printstring STRINGID_ENCDARKRAINM3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	return

// NIGHTMARE COPIES unlock here, and with them the quiet consequence: while Darkrai is wearing the
// player's face it is not Dark-typed, so their best Wakefulness source shuts off on its own.
EncScript_Darkrai_Nm4:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Darkrai_Nm_Done
	encsetvar 2, 4
	printstring STRINGID_ENCDARKRAINM4
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// The floor of the fight: guard 96 with no counter-play left but Wakefulness. A player who ignores
// the meter entirely ends here and will not win. The fight has told them something is wrong six
// different ways before it gets this far.
EncScript_Darkrai_Nm5:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Darkrai_Nm_Done
	encsetvar 2, 5
	printstring STRINGID_ENCDARKRAINM5
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Darkrai_Nm_Done:
	return

EncScript_Darkrai_RaiseNightmare:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Darkrai_RaiseNightmare_Done
	encaddvar 1, 1
	call EncScript_Darkrai_ApplyNightmare
EncScript_Darkrai_RaiseNightmare_Done:
	return

EncScript_Darkrai_LowerNightmare:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Darkrai_LowerNightmare_Done
	encsubvar 1, 1
	call EncScript_Darkrai_ApplyNightmare
EncScript_Darkrai_LowerNightmare_Done:
	return

// MoveGuard is written first and unconditionally: it caps Wakefulness at one gain a turn whatever
// the source, and it is also the self-disabling condition for both triggers that reach here.
EncScript_Darkrai_RaiseWake:
	encsetvar 6, 1
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Darkrai_RaiseWake_Done
	encaddvar 3, 1
	printstring STRINGID_ENCDARKRAIWAKEGAIN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Darkrai_RaiseWake_Done:
	return

// Only slot 3 moves. Slots 0-2 are Dark Pulse / Hypnosis / Hex on the PARTY mon (the Moves: property
// writes the party Pokemon, not the battler), so encuntransform restores them for free and
// re-stating them here would do nothing but refill their PP.
EncScript_Darkrai_ApplyMoveset:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Darkrai_ApplyMoveset_Deepened
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Darkrai_ApplyMoveset_Absolute
	encsetmove ENC_TARGET_BOSS, 3, MOVE_NASTY_PLOT
	return
EncScript_Darkrai_ApplyMoveset_Deepened:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_DARK_VOID
	return
EncScript_Darkrai_ApplyMoveset_Absolute:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_DREAM_EATER
	return

// Ends a NIGHTMARE COPY, from all four of its callers. Copy drops to 3 rather than 0 so an early end
// still spends the cooldown half of the counter - switching out of a copy buys the turns back, not
// an immediate second copy.
// No form change is re-applied: encformchange wrote MON_DATA_SPECIES on the party mon, so
// encuntransform rebuilds a Phase 2 Darkrai straight back into SPECIES_DARKRAI_MEGA. The moveset is
// the one thing untransform does clobber, because it rebuilds moves from the party mon too.
EncScript_Darkrai_Revert:
	encjumpifvar CMP_LESS_THAN, 9, 4, EncScript_Darkrai_Revert_Done
	encsetvar 9, 3
	encuntransform ENC_TARGET_BOSS
	call EncScript_Darkrai_ApplyMoveset
	printstring STRINGID_ENCDARKRAICOPYENDS
	waitmessage B_WAIT_TIME_LONG
EncScript_Darkrai_Revert_Done:
	return

// --- THE LIES ---

// Rolled once a turn by TurnClose, and never on a turn that already has a copy, a pending collapse
// or an open Shatter window on it. The Phase gates are what make the fight get less trustworthy as
// it goes: Phase 0 needs Nightmare 3, Phase 1 needs 2, Absolute Nightmare needs nothing.
EncScript_Darkrai_LieRoll:
	encjumpifvar CMP_GREATER_THAN, 11, 0, EncScript_Darkrai_LieRoll_Done   @ Shatter: the dark is off
	encjumpifvar CMP_GREATER_THAN, 9, 3, EncScript_Darkrai_LieRoll_Done    @ a copy is up
	encjumpifvar CMP_NOT_EQUAL, 13, 255, EncScript_Darkrai_LieRoll_Done    @ a collapse is pending
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Darkrai_LieRoll_Do
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Darkrai_LieRoll_Deepened
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Darkrai_LieRoll_Done
	goto EncScript_Darkrai_LieRoll_Do
EncScript_Darkrai_LieRoll_Deepened:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Darkrai_LieRoll_Done
EncScript_Darkrai_LieRoll_Do:
	encjumpifchance 40, EncScript_Darkrai_LieRoll_Done                     @ not every single turn
	encjumpifchance 25, EncScript_Darkrai_Lie_Collapse
	encjumpifchance 33, EncScript_Darkrai_Lie_Status
	encjumpifchance 50, EncScript_Darkrai_Lie_Boost
	goto EncScript_Darkrai_Lie_Vanish
EncScript_Darkrai_LieRoll_Done:
	return

// FALSE COLLAPSE - the one that is real, and the only lie with teeth. The bar really drops a quarter
// and TurnOpen really puts it back next turn. It can only punish a player who ACTS on it: a
// finisher, an X-item turn or a last Full Restore spent on a Darkrai that looks nearly dead.
// Two guards, both required. The 40% floor keeps a collapse from reading as a kill, and Survive:
// True means the drop can never actually take it down. The phase and catch-window triggers are all
// gated on HpMark == 255, so a threshold crossed on the false number cannot advance the fight -
// which is what lets this lie be told at all.
// encsnapshothp is both the test and the record: it writes the percentage this abort path then
// reads, and the same value TurnOpen rewinds to.
EncScript_Darkrai_Lie_Collapse:
	encsnapshothp ENC_TARGET_BOSS, 13, ENC_SNAP_SET
	encjumpifvar CMP_LESS_THAN, 13, 40, EncScript_Darkrai_Lie_Collapse_Abort
	call EncScript_Darkrai_Whisper
	enchangehp ENC_TARGET_BOSS, -10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCDARKRAICOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Darkrai_Lie_Collapse_Abort:
	encsetvar 13, 255
	goto EncScript_Darkrai_Lie_Status

// FALSE STATUS. The animation plays and the line prints; nothing is applied. The tell is that no
// status icon appears in the health box and no chip damage arrives at end of turn.
EncScript_Darkrai_Lie_Status:
	call EncScript_Darkrai_Whisper
	statusanimation BS_PLAYER1, STATUS1_TOXIC_POISON
	printstring STRINGID_ENCDARKRAIFALSESTATUS
	waitmessage B_WAIT_TIME_LONG
	return

// FALSE BOOST. Deliberately no B_ANIM_STATS_CHANGE - the stat-up animation the engine always plays
// is the tell, and it is missing.
EncScript_Darkrai_Lie_Boost:
	call EncScript_Darkrai_Whisper
	printstring STRINGID_ENCDARKRAIFALSEBOOST
	waitmessage B_WAIT_TIME_LONG
	return

// FALSE VANISH. No semi-invulnerability is set: Darkrai is still there, still targetable, and
// attacks normally next turn.
EncScript_Darkrai_Lie_Vanish:
	call EncScript_Darkrai_Whisper
	printstring STRINGID_ENCDARKRAIFALSEVANISH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_SCARED
	return

// --- Turn resolution helpers ---

// ANTI-ABUSE, and it is a price rather than an immunity. Every status the player lands costs a
// Nightmare level once, on application, and then works completely and permanently - Toxic included,
// which FlatToxicDamage: True keeps honest over a long fight. The trade is real and the player
// chooses it.
// Sleep is the exception because sleep is the one status that would take turns away from the boss.
// It lands, costs Darkrai the rest of that turn, and is scrubbed here for the same Nightmare level.
// curestatus takes an explicit battler: BS_TARGET/BS_ATTACKER are stale at OnTurnEnd.
EncScript_Darkrai_Statuses:
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Darkrai_Statuses_NoSleep
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Darkrai_Statuses_Feeds
	encsetvar 15, 0     @ BossStatus: nothing standing, so the latch re-arms
	goto EncScript_Darkrai_Statuses_Player
EncScript_Darkrai_Statuses_NoSleep:
	printstring STRINGID_ENCDARKRAINOSLEEP
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encsetvar 15, 0
	call EncScript_Darkrai_RaiseNightmare
	goto EncScript_Darkrai_Statuses_Player
EncScript_Darkrai_Statuses_Feeds:
	encjumpifvar CMP_EQUAL, 15, 1, EncScript_Darkrai_Statuses_Player
	encsetvar 15, 1
	printstring STRINGID_ENCDARKRAIFEEDS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Darkrai_RaiseNightmare
// A status STANDING on the player is the other half of the same idea, and it is why letting Hypnosis
// through is expensive twice over - Bad Dreams is already chipping. Charged once per application.
EncScript_Darkrai_Statuses_Player:
	jumpifstatus BS_PLAYER1, STATUS1_ANY, EncScript_Darkrai_Statuses_Held
	encsetvar 10, 0     @ PlayerStatus
	return
EncScript_Darkrai_Statuses_Held:
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Darkrai_Statuses_Done
	encsetvar 10, 1
	call EncScript_Darkrai_RaiseNightmare
EncScript_Darkrai_Statuses_Done:
	return

// The passive climb. Ticked at OnTurnStart and resolved here, two distinct checkpoints, so a tick
// can never eat a step off a clock re-armed in the same dispatch. A Clock of N+1 is "+1 every N
// turns". Stops mattering in Absolute Nightmare, where the dial is pinned.
EncScript_Darkrai_ClockTick:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Darkrai_ClockTick_Done
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Darkrai_ClockTick_Done
	call EncScript_Darkrai_RaiseNightmare
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Darkrai_ClockTick_Fast
	encsetvar 4, 4      @ Phase 0: one step every three turns
	return
EncScript_Darkrai_ClockTick_Fast:
	encsetvar 4, 3      @ Phase 1: every two
EncScript_Darkrai_ClockTick_Done:
	return

// The window closing. ApplyNightmare refuses to touch the guard while Shatter is up, so the way back
// to 96 is written here directly, with LastNm moved alongside it so the closing line IS the
// announcement and NM5 does not repeat itself a beat later.
EncScript_Darkrai_ShatterTick:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Darkrai_ShatterTick_Done
	encsubvar 11, 1
	encjumpifvar CMP_GREATER_THAN, 11, 0, EncScript_Darkrai_ShatterTick_Done
	printstring STRINGID_ENCDARKRAIWINDOWCLOSES
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 5      @ Nightmare
	encsetvar 2, 5      @ LastNm
	encsetdamagereduction ENC_TARGET_BOSS, 96
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Darkrai_ShatterTick_Done:
	return

// THE RECURRING CHANNEL (guideline 14). The level crossings announce a change; this is what a player
// who skipped past one eventually reads. Roughly every other turn, and only while the dark is deep
// enough to be doing something.
EncScript_Darkrai_Pressing:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Darkrai_Pressing_Done
	encjumpifvar CMP_GREATER_THAN, 11, 0, EncScript_Darkrai_Pressing_Done
	encjumpifchance 50, EncScript_Darkrai_Pressing_Done
	printstring STRINGID_ENCDARKRAIPRESSES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Darkrai_Pressing_Done:
	return

// --- Trigger scripts ---

// The fog is the ANIMATION only, not encsetweather. Real B_WEATHER_FOG multiplies every accuracy
// check on the field by 3/5 (battle_util.c), which would turn a long boss fight into a coin flip for
// both sides - and this encounter is already the one where the presentation is not the state.
// HpMark starts at its 255 "nothing pending" sentinel rather than the 0 gEncounterVars is zeroed to.
EncScript_Darkrai_Intro::
	printstring STRINGID_ENCDARKRAIINTRO1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOG_CONTINUES
	printstring STRINGID_ENCDARKRAIINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 4, 4      @ Clock
	encsetvar 13, 255   @ HpMark
	call EncScript_Darkrai_ApplyNightmare
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail, so no branch out of this script can leave it re-armed.
// The pending FALSE COLLAPSE is undone FIRST, before anything else this turn can read the health
// bar - and before the WAKE UP prompt at the next priority down.
EncScript_Darkrai_TurnOpen::
	flushtextbox
	encsetvar 5, 1      @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Darkrai_TurnOpen_Done   @ Weakened: inert
	encjumpifvar CMP_EQUAL, 13, 255, EncScript_Darkrai_TurnOpen_Copy
	encrewindhp ENC_TARGET_BOSS, 13
	encsetvar 13, 255
	printstring STRINGID_ENCDARKRAIREWIND
	waitmessage B_WAIT_TIME_LONG
EncScript_Darkrai_TurnOpen_Copy:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Darkrai_TurnOpen_Ask
	encjumpifvar CMP_EQUAL, 9, 4, EncScript_Darkrai_TurnOpen_Revert
	encsubvar 9, 1
	goto EncScript_Darkrai_TurnOpen_Ask
EncScript_Darkrai_TurnOpen_Revert:
	call EncScript_Darkrai_Revert
EncScript_Darkrai_TurnOpen_Ask:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Darkrai_TurnOpen_Clock
	encsubvar 12, 1     @ AskCd
EncScript_Darkrai_TurnOpen_Clock:
	encjumpifvar CMP_LESS_THAN, 4, 2, EncScript_Darkrai_TurnOpen_Done
	encsubvar 4, 1      @ Clock
EncScript_Darkrai_TurnOpen_Done:
	return

// WAKEFULNESS, the move source. The type clause lives in the trigger's conditions, including the one
// that matters most - Battler(Boss).Type == TYPE_DARK, which is what a NIGHTMARE COPY switches off.
EncScript_Darkrai_Wakefulness::
	call EncScript_Darkrai_RaiseWake
	return

// WAKEFULNESS, the switch source - and the same action that ends a copy early. Fresh eyes on the
// field are worth a step whatever came in, which is what keeps the fight winnable once the copy has
// closed the type route down.
EncScript_Darkrai_FreshEyes::
	call EncScript_Darkrai_RaiseWake
	call EncScript_Darkrai_Revert
	return

EncScript_Darkrai_TheyFell::
	encsetvar 8, 1      @ FaintGuard
	printstring STRINGID_ENCDARKRAITHEYFELL
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Darkrai_RaiseNightmare
	return

// "It has seen this before." Three entry points, one per category, each pinning its own literal
// because a condition can only compare a variable against a literal. Each sets CatGuard on entry: an
// Event.* condition stays true for the whole dispatch and no script can move it, so an
// event-reacting trigger must gate on a var it writes or the re-evaluation loop re-selects it until
// the checkpoint cap trips.
// LastCat is cleared when it fires, so the read costs a Nightmare level every second turn of leaning
// on one category rather than every turn.
EncScript_Darkrai_ReadPhysical::
	encsetvar 14, 1
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Darkrai_Read_Same
	encsetvar 7, 1
	return

EncScript_Darkrai_ReadSpecial::
	encsetvar 14, 1
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Darkrai_Read_Same
	encsetvar 7, 2
	return

EncScript_Darkrai_ReadStatus::
	encsetvar 14, 1
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Darkrai_Read_Same
	encsetvar 7, 3
	return

EncScript_Darkrai_Read_Same:
	encsetvar 7, 0
	printstring STRINGID_ENCDARKRAIREAD
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Darkrai_RaiseNightmare
	return

// THE SPEND. Three Wakefulness buys one Nightmare level back and clears whatever illusion is
// standing. Phase 0-1 only - in Absolute Nightmare the meter charges to 3 and offers WAKE UP
// instead.
EncScript_Darkrai_Waking::
	encsetvar 3, 0      @ Wake
	printstring STRINGID_ENCDARKRAIWAKESPEND
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_RESTORE_BG
	call EncScript_Darkrai_Revert
	call EncScript_Darkrai_LowerNightmare
	return

// NIGHTMARE COPIES. For three turns Darkrai IS the player's Pokemon - species, stats, stat stages,
// types, ability and moveset. Its HP bar keeps ticking from where it was, it is still the boss, and
// it still catches as Darkrai: enctransform writes battle data only.
// The interesting half is the one nothing states. Wakefulness's move source is conditioned on the
// boss still being Dark-typed, so the copy turns the player's own answer off and leaves switching as
// the only lever - which is also the thing that ends the copy early.
EncScript_Darkrai_Copy::
	call EncScript_Darkrai_Whisper
	enctransform ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, EncScript_Darkrai_Copy_Failed
	encsetvar 9, 7      @ Copy: three turns up, then three cooling down
	printstring STRINGID_ENCDARKRAICOPY
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Darkrai_Copy_Failed:
	@ A semi-invulnerable, already-transformed or Illusion'd source is an ordinary battle state, not
	@ an error, so it costs the cooldown and comes back around.
	encsetvar 9, 3
	return

// WAKE UP - the climax, and the whole fight's payoff in one prompt. Wakefulness is NOT spent on a
// NO; the prompt just goes quiet for three turns, so declining is a timing decision.
EncScript_Darkrai_WakeUpPrompt::
	encaskyesno STRINGID_ENCDARKRAIWAKEUP, EncScript_Darkrai_Shatter
	encsetvar 12, 3     @ AskCd
	printstring STRINGID_ENCDARKRAINOTYET
	waitmessage B_WAIT_TIME_LONG
	return

// The nightmare comes apart: illusions off, the copy dropped, the player's own sleep cured, and the
// guard at 45 for three turns against a Darkrai that has lost two Sp. Atk stages and this turn's
// action. encsetrecharge with 1 at OnTurnStart is what costs it the action in the turn already in
// progress - the timer is decremented after OnTurnEnd, so anywhere else would need 2.
// HpMark is not touched: TurnOpen runs one priority ahead of this and has already rewound anything
// pending, so a collapse can never be left un-rewound by the shatter.
EncScript_Darkrai_Shatter:
	encsetvar 3, 0      @ Wake
	encsetvar 12, 0     @ AskCd
	call EncScript_Darkrai_Revert
	printstring STRINGID_ENCDARKRAISHATTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	curestatus BS_PLAYER1
	updatestatusicon BS_PLAYER1
	encsetvar 11, 3     @ Shatter: written before the guard, so ApplyNightmare cannot take it back
	encsetvar 1, 0      @ Nightmare
	encsetvar 2, 0      @ LastNm: the shatter line is the announcement
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -2
	encsetrecharge ENC_TARGET_BOSS, 1
	return

// Runs last so it sees the finished turn. TurnGuard is cleared first - it is this trigger's own
// self-disabling condition - and the three event-reaction guards with it, so the next turn's
// OnMoveEnd/OnFaint reactions arrive re-armed.
EncScript_Darkrai_TurnClose::
	encsetvar 5, 0      @ TurnGuard
	encsetvar 6, 0      @ MoveGuard
	encsetvar 14, 0     @ CatGuard
	encsetvar 8, 0      @ FaintGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Darkrai_TurnClose_Done   @ Weakened: inert
	call EncScript_Darkrai_Statuses
	call EncScript_Darkrai_ClockTick
	call EncScript_Darkrai_ShatterTick
	call EncScript_Darkrai_LieRoll
	call EncScript_Darkrai_Pressing
EncScript_Darkrai_TurnClose_Done:
	return

// --- Phases ---

// 60% - THE NIGHTMARE DEEPENS. The clock shortens, the lies start one level earlier, and NASTY PLOT
// gives way to DARK VOID. The Nightmare step on entry is the point: the phase itself is a rung.
EncScript_Darkrai_Deepens::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCDARKRAIPHASE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Darkrai_ApplyMoveset
	encsetvar 4, 3      @ Clock
	call EncScript_Darkrai_RaiseNightmare
	return

// 30% - ABSOLUTE NIGHTMARE. Def/SpDef 90 -> 130 and SpAtk 135 -> 165 behind a guard pinned at 96 is
// the hardest point in the fight; base Speed 125 -> 85 is the compensation, and it is the one honest
// advantage the climax hands the player. It gets its own line.
// DARK VOID comes out because B_DARK_VOID_FAIL is GEN_LATEST and the engine's check is a literal
// `species != SPECIES_DARKRAI` - on the Mega it would be a dead slot. DREAM EATER takes it, and the
// engine scales what the boss keeps from a drain by the authored DamageReduction:.
// No trapping: switching is a Wakefulness source and Wakefulness is the win condition.
EncScript_Darkrai_Absolute::
	encsetvar 0, 2      @ Phase
	call EncScript_Darkrai_Revert            @ a copy cannot survive the form change
	printstring STRINGID_ENCDARKRAIPHASE2
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_DARKRAI_MEGA, EncScript_Darkrai_Absolute_Body, B_ANIM_MEGA_EVOLUTION
EncScript_Darkrai_Absolute_Body:
	call EncScript_Darkrai_ApplyMoveset
	printstring STRINGID_ENCDARKRAIHEAVY
	waitmessage B_WAIT_TIME_LONG
	encsetvar 11, 0     @ Shatter
	encsetvar 1, 5      @ Nightmare, pinned from here on
	call EncScript_Darkrai_ApplyNightmare
	return

// 10% - the catch window, and everything false in the battle coming down at once. Phase 3 retires
// every remaining trigger; all of them are gated Phase <= 2, and both turn scaffolds return early.
// From here the engine's automatic catch-window damage guard owns the reduction - the 99 below is
// setting the value that guard will restore, not the live one. The status is cured because a Toxic
// standing on a boss that no longer has Survive: True is the one thing that could still kill the
// catch target.
EncScript_Darkrai_Weakened::
	encsetvar 0, 3      @ Phase
	call EncScript_Darkrai_Revert
	@ Back out of the Mega explicitly rather than leaning on FORM_CHANGE_END_BATTLE: encformchange
	@ wrote SPECIES_DARKRAI_MEGA onto the party mon, and this is the point the fight says out loud
	@ that there is nothing holding the shape up any more.
	encformchange ENC_TARGET_BOSS, SPECIES_DARKRAI, EncScript_Darkrai_Weakened_Body, B_ANIM_FORM_CHANGE
EncScript_Darkrai_Weakened_Body:
	encsetvar 9, 0      @ Copy
	encsetvar 11, 0     @ Shatter
	encsetvar 13, 255   @ HpMark
	encsetvar 1, 0      @ Nightmare
	encsetvar 2, 0      @ LastNm
	encsetvar 3, 0      @ Wake
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	printstring STRINGID_ENCDARKRAIWEAKENED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return
