// Xerneas, "The Life Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Xerneas_Intro:
// 0 Phase (0 Giving Tree / 1 Forest Awakens / 2 Life Eternal / 3 Spent), 1 Life (LIFE ENERGY, 0-10),
// 2 LastBand (the last ANNOUNCED band; 9 = nothing said yet), 3 TurnGuard, 4 BossMark (Xerneas HP%
// at turn open), 5 Scratch (the copy ENC_SNAP_DAMAGE consumes, then reused as the in-script HP
// probe), 6 Blooms (LIFE BLOOM count, 0-4), 7 Gain (life earned this turn, applied at the ledger),
// 8 Active (0 Neutral / 1 Active Mode taken in Phase 0-1; reused in Phase 2 as the ImmortalityCycle
// countdown - 6-10 ON, 1-5 OFF, see EncScript_Xerneas_ImmortalityCycle), 9 BlessTick (0 nobody
// blessed / 1-3 turns left),
// 10 Charge (GEOMANCY: 0 none / 1-3 turns left), 11 LifeGuard, 12 GardenGuard, 13 CueTick,
// 14 Held (the 25% floor already held this turn), 15 BlessCool.
//
// LIFE ENERGY IS THE GUARD, IN BOTH DIRECTIONS. EncScript_Xerneas_ApplyGuard is the damage
// reduction's only writer outside _Spent and reads Phase, Life and Charge absolutely, so a band
// falling undoes a band rising with no separate teardown path.
//
// EVERY Event.*-REACTING TRIGGER IS SELF-DISABLING through a latch its own script writes:
// _Gardener sets GardenGuard, _LifeSurge sets LifeGuard, _BlessingReturns/_BlessingLeaves clear
// BlessTick. _TurnOpen clears GardenGuard/LifeGuard/Held every turn.
//
// LIFE BLOOMS are real side statuses on Xerneas's own side (ENC_TARGET_ALL_ALLIES). ApplyGarden is
// the single absolute writer, so it is safe to call unconditionally after a real Defog/Brick Break
// strips the statuses out from under the script - the board is correct the instant the move
// resolves rather than a turn later.
//
// STAT STAGES ARE WRITTEN ABSOLUTELY: +12 clamps to the ceiling, then a single drop lands on the
// stage that was meant. That is what makes GEOMANCY's boosts and LIFE BLESSING's Def/SpDef survive
// a player Haze, a switch, or a scour landing in between.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase 3 returns immediately: once balls are allowed the
// engine's own catch guard owns the number. Charging costs Xerneas 6 points of cover in every phase
// it can charge in - concentration is the fight's cleanest damage window.
EncScript_Xerneas_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Xerneas_ApplyGuard_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Xerneas_ApplyGuard_P2
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Xerneas_ApplyGuard_Charging
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Xerneas_ApplyGuard_84
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Xerneas_ApplyGuard_88
	encjumpifvar CMP_LESS_THAN, 1, 9, EncScript_Xerneas_ApplyGuard_91
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Xerneas_ApplyGuard_84:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Xerneas_ApplyGuard_88:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Xerneas_ApplyGuard_91:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Xerneas_ApplyGuard_Charging:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Xerneas_ApplyGuard_78
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Xerneas_ApplyGuard_82
	encjumpifvar CMP_LESS_THAN, 1, 9, EncScript_Xerneas_ApplyGuard_85
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Xerneas_ApplyGuard_78:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Xerneas_ApplyGuard_82:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Xerneas_ApplyGuard_85:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
// Phase 2: it has stopped defending on a curve. Flat 90 - the 25% floor is the guard in this phase.
EncScript_Xerneas_ApplyGuard_P2:
	encsetdamagereduction ENC_TARGET_BOSS, 84
EncScript_Xerneas_ApplyGuard_Done:
	return

// THE BAND CALLOUT (guideline 14). LastBand holds the last ANNOUNCED band; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a change - in BOTH directions. The falling lines are as load-bearing as the rising ones: in Phase
// 2 a fall is the player's own work and the fight has no other way to confirm it.
EncScript_Xerneas_Band:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Xerneas_Band_Withering
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Xerneas_Band_Blooming
	encjumpifvar CMP_LESS_THAN, 1, 9, EncScript_Xerneas_Band_Flourishing
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Xerneas_Band_Done
	encsetvar 2, 3
	printstring STRINGID_ENCXERNEASBANDETERNAL
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Xerneas_Band_Withering:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Xerneas_Band_Done
	encsetvar 2, 0
	printstring STRINGID_ENCXERNEASBANDWITHERING
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Xerneas_Band_Blooming:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Xerneas_Band_Done
	encsetvar 2, 1
	printstring STRINGID_ENCXERNEASBANDBLOOMING
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Xerneas_Band_Flourishing:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Xerneas_Band_Done
	encsetvar 2, 2
	printstring STRINGID_ENCXERNEASBANDFLOURISHING
	waitmessage B_WAIT_TIME_LONG
