// Tapu Koko, "The Land Spirit Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_TapuKoko_Intro:
// 0 Phase (0 Lightning Guardian / 1 Guardian State / 2 Island Guardian / 3 Exhausted), 1 Voltage
// (0-5), 2 TurnGuard, 3 LastBand (last ANNOUNCED Voltage value), 4 Charged, 5 ChargeTimer
// (tick/resolve idiom - resolves at 1, not 0), 6 SpeedCmp (scratch), 7 ChallengeTimer (same
// tick/resolve idiom, shared by both Challenge kinds), 8 ChallengeGuard, 9 ThunderTimer (same
// idiom), 10 GigaCharge (0 none / 1 charging / 9 resolved), 11 Exhausted, 12 ChallengeSlot
// (scratch, consumed immediately by encforceswitch), 13 MoveGuard (per-turn OnMoveEnd latch),
// 14 ThunderGuard, 15 SwitchGuard (per-turn OnSwitchIn latch).
//
// VOLTAGE IS THE GUARD, IN BOTH DIRECTIONS. TapuKoko_VoltageUp/Down are the only writers, so
// nothing needs to remember what it added before undoing it. TapuKoko_AnnounceVoltage prints only
// on a real change (guideline 14), in both directions - a fall is the player's own work and the
// fight has no other way to confirm it did anything.
//
// THE TICK/RESOLVE IDIOM (ChargeTimer/ChallengeTimer/ThunderTimer): tick at OnTurnStart, resolve
// at OnTurnEnd when the value reads 1 - not 0. Reshiram's Judgment charge uses the same off-by-one:
// a timer set to N and decremented once a turn resolves after N-1 further turns, which is what
// lets tick and resolve live at different checkpoints without one chasing the other through every
// state in a single dispatch (the split-countdown rule).
//
// MOVEGUARD is why ElectricApproval/ChallengePunish don't gate on the meter they themselves move:
// Voltage <= 4 (or ChallengeTimer >= 1) can still be true for another re-evaluation pass right
// after a +1, and an Event.* condition can't self-disable through OnEnter (its edge tracks against
// the checkpoint's own history, not the event) - so it needs a dedicated latch, cleared at
// TurnOpen, the same fix authoring.md prescribes for any event-reacting trigger.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// Voltage +1, capped at 5. Silent; every caller supplies its own line via TapuKoko_AnnounceVoltage.
TapuKoko_VoltageUp:
	encjumpifvar CMP_EQUAL, 1, 5, TapuKoko_VoltageUp_Done
	encaddvar 1, 1
TapuKoko_VoltageUp_Done:
	return

// Voltage -1, floored at 0. Silent.
TapuKoko_VoltageDown:
	encjumpifvar CMP_EQUAL, 1, 0, TapuKoko_VoltageDown_Done
	encsubvar 1, 1
TapuKoko_VoltageDown_Done:
	return

// THE VOLTAGE CALLOUT (guideline 14). LastBand holds the last ANNOUNCED value; each arm compares
// it against its own literal and stays quiet if that is what was last said - conditions and
// encjumpifvar only ever compare a var against a literal, so a rule that weighs one var against
// another needs to be expressed as a chain of literal-vs-literal branches instead (the "no
// var-to-var comparison" gotcha). Voltage 0 can only ever be a fall into the floor and Voltage 5
// only ever a rise into the ceiling, so those two arms skip the direction test entirely.
TapuKoko_AnnounceVoltage:
	encjumpifvar CMP_EQUAL, 1, 0, TapuKoko_AnnounceVoltage_V0
	encjumpifvar CMP_EQUAL, 1, 1, TapuKoko_AnnounceVoltage_V1
	encjumpifvar CMP_EQUAL, 1, 2, TapuKoko_AnnounceVoltage_V2
	encjumpifvar CMP_EQUAL, 1, 3, TapuKoko_AnnounceVoltage_V3
	encjumpifvar CMP_EQUAL, 1, 4, TapuKoko_AnnounceVoltage_V4
	goto TapuKoko_AnnounceVoltage_V5
