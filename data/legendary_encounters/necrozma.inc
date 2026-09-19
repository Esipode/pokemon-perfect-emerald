// Necrozma, "The Prism Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Necrozma_Intro:
// 0 Phase (0 Starved / 1 Fusion / 2 Ultra / 3 Catch-window), 1 Light (0-100, also the Ultra Light
// Overload meter), 2 LastBand (last ANNOUNCED Light band - 0 Hungry/1 Feeding/2 Sated/3
// Overcharged - the callout's re-fire latch and the crit/stat one-time-per-band-entry gate),
// 3 FragmentTimer (turns left on the active Light Fragment, 0 = none - Phase 0 only), 4 Mark (boss
// HP% at turn open), 5 FormRoll (0 undetermined / 1 Dusk Mane / 2 Dawn Wings), 6 Armor (Dusk Mane
// 3-0 Solar Armor layers), 7 EclipseDepth (Dawn Wings 0-3), 8 TurnGuard (once-per-turn scaffold),
// 9 EclipseTimer (Dawn Wings countdown to the next depth increase), 10 Scratch (the copy
// ENC_SNAP_DAMAGE/RECOVERY consumes, and the Ultra-phase prediction slot), 11 PMark (player's HP%
// at turn open, for heal detection).
//
// LIGHT ONLY RISES IN PHASE 0. Nothing in this fight lowers it once set, so EncScript_Necrozma_Band
// is a single writer that only ever needs to move forward - it still writes crit absolutely every
// call (TRUE at Sated/Overcharged, FALSE below) rather than assuming that, so a future mechanic
// that does lower Light can't leave a stale crit behind.
//
// LIGHT FRAGMENTS ARE PHASE 0 ONLY. Dusk Mane's armor breaks and Dawn Wings' eclipse depth are the
// Fusion phase's own race instead of a second fragment cycle - keeping the resource distinct per
// phase is what makes each phase read as a different fight rather than the same meter three times.
//
// ARMOR BREAK THRESHOLDS ARE HP DECILES (<=40/<=30/<=20), not a landed-hit counter - Fusion runs
// from 50% to 20% HP, so the three breaks land evenly across the phase regardless of how the player
// gets there (one big hit or many small ones).
//
// THE ULTRA TELEGRAPH READS THE BOSS'S OWN CATEGORY. encstoreprediction's target is the battler
// whose upcoming move category it reports; here that is ENC_TARGET_BOSS, so the line warns about
// Necrozma's own incoming special move rather than predicting the player. Category-only, not
// move-specific - documented in the cheat sheet as an approximation, since Photon Geyser's own
// power can't be scaled per-cast from Light.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation. No trainerslidein or
// trainerslideout anywhere - this is a wild Pokemon (guideline 13).

// --- Shared subroutines (call/return) ---

// THE BAND CALLOUT AND CRIT WRITER (guideline 14). LastBand holds the last ANNOUNCED band; each arm
// compares it against its own literal and stays quiet if that's what was last said. Crit is written
// absolutely on every call, ahead of the announce check, so it never drifts from what Light says it
// should be. The Overcharged arm's Atk/SpAtk buff only fires on the edge into that band (LastBand
// still != 3), so it's a one-time buff without a separate latch.
EncScript_Necrozma_Band:
	encjumpifvar CMP_LESS_THAN, 1, 70, EncScript_Necrozma_Band_NoCrit
	encsetcrit ENC_TARGET_BOSS, TRUE
	goto EncScript_Necrozma_Band_Announce
EncScript_Necrozma_Band_NoCrit:
	encsetcrit ENC_TARGET_BOSS, FALSE
EncScript_Necrozma_Band_Announce:
	encjumpifvar CMP_LESS_THAN, 1, 40, EncScript_Necrozma_Band_Hungry
	encjumpifvar CMP_LESS_THAN, 1, 70, EncScript_Necrozma_Band_Feeding
	encjumpifvar CMP_LESS_THAN, 1, 90, EncScript_Necrozma_Band_Sated
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Necrozma_Band_Done
	encsetvar 2, 3
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -11    @ net absolute +1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -11
	printstring STRINGID_ENCNECROZMAOVERCHARGED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return
EncScript_Necrozma_Band_Sated:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Necrozma_Band_Done
	encsetvar 2, 2
	printstring STRINGID_ENCNECROZMASATED
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Necrozma_Band_Feeding:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Necrozma_Band_Done
	encsetvar 2, 1
	printstring STRINGID_ENCNECROZMAFEEDING
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Necrozma_Band_Hungry:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Necrozma_Band_Done
	encsetvar 2, 0
EncScript_Necrozma_Band_Done:
	return

