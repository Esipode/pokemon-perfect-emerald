// Shaymin, "The Gratitude Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Shaymin_Intro:
// 0 Phase (0 Gratitude Pokemon / 1 Gratitude / 2 Seed Flare / 3 Weakened), 1 Garden (0-10, starts 5),
// 2 LastBand (last ANNOUNCED band index; 255 = force), 3 Swing (the turn's ledger, opens at 10),
// 4 Forme (0 Land / 1 Sky), 5 TurnGuard, 6 PlayerHpMark, 7 StatusMark, 8 PlayerMoveGuard,
// 9 BossMoveGuard, 10 StatusGuard, 11 FlareClock, 12 Verdict (0 none / 1 gratitude / 2 indifferent
// / 3 distress), 13 RecovGuard, 14 Pulse (paces the standing line).
//
// One number runs the whole fight. Tending the Garden pushes it up, harming it pushes it down, and
// the guard follows: 94 at Barren, 74 at Full Bloom - the same attack lands more than four times as
// hard in a garden the player kept alive. Nothing says so. The fight only ever describes the field.

// --- Shared subroutines (call/return) ---

// Every band's entry hands out stat stages, and a fight that crosses six bands would otherwise end
// on a pile of accumulated +1s. So each band writes its stance ABSOLUTELY: everything back to
// neutral first, then its own. encchangestat clamps at the stage bounds, so -12 lands on neutral
// from wherever the stage had drifted, and a band can be re-entered from either direction.
EncScript_Shaymin_NeutralStance:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	return

// Field teardown, split three ways so a band only ever removes what it does not want and what the
// band it is leaving actually raised. LastBand still holds the OLD band when these run, which is what
// every gate below reads; 255 is the intro's "nothing has been raised yet".
// Only bands 5 and 6 raise the Safeguard.
EncScript_Shaymin_ClearShield:
	encjumpifvar CMP_EQUAL, 2, 255, EncScript_Shaymin_ClearShield_Done
	encjumpifvar CMP_LESS_THAN, 2, 5, EncScript_Shaymin_ClearShield_Done
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SAFEGUARD
EncScript_Shaymin_ClearShield_Done:
	return

// The Safeguard plus the terrain bands 4-6 raise. Called only by the bands that want neither, so the
// three Bloom bands hand the terrain between themselves without a removal and a repaint every step.
EncScript_Shaymin_ClearGarden:
	call EncScript_Shaymin_ClearShield
	encjumpifvar CMP_EQUAL, 2, 255, EncScript_Shaymin_ClearGarden_Done
	encjumpifvar CMP_LESS_THAN, 2, 4, EncScript_Shaymin_ClearGarden_Done
	removeterrain
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
EncScript_Shaymin_ClearGarden_Done:
	return

// Band 0's fire, put out on the way out of it. The distress beat's own timed Sea of Fire is not
// covered here on purpose - it expires on its own, and is not the band's to take back.
EncScript_Shaymin_ClearBurn:
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Shaymin_ClearBurn_Done
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
EncScript_Shaymin_ClearBurn_Done:
	return

// LAND <-> SKY. Not an HP threshold: the Garden decides it, and the player can change their mind.
// The bands overlap deliberately - Sky at 2 and below, Land at 4 and above - so Garden 3 is a gap
// where Shaymin keeps whatever forme it arrived in and the transformation cannot flap turn over turn
// at a boundary. Checked on every ApplyGarden rather than only on a band change, because the whole
// hysteresis gap sits inside one band.
EncScript_Shaymin_Forme:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Shaymin_Forme_Ground
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Shaymin_Forme_Done
	goto EncScript_Shaymin_ToSky
EncScript_Shaymin_Forme_Ground:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Shaymin_Forme_Done
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Shaymin_Forme_Done
	goto EncScript_Shaymin_ToLand
EncScript_Shaymin_Forme_Done:
	return

