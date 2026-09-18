// Terrakion, "The Unstoppable Force". Var indices, pinned by the always-true Conditions on
// EncScript_Terrakion_Intro:
// 0 Phase (0 The Unstoppable Force / 1 Rampage / 2 Unstoppable / 3 Spent), 1 Impact (0-5, or 0-7
// from Phase 1), 2 LastImpact (the last ANNOUNCED level), 3 TurnGuard, 4 Rose (Impact rose this
// turn), 5 Landed (Terrakion's side took HP off the player this turn), 6 Hit (_StruckHard's
// re-entry guard), 7 EventGuard (shared by _TheyEntered and _TheyFell), 8 PlayerMark, 9 BossMark,
// 10 Scratch, 11 Wave (Shockwave wheel cursor 1-3), 12 BreakCd, 13 Stagger, 14 Seen, 15 Punched.
//
// IMPACT is the fight. It rises almost on its own - Terrakion attacks every turn and its whole
// moveset is Rock and Fighting - so the player's job is to hold it DOWN by denying Terrakion a
// landed hit. IMPACT never touches the damage reduction: EncScript_Terrakion_ApplyGuard is the
// guard's only writer and the phase is its only input, so there is exactly one dial on the number
// the player feels. Then at 50% the fight inverts - the ceiling rises to 7 and reaching 7 is
// COLLAPSE, the biggest damage window in the fight, opened by driving IMPACT up on purpose.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction, written absolutely from Phase and Stagger. A stagger
// expiring restores the guard with nothing having to remember it was dropped, and a phase
// transition during a stagger cannot lose it. Phase 3 does not call it - once balls are allowed the
// engine's own catch guard owns the number.
EncScript_Terrakion_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Terrakion_ApplyGuard_Done
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Terrakion_ApplyGuard_Ph1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Terrakion_ApplyGuard_Ph2
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Terrakion_ApplyGuard_Ph1:
	encjumpifvar CMP_GREATER_THAN, 13, 0, EncScript_Terrakion_ApplyGuard_Ph1Stag
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Terrakion_ApplyGuard_Ph1Stag:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Terrakion_ApplyGuard_Ph2:
	encjumpifvar CMP_GREATER_THAN, 13, 0, EncScript_Terrakion_ApplyGuard_Ph2Stag
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Terrakion_ApplyGuard_Ph2Stag:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Terrakion_ApplyGuard_Done:
	return

// THE IMPACT CALLOUT (guideline 14). LastImpact holds the last ANNOUNCED level; each arm compares
// it against its own literal and stays quiet if that is what was last said, so the line prints only
// on a difference - in BOTH directions. Level 0's line is as load-bearing as level 7's: it is the
// confirmation that the turn spent not trading did something.
EncScript_Terrakion_AnnounceImpact:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Terrakion_AnnounceImpact_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Terrakion_AnnounceImpact_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Terrakion_AnnounceImpact_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Terrakion_AnnounceImpact_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Terrakion_AnnounceImpact_4
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Terrakion_AnnounceImpact_5
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Terrakion_AnnounceImpact_6
	encjumpifvar CMP_EQUAL, 2, 7, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT7
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Terrakion_AnnounceImpact_Latch
EncScript_Terrakion_AnnounceImpact_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT0
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Terrakion_AnnounceImpact_Latch
EncScript_Terrakion_AnnounceImpact_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Terrakion_AnnounceImpact_Latch
EncScript_Terrakion_AnnounceImpact_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Terrakion_AnnounceImpact_Latch
EncScript_Terrakion_AnnounceImpact_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Terrakion_AnnounceImpact_Latch
EncScript_Terrakion_AnnounceImpact_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT4
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Terrakion_AnnounceImpact_Latch
EncScript_Terrakion_AnnounceImpact_5:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT5
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Terrakion_AnnounceImpact_Latch
EncScript_Terrakion_AnnounceImpact_6:
	encjumpifvar CMP_EQUAL, 2, 6, EncScript_Terrakion_AnnounceImpact_Done
	printstring STRINGID_ENCTERRAKIONIMPACT6
	waitmessage B_WAIT_TIME_LONG
EncScript_Terrakion_AnnounceImpact_Latch:
	enccopyvar 2, 1     @ LastImpact = Impact
EncScript_Terrakion_AnnounceImpact_Done:
	return

// Impact +1, and +2 again in UNSTOPPABLE. Rose is set FIRST (before any ceiling return), so however
// many things feed the meter in one turn the rise is capped at one step and any event-reacting
// trigger gated on Rose self-disables after the first call. Phase 0's ceiling is 5; Phase 1+ is 7.
EncScript_Terrakion_Gain:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Terrakion_Gain_Done      @ Rose already spent this turn
	encsetvar 4, 1                                                  @ Rose = 1
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Terrakion_Gain_Ceil7
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Terrakion_Gain_Done   @ Phase 0 ceiling
	goto EncScript_Terrakion_Gain_Rise