EncScript_Xerneas_Band_Done:
	return

// THE SINGLE ABSOLUTE WRITER of the LIFE BLOOMS. Given Blooms = n, raises blooms 1..n and clears
// n+1..4 on Xerneas's own side. Silent - every caller supplies its own line, because this is also
// what repairs the board right after a real Defog/Brick Break/etc. strips it.
EncScript_Xerneas_ApplyGarden:
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Xerneas_ApplyGarden_Mist
	encclearsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_MIST
	goto EncScript_Xerneas_ApplyGarden_Chant
EncScript_Xerneas_ApplyGarden_Mist:
	encsetsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_MIST, 0
EncScript_Xerneas_ApplyGarden_Chant:
	encjumpifvar CMP_GREATER_THAN, 6, 1, EncScript_Xerneas_ApplyGarden_LuckyChant
	encclearsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_LUCKY_CHANT
	goto EncScript_Xerneas_ApplyGarden_Safeguard
EncScript_Xerneas_ApplyGarden_LuckyChant:
	encsetsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_LUCKY_CHANT, 0
EncScript_Xerneas_ApplyGarden_Safeguard:
	encjumpifvar CMP_GREATER_THAN, 6, 2, EncScript_Xerneas_ApplyGarden_SafeguardSet
	encclearsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_SAFEGUARD
	goto EncScript_Xerneas_ApplyGarden_Veil
EncScript_Xerneas_ApplyGarden_SafeguardSet:
	encsetsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_SAFEGUARD, 0
EncScript_Xerneas_ApplyGarden_Veil:
	encjumpifvar CMP_GREATER_THAN, 6, 3, EncScript_Xerneas_ApplyGarden_VeilSet
	encclearsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_AURORA_VEIL
	return
EncScript_Xerneas_ApplyGarden_VeilSet:
	encsetsidestatus ENC_TARGET_ALL_ALLIES, ENC_SIDE_AURORA_VEIL, 0
	return

// The _Spend family. Four tiny labels rather than a loop: encsubvar on a u8 would wrap below zero,
// and there is no clamp opcode. Callers print their own line.
EncScript_Xerneas_Spend1:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Xerneas_Spend1_Done
	encsubvar 1, 1
EncScript_Xerneas_Spend1_Done:
	return
EncScript_Xerneas_Spend2:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Xerneas_Spend2_Done
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Xerneas_Spend2_Floor
	encsubvar 1, 2
	return
EncScript_Xerneas_Spend2_Floor:
	encsetvar 1, 0
EncScript_Xerneas_Spend2_Done:
	return
EncScript_Xerneas_Spend4:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Xerneas_Spend4_Done
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Xerneas_Spend4_Floor
	encsubvar 1, 4
	return
EncScript_Xerneas_Spend4_Floor:
	encsetvar 1, 0
EncScript_Xerneas_Spend4_Done:
	return

// GEOMANCY resolves. First eruption only: encformchange into Active Mode. Every eruption after
// that boosts SpAtk/SpDef/Speed absolutely, scours the player's stages, and fires a payload scaled
// by the live meter - Substitute never sees it, it's a direct HP write.
EncScript_Xerneas_GeomancyFires:
	printstring STRINGID_ENCXERNEASGEOMANCYFIRES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Xerneas_GeomancyFires_Stats
	encformchange ENC_TARGET_BOSS, SPECIES_XERNEAS_ACTIVE, EncScript_Xerneas_GeomancyFires_Stats, B_ANIM_FORM_CHANGE
	printstring STRINGID_ENCXERNEASACTIVEMODE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 8, 1     @ Active
