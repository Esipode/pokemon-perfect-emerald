// Cobalion, "The Iron Will". Var indices, pinned by the always-true Conditions on
// EncScript_Cobalion_Intro:
// 0 Phase (0 The Vanguard / 1 Leader of the Swords / 2 Iron Resolve / 3 Spent),
// 1 Stance (0 GUARD, 1 ASSAULT, 2 COMMAND), 2 LastStance (the last ANNOUNCED stance),
// 3 Discipline (0-5), 4 LastDisc (the last ANNOUNCED level), 5 LastCat (0 nothing, 1 physical,
// 2 special, 3 status, 4 switched), 6 ThisCat, 7 Broke, 8 TurnGuard, 9 Hit, 10 EventGuard,
// 11 Predict, 12 Stall, 13 Cooldown, 14 Seen, 15 Declared.
//
// Cobalion's STANCE answers your last turn; its DISCIPLINE answers your last ten. The stance is
// the only input to the damage reduction and EncScript_Cobalion_ApplyGuard is its only writer, so
// the guard can never drift out of agreement with the line the player was told. Discipline never
// touches the guard at all - it counters, it disables, and it drags the player out of position.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction, written absolutely from Stance and Phase. A stance
// falling back therefore undoes a stance rising with no separate teardown path, and a guard
// dropped by a misplaced read is restored by the next _TurnClose without anything having to
// remember it was dropped. Phase 3 does not call it: once balls are allowed the engine's own
// catch-window guard owns the number.
EncScript_Cobalion_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Cobalion_ApplyGuard_Done
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Cobalion_ApplyGuard_Assault
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Cobalion_ApplyGuard_Command
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_ApplyGuard_Guard1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_ApplyGuard_Guard2
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Cobalion_ApplyGuard_Guard1:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Cobalion_ApplyGuard_Guard2:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Cobalion_ApplyGuard_Assault:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_ApplyGuard_Assault1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_ApplyGuard_Assault2
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Cobalion_ApplyGuard_Assault1:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	return
EncScript_Cobalion_ApplyGuard_Assault2:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Cobalion_ApplyGuard_Command:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_ApplyGuard_Command1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_ApplyGuard_Command2
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Cobalion_ApplyGuard_Command1:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Cobalion_ApplyGuard_Command2:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Cobalion_ApplyGuard_Done:
	return

// The same table minus 18, for the turn Cobalion's declared read lands on the wrong category.
// It holds only until _TurnClose calls _ApplyGuard again, and it is deliberately the largest
// damage window in the fight - the reward for being unpredictable.
EncScript_Cobalion_ApplyGuardDrop:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Cobalion_ApplyGuardDrop_Done
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Cobalion_ApplyGuardDrop_Assault
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Cobalion_ApplyGuardDrop_Command
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_ApplyGuardDrop_Guard1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_ApplyGuardDrop_Guard2
	encsetdamagereduction ENC_TARGET_BOSS, 73
	return
EncScript_Cobalion_ApplyGuardDrop_Guard1:
	encsetdamagereduction ENC_TARGET_BOSS, 75
	return
EncScript_Cobalion_ApplyGuardDrop_Guard2:
	encsetdamagereduction ENC_TARGET_BOSS, 71
	return
EncScript_Cobalion_ApplyGuardDrop_Assault:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_ApplyGuardDrop_Assault1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_ApplyGuardDrop_Assault2
	encsetdamagereduction ENC_TARGET_BOSS, 61
	return
EncScript_Cobalion_ApplyGuardDrop_Assault1:
	encsetdamagereduction ENC_TARGET_BOSS, 63
	return
EncScript_Cobalion_ApplyGuardDrop_Assault2:
	encsetdamagereduction ENC_TARGET_BOSS, 56
	return
EncScript_Cobalion_ApplyGuardDrop_Command:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_ApplyGuardDrop_Command1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_ApplyGuardDrop_Command2
	encsetdamagereduction ENC_TARGET_BOSS, 68
	return