// Sky Forme trades Shaymin's bulk for its speed - 100/100 defences down to 75/75 - and swaps Natural
// Cure for Serene Grace, which gives the Air Slash it picks up here a 60% flinch chance. The frailer,
// faster, meaner Shaymin is the one the player made.
// The move swap sits at the form change's fail label so slot 2 is correct either way.
EncScript_Shaymin_ToSky:
	encsetvar 4, 1    @ Forme
	encformchange ENC_TARGET_BOSS, SPECIES_SHAYMIN_SKY, EncScript_Shaymin_ToSky_Move, B_ANIM_FORM_CHANGE
EncScript_Shaymin_ToSky_Move:
	encsetmove ENC_TARGET_BOSS, 1, MOVE_AIR_SLASH
	printstring STRINGID_ENCSHAYMINTOSKY
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Shaymin_ToLand:
	encsetvar 4, 0    @ Forme
	encformchange ENC_TARGET_BOSS, SPECIES_SHAYMIN, EncScript_Shaymin_ToLand_Move, B_ANIM_FORM_CHANGE
EncScript_Shaymin_ToLand_Move:
	encsetmove ENC_TARGET_BOSS, 1, MOVE_EARTH_POWER
	printstring STRINGID_ENCSHAYMINTOLAND
	waitmessage B_WAIT_TIME_LONG
	return

// THE SOLE OWNER of the forme, the stance, the guard, the field and the cue. Nothing else writes any
// of them, which is what makes it safe to call from a phase script and idempotent when the band did
// not move. Called once per turn from TurnClose and once from each phase transition.
// Stance and guard are re-applied every call: both are absolute writes, and re-stating them after a
// form change is what keeps the ordering rule simple. Field and dialogue run only on a real band
// change, comparing against LastBand - the "store the last announced value" pattern a hidden mechanic
// needs, since a single message is trivially missed.
EncScript_Shaymin_ApplyGarden:
	call EncScript_Shaymin_Forme
	encjumpifvar CMP_GREATER_THAN, 1, 9, EncScript_Shaymin_Band6
	encjumpifvar CMP_GREATER_THAN, 1, 7, EncScript_Shaymin_Band5
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Shaymin_Band4
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Shaymin_Band3
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Shaymin_Band2
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Shaymin_Band1
	goto EncScript_Shaymin_Band0

// FULL BLOOM. Guard 74 - Shaymin takes more than four times what it takes at Barren - and its Sp. Atk
// is on the floor. Both halves of gratitude at once: easier to hurt, and it stops hitting so hard.
EncScript_Shaymin_Band6:
	call EncScript_Shaymin_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -3
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -2
	encsetdamagereduction ENC_TARGET_BOSS, 74
	encjumpifvar CMP_EQUAL, 2, 6, EncScript_Shaymin_Band_Done
	call EncScript_Shaymin_ClearBurn
	encsetvar 2, 6    @ LastBand
	encsetvar 14, 0   @ Pulse: the standing line restarts its count
	encsetterrain ENC_TERRAIN_GRASSY
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SAFEGUARD, 0
	printstring STRINGID_ENCSHAYMINBLOOM3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAINBOW
	return

EncScript_Shaymin_Band5:
	call EncScript_Shaymin_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -1
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Shaymin_Band_Done
	call EncScript_Shaymin_ClearBurn
	encsetvar 2, 5    @ LastBand
	encsetvar 14, 0   @ Pulse: the standing line restarts its count
	encsetterrain ENC_TERRAIN_GRASSY
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SAFEGUARD, 0
	printstring STRINGID_ENCSHAYMINBLOOM2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	return

// The first Bloom band, and the one that opens the terrain. Grassy Terrain heals both grounded sides
// 1/16 a turn, which the engine does for free - a garden that tends the player as much as Shaymin.
EncScript_Shaymin_Band4:
	call EncScript_Shaymin_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -1
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Shaymin_Band_Done
	call EncScript_Shaymin_ClearBurn
	call EncScript_Shaymin_ClearShield
	encsetvar 2, 4    @ LastBand
	encsetvar 14, 0   @ Pulse: the standing line restarts its count
	encsetterrain ENC_TERRAIN_GRASSY
	printstring STRINGID_ENCSHAYMINBLOOM1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_AQUA_RING_HEAL
	return

