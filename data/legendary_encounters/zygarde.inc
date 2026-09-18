// Zygarde, "The Order Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Zygarde_Awaken:
// 0 Phase (0 ORDER / 1 COMPLETE FORME / 2 THOUSAND-YEAR BALANCE / 3 SPENT), 1 Balance (0-10, 5
// neutral; above favours the player, below favours Zygarde), 2 Cells (0-5, each raises the
// enforcement band), 3 CellState (0 none live / 1 live, resolves next turn), 4 CellMark (boss HP%
// at Cell spawn), 5 TurnGuard, 6 Pattern (0 none / 1 boost / 2 heal / 3 switch / 4 setup),
// 7 Repeat (consecutive repeats of Pattern), 8 EventLatch, 9 Cycle (0 STAT / 1 LIFE / 2 FIELD),
// 10 AnnouncedBand (last band announced; 9 is the "nothing announced yet" sentinel), 11 HealMark
// (player HP% at turn open), 12 Scratch (band arithmetic, reused by the FIELD correction as
// "something was cleared"), 13 BalDone, 14 WrathUsed, 15 CoreCharge.
//
// BALANCE IS THE GUARD, IN BOTH DIRECTIONS. EncScript_Zygarde_ApplyBand is the damage reduction's
// only writer outside _LandsWrath and _Spent, and it short-circuits once Phase >= 2 - Land's Wrath
// owns the number from there.
//
// EVERY Event.*-REACTING TRIGGER (PatternBoost/PatternSetup/PatternSwitch) CLOSES EventLatch, and
// _TurnClose clears it - a script that returns without closing its own latch re-selects forever.
//
// STAT STAGES ARE WRITTEN ABSOLUTELY where it matters (encresetstats is the targeted Haze); the raw
// stat bump on Complete Forme uses encchangestatvalue so it survives every later encresetstats call.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase >= 2 returns immediately - Land's Wrath owns the
// number after that. Scratch (12) holds the Cells + Balance-band sum, clamped to 0-4, and is reused
// unchanged as the literal each Arm compares AnnouncedBand against - no var-to-var comparison exists,
// so each Arm tests AnnouncedBand against its own compile-time-known band instead.
// NOTE: AnnouncedBand starts at the sentinel 9, which is numerically above every real band, so the
// very first call always takes a "Fell" branch regardless of the true starting band. This is the
// harmless cost of a rise/fall-only callout (guideline 14) rather than per-band text; the AWAKEN
// line already carries the turn-0 narrative.
EncScript_Zygarde_ApplyBand:
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Zygarde_ApplyBand_Done
	enccopyvar 12, 2                                          @ Scratch = Cells
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Zygarde_ApplyBand_Bal0
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zygarde_ApplyBand_Bal1
	encjumpifvar CMP_LESS_THAN, 1, 7, EncScript_Zygarde_ApplyBand_Bal2
	encjumpifvar CMP_LESS_THAN, 1, 9, EncScript_Zygarde_ApplyBand_Bal3
	encaddvar 12, 4
	goto EncScript_Zygarde_ApplyBand_Clamp
EncScript_Zygarde_ApplyBand_Bal0:
	goto EncScript_Zygarde_ApplyBand_Clamp
EncScript_Zygarde_ApplyBand_Bal1:
	encaddvar 12, 1
	goto EncScript_Zygarde_ApplyBand_Clamp
EncScript_Zygarde_ApplyBand_Bal2:
	encaddvar 12, 2
	goto EncScript_Zygarde_ApplyBand_Clamp
EncScript_Zygarde_ApplyBand_Bal3:
	encaddvar 12, 3
EncScript_Zygarde_ApplyBand_Clamp:
	encjumpifvar CMP_LESS_THAN, 12, 5, EncScript_Zygarde_ApplyBand_Ladder
	encsetvar 12, 4
EncScript_Zygarde_ApplyBand_Ladder:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Zygarde_ApplyBand_Arm0
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Zygarde_ApplyBand_Arm1
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Zygarde_ApplyBand_Arm2
	encjumpifvar CMP_EQUAL, 12, 3, EncScript_Zygarde_ApplyBand_Arm3
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encjumpifvar CMP_EQUAL, 10, 4, EncScript_Zygarde_ApplyBand_Done
	encjumpifvar CMP_LESS_THAN, 10, 4, EncScript_Zygarde_ApplyBand_Arm4Rose
	encsetvar 10, 4
	goto EncScript_Zygarde_ApplyBand_Fell