EncScript_Cobalion_ApplyGuardDrop_Command1:
	encsetdamagereduction ENC_TARGET_BOSS, 70
	return
EncScript_Cobalion_ApplyGuardDrop_Command2:
	encsetdamagereduction ENC_TARGET_BOSS, 66
	return
EncScript_Cobalion_ApplyGuardDrop_Done:
	return

// THE DISCIPLINE CALLOUT (guideline 14). LastDisc holds the last ANNOUNCED level; each leaf
// compares it against its own level and stays quiet if that is what was last said, so the line
// prints only on a difference - in BOTH directions. Level 0's line is as load-bearing as level
// 5's: it is the confirmation that varying your play did something.
EncScript_Cobalion_AnnounceDiscipline:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Cobalion_AnnounceDiscipline_0
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Cobalion_AnnounceDiscipline_1
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Cobalion_AnnounceDiscipline_2
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Cobalion_AnnounceDiscipline_3
	encjumpifvar CMP_EQUAL, 3, 4, EncScript_Cobalion_AnnounceDiscipline_4
	encjumpifvar CMP_EQUAL, 4, 5, EncScript_Cobalion_AnnounceDiscipline_Done
	printstring STRINGID_ENCCOBALIONDISC5
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Cobalion_AnnounceDiscipline_Latch
EncScript_Cobalion_AnnounceDiscipline_0:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Cobalion_AnnounceDiscipline_Done
	printstring STRINGID_ENCCOBALIONDISC0
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Cobalion_AnnounceDiscipline_Latch
EncScript_Cobalion_AnnounceDiscipline_1:
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Cobalion_AnnounceDiscipline_Done
	printstring STRINGID_ENCCOBALIONDISC1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Cobalion_AnnounceDiscipline_Latch
EncScript_Cobalion_AnnounceDiscipline_2:
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Cobalion_AnnounceDiscipline_Done
	printstring STRINGID_ENCCOBALIONDISC2
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Cobalion_AnnounceDiscipline_Latch
EncScript_Cobalion_AnnounceDiscipline_3:
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Cobalion_AnnounceDiscipline_Done
	printstring STRINGID_ENCCOBALIONDISC3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Cobalion_AnnounceDiscipline_Latch
EncScript_Cobalion_AnnounceDiscipline_4:
	encjumpifvar CMP_EQUAL, 4, 4, EncScript_Cobalion_AnnounceDiscipline_Done
	printstring STRINGID_ENCCOBALIONDISC4
	waitmessage B_WAIT_TIME_LONG
EncScript_Cobalion_AnnounceDiscipline_Latch:
	enccopyvar 4, 3     @ LastDisc = Discipline
EncScript_Cobalion_AnnounceDiscipline_Done:
	return

// Discipline +1, capped at 5. The announcement is deliberately NOT made here: _TurnClose calls
// _AnnounceDiscipline once at the end of the turn, so however many things moved the meter during
// it the player reads exactly one line about it.
EncScript_Cobalion_RaiseDiscipline:
	encjumpifvar CMP_EQUAL, 3, 5, EncScript_Cobalion_RaiseDiscipline_Done
	encaddvar 3, 1
EncScript_Cobalion_RaiseDiscipline_Done:
	return

// Discipline -1, floored by phase: 0 / 1 / 2. The floor is why the late fight never forgets what
// it learned, and it is checked before the subtraction so the byte can never wrap.
EncScript_Cobalion_LowerDiscipline:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_LowerDiscipline_Floor1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_LowerDiscipline_Floor2
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Cobalion_LowerDiscipline_Done
	encsubvar 3, 1
	return
EncScript_Cobalion_LowerDiscipline_Floor1:
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Cobalion_LowerDiscipline_Done
	encsubvar 3, 1
	return
EncScript_Cobalion_LowerDiscipline_Floor2:
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Cobalion_LowerDiscipline_Done
	encsubvar 3, 1
