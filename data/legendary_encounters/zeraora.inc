// Zeraora, "The Thunderclap Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Zeraora_Init:
// 0 Momentum (0-10), 1 Phase (1-4: Opening/Overdrive/Maximum Velocity/Spent), 2 Network (0-5
// Lightning Network), 3 Chain (Speed Chain streak), 4 MvtTimer - RETIRED (Maximum Velocity is a
// persistent phase now, not a timed window; the index is kept unused rather than renumbering
// every var below it), 5 SpeedCmp (enccomparestat Boss vs PlayerLeft, per turn), 6 TerrainGuard
// (once-per-turn Electric Terrain gain gate), 7 PlasmaArmed
// (Plasma Overload armed-this-turn latch), 8 ThunderGuard (0 idle / 1 rolled-missed /
// 2 rolled-fired), 9 HandledBossHit, 10 HandledPlayerHit, 11 TurnGuard, 12 InitDone,
// 13 PlasmaHandled (once-per-turn gate for the Plasma Fists connect bonus), 14 EventGuard
// (shared once-per-turn latch for KO/PlayerSwitch/ForcedOut - OnFaint and OnSwitchIn never
// overlap, so one var covers all three without exceeding the checkpoint's 8-script cap).
//
// MOMENTUM IS THE RESOURCE. It only ever moves through the MomentumAdd*/MomentumSub* helpers
// below, each of which clamps to 0-10 on its own side, so no call site can push it out of range.
//
// EVERY OnMoveEnd/OnFaint/OnSwitchIn TRIGGER IS SELF-DISABLING through a variable its own script
// writes (HandledBossHit/HandledPlayerHit/PlasmaHandled/InitDone-gated latches), because the
// re-evaluation loop keeps re-selecting a trigger whose Event.* condition still holds. The Plasma
// Fists bonus gets its own latch (PlasmaHandled) rather than sharing HandledBossHit, because
// whichever of the two triggers runs first in the same dispatch would otherwise close the other's
// gate before it had a chance to fire.
//
// STAT STAGES ARE WRITTEN ABSOLUTELY: +12 clamps to the +6 ceiling, then a single drop lands on
// the stage actually meant. Overdrive's Speed buff, Plasma Overload's Atk/SpAtk buff and its
// revert all use this so they survive a Haze or a switch landing in between.
//
// THUNDERCLAP'S TWO-STATE LATCH: ThunderGuard is 0 (idle), 1 (rolled this turn, missed) or
// 2 (rolled, fired). The revert script only re-issues encsetmove - which refills PP - when it
// finds state 2, so a missed roll never quietly tops off a moveset that was never touched.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so every script here
// uses playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// Momentum, capped at 10. Each Add helper checks its own headroom before adding so the var never
// wraps past the cap.
EncScript_Zeraora_MomentumAdd1:
	encjumpifvar CMP_GREATER_THAN, 0, 9, EncScript_Zeraora_MomentumAdd1_Cap
	encaddvar 0, 1
	return
EncScript_Zeraora_MomentumAdd1_Cap:
	encsetvar 0, 10
	return

EncScript_Zeraora_MomentumAdd2:
	encjumpifvar CMP_GREATER_THAN, 0, 8, EncScript_Zeraora_MomentumAdd2_Cap
	encaddvar 0, 2
	return
EncScript_Zeraora_MomentumAdd2_Cap:
	encsetvar 0, 10
	return

EncScript_Zeraora_MomentumAdd3:
	encjumpifvar CMP_GREATER_THAN, 0, 7, EncScript_Zeraora_MomentumAdd3_Cap
	encaddvar 0, 3
	return
EncScript_Zeraora_MomentumAdd3_Cap:
	encsetvar 0, 10
	return

// Momentum, floored at 0. subbyte does not floor on its own, so each Sub helper tests headroom
// before subtracting.
EncScript_Zeraora_MomentumSub1:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Zeraora_MomentumSub1_Done
	encsubvar 0, 1
EncScript_Zeraora_MomentumSub1_Done:
	return

EncScript_Zeraora_MomentumSub2:
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Zeraora_MomentumSub2_Floor
	encsubvar 0, 2
	return
EncScript_Zeraora_MomentumSub2_Floor:
	encsetvar 0, 0
	return

