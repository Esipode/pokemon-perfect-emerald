// Tapu Lele - variable index map (must match the always-true block in tapu_lele.encounter):
// 0 Distortion, 1 Phase, 2 MoveGuard, 3 ShiftTimer, 4 ShiftGuard, 5 DreamTurns, 6 MirageGuard,
// 7 ShiftTick, 8 AdaptCount, 9 AdaptResult.

EncScript_TapuLele_Intro::
	return

// DISTORTION SOURCE 1 - the boss's own Psychic moves. No dialogue per use; the meaningful callouts
// are the field-layer latches below, which fire the moment a threshold is actually crossed.
EncScript_TapuLele_PsychicUse::
	encaddvar 0, 1     @ Distortion +1
	encsetvar 2, 1     @ MoveGuard latch
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	return

// MIRRORED RESONANCE - every player move that lands on Tapu Lele is filed as a type resistance.
EncScript_TapuLele_MirroredResonance::
	encsetvar 2, 1     @ MoveGuard latch
	encadapt ENC_TARGET_BOSS, 45, 3, 8, 9
	printstring STRINGID_ENCTAPULELERESONANCE
	waitmessage B_WAIT_TIME_LONG
	return

// DISTORTION SOURCE 2 - turns passing on their own, at 5/10/15.
EncScript_TapuLele_TurnsPass::
	encaddvar 0, 1     @ Distortion +1
	return

// One shared clear for MoveGuard, MirageGuard and ShiftTick - safe to zero all three unconditionally.
EncScript_TapuLele_ClearGuards::
	encsetvar 2, 0     @ MoveGuard
	encsetvar 6, 0     @ MirageGuard
	encsetvar 7, 0     @ ShiftTick
	return

// THE DISTORTION-GATED FIELD LAYERS. Each is a one-shot latch, telegraphed the moment it activates.
EncScript_TapuLele_TerrainLatch::
	encsetterrain ENC_TERRAIN_PSYCHIC
	printstring STRINGID_ENCTAPULELEPSYCHICTERRAINACTIVATES
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TapuLele_TrapLatch::
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	printstring STRINGID_ENCTAPULELETRAPPED
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TapuLele_WonderRoomLatch::
	encsetfieldstatus ENC_FIELD_WONDER_ROOM
	printstring STRINGID_ENCTAPULELEWONDERROOM
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TapuLele_TrickRoomLatch::
	encsetfieldstatus ENC_FIELD_TRICK_ROOM
	printstring STRINGID_ENCTAPULELEREALITYDISTORTED
	waitmessage B_WAIT_TIME_LONG
	return

// MIRAGE TELL - a fake stat-change line while a different real stat quietly moves. MirageGuard
// latches on the roll itself (hit or miss) so this can't reselect twice in the same OnTurnStart.
EncScript_TapuLele_MirageTell::
	encsetvar 6, 1     @ MirageGuard latch
	encjumpifchance 40, EncTapuLele_MirageTell_Fire
	return
EncTapuLele_MirageTell_Fire:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
	printstring STRINGID_ENCTAPULELEMIRAGETELL
	waitmessage B_WAIT_TIME_LONG
	return

// REALITY SHIFT countdown tick - resolved at OnTurnEnd by ShiftResolve below.
EncScript_TapuLele_ShiftTick::
	encsubvar 3, 1     @ ShiftTimer -1
	encsetvar 7, 1     @ ShiftTick latch
	return

EncScript_TapuLele_ClearShiftGuard::
	encsetvar 4, 0     @ ShiftGuard
	return

// REALITY SHIFT itself - weather, a stat tilt, or a side-status swap, each ending on the same
// recurring line so the mechanic stays legible while it's live.
EncTapuLele_RealityShift:
	encjumpifchance 33, EncTapuLele_RealityShift_Weather
	encjumpifchance 50, EncTapuLele_RealityShift_StatTilt
	goto EncTapuLele_RealityShift_SideSwap
EncTapuLele_RealityShift_Weather:
	encsetweather BATTLE_WEATHER_SANDSTORM, 5
	printstring STRINGID_ENCTAPULELEREALITYSHIFT
	waitmessage B_WAIT_TIME_LONG
	return
EncTapuLele_RealityShift_StatTilt:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	printstring STRINGID_ENCTAPULELEREALITYSHIFT
	waitmessage B_WAIT_TIME_LONG
	return
EncTapuLele_RealityShift_SideSwap:
	encsetsidestatus ENC_TARGET_OPPONENT_LEFT, ENC_SIDE_TAILWIND, 3
	printstring STRINGID_ENCTAPULELEREALITYSHIFT
	waitmessage B_WAIT_TIME_LONG
	return

// REALITY SHIFT resolve. While Phase == 2 the Dream-turn countdown rides the same routine: force
// ShiftTimer back to 0 instead of 3, so the next shift fires next turn instead of in three.
EncScript_TapuLele_ShiftResolve::
	encsetvar 4, 1     @ ShiftGuard latch
	call EncTapuLele_RealityShift
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_TapuLele_ShiftResolve_Dream
	encsetvar 3, 3     @ ShiftTimer reset - Phase 1 cadence, every 3 turns
	return
EncScript_TapuLele_ShiftResolve_Dream:
	encsubvar 5, 1     @ DreamTurns -1
	encsetvar 3, 0     @ ShiftTimer stays 0 - Phase 2 cadence, every turn
	return

// PHASE 1 - PSYCHIC REVERIE begins at <=50% HP. Distortion snaps to 5, which lets the field-layer
// latches above cascade over the next OnTurnStart or two, announcing whatever wasn't already up.
EncScript_TapuLele_ReverieBegins::
	encsetvar 0, 5     @ Distortion pinned to 5
	encsetvar 1, 1     @ Phase = Reverie
	encsetvar 3, 3     @ ShiftTimer = 3
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// PHASE 2 - GUARDIAN'S DREAM begins at <=10% HP. A 3-turn window, Reality Shift every turn.
EncScript_TapuLele_DreamBegins::
	encsetvar 1, 2     @ Phase = Dream
	encsetvar 5, 3     @ DreamTurns = 3
	encsetvar 3, 0     @ ShiftTimer forced 0 - the first Dream-turn shift fires immediately
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	printstring STRINGID_ENCTAPULELEDREAMBEGINS
	waitmessage B_WAIT_TIME_LONG
	return

// PHASE 3 - SPENT. The Guardian Psychic finisher, then the full collapse and the catch window.
EncScript_TapuLele_Spent::
	encsetvar 1, 3     @ Phase = Spent
	printstring STRINGID_ENCTAPULELEVULNERABLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -28, ENC_AMOUNT_PERCENT
	removeweather
	encclearfieldstatus ENC_FIELD_WONDER_ROOM
	encclearfieldstatus ENC_FIELD_TRICK_ROOM
	removeterrain
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	return