EncScript_Terrakion_Gain_Ceil7:
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Terrakion_Gain_Done   @ Rampage ceiling
EncScript_Terrakion_Gain_Rise:
	encaddvar 1, 1
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Terrakion_Gain_Announce  @ +2 only in UNSTOPPABLE
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Terrakion_Gain_Announce
	encaddvar 1, 1
EncScript_Terrakion_Gain_Announce:
	call EncScript_Terrakion_AnnounceImpact
EncScript_Terrakion_Gain_Done:
	return

// A knockout is the spec's biggest single source: +2, ignoring the +1-per-turn cap but still
// bounded by the phase ceiling. A KO is also, by definition, Terrakion's side taking HP off the
// player, so it sets Landed - without which the replacement switching in at full HP would read as a
// turn Terrakion connected on nothing and hand the player a free decay.
EncScript_Terrakion_GainKO:
	encsetvar 4, 1     @ Rose - a KO spends the turn's rise
	encsetvar 5, 1     @ Landed
	encaddvar 1, 2
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Terrakion_GainKO_Hard
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Terrakion_GainKO_Announce
	encsetvar 1, 5
	goto EncScript_Terrakion_GainKO_Announce
EncScript_Terrakion_GainKO_Hard:
	encjumpifvar CMP_LESS_THAN, 1, 8, EncScript_Terrakion_GainKO_Announce
	encsetvar 1, 7
EncScript_Terrakion_GainKO_Announce:
	call EncScript_Terrakion_AnnounceImpact
	return

// BREAKER's strip half, on a 3-turn cooldown so it punishes setup without erasing every turn of it.
// Both encresetstats and encclearscreens carry fail labels, so a player with nothing set up sees no
// line rather than a message about something that did not happen.
EncScript_Terrakion_BreakerStrip:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Terrakion_BreakerStrip_Screens
	printstring STRINGID_ENCTERRAKIONSTRIPS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
EncScript_Terrakion_BreakerStrip_Screens:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Terrakion_BreakerStrip_Cd
	printstring STRINGID_ENCTERRAKIONSHATTERS
	waitmessage B_WAIT_TIME_LONG
EncScript_Terrakion_BreakerStrip_Cd:
	encsetvar 12, 3     @ BreakCd
	return

// SHOCKWAVES. The spec's key distinction: Terrakion is not using another move - the force of the
// attack it already made is rolling across the board. So the shockwave is gated on Terrakion having
// actually connected (Landed), never on its own clock. Phase 0-1 also require the blow to have
// raised Impact (Rose); Phase 2 fires on every landed hit. The 4% chip is felt every time, even
// against a bare board - it is what replaces the spec's cut "damage benched Pokemon slightly". Then
// one effect off the wheel, cursor 1 -> 2 -> 3 -> 1, each arm falling through to the next on its
// fail label so a shockwave never announces clearing something that was not there.
EncScript_Terrakion_Shockwave:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Terrakion_Shockwave_Done   @ Impact < 2
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Terrakion_Shockwave_Done       @ it did not connect
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Terrakion_Shockwave_Fire       @ Phase 2: any landed hit
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Terrakion_Shockwave_Done       @ Phase 0-1: only if Impact rose
EncScript_Terrakion_Shockwave_Fire:
	printstring STRINGID_ENCTERRAKIONSHOCKWAVE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_BULLDOZE
	encchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Terrakion_Shockwave_Screens
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Terrakion_Shockwave_Field
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Terrakion_Shockwave_Screens
	printstring STRINGID_ENCTERRAKIONGROUNDSPLITS
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Terrakion_Shockwave_Advance
EncScript_Terrakion_Shockwave_Screens:
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Terrakion_Shockwave_Field
	printstring STRINGID_ENCTERRAKIONAIRCRACKS
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Terrakion_Shockwave_Advance
EncScript_Terrakion_Shockwave_Field:
	printstring STRINGID_ENCTERRAKIONFIELDTEARS
	waitmessage B_WAIT_TIME_LONG
	removeterrain
	encclearfieldstatus ENC_FIELD_TRICK_ROOM
	encclearfieldstatus ENC_FIELD_GRAVITY
EncScript_Terrakion_Shockwave_Advance:
	encaddvar 11, 1
	encjumpifvar CMP_LESS_THAN, 11, 4, EncScript_Terrakion_Shockwave_Done
	encsetvar 11, 1
EncScript_Terrakion_Shockwave_Done:
	return

// --- Trigger scripts ---

