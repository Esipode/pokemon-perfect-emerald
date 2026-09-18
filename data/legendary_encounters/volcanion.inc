// Volcanion, "The Steam Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Volcanion_Intro:
// 0 Phase (0 PRESSURE / 1 SUPERHEATED / 2 MELTDOWN / 3 SPENT), 1 Pressure (STEAM PRESSURE, 0-10),
// 2 Band (last ANNOUNCED band: 0 LOW / 1 HIGH / 2 CRITICAL), 3 Fields (live STEAM FIELDs, 0-2),
// 4 FieldTimer (turns until the standing field collapses), 5 SpentTimer (turns left in the
// post-release weakness window), 6 HpMark (boss HP% at turn start, then damage-taken-this-turn),
// 7 TurnGuard (0 start / 1 accumulate / 2 close), 8 BossMoveLatch, 9 WaterHitLatch,
// 10 InjectTimer (WATER INJECTION delayed spike countdown), 11 Releases.
//
// STEAM PRESSURE IS THE GUARD, IN ONE DIRECTION ONLY. EncScript_Volcanion_ApplyGuard is the damage
// reduction's only writer and reads SpentTimer, Phase and Band absolutely, so a band falling undoes
// a band rising with no separate teardown path. CRITICAL sits BELOW LOW on purpose - an overpressured
// hull is a strained hull, and it pays the player for pushing the meter up.
//
// EncScript_Volcanion_ApplySteamHeat is the single writer of Volcanion's Sp. Atk, written absolutely
// (push to +12, clamp at +6, then one delta) so a player Haze or a scour can't desync it from Fields
// and InjectTimer.
//
// EVERY OnMoveEnd EVENT TRIGGER CLOSES ITS OWN LATCH (BossMoveLatch, WaterHitLatch), cleared by
// _TurnClose - the runaway-script guard hangs on anything that doesn't.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction.
EncScript_Volcanion_ApplyGuard:
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Volcanion_ApplyGuard_Spent
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Volcanion_ApplyGuard_Meltdown
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Volcanion_ApplyGuard_Critical
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Volcanion_ApplyGuard_High
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Volcanion_ApplyGuard_Spent:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Volcanion_ApplyGuard_Meltdown:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Volcanion_ApplyGuard_Critical:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Volcanion_ApplyGuard_High:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return

// THE SINGLE WRITER of Volcanion's Sp. Atk stage. heat = Fields + (InjectTimer > 0 ? 1 : 0),
// capped at 2, written as stage 0 / +2 / +4. Push to +12 clamps at the +6 ceiling, then one drop
// lands on the stage that was meant - the Rayquaza idiom, immune to a Haze landing in between.
EncScript_Volcanion_ApplySteamHeat:
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encjumpifvar CMP_GREATER_THAN, 3, 1, EncScript_Volcanion_ApplySteamHeat_High
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Volcanion_ApplySteamHeat_Mid1
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Volcanion_ApplySteamHeat_Mid2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6     @ heat 0 -> stage 0
	return
EncScript_Volcanion_ApplySteamHeat_Mid1:
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Volcanion_ApplySteamHeat_High
EncScript_Volcanion_ApplySteamHeat_Mid2:
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -4     @ heat 1 -> stage 2
	return
EncScript_Volcanion_ApplySteamHeat_High:
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -2     @ heat 2 -> stage 4
	return

// Drops the standing STEAM FIELD(s): fog off, Sea of Fire off the player's side, Sp. Atk re-derived.
// Safe to call unconditionally from a phase transition or the finale.
EncScript_Volcanion_CollapseField:
	encsetvar 3, 0     @ Fields
	encsetvar 4, 0     @ FieldTimer
	removeweather
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	call EncScript_Volcanion_ApplySteamHeat
	printstring STRINGID_ENCVOLCANIONFIELDFADE
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger entry points ---

// No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Volcanion_Intro::
	printstring STRINGID_ENCVOLCANIONINTRO
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_BEAK_BLAST_SETUP
	call EncScript_Volcanion_ApplyGuard
	return

// Top of every turn. Leads with flushtextbox - the OnTurnStart BG0 gotcha. FieldTimer, SpentTimer
// and InjectTimer each tick here and resolve in the same procedural script (not via re-evaluated
// trigger conditions), so there is no chase-through-states risk. TurnGuard is set last so no branch
// out can leave it re-armed.
EncScript_Volcanion_TurnOpen::
	flushtextbox
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Volcanion_TurnOpen_Spent
	encsubvar 4, 1     @ FieldTimer
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Volcanion_TurnOpen_Spent
	call EncScript_Volcanion_CollapseField