TapuKoko_AnnounceVoltage_V0:
	encjumpifvar CMP_EQUAL, 3, 0, TapuKoko_AnnounceVoltage_Done
	encsetvar 3, 0
	printstring STRINGID_ENCTAPUKOKOVOLTAGEFALLING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V1:
	encjumpifvar CMP_EQUAL, 3, 1, TapuKoko_AnnounceVoltage_Done
	encjumpifvar CMP_LESS_THAN, 3, 1, TapuKoko_AnnounceVoltage_V1_Rising
	encsetvar 3, 1
	printstring STRINGID_ENCTAPUKOKOVOLTAGEFALLING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V1_Rising:
	encsetvar 3, 1
	printstring STRINGID_ENCTAPUKOKOVOLTAGERISING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V2:
	encjumpifvar CMP_EQUAL, 3, 2, TapuKoko_AnnounceVoltage_Done
	encjumpifvar CMP_LESS_THAN, 3, 2, TapuKoko_AnnounceVoltage_V2_Rising
	encsetvar 3, 2
	printstring STRINGID_ENCTAPUKOKOVOLTAGEFALLING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V2_Rising:
	encsetvar 3, 2
	printstring STRINGID_ENCTAPUKOKOVOLTAGERISING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V3:
	encjumpifvar CMP_EQUAL, 3, 3, TapuKoko_AnnounceVoltage_Done
	encjumpifvar CMP_LESS_THAN, 3, 3, TapuKoko_AnnounceVoltage_V3_Rising
	encsetvar 3, 3
	printstring STRINGID_ENCTAPUKOKOVOLTAGEFALLING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V3_Rising:
	encsetvar 3, 3
	printstring STRINGID_ENCTAPUKOKOVOLTAGERISING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V4:
	encjumpifvar CMP_EQUAL, 3, 4, TapuKoko_AnnounceVoltage_Done
	encjumpifvar CMP_LESS_THAN, 3, 4, TapuKoko_AnnounceVoltage_V4_Rising
	encsetvar 3, 4
	printstring STRINGID_ENCTAPUKOKOVOLTAGEFALLING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V4_Rising:
	encsetvar 3, 4
	printstring STRINGID_ENCTAPUKOKOVOLTAGERISING
	waitmessage B_WAIT_TIME_SHORT
	return
TapuKoko_AnnounceVoltage_V5:
	encjumpifvar CMP_EQUAL, 3, 5, TapuKoko_AnnounceVoltage_Done
	encsetvar 3, 5
	printstring STRINGID_ENCTAPUKOKOVOLTAGERISING
	waitmessage B_WAIT_TIME_SHORT
TapuKoko_AnnounceVoltage_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. Electric Terrain is already up from Electric Surge by the time this prints - no
// trainerslidein, this is a wild Pokemon (guideline 13).
EncScript_TapuKoko_Intro::
	printstring STRINGID_ENCTAPUKOKOINTRO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// 50% - GUARDIAN STATE. Voltage is forced to at least 3 (never lowered if already higher).
EncScript_TapuKoko_GuardianState::
	printstring STRINGID_ENCTAPUKOKOGUARDIANSTATE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 1     @ Phase
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_TapuKoko_GuardianState_Done
	encsetvar 1, 3     @ Voltage floored at 3
EncScript_TapuKoko_GuardianState_Done:
	call TapuKoko_AnnounceVoltage
	return

// 20% - ISLAND GUARDIAN. Voltage locked to 5, Terrain forced permanent (redundant with the ability
// but survives a Terrain-swap move), Speed pushed to +2 written absolutely (push to the ceiling,
// then land the delta that reaches +2 - the Rayquaza idiom, immune to a Haze landing in between).
EncScript_TapuKoko_IslandGuardian::
	printstring STRINGID_ENCTAPUKOKOISLANDGUARDIAN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 2     @ Phase
	encsetvar 1, 5     @ Voltage
	encsetterrain ENC_TERRAIN_ELECTRIC
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -4
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call TapuKoko_AnnounceVoltage
	return

// 10% - GIGAVOLT GUARDIAN begins gathering. TurnOpen flips GigaCharge 1->9 on the next turn
// boundary, so a full turn always separates this telegraph from Exhausted firing.
EncScript_TapuKoko_GigavoltBegins::
	printstring STRINGID_ENCTAPUKOKOGIGAVOLTBEGINS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encsetvar 10, 1    @ GigaCharge
	return

// Top of every turn. flushtextbox - an OnTurnStart script that animates before any dialogue
// renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set immediately so
// no branch out can leave it re-armed.
EncScript_TapuKoko_TurnOpen::
	flushtextbox
	encsetvar 2, 1     @ TurnGuard
	encsetvar 13, 0    @ MoveGuard - fresh for this turn's OnMoveEnd dispatches
	encsetvar 15, 0    @ SwitchGuard - fresh for this turn's OnSwitchIn dispatches
	enccomparestat ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 6   @ SpeedCmp
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_TapuKoko_TurnOpen_Thunder
	encsubvar 5, 1     @ ChargeTimer