EncScript_Cobalion_LowerDiscipline_Done:
	return

// Takes the CURRENT stance's stat stages back off before a new one goes on. Arithmetic, not a
// snapshot: the profiles are kept to +-1/+-2 against a +-6 clamp so the worst case a player can
// engineer with sustained stat warfare is about a stage of drift.
EncScript_Cobalion_ExitProfile:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Cobalion_ExitProfile_Assault
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Cobalion_ExitProfile_Done
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	return
EncScript_Cobalion_ExitProfile_Assault:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
EncScript_Cobalion_ExitProfile_Done:
	return

// COMMAND's anti-abuse, and the whole of it: Cobalion has no status immunities, so a status lands
// and works completely until the cooldown lets COMMAND come around and shed it. Displacement, not
// immunity - and Safeguard, GUARD's slot 3, is the dispellable other half.
EncScript_Cobalion_Shed:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Cobalion_Shed_Do
	return
EncScript_Cobalion_Shed_Do:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCCOBALIONSHEDS
	waitmessage B_WAIT_TIME_LONG
	return

// The three stance entries. Each is exit-old, write, enter-new, reset the guard, then announce -
// but only if this is not the stance that was last announced, so a stance held across a phase
// transition does not re-print its banner.
EncScript_Cobalion_ToGuard:
	call EncScript_Cobalion_ExitProfile
	encsetvar 1, 0
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encsetmove ENC_TARGET_BOSS, 2, MOVE_METAL_BURST
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SAFEGUARD
	call EncScript_Cobalion_ApplyGuard
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Cobalion_ToGuard_Done
	printstring STRINGID_ENCCOBALIONGUARD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	encsetvar 2, 0      @ LastStance
EncScript_Cobalion_ToGuard_Done:
	return

EncScript_Cobalion_ToAssault:
	call EncScript_Cobalion_ExitProfile
	encsetvar 1, 1
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encsetmove ENC_TARGET_BOSS, 2, MOVE_CLOSE_COMBAT
	encsetmove ENC_TARGET_BOSS, 3, MOVE_QUICK_ATTACK
	call EncScript_Cobalion_ApplyGuard
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Cobalion_ToAssault_Done
	printstring STRINGID_ENCCOBALIONASSAULT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 2, 1      @ LastStance
EncScript_Cobalion_ToAssault_Done:
	return

// COMMAND carries no stat profile: it is the stance where Cobalion stops attacking and reads. It
// sheds its own status on entry and sets the cooldown that keeps it from chaining.
EncScript_Cobalion_ToCommand:
	call EncScript_Cobalion_ExitProfile
	encsetvar 1, 2
	encsetmove ENC_TARGET_BOSS, 2, MOVE_TAUNT
	encsetmove ENC_TARGET_BOSS, 3, MOVE_AURA_SPHERE
	call EncScript_Cobalion_ApplyGuard
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Cobalion_ToCommand_Shed
	printstring STRINGID_ENCCOBALIONCOMMAND
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM  @ never TERA_CHARGE bare - see the constant's comment
	encsetvar 2, 2      @ LastStance
EncScript_Cobalion_ToCommand_Shed:
	call EncScript_Cobalion_Shed
	encsetvar 13, 3     @ Cooldown
	return

// COUNTERSTRIKE - Discipline 3+, a repeated damage category, into GUARD. Repetition costs the
// player health rather than doing less damage, which is a thing they can actually see.
EncScript_Cobalion_Counterstrike:
	printstring STRINGID_ENCCOBALIONCOUNTERSTRIKE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_METAL_BURST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Cobalion_Counterstrike_Ph1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_Counterstrike_Ph2
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return
EncScript_Cobalion_Counterstrike_Ph1:
	encchangehp ENC_TARGET_ALL_FOES, -15, ENC_AMOUNT_PERCENT
	return
