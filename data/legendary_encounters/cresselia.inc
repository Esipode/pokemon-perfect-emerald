// Cresselia, "The Lunar Guardian". Var indices, pinned by the always-true Conditions on
// EncScript_Cresselia_Intro:
// 0 Phase (0 Lunar Guardian / 1 Lucid Nightmare / 2 Eternal Dream / 3 Weakened), 1 Dream (0-5, starts
// 1), 2 LastDream (last ANNOUNCED band; 255 = force), 3 TurnGuard, 4 Peace (a peace source fired this
// turn; also the re-entry guard for most of them), 5 HpMark (boss HP% at turn open; ENC_SNAP_DAMAGE
// turns it into "lost this turn"), 6 PlayerHpMark (ENC_SNAP_RECOVERY turns it into "healed this
// turn"), 7 Construct (3 up this turn, 2-1 cooling down, 0 free), 8 AskCd, 9 Wake (Eternal Dream
// broken; window countdown), 10 Pulse (paces the standing line), 11 Fluct (Phase 1 lurch counter),
// 12 ReprisalGuard (Reprisal's own re-entry guard - Construct == 3 and Protected == 1 both latch for
// the whole checkpoint, so it can't share Peace, which an unrelated status move may already hold),
// 13 LastWeakType (0 none / 1 Bug / 2 Ghost / 3 Dark), 14 WeakGuard (the read's own re-entry guard).
//
// One hidden number runs the fight, and it moves the OPPOSITE way a boss meter usually does: peace
// feeds it, and a deep dream is a Cresselia the player cannot touch. Nothing in the text ever names
// it - the fight only ever describes the moonlight getting thicker.
//
// THE READ is the fight's one honest defence against a single super-effective move spammed on
// repeat: leaning on the same weakness type two turns running raises the Dream a step regardless of
// how much damage that turn did, so a repeat that also clears the aggression threshold below nets to
// a wash instead of a guaranteed drop. Varying the angle of attack is the counter.

// --- Shared subroutines (call/return) ---

// Every band's entry hands out stat stages, and a fight that crosses six bands would otherwise end on
// a pile of accumulated stages. So each band writes its stance ABSOLUTELY: everything back to neutral
// first, then its own. Only the three stats this fight ever touches need resetting.
EncScript_Cresselia_NeutralStance:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 6
	return

// Field teardown, split three ways so a band only ever removes what it does not want and what the
// band it is LEAVING actually raised. LastDream still holds the OLD band when these run, which is
// what every gate below reads; 255 is the intro's "nothing has been raised yet".
EncScript_Cresselia_ClearShelter:
	encjumpifvar CMP_EQUAL, 2, 255, EncScript_Cresselia_ClearShelter_Done
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Cresselia_ClearShelter_Done
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD
EncScript_Cresselia_ClearShelter_Done:
	return

EncScript_Cresselia_ClearMist:
	encjumpifvar CMP_EQUAL, 2, 255, EncScript_Cresselia_ClearMist_Done
	encjumpifvar CMP_LESS_THAN, 2, 4, EncScript_Cresselia_ClearMist_Done
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST
EncScript_Cresselia_ClearMist_Done:
	return

EncScript_Cresselia_ClearVeil:
	encjumpifvar CMP_EQUAL, 2, 255, EncScript_Cresselia_ClearVeil_Done
	encjumpifvar CMP_NOT_EQUAL, 2, 5, EncScript_Cresselia_ClearVeil_Done
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_AURORA_VEIL
EncScript_Cresselia_ClearVeil_Done:
	return

// The full teardown, for the only two bands (Waking, Moonlit) that want none of it.
EncScript_Cresselia_ClearDreamField:
	call EncScript_Cresselia_ClearShelter
	call EncScript_Cresselia_ClearMist
	call EncScript_Cresselia_ClearVeil
	encjumpifvar CMP_EQUAL, 2, 255, EncScript_Cresselia_ClearDreamField_Done
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Cresselia_ClearDreamField_Done
	removeterrain
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
EncScript_Cresselia_ClearDreamField_Done:
	return