EncScript_Volcanion_TurnOpen_Spent:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Volcanion_TurnOpen_Inject
	encsubvar 5, 1     @ SpentTimer
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Volcanion_TurnOpen_Inject
	printstring STRINGID_ENCVOLCANIONSPENTEND
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Volcanion_ApplyGuard
EncScript_Volcanion_TurnOpen_Inject:
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Volcanion_TurnOpen_Meltdown
	encsubvar 10, 1    @ InjectTimer
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Volcanion_TurnOpen_Meltdown
	encaddvar 1, 2     @ Pressure - the fuse flashes to steam
	printstring STRINGID_ENCVOLCANIONINJECTSPIKE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Volcanion_ApplySteamHeat
EncScript_Volcanion_TurnOpen_Meltdown:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Volcanion_TurnOpen_Hiss
	printstring STRINGID_ENCVOLCANIONMELTDOWNWARN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Volcanion_TurnOpen_Hiss:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Volcanion_TurnOpen_Done
	printstring STRINGID_ENCVOLCANIONHISS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Volcanion_TurnOpen_Done:
	encsetvar 7, 1     @ TurnGuard
	return

// THE LEDGER's first half: Pressure climbs by phase, plus +2 if the player took 4% or more off
// Volcanion this turn. HpMark doubles as the damage scratch; zeroed on a snapshot miss so a stale
// mark can never misread as a hit.
EncScript_Volcanion_Accumulate::
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Volcanion_Accumulate_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Volcanion_Accumulate_P1
	encaddvar 1, 1     @ Phase 0: Pressure +1
	goto EncScript_Volcanion_Accumulate_Damage
EncScript_Volcanion_Accumulate_P1:
	encaddvar 1, 2     @ Phase 1: Pressure +2
	goto EncScript_Volcanion_Accumulate_Damage
EncScript_Volcanion_Accumulate_P2:
	encaddvar 1, 3     @ Phase 2: Pressure +3
EncScript_Volcanion_Accumulate_Damage:
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_DAMAGE, EncScript_Volcanion_Accumulate_NoDamage
	goto EncScript_Volcanion_Accumulate_Measured
EncScript_Volcanion_Accumulate_NoDamage:
	encsetvar 6, 0
EncScript_Volcanion_Accumulate_Measured:
	encjumpifvar CMP_LESS_THAN, 6, 4, EncScript_Volcanion_Accumulate_Clamp
	encaddvar 1, 2     @ heavy-damage threshold (>=4% this turn)
EncScript_Volcanion_Accumulate_Clamp:
	encjumpifvar CMP_LESS_THAN, 1, 11, EncScript_Volcanion_Accumulate_Done
	encsetvar 1, 10
EncScript_Volcanion_Accumulate_Done:
	encsetvar 7, 2     @ TurnGuard
	return

// Volcanion lands a Fire or Water move. One script serves both the Fire and the Water OnMoveEnd
// trigger. Silent - the move's own battle text already covers the hit.
EncScript_Volcanion_BossHeat::
	encaddvar 1, 2     @ Pressure
	encsetvar 8, 1     @ BossMoveLatch
	return

// A Water move hits Volcanion: Water Absorb heals it (engine-handled) and this adds the Pressure
// spike and calls it out.
EncScript_Volcanion_Drink::
	encaddvar 1, 2     @ Pressure
	encsetvar 9, 1     @ WaterHitLatch
	printstring STRINGID_ENCVOLCANIONDRINK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_AQUA_RING_HEAL
	return

// THE BAND CALLOUT (guideline 14). Each writes Band and prints, then re-derives the guard.
EncScript_Volcanion_BandLow::
	encsetvar 2, 0     @ Band
	printstring STRINGID_ENCVOLCANIONPRESSURELOW
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Volcanion_ApplyGuard
	return

EncScript_Volcanion_BandHigh::
	encsetvar 2, 1     @ Band
	printstring STRINGID_ENCVOLCANIONPRESSUREHIGH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Volcanion_ApplyGuard
	return

EncScript_Volcanion_BandCritical::
	encsetvar 2, 2     @ Band
	printstring STRINGID_ENCVOLCANIONPRESSURECRITICAL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Volcanion_ApplyGuard
	return

// Volcanion vents: -3 Pressure (floored at 0), a new STEAM FIELD (fog both sides, Sea of Fire on
// the player), a typing-blind 8% scripted hit (closes the Fire-type hole in Sea of Fire), and the
// Sp. Atk it buys. Reused for both the first field and the SUPERHEATED STEAM stack.
EncScript_Volcanion_Vent::
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Volcanion_Vent_Floor
	encsubvar 1, 3     @ Pressure
	goto EncScript_Volcanion_Vent_Field