EncScript_TapuKoko_TurnOpen_Thunder:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_TapuKoko_TurnOpen_ChallengeTick
	encsubvar 9, 1     @ ThunderTimer
EncScript_TapuKoko_TurnOpen_ChallengeTick:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_TapuKoko_TurnOpen_Guards
	encsubvar 7, 1     @ ChallengeTimer
EncScript_TapuKoko_TurnOpen_Guards:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_TapuKoko_TurnOpen_ThunderGuard
	encsubvar 8, 1     @ ChallengeGuard
EncScript_TapuKoko_TurnOpen_ThunderGuard:
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_TapuKoko_TurnOpen_Giga
	encsubvar 14, 1    @ ThunderGuard
EncScript_TapuKoko_TurnOpen_Giga:
	encjumpifvar CMP_NOT_EQUAL, 10, 1, EncScript_TapuKoko_TurnOpen_Done
	encsetvar 10, 9    @ Gigavolt Guardian resolves this turn's close
EncScript_TapuKoko_TurnOpen_Done:
	return

// Electric Terrain's own upkeep feeds Voltage - the literal "Terrain is active" beat.
EncScript_TapuKoko_TerrainGain::
	call TapuKoko_VoltageUp
	call TapuKoko_AnnounceVoltage
	return

// THUNDER CALL telegraph. A switch clears ThunderTimer outright (EncScript_TapuKoko_ThunderDodged,
// OnSwitchIn) rather than retargeting the strike - the singles-only "reposition" substitution.
EncScript_TapuKoko_ThunderArm::
	printstring STRINGID_ENCTAPUKOKOTHUNDERWARN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	encsetvar 9, 2     @ ThunderTimer
	encsetvar 14, 5    @ ThunderGuard
	return

// Speed Challenge (Phase 0) or Guardian's Challenge (Phase 1), branching on Phase.
EncScript_TapuKoko_OpenChallenge::
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_TapuKoko_OpenChallenge_Guardian
	printstring STRINGID_ENCTAPUKOKOCHALLENGEOPEN
	waitmessage B_WAIT_TIME_LONG
	encpartybest ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 12    @ ChallengeSlot
	encforceswitch ENC_TARGET_PLAYER_LEFT, 12, EncScript_TapuKoko_OpenChallenge_NoBench
	encsetvar 7, 3     @ ChallengeTimer
	call TapuKoko_VoltageUp
	call TapuKoko_AnnounceVoltage
	encsetvar 8, 4     @ ChallengeGuard
	return
EncScript_TapuKoko_OpenChallenge_NoBench:
	encsetvar 8, 1     @ nobody to call out; try again next turn
	return
EncScript_TapuKoko_OpenChallenge_Guardian:
	printstring STRINGID_ENCTAPUKOKOCHALLENGEOPEN
	waitmessage B_WAIT_TIME_LONG
	encsetvar 7, 3     @ ChallengeTimer, against whoever is already active
	call TapuKoko_VoltageUp
	call TapuKoko_AnnounceVoltage
	encsetvar 8, 4     @ ChallengeGuard
	return

// Koko attacked while wound up - lay an Electric Charge.
EncScript_TapuKoko_MarkCharge::
	encsetvar 4, 1     @ Charged
	encsetvar 5, 2     @ ChargeTimer
	printstring STRINGID_ENCTAPUKOKOCHARGEMARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	return

// The player used a Ground move while charged - the charge grounds out. A real answer, not a
// wall (guideline 8): it costs a moveslot and matches the type matchup the idea specified.
EncScript_TapuKoko_ClearCharge::
	encsetvar 4, 0     @ Charged
	encsetvar 5, 0     @ ChargeTimer
	printstring STRINGID_ENCTAPUKOKOCHARGECLEAR
	waitmessage B_WAIT_TIME_SHORT
	return

// The player used an Electric move - Koko approves. MoveGuard is what stops this from stacking
// more than once within a single OnMoveEnd dispatch (see the header comment).
EncScript_TapuKoko_ElectricApproval::
	encsetvar 13, 1    @ MoveGuard
	call TapuKoko_VoltageUp
	call TapuKoko_AnnounceVoltage
	return