EncScript_Cobalion_Counterstrike_Ph2:
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
	return

// TACTICAL COMMAND resolved. Cobalion declared a category at the top of the turn and the player
// committed to one; these are the two answers.
EncScript_Cobalion_ReadLands:
	printstring STRINGID_ENCCOBALIONREADLANDS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Cobalion_ReadLands_Ph2
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	goto EncScript_Cobalion_ReadLands_Cost
EncScript_Cobalion_ReadLands_Ph2:
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
EncScript_Cobalion_ReadLands_Cost:
	call EncScript_Cobalion_RaiseDiscipline
	encsetvar 11, 0     @ Predict, spent
	return

EncScript_Cobalion_ReadMisplaced:
	printstring STRINGID_ENCCOBALIONREADMISPLACED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Cobalion_ApplyGuardDrop
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encsetvar 11, 0     @ Predict, spent
	return

// --- Trigger scripts ---

// Turn 0 opens braced. LastStance is already 0, so the GUARD banner is printed here rather than
// through _ToGuard, and the stat profile is applied without an exit pass.
EncScript_Cobalion_Intro::
	printstring STRINGID_ENCCOBALIONINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCCOBALIONINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 0      @ Stance: GUARD
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	call EncScript_Cobalion_ApplyGuard
	printstring STRINGID_ENCCOBALIONGUARD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

// Top of every turn: tick the cooldown, pick the stance the ledger asks for, then clear the
// per-turn state. Leads with flushtextbox - an OnTurnStart script that animates before printing
// anything renders on top of the still-open action menu. TurnGuard is set immediately so no
// branch out of this script can leave it re-armed.
EncScript_Cobalion_TurnOpen::
	flushtextbox
	encsetvar 8, 1      @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Cobalion_TurnOpen_Clear
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Cobalion_TurnOpen_Pick
	encsubvar 13, 1     @ Cooldown
EncScript_Cobalion_TurnOpen_Pick:
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Cobalion_TurnOpen_Ledger
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Cobalion_TurnOpen_WantCommand
	encjumpifvar CMP_GREATER_THAN, 3, 3, EncScript_Cobalion_TurnOpen_WantCommand
// The stance answers WHAT the player did last turn, not whether they varied: an attack braces it,
// anything else gives it room to come forward. LastCat is read as a literal per arm, which is the
// only comparison the condition language has.
EncScript_Cobalion_TurnOpen_Ledger:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Cobalion_TurnOpen_WantGuard
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Cobalion_TurnOpen_WantGuard
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Cobalion_TurnOpen_WantAssault
	encjumpifvar CMP_EQUAL, 5, 4, EncScript_Cobalion_TurnOpen_WantAssault
	goto EncScript_Cobalion_TurnOpen_HoldOrFlip
EncScript_Cobalion_TurnOpen_WantCommand:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Cobalion_TurnOpen_AlreadyCommand
	call EncScript_Cobalion_ToCommand
	goto EncScript_Cobalion_TurnOpen_Clear
// COMMAND is the override, so it is never flipped out of by the Phase 2 tempo rule - shedding the
// status is the whole reason it was picked.
EncScript_Cobalion_TurnOpen_AlreadyCommand:
	call EncScript_Cobalion_Shed
	goto EncScript_Cobalion_TurnOpen_NoChange
EncScript_Cobalion_TurnOpen_WantAssault:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Cobalion_TurnOpen_HoldOrFlip
	call EncScript_Cobalion_ToAssault
	goto EncScript_Cobalion_TurnOpen_Clear
EncScript_Cobalion_TurnOpen_WantGuard:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Cobalion_TurnOpen_HoldOrFlip
	call EncScript_Cobalion_ToGuard
	goto EncScript_Cobalion_TurnOpen_Clear