// THE SOLE OWNER of the guard, the stance, the field and the cue. Nothing else writes any of them
// except the Wake window (which is why this bails out first while one is open, the same way Darkrai's
// ApplyNightmare defers to its own Shatter window) and Eternal Dream's own guard override right after
// calling this. Idempotent when the band did not move; safe to call from a phase script.
// Bands 2-5 each raise their own field pieces on real entry rather than leaving them for a neighbour
// to inherit - safe and idempotent, and correct regardless of how many bands a single turn crosses.
EncScript_Cresselia_ApplyDream:
	encjumpifvar CMP_GREATER_THAN, 9, 0, EncScript_Cresselia_ApplyDream_Done   @ Wake owns the guard
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Cresselia_Band5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Cresselia_Band4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Cresselia_Band3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Cresselia_Band2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Cresselia_Band1

// Dream 0 - WAKING. Below the guideline baseline on purpose: winning the meter war outright is meant
// to feel like the boss stopped being a wall, not like it got slightly easier.
	call EncScript_Cresselia_NeutralStance
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Cresselia_ApplyDream_Done
	call EncScript_Cresselia_ClearDreamField
	encsetvar 2, 0      @ LastDream
	encsetvar 10, 0     @ Pulse: the standing line restarts its count
	printstring STRINGID_ENCCRESSELIADREAM0
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Cresselia_Band1:
	call EncScript_Cresselia_NeutralStance
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Cresselia_ApplyDream_Done
	call EncScript_Cresselia_ClearDreamField
	encsetvar 2, 1
	encsetvar 10, 0
	printstring STRINGID_ENCCRESSELIADREAM1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// LUCID - the world starts to refuse harm. Misty Terrain protects the PLAYER (grounded); the boss's
// own Levitate means the Safeguard is what shelters Cresselia. Status immunity is earned here, not
// innate - below this band Cresselia can be slept or poisoned same as anything else.
EncScript_Cresselia_Band2:
	call EncScript_Cresselia_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encsetdamagereduction ENC_TARGET_BOSS, 89
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Cresselia_ApplyDream_Done
	call EncScript_Cresselia_ClearMist
	call EncScript_Cresselia_ClearVeil
	encsetvar 2, 2
	encsetvar 10, 0
	encsetterrain ENC_TERRAIN_MISTY
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 0
	printstring STRINGID_ENCCRESSELIADREAM2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	return

EncScript_Cresselia_Band3:
	call EncScript_Cresselia_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Cresselia_ApplyDream_Done
	call EncScript_Cresselia_ClearMist
	call EncScript_Cresselia_ClearVeil
	encsetvar 2, 3
	encsetvar 10, 0
	encsetterrain ENC_TERRAIN_MISTY
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 0
	printstring STRINGID_ENCCRESSELIADREAM3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_AQUA_RING_HEAL
	return

// REVERIE - Mist joins the field, shutting off Intimidate and stat-drop strategies. A privilege of
// the deep dream, not a property of the species.
EncScript_Cresselia_Band4:
	call EncScript_Cresselia_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 2
	encsetdamagereduction ENC_TARGET_BOSS, 93
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Cresselia_ApplyDream_Done
	call EncScript_Cresselia_ClearVeil
	encsetvar 2, 4
	encsetvar 10, 0
	encsetterrain ENC_TERRAIN_MISTY
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST, 0
	printstring STRINGID_ENCCRESSELIADREAM4
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	return

// PERFECT DREAM. The guard stops climbing at 95 on purpose - past that each point is worth a 20-50%
// swing in effective bulk and the fight stops being tunable. Aurora Veil arriving without hail is the
// one moment the fight is meant to feel genuinely unfair.
EncScript_Cresselia_Band5:
	call EncScript_Cresselia_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encsetdamagereduction ENC_TARGET_BOSS, 95
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Cresselia_ApplyDream_Done
	encsetvar 2, 5
	encsetvar 10, 0
	encsetterrain ENC_TERRAIN_MISTY
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_AURORA_VEIL, 0
	printstring STRINGID_ENCCRESSELIADREAM5
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAINBOW
EncScript_Cresselia_ApplyDream_Done:
	return

EncScript_Cresselia_RaiseDream:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Cresselia_RaiseDream_Done
	encaddvar 1, 1
	call EncScript_Cresselia_ApplyDream
EncScript_Cresselia_RaiseDream_Done:
	return

EncScript_Cresselia_LowerDream:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Cresselia_LowerDream_Done
	encsubvar 1, 1
	call EncScript_Cresselia_ApplyDream
