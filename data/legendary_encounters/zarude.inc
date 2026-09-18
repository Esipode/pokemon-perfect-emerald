// Zarude, "The Rogue Monkey Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Zarude_Init:
// 0 Territory (0-6), 1 Phase (0 Territorial / 1 Sovereign / 2 Guardian), 2 VineTimer (0-3),
// 3 BarrierCharges (0-3), 4 Rage (0-3), 5 AmbushState (0 idle / 1 resolves this turn),
// 6 Mark (boss HP% snapshot taken every turn open), 7 TurnGuard, 8 ReminderCounter (0-4),
// 9 MoveGuard (once-per-turn gate for the territory-raising OnMoveEnd triggers), 10 Scratch
// (consumable copy of Mark for damage-since-mark checks), 11 FeedTimer (0-2).
//
// TERRITORY IS THE GUARD, in both directions: it climbs from fighting and only falls from an
// anticipated ambush or a vine's own expiry. Everything else - tiers, the barrier, Jungle Feed -
// reads Phase/Territory/Rage/BarrierCharges rather than tracking its own copy.
//
// EVERY OnMoveEnd TRIGGER WITHOUT Once/OnEnter IS SELF-DISABLING through a variable its own script
// writes: the three territory-raising scripts all set MoveGuard=1 first (cleared at the next
// TurnOpen, capping territory gain to once per turn); ambush resolution clears AmbushState; vine
// bind sets VineTimer to 3. None of them rely on Once, which would mean once per battle.
//
// THE LEDGER (TurnClose) is the only place vines, the barrier, Jungle Feed and the reminder cue
// resolve - one script with internal branches rather than several triggers, to stay clear of the
// 8-scripts-per-checkpoint cap. It is gated on TurnGuard and closes its own latch last.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared entry points (called directly by triggers) ---

// OnBattleStart.
EncScript_Zarude_Init::
	printstring STRINGID_ENCZARUDESTIRSINTHETREES
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn. Leads with flushtextbox since a later branch can play an animation before
// any dialogue (the BG0 gotcha). TurnGuard is set immediately so no branch out can leave it
// re-armed. The ambush telegraph piggybacks ReminderCounter's cadence (every 5th turn, when it
// is freshly reset to 0) rather than a separate modulo counter.
EncScript_Zarude_TurnOpen::
	flushtextbox
	encsetvar 7, 1     @ TurnGuard
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET   @ Mark
	encsetvar 9, 0     @ MoveGuard
	encjumpifvar CMP_LESS_THAN, 0, 4, EncScript_Zarude_TurnOpen_Done
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Zarude_TurnOpen_Done
	encjumpifvar CMP_NOT_EQUAL, 8, 0, EncScript_Zarude_TurnOpen_Done
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCZARUDEAMBUSHTELEGRAPH
	waitmessage B_WAIT_TIME_LONG
	encsetvar 5, 1     @ AmbushState
EncScript_Zarude_TurnOpen_Done:
	return

// Territory +1 from a Grass move of the boss's landing on the player. Silent - tier crossings
// print their own line.
EncScript_Zarude_TerritoryFromGrass::
	encsetvar 9, 1     @ MoveGuard
	encaddvar 0, 1     @ Territory
	return

// Territory +1 from any other move damage the boss lands on the player.
EncScript_Zarude_TerritoryFromDamage::
	encsetvar 9, 1
	encaddvar 0, 1
	return

// Territory +1 when the player lands a real hit on the boss - measured against the turn-open
// mark, since Event.OldValue/NewValue are stale after a miss and can't stand in for it.
EncScript_Zarude_TerritoryFromThreat::
	encsetvar 9, 1
	enccopyvar 10, 6   @ Scratch = Mark
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Zarude_TerritoryFromThreat_Done
	encjumpifvar CMP_LESS_THAN, 10, 15, EncScript_Zarude_TerritoryFromThreat_Done
	encaddvar 0, 1
EncScript_Zarude_TerritoryFromThreat_Done:
	return

// Jungle Growth tier crossings during the Territorial Fight. Once + OnEnter closes each
// permanently on its own crossing.
EncScript_Zarude_OvergrownTier::
	printstring STRINGID_ENCZARUDEOVERGROWN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
	return

EncScript_Zarude_JungleTier::
	printstring STRINGID_ENCZARUDEJUNGLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
	return