EncScript_Volcanion_Vent_Floor:
	encsetvar 1, 0
EncScript_Volcanion_Vent_Field:
	encaddvar 3, 1     @ Fields
	encsetvar 4, 4     @ FieldTimer
	encsetweather BATTLE_WEATHER_FOG
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 0
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Volcanion_Vent_First
	printstring STRINGID_ENCVOLCANIONSUPERHEATEDSTEAM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOG_CONTINUES
	goto EncScript_Volcanion_Vent_Hit
EncScript_Volcanion_Vent_First:
	printstring STRINGID_ENCVOLCANIONVENT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SEA_OF_FIRE
EncScript_Volcanion_Vent_Hit:
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	call EncScript_Volcanion_ApplySteamHeat
	return

// The fight's anti-abuse spine: refuses to let outside weather delete the fog while a field stands.
// encsetweather is a documented no-op when that weather is already up, so this costs nothing on the
// turns it isn't needed.
EncScript_Volcanion_Reassert::
	encsetweather BATTLE_WEATHER_FOG
	playanimation BS_OPPONENT1, B_ANIM_FOG_CONTINUES
	printstring STRINGID_ENCVOLCANIONREASSERT
	waitmessage B_WAIT_TIME_LONG
	return

// WATER INJECTION's opening half: draws Pressure now, arms the two-turn fuse _TurnOpen resolves.
EncScript_Volcanion_Inject::
	encaddvar 1, 3     @ Pressure
	encsetvar 10, 2    @ InjectTimer
	printstring STRINGID_ENCVOLCANIONINJECT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_AQUA_RING_HEAL
	call EncScript_Volcanion_ApplySteamHeat
	return

// PRESSURE RELEASE: the field blows away, Volcanion loses its next action, and its guard falls to
// 60 for three turns - the fight's real damage window.
EncScript_Volcanion_Release::
	encaddvar 11, 1    @ Releases
	encsetvar 1, 0     @ Pressure
	encsetvar 2, 0     @ Band
	call EncScript_Volcanion_CollapseField
	printstring STRINGID_ENCVOLCANIONRELEASE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encresetstats ENC_TARGET_BOSS, EncScript_Volcanion_Release_NoStats
EncScript_Volcanion_Release_NoStats:
	encchangehp ENC_TARGET_ALL_FOES, -25, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encsetrecharge ENC_TARGET_BOSS, 2
	encsetvar 5, 3     @ SpentTimer
	call EncScript_Volcanion_ApplyGuard
	call EncScript_Volcanion_ApplySteamHeat
	printstring STRINGID_ENCVOLCANIONSPENT
	waitmessage B_WAIT_TIME_LONG
	return

// --- Phase transitions ---

EncScript_Volcanion_Superheated::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCVOLCANIONPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	encaddvar 1, 2     @ Pressure
	call EncScript_Volcanion_ApplyGuard
	return

// Grants MOVE_STEAM_ERUPTION in Flame Charge's slot - the signature payload a phase unlocks
// (guideline 5). It is a Water move, so Volcanion using it pressurizes itself further.
EncScript_Volcanion_Meltdown::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCVOLCANIONPHASE3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetmove ENC_TARGET_BOSS, 3, MOVE_STEAM_ERUPTION
	printstring STRINGID_ENCVOLCANIONSTEAMERUPTION
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Volcanion_ApplyGuard
	return

// ERUPTION, and the catch window (guideline 10). Fires the moment Pressure hits 10 in Meltdown.
// Survive: True holds Volcanion at 1 HP until this runs, so the player cannot chip past it.
EncScript_Volcanion_Eruption::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCVOLCANIONERUPTION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	call EncScript_Volcanion_CollapseField
	encsetvar 1, 0     @ Pressure
	encsetvar 2, 0     @ Band
	encsetvar 5, 0     @ SpentTimer
	encsetvar 10, 0    @ InjectTimer
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Volcanion_Eruption_NoStats
EncScript_Volcanion_Eruption_NoStats:
	printstring STRINGID_ENCVOLCANIONFADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCVOLCANIONWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// --- Once-per-turn close ---

EncScript_Volcanion_TurnClose::
	encsetvar 8, 0     @ BossMoveLatch
	encsetvar 9, 0     @ WaterHitLatch
	encsetvar 7, 0     @ TurnGuard
	return