EncScript_Shaymin_Band3:
	call EncScript_Shaymin_NeutralStance
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Shaymin_Band_Done
	call EncScript_Shaymin_ClearBurn
	call EncScript_Shaymin_ClearGarden
	encsetvar 2, 3    @ LastBand
	encsetvar 14, 0   @ Pulse: the standing line restarts its count
	printstring STRINGID_ENCSHAYMINNEUTRAL
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Shaymin_Band2:
	call EncScript_Shaymin_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Shaymin_Band_Done
	call EncScript_Shaymin_ClearBurn
	call EncScript_Shaymin_ClearGarden
	encsetvar 2, 2    @ LastBand
	encsetvar 14, 0   @ Pulse: the standing line restarts its count
	printstring STRINGID_ENCSHAYMINBLIGHT1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

EncScript_Shaymin_Band1:
	call EncScript_Shaymin_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Shaymin_Band_Done
	call EncScript_Shaymin_ClearBurn
	call EncScript_Shaymin_ClearGarden
	encsetvar 2, 1    @ LastBand
	encsetvar 14, 0   @ Pulse: the standing line restarts its count
	printstring STRINGID_ENCSHAYMINBLIGHT2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// BARREN. The guard stops climbing at 94 on purpose - past that the last few points are worth 25-50%
// swings each and the fight stops being tunable. Barren expresses itself through offence and a
// burning field instead, which the player can actually see.
EncScript_Shaymin_Band0:
	call EncScript_Shaymin_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 3
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Shaymin_Band_Done
	call EncScript_Shaymin_ClearGarden
	encsetvar 2, 0    @ LastBand
	encsetvar 14, 0   @ Pulse: the standing line restarts its count
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 0
	printstring STRINGID_ENCSHAYMINBLIGHT3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
EncScript_Shaymin_Band_Done:
	return

// The recurring channel guideline 14 asks for: a band transition is loud but fires once, so a short
// line keeps running underneath it for as long as the field is not neutral. It is what a player who
// missed the transition eventually reads.
// Paced by Pulse to every other turn, and Pulse is zeroed by every band change - so the standing line
// never lands on the turn after a loud one, and a long fight is not one message box per turn.
EncScript_Shaymin_Standing:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Shaymin_Standing_Done
	encaddvar 14, 1     @ Pulse
	encjumpifvar CMP_LESS_THAN, 14, 2, EncScript_Shaymin_Standing_Done
	encsetvar 14, 0
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Shaymin_Standing_Bloom
	printstring STRINGID_ENCSHAYMINSTANDINGBLIGHT
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Shaymin_Standing_Bloom:
	printstring STRINGID_ENCSHAYMINSTANDINGBLOOM
	waitmessage B_WAIT_TIME_SHORT
EncScript_Shaymin_Standing_Done:
	return

// The Phase 2 countdown. Ticked here and resolved at OnTurnEnd, one checkpoint away, so a real turn
// passes between steps - a timer that both ticks and resolves at the same checkpoint chases itself
// through every state in one dispatch. Parked at 1 rather than 0 so the resolution trigger is what
// re-arms it.
EncScript_Shaymin_FlareTick:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Shaymin_FlareTick_Done
	encjumpifvar CMP_LESS_THAN, 11, 2, EncScript_Shaymin_FlareTick_Done
	encsubvar 11, 1