// Rolls whether a Light Fragment appears this turn. Called only when FragmentTimer is already 0.
EncScript_Necrozma_FragmentSpawn:
	encjumpifchance 35, EncScript_Necrozma_FragmentSpawn_Go
	return
EncScript_Necrozma_FragmentSpawn_Go:
	encsetvar 3, 2    @ FragmentTimer: a 2-turn window
	printstring STRINGID_ENCNECROZMAFRAGMENTSPAWN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// --- Trigger entry points ---

EncScript_Necrozma_Intro::
	encsetvar 2, 9     @ LastBand: nothing announced yet
	call EncScript_Necrozma_Band
	return

// Top of every turn. Phase 0 takes its snapshots and rolls a fragment spawn; Phase 1 (Dawn Wings
// only) ticks the eclipse timer; Phase 2 runs the Photon Geyser telegraph. TurnGuard is set
// immediately so no branch out can leave the trigger re-armed.
EncScript_Necrozma_TurnOpen::
	encsetvar 8, 1     @ TurnGuard
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Necrozma_TurnOpen_Fusion
	encsnapshothp ENC_TARGET_BOSS, 4, ENC_SNAP_SET         @ Mark
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 11, ENC_SNAP_SET  @ PMark
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Necrozma_TurnOpen_Done
	call EncScript_Necrozma_FragmentSpawn
	return
EncScript_Necrozma_TurnOpen_Fusion:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Necrozma_TurnOpen_Ultra
	encjumpifvar CMP_NOT_EQUAL, 5, 2, EncScript_Necrozma_TurnOpen_Done    @ Dusk Mane: nothing to tick
	encjumpifvar CMP_GREATER_THAN, 7, 2, EncScript_Necrozma_TurnOpen_Done  @ already Total Eclipse
	encsubvar 9, 1     @ EclipseTimer
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Necrozma_TurnOpen_Done
	encaddvar 7, 1     @ EclipseDepth
	encsetvar 9, 3     @ EclipseTimer resets
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCNECROZMAECLIPSE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	return
EncScript_Necrozma_TurnOpen_Ultra:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Necrozma_TurnOpen_Done
	encstoreprediction ENC_TARGET_BOSS, 10    @ Scratch
	encjumpifvar CMP_NOT_EQUAL, 10, 2, EncScript_Necrozma_TurnOpen_Done   @ 2 = special
	printstring STRINGID_ENCNECROZMAGEYSER
	waitmessage B_WAIT_TIME_LONG
EncScript_Necrozma_TurnOpen_Done:
	return

// The ledger. Phase 0 resolves the turn's Fragment (claimed if the boss took any damage this turn,
// otherwise ticked down and absorbed at 0) and the Light gained from a big hit (>=15% of the boss's
// HP) or a real heal (>=5% recovery), then re-checks the band. Phase 1 has nothing scheduled here -
// the eclipse timer lives entirely in TurnOpen. Phase 2 decays Light 5/turn, floored at 0.
EncScript_Necrozma_TurnClose::
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Necrozma_TurnClose_Phase1
	enccopyvar 10, 4    @ Scratch = Mark
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Necrozma_TurnClose_P0_NoBossDmg
	goto EncScript_Necrozma_TurnClose_P0_Measured
EncScript_Necrozma_TurnClose_P0_NoBossDmg:
	encsetvar 10, 0
EncScript_Necrozma_TurnClose_P0_Measured:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Necrozma_TurnClose_P0_BigHit
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Necrozma_TurnClose_P0_FragTick
	encchangehp ENC_TARGET_BOSS, -3, ENC_AMOUNT_PERCENT
	encsetvar 3, 0     @ FragmentTimer: claimed
	printstring STRINGID_ENCNECROZMAFRAGMENTCLAIMED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	goto EncScript_Necrozma_TurnClose_P0_BigHit
EncScript_Necrozma_TurnClose_P0_FragTick:
	encsubvar 3, 1
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Necrozma_TurnClose_P0_BigHit
	encaddvar 1, 12    @ Light: absorbed
	printstring STRINGID_ENCNECROZMAFRAGMENTABSORBED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Necrozma_TurnClose_P0_BigHit:
	encjumpifvar CMP_LESS_THAN, 10, 15, EncScript_Necrozma_TurnClose_P0_Heal
	encaddvar 1, 8     @ Light: big hit
EncScript_Necrozma_TurnClose_P0_Heal:
	enccopyvar 10, 11   @ Scratch = PMark
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 10, ENC_SNAP_RECOVERY, EncScript_Necrozma_TurnClose_P0_Clamp
	encjumpifvar CMP_LESS_THAN, 10, 5, EncScript_Necrozma_TurnClose_P0_Clamp
	encaddvar 1, 6     @ Light: real heal
