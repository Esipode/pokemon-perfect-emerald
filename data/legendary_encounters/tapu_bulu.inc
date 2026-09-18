// Tapu Bulu, "The Land Spirit Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_TapuBulu_Intro:
// 0 Growth (0-6, the fight's main meter), 1 Wrath (0-5, rises only on Fire hitting Roots/
// Overgrowth), 2 Phase (0 Nature Guardian / 1 Ancient Guardian / 2 The Forest Awakens),
// 3 RootsActive (0/1), 4 OvergrowthActive (0/1), 5 Handled (per-turn Fire-reaction latch),
// 6 TurnGuard (the once-per-turn scaffold), 7 LastReduction (the last ANNOUNCED reduction band).
//
// GROWTH VS WRATH IS THE FIGHT. Growth climbs on its own and unlocks Roots/Overgrowth - the
// player's guard-stripping tool is Fire, and every use of it feeds Wrath instead. Leave the
// vegetation alone: a slower, tankier Bulu. Burn it: a faster fight that hits harder both ways.
//
// EncScript_TapuBulu_ApplyReduction is the single writer of the damage reduction outside
// _ForestFallsSilent, so a band falling always undoes a band rising with no separate teardown
// path (guideline: one writer per number).
//
// EVERY Event.*-REACTING TRIGGER CLOSES ITS OWN LATCH: _RootsDestroyed and _OvergrowthScorched
// both set Handled, which _TurnEndTick clears - Once is "once per battle", not "once per Fire
// hit", so the latch is what stops the re-evaluation loop from re-selecting them mid-checkpoint.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction, read absolutely off Phase/RootsActive/
// OvergrowthActive/Wrath. Guideline 6/9's 90 base, +4 for Overgrowth's hardened guard, -10 once
// Wrath 3 turns Bulu reckless (Reckless replaces Overgrowth's bonus rather than stacking under
// it), +2 more at Phase 2 (Primal Overgrowth, capped 96). Silent; the caller re-announces.
EncScript_TapuBulu_ApplyReduction:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_TapuBulu_ApplyReduction_Reckless
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_TapuBulu_ApplyReduction_Base
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_TapuBulu_ApplyReduction_P2Guarded
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_TapuBulu_ApplyReduction_Base:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_TapuBulu_ApplyReduction_P2Bare
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_TapuBulu_ApplyReduction_P2Guarded:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_TapuBulu_ApplyReduction_P2Bare:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_TapuBulu_ApplyReduction_Reckless:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return

// THE GUARD CALLOUT (guideline 14). LastReduction holds the last ANNOUNCED band; each arm
// compares against its own literal and stays quiet if that is what was last said, so the line
// prints only when the band actually changed.
EncScript_TapuBulu_AnnounceReduction:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_TapuBulu_AnnounceReduction_Reckless
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_TapuBulu_AnnounceReduction_Base
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_TapuBulu_AnnounceReduction_P2Guarded
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_TapuBulu_AnnounceReduction_Done
	encsetvar 7, 1
	printstring STRINGID_ENCTAPUBULUOVERGROWTHGROWS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_TapuBulu_AnnounceReduction_Base:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_TapuBulu_AnnounceReduction_P2Bare
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_TapuBulu_AnnounceReduction_Done
	encsetvar 7, 0
	printstring STRINGID_ENCTAPUBULUGUARDLINE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_TapuBulu_AnnounceReduction_P2Guarded:
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_TapuBulu_AnnounceReduction_Done
	encsetvar 7, 3
	printstring STRINGID_ENCTAPUBULUPHASE3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_TapuBulu_AnnounceReduction_P2Bare:
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_TapuBulu_AnnounceReduction_Done
	encsetvar 7, 4
	printstring STRINGID_ENCTAPUBULUGUARDLINE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_TapuBulu_AnnounceReduction_Reckless:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_TapuBulu_AnnounceReduction_Done
	encsetvar 7, 2
	printstring STRINGID_ENCTAPUBULURECKLESS
	waitmessage B_WAIT_TIME_LONG