// IRON RESOLVE's tempo: in Phase 2 a turn never passes without the stance moving, so on any turn
// the ledger would have left it alone the stance flips GUARD <-> ASSAULT anyway and the player can
// never settle into one line of play.
EncScript_Cobalion_TurnOpen_HoldOrFlip:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Cobalion_TurnOpen_NoChange
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Cobalion_TurnOpen_FlipToAssault
	call EncScript_Cobalion_ToGuard
	goto EncScript_Cobalion_TurnOpen_Clear
EncScript_Cobalion_TurnOpen_FlipToAssault:
	call EncScript_Cobalion_ToAssault
	goto EncScript_Cobalion_TurnOpen_Clear
// The recurring Discipline line (guideline 14). It runs only on a turn the stance did NOT change,
// so the fight prints at most one Discipline-flavoured line per turn and never goes quiet while
// the meter is dangerous.
EncScript_Cobalion_TurnOpen_NoChange:
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Cobalion_TurnOpen_Clear
	printstring STRINGID_ENCCOBALIONDISCREMIND
	waitmessage B_WAIT_TIME_SHORT
EncScript_Cobalion_TurnOpen_Clear:
	encsetvar 9, 0      @ Hit
	encsetvar 10, 0     @ EventGuard
	encsetvar 14, 0     @ Seen
	encsetvar 6, 0      @ ThisCat
	encsetvar 7, 0      @ Broke
	encsetvar 11, 0     @ Predict
	encsetvar 15, 0     @ Declared
	return

// TACTICAL COMMAND's declaration. encstoreprediction reads the engine's real AI_FLAG_PREDICT_MOVE
// answer, so outguessing it is a genuine act rather than a scripted coin flip. It can decline to
// answer (0), which is an ordinary quiet turn - Declared, not Predict, is what stops the trigger
// re-selecting. The line says what Cobalion has set itself against, never what that will do.
EncScript_Cobalion_ResolveDeclare::
	encsetvar 15, 1     @ Declared
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 11
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Cobalion_ResolveDeclare_Done
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Cobalion_ResolveDeclare_Phys
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Cobalion_ResolveDeclare_Spec
	printstring STRINGID_ENCCOBALIONDECLARESTATUS
	goto EncScript_Cobalion_ResolveDeclare_Brace
EncScript_Cobalion_ResolveDeclare_Phys:
	printstring STRINGID_ENCCOBALIONDECLAREPHYSICAL
	goto EncScript_Cobalion_ResolveDeclare_Brace
EncScript_Cobalion_ResolveDeclare_Spec:
	printstring STRINGID_ENCCOBALIONDECLARESPECIAL
EncScript_Cobalion_ResolveDeclare_Brace:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
EncScript_Cobalion_ResolveDeclare_Done:
	return

// THE LEDGER, three arms of the same script with three literals. Hit is set first, so the ledger
// can only speak once per turn however many moves resolve. Each arm compares LastCat against its
// OWN literal, which is how "did they do the same kind of thing again" is asked without a
// var-to-var comparison, and then resolves Cobalion's declared read against the same literal.
EncScript_Cobalion_StruckPhysical::
	encsetvar 9, 1      @ Hit
	encsetvar 6, 1      @ ThisCat: physical
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Cobalion_StruckPhysical_Read
	encsetvar 7, 1      @ Broke
EncScript_Cobalion_StruckPhysical_Read:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Cobalion_StruckPhysical_Done
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Cobalion_ReadLands
	goto EncScript_Cobalion_ReadMisplaced
EncScript_Cobalion_StruckPhysical_Done:
	return

EncScript_Cobalion_StruckSpecial::
	encsetvar 9, 1      @ Hit
	encsetvar 6, 2      @ ThisCat: special
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Cobalion_StruckSpecial_Read
	encsetvar 7, 1      @ Broke
EncScript_Cobalion_StruckSpecial_Read:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Cobalion_StruckSpecial_Done
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Cobalion_ReadLands
	goto EncScript_Cobalion_ReadMisplaced
EncScript_Cobalion_StruckSpecial_Done:
	return