EncScript_Zygarde_ApplyBand_Arm4Rose:
	encsetvar 10, 4
	goto EncScript_Zygarde_ApplyBand_Rose
EncScript_Zygarde_ApplyBand_Arm0:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Zygarde_ApplyBand_Done
	encjumpifvar CMP_LESS_THAN, 10, 0, EncScript_Zygarde_ApplyBand_Arm0Rose
	encsetvar 10, 0
	goto EncScript_Zygarde_ApplyBand_Fell
EncScript_Zygarde_ApplyBand_Arm0Rose:
	encsetvar 10, 0
	goto EncScript_Zygarde_ApplyBand_Rose
EncScript_Zygarde_ApplyBand_Arm1:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Zygarde_ApplyBand_Done
	encjumpifvar CMP_LESS_THAN, 10, 1, EncScript_Zygarde_ApplyBand_Arm1Rose
	encsetvar 10, 1
	goto EncScript_Zygarde_ApplyBand_Fell
EncScript_Zygarde_ApplyBand_Arm1Rose:
	encsetvar 10, 1
	goto EncScript_Zygarde_ApplyBand_Rose
EncScript_Zygarde_ApplyBand_Arm2:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Zygarde_ApplyBand_Done
	encjumpifvar CMP_LESS_THAN, 10, 2, EncScript_Zygarde_ApplyBand_Arm2Rose
	encsetvar 10, 2
	goto EncScript_Zygarde_ApplyBand_Fell
EncScript_Zygarde_ApplyBand_Arm2Rose:
	encsetvar 10, 2
	goto EncScript_Zygarde_ApplyBand_Rose
EncScript_Zygarde_ApplyBand_Arm3:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encjumpifvar CMP_EQUAL, 10, 3, EncScript_Zygarde_ApplyBand_Done
	encjumpifvar CMP_LESS_THAN, 10, 3, EncScript_Zygarde_ApplyBand_Arm3Rose
	encsetvar 10, 3
	goto EncScript_Zygarde_ApplyBand_Fell
EncScript_Zygarde_ApplyBand_Arm3Rose:
	encsetvar 10, 3
EncScript_Zygarde_ApplyBand_Rose:
	printstring STRINGID_ENCZYGARDEBANDGUARD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return
EncScript_Zygarde_ApplyBand_Fell:
	printstring STRINGID_ENCZYGARDEBANDYIELD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
EncScript_Zygarde_ApplyBand_Done:
	return

// THE ROTATING CORRECTION. Cycle (9) advances STAT -> LIFE -> FIELD regardless of which arm ran,
// wrapping 2 -> 0. Each arm targets the leader (guideline 8's "corrects whoever is ahead").
EncScript_Zygarde_Correct:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Zygarde_Correct_Stat
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Zygarde_Correct_Life
	goto EncScript_Zygarde_Correct_Field
EncScript_Zygarde_Correct_Stat:
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Zygarde_Correct_StatExcess
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zygarde_Correct_StatExcessSelf
	goto EncScript_Zygarde_Correct_Advance
EncScript_Zygarde_Correct_StatExcess:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Zygarde_Correct_Advance
	printstring STRINGID_ENCZYGARDEEXCESS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	goto EncScript_Zygarde_Correct_Advance
EncScript_Zygarde_Correct_StatExcessSelf:
	encresetstats ENC_TARGET_BOSS, EncScript_Zygarde_Correct_Advance
	printstring STRINGID_ENCZYGARDEEXCESSSELF
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	goto EncScript_Zygarde_Correct_Advance
EncScript_Zygarde_Correct_Life:
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Zygarde_Correct_LifeDraw
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zygarde_Correct_Scarcity
	goto EncScript_Zygarde_Correct_Advance
