// Celebi, "The Guardian of Time". Var indices, pinned by
// the always-true Conditions on EncScript_Celebi_Intro:
// 0 Phase (0 Guardian of Time / 1 Future Sight / 2 Temporal Collapse / 3 Weakened),
// 1 Echo (recorded HP percentage - the rewind target; 101 means nothing recorded yet),
// 2 Cycle (0 idle, else 3->2->1 resolved at 1, 4 = spent sentinel), 3 Window (0 closed, 1 open,
// 2 anchored this turn), 4 Anchored (0-5), 5 Pred (0 clouded / 1 physical / 2 special / 3 status /
// 4 consumed / 5 unread), 6 Broken (turns of open guard left), 7 LastGuard (reduction last
// ANNOUNCED - the re-fire latch), 8 TurnGuard, 9 PEcho (player HP percentage at record time),
// 10 Delta (scratch: player recovery since PEcho), 11 Collapsed, 12 Prev (ApplyGuard's before-value
// scratch), 13 Breach (which opening is live: 1 broken future, 2 failed collapse).
//
// Everything hangs off Echo. A record window opens, Celebi snapshots its HP, and two turns later
// TIME REWIND snaps its HP back to that number - so damage is not permanent until it is recorded.
// The snapshot is monotone downward, and re-taken whenever the player lands a hit during the window
// (an ANCHOR), which is what makes "when does my damage count?" a question the player controls
// rather than one the script answers. Phase 1 surfaces the AI's real move prediction and pays the
// player for defying it; phase 2 shortens the cycle and ends in TIME COLLAPSE, which the anchors
// banked earlier are what survive.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. Prev holds the value last announced so each
// leaf can write LastGuard before comparing; there is no UI for damage reduction, so a single
// showing would be trivially missed and the line has to re-fire on every real change.
EncScript_Celebi_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Celebi_ApplyGuard_Done   @ weakened owns its own guard
	enccopyvar 12, 7
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Celebi_Guard_Open
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Celebi_Guard92
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Celebi_Guard88
	goto EncScript_Celebi_Guard84
// A failed TIME COLLAPSE opens Celebi further than a broken prediction does - it is the payoff for
// a whole fight's worth of anchoring, not a single good read.
EncScript_Celebi_Guard_Open:
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Celebi_Guard72
	goto EncScript_Celebi_Guard78

EncScript_Celebi_Guard72:
	encsetdamagereduction ENC_TARGET_BOSS, 72
	encsetvar 7, 72
	encjumpifvar CMP_EQUAL, 12, 72, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 72, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard78:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 7, 78
	encjumpifvar CMP_EQUAL, 12, 78, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 78, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 7, 84
	encjumpifvar CMP_EQUAL, 12, 84, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 84, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 7, 88
	encjumpifvar CMP_EQUAL, 12, 88, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 88, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin
EncScript_Celebi_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 7, 92
	encjumpifvar CMP_EQUAL, 12, 92, EncScript_Celebi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 12, 92, EncScript_Celebi_ApplyGuard_Thicken
	goto EncScript_Celebi_ApplyGuard_Thin

EncScript_Celebi_ApplyGuard_Thicken:
	printstring STRINGID_ENCCELEBITHICKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Celebi_ApplyGuard_Thin:
	printstring STRINGID_ENCCELEBITHINS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Celebi_ApplyGuard_Done:
	return

// Opens a record window. ENC_SNAP_LOWEST is what makes Echo a monotone floor: the mark only ever
// moves down, so damage dealt while the window is open is damage the rewind can never take back.
// PEcho is the player's own HP at the same instant, which is the reference Paradox measures
// recovery against.
EncScript_Celebi_Record:
	encsnapshothp ENC_TARGET_BOSS, 1, ENC_SNAP_LOWEST
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_SET
	encsetvar 3, 1   @ Window open
	printstring STRINGID_ENCCELEBIRECORDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	return

// TIME REWIND. Celebi's HP is moved back TO the recorded percentage - which heals it in the usual
// case and cuts it back down if it has climbed above the mark since. Stat stages revert on both
// sides and any status inflicted since the echo is gone with it: the restore is the whole battle
// state Celebi can reach, not just its own health bar.
EncScript_Celebi_DoRewind:
	printstring STRINGID_ENCCELEBIREWIND
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encrewindhp ENC_TARGET_BOSS, 1
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	normalisebuffs
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCCELEBIRESTORED
	waitmessage B_WAIT_TIME_SHORT
	return