EncScript_Cobalion_TheyManeuvered::
	encsetvar 9, 1      @ Hit
	encsetvar 6, 3      @ ThisCat: status
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Cobalion_TheyManeuvered_Read
	encsetvar 7, 1      @ Broke
EncScript_Cobalion_TheyManeuvered_Read:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Cobalion_TheyManeuvered_Done
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Cobalion_ReadLands
	goto EncScript_Cobalion_ReadMisplaced
EncScript_Cobalion_TheyManeuvered_Done:
	return

// A player who answers every stance change with a switch is repeating a pattern and gains
// Discipline exactly like a player who spams one attack - the spec's "same Pokemon, same
// strategy" clause. A declared read is never resolved against a switch: no category matches it,
// and _TurnOpen clears it next turn.
EncScript_Cobalion_TheyRepositioned::
	encsetvar 10, 1     @ EventGuard
	encsetvar 6, 4      @ ThisCat: repositioned
	encjumpifvar CMP_EQUAL, 5, 4, EncScript_Cobalion_TheyRepositioned_Done
	encsetvar 7, 1      @ Broke
EncScript_Cobalion_TheyRepositioned_Done:
	return

// A KO is followed by a replacement switch-in. EventGuard is what stops that replacement being
// charged as a voluntary repositioning; nothing else about the ledger moves.
EncScript_Cobalion_TheyFell::
	encsetvar 10, 1     @ EventGuard
	return

// Runs last, so it sees a finished turn. Doing nothing is itself a pattern, and the stall ledger
// is what charges for it - which is what closes Protect/Substitute/recover looping. It cannot be
// dodged by alternating Protect with a status move, because a status move sets ThisCat and
// therefore feeds the ordinary Discipline path instead.
EncScript_Cobalion_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Cobalion_TurnClose_Clear
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Cobalion_TurnClose_Acted
	encaddvar 12, 1     @ Stall
	encjumpifvar CMP_LESS_THAN, 12, 3, EncScript_Cobalion_TurnClose_Announce
	printstring STRINGID_ENCCOBALIONCONTEMPT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SACRED_SWORD
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	call EncScript_Cobalion_RaiseDiscipline
	encsetvar 12, 0     @ Stall
	goto EncScript_Cobalion_TurnClose_Announce
EncScript_Cobalion_TurnClose_Acted:
	encsetvar 12, 0     @ Stall
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Cobalion_TurnClose_Broke
	call EncScript_Cobalion_RaiseDiscipline
	enccopyvar 5, 6     @ LastCat = ThisCat
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Cobalion_TurnClose_Announce
	encjumpifvar CMP_NOT_EQUAL, 1, 0, EncScript_Cobalion_TurnClose_Announce
	call EncScript_Cobalion_Counterstrike
	goto EncScript_Cobalion_TurnClose_Announce
EncScript_Cobalion_TurnClose_Broke:
	call EncScript_Cobalion_LowerDiscipline
	enccopyvar 5, 6     @ LastCat = ThisCat
EncScript_Cobalion_TurnClose_Announce:
	call EncScript_Cobalion_AnnounceDiscipline
	call EncScript_Cobalion_ApplyGuard      @ absolute: restores a misplaced read's drop
EncScript_Cobalion_TurnClose_Clear:
	encsetvar 8, 0      @ TurnGuard
	return

// THE READ. Both halves have a fail label because both can legitimately find nothing to take -
// turn 1, a move already disabled or out of PP, a player down to their last Pokemon. THE READ
// then does only the half that worked, silently. Discipline is spent back down to 3 afterwards,
// so this is a spike rather than a lock.
EncScript_Cobalion_TheRead::
	printstring STRINGID_ENCCOBALIONTHEREAD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM  @ never TERA_CHARGE bare - see the constant's comment
	encdisablemove ENC_TARGET_PLAYER_LEFT, 3, EncScript_Cobalion_TheRead_NoMove
	printstring STRINGID_ENCCOBALIONLOCKED
	waitmessage B_WAIT_TIME_LONG