EncScript_Zygarde_Correct_LifeDraw:
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCZYGARDELIFEDRAW
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	goto EncScript_Zygarde_Correct_Advance
EncScript_Zygarde_Correct_Scarcity:
	encchangehp ENC_TARGET_ALL_FOES, 2, ENC_AMOUNT_PERCENT
	encrevive ENC_TARGET_ALL_FOES, 30
	printstring STRINGID_ENCZYGARDESCARCITY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_WISH_HEAL
	goto EncScript_Zygarde_Correct_Advance
// FIELD arm reaches weather/terrain through ordinary battle-script opcodes - there is no
// encounter-script condition read for them, only the encounter-specific clear commands for
// hazards/screens. jumpifhalfword on gBattleWeather rather than jumpifweatheraffected: the latter
// reads gBattlerAttacker, which is stale at a checkpoint. Scratch (12) accumulates "something was
// cleared/present"; nothing set means STAGNATION.
EncScript_Zygarde_Correct_Field:
	encsetvar 12, 0
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_Correct_FieldScreens
	encsetvar 12, 1
EncScript_Zygarde_Correct_FieldScreens:
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_Correct_FieldWeather
	encsetvar 12, 1
EncScript_Zygarde_Correct_FieldWeather:
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_ANY, EncScript_Zygarde_Correct_FieldTerrain
	goto EncScript_Zygarde_Correct_FieldWeatherFound
EncScript_Zygarde_Correct_FieldWeatherFound:
	encsetvar 12, 1
EncScript_Zygarde_Correct_FieldTerrain:
	jumpifterrain CMP_NOT_EQUAL, B_TERRAIN_NONE, EncScript_Zygarde_Correct_FieldTerrainFound
	goto EncScript_Zygarde_Correct_FieldCheck
EncScript_Zygarde_Correct_FieldTerrainFound:
	encsetvar 12, 1
EncScript_Zygarde_Correct_FieldCheck:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Zygarde_Correct_Stagnation
	removeweather
	removeterrain
	printstring STRINGID_ENCZYGARDECHAOS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	goto EncScript_Zygarde_Correct_Advance
EncScript_Zygarde_Correct_Stagnation:
	encjumpifchance 50, EncScript_Zygarde_Correct_StagnationTerrain
	encsetfieldstatus ENC_FIELD_TRICK_ROOM, 5
	goto EncScript_Zygarde_Correct_StagnationPrint
EncScript_Zygarde_Correct_StagnationTerrain:
	encsetterrain ENC_TERRAIN_GRASSY, 5
EncScript_Zygarde_Correct_StagnationPrint:
	printstring STRINGID_ENCZYGARDESTAGNATION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
EncScript_Zygarde_Correct_Advance:
	encaddvar 9, 1
	encjumpifvar CMP_LESS_THAN, 9, 3, EncScript_Zygarde_Correct_Done
	encsetvar 9, 0
EncScript_Zygarde_Correct_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. Balance opens neutral; AnnouncedBand opens at the sentinel 9 so the first
// _ApplyBand call always announces. No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Zygarde_Awaken::
	encsetvar 0, 0     @ Phase
	encsetvar 1, 5     @ Balance
	encsetvar 2, 0     @ Cells
	encsetvar 3, 0     @ CellState
	encsetvar 4, 0     @ CellMark
	encsetvar 5, 0     @ TurnGuard
	encsetvar 6, 0     @ Pattern
	encsetvar 7, 0     @ Repeat
	encsetvar 8, 0     @ EventLatch
	encsetvar 9, 0     @ Cycle
	encsetvar 10, 9    @ AnnouncedBand: nothing announced yet
	encsetvar 11, 0    @ HealMark
	encsetvar 12, 0    @ Scratch
	encsetvar 13, 0    @ BalDone
	encsetvar 14, 0    @ WrathUsed
	encsetvar 15, 0    @ CoreCharge
	printstring STRINGID_ENCZYGARDEAWAKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Zygarde_ApplyBand
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch out can leave it re-armed.
EncScript_Zygarde_TurnOpen::
	flushtextbox
	encsetvar 5, 1                                             @ TurnGuard
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 11                   @ HealMark
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Zygarde_TurnOpen_Spawn
// Resolve a live Cell: damage dealt since the mark, measured against a 3% threshold.
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_DAMAGE
	encjumpifvar CMP_LESS_THAN, 4, 3, EncScript_Zygarde_TurnOpen_CellAbsorb
	printstring STRINGID_ENCZYGARDECELLBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	goto EncScript_Zygarde_TurnOpen_CellDone