EncScript_TapuBulu_AnnounceReduction_Done:
	return

// --- Trigger entry points ---

// OnBattleStart. Grassy Terrain is real field terrain, up for the whole fight. LastReduction
// starts at 9 (nothing announced yet) so the opening callout always prints. No trainerslidein -
// this is a wild Pokemon (guideline 13).
EncScript_TapuBulu_Intro::
	printstring STRINGID_ENCTAPUBULUINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTAPUBULUINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetterrain ENC_TERRAIN_GRASSY
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	encsetvar 7, 9    @ LastReduction: nothing announced yet
	call EncScript_TapuBulu_ApplyReduction
	call EncScript_TapuBulu_AnnounceReduction
	return

// Top of every turn (SPEC's once-per-turn scaffold, OnTurnStart half). Leads with flushtextbox -
// an OnTurnStart script that animates before any dialogue renders on top of the still-open action
// menu. Overgrowth's camouflage: a flat 40% chance of encsetprotect, only meaningful set here
// since gProtectStructs is cleared after end-of-turn effects.
EncScript_TapuBulu_TurnStartGuard::
	flushtextbox
	encsetvar 6, 1    @ TurnGuard
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_TapuBulu_TurnStartGuard_Done
	encjumpifchance 40, EncScript_TapuBulu_TurnStartGuard_Camouflage
	goto EncScript_TapuBulu_TurnStartGuard_Done
EncScript_TapuBulu_TurnStartGuard_Camouflage:
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCTAPUBULUCAMOUFLAGE
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_PROTECTED_ITSELF
EncScript_TapuBulu_TurnStartGuard_Done:
	return

// THE MAIN TICK (OnTurnEnd half of the scaffold). Growth climbs while Phase < 2 (capped 6);
// Overgrowth heals Bulu 2% while it holds (guideline 7's regular-heal cap); Phase 2 instead decays
// Growth by 1, floored at 0, toward the forest falling silent. Self-disables by clearing
// TurnGuard/Handled.
EncScript_TapuBulu_TurnEndTick::
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_TapuBulu_TurnEndTick_Decay
	encjumpifvar CMP_GREATER_THAN, 0, 5, EncScript_TapuBulu_TurnEndTick_Heal
	encaddvar 0, 1    @ Growth
EncScript_TapuBulu_TurnEndTick_Heal:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_TapuBulu_TurnEndTick_Clear
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	goto EncScript_TapuBulu_TurnEndTick_Clear
EncScript_TapuBulu_TurnEndTick_Decay:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_TapuBulu_TurnEndTick_Clear
	encsubvar 0, 1    @ Growth
EncScript_TapuBulu_TurnEndTick_Clear:
	encsetvar 5, 0    @ Handled
	encsetvar 6, 0    @ TurnGuard
	return

// Growth 2 - Roots coil around the player's feet: trap plus a one-time -1 Speed stage (guideline
// 7's Stage 2 substitution). Stat stage written absolutely so it survives a Haze/switch landing
// in between.
EncScript_TapuBulu_RootsAttach::
	printstring STRINGID_ENCTAPUBULUROOTSATTACH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	encsetvar 3, 1    @ RootsActive
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -7    @ ceiling 12 -> stage 5 (-1 from neutral)
	return

// Growth 3 - Overgrowth thickens: guard hardens (90->94) and Bulu starts healing off it each
// turn.
EncScript_TapuBulu_OvergrowthGrows::
	printstring STRINGID_ENCTAPUBULUOVERGROWTHGROWS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encsetvar 4, 1    @ OvergrowthActive
	call EncScript_TapuBulu_ApplyReduction
	call EncScript_TapuBulu_AnnounceReduction
	return