EncScript_Terrakion_Intro::
	printstring STRINGID_ENCTERRAKIONINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTERRAKIONINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 11, 1     @ Wave: the wheel starts on THE GROUND SPLITS
	call EncScript_Terrakion_ApplyGuard
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that prints before any
// animation, per the BG0 gotcha. TurnGuard is set immediately so no branch out of this script can
// leave it re-armed. Ticks Stagger (restoring the guard when it expires) and BreakCd, takes the two
// HP marks the turn is measured against, runs the recurring Breaker line, and clears the per-turn
// state.
EncScript_Terrakion_TurnOpen::
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Terrakion_TurnOpen_Clear   @ Phase 3: quiet
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Terrakion_TurnOpen_BreakCd
	encsubvar 13, 1    @ Stagger
	call EncScript_Terrakion_ApplyGuard
EncScript_Terrakion_TurnOpen_BreakCd:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Terrakion_TurnOpen_Mark
	encsubvar 12, 1    @ BreakCd
EncScript_Terrakion_TurnOpen_Mark:
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 8, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_SET
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Terrakion_TurnOpen_Clear
	printstring STRINGID_ENCTERRAKIONIMPACTREMIND
	waitmessage B_WAIT_TIME_SHORT
EncScript_Terrakion_TurnOpen_Clear:
	encsetvar 6, 0     @ Hit
	encsetvar 7, 0     @ EventGuard
	encsetvar 14, 0    @ Seen
	encsetvar 15, 0    @ Punched
	encsetvar 4, 0     @ Rose
	encsetvar 5, 0     @ Landed
	return

// A move landed on Terrakion. Hit is set first, so this can only speak once per turn however many
// moves resolve. Copies BossMark into the scratch so ENC_SNAP_DAMAGE can overwrite it with the
// damage since the mark, then gains an Impact level if that is >= 12% of Terrakion's max HP.
EncScript_Terrakion_StruckHard::
	encsetvar 6, 1     @ Hit
	enccopyvar 10, 9   @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Terrakion_StruckHard_Done
	encjumpifvar CMP_LESS_THAN, 10, 12, EncScript_Terrakion_StruckHard_Done
	call EncScript_Terrakion_Gain
EncScript_Terrakion_StruckHard_Done:
	return

// Terrakion's Rock or Fighting move resolved against the player. Both triggers point here - the
// only difference is the Event.MoveType literal in their conditions. Gains an Impact level; Gain's
// Rose cap collapses a Rock hit and a heavy hit in the same turn to one level. Deliberately does
// NOT set Landed - the decay in _TurnClose is HP-measured, so a missed attack still lets the meter
// fall even though this fired.
EncScript_Terrakion_Force::
	call EncScript_Terrakion_Gain
	return

// A player Pokemon went down. EventGuard stops the replacement switch-in being processed a second
// time; the +2 is applied by _GainKO, ignoring the per-turn cap.
EncScript_Terrakion_TheyFell::
	encsetvar 7, 1     @ EventGuard
	call EncScript_Terrakion_GainKO
	return

// A mid-turn switch (U-turn, Eject Button). Sets EventGuard and nothing else - Terrakion has no
// switch penalty, and this only exists so a faint later in the same turn is not double-counted.
EncScript_Terrakion_TheyEntered::
	encsetvar 7, 1     @ EventGuard
	return

// BREAKER's punch-through - UNCOOLED. The player protected at Impact 4+, and the force goes through
// it anyway. The 12% chip is scripted damage to the player, so _TurnClose's decay measurement (at
// priority 90, after this) sees it and refuses the decay - Protect stops being a free out.
EncScript_Terrakion_BreakerThrough::
	encsetvar 15, 1    @ Punched
	printstring STRINGID_ENCTERRAKIONTHROUGH
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ROCK_SLIDE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	call EncScript_Terrakion_Gain      @ "the force went through a shield"
	return

// Runs last, so it sees the finished turn. Measures the damage Terrakion's side took off the
// player this turn (real hits plus the punch-through chip), sets Landed and the heavy-damage bonus
// from it, runs Breaker's strip half and the Shockwave, then THE DECAY: a turn where Terrakion
// connected on nothing costs it an Impact level. The guard is restored absolutely, so an expired
// stagger comes back with nothing having to track it.
EncScript_Terrakion_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Terrakion_TurnClose_Clear   @ Phase 3: quiet
	enccopyvar 10, 8   @ Scratch = PlayerMark
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 10, ENC_SNAP_DAMAGE, EncScript_Terrakion_TurnClose_Breaker
	encsetvar 5, 1     @ Landed
	encjumpifvar CMP_LESS_THAN, 10, 20, EncScript_Terrakion_TurnClose_Breaker
	call EncScript_Terrakion_Gain      @ "deals heavy damage"