EncScript_Zeraora_MomentumSub6:
	encjumpifvar CMP_LESS_THAN, 0, 6, EncScript_Zeraora_MomentumSub6_Floor
	encsubvar 0, 6
	return
EncScript_Zeraora_MomentumSub6_Floor:
	encsetvar 0, 0
	return

// THE LIGHTNING NETWORK. Network can only reach 5 by way of this add (nothing else writes it
// upward), so completion resets it to 0 in the same call - the var is never left holding 5.
EncScript_Zeraora_NetworkAdd1:
	encaddvar 2, 1
	encjumpifvar CMP_LESS_THAN, 2, 5, EncScript_Zeraora_NetworkAdd1_Done
	printstring STRINGID_ENCZERAORANETWORKCOMPLETE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 2, 0
	call EncScript_Zeraora_MomentumAdd3
EncScript_Zeraora_NetworkAdd1_Done:
	return

EncScript_Zeraora_NetworkSub1:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Zeraora_NetworkSub1_Done
	encsubvar 2, 1
EncScript_Zeraora_NetworkSub1_Done:
	return

// THE SPEED CHAIN. Capped at 9 as a safety margin past the 3/6 thresholds it actually cares
// about. The crit window opens at 3 and the callout fires once, on the crossing - every further
// hit re-sets the same crit flag (harmless, silent) without re-printing the line.
EncScript_Zeraora_ChainAdd1:
	encjumpifvar CMP_GREATER_THAN, 3, 8, EncScript_Zeraora_ChainAdd1_Check
	encaddvar 3, 1
EncScript_Zeraora_ChainAdd1_Check:
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Zeraora_ChainAdd1_Done
	encsetcrit ENC_TARGET_BOSS, TRUE
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Zeraora_ChainAdd1_Done
	printstring STRINGID_ENCZERAORASPEEDCHAIN
	waitmessage B_WAIT_TIME_LONG
EncScript_Zeraora_ChainAdd1_Done:
	return

EncScript_Zeraora_ChainReset:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Zeraora_ChainReset_Done
	encsetvar 3, 0
	encsetcrit ENC_TARGET_BOSS, FALSE
EncScript_Zeraora_ChainReset_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. No trainerslidein - this is a wild Pokemon (guideline 13). The system's own
// "Wild Zeraora appeared!" message covers the introduction; nothing scripted needed on top of it.
EncScript_Zeraora_Init::
	encsetvar 1, 1     @ Phase starts at Opening - vars zero-init, and 0 is not a valid Phase value
	encsetvar 12, 1    @ InitDone
	return

// Top of every turn. TurnGuard is set immediately so no branch out can leave it re-armed.
// SpeedCmp is measured here (Boss vs PlayerLeft) so PlayerHit can read "who moved first" without
// re-deriving it mid-checkpoint. Evasion is re-applied absolutely to neutral before this turn's
// Lightning Step can raise it again, so a Momentum drop that happens later never leaves a stale
// +1 standing from a hit two turns ago.
EncScript_Zeraora_Upkeep::
	flushtextbox
	encsetvar 11, 1    @ TurnGuard
	enccomparestat ENC_TARGET_BOSS, ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 5
	encsetvar 9, 0     @ HandledBossHit
	encsetvar 10, 0    @ HandledPlayerHit
	encsetvar 13, 0    @ PlasmaHandled
	encsetvar 14, 0    @ EventGuard
	encsetvar 6, 0     @ TerrainGuard
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -6
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Zeraora_Upkeep_Overdrive
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Zeraora_Upkeep_MaxVelocity
	return
EncScript_Zeraora_Upkeep_Overdrive:
	call EncScript_Zeraora_MomentumAdd1
	return
EncScript_Zeraora_Upkeep_MaxVelocity:
	call EncScript_Zeraora_MomentumAdd2
	return

// Just clears TurnGuard. Maximum Velocity is now a persistent phase (40% down to the 10% catch
// trigger, not a timed window), so there is nothing left to resolve here.
EncScript_Zeraora_ClearTurnGuard::
	encsetvar 11, 0    @ TurnGuard
	return