EncScript_Shaymin_FlareTick_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only the three that do not start at zero are seeded.
// LastBand at 255 forces the first ApplyGarden to announce whatever band it lands in.
EncScript_Shaymin_Intro::
	printstring STRINGID_ENCSHAYMININTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCSHAYMININTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 5      @ Garden
	encsetvar 2, 255    @ LastBand
	encsetvar 3, 10     @ Swing
	call EncScript_Shaymin_ApplyGarden
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set and every
// re-entry guard cleared immediately rather than at the tail, so no branch out of this script can
// leave one wrong.
EncScript_Shaymin_TurnOpen::
	flushtextbox
	encsetvar 5, 1      @ TurnGuard
	encsetvar 3, 10     @ Swing: the turn's ledger opens level
	encsetvar 7, 0      @ StatusMark
	encsetvar 8, 0      @ PlayerMoveGuard
	encsetvar 9, 0      @ BossMoveGuard
	encsetvar 10, 0     @ StatusGuard
	encsetvar 13, 0     @ RecovGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Shaymin_TurnOpen_Done   @ Weakened: inert
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 6, ENC_SNAP_SET
	call EncScript_Shaymin_FlareTick
	call EncScript_Shaymin_Standing
EncScript_Shaymin_TurnOpen_Done:
	return

// A script cannot read a battler's status, so the mark is set by its own trigger's condition and
// this only records the answer. Self-disabling.
EncScript_Shaymin_MarkStatus::
	encsetvar 7, 1      @ StatusMark
	return

// --- The ledger ---

EncScript_Shaymin_TendGrassPlayer::
	encsetvar 8, 1      @ PlayerMoveGuard
	encaddvar 3, 1      @ Swing
	return

EncScript_Shaymin_TendGrassBoss::
	encsetvar 9, 1      @ BossMoveGuard
	encaddvar 3, 1      @ Swing
	return

EncScript_Shaymin_HarmBurnRot::
	encsetvar 8, 1      @ PlayerMoveGuard
	encsubvar 3, 1      @ Swing
	return

// A hazard move that fails - Stealth Rock already up - still counts. Intent is the signal here, and
// the engine exposes no side-status read to check against.
EncScript_Shaymin_HarmHazards::
	encsetvar 8, 1      @ PlayerMoveGuard
	encsubvar 3, 1      @ Swing
	return

// The debit that ticks every turn the status persists. Toxic and sleep both function completely; they
// just cost a step of Garden per turn, and the guard climbs while they run.
EncScript_Shaymin_HarmStatusStanding::
	encsetvar 10, 1     @ StatusGuard
	encsubvar 3, 1      @ Swing
	return

EncScript_Shaymin_TendCured::
	encsetvar 7, 0      @ StatusMark: self-disabling
	encaddvar 3, 1      @ Swing
	return

// encsnapshothp's failLabel is the test as well as the record: nothing recovered falls straight
// through and the turn's ledger is untouched. ENC_SNAP_RECOVERY leaves the recovery in the mark as a
// percentage, so the threshold below is a plain literal test on it.
// The 7% floor is what keeps the garden from tending itself. Grassy Terrain heals 1/16 - 6% after the
// integer division - to both grounded sides every turn, so without it the Bloom bands would credit a
// Tend every turn purely for having raised the terrain, and the meter would ratchet to Full Bloom on
// its own. Leftovers-sized chip is excluded by the same number. Anything a player actually spends a
// turn or an item on clears it comfortably.
EncScript_Shaymin_TendRecovery::
	encsetvar 13, 1     @ RecovGuard
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 6, ENC_SNAP_RECOVERY, EncScript_Shaymin_TendRecovery_Done
	encjumpifvar CMP_LESS_THAN, 6, 7, EncScript_Shaymin_TendRecovery_Done
	encaddvar 3, 1      @ Swing
EncScript_Shaymin_TendRecovery_Done:
	return

// The turn's verdict. Swing opened at 10; the Verdict bias from the 50% beat is added last, then the
// Garden moves one step toward whichever side is ahead and a tie holds it still. The 0 and 10 guards
// are explicit rather than a clamp afterwards - a byte underflowing from 0 to 255 would put the fight
// in a band that does not exist.
EncScript_Shaymin_TurnClose::
	encsetvar 5, 0      @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Shaymin_TurnClose_Done   @ Weakened: inert
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Shaymin_TurnClose_BiasTend
	encjumpifvar CMP_EQUAL, 12, 3, EncScript_Shaymin_TurnClose_BiasHarm
	goto EncScript_Shaymin_TurnClose_Resolve