EncScript_Cresselia_LowerDream_Done:
	return

// The passive heal, silent and unobtrusive by design - encchangehp already animates its own health
// bar, and a fight this long cannot afford a message box for every tick of it. Reverie and Perfect
// Dream heal the player too; below that only Cresselia benefits.
EncScript_Cresselia_Moonlight:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Cresselia_Moonlight_5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Cresselia_Moonlight_4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Cresselia_Moonlight_3
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Cresselia_Moonlight_Done
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	return
EncScript_Cresselia_Moonlight_3:
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	return
// CHANGE_HP asserts on a fainted/absent target, and by the time this runs (TurnClose, after the
// turn's combat) the player's battler can legitimately be down and not yet replaced - so every
// player-targeting heal here is guarded, boss side unaffected.
EncScript_Cresselia_Moonlight_4:
	encchangehp ENC_TARGET_BOSS, 4, ENC_AMOUNT_PERCENT
	jumpiffainted BS_PLAYER1, TRUE, EncScript_Cresselia_Moonlight_Done
	encchangehp ENC_TARGET_PLAYER_LEFT, 4, ENC_AMOUNT_PERCENT
	return
EncScript_Cresselia_Moonlight_5:
	encchangehp ENC_TARGET_BOSS, 5, ENC_AMOUNT_PERCENT
	jumpiffainted BS_PLAYER1, TRUE, EncScript_Cresselia_Moonlight_Done
	encchangehp ENC_TARGET_PLAYER_LEFT, 5, ENC_AMOUNT_PERCENT
EncScript_Cresselia_Moonlight_Done:
	return

// THE RECURRING CHANNEL (guideline 14). A band crossing announces itself once; this is what a player
// who missed it eventually reads. Paced by Pulse to every other turn, reset by every band change so
// it never lands the turn right after a loud one. Silent at Dream 0 - nothing there to announce.
EncScript_Cresselia_Standing:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Cresselia_Standing_Done
	encaddvar 10, 1     @ Pulse
	encjumpifvar CMP_LESS_THAN, 10, 2, EncScript_Cresselia_Standing_Done
	encsetvar 10, 0
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Cresselia_Standing_Deep
	printstring STRINGID_ENCCRESSELIASTANDINGSHALLOW
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Cresselia_Standing_Deep:
	printstring STRINGID_ENCCRESSELIASTANDINGDEEP
	waitmessage B_WAIT_TIME_SHORT
EncScript_Cresselia_Standing_Done:
	return

// Phase 1's instability. Two of three lurches feed the dream and one fights it, but the distortion is
// the only thing in the fight that strips Cresselia's own field, and it is a genuine window.
EncScript_Cresselia_Lurch:
	encjumpifchance 33, EncScript_Cresselia_Lurch_Distort
	encjumpifchance 50, EncScript_Cresselia_Lurch_Restore
	printstring STRINGID_ENCCRESSELIALURCHWARM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_PERCENT
	jumpiffainted BS_PLAYER1, TRUE, EncScript_Cresselia_Lurch_Warm_Dream
	encchangehp ENC_TARGET_PLAYER_LEFT, 5, ENC_AMOUNT_PERCENT
EncScript_Cresselia_Lurch_Warm_Dream:
	call EncScript_Cresselia_RaiseDream
	return
EncScript_Cresselia_Lurch_Distort:
	printstring STRINGID_ENCCRESSELIALURCHDISTORT
	waitmessage B_WAIT_TIME_SHORT
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Cresselia_Lurch_Distort_Hazards
EncScript_Cresselia_Lurch_Distort_Hazards:
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Cresselia_Lurch_Distort_Done
EncScript_Cresselia_Lurch_Distort_Done:
	call EncScript_Cresselia_LowerDream
	return
EncScript_Cresselia_Lurch_Restore:
	printstring STRINGID_ENCCRESSELIALURCHRESTORE
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	call EncScript_Cresselia_RaiseDream
	return

// Ticked at OnTurnEnd, Phase 1 only. A Fluct of 3 is "every third turn" - resolved one call away from
// TurnClose's own dispatch rather than inline, so the lurch reads as its own beat.
EncScript_Cresselia_FluctTick:
	encaddvar 11, 1     @ Fluct
	encjumpifvar CMP_LESS_THAN, 11, 3, EncScript_Cresselia_FluctTick_Done
	encsetvar 11, 0
	call EncScript_Cresselia_Lurch