// TEMPORAL PARADOX - the phase-1+ alternative that REPLACES a rewind rather than adding to it. The
// player's damage survives, but whatever they did during the cycle comes back around. Three
// branches in priority order: a recovery Celebi copies, an ascent Celebi copies, or the blow
// returning out of the past. Re-running the player's actual move isn't reachable from a script
// (that is an EFFECT_INSTRUCT problem), so the outcome is what repeats.
EncScript_Celebi_DoParadox:
	printstring STRINGID_ENCCELEBIPARADOX
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	@ Delta = how much the player has healed since the echo. ENC_SNAP_RECOVERY does the var-to-var
	@ subtraction a script can't express, and jumps out when there was none.
	enccopyvar 10, 9
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 10, ENC_SNAP_RECOVERY, EncScript_Celebi_DoParadox_Stats
	encjumpifvar CMP_LESS_THAN, 10, 5, EncScript_Celebi_DoParadox_Stats
	enchangehp ENC_TARGET_BOSS, 10, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	printstring STRINGID_ENCCELEBIPARADOXHEAL
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Celebi_DoParadox_Stats:
	jumpifstat BS_PLAYER1, CMP_GREATER_THAN, STAT_ATK, DEFAULT_STAT_STAGE, EncScript_Celebi_DoParadox_Copy
	jumpifstat BS_PLAYER1, CMP_GREATER_THAN, STAT_SPATK, DEFAULT_STAT_STAGE, EncScript_Celebi_DoParadox_Copy
	jumpifstat BS_PLAYER1, CMP_GREATER_THAN, STAT_SPEED, DEFAULT_STAT_STAGE, EncScript_Celebi_DoParadox_Copy
	goto EncScript_Celebi_DoParadox_Blow
EncScript_Celebi_DoParadox_Copy:
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCCELEBIPARADOXSTAT
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Celebi_DoParadox_Blow:
	printstring STRINGID_ENCCELEBIPARADOXBLOW
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return

// Temporal Collapse sheds status on its own - locking Celebi down is temporary by construction
// anyway, since every rewind cures it. Silent no-op when it has none.
EncScript_Celebi_Shed:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Celebi_Shed_Cure
	return
EncScript_Celebi_Shed_Cure:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCCELEBISHEDS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// --- Trigger scripts ---

// Echo starts at 101 rather than 0 on purpose: ENC_SNAP_LOWEST only writes a value lower than what
// the var holds, so the first snapshot has to find something above any real percentage. LastGuard
// is seeded to the Properties reduction so the first real change reads as a change. The window
// itself is left to turn 1's OpenWindow, so the player can act inside the first record turn.
EncScript_Celebi_Intro::
	encsetvar 1, 101
	encsetvar 7, 86
	encsetvar 5, 5    @ Pred: unread
	printstring STRINGID_ENCCELEBIINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCCELEBIWATCHES
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn
// scratch, re-arms the prediction, and keeps the open-window callout running.
EncScript_Celebi_TurnOpen::
	flushtextbox
	encsetvar 8, 1   @ TurnGuard
	encsetvar 3, 0   @ Window closed
	encjumpifvar CMP_NOT_EQUAL, 2, 4, EncScript_Celebi_TurnOpen_Phase
	encsetvar 2, 0   @ a spent cycle clears here, so a rewind turn and a record turn are never one turn
EncScript_Celebi_TurnOpen_Phase:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Celebi_TurnOpen_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Celebi_TurnOpen_Broken
	encsetvar 5, 5   @ Pred: unread, so Foresee runs this turn
EncScript_Celebi_TurnOpen_Broken:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Celebi_TurnOpen_Shed
	printstring STRINGID_ENCCELEBIUNSETTLED
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Celebi_TurnOpen_Done
EncScript_Celebi_TurnOpen_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Celebi_TurnOpen_Done
	call EncScript_Celebi_Shed
EncScript_Celebi_TurnOpen_Done:
	return