EncScript_Shaymin_TurnClose_BiasTend:
	encaddvar 3, 1      @ Swing: the field tends itself
	goto EncScript_Shaymin_TurnClose_Resolve
EncScript_Shaymin_TurnClose_BiasHarm:
	encsubvar 3, 1      @ Swing: the field rots by itself
EncScript_Shaymin_TurnClose_Resolve:
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Shaymin_TurnClose_Up
	encjumpifvar CMP_LESS_THAN, 3, 10, EncScript_Shaymin_TurnClose_Down
	goto EncScript_Shaymin_TurnClose_Apply
EncScript_Shaymin_TurnClose_Up:
	encjumpifvar CMP_EQUAL, 1, 10, EncScript_Shaymin_TurnClose_Apply
	encaddvar 1, 1      @ Garden
	goto EncScript_Shaymin_TurnClose_Apply
EncScript_Shaymin_TurnClose_Down:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Shaymin_TurnClose_Apply
	encsubvar 1, 1      @ Garden
EncScript_Shaymin_TurnClose_Apply:
	call EncScript_Shaymin_ApplyGarden
EncScript_Shaymin_TurnClose_Done:
	return

// --- Phases ---

// 50% - GRATITUDE. Shaymin reads the Garden ONCE and commits a bias, and from here the field tends or
// rots itself every turn for the rest of the fight. That is the load-bearing part: the second half of
// the battle keeps moving in the direction the first half earned, while still leaving the player able
// to fight the current - two Tend sources in a turn beats a one-step Harm bias, so a distressed
// garden can be brought back, slowly, by someone who works out how.
// Clearing Shaymin's status is the natural reboot point that caps what a status lock is worth.
EncScript_Shaymin_Gratitude::
	encsetvar 0, 1      @ Phase
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Shaymin_Gratitude_Bloom
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Shaymin_Gratitude_Distress
	encsetvar 12, 2     @ Verdict: indifferent - no bias, but a line and a bump rather than silence
	printstring STRINGID_ENCSHAYMININDIFFERENT
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	goto EncScript_Shaymin_Gratitude_Apply
// The blessing is mutual and mostly the player's. Read straight, gratitude making the peaceful path
// harder would inverse the lesson the fight is teaching; Shaymin heals a little, the player is healed
// and cleansed, and the field starts tending itself.
EncScript_Shaymin_Gratitude_Bloom:
	encsetvar 12, 1     @ Verdict: gratitude - +1 Tend every turn from here
	printstring STRINGID_ENCSHAYMINGRATITUDE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAINBOW
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_ALL_FOES, 10, ENC_AMOUNT_PERCENT
	curestatus BS_PLAYER1
	updatestatusicon BS_PLAYER1
	playanimation BS_PLAYER1, B_ANIM_INGRAIN_HEAL
	printstring STRINGID_ENCSHAYMINGRATITUDEGIFT
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Shaymin_Gratitude_Apply
// Driven into the withered band rather than form-changed directly, so ApplyGarden stays the only
// writer of the forme and the Sky Forme arrives through the same door it always does.
EncScript_Shaymin_Gratitude_Distress:
	encsetvar 12, 3     @ Verdict: distress - +1 Harm every turn from here
	printstring STRINGID_ENCSHAYMINDISTRESS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	encsetvar 1, 2      @ Garden
	call EncScript_Shaymin_ApplyGarden
// The burning field is raised AFTER the band lands: a Shaymin already at Barren would otherwise have
// its own permanent Sea of Fire cleared by the band change out of 0, taking this one with it.
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 5
	printstring STRINGID_ENCSHAYMINDISTRESSFIELD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	return