EncScript_Cresselia_FluctTick_Done:
	return

// The construct's cooldown. Always seeded at 3 when one forms (Reprisal's Construct == 3 read relies
// on that staying a fixed sentinel); Eternal Dream halves the cadence by ticking it down by 2 instead
// of 1, so it reaches 0 - and a new construct is free to form - every other turn instead of every
// third.
EncScript_Cresselia_ConstructTick:
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Cresselia_ConstructTick_Done
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Cresselia_ConstructTick_Slow
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Cresselia_ConstructTick_Zero
	encsubvar 7, 2
	return
EncScript_Cresselia_ConstructTick_Zero:
	encsetvar 7, 0
	return
EncScript_Cresselia_ConstructTick_Slow:
	encsubvar 7, 1
EncScript_Cresselia_ConstructTick_Done:
	return

// THE BREAK. Shared by the type-based trigger and Eternal Dream's own damage door, so both routes
// into the window look identical. encsetrecharge with 2 here (OnMoveEnd/OnTurnEnd) costs NEXT turn's
// action - it costs THIS turn's only when set at OnTurnStart.
EncScript_Cresselia_Break:
	printstring STRINGID_ENCCRESSELIAWAKES
	waitmessage B_WAIT_TIME_LONG
	encsetvar 9, 3      @ Wake
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetrecharge ENC_TARGET_BOSS, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return

// --- Trigger scripts ---

EncScript_Cresselia_Intro::
	printstring STRINGID_ENCCRESSELIAINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCCRESSELIAINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1      @ Dream: the battle opens already dreaming gently
	encsetvar 2, 255    @ LastDream
	call EncScript_Cresselia_ApplyDream
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard and Peace are
// cleared immediately rather than at the tail, so no branch out of this script can leave either wrong.
EncScript_Cresselia_TurnOpen::
	flushtextbox
	encsetvar 3, 1      @ TurnGuard
	encsetvar 4, 0      @ Peace
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Cresselia_TurnOpen_Done   @ Weakened: inert
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_SET         @ HpMark
	jumpiffainted BS_PLAYER1, TRUE, EncScript_Cresselia_TurnOpen_AskCd
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 6, ENC_SNAP_SET  @ PlayerHpMark
EncScript_Cresselia_TurnOpen_AskCd:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Cresselia_TurnOpen_Done   @ AskCd
	encsubvar 8, 1
EncScript_Cresselia_TurnOpen_Done:
	return

// LUCID DREAMING. Wakefulness is not spent on the offer itself: AskCd goes to 3 on both paths, so
// declining is a timing decision rather than a punishment.
EncScript_Cresselia_Prompt::
	encaskyesno STRINGID_ENCCRESSELIAOFFER, EncScript_Cresselia_Accept
	encsetvar 8, 3      @ AskCd
	printstring STRINGID_ENCCRESSELIARESIST
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Cresselia_LowerDream
	return

// Courage is the joke the fight is telling: the boon that best equips the player to break the dream
// is the boon that feeds it.
EncScript_Cresselia_Accept:
	encsetvar 8, 3      @ AskCd
	call EncScript_Cresselia_RaiseDream
	encjumpifchance 33, EncScript_Cresselia_Accept_Courage
	encjumpifchance 50, EncScript_Cresselia_Accept_Shelter
	printstring STRINGID_ENCCRESSELIABOONREST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_WISH_HEAL
	encchangehp ENC_TARGET_PLAYER_LEFT, 35, ENC_AMOUNT_PERCENT
	curestatus BS_PLAYER1
	updatestatusicon BS_PLAYER1
	return
EncScript_Cresselia_Accept_Courage:
	printstring STRINGID_ENCCRESSELIABOONCOURAGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, 2
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, 2
	return
EncScript_Cresselia_Accept_Shelter:
	printstring STRINGID_ENCCRESSELIABOONSHELTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	encsetsidestatus ENC_TARGET_PLAYER_LEFT, ENC_SIDE_REFLECT, 5
	encsetsidestatus ENC_TARGET_PLAYER_LEFT, ENC_SIDE_LIGHT_SCREEN, 5
	return