EncScript_Zygarde_TurnOpen_CellAbsorb:
	printstring STRINGID_ENCZYGARDECELLABSORB
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Zygarde_TurnOpen_CellDone
	encaddvar 2, 1                                             @ Cells, capped at 5
EncScript_Zygarde_TurnOpen_CellDone:
	encsetvar 3, 0                                             @ CellState
// Spawn a Cell: coin flip in Phase 0, guaranteed in Phase 1, none from Phase 2 on.
EncScript_Zygarde_TurnOpen_Spawn:
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Zygarde_TurnOpen_BalanceLine
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Zygarde_TurnOpen_BalanceLine
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Zygarde_TurnOpen_DoSpawn
	encjumpifchance 50, EncScript_Zygarde_TurnOpen_DoSpawn
	goto EncScript_Zygarde_TurnOpen_BalanceLine
EncScript_Zygarde_TurnOpen_DoSpawn:
	encsetvar 3, 1
	encsnapshothp ENC_TARGET_BOSS, 4                           @ CellMark
	printstring STRINGID_ENCZYGARDECELLFORM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
// The recurring Balance line (guideline 14). Silent in the 4-6 neutral band.
EncScript_Zygarde_TurnOpen_BalanceLine:
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Zygarde_TurnOpen_TipPlayer
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zygarde_TurnOpen_TipBoss
	goto EncScript_Zygarde_TurnOpen_Correction
EncScript_Zygarde_TurnOpen_TipPlayer:
	printstring STRINGID_ENCZYGARDEBALANCETIP
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Zygarde_TurnOpen_Correction
EncScript_Zygarde_TurnOpen_TipBoss:
	printstring STRINGID_ENCZYGARDEBALANCETIPBOSS
	waitmessage B_WAIT_TIME_SHORT
// The rotating correction. Threshold is 2 steps off neutral in Phase 0, 1 step from Phase 1 on.
EncScript_Zygarde_TurnOpen_Correction:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Zygarde_TurnOpen_CorrectionP1
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Zygarde_TurnOpen_DoCorrect
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Zygarde_TurnOpen_DoCorrect
	goto EncScript_Zygarde_TurnOpen_Charge
EncScript_Zygarde_TurnOpen_CorrectionP1:
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Zygarde_TurnOpen_DoCorrect
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zygarde_TurnOpen_DoCorrect
	goto EncScript_Zygarde_TurnOpen_Charge
EncScript_Zygarde_TurnOpen_DoCorrect:
	call EncScript_Zygarde_Correct
EncScript_Zygarde_TurnOpen_Charge:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Zygarde_TurnOpen_ApplyBand
	encaddvar 15, 1                                            @ CoreCharge, Phase 1 only
EncScript_Zygarde_TurnOpen_ApplyBand:
	call EncScript_Zygarde_ApplyBand
	return

// CORE PUNISHMENT discriminators. Each is a two-line test: does Pattern already hold this id?
// Same id -> Repeat++. Different id -> Pattern reassigned, Repeat reset to 1.
EncScript_Zygarde_PatternBoost::
	encsetvar 8, 1                                             @ EventLatch
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Zygarde_PatternBoost_Same
	encsetvar 6, 1                                             @ Pattern = boost
	encsetvar 7, 1                                             @ Repeat = 1
	return
EncScript_Zygarde_PatternBoost_Same:
	encaddvar 7, 1
	return

EncScript_Zygarde_PatternSetup::
	encsetvar 8, 1
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Zygarde_PatternSetup_Same
	encsetvar 6, 4                                             @ Pattern = setup
	encsetvar 7, 1
	return
EncScript_Zygarde_PatternSetup_Same:
	encaddvar 7, 1
	return

EncScript_Zygarde_PatternSwitch::
	encsetvar 8, 1
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zygarde_PatternSwitch_Same
	encsetvar 6, 3                                             @ Pattern = switch
	encsetvar 7, 1
	return
EncScript_Zygarde_PatternSwitch_Same:
	encaddvar 7, 1
	return