EncScript_Necrozma_TurnClose_P0_Clamp:
	encjumpifvar CMP_LESS_THAN, 1, 100, EncScript_Necrozma_TurnClose_P0_Band
	encsetvar 1, 100
EncScript_Necrozma_TurnClose_P0_Band:
	call EncScript_Necrozma_Band
	goto EncScript_Necrozma_TurnClose_Done
EncScript_Necrozma_TurnClose_Phase1:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Necrozma_TurnClose_Done
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Necrozma_TurnClose_P2_Floor
	encsubvar 1, 5
	goto EncScript_Necrozma_TurnClose_Done
EncScript_Necrozma_TurnClose_P2_Floor:
	encsetvar 1, 0
EncScript_Necrozma_TurnClose_Done:
	encsetvar 8, 0     @ TurnGuard
	return

// --- Phase transitions ---

// 50% - THE FUSION. A straight 50/50 coin flip (no party-based Solgaleo/Lunala check exists) locks
// in the rest of the fight's shape. Both branches replace the flavour slot (Stealth Rock) with the
// form's signature move; Photon Geyser waits for Ultra.
EncScript_Necrozma_FusionTransition::
	printstring STRINGID_ENCNECROZMAFUSION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 1     @ Phase
	encsetvar 6, 3     @ Armor - only meaningful if Dusk Mane is rolled
	encjumpifchance 50, EncScript_Necrozma_FusionTransition_DawnWings
EncScript_Necrozma_FusionTransition_DuskMane:
	encformchange ENC_TARGET_BOSS, SPECIES_NECROZMA_DUSK_MANE, EncScript_Necrozma_FusionTransition_DuskManeDone, B_ANIM_FORM_CHANGE
EncScript_Necrozma_FusionTransition_DuskManeDone:
	encsetvar 5, 1     @ FormRoll
	encsetmove ENC_TARGET_BOSS, 2, MOVE_SUNSTEEL_STRIKE
	printstring STRINGID_ENCNECROZMADUSKMANE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Necrozma_FusionTransition_DawnWings:
	encformchange ENC_TARGET_BOSS, SPECIES_NECROZMA_DAWN_WINGS, EncScript_Necrozma_FusionTransition_DawnWingsDone, B_ANIM_FORM_CHANGE
EncScript_Necrozma_FusionTransition_DawnWingsDone:
	encsetvar 5, 2     @ FormRoll
	encsetvar 7, 0     @ EclipseDepth
	encsetvar 9, 3     @ EclipseTimer
	encsetmove ENC_TARGET_BOSS, 2, MOVE_MOONGEIST_BEAM
	printstring STRINGID_ENCNECROZMADAWNWINGS
	waitmessage B_WAIT_TIME_LONG
	return

// Dusk Mane's three Solar Armor layers, each one HP decile apart (<=40/<=30/<=20). Every break
// weakens the guard, buffs Sunsteel Strike's raw Attack, and reprints the same break line - the
// recurring cue is the layer count itself, visible on the health bar's pace.
EncScript_Necrozma_ArmorBreak1::
	encsetvar 6, 2     @ Armor
	encsetdamagereduction ENC_TARGET_BOSS, 87
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCNECROZMAARMORBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

EncScript_Necrozma_ArmorBreak2::
	encsetvar 6, 1
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCNECROZMAARMORBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

EncScript_Necrozma_ArmorBreak3::
	encsetvar 6, 0
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCNECROZMAARMORBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

// 20% - ULTRA NECROZMA, folding in "The Light Devours All". Light resets to 100 as the Light
// Overload meter and decays every turn from here (TurnClose); Photon Geyser enters the moveset in
// the Iron Defense slot, telegraphed each turn from TurnOpen.
EncScript_Necrozma_UltraTransform::
	printstring STRINGID_ENCNECROZMAULTRA
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	encsetvar 0, 2     @ Phase
	encsetvar 1, 100   @ Light: the Light Overload meter
	encsetvar 2, 9     @ LastBand: bands no longer apply
	encformchange ENC_TARGET_BOSS, SPECIES_NECROZMA_ULTRA, EncScript_Necrozma_UltraTransform_Done, B_ANIM_FORM_CHANGE
EncScript_Necrozma_UltraTransform_Done:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_PHOTON_GEYSER
	return

// 10% - the catch window (guideline 10). The automatic catch-window damage guard takes over the
// live reduction the moment Balls: Allowed is set, so there's no manual bump-then-drop step here.
EncScript_Necrozma_CatchWindow::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCNECROZMACATCH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 45
	return
