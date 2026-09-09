// Virizion, "The Graceful Blade". Var indices, pinned by the always-true Conditions on
// EncScript_Virizion_Intro:
// 0 Phase (0 The Graceful Blade / 1 Untouchable / 2 Sacred Blade / 3 Spent),
// 1 Momentum (0-5, or 0-7 from Phase 1), 2 LastMom (the last ANNOUNCED level), 3 Tier (perks
// applied: 0 none / 1 the 3+ set / 2 the 5+ set), 4 TurnGuard, 5 Landed (Virizion's side took HP
// off the player this turn), 6 Broke (a break already fired this turn), 7 Rose (the meter rose this
// turn), 8 Danced (Blade Dance already fired this turn), 9 PlayerMark, 10 BossMark, 11 Scratch,
// 12 DodgeCd, 13 EventGuard, 14 Ultimate.
//
// MOMENTUM is Virizion's armour: EncScript_Virizion_ApplyFlow is the damage reduction's only writer
// and Momentum its only input, written absolutely (78 at 0, 97 at 7). The whole fight is the turns
// after a break drops the meter to 0 - which also strips evasion, crit and Speed in one beat.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction, written absolutely from Momentum. A break undoes a
// climb with no separate teardown path. Phase 3 (Spent) never calls it - once balls are allowed the
// engine's own catch guard owns the number.
EncScript_Virizion_ApplyFlow:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Virizion_ApplyFlow_Done
	encjumpifvar CMP_EQUAL, 1, 7, EncScript_Virizion_ApplyFlow_7
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Virizion_ApplyFlow_6
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Virizion_ApplyFlow_5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Virizion_ApplyFlow_4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Virizion_ApplyFlow_3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Virizion_ApplyFlow_2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Virizion_ApplyFlow_1
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Virizion_ApplyFlow_1:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Virizion_ApplyFlow_2:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Virizion_ApplyFlow_3:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Virizion_ApplyFlow_4:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Virizion_ApplyFlow_5:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Virizion_ApplyFlow_6:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Virizion_ApplyFlow_7:
	encsetdamagereduction ENC_TARGET_BOSS, 97
	return
EncScript_Virizion_ApplyFlow_Done:
	return

// THE MOMENTUM CALLOUT (guideline 14). LastMom holds the last ANNOUNCED level; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a difference - in BOTH directions. Level 0's line is as load-bearing as level 7's: the fall is
// the player's own work and the fight has no other way to confirm it.
EncScript_Virizion_AnnounceMomentum:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Virizion_AnnounceMomentum_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Virizion_AnnounceMomentum_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Virizion_AnnounceMomentum_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Virizion_AnnounceMomentum_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Virizion_AnnounceMomentum_4
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Virizion_AnnounceMomentum_5
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Virizion_AnnounceMomentum_6
	encjumpifvar CMP_EQUAL, 2, 7, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM7
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Virizion_AnnounceMomentum_Latch
EncScript_Virizion_AnnounceMomentum_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM0
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Virizion_AnnounceMomentum_Latch
EncScript_Virizion_AnnounceMomentum_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Virizion_AnnounceMomentum_Latch
EncScript_Virizion_AnnounceMomentum_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Virizion_AnnounceMomentum_Latch
EncScript_Virizion_AnnounceMomentum_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Virizion_AnnounceMomentum_Latch
EncScript_Virizion_AnnounceMomentum_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM4
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Virizion_AnnounceMomentum_Latch
EncScript_Virizion_AnnounceMomentum_5:
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM5
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Virizion_AnnounceMomentum_Latch
EncScript_Virizion_AnnounceMomentum_6:
	encjumpifvar CMP_EQUAL, 2, 6, EncScript_Virizion_AnnounceMomentum_Done
	printstring STRINGID_ENCVIRIZIONMOMENTUM6
	waitmessage B_WAIT_TIME_LONG
EncScript_Virizion_AnnounceMomentum_Latch:
	enccopyvar 2, 1     @ LastMom = Momentum
EncScript_Virizion_AnnounceMomentum_Done:
	return

// Grants whichever perk tier the current Momentum has earned but Tier has not yet applied. Called
// after any Momentum rise. PerkThree sets Tier 1; the second clause then sees Tier == 1 and, at
// Momentum 5+, adds PerkFive on top.
EncScript_Virizion_ApplyTier:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Virizion_ApplyTier_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 0, EncScript_Virizion_ApplyTier_Five
	call EncScript_Virizion_PerkThree