EncScript_Xerneas_GeomancyFires_Stats:
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -8
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -8
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -8
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Xerneas_GeomancyFires_Payload
EncScript_Xerneas_GeomancyFires_Payload:
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Xerneas_GeomancyFires_12
	encjumpifvar CMP_LESS_THAN, 1, 9, EncScript_Xerneas_GeomancyFires_16
	encchangehp ENC_TARGET_PLAYER_LEFT, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Xerneas_GeomancyFires_Done
EncScript_Xerneas_GeomancyFires_16:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Xerneas_GeomancyFires_Done
EncScript_Xerneas_GeomancyFires_12:
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
EncScript_Xerneas_GeomancyFires_Done:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encsetvar 10, 0    @ Charge: it can gather again once the meter climbs back to 6
	call EncScript_Xerneas_ApplyGuard
	return

// GEOMANCY breaks. 12% or more off Xerneas in a charge turn wastes 2 Life Energy and nothing fires.
EncScript_Xerneas_GeomancyBreaks:
	printstring STRINGID_ENCXERNEASGEOMANCYBREAKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetvar 10, 0    @ Charge
	call EncScript_Xerneas_Spend2
	call EncScript_Xerneas_ApplyGuard
	return

// THE IMMORTALITY CYCLE. Var8 (Active) is dead weight once Phase 2 starts - GEOMANCY never fires
// again - so it is reused as a countdown: 6-10 is ON with (value-5) turns left, 1-5 is OFF with
// (value) turns left. "Nothing dies" cannot just sit there for the rest of the fight; it has to lift
// on a timer so the player gets real turns to finish Xerneas off, then reasserts itself.
EncScript_Xerneas_ImmortalityCycle:
	encsubvar 8, 1
	encjumpifvar CMP_EQUAL, 8, 5, EncScript_Xerneas_ImmortalityCycle_Off
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Xerneas_ImmortalityCycle_On
	return
EncScript_Xerneas_ImmortalityCycle_Off:
	encsetsurvive ENC_TARGET_ALL_FOES, FALSE
	printstring STRINGID_ENCXERNEASGUARDDROPS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Xerneas_ImmortalityCycle_On:
	encsetvar 8, 10
	encsetsurvive ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCXERNEASLIFERETURNS
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger entry points ---

// OnBattleStart. _ApplyGuard runs on turn 0 so the guard already reflects the table rather than the
// Properties: seed, and LastBand starts at 9 so the opening band line is a real announcement.
// No trainerslidein - this is a wild Pokemon (guideline 13).
EncScript_Xerneas_Intro::
	printstring STRINGID_ENCXERNEASINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCXERNEASINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 1, 2     @ Life
	encsetvar 2, 9     @ LastBand: nothing announced yet
	call EncScript_Xerneas_ApplyGuard
	call EncScript_Xerneas_Band
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before any
// dialogue renders on top of the still-open action menu (the BG0 gotcha). TurnGuard is set
// immediately so no branch out can leave it re-armed.
EncScript_Xerneas_TurnOpen::
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET   @ BossMark
	encsetvar 11, 0    @ LifeGuard
	encsetvar 12, 0    @ GardenGuard
	encsetvar 14, 0    @ Held
	encjumpifvar CMP_EQUAL, 15, 0, EncScript_Xerneas_TurnOpen_Charge
	encsubvar 15, 1    @ BlessCool
EncScript_Xerneas_TurnOpen_Charge:
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Xerneas_TurnOpen_Cue
	encsubvar 10, 1    @ Charge
	printstring STRINGID_ENCXERNEASGEOMANCYCUE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Xerneas_TurnOpen_Cue:
	call EncScript_Xerneas_Band
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Xerneas_TurnOpen_CueA
	encsetvar 13, 0
	printstring STRINGID_ENCXERNEASCUEB
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Xerneas_TurnOpen_CueA:
	encsetvar 13, 1
	printstring STRINGID_ENCXERNEASCUEA
	waitmessage B_WAIT_TIME_SHORT
	return

// LIFE BLESSING. A genuine gift and a trap: fainting under it returns life, switching out abandons
// it, surviving it gives Xerneas nothing back. Self-disabling through BlessTick going non-zero.
EncScript_Xerneas_Bless::
	printstring STRINGID_ENCXERNEASBLESSING
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Xerneas_Spend2
	encchangehp ENC_TARGET_PLAYER_LEFT, 15, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	curestatus BS_PLAYER1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_DEF, -5
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -5
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	encsetvar 9, 3     @ BlessTick
	return

