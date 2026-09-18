// Galarian Articuno, "The Cruel Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_GalarianArticuno_Intro:
// 0 Phase (0/1/2/3), 1 Focus (0-3), 2 Predicted (0 none/1 phys/2 spec/3 status),
// 3 PredictionResolved (this-turn latch), 4 LastCategory (previous turn's player category),
// 5 MindGamesCooldown, 6 MindBreakCooldown (Phase 2 only), 7 MoveGuard (shared by the three
// category triggers), 8 SEGuard, 9 SwitchGuard, 10 TurnGuard (the once-per-turn scaffold).
//
// MoveCategory can only be read from a Condition, never from a script, so CategoryPhysical/
// Special/Status are the only place that fact is tested. Everything past that point - the repeat
// check, the category store, the Predict hit/resolve - is Var-driven and lives inside the script.
// Every OnMoveEnd/OnSwitchIn trigger that reacts to Event.* closes its own guard var (MoveGuard/
// SEGuard/SwitchGuard) the moment its script runs, since Event.* stays true for the whole dispatch
// and a script that leaves its own condition true re-selects until the runaway guard trips.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// Focus rises by one, clamped at 3. Silent unless it actually moved, so a call at the cap costs
// nothing but a wasted call.
EncScript_GalarianArticuno_FocusGain:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_GalarianArticuno_FocusGain_Done
	encaddvar 1, 1
	printstring STRINGID_ENCGALARIANARTICUNOFOCUSRISING
	waitmessage B_WAIT_TIME_LONG
EncScript_GalarianArticuno_FocusGain_Done:
	return

// Focus falls by one, floored at 0. Silent - every caller prints its own line.
EncScript_GalarianArticuno_FocusDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_GalarianArticuno_FocusDown_Done
	encsubvar 1, 1
EncScript_GalarianArticuno_FocusDown_Done:
	return

// PSYCHIC LOCK. One random disruption, roughly a 25/25/25/25 split across four cascading rolls.
// encdisablemove chains to the stat branch on its own fail label (turn 1, nothing to disable, or
// something already disabled), so a bare field never wastes the roll. Stat stages are written
// absolutely (push to +12, which clamps at +6, then one delta lands on -2) so the debuff survives a
// Haze or a switch landing in between. Shared by the trigger-fired Psychic Lock and Mind Break.
EncScript_GalarianArticuno_PsychicLockEffect:
	printstring STRINGID_ENCGALARIANARTICUNOLOCKBANNER
	waitmessage B_WAIT_TIME_LONG
	encjumpifchance 25, EncScript_GalarianArticuno_PsychicLockEffect_Seal
	encjumpifchance 33, EncScript_GalarianArticuno_PsychicLockEffect_Trap
	encjumpifchance 50, EncScript_GalarianArticuno_PsychicLockEffect_Item
	goto EncScript_GalarianArticuno_PsychicLockEffect_Stats
EncScript_GalarianArticuno_PsychicLockEffect_Seal:
	encdisablemove ENC_TARGET_PLAYER_LEFT, 3, EncScript_GalarianArticuno_PsychicLockEffect_Stats
	printstring STRINGID_ENCGALARIANARTICUNOLOCKMOVE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianArticuno_PsychicLockEffect_Trap:
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	printstring STRINGID_ENCGALARIANARTICUNOLOCKTRAP
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianArticuno_PsychicLockEffect_Item:
	encsetembargo ENC_TARGET_PLAYER_LEFT, 3
	printstring STRINGID_ENCGALARIANARTICUNOLOCKITEM
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_GalarianArticuno_PsychicLockEffect_Stats:
	encjumpifchance 50, EncScript_GalarianArticuno_PsychicLockEffect_SpAtk
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -7
	goto EncScript_GalarianArticuno_PsychicLockEffect_StatsShow
EncScript_GalarianArticuno_PsychicLockEffect_SpAtk:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, -7
EncScript_GalarianArticuno_PsychicLockEffect_StatsShow:
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCGALARIANARTICUNOLOCKSTATS
	waitmessage B_WAIT_TIME_LONG
	return

// THE COUNTER-BLAST. 16% of the player's max HP in Phase 0, 24% in Phase 1-2 (Perfect Prediction).
// A direct HP write, same as every other scripted payload in this system - Substitute doesn't see it.
EncScript_GalarianArticuno_PredictHit:
	printstring STRINGID_ENCGALARIANARTICUNOPREDICTHIT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_GalarianArticuno_PredictHit_16
	encchangehp ENC_TARGET_PLAYER_LEFT, -24, ENC_AMOUNT_PERCENT
	goto EncScript_GalarianArticuno_PredictHit_Done
EncScript_GalarianArticuno_PredictHit_16:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
EncScript_GalarianArticuno_PredictHit_Done:
	encsetvar 3, 1     @ PredictionResolved
	return

// --- Trigger entry points ---

// No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_GalarianArticuno_Intro::
	printstring STRINGID_ENCGALARIANARTICUNOINTRO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch out can leave it re-armed.
EncScript_GalarianArticuno_TurnOpen::
	flushtextbox
	encsetvar 10, 1    @ TurnGuard
	encsetvar 7, 0     @ MoveGuard: re-arms the category triggers
	encsetvar 8, 0     @ SEGuard
	encsetvar 9, 0     @ SwitchGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_GalarianArticuno_TurnOpen_Done   @ Phase 3: quiet
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 2    @ Predicted
	encsetvar 3, 0     @ PredictionResolved
EncScript_GalarianArticuno_TurnOpen_Done:
	return