// Guardian's Challenge status-move punish: a soft punish rather than a hard block (guideline 8's
// Stage 2 substitution) - status still lands, it just costs Voltage and 5% chip.
EncScript_TapuKoko_ChallengePunish::
	encsetvar 13, 1    @ MoveGuard
	call TapuKoko_VoltageUp
	printstring STRINGID_ENCTAPUKOKOCHALLENGEPUNISH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
	call TapuKoko_AnnounceVoltage
	return

// The Challenge target went down - Koko approves and the Challenge closes.
EncScript_TapuKoko_ChallengeFalls::
	call TapuKoko_VoltageUp
	encsetvar 7, 0     @ ChallengeTimer
	printstring STRINGID_ENCTAPUKOKOCHALLENGEFALLS
	waitmessage B_WAIT_TIME_LONG
	call TapuKoko_AnnounceVoltage
	return

// The Electric Charge follows whoever just walked in. SwitchGuard is what stops this from
// re-selecting every re-evaluation pass of this one switch-in (see the header comment).
EncScript_TapuKoko_ChargeFollows::
	encsetvar 15, 1    @ SwitchGuard
	printstring STRINGID_ENCTAPUKOKOCHARGEMARK
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	return

// A switch during a Thunder Call telegraph throws its aim off outright - the singles-only
// "reposition" substitution (guideline 8's Stage 2 note).
EncScript_TapuKoko_ThunderDodged::
	encsetvar 9, 0     @ ThunderTimer
	printstring STRINGID_ENCTAPUKOKOTHUNDERDODGED
	waitmessage B_WAIT_TIME_SHORT
	return

// THE LEDGER. Measures nothing itself - every meter here already moved at its own checkpoint -
// and only resolves the timers the tick/resolve idiom split across turns. Self-disables by
// clearing TurnGuard.
EncScript_TapuKoko_TurnClose::
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_TapuKoko_TurnClose_Thunder
	encjumpifvar CMP_NOT_EQUAL, 5, 1, EncScript_TapuKoko_TurnClose_Thunder
	printstring STRINGID_ENCTAPUKOKOCHARGESTRIKE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encsetvar 4, 0     @ Charged
	encsetvar 5, 0     @ ChargeTimer
EncScript_TapuKoko_TurnClose_Thunder:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_TapuKoko_TurnClose_Challenge   @ Phase 1 only
	encjumpifvar CMP_NOT_EQUAL, 9, 1, EncScript_TapuKoko_TurnClose_Challenge
	printstring STRINGID_ENCTAPUKOKOTHUNDERSTRIKE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	encsetvar 9, 0     @ ThunderTimer
EncScript_TapuKoko_TurnClose_Challenge:
	encjumpifvar CMP_NOT_EQUAL, 7, 1, EncScript_TapuKoko_TurnClose_Storm
	call TapuKoko_VoltageDown
	encsetvar 7, 0     @ ChallengeTimer
	printstring STRINGID_ENCTAPUKOKOCHALLENGEENDURES
	waitmessage B_WAIT_TIME_LONG
	call TapuKoko_AnnounceVoltage
EncScript_TapuKoko_TurnClose_Storm:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_TapuKoko_TurnClose_Done   @ Phase 2 only
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_TapuKoko_TurnClose_Done   @ already charged this turn
	encsetvar 4, 1     @ Charged
	encsetvar 5, 2     @ ChargeTimer
	printstring STRINGID_ENCTAPUKOKOCHARGEMARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
EncScript_TapuKoko_TurnClose_Done:
	encsetvar 2, 0     @ TurnGuard
	return

// GIGAVOLT GUARDIAN has resolved, and the catch window (guideline 10). The teardown mirrors the
// Properties: block plus everything the fight raised, so nothing scripted survives into the
// window. encsetdamagereduction 99 writes the value the automatic catch guard will restore, not
// the live one.
EncScript_TapuKoko_Exhausted::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCTAPUKOKOEXHAUSTED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Voltage
	encsetvar 4, 0     @ Charged
	encsetvar 5, 0     @ ChargeTimer
	encsetvar 7, 0     @ ChallengeTimer
	encsetvar 9, 0     @ ThunderTimer
	encsetvar 10, 0    @ GigaCharge
	encsetvar 11, 1    @ Exhausted
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	removeterrain
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_TapuKoko_Exhausted_NoStats
EncScript_TapuKoko_Exhausted_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 9     @ species catch rate (3) x3
	encsetballs ENC_BALLS_ALLOWED
	return