// Ambush resolution. Both arms clear AmbushState on every path, which is what lets the trigger
// have no Once/OnEnter at all.
EncScript_Zarude_AmbushAnticipated::
	printstring STRINGID_ENCZARUDEAMBUSHANTICIPATED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_PROTECTED_ITSELF
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Zarude_AmbushAnticipated_Clear
	encsubvar 0, 1     @ Territory, floored at 0 by the check above
EncScript_Zarude_AmbushAnticipated_Clear:
	encsetvar 5, 0     @ AmbushState
	return

EncScript_Zarude_AmbushStrikes::
	printstring STRINGID_ENCZARUDEAMBUSHSTRIKES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	encsetvar 5, 0     @ AmbushState
	return

// Vine bind. Self-disabling: setting VineTimer to 3 closes this trigger's own condition until the
// ledger counts it back down to 0.
EncScript_Zarude_VineBindApply::
	printstring STRINGID_ENCZARUDEVINESWRAP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
	encsetvar 2, 3     @ VineTimer
	return

// 50% - JUNGLE SOVEREIGN. Territory is forced to 5 (Deep Jungle) and the barrier goes up at full
// charges. No trainerslide - this is a wild Pokemon (guideline 13).
EncScript_Zarude_Phase2_DeepJungle::
	printstring STRINGID_ENCZARUDECHESTBEAT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 1, 1     @ Phase: Sovereign
	encsetvar 0, 5     @ Territory
	encsetvar 3, 3     @ BarrierCharges
	printstring STRINGID_ENCZARUDEDEEPJUNGLE
	waitmessage B_WAIT_TIME_LONG
	return

// 20% - GUARDIAN OF THE JUNGLE. Territory is forced to 6 (Ancient Jungle) and Jungle Fury raises
// Attack once.
EncScript_Zarude_Phase3_Guardian::
	printstring STRINGID_ENCZARUDEANCIENTJUNGLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 1, 2     @ Phase: Guardian
	encsetvar 0, 6     @ Territory
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCZARUDEJUNGLEFURY
	waitmessage B_WAIT_TIME_LONG
	return

// 10% - the catch window (guideline 10).
EncScript_Zarude_BallsUnlock::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 45
	printstring STRINGID_ENCZARUDENOWISTHEMOMENT
	waitmessage B_WAIT_TIME_LONG
	return

// JUNGLE RAGE finisher, once, at <= 8% HP. Base 20% scaled by Rage (0-3, +2 each) and by the
// barrier charges spent (3 - BarrierCharges, +2 each), branched rather than computed since the
// engine has no var arithmetic beyond add/sub-by-constant. Every leaf is <= 30% (guideline 7
// ceiling); the two combinations that would overshoot it (Rage 2-3 with BarrierCharges 0) are
// written as the clamped 30 directly.
EncScript_Zarude_JungleRage::
	printstring STRINGID_ENCZARUDEJUNGLERAGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Zarude_JungleRage_R0
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Zarude_JungleRage_R1
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Zarude_JungleRage_R2
	goto EncScript_Zarude_JungleRage_R3
EncScript_Zarude_JungleRage_R0:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Zarude_JungleRage_Deal26
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Zarude_JungleRage_Deal24
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Zarude_JungleRage_Deal22
	goto EncScript_Zarude_JungleRage_Deal20
EncScript_Zarude_JungleRage_R1:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Zarude_JungleRage_Deal28
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Zarude_JungleRage_Deal26
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Zarude_JungleRage_Deal24
	goto EncScript_Zarude_JungleRage_Deal22
EncScript_Zarude_JungleRage_R2:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Zarude_JungleRage_Deal30
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Zarude_JungleRage_Deal28
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Zarude_JungleRage_Deal26
	goto EncScript_Zarude_JungleRage_Deal24
EncScript_Zarude_JungleRage_R3:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Zarude_JungleRage_Deal30
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Zarude_JungleRage_Deal30
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Zarude_JungleRage_Deal28
	goto EncScript_Zarude_JungleRage_Deal26
EncScript_Zarude_JungleRage_Deal30:
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	return
EncScript_Zarude_JungleRage_Deal28:
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
	return
EncScript_Zarude_JungleRage_Deal26:
	encchangehp ENC_TARGET_PLAYER_LEFT, -26, ENC_AMOUNT_PERCENT
	return
EncScript_Zarude_JungleRage_Deal24:
	encchangehp ENC_TARGET_PLAYER_LEFT, -24, ENC_AMOUNT_PERCENT
	return