EncScript_Virizion_ApplyTier_Five:
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Virizion_ApplyTier_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 1, EncScript_Virizion_ApplyTier_Done
	call EncScript_Virizion_PerkFive
EncScript_Virizion_ApplyTier_Done:
	return

EncScript_Virizion_PerkThree:
	encsetvar 3, 1     @ Tier
	printstring STRINGID_ENCVIRIZIONBLADEDANCE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encsetcrit ENC_TARGET_BOSS, TRUE
	return

EncScript_Virizion_PerkFive:
	encsetvar 3, 2     @ Tier
	printstring STRINGID_ENCVIRIZIONUNTOUCHABLE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	return

// Momentum +1. Phase 0's ceiling is 5; Phase 1+ is 7. The announcement is NOT made here - the
// turn's scripts call _AnnounceMomentum once at the end, so however many times the meter moves the
// player reads one line about it.
EncScript_Virizion_Gain:
	encjumpifvar CMP_GREATER_THAN, 1, 6, EncScript_Virizion_Gain_Done       @ Untouchable ceiling
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Virizion_Gain_Rise          @ Phase 1+: ceiling is 7
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Virizion_Gain_Done       @ Phase 0 ceiling is 5
EncScript_Virizion_Gain_Rise:
	encaddvar 1, 1
	encsetvar 7, 1     @ Rose
	call EncScript_Virizion_ApplyTier
EncScript_Virizion_Gain_Done:
	return

// THE ONE THING THE PLAYER IS PLAYING FOR. Called from _BreakType, _Slowed, _Statused and
// _TurnClose's master rule. A break drops the meter to 0 and nothing more - that alone is 15-19
// guard points and evasion, crit and Speed stripped in one beat.
EncScript_Virizion_Break:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Virizion_Break_Done            @ one break per turn
	encsetvar 6, 1     @ Broke
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Virizion_Break_Done           @ nothing to break
	encsetvar 1, 0     @ Momentum
	printstring STRINGID_ENCVIRIZIONBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Virizion_ClearPerks
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Virizion_Break_Exhaust        @ SACRED BLADE: also EXHAUSTED
	goto EncScript_Virizion_Break_Flow
EncScript_Virizion_Break_Exhaust:
	call EncScript_Virizion_Exhausted
EncScript_Virizion_Break_Flow:
	call EncScript_Virizion_ApplyFlow
	call EncScript_Virizion_AnnounceMomentum
EncScript_Virizion_Break_Done:
	return

// Takes the tier's stat stages and the crit boost back off. Arithmetic, not a snapshot, kept small
// against the +-6 clamp; the stage clamp bounds the worst-case drift.
EncScript_Virizion_ClearPerks:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Virizion_ClearPerks_Done
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -1
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Virizion_ClearPerks_Fin
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -1
EncScript_Virizion_ClearPerks_Fin:
	encsetcrit ENC_TARGET_BOSS, FALSE
	encsetvar 3, 0     @ Tier
EncScript_Virizion_ClearPerks_Done:
	return

// The spec's phase-3 payoff, kept as a SACRED BLADE exclusive: breaking the charge leaves Virizion
// EXHAUSTED. encsetrecharge 2 at OnTurnEnd costs NEXT turn's action.
EncScript_Virizion_Exhausted:
	printstring STRINGID_ENCVIRIZIONEXHAUSTED
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	encsetrecharge ENC_TARGET_BOSS, 2
	return

// --- Trigger scripts ---

EncScript_Virizion_Intro::
	printstring STRINGID_ENCVIRIZIONINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCVIRIZIONINTRO2
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Virizion_ApplyFlow
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that prints before any
// animation, per the BG0 gotcha. TurnGuard is set immediately so no branch can leave it re-armed.
// Ticks the dodge cooldown, takes the two HP marks the turn is measured against as percentages (the
// only portable choice under Level: LevelCap and New Game Plus offsets), runs the recurring flow
// line, and clears the per-turn state.
EncScript_Virizion_TurnOpen::
	flushtextbox
	encsetvar 4, 1     @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Virizion_TurnOpen_Clear   @ Phase 3: quiet
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Virizion_TurnOpen_Mark
	encsubvar 12, 1    @ DodgeCd