// THE GEOMANCY TELEGRAPH, announced before the player picks a move so the turn can be spent on the
// answer. Charging costs 6 points of cover the whole time - the cleanest damage window in the fight.
EncScript_Xerneas_BeginGeomancy::
	printstring STRINGID_ENCXERNEASGEOMANCYBEGINS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encsetvar 10, 3    @ Charge
	call EncScript_Xerneas_ApplyGuard
	return

// A LIFE BLOOM is shattered by a named move. Its energy returns to Xerneas (+1), and the board is
// re-applied immediately so it reflects the new count rather than whatever the move's own effect did.
EncScript_Xerneas_Gardener::
	printstring STRINGID_ENCXERNEASBLOOMSHATTERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	encsubvar 6, 1     @ Blooms
	call EncScript_Xerneas_ApplyGarden
	encaddvar 7, 1     @ Gain
	encsetvar 12, 1    @ GardenGuard
	return

// A Fairy or Grass move resolved on either side - life stirs, and Xerneas drinks it in.
EncScript_Xerneas_LifeSurge::
	printstring STRINGID_ENCXERNEASLIFESURGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	encaddvar 7, 1     @ Gain
	encsetvar 11, 1    @ LifeGuard
	return

// The blessed Pokemon fainted - its life returns to Xerneas at a premium.
EncScript_Xerneas_BlessingReturns::
	printstring STRINGID_ENCXERNEASBLESSINGRETURNS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_AQUA_RING_HEAL
	encaddvar 7, 2     @ Gain
	encsetvar 9, 0     @ BlessTick
	return

// The blessed Pokemon switched out - the blessing is left behind. No gain, and a cooldown before
// Xerneas will bless again.
EncScript_Xerneas_BlessingLeaves::
	printstring STRINGID_ENCXERNEASBLESSINGLEAVES
	waitmessage B_WAIT_TIME_LONG
	encsetvar 9, 0     @ BlessTick
	encsetvar 15, 2    @ BlessCool
	return

// THE PHASE 2 FLOOR HOLD. Xerneas spends 1 Life Energy to refuse falling below 25% - the fastest
// drain available to the player, and at most one hold per turn.
EncScript_Xerneas_EternalHold::
	printstring STRINGID_ENCXERNEASHOLDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HANGED_ON
	call EncScript_Xerneas_Spend1
	encsetvar 5, 25    @ Scratch: the floor
	encrewindhp ENC_TARGET_BOSS, 5
	encsetvar 14, 1    @ Held
	return

// Grassy Terrain heals every grounded battler at end of turn, including Xerneas itself, and it is
// up permanently from Phase 1 on. The engine scales that heal by the AUTHORED DamageReduction: (84,
// fixed), never the LIVE value _ApplyGuard writes (84-94, climbing with Life) - so the higher Life
// gets, the more Xerneas's own regen outpaces the shrinking trickle of damage the live guard lets
// through. Left alone that is a death spiral the player cannot out-damage. Cancel it outright each
// turn so VERDANT STATE stays a player-only benefit, and do it BEFORE the damage measurement below
// so a real hit isn't partly masked by the heal that already landed this turn.
EncScript_Xerneas_CancelTerrainHeal:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Xerneas_CancelTerrainHeal_Done
	encsetvar 5, 0
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_SET, EncScript_Xerneas_CancelTerrainHeal_Done
	encjumpifvar CMP_GREATER_THAN, 5, 99, EncScript_Xerneas_CancelTerrainHeal_Done
	encchangehp ENC_TARGET_BOSS, -1, ENC_AMOUNT_PERCENT
EncScript_Xerneas_CancelTerrainHeal_Done:
	return

// THE LEDGER, and the only place the turn's subsystems resolve. The order is load-bearing - Scratch
// is used twice and must be branched on before it is overwritten.
EncScript_Xerneas_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Xerneas_TurnClose_Quiet
	call EncScript_Xerneas_CancelTerrainHeal
	encsetvar 5, 0     @ Scratch
	enccopyvar 5, 4    @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Xerneas_TurnClose_NoDamage
	goto EncScript_Xerneas_TurnClose_Charge
EncScript_Xerneas_TurnClose_NoDamage:
	encsetvar 5, 0     @ nothing taken off Xerneas this turn