EncScript_Zarude_JungleRage_Deal22:
	encchangehp ENC_TARGET_PLAYER_LEFT, -22, ENC_AMOUNT_PERCENT
	return
EncScript_Zarude_JungleRage_Deal20:
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	return

// THE LEDGER. Vines, the barrier, Jungle Feed and the reminder cue - resolved once per turn,
// gated on TurnGuard, which it clears last.
EncScript_Zarude_TurnClose::
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Zarude_TurnClose_Barrier
	encsubvar 2, 1     @ VineTimer
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Zarude_TurnClose_VineChip
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	printstring STRINGID_ENCZARUDEVINESWITHER
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 0, 6, EncScript_Zarude_TurnClose_Barrier
	encaddvar 0, 1     @ Territory: the vines wither back into it
	goto EncScript_Zarude_TurnClose_Barrier
EncScript_Zarude_TurnClose_VineChip:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -3, ENC_AMOUNT_PERCENT

// Jungle Barrier: a turn where the player's damage clears 5% breaks a charge; a turn where none
// breaks lets the troop rally.
EncScript_Zarude_TurnClose_Barrier:
	encjumpifvar CMP_NOT_EQUAL, 1, 1, EncScript_Zarude_TurnClose_Feed
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Zarude_TurnClose_Feed
	enccopyvar 10, 6   @ Scratch = Mark
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Zarude_TurnClose_Barrier_NoDamage
	goto EncScript_Zarude_TurnClose_Barrier_Check
EncScript_Zarude_TurnClose_Barrier_NoDamage:
	encsetvar 10, 0
EncScript_Zarude_TurnClose_Barrier_Check:
	encjumpifvar CMP_LESS_THAN, 10, 5, EncScript_Zarude_TurnClose_Rally
	encsubvar 3, 1     @ BarrierCharges
	printstring STRINGID_ENCZARUDEBARRIERBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	goto EncScript_Zarude_TurnClose_Feed
EncScript_Zarude_TurnClose_Rally:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCZARUDETROOPRALLIES
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Zarude_TurnClose_Feed
	encaddvar 4, 1     @ Rage

// Jungle Feed: every third turn in the Guardian phase, heal 4% unless the player dealt 5%+ that
// turn.
EncScript_Zarude_TurnClose_Feed:
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Zarude_TurnClose_Reminder
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Zarude_TurnClose_Feed_Attempt
	encaddvar 11, 1    @ FeedTimer
	goto EncScript_Zarude_TurnClose_Reminder
EncScript_Zarude_TurnClose_Feed_Attempt:
	encsetvar 11, 0
	enccopyvar 10, 6   @ Scratch = Mark
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Zarude_TurnClose_Feed_NoDamage
	goto EncScript_Zarude_TurnClose_Feed_Check
EncScript_Zarude_TurnClose_Feed_NoDamage:
	encsetvar 10, 0
EncScript_Zarude_TurnClose_Feed_Check:
	encjumpifvar CMP_GREATER_THAN, 10, 4, EncScript_Zarude_TurnClose_Feed_Interrupted
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCZARUDEJUNGLEFEEDS
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Zarude_TurnClose_Reminder
EncScript_Zarude_TurnClose_Feed_Interrupted:
	printstring STRINGID_ENCZARUDEFEEDINTERRUPTED
	waitmessage B_WAIT_TIME_LONG

// The recurring cue (guideline 14): a short pulse every 4 turns so a long fight never reads as
// having gone quiet.
EncScript_Zarude_TurnClose_Reminder:
	encjumpifvar CMP_NOT_EQUAL, 8, 4, EncScript_Zarude_TurnClose_ReminderTick
	encsetvar 8, 0     @ ReminderCounter
	printstring STRINGID_ENCZARUDETERRITORYRISES
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Zarude_TurnClose_Safety
EncScript_Zarude_TurnClose_ReminderTick:
	encaddvar 8, 1     @ ReminderCounter

EncScript_Zarude_TurnClose_Safety:
	encsetvar 5, 0     @ AmbushState: safety clear if a turn passed with no resolution
	encsetvar 7, 0     @ TurnGuard
	return

// Presentation-only wrap-up.
EncScript_Zarude_BattleEndFlavor::
	printstring STRINGID_ENCZARUDEFALLSSILENT
	waitmessage B_WAIT_TIME_LONG
	return
