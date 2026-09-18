// Tapu Fini. MIST (0-5, var 0) governs perception: Water hits and turns passing raise it, and it
// drives evasion/accuracy swings, terrain strength, and a Purification cycle that costs more Mist
// than it returns in tempo.
// 0 Mist (0-5 core meter), 1 Phase (1/2/3/4), 2 TurnGuard (once-per-OnTurnStart scaffold),
// 3 MistGuard (once-per-turn Water-hit mist gain scaffold), 4 MistTimer (countdown to passive Mist
// gain), 5 PurifyTimer (countdown to next Purification), 6 TrialTimer (countdown to next Mist
// Trial, Phase 2 only), 7 TrialActive (1 while ENTER's guard-drop window is live), 8 SacredTimer
// (Phase 3 countdown before mist recedes), 9 LastMist (last Mist value a callout printed for),
// 10 LastReduction (last damage-reduction value announced).
//
// Deviations from the written plan: TapuFini_ApplyMistLevel/TapuFini_PrintMistLine collapse into
// inline code at the two call sites (Mist 1-4 stay narrative-only in Phase 1, per the plan's own
// known-gaps note, so a separate absolute-set subroutine had nothing to do). Phase 2/3's stat
// changes target ENC_TARGET_BOSS explicitly rather than ENC_TARGET_SELF, which would resolve
// through Event.Battler and give the wrong battler at these dual OnMoveEnd/OnTurnEnd triggers.
// EncScript_TapuFini_MistRecedes drops the plan's trainerslidein/slideout (guideline 13: no
// trainer). One extra string (STRINGID_ENCFINITRIALCLOSES) covers the Mist Trial's close line the
// plan's dialogue described but didn't allocate an id for. LastMist's reprint-on-change trigger
// was dropped - conditions/encjumpifvar only compare a variable against a literal, never another
// variable, so `Var(Mist) != Var(LastMist)` doesn't compile. The mist-thickens line is printed
// directly at every site that raises Mist instead (TurnStart's passive tick, WaterHit); LastMist
// stays declared (its index is baked into every later var's bare-integer offset) but unread.

EncScript_TapuFini_Init::
	encsetterrain ENC_TERRAIN_MISTY
	encsetvar 1, 1     @ Phase = 1
	encsetvar 4, 3     @ MistTimer = 3
	encsetvar 5, 6     @ PurifyTimer = 6
	encsetvar 6, 5     @ TrialTimer = 5
	encsetvar 10, 90   @ LastReduction = 90
	return

EncScript_TapuFini_TurnStart::
	encsetvar 2, 1     @ TurnGuard = 1
	encsubvar 4, 1     @ MistTimer -= 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_TapuFini_TurnStart_Timers
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_TapuFini_TurnStart_MistCapped
	encaddvar 0, 1     @ Mist += 1 (passive gain, capped at 5)
	printstring STRINGID_ENCFINIMISTTHICKENS
	waitmessage B_WAIT_TIME_LONG
EncScript_TapuFini_TurnStart_MistCapped:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_TapuFini_TurnStart_ResetSlow
	encsetvar 4, 2     @ Phase 2/3: MistTimer = 2
	goto EncScript_TapuFini_TurnStart_Timers
EncScript_TapuFini_TurnStart_ResetSlow:
	encsetvar 4, 3     @ Phase 1: MistTimer = 3
EncScript_TapuFini_TurnStart_Timers:
	encsubvar 5, 1     @ PurifyTimer -= 1
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_TapuFini_TurnStart_Done
	encsubvar 6, 1     @ TrialTimer -= 1 (Phase 2 only)
EncScript_TapuFini_TurnStart_Done:
	return

EncScript_TapuFini_ClearTurnGuard::
	encsetvar 2, 0
	return