// THE BALANCE AUDIT. Five entry points, exactly one selected per turn (each requires BalDone == 0
// and sets it). Silent - the recurring line at _TurnOpen already narrates the meter.
EncScript_Zygarde_BalanceTipPlayerHp::
	encaddvar 1, 1
	encsetvar 13, 1                                            @ BalDone
	return

EncScript_Zygarde_BalanceTipPlayerStat::
	encaddvar 1, 1
	encsetvar 13, 1
	return

EncScript_Zygarde_BalanceTipBossHp::
	encsubvar 1, 1
	encsetvar 13, 1
	return

EncScript_Zygarde_BalanceTipBossStat::
	encsubvar 1, 1
	encsetvar 13, 1
	return

EncScript_Zygarde_BalanceHold::
	encsetvar 13, 1
	return

// CORE ENFORCER, Phase 1 only. Fires on the leader: player ahead loses 12% and a move for 4 turns,
// Zygarde ahead loses 8% and one of its own Cells, dead even strips every stage on the field.
EncScript_Zygarde_CoreEnforcer::
	encsetvar 15, 0                                            @ CoreCharge - re-arms itself off
	printstring STRINGID_ENCZYGARDECOREENFORCER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Zygarde_CoreEnforcer_Player
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Zygarde_CoreEnforcer_Boss
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_CoreEnforcer_Apply
	goto EncScript_Zygarde_CoreEnforcer_Apply
EncScript_Zygarde_CoreEnforcer_Player:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Zygarde_CoreEnforcer_PlayerHp
EncScript_Zygarde_CoreEnforcer_PlayerHp:
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	encdisablemove ENC_TARGET_PLAYER_LEFT, 4, EncScript_Zygarde_CoreEnforcer_Apply
	goto EncScript_Zygarde_CoreEnforcer_Apply
EncScript_Zygarde_CoreEnforcer_Boss:
	encresetstats ENC_TARGET_BOSS, EncScript_Zygarde_CoreEnforcer_BossHp
EncScript_Zygarde_CoreEnforcer_BossHp:
	encchangehp ENC_TARGET_BOSS, -12, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Zygarde_CoreEnforcer_Apply
	encsubvar 2, 1                                             @ Cells: enforces against itself
EncScript_Zygarde_CoreEnforcer_Apply:
	call EncScript_Zygarde_ApplyBand
	return

// THE LEDGER. Heal-spam detection (the fourth pattern, needing the end-of-turn recovery read),
// Core Punishment, then the band - the Balance step taken by the audit triggers has to reach the
// reduction before the turn ends. The three clears are last so TurnGuard going to 0 self-disables.
EncScript_Zygarde_TurnClose::
	encjumpifvar CMP_NOT_EQUAL, 8, 0, EncScript_Zygarde_TurnClose_Punish
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 11, ENC_SNAP_RECOVERY
	encjumpifvar CMP_LESS_THAN, 11, 20, EncScript_Zygarde_TurnClose_Punish
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Zygarde_TurnClose_HealSame
	encsetvar 6, 2                                             @ Pattern = heal
	encsetvar 7, 1
	goto EncScript_Zygarde_TurnClose_HealLatch
EncScript_Zygarde_TurnClose_HealSame:
	encaddvar 7, 1
EncScript_Zygarde_TurnClose_HealLatch:
	encsetvar 8, 1                                             @ EventLatch
EncScript_Zygarde_TurnClose_Punish:
	encjumpifvar CMP_LESS_THAN, 7, 3, EncScript_Zygarde_TurnClose_Balance
	call EncScript_Zygarde_Punish
EncScript_Zygarde_TurnClose_Balance:
	call EncScript_Zygarde_ApplyBand
	encsetvar 13, 0                                            @ BalDone
	encsetvar 8, 0                                             @ EventLatch
	encsetvar 5, 0                                             @ TurnGuard
	return

// CORE PUNISHMENT payload. Shuts off exactly the pattern the player leaned on for 3 turns running.
EncScript_Zygarde_Punish:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Zygarde_Punish_Boost
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Zygarde_Punish_Heal
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zygarde_Punish_Switch
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Zygarde_Punish_Setup
	goto EncScript_Zygarde_Punish_Clear