// A Fire move burns the Roots away: trap lifts, Wrath rises. Self-disables through Handled.
EncScript_TapuBulu_RootsDestroyed::
	printstring STRINGID_ENCTAPUBULUROOTSDESTROYED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encsetvar 3, 0    @ RootsActive
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encaddvar 1, 1    @ Wrath
	encsetvar 5, 1    @ Handled
	return

// A Fire move scorches Overgrowth away: guard drops back to base, Wrath rises. Self-disables
// through Handled.
EncScript_TapuBulu_OvergrowthScorched::
	printstring STRINGID_ENCTAPUBULUOVERGROWTHSCORCHED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encsetvar 4, 0    @ OvergrowthActive
	encaddvar 1, 1    @ Wrath
	encsetvar 5, 1    @ Handled
	call EncScript_TapuBulu_ApplyReduction
	call EncScript_TapuBulu_AnnounceReduction
	return

// Wrath 3 - Reckless Guardian. Wrath consumed it: Atk+2 (guideline 7's stage cap), guard drops
// to a flat 84 for the rest of the fight, permanent - the reward for restraint is a softer Bulu.
EncScript_TapuBulu_RecklessGuardian::
	printstring STRINGID_ENCTAPUBULURECKLESS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -4    @ ceiling 12 -> stage 8 (+2 from neutral)
	call EncScript_TapuBulu_ApplyReduction
	call EncScript_TapuBulu_AnnounceReduction
	return

// Wrath 5 - Forest Reclamation. Roots surge back even after being burned away once; the second
// and last -1 Speed stage (guideline 7's total -2 cap for this fight).
EncScript_TapuBulu_ForestReclamation::
	printstring STRINGID_ENCTAPUBULURECLAMATION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	encsetvar 3, 1    @ RootsActive
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -8    @ ceiling 12 -> stage 4 (-2 from neutral, total cap)
	return

// 50% - Ancient Guardian. Growth forced to 4+ and Roots force-attached if not already up, so
// Phase 2 always opens with the trap live.
EncScript_TapuBulu_Phase2Transition::
	printstring STRINGID_ENCTAPUBULUPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 2, 1    @ Phase
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_TapuBulu_Phase2Transition_Roots
	encsetvar 0, 4    @ Growth
EncScript_TapuBulu_Phase2Transition_Roots:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_TapuBulu_Phase2Transition_Done
	printstring STRINGID_ENCTAPUBULUROOTSATTACH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	encsetvar 3, 1    @ RootsActive
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, 12
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -7    @ ceiling 12 -> stage 5 (-1 from neutral)
EncScript_TapuBulu_Phase2Transition_Done:
	return

// 20% - The Forest Awakens. Growth snaps to 6, guard maxes at 96, then Nature's Wrath fires: one
// 16% burst (guideline 7's "real hit" baseline, not a scripted move) telegraphed a beat ahead.
// From here TurnEndTick decays Growth back to 0, which is what opens the catch window.
EncScript_TapuBulu_Phase3Transition::
	printstring STRINGID_ENCTAPUBULUPHASE3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 2, 2    @ Phase
	encsetvar 0, 6    @ Growth
	call EncScript_TapuBulu_ApplyReduction
	call EncScript_TapuBulu_AnnounceReduction
	printstring STRINGID_ENCTAPUBULUNATURESWRATH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
	return

// Growth decays to 0 in Phase 3: THE FOREST FALLS SILENT (guideline 10, the catch window). Guard
// drops to 70 - below the fight's normal band, but the catch window is exempt (guideline 6) - and
// this is only the value the automatic catch guard will restore once it lifts, not the live
// number, since encsetballs below forces the engine's own max-reduction guard on immediately.
EncScript_TapuBulu_ForestFallsSilent::
	encsetvar 2, 3    @ Phase (past the fight, for any later condition that reads it)
	printstring STRINGID_ENCTAPUBULUFORESTSILENT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_TapuBulu_ForestFallsSilent_NoStats
EncScript_TapuBulu_ForestFallsSilent_NoStats:
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	return