EncScript_Shaymin_Gratitude_Apply:
	call EncScript_Shaymin_ApplyGarden
	return

// 20% - SEED FLARE. Shaymin's signature, arriving exactly where the spec has it gather the energy of
// the battlefield. It overwrites Play Rough.
EncScript_Shaymin_SeedFlarePhase::
	encsetvar 0, 2      @ Phase
	encsetvar 11, 4     @ FlareClock: one Flare every third turn
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SEED_FLARE
	printstring STRINGID_ENCSHAYMINFLAREARRIVES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	call EncScript_Shaymin_ApplyGarden
	return

// What the Flare IS gets read live, at the moment it fires, not committed at 20% - decided once, the
// mechanic would be dead for the last third of the fight. Read this way the player can still turn a
// Blight Flare into a Bloom Flare with three good turns.
EncScript_Shaymin_SeedFlare::
	encsetvar 11, 4     @ FlareClock: re-armed
	printstring STRINGID_ENCSHAYMINFLARE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SEED_FLARE
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Shaymin_SeedFlare_Bloom
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Shaymin_SeedFlare_Blight
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return
EncScript_Shaymin_SeedFlare_Bloom:
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	encsetterrain ENC_TERRAIN_GRASSY
	printstring STRINGID_ENCSHAYMINFLAREBLOOM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	return
// 28% every third turn, stacked on a Sky Forme that already moves first. That is the fight the player
// chose, and it is the real pressure of the Blight endgame.
EncScript_Shaymin_SeedFlare_Blight:
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Shaymin_SeedFlare_Blight_Body
	printstring STRINGID_ENCSHAYMINFLAREBLIGHTSTRIP
	waitmessage B_WAIT_TIME_SHORT
EncScript_Shaymin_SeedFlare_Blight_Body:
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPDEF, -1
	printstring STRINGID_ENCSHAYMINFLAREBLIGHT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	return

// 10% - the catch window, the mirror image of the Properties: block plus everything the fight built.
// Phase 3 retires every other trigger, so the ledger, the flare and the bands all stop here.
// The window itself is identical on every path - guideline 10 is not negotiable. What the Garden buys
// is how many balls it takes, which is the right size of reward for a mechanic nothing ever names.
// From here the engine's automatic catch-window damage guard keeps the catch target alive;
// encsetdamagereduction sets the value that guard will restore, not the live one.
EncScript_Shaymin_Weakened::
	encsetvar 0, 3      @ Phase
	encsetvar 11, 0     @ FlareClock
	encsetvar 12, 0     @ Verdict
	call EncScript_Shaymin_ClearGarden
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	encsetvar 2, 255    @ LastBand: nothing of the garden is standing any more
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Shaymin_Weakened_Body
	encsetvar 4, 0      @ Forme
	encformchange ENC_TARGET_BOSS, SPECIES_SHAYMIN, EncScript_Shaymin_Weakened_Land, B_ANIM_FORM_CHANGE
EncScript_Shaymin_Weakened_Land:
	encsetmove ENC_TARGET_BOSS, 1, MOVE_EARTH_POWER
EncScript_Shaymin_Weakened_Body:
	call EncScript_Shaymin_NeutralStance
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Shaymin_Weakened_Bloom
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Shaymin_Weakened_Blight
	encsetcatchrate 30
	printstring STRINGID_ENCSHAYMINWEAKENEDNEUTRAL
	goto EncScript_Shaymin_Weakened_Open
EncScript_Shaymin_Weakened_Bloom:
	encsetcatchrate 30
	printstring STRINGID_ENCSHAYMINWEAKENEDBLOOM
	goto EncScript_Shaymin_Weakened_Open
EncScript_Shaymin_Weakened_Blight:
	encsetcatchrate 30
	printstring STRINGID_ENCSHAYMINWEAKENEDBLIGHT
EncScript_Shaymin_Weakened_Open:
	encsetballs ENC_BALLS_ALLOWED
	waitmessage B_WAIT_TIME_LONG
	return