// The rewind resolves the countdown TurnClose stepped, one checkpoint later, so a real turn always
// passes between a tick and the snap-back. It resolves to the spent sentinel 4 first: every trigger
// is re-evaluated after each script, so writing 0 here would re-arm OpenWindow in this same
// dispatch and record the post-rewind HP as a brand new window.
EncScript_Celebi_Rewind::
	flushtextbox
	encsetvar 2, 4
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Celebi_Rewind_Plain
	encjumpifchance 35, EncScript_Celebi_Rewind_Paradox
EncScript_Celebi_Rewind_Plain:
	call EncScript_Celebi_DoRewind
	return
EncScript_Celebi_Rewind_Paradox:
	call EncScript_Celebi_DoParadox
	return

// Behind the rewind at priority 30, so on a turn that does both the snapshot sees the post-rewind
// value rather than the damage the rewind is about to erase.
EncScript_Celebi_OpenWindow::
	flushtextbox
	call EncScript_Celebi_Record
	encsetvar 2, 3
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Celebi_OpenWindow_Done
	encsetvar 2, 2   @ Temporal Collapse: the cycle comes round faster
EncScript_Celebi_OpenWindow_Done:
	return

// The prediction, surfaced. encstoreprediction reads the AI's real AI_FLAG_PREDICT_MOVE answer for
// the player, which is computed during action selection - so this is a genuine read-ahead, not a
// scripted fake, and the player can invalidate it by choosing differently. Writing Pred is also
// this trigger's own self-disabling gate.
EncScript_Celebi_Foresee::
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 5
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Celebi_Foresee_Phys
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Celebi_Foresee_Spec
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Celebi_Foresee_Status
	printstring STRINGID_ENCCELEBIFORESEENONE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Celebi_Foresee_Phys:
	printstring STRINGID_ENCCELEBIFORESEEPHYS
	goto EncScript_Celebi_Foresee_Show
EncScript_Celebi_Foresee_Spec:
	printstring STRINGID_ENCCELEBIFORESEESPEC
	goto EncScript_Celebi_Foresee_Show
EncScript_Celebi_Foresee_Status:
	printstring STRINGID_ENCCELEBIFORESEESTAT
EncScript_Celebi_Foresee_Show:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	return

// Damaging Celebi while the window is open re-takes the mark at the lower value, which is what
// permanently banks the damage. encsnapshothp's failInstr does the whole test: if the mark didn't
// move, the hit did nothing worth recording (a miss, an immunity, a status move) and no anchor is
// claimed. Window 2 is the consumed value its own trigger condition rejects - one attempt per turn,
// as an event condition stays true for the whole dispatch.
EncScript_Celebi_Anchor::
	encsetvar 3, 2
	encsnapshothp ENC_TARGET_BOSS, 1, ENC_SNAP_LOWEST, EncScript_Celebi_Anchor_Done
	encjumpifvar CMP_GREATER_THAN, 4, 4, EncScript_Celebi_Anchor_Show
	encaddvar 4, 1
EncScript_Celebi_Anchor_Show:
	printstring STRINGID_ENCCELEBIANCHOR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
EncScript_Celebi_Anchor_Done:
	return

// Shared by the three matched-category triggers. The foreseen future arrives sooner as well as
// hitting: a correct read pulls the next rewind a turn closer.
EncScript_Celebi_PredHeld::
	encsetvar 5, 4   @ Pred consumed
	printstring STRINGID_ENCCELEBIPREDHELD
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Celebi_PredHeld_Done
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Celebi_PredHeld_Done   @ the spent sentinel isn't a countdown
	encsubvar 2, 1
EncScript_Celebi_PredHeld_Done:
	return

// The catch-all at priority 30: reached only when none of the three matched triggers claimed the
// dispatch, i.e. the player played a category Celebi did not foresee. The pending rewind is
// cancelled outright and the guard falls for two turns - this is what makes reading the prediction
// worth doing rather than an announcement to ignore.
EncScript_Celebi_PredBroken::
	encsetvar 5, 4   @ Pred consumed
	printstring STRINGID_ENCCELEBIPREDBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 6, 2    @ Broken: two turns of open guard
	encsetvar 13, 1   @ Breach: broken future
	encsetvar 2, 4    @ the pending rewind is cancelled
	call EncScript_Celebi_ApplyGuard
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown in one place, and the phase-2 end-of-turn shed.
EncScript_Celebi_TurnClose::
	encsetvar 8, 0   @ TurnGuard
	encsetvar 3, 0   @ Window closed
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Celebi_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Celebi_TurnClose_Broken
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Celebi_TurnClose_Broken   @ 4 is the spent sentinel
	encsubvar 2, 1