EncScript_GalarianArticuno_TurnClose::
	encsetvar 10, 0    @ TurnGuard
	return

// A repeated PHYSICAL category feeds Focus; every use overwrites LastCategory; a category that
// matches Articuno's prediction pays the counter-blast.
EncScript_GalarianArticuno_CategoryPhysical::
	encsetvar 7, 1     @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_GalarianArticuno_CategoryPhysical_Store
	call EncScript_GalarianArticuno_FocusGain
EncScript_GalarianArticuno_CategoryPhysical_Store:
	encsetvar 4, 1     @ LastCategory
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_GalarianArticuno_CategoryPhysical_Done
	call EncScript_GalarianArticuno_PredictHit
EncScript_GalarianArticuno_CategoryPhysical_Done:
	return

EncScript_GalarianArticuno_CategorySpecial::
	encsetvar 7, 1     @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 4, 2, EncScript_GalarianArticuno_CategorySpecial_Store
	call EncScript_GalarianArticuno_FocusGain
EncScript_GalarianArticuno_CategorySpecial_Store:
	encsetvar 4, 2     @ LastCategory
	encjumpifvar CMP_NOT_EQUAL, 2, 2, EncScript_GalarianArticuno_CategorySpecial_Done
	call EncScript_GalarianArticuno_PredictHit
EncScript_GalarianArticuno_CategorySpecial_Done:
	return

// A status move feeds Focus unconditionally, repeat or not - reading a lull is as informative as
// reading a repeat.
EncScript_GalarianArticuno_CategoryStatus::
	encsetvar 7, 1     @ MoveGuard
	call EncScript_GalarianArticuno_FocusGain
	encsetvar 4, 3     @ LastCategory
	encjumpifvar CMP_NOT_EQUAL, 2, 3, EncScript_GalarianArticuno_CategoryStatus_Done
	call EncScript_GalarianArticuno_PredictHit
EncScript_GalarianArticuno_CategoryStatus_Done:
	return

EncScript_GalarianArticuno_SuperEffective::
	encsetvar 8, 1     @ SEGuard
	call EncScript_GalarianArticuno_FocusGain
	return

EncScript_GalarianArticuno_SwitchFocus::
	encsetvar 9, 1     @ SwitchGuard
	call EncScript_GalarianArticuno_FocusGain
	return

EncScript_GalarianArticuno_PsychicLock::
	call EncScript_GalarianArticuno_PsychicLockEffect
	encsetvar 1, 0     @ Focus
	printstring STRINGID_ENCGALARIANARTICUNOFOCUSFADING
	waitmessage B_WAIT_TIME_LONG
	return

// The player varied and stayed unread. Focus falls, Predicted clears (TurnOpen overwrites it next
// turn regardless, but a stale nonzero value here is dead weight).
EncScript_GalarianArticuno_PredictMiss::
	printstring STRINGID_ENCGALARIANARTICUNOPREDICTMISS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_GalarianArticuno_FocusDown
	encsetvar 2, 0     @ Predicted
	return

EncScript_GalarianArticuno_MindGamesTick::
	encsubvar 5, 1     @ MindGamesCooldown
	return

// RESIST steels the player's nerve (Focus+1, a crit buff); YIELD gives ground (Articuno ATK+1,
// Focus-1). Cooldown resets to 4 on either branch.
EncScript_GalarianArticuno_MindGamesOffer::
	flushtextbox
	encsetvar 5, 4     @ MindGamesCooldown
	encaskyesno STRINGID_ENCGALARIANARTICUNOMINDGAMESASK, EncScript_GalarianArticuno_MindGamesOffer_Resist
	printstring STRINGID_ENCGALARIANARTICUNOMINDGAMESYIELD
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_GalarianArticuno_FocusDown
	return
EncScript_GalarianArticuno_MindGamesOffer_Resist:
	printstring STRINGID_ENCGALARIANARTICUNOMINDGAMESRESIST
	waitmessage B_WAIT_TIME_LONG
	encsetcrit ENC_TARGET_PLAYER_LEFT, TRUE
	call EncScript_GalarianArticuno_FocusGain
	return

EncScript_GalarianArticuno_MindBreakTick::
	encsubvar 6, 1     @ MindBreakCooldown
	return

// Fires regardless of Focus - Mind Break stacks on top of everything above, not instead of it.
EncScript_GalarianArticuno_MindBreakFire::
	flushtextbox
	encsetvar 6, 3     @ MindBreakCooldown
	call EncScript_GalarianArticuno_PsychicLockEffect
	return

// --- Phase transitions ---

EncScript_GalarianArticuno_Phase1Enter::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCGALARIANARTICUNOPHASE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

EncScript_GalarianArticuno_Phase2Enter::
	encsetvar 0, 2     @ Phase
	encsetvar 6, 3     @ MindBreakCooldown
	printstring STRINGID_ENCGALARIANARTICUNOPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// SPENT, and the catch window (guideline 10). Tears down every meter and every disruption the fight
// raised, mirroring the Properties: block plus everything Psychic Lock/Mind Break imposed.
EncScript_GalarianArticuno_Phase3Enter::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCGALARIANARTICUNOCATCHWINDOW
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Focus
	encsetvar 2, 0     @ Predicted
	encsetvar 3, 0     @ PredictionResolved
	encsetvar 4, 0     @ LastCategory
	encsetvar 5, 0     @ MindGamesCooldown
	encsetvar 6, 0     @ MindBreakCooldown
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encsetembargo ENC_TARGET_PLAYER_LEFT, 0
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_GalarianArticuno_Phase3Enter_NoStats
EncScript_GalarianArticuno_Phase3Enter_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	return