EncScript_Zygarde_Punish_Boost:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Zygarde_Punish_Clear
	printstring STRINGID_ENCZYGARDEENFORCEBOOST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	goto EncScript_Zygarde_Punish_Clear
EncScript_Zygarde_Punish_Heal:
	encsethealblock ENC_TARGET_ALL_FOES, 5
	printstring STRINGID_ENCZYGARDEENFORCEHEAL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MAGIC_ROOM
	goto EncScript_Zygarde_Punish_Clear
EncScript_Zygarde_Punish_Switch:
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCZYGARDEENFORCESWITCH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	goto EncScript_Zygarde_Punish_Clear
EncScript_Zygarde_Punish_Setup:
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_Punish_SetupHazards
EncScript_Zygarde_Punish_SetupHazards:
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_Punish_SetupMsg
EncScript_Zygarde_Punish_SetupMsg:
	printstring STRINGID_ENCZYGARDEENFORCESETUP
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
EncScript_Zygarde_Punish_Clear:
	encsetvar 6, 0                                             @ Pattern
	encsetvar 7, 0                                             @ Repeat
	return

// --- Phase transitions ---

// 50% - COMPLETE FORME. encformchange keeps HP and moveset; stats/types/ability come from the new
// species. encsetmove installs Core Enforcer over Glare. The raw stat bump uses encchangestatvalue
// so a later encresetstats can't strip the form change itself.
EncScript_Zygarde_CompleteForme::
	encsetvar 0, 1                                             @ Phase
	printstring STRINGID_ENCZYGARDECOMPLETE
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_ZYGARDE_COMPLETE, EncScript_Zygarde_CompleteForme_Move, B_ANIM_POWER_CONSTRUCT
EncScript_Zygarde_CompleteForme_Move:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_CORE_ENFORCER
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 15, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 15, ENC_AMOUNT_PERCENT
	encsetvar 15, 0                                            @ CoreCharge
	call EncScript_Zygarde_ApplyBand
	return

// 20% - LAND'S WRATH. Total reset: the field, both sides' restrictions, and Zygarde's own power
// all go together. _ApplyBand short-circuits on Phase >= 2, so the flat 60 stands for the rest of
// the fight and nothing here can be re-raised behind it.
EncScript_Zygarde_LandsWrath::
	encsetvar 0, 2                                             @ Phase
	encsetvar 14, 1                                            @ WrathUsed
	printstring STRINGID_ENCZYGARDELANDSWRATH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_LandsWrath_Hazards
EncScript_Zygarde_LandsWrath_Hazards:
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_LandsWrath_Screens
EncScript_Zygarde_LandsWrath_Screens:
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_LandsWrath_Field
EncScript_Zygarde_LandsWrath_Field:
	removeweather
	removeterrain
	encclearfieldstatus ENC_FIELD_TRICK_ROOM
	encclearfieldstatus ENC_FIELD_GRAVITY
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encsetembargo ENC_TARGET_ALL_FOES, 0
	encsetvar 2, 0                                             @ Cells
	encsetvar 1, 5                                             @ Balance neutral
	encsetvar 6, 0                                             @ Pattern
	encsetvar 7, 0                                             @ Repeat
	encsetdamagereduction ENC_TARGET_BOSS, 60
	encsetvar 10, 9                                            @ AnnouncedBand sentinel
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return

// 10% - SPENT, the catch window (guideline 10). The teardown mirrors the Properties: block plus
// everything Phase 1/2 raised, so nothing the fight imposed survives into the window.
EncScript_Zygarde_Spent::
	encsetvar 0, 3                                             @ Phase
	printstring STRINGID_ENCZYGARDEFADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0                                             @ Balance
	encsetvar 2, 0                                             @ Cells
	encsetvar 3, 0                                             @ CellState
	encsetvar 6, 0                                             @ Pattern
	encsetvar 7, 0                                             @ Repeat
	encsetvar 9, 0                                             @ Cycle
	encsetvar 15, 0                                            @ CoreCharge
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsethealblock ENC_TARGET_ALL_FOES, 0
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Zygarde_Spent_NoStats
EncScript_Zygarde_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCZYGARDEWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