EncScript_Virizion_TurnOpen_Mark:
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 9, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_SET
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Virizion_TurnOpen_Clear
	printstring STRINGID_ENCVIRIZIONMOMENTUMREMIND
	waitmessage B_WAIT_TIME_SHORT
EncScript_Virizion_TurnOpen_Clear:
	encsetvar 5, 0     @ Landed
	encsetvar 6, 0     @ Broke
	encsetvar 7, 0     @ Rose
	encsetvar 8, 0     @ Danced
	encsetvar 13, 0    @ EventGuard
	encsetvar 15, 0    @ StepGuard
	return

// Phase 1+ anti-abuse: a status is cleared at the top of every turn, so status keeps working but
// costs a move every turn instead of once.
EncScript_Virizion_Shed::
	printstring STRINGID_ENCVIRIZIONSHEDS
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// Phase 1+ anti-abuse: a lowered Speed stage climbs back one stage a turn - one step at a time, so
// a player who invested two turns in Icy Wind still gets two turns of value out of it.
EncScript_Virizion_Footing::
	encsetvar 15, 1    @ StepGuard - one stage per turn, not once per re-evaluation pass
	printstring STRINGID_ENCVIRIZIONFOOTING
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	return

// The telegraphed dodge (Phase 1+, Momentum 5+). Announced before the player's move resolves, so
// the turn can be spent on setup, a heal or a switch instead. encsetprotect at OnTurnStart covers
// the whole turn and expires on its own; DodgeCd is the 2-turn cooldown.
EncScript_Virizion_Dodge::
	printstring STRINGID_ENCVIRIZIONBLUR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_PROTECTED_ITSELF
	encsetprotect ENC_TARGET_BOSS
	encsetvar 12, 2    @ DodgeCd
	return

// A Flying / Fire / Ice / Poison / Psychic / Fairy move landed on Virizion. Prints its own cue so
// the player can tell which thing they did worked, then breaks. Broke gate is in the trigger's
// Conditions.
EncScript_Virizion_BreakType::
	printstring STRINGID_ENCVIRIZIONBREAKTYPE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Virizion_Break
	return

// A player Pokemon went down: +1, and it counts as Virizion's side taking HP off the player so the
// replacement at full HP does not read as a flawless-pass-denied turn. EventGuard stops the
// replacement switch-in being processed a second time.
EncScript_Virizion_TheyFell::
	encsetvar 13, 1    @ EventGuard
	encsetvar 5, 1     @ Landed
	call EncScript_Virizion_Gain
	return

// A mid-turn switch (U-turn, Eject Button). Sets EventGuard and nothing else - Virizion has no
// switch penalty; a switch is a turn the player's health still drops to Quick Attack.
EncScript_Virizion_TheyEntered::
	encsetvar 13, 1    @ EventGuard
	return

// THE LEDGER. Runs at OnTurnEnd priority 80, so it sees a finished turn. THE MASTER RULE: a turn
// Virizion's side took no health off the player breaks the meter. Otherwise +1 for keeping its
// rhythm, and +1 more for a flawless pass (nothing touched Virizion). In SACRED BLADE the meter
// climbs +1 unconditionally instead. Then the guard restored absolutely and the callout.
EncScript_Virizion_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Virizion_TurnClose_Quiet   @ Phase 3: quiet
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Virizion_TurnClose_Charge
	enccopyvar 11, 9   @ Scratch = PlayerMark
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 11, ENC_SNAP_DAMAGE, EncScript_Virizion_TurnClose_NoHit
	encsetvar 5, 1     @ Landed
	call EncScript_Virizion_Gain
	enccopyvar 11, 10  @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_DAMAGE, EncScript_Virizion_TurnClose_Flawless
	goto EncScript_Virizion_TurnClose_Close
EncScript_Virizion_TurnClose_Flawless:
	call EncScript_Virizion_Gain      @ +1 more: nothing touched Virizion
	goto EncScript_Virizion_TurnClose_Close
EncScript_Virizion_TurnClose_NoHit:
	call EncScript_Virizion_Break      @ THE MASTER RULE
	goto EncScript_Virizion_TurnClose_Close
EncScript_Virizion_TurnClose_Charge:
	call EncScript_Virizion_Gain      @ SACRED BLADE: +1 every turn regardless