// Resolve GEOMANCY using the turn's damage. Charge == 1 fires; 2-3 with >= 12% breaks it; otherwise
// it keeps gathering (the tick already happened at _TurnOpen).
EncScript_Xerneas_TurnClose_Charge:
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Xerneas_TurnClose_Pressure
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Xerneas_TurnClose_Fire
	encjumpifvar CMP_LESS_THAN, 5, 12, EncScript_Xerneas_TurnClose_Pressure
	call EncScript_Xerneas_GeomancyBreaks
	goto EncScript_Xerneas_TurnClose_Pressure
EncScript_Xerneas_TurnClose_Fire:
	call EncScript_Xerneas_GeomancyFires
// Pressure spend: 10% or more off Xerneas this turn makes it spend 1 Life Energy to mend the wound -
// the player's always-available lever, independent of whether Geomancy also broke.
EncScript_Xerneas_TurnClose_Pressure:
	encjumpifvar CMP_LESS_THAN, 5, 10, EncScript_Xerneas_TurnClose_Cling
	call EncScript_Xerneas_Spend1
	printstring STRINGID_ENCXERNEASPRESSURE
	waitmessage B_WAIT_TIME_LONG
// Clinging to life: a player Pokemon ending the turn at or below 15% HP feeds the meter. Phase 2
// only - Survive: True on ALL_FOES already means nothing can die there, so clinging would otherwise
// let the player park a mon at low HP forever and cancel the burn to a 0-net stall that never ends.
EncScript_Xerneas_TurnClose_Cling:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Xerneas_TurnClose_Passive
	encsetvar 5, 0
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 5, ENC_SNAP_SET
	encjumpifvar CMP_GREATER_THAN, 5, 15, EncScript_Xerneas_TurnClose_Passive
	encaddvar 7, 1     @ Gain
// The passive: life persists on its own, every turn.
EncScript_Xerneas_TurnClose_Passive:
	encaddvar 7, 1     @ Gain
// Blessing upkeep.
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Xerneas_TurnClose_ApplyGain
	encchangehp ENC_TARGET_PLAYER_LEFT, 8, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	encsubvar 9, 1     @ BlessTick
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Xerneas_TurnClose_ApplyGain
	printstring STRINGID_ENCXERNEASBLESSINGFADES
	waitmessage B_WAIT_TIME_LONG
	encsetvar 15, 3    @ BlessCool
// Apply the turn's Gain to Life, clamped at 10.
EncScript_Xerneas_TurnClose_ApplyGain:
	encaddvar 1, 7
	encjumpifvar CMP_LESS_THAN, 1, 11, EncScript_Xerneas_TurnClose_Garden
	encsetvar 1, 10
EncScript_Xerneas_TurnClose_Garden:
	encsetvar 7, 0     @ Gain
// Grow the garden: Phase 0 targets 2 blooms, Phase 1 targets 3, Phase 2 targets 4.
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Xerneas_TurnClose_GardenP0
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Xerneas_TurnClose_GardenP1
	encjumpifvar CMP_LESS_THAN, 6, 4, EncScript_Xerneas_TurnClose_GardenGrow
	goto EncScript_Xerneas_TurnClose_Burn
EncScript_Xerneas_TurnClose_GardenP0:
	encjumpifvar CMP_LESS_THAN, 6, 2, EncScript_Xerneas_TurnClose_GardenGrow
	goto EncScript_Xerneas_TurnClose_Burn
EncScript_Xerneas_TurnClose_GardenP1:
	encjumpifvar CMP_LESS_THAN, 6, 3, EncScript_Xerneas_TurnClose_GardenGrow
	goto EncScript_Xerneas_TurnClose_Burn
EncScript_Xerneas_TurnClose_GardenGrow:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Xerneas_TurnClose_Burn
	call EncScript_Xerneas_Spend2
	encaddvar 6, 1     @ Blooms
	call EncScript_Xerneas_ApplyGarden
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Xerneas_TurnClose_Bloom1
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Xerneas_TurnClose_Bloom2
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Xerneas_TurnClose_Bloom3
	printstring STRINGID_ENCXERNEASBLOOM4
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Xerneas_TurnClose_Burn
EncScript_Xerneas_TurnClose_Bloom1:
	printstring STRINGID_ENCXERNEASBLOOM1
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Xerneas_TurnClose_Burn
EncScript_Xerneas_TurnClose_Bloom2:
	printstring STRINGID_ENCXERNEASBLOOM2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Xerneas_TurnClose_Burn