EncScript_Celebi_TurnClose_Broken:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Celebi_TurnClose_Shed
	encsubvar 6, 1
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Celebi_TurnClose_Done
	encsetvar 13, 0   @ Breach cleared: ApplyGuard re-announces the guard closing again
	call EncScript_Celebi_ApplyGuard
	goto EncScript_Celebi_TurnClose_Done
EncScript_Celebi_TurnClose_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Celebi_TurnClose_Done
	call EncScript_Celebi_Shed
EncScript_Celebi_TurnClose_Done:
	return

// Phase 1 at 60% HP. Celebi stops reacting and starts reading ahead: from here every turn opens
// with a stated prediction, and the fight gains a second layer the player can play against.
EncScript_Celebi_FutureSight::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCCELEBIFUTURESIGHT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 5, 5   @ Pred: unread, so the foresight starts next turn
	call EncScript_Celebi_ApplyGuard
	return

// Phase 2 at 25% HP. The cycle shortens, the past bleeds back onto the field as a weather that is
// no longer falling anywhere else, and Survive guarantees the TIME COLLAPSE beat is reachable.
EncScript_Celebi_Collapse::
	encsetvar 0, 2   @ Phase 2
	printstring STRINGID_ENCCELEBICOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetweather BATTLE_WEATHER_RAIN
	playanimation BS_PLAYER1, B_ANIM_RAIN_CONTINUES
	printstring STRINGID_ENCCELEBIWEATHER
	waitmessage B_WAIT_TIME_LONG
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encjumpifvar CMP_NOT_EQUAL, 2, 3, EncScript_Celebi_Collapse_Guard
	encsetvar 2, 2   @ shorten a cycle already in flight
EncScript_Celebi_Collapse_Guard:
	call EncScript_Celebi_ApplyGuard
	return

// TIME COLLAPSE at 13% HP - one final rewind, and the anchors banked all fight are what decides how
// much of it lands. Collapsed is set in every branch, which is what unlocks the weakened trigger:
// the finale always plays before the catch window opens, even when one hit crosses both thresholds.
EncScript_Celebi_TimeCollapse::
	encsetvar 11, 1   @ Collapsed
	printstring STRINGID_ENCCELEBITIMECOLLAPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	encsetvar 2, 4    @ nothing else resolves this turn
	encjumpifvar CMP_GREATER_THAN, 4, 2, EncScript_Celebi_TimeCollapse_Held
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Celebi_TimeCollapse_Partial
	printstring STRINGID_ENCCELEBICOLLAPSEFULL
	waitmessage B_WAIT_TIME_LONG
	encrewindhp ENC_TARGET_BOSS, 1
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	normalisebuffs
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	enchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return
EncScript_Celebi_TimeCollapse_Partial:
	printstring STRINGID_ENCCELEBICOLLAPSEPART
	waitmessage B_WAIT_TIME_LONG
	encrewindhp ENC_TARGET_BOSS, 1
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	normalisebuffs
	encsetvar 6, 1    @ Broken: one turn of open guard
	encsetvar 13, 1
	call EncScript_Celebi_ApplyGuard
	return
// Three or more anchors and the collapse finds nothing to reach past. No heal, and Celebi is left
// wide open for three turns - the mechanic it introduced on turn one is what beats it.
EncScript_Celebi_TimeCollapse_Held:
	printstring STRINGID_ENCCELEBICOLLAPSEHELD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encsetvar 6, 3    @ Broken: three turns of open guard
	encsetvar 13, 2   @ Breach: the collapse failed
	call EncScript_Celebi_ApplyGuard
	return

// Below 10% once the collapse has played. Everything off, and Survive released so the catch window
// is a real one. CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the window.
EncScript_Celebi_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 2, 0   @ Cycle
	encsetvar 3, 0   @ Window
	encsetvar 5, 5   @ Pred
	encsetvar 6, 0   @ Broken
	encsetvar 13, 0  @ Breach
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	removeweather
	encsetdamagereduction ENC_TARGET_BOSS, 75
	encsetvar 7, 75
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCCELEBIWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