EncScript_Cobalion_TheRead_NoMove:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Cobalion_TheRead_Done
	printstring STRINGID_ENCCOBALIONREPOSITION
	waitmessage B_WAIT_TIME_LONG
	encswitchout ENC_TARGET_PLAYER_LEFT, EncScript_Cobalion_TheRead_Done
EncScript_Cobalion_TheRead_Done:
	encsetvar 3, 3      @ Discipline, spent back down
	encsetvar 13, 3     @ Cooldown
	call EncScript_Cobalion_AnnounceDiscipline
	return

// Aura Sphere in COMMAND is the other half of this answer. Between them, Double Team is a
// delaying tactic rather than a win condition.
EncScript_Cobalion_SeeThrough::
	encsetvar 14, 1     @ Seen
	printstring STRINGID_ENCCOBALIONSEESTHROUGH
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_ALL_FOES, STAT_EVASION, -2
	return

// --- Phases ---

// 50% - LEADER OF THE SWORDS. Raw stats rather than stages, deliberately: this is the spec's
// permanent enhanced state, and it should survive a Haze. TACTICAL COMMAND becomes live because
// its trigger's condition is Phase >= 1.
EncScript_Cobalion_Leader::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCCOBALIONLEADER1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 25, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 15, ENC_AMOUNT_PERCENT
	restoremovepp BS_OPPONENT1
	encjumpifvar CMP_GREATER_THAN, 3, 0, EncScript_Cobalion_Leader_Guard
	encsetvar 3, 1      @ Discipline, floored at 1 from here
EncScript_Cobalion_Leader_Guard:
	call EncScript_Cobalion_ApplyGuard
	call EncScript_Cobalion_AnnounceDiscipline
	printstring STRINGID_ENCCOBALIONLEADER2
	waitmessage B_WAIT_TIME_LONG
	return

// 25% - IRON RESOLVE. The Discipline floor rises to 2, counters and the read hit harder, ASSAULT's
// guard falls to 80, and _TurnOpen's last clause forces a stance flip on any turn the ledger would
// have left it alone. Survive: True is load-bearing from here.
EncScript_Cobalion_IronResolve::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCCOBALIONRESOLVE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HANGED_ON
	restoremovepp BS_OPPONENT1
	encjumpifvar CMP_GREATER_THAN, 3, 1, EncScript_Cobalion_IronResolve_Guard
	encsetvar 3, 2      @ Discipline, floored at 2 from here
EncScript_Cobalion_IronResolve_Guard:
	call EncScript_Cobalion_ApplyGuard
	call EncScript_Cobalion_AnnounceDiscipline
	printstring STRINGID_ENCCOBALIONRESOLVE2
	waitmessage B_WAIT_TIME_LONG
	return

// 1 HP - THE LAST STAND, and the catch window (guideline 10). Every trigger that could still fire
// is gated Phase <= 2, so from here the fight is quiet: no stances, no Discipline, no counters. A
// player lining up a ball should not be answering a counterattack.
// encchangehp faints a battler it takes to 0, exactly like weather chip, so the final assault can
// end the battle against a nearly-dead team. That is the intended weight of the beat.
// The reduction set below is the value the automatic catch-window guard will RESTORE, not the live
// one - that guard pins the live number at ENC_MAX_DAMAGE_REDUCTION for as long as balls are
// allowed, which is what keeps the catch target alive.
EncScript_Cobalion_LastStand::
	encsetvar 0, 3      @ Phase
	printstring STRINGID_ENCCOBALIONSTANDSFIRM
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_HANGED_ON
	printstring STRINGID_ENCCOBALIONFINALASSAULT
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SACRED_SWORD
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -35, ENC_AMOUNT_PERCENT
	call EncScript_Cobalion_ExitProfile
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -2
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -2
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCCOBALIONWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
