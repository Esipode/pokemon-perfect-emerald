// Terapagos, "The Terastal Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Terapagos_Intro:
// 0 Phase (0 Dormant / 1 Terastal / 2 Cataclysm), 1 StellarEnergy (0-18ish, the meter),
// 2 ActiveTypeSlot (0-5, rotates every turn in Phases 1-2; also the Cascade chain's matched
// type), 3 WindowUp (1 while a Type Crystal / Stellar Crown exposed window is open), 4
// WindowTurns (countdown, ticked OnTurnStart, resolved OnTurnEnd), 5 AdaptCount (mirrors
// encadapt's board size), 6 AdaptResult (encadapt's ENC_ADAPT_RESULT_* outcome), 7 CascadeCount
// (consecutive matched-type hits this Cataclysm window), 8 WindowMark (boss HP% snapshot when a
// window opened), 9 TurnGuard, 10 SiphonUp (1 while the Cataclysm Tera-punish window is armed),
// 11 Handled (per-turn OnMoveEnd latch for the Dormant/Terastal/Cataclysm-crown reactions, shared
// since only one phase is ever active), 12 Scratch (ENC_SNAP_DAMAGE's working copy, so WindowMark
// survives being measured more than once across a window), 13 Handled2 (per-turn latch for the
// Stellar Cascade reaction - separate from Handled because both can legitimately fire on the same
// hit in Phase 2).
//
// STELLAR ENERGY IS A THEMATIC PROXY, not a literal per-type tally It
// starts at 10, Crystal Resonance HARDENED results feed it (+1, capped), the exposed windows
// strip it (-2 Crystal, -4 Crown, clamped at 0 since encsubvar wraps on an unsigned byte), and a
// Cataclysm Tera-punish feeds it back (+2). Tera Starstorm's in-fight damage is not rescaled by
// it - no enc* command rescales a move's own output - the meter is flavor text only (Known gap).
//
// THE ACTIVE TYPE ROTATION (var 2) cycles through six types every turn in Phases 1-2 and drives
// the Stellar Cascade chain via six near-identical triggers in the .encounter, one per type, since
// Conditions can test Event.MoveType against a literal but no enc* command can read Event.MoveType
// into a variable for a script to branch on. No engine command buffers an arbitrary TYPE_* name
// into dialogue outside encadapt's own call (which buffers {B_BUFF1} itself, immediately after
// being called) - so window/rotation dialogue stays generic rather than naming the live type,
// and only Resonance Break (printed right after its own encadapt call) uses {B_BUFF1}. No {TYPE} placeholder is used in the strings.
//
// EVERY OnMoveEnd HIT-REACTION IS SELF-DISABLING through Handled/Handled2, cleared at OnTurnEnd
// alongside TurnGuard. THE OnTurnStart ROLL IS FOLDED INTO _TurnOpen, which only ever runs once a
// turn - that satisfies a probabilistic trigger's own self-disable requirement for free.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation. _TurnOpen leads with
// dialogue before its animation, per the OnTurnStart action-menu gotcha.

// --- Shared subroutines (call/return) ---

// Rotates the Active Type slot 0->5->0.
EncScript_Terapagos_RotateType:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Terapagos_RotateType_Wrap
	encaddvar 2, 1
	return
EncScript_Terapagos_RotateType_Wrap:
	encsetvar 2, 0
	return

// Opens an exposed window: marks the boss's current HP% as the ENC_SNAP_DAMAGE baseline, arms the
// window, and starts its 3-turn countdown.
EncScript_Terapagos_OpenWindow:
	encsetvar 3, 1
	encsetvar 4, 3
	encsnapshothp ENC_TARGET_BOSS, 8
	return

// Strips \1 Stellar Energy points, clamped at 0 rather than wrapping (encsubvar wraps an unsigned
// byte on underflow). Not call-able with an argument, so each call site inlines its own clamp
// check instead - kept here as documentation of the pattern used at every strip site.

// --- Trigger scripts ---

EncScript_Terapagos_Intro::
	encsetvar 1, 10   @ StellarEnergy starts at 10 - the fight's inherent stored charge
	return

// CRYSTAL RESONANCE. Files the type that just hit the boss; a HARDENED result (the type was
// already held) feeds Stellar Energy and lashes back. Handled closes the latch before anything
// else runs, on every path, since Event.Battler stays true for the rest of this checkpoint's
// re-evaluation passes.
EncScript_Terapagos_DormantHit::
	encsetvar 11, 1
	encsetvar 6, 255  @ AdaptResult sentinel - no outcome unless encadapt writes one
	encadapt ENC_TARGET_BOSS, 25, 3, 5, 6
	encjumpifvar CMP_NOT_EQUAL, 6, ENC_ADAPT_RESULT_HARDENED, EncScript_Terapagos_DormantHit_Done
	encjumpifvar CMP_GREATER_THAN, 1, 17, EncScript_Terapagos_DormantHit_Lash
	encaddvar 1, 1
EncScript_Terapagos_DormantHit_Lash:
	printstring STRINGID_ENCTERAPAGOSRESONANCEBREAK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
EncScript_Terapagos_DormantHit_Done:
	return

EncScript_Terapagos_EnterTerastal::
	encsetvar 0, 1
	printstring STRINGID_ENCTERAPAGOSENTERSTERASTAL
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_TERAPAGOS_TERASTAL, EncScript_Terapagos_EnterTerastal_Done, B_ANIM_TERA_ACTIVATE
EncScript_Terapagos_EnterTerastal_Done:
	return