// DREAM CONSTRUCTS. A real Protect via encsetprotect, so Feint, never-miss moves and contact
// punishment all resolve against it exactly as they would a real Protect - the counter-play is real
// and already in the player's hands.
EncScript_Cresselia_Construct::
	printstring STRINGID_ENCCRESSELIACONSTRUCT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_PROTECT
	encsetprotect ENC_TARGET_BOSS
	encsetvar 7, 3      @ Construct: always seeded at 3; ConstructTick's phase-2 double-step is what
	return              @ shortens the cadence, not this value

// The reprisal, read at OnTurnEnd from the whole-turn Protected latch - gProtectStructs is cleared
// before end-of-turn effects run, so this is the one place that can still answer "did it protect".
// ReprisalGuard is its own self-disabling write, kept separate from Peace: Construct == 3 and
// Protected == 1 both latch for the whole checkpoint, so sharing Peace would let an earlier status
// move this same turn silently swallow the reprisal.
EncScript_Cresselia_Reprisal::
	encsetvar 12, 1     @ ReprisalGuard
	encsetvar 4, 1      @ Peace: the construct earned its keep too
	jumpiffainted BS_PLAYER1, TRUE, EncScript_Cresselia_Reprisal_Done   @ nothing left to strike back at
	printstring STRINGID_ENCCRESSELIAREPRISAL
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_PSYCHIC
	encchangehp ENC_TARGET_PLAYER_LEFT, -8, ENC_AMOUNT_PERCENT
EncScript_Cresselia_Reprisal_Done:
	return

EncScript_Cresselia_Cleanse::
	encsetvar 4, 1      @ Peace
	printstring STRINGID_ENCCRESSELIACLEANSE
	waitmessage B_WAIT_TIME_SHORT
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	return

// Silent by design - a status move already prints its own battle text, and a message on top of every
// single one across a long fight would bury the cues that matter.
EncScript_Cresselia_PeaceStatusMove::
	encsetvar 4, 1      @ Peace
	return

// THE READ. Three entry points, one per weakness type, each pinning its own literal because a
// condition can only compare a variable against a literal - the same shape Darkrai's category read
// uses. LastWeakType is cleared when it fires, so leaning on one type costs a step every SECOND turn
// of repetition, not every turn.
EncScript_Cresselia_ReadBug::
	encsetvar 14, 1     @ WeakGuard
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Cresselia_Read_Same
	encsetvar 13, 1     @ LastWeakType: Bug
	return

EncScript_Cresselia_ReadGhost::
	encsetvar 14, 1
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Cresselia_Read_Same
	encsetvar 13, 2     @ LastWeakType: Ghost
	return

EncScript_Cresselia_ReadDark::
	encsetvar 14, 1
	encjumpifvar CMP_EQUAL, 13, 3, EncScript_Cresselia_Read_Same
	encsetvar 13, 3     @ LastWeakType: Dark
	return

EncScript_Cresselia_Read_Same:
	encsetvar 13, 0     @ LastWeakType: cleared, so a third repeat in a row costs again
	printstring STRINGID_ENCCRESSELIAREAD
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Cresselia_RaiseDream
	return

EncScript_Cresselia_Wake::
	call EncScript_Cresselia_Break
	return

// Runs last so it sees the finished turn. TurnGuard is cleared first - this trigger's own
// self-disabling condition.
EncScript_Cresselia_TurnClose::
	encsetvar 3, 0      @ TurnGuard
	encsetvar 12, 0     @ ReprisalGuard
	encsetvar 14, 0     @ WeakGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Cresselia_TurnClose_Done          @ Weakened: inert
	call EncScript_Cresselia_ConstructTick
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cresselia_TurnClose_Eternal
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cresselia_TurnClose_Fluct
	goto EncScript_Cresselia_TurnClose_Ledger
EncScript_Cresselia_TurnClose_Fluct:
	call EncScript_Cresselia_FluctTick

// THE LEDGER. The aggression read is evaluated first and short-circuits: a turn where the player hit
// hard is a turn the dream loses, whatever else happened in it. A quiet turn (under 2% taken) raises
// it outright; anything in between only rises if a peace source fired, and the player's own recovery
// is read here directly rather than through a trigger, off the mark TurnOpen took.
EncScript_Cresselia_TurnClose_Ledger:
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE          @ HpMark
	encjumpifvar CMP_GREATER_THAN, 5, 5, EncScript_Cresselia_TurnClose_Down
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Cresselia_TurnClose_Up
	jumpiffainted BS_PLAYER1, TRUE, EncScript_Cresselia_TurnClose_CheckPeace   @ no recovery to read
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 6, ENC_SNAP_RECOVERY, EncScript_Cresselia_TurnClose_CheckPeace
	encjumpifvar CMP_LESS_THAN, 6, 8, EncScript_Cresselia_TurnClose_CheckPeace
	encsetvar 4, 1      @ Peace