EncScript_TapuFini_WaterHit::
	encsetvar 3, 1     @ MistGuard = 1
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_TapuFini_WaterHit_Done
	encaddvar 0, 1     @ Mist += 1
	printstring STRINGID_ENCFINIMISTTHICKENS
	waitmessage B_WAIT_TIME_LONG
EncScript_TapuFini_WaterHit_Done:
	return

EncScript_TapuFini_ClearMistGuard::
	encsetvar 3, 0
	return

EncScript_TapuFini_Purification::
	printstring STRINGID_ENCFINIPURIFIES
	waitmessage B_WAIT_TIME_LONG
	encresetstats ENC_TARGET_BOSS, EncScript_TapuFini_Purification_NoStats
EncScript_TapuFini_Purification_NoStats:
	encclearhazards ENC_TARGET_ALL_ALLIES, EncScript_TapuFini_Purification_NoHazards
EncScript_TapuFini_Purification_NoHazards:
	curestatus BS_OPPONENT1
	encchangehp ENC_TARGET_BOSS, 4, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_TapuFini_Purification_MistCapped
	encaddvar 0, 1     @ Mist += 1 (Purification costs a Mist tick)
EncScript_TapuFini_Purification_MistCapped:
	encsetvar 5, 6     @ PurifyTimer = 6
	return

EncScript_TapuFini_MistTrial::
	encaskyesno STRINGID_ENCFINIMISTTRIAL, EncScript_TapuFini_MistTrial_Enter
	@ RESIST (NO/B): banks Mist, keeps visibility
	encjumpifvar CMP_EQUAL, 0, 5, EncScript_TapuFini_MistTrial_ResistCapped
	encaddvar 0, 1
EncScript_TapuFini_MistTrial_ResistCapped:
	printstring STRINGID_ENCFINITRIALRESIST
	waitmessage B_WAIT_TIME_LONG
	encsetvar 6, 5     @ TrialTimer = 5
	return
EncScript_TapuFini_MistTrial_Enter:
	encsetvar 7, 1     @ TrialActive = 1
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 10, 84   @ LastReduction
	printstring STRINGID_ENCFINITRIALENTER
	waitmessage B_WAIT_TIME_LONG
	encsetvar 6, 5     @ TrialTimer = 5
	return

EncScript_TapuFini_ClearTrial::
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 10, 90   @ LastReduction
	encsetvar 7, 0     @ TrialActive = 0
	printstring STRINGID_ENCFINITRIALCLOSES
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TapuFini_Phase2::
	encsetvar 1, 2     @ Phase = 2
	encsetvar 4, 2     @ MistTimer = 2
	encsetvar 6, 3     @ TrialTimer = 3
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 2
	encchangestat ENC_TARGET_ALL_FOES, STAT_ACC, -1
	encsetterrain ENC_TERRAIN_MISTY
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCFINIMISTTHICKENS
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TapuFini_Phase3::
	encsetvar 1, 3     @ Phase = 3
	encsetvar 0, 5     @ Mist forced to 5
	encsetvar 8, 3     @ SacredTimer = 3
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 2
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	printstring STRINGID_ENCFINISACREDMIST
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TapuFini_SacredTick::
	encsubvar 8, 1     @ SacredTimer -= 1
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_TapuFini_SacredTick_Done
	flushtextbox
	encchangehp ENC_TARGET_BOSS, 4, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_TapuFini_SacredTick_Done:
	return

EncScript_TapuFini_MistRecedes::
	encsetvar 1, 4     @ Phase = 4 (mist spent)
	printstring STRINGID_ENCFINIRECEDES
	waitmessage B_WAIT_TIME_LONG
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_TapuFini_MistRecedes_NoStats
EncScript_TapuFini_MistRecedes_NoStats:
	printstring STRINGID_ENCFINISURGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	return

EncScript_TapuFini_CatchWindow::
	printstring STRINGID_ENCFINICATCHWINDOW
	waitmessage B_WAIT_TIME_LONG
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetvar 10, 99   @ LastReduction
	return