// Zeraora hits the player. Chain/Network both move, and Lightning Step raises evasion for the
// exchange once Momentum is at least 2 (upkeep resets it absolutely next turn).
EncScript_Zeraora_BossHit::
	encsetvar 9, 1     @ HandledBossHit
	call EncScript_Zeraora_MomentumAdd1
	call EncScript_Zeraora_ChainAdd1
	call EncScript_Zeraora_NetworkAdd1
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Zeraora_BossHit_Done
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 1
EncScript_Zeraora_BossHit_Done:
	return

// Plasma Fists connected on top of the general hit above. Own latch (PlasmaHandled) so it can't
// be starved by EncScript_Zeraora_BossHit closing HandledBossHit first in the same dispatch.
EncScript_Zeraora_PlasmaFistsCharge::
	encsetvar 13, 1    @ PlasmaHandled
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Zeraora_PlasmaFistsCharge_Small
	call EncScript_Zeraora_MomentumAdd3
	return
EncScript_Zeraora_PlasmaFistsCharge_Small:
	call EncScript_Zeraora_MomentumAdd1
	return

// The player hits Zeraora. Momentum only drops if the player also moved first this turn
// (SpeedCmp == 0, Boss reads lower than PlayerLeft) - denying speed AND landing the hit is what
// the fight is asking for, not the hit alone.
EncScript_Zeraora_PlayerHit::
	encsetvar 10, 1    @ HandledPlayerHit
	call EncScript_Zeraora_NetworkSub1
	call EncScript_Zeraora_ChainReset
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Zeraora_PlayerHit_Done
	call EncScript_Zeraora_MomentumSub1
EncScript_Zeraora_PlayerHit_Done:
	return

// Zeraora's own move whiffed or failed outright. Shares HandledBossHit with the hit case - the
// two conditions are mutually exclusive on Event.Cause, so only one of them ever runs per event.
EncScript_Zeraora_BossMiss::
	encsetvar 9, 1
	call EncScript_Zeraora_MomentumSub1
	call EncScript_Zeraora_ChainReset
	return

// Newly paralyzed: losing Speed control costs it more than a whiff does.
EncScript_Zeraora_Paralyzed::
	call EncScript_Zeraora_MomentumSub2
	call EncScript_Zeraora_ChainReset
	return

// The player's mon went down to Zeraora - the single biggest swing in the fight, on purpose.
EncScript_Zeraora_KO::
	encsetvar 14, 1    @ EventGuard
	call EncScript_Zeraora_MomentumAdd3
	call EncScript_Zeraora_NetworkAdd1
	return

// A fresh Pokemon walking in hands Zeraora tempo.
EncScript_Zeraora_PlayerSwitch::
	encsetvar 14, 1    @ EventGuard
	call EncScript_Zeraora_MomentumAdd1
	return

// Zeraora dragged out mid-exchange loses its rhythm.
EncScript_Zeraora_ForcedOut::
	encsetvar 14, 1    @ EventGuard
	call EncScript_Zeraora_MomentumSub2
	return

// Electric Terrain favours it - once per turn, whoever raised it.
EncScript_Zeraora_TerrainGain::
	encsetvar 6, 1     @ TerrainGuard
	call EncScript_Zeraora_MomentumAdd1
	return

// 70% HP - OVERDRIVE. Momentum starts trickling up passively (see EncScript_Zeraora_Upkeep) and
// Speed is buffed absolutely to +2.
EncScript_Zeraora_Overdrive::
	printstring STRINGID_ENCZERAORAOVERDRIVE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetvar 1, 2     @ Phase
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -4    @ lands at absolute +2
	return

// 40% HP - MAXIMUM VELOCITY. Momentum maxes out and the Mega Evolution fires with its own stock
// presentation (no extra playanimation needed on top of it). Survive guards this whole phase down
// to the 10% catch trigger (EncScript_Zeraora_Spent), which is what clears it - without it a big
// enough hit anywhere in this stretch KOs the boss outright and the catch window never opens.
EncScript_Zeraora_MaxVelocity::
	printstring STRINGID_ENCZERAORAMAXVELOCITY
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 3     @ Phase
	encsetvar 0, 10    @ Momentum
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encmegaevolve ENC_TARGET_BOSS, EncScript_Zeraora_MaxVelocity_Done
EncScript_Zeraora_MaxVelocity_Done:
	return