// TYPE CRYSTAL. Cumulative damage since the window opened (ENC_SNAP_DAMAGE off a copy of
// WindowMark, so the mark itself survives being measured more than once across the window's 3
// turns) at or above the guideline-7 single-hit ceiling breaks it and strips Stellar Energy.
EncScript_Terapagos_TerastalHit::
	encsetvar 11, 1
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Terapagos_TerastalHit_Done
	enccopyvar 12, 8
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE
	encjumpifvar CMP_LESS_THAN, 12, 5, EncScript_Terapagos_TerastalHit_Done
	encsetvar 3, 0
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Terapagos_TerastalHit_Zero
	encsubvar 1, 2
	goto EncScript_Terapagos_TerastalHit_Broken
EncScript_Terapagos_TerastalHit_Zero:
	encsetvar 1, 0
EncScript_Terapagos_TerastalHit_Broken:
	printstring STRINGID_ENCTERAPAGOSCRYSTALBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Terapagos_TerastalHit_Done:
	return

EncScript_Terapagos_EnterCataclysm::
	encsetvar 0, 2
	encsetvar 3, 0
	encsetvar 10, 1   @ arm the Terastal-punish siphon
	printstring STRINGID_ENCTERAPAGOSENTERSSTELLAR
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_TERAPAGOS_STELLAR, EncScript_Terapagos_EnterCataclysm_Done, B_ANIM_TERA_ACTIVATE
EncScript_Terapagos_EnterCataclysm_Done:
	return

// TERASTAL PUNISH (the player Terastallizing while SiphonUp is armed costs them Stellar Energy
// instead of being free value - guideline 8's anti-abuse beat) and STELLAR CROWN (the Phase-2
// exposed window, same shape as the Type Crystal but a bigger strip). One script, one latch -
// Stellar Cascade is a separate trigger/script (below) so it can fire on the same hit as this one.
EncScript_Terapagos_CataclysmHit::
	encsetvar 11, 1
	encjumpifvar CMP_NOT_EQUAL, 10, 1, EncScript_Terapagos_CataclysmHit_Crown
	encreadgimmick ENC_TARGET_PLAYER_LEFT, 5, 12  @ GIMMICK_TERA - a C enum, not asm-visible; literal
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Terapagos_CataclysmHit_Crown
	encsetvar 10, 0
	encjumpifvar CMP_GREATER_THAN, 1, 16, EncScript_Terapagos_CataclysmHit_PunishCapped
	encaddvar 1, 2
EncScript_Terapagos_CataclysmHit_PunishCapped:
	printstring STRINGID_ENCTERAPAGOSTERAPUNISH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
EncScript_Terapagos_CataclysmHit_Crown:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Terapagos_CataclysmHit_Done
	enccopyvar 12, 8
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE
	encjumpifvar CMP_LESS_THAN, 12, 5, EncScript_Terapagos_CataclysmHit_Done
	encsetvar 3, 0
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Terapagos_CataclysmHit_CrownZero
	encsubvar 1, 4
	goto EncScript_Terapagos_CataclysmHit_CrownBroken
EncScript_Terapagos_CataclysmHit_CrownZero:
	encsetvar 1, 0
EncScript_Terapagos_CataclysmHit_CrownBroken:
	printstring STRINGID_ENCTERAPAGOSCRYSTALBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Terapagos_CataclysmHit_Done:
	return

// STELLAR CASCADE. Six triggers in the .encounter (one per Active Type value, since a condition
// can test Event.MoveType against a literal but no command reads it into a variable) all point
// here: a boss hit whose type matches the rotated Active Type chains the counter; three chained
// hits caps it with a minor payload and resets. Handled2 is its own latch, independent of
// Handled, so this can still fire on the same event as the siphon/crown script above.
EncScript_Terapagos_CascadeHit::
	encsetvar 13, 1
	encaddvar 7, 1
	encjumpifvar CMP_LESS_THAN, 7, 3, EncScript_Terapagos_CascadeHit_Done
	encsetvar 7, 0
	printstring STRINGID_ENCTERAPAGOSCASCADEBREAKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
EncScript_Terapagos_CascadeHit_Done:
	return

// The catch window. Balls unlock only here (guideline 10); the engine's own catch-window damage
// guard takes over damage reduction the moment balls are allowed, so nothing else needs touching.
EncScript_Terapagos_CatchWindow::
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	printstring STRINGID_ENCTERAPAGOSCATCHWINDOW
	waitmessage B_WAIT_TIME_LONG
	return

// Once-per-turn scaffold. Also where the Active Type rotates and the exposed-window roll happens
// (Phases 1-2 only) - folding both into this once-a-turn script satisfies their own self-disable
// requirement for free, since the trigger only ever fires once a turn.
EncScript_Terapagos_TurnOpen::
	encsetvar 9, 1
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Terapagos_TurnOpen_Done   @ Dormant: no rotation/window yet
	call EncScript_Terapagos_RotateType
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Terapagos_TurnOpen_Tick
	encjumpifchance 30, EncScript_Terapagos_TurnOpen_Open
	goto EncScript_Terapagos_TurnOpen_Done
EncScript_Terapagos_TurnOpen_Open:
	call EncScript_Terapagos_OpenWindow
	printstring STRINGID_ENCTERAPAGOSCRYSTALWINDOW
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	goto EncScript_Terapagos_TurnOpen_Done
EncScript_Terapagos_TurnOpen_Tick:
	encsubvar 4, 1
EncScript_Terapagos_TurnOpen_Done:
	return

// Window expiry - Terapagos keeps whatever Stellar Energy it had; the player just missed the beat.
EncScript_Terapagos_WindowExpire::
	encsetvar 3, 0
	return

EncScript_Terapagos_TurnClose::
	encsetvar 9, 0
	encsetvar 11, 0
	encsetvar 13, 0
	return