EncScript_Virizion_TurnClose_Close:
	call EncScript_Virizion_ApplyFlow
	call EncScript_Virizion_AnnounceMomentum
EncScript_Virizion_TurnClose_Quiet:
	encsetvar 4, 0     @ TurnGuard
	return

// Runs AFTER _TurnClose (80): the meter ticks up, then a Speed drop takes it straight back to 0 in
// the same checkpoint. The trigger's Broke == 0 condition is what stops it re-selecting - _Break
// sets Broke, so the cue and the break resolve exactly once.
EncScript_Virizion_Slowed::
	printstring STRINGID_ENCVIRIZIONBREAKSLOWED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Virizion_Break
	return

EncScript_Virizion_Statused::
	printstring STRINGID_ENCVIRIZIONBREAKSTATUSED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Virizion_Break
	return

// BLADE DANCE's second slash (Momentum 3+, on a turn the meter rose). Virizion is NOT taking
// another turn - the blow it already landed carries into a second cut. Runs after the ledger, so
// its damage never feeds the ledger's own measurement.
EncScript_Virizion_Dance::
	encsetvar 8, 1     @ Danced
	printstring STRINGID_ENCVIRIZIONDANCE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_LEAF_BLADE
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Virizion_Dance_Ph1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Virizion_Dance_Ph2
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	return
EncScript_Virizion_Dance_Ph1:
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Virizion_Dance_Ph2:
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return

// SACRED SWORD. Momentum hit 7 in SACRED BLADE: the charge fires for 32% of the player's side, then
// the meter falls back to 3 to charge again. Tier is left where it is - the perks were earned at
// 3+ and Momentum is still 3. Selected on the re-evaluation pass after _TurnClose (rule 3).
EncScript_Virizion_SacredSword::
	encaddvar 14, 1    @ Ultimate
	printstring STRINGID_ENCVIRIZIONSACREDBLADE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SACRED_SWORD
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	encsetvar 1, 3     @ Momentum -> 3
	call EncScript_Virizion_ApplyFlow
	call EncScript_Virizion_AnnounceMomentum
	encjumpifvar CMP_GREATER_THAN, 14, 1, EncScript_Virizion_SacredSword_Again2
	printstring STRINGID_ENCVIRIZIONAGAIN
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Virizion_SacredSword_Again2:
	printstring STRINGID_ENCVIRIZIONAGAIN2
	waitmessage B_WAIT_TIME_LONG
	return

// --- Phases ---

// 50% - UNTOUCHABLE. Raw Speed +10% written into the stats (survives a Haze), the ceiling rises to
// 7 (its trigger conditions read Phase), the scripted dodges go live, and slot 3 becomes Close
// Combat. The ceiling change is announced because it changes what the player is fighting over.
EncScript_Virizion_Untouchable::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCVIRIZIONFASTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 10, ENC_AMOUNT_PERCENT
	encsetmove ENC_TARGET_BOSS, 3, MOVE_CLOSE_COMBAT
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCVIRIZIONCEILING
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCVIRIZIONSHAKESOFF
	waitmessage B_WAIT_TIME_LONG
	return

// 15% - SACRED BLADE. The meter climbs +1 every turn regardless of whether Virizion landed
// anything, so Protect and denial stop being enough on their own. A break now also leaves it
// EXHAUSTED. Survive: True is load-bearing from here.
EncScript_Virizion_SacredBlade::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCVIRIZIONGATHERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Virizion_SacredBlade_Flow
	encsetvar 1, 3     @ Momentum floored at 3
EncScript_Virizion_SacredBlade_Flow:
	call EncScript_Virizion_ApplyTier
	call EncScript_Virizion_ApplyFlow
	call EncScript_Virizion_AnnounceMomentum
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCVIRIZIONCHARGING
	waitmessage B_WAIT_TIME_LONG
	return

// 1 HP - SPENT, and the catch window (guideline 10). One last strike, then the guard, the
// immunities and Survive all come off and the ball is allowed. Every trigger that could still fire
// is gated Phase <= 2, so from here the fight is quiet. encsetdamagereduction 0 writes the value
// the automatic catch guard will RESTORE, not the live one.
EncScript_Virizion_Spent::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCVIRIZIONFALLS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_CLOSE_COMBAT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCVIRIZIONSTILL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Virizion_ClearPerks
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCVIRIZIONWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