EncScript_Terrakion_TurnClose_Breaker:
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Terrakion_TurnClose_Shockwave
	encjumpifvar CMP_NOT_EQUAL, 12, 0, EncScript_Terrakion_TurnClose_Shockwave
	call EncScript_Terrakion_BreakerStrip
EncScript_Terrakion_TurnClose_Shockwave:
	call EncScript_Terrakion_Shockwave
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Terrakion_TurnClose_Guard   @ it connected: no decay
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Terrakion_TurnClose_Guard   @ nothing to decay
	encsubvar 1, 1
	call EncScript_Terrakion_AnnounceImpact
EncScript_Terrakion_TurnClose_Guard:
	call EncScript_Terrakion_ApplyGuard
EncScript_Terrakion_TurnClose_Clear:
	encsetvar 3, 0     @ TurnGuard
	return

// COLLAPSE. Impact hit 7 in Rampage or Unstoppable: Terrakion overreaches, fires the charge, then
// staggers. The largest damage window in the fight - two turns at a dropped guard against a
// Terrakion at -2/-2 that cannot act on the first of them. Self-disables by resetting Impact to 0.
EncScript_Terrakion_Collapse::
	printstring STRINGID_ENCTERRAKIONOVERWHELMS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCTERRAKIONCHARGE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_STONE_EDGE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCTERRAKIONSTAGGERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encsetvar 1, 0     @ Impact = 0
	call EncScript_Terrakion_AnnounceImpact
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -2
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -2
	encsetrecharge ENC_TARGET_BOSS, 2   @ OnTurnEnd: it loses next turn's action
	encsetvar 13, 2    @ Stagger
	call EncScript_Terrakion_ApplyGuard
	printstring STRINGID_ENCTERRAKIONEXPOSED
	waitmessage B_WAIT_TIME_LONG
	return

// The evasion answer. Two stages a turn, gated on Seen so it does not re-select inside one
// checkpoint's re-evaluation loop. Terrakion's whole moveset is physical and none of it never-
// misses, so without this an evasion stack walls the fight outright.
EncScript_Terrakion_SeeThrough::
	encsetvar 14, 1    @ Seen
	printstring STRINGID_ENCTERRAKIONSEESTHROUGH
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_ALL_FOES, STAT_EVASION, -2
	return

// --- Phases ---

// 50% - RAMPAGE. Raw Attack +20% and Speed +10% written into the stats (they survive a Haze), the
// moveset hardens, and the guard DROPS 90 -> 86 - Terrakion has stopped defending itself, and the
// dialogue says so. COLLAPSE goes live because its trigger condition is Phase >= 1.
EncScript_Terrakion_Rampage::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCTERRAKIONENRAGED1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 20, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 10, ENC_AMOUNT_PERCENT
	encsetmove ENC_TARGET_BOSS, 1, MOVE_STONE_EDGE
	encsetmove ENC_TARGET_BOSS, 2, MOVE_CLOSE_COMBAT
	encsetmove ENC_TARGET_BOSS, 3, MOVE_QUICK_ATTACK
	restoremovepp BS_OPPONENT1
	call EncScript_Terrakion_ApplyGuard     @ 90 -> 86
	printstring STRINGID_ENCTERRAKIONABANDONS
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTERRAKIONUNSTABLE
	waitmessage B_WAIT_TIME_LONG
	return

// 20% - UNSTOPPABLE. The guard climbs to 93, Mist blocks Attack drops (Defog and Infiltrator still
// beat it), Impact rises +2 a turn, Shockwaves fire on every landed hit, and Reversal - a 150-200
// BP move at this HP - fills slot 3. Survive: True carries the fight from here.
EncScript_Terrakion_Unstoppable::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCTERRAKIONREFUSES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HANGED_ON
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST, 0
	encsetmove ENC_TARGET_BOSS, 3, MOVE_REVERSAL
	restoremovepp BS_OPPONENT1
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Terrakion_Unstoppable_Guard
	encsetvar 1, 3     @ Impact floored at 3 from here
EncScript_Terrakion_Unstoppable_Guard:
	call EncScript_Terrakion_ApplyGuard     @ 86 -> 93
	call EncScript_Terrakion_AnnounceImpact
	printstring STRINGID_ENCTERRAKIONUNSTOPPABLE
	waitmessage B_WAIT_TIME_LONG
	return

// 1 HP - SPENT, and the catch window (guideline 10). One last charge, then the guard, the Mist, the
// immunities and Survive all come off and the ball is allowed. Every trigger that could still fire
// is gated Phase <= 2, so from here the fight is quiet. encsetdamagereduction 0 writes the value
// the automatic catch guard will RESTORE, not the live one.
EncScript_Terrakion_Spent::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCTERRAKIONGIVESOUT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_CLOSE_COMBAT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -25, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCTERRAKIONSPENT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -2
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -2
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCTERRAKIONWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