EncScript_Cresselia_TurnClose_CheckPeace:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Cresselia_TurnClose_Up
	goto EncScript_Cresselia_TurnClose_Moonlight
EncScript_Cresselia_TurnClose_Down:
	call EncScript_Cresselia_LowerDream
	goto EncScript_Cresselia_TurnClose_Moonlight
EncScript_Cresselia_TurnClose_Up:
	call EncScript_Cresselia_RaiseDream
EncScript_Cresselia_TurnClose_Moonlight:
	call EncScript_Cresselia_Moonlight
	goto EncScript_Cresselia_TurnClose_Standing

// ETERNAL DREAM. Dream stays pinned at 5 and ApplyDream is a no-op turn over turn (band already
// applied), except that its guard write (95) is not this phase's guard (93) - overridden right after,
// and only while the Wake window is shut. The window suspends both the heal and the second door.
EncScript_Cresselia_TurnClose_Eternal:
	call EncScript_Cresselia_ApplyDream                          @ no-op while Wake > 0
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Cresselia_TurnClose_EternalHeal   @ Wake
	encsubvar 9, 1
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Cresselia_TurnClose_Standing
	printstring STRINGID_ENCCRESSELIAWINDOWCLOSES
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Cresselia_TurnClose_Standing
EncScript_Cresselia_TurnClose_EternalHeal:
	encsetdamagereduction ENC_TARGET_BOSS, 93     @ Eternal Dream's own guard, below ApplyDream's 95
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	@ The second door: for a player with no Bug/Ghost/Dark coverage, a genuinely heavy hit still
	@ breaks the dream. Worse than the type route on purpose - guard 93 demands a lot of it.
	encsnapshothp ENC_TARGET_BOSS, 5, ENC_SNAP_DAMAGE, EncScript_Cresselia_TurnClose_Standing
	encjumpifvar CMP_LESS_THAN, 5, 6, EncScript_Cresselia_TurnClose_Standing
	call EncScript_Cresselia_Break

EncScript_Cresselia_TurnClose_Standing:
	call EncScript_Cresselia_Standing
EncScript_Cresselia_TurnClose_Done:
	return

// --- Phases ---

// 50% - LUCID NIGHTMARE. The dream is no longer entirely under Cresselia's control: the Phase 1
// instability roll comes online, and Aurora Beam gives way to Future Sight.
EncScript_Cresselia_LucidNightmare::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCCRESSELIAPHASE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	encsetmove ENC_TARGET_BOSS, 3, MOVE_FUTURE_SIGHT
	encsetvar 11, 0     @ Fluct
	return

// 25% - ETERNAL DREAM. The meter stops: Dream is pinned at 5 and the ledger goes inert from here.
// Calm Mind gives way to Moonlight, and the AI will use it - that is the pressure.
EncScript_Cresselia_EternalDream::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCCRESSELIAPHASE2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	encsetmove ENC_TARGET_BOSS, 2, MOVE_MOONLIGHT
	encsetvar 9, 0      @ Wake
	encsetvar 1, 5      @ Dream, pinned from here on
	call EncScript_Cresselia_ApplyDream
	encsetdamagereduction ENC_TARGET_BOSS, 93     @ Eternal Dream's own guard, below ApplyDream's 95
	return

// 10% - the catch window, and the mirror image of the Properties: block plus everything the fight
// built. Phase 3 retires every other trigger - the ledger, the prompt, the constructs and the lurches
// all stop here. The window itself is identical on every path per guideline 10.
EncScript_Cresselia_Weakened::
	encsetvar 0, 3      @ Phase
	call EncScript_Cresselia_ClearDreamField
	encsetvar 2, 255    @ LastDream: nothing of the dream is standing any more
	call EncScript_Cresselia_NeutralStance
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99      @ the value the catch-window damage guard restores
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCCRESSELIAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	return