// Momentum re-maxed while in Maximum Velocity: arm Plasma Overload. Atk/SpAtk go to a flat
// absolute +2 (guideline 7's ceiling), reverted in full by the release script below.
EncScript_Zeraora_PlasmaOverloadArm::
	printstring STRINGID_ENCZERAORAPLASMAARM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -4      @ absolute +2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -4    @ absolute +2
	encsetvar 7, 1     @ PlasmaArmed
	return

// A hit lands on Zeraora while Plasma Overload is armed: the discharge. A mid-fight burst, not
// the catch window - that's the flat 10% HP trigger now (EncScript_Zeraora_Spent), so Survive is
// left untouched here; it keeps guarding the boss through the rest of Maximum Velocity. The
// Atk/SpAtk buff is reverted to neutral, not just undone by the same delta, so it can't drift if
// something else touched those stages while armed. PlasmaArmed clears so a second Momentum
// max-out later in the phase can arm it again.
EncScript_Zeraora_PlasmaOverloadRelease::
	printstring STRINGID_ENCZERAORAPLASMARELEASE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -6      @ back to neutral
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6    @ back to neutral
	encsetvar 0, 0     @ Momentum spent
	encsetvar 7, 0     @ PlasmaArmed
	return

// Roll Thunderclap. ThunderGuard closes on either outcome so the trigger cannot re-select this
// turn regardless of whether the roll hit.
EncScript_Zeraora_ThunderclapArm::
	encsetvar 8, 1     @ ThunderGuard: rolled
	encjumpifchance 50, EncScript_Zeraora_ThunderclapArm_Fire
	return
EncScript_Zeraora_ThunderclapArm_Fire:
	call EncScript_Zeraora_MomentumSub6
	encsetmove ENC_TARGET_BOSS, 0, MOVE_QUICK_ATTACK
	encsetmove ENC_TARGET_BOSS, 1, MOVE_QUICK_ATTACK
	encsetmove ENC_TARGET_BOSS, 2, MOVE_QUICK_ATTACK
	encsetmove ENC_TARGET_BOSS, 3, MOVE_QUICK_ATTACK
	encsetvar 8, 2     @ ThunderGuard: fired
	printstring STRINGID_ENCZERAORATHUNDERCLAP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// Restore the real moveset - only when ThunderGuard shows a fire (2). A missed roll (1) just
// clears the latch, so a turn where nothing changed never re-issues PP into an untouched moveset.
EncScript_Zeraora_ThunderclapRevert::
	encjumpifvar CMP_NOT_EQUAL, 8, 2, EncScript_Zeraora_ThunderclapRevert_Clear
	encsetmove ENC_TARGET_BOSS, 0, MOVE_PLASMA_FISTS
	encsetmove ENC_TARGET_BOSS, 1, MOVE_WILD_CHARGE
	encsetmove ENC_TARGET_BOSS, 2, MOVE_CLOSE_COMBAT
	encsetmove ENC_TARGET_BOSS, 3, MOVE_THUNDER_PUNCH
EncScript_Zeraora_ThunderclapRevert_Clear:
	encsetvar 8, 0
	return

// 10% HP - the catch window (guideline 10). De-Megas via encformchange (the general, reversible
// form of encmegaevolve - see heatran.inc for the same idiom) rather than leaving it Mega for a
// catch, then tears down everything the fight raised: any stat stage on either side, a Plasma
// Overload buff still live if the boss crossed 10% while armed, and the Speed Chain's crit flag.
// Survive clears last, matching every other legendary's catch-window teardown - the automatic
// catch-window guard takes over once Balls: allowed lands.
EncScript_Zeraora_Spent::
	encsetvar 1, 4     @ Phase: Spent
	printstring STRINGID_ENCZERAORACATCH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encformchange ENC_TARGET_BOSS, SPECIES_ZERAORA, EncScript_Zeraora_Spent_NoRevert, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Zeraora_Spent_NoRevert:
	encsetvar 0, 0     @ Momentum
	encsetvar 2, 0     @ Network
	encsetvar 7, 0     @ PlasmaArmed
	call EncScript_Zeraora_ChainReset    @ also clears the crit flag if the chain was live
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Zeraora_Spent_NoStats
EncScript_Zeraora_Spent_NoStats:
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	return