EncScript_Xerneas_TurnClose_Bloom3:
	printstring STRINGID_ENCXERNEASBLOOM3
	waitmessage B_WAIT_TIME_LONG
// Phase 2 burn: the meter is now a countdown, and it costs 2 every turn on top of everything else.
EncScript_Xerneas_TurnClose_Burn:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Xerneas_TurnClose_Settle
	call EncScript_Xerneas_Spend2
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Xerneas_TurnClose_Cycle
	printstring STRINGID_ENCXERNEASSTARVING
	waitmessage B_WAIT_TIME_LONG
EncScript_Xerneas_TurnClose_Cycle:
	call EncScript_Xerneas_ImmortalityCycle
EncScript_Xerneas_TurnClose_Settle:
	call EncScript_Xerneas_ApplyGuard
	call EncScript_Xerneas_Band
EncScript_Xerneas_TurnClose_Quiet:
	encsetvar 7, 0     @ Gain
	encsetvar 11, 0    @ LifeGuard
	encsetvar 12, 0    @ GardenGuard
	encsetvar 14, 0    @ Held
	encsetvar 3, 0     @ TurnGuard
	return

// --- Phase transitions ---

// 60% - THE FOREST AWAKENS. Permanent Grassy Terrain (VERDANT STATE), which heals the player too -
// intended, and it interacts with the clinging check in the player's favour. Bloom target and
// GEOMANCY availability are read live off Phase elsewhere, so nothing else needs setting here.
EncScript_Xerneas_ForestAwakens::
	printstring STRINGID_ENCXERNEASFORESTAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 1     @ Phase
	encsetterrain ENC_TERRAIN_GRASSY
	printstring STRINGID_ENCXERNEASVERDANT
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Xerneas_ApplyGuard
	call EncScript_Xerneas_Band
	return

// 30% - LIFE ETERNAL. Life is forced to 10 and becomes a countdown. Nothing on the field can be
// KO'd, but only for 5 turns at a time (_ImmortalityCycle) - the garden takes back what was borrowed
// and the fight becomes a denial puzzle with real openings, not an unlosable stall.
EncScript_Xerneas_LifeEternal::
	printstring STRINGID_ENCXERNEASLIFEETERNAL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 2     @ Phase
	encsetvar 1, 10    @ Life
	encsetvar 8, 10    @ Active, reused as the ImmortalityCycle counter from here on
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Xerneas_LifeEternal_Survive
EncScript_Xerneas_LifeEternal_Survive:
	encsetsurvive ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCXERNEASNOTHINGDIES
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Xerneas_ApplyGuard
	call EncScript_Xerneas_Band
	return

// Life reaches 0 in Phase 2 - NATURE'S GIFT. Xerneas gives everything it has left to the player,
// then falls silent. Falls through into SPENT: this is the encounter's thesis paid off, and it is
// mechanically harmless because the fight is over in all but name.
EncScript_Xerneas_NaturesGift::
	printstring STRINGID_ENCXERNEASNATURESGIFT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	encchangehp ENC_TARGET_ALL_FOES, 40, ENC_AMOUNT_PERCENT
	encrevive ENC_TARGET_ALL_FOES, 50
	printstring STRINGID_ENCXERNEASGIFTHEALS
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Xerneas_Spent

// SPENT, and the catch window (guideline 10). Reached from NATURE'S GIFT or the <= 1% deadlock
// fallback at either checkpoint, and Once on all of them, so the first selection disables the rest.
// The teardown mirrors Properties: plus everything the fight raised. _ApplyGuard returns immediately
// at Phase 3, so nothing can re-raise the guard behind it, and encsetdamagereduction 99 writes the
// value the automatic catch guard will RESTORE, not the live one. The form reverts to Neutral first
// so a caught Xerneas is unambiguous.
EncScript_Xerneas_Spent::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCXERNEASSILENT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Life
	encsetvar 6, 0     @ Blooms
	encsetvar 9, 0     @ BlessTick
	encsetvar 10, 0    @ Charge
	call EncScript_Xerneas_ApplyGarden
	removeterrain
	encformchange ENC_TARGET_BOSS, SPECIES_XERNEAS_NEUTRAL, EncScript_Xerneas_Spent_NoStats, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Xerneas_Spent_NoStats:
	encsetsurvive ENC_TARGET_ALL_FOES, FALSE
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Xerneas_Spent_Weaken
EncScript_Xerneas_Spent_Weaken:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCXERNEASWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
