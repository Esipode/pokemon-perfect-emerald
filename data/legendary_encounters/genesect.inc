// Genesect, "The Weapon Platform". Var indices, pinned by the always-true Conditions on
// EncScript_Genesect_Intro:
// 0 Phase (0 Drive System / 1 Combat Protocol / 2 Weaponized Adaptation / 3 Systems Down),
// 1 Drive (0 none / 1 Burn / 2 Chill / 3 Douse / 4 Shock), 2 Charge (Weapon Charge, 0-5),
// 3 Strain (System Strain, 0-3), 4 Load (adaptations held - written ONLY by encadapt/encpurgeadapt),
// 5 Result (encadapt's ENC_ADAPT_RESULT_*, or 255 for "it did not file"), 6 TurnGuard,
// 7 MoveGuard (the analysis arms' per-turn re-entry guard: 0 armed / 1 a hit was filed / 2 a Drive
// was changed - which is also what Strain's decay tests), 8 PlayerMark, 9 BossMark, 10 Scratch,
// 11 Lock (TARGET LOCK, one variable for both halves of its cycle: 0 idle, 1-3 turns of trap
// remaining, 4-6 turns of cooldown remaining), 12 FaintGuard, 13 Overclock, 14 Cooling,
// 15 PrevDrive (the Drive displaced by the last re-equip - Dual Drive's second half).
//
// GENESECT ANSWERS WHATEVER YOU LAST DID. The attack that lands is filed by encadapt and stops
// working; the same hit equips the Drive that counters it, and every new Drive is a point of System
// Strain. Three consecutive off-profile turns overload the machine and wipe the file. The board is
// the fight's variable; the flat 86 guard underneath never moves except inside a cooling window.

// --- Shared subroutines (call/return) ---

// ANALYSIS, the single writer of the type board. Result is set to the 255 sentinel first, so a move
// that missed, was blocked or had no effect - which encadapt ignores outright, leaving both vars
// alone - leaves the calling arm able to tell that nothing was filed and skip the re-equip with it.
// The second encadapt is the HARDENING step: the command only ever raises a held type's percent, so
// re-filing at a higher number is how a type the player keeps throwing climbs toward the 90% ceiling
// within one phase. Repeating it is idempotent - the third hit finds the percent already there.
EncScript_Genesect_File:
	encsetvar 5, 255
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Genesect_File_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Genesect_File_P1
	encadapt ENC_TARGET_BOSS, 50, 2, 4, 5
	encjumpifvar CMP_NOT_EQUAL, 5, ENC_ADAPT_RESULT_HARDENED, EncScript_Genesect_File_Report
	encadapt ENC_TARGET_BOSS, 65, 2, 4, 5
	goto EncScript_Genesect_File_Report
EncScript_Genesect_File_P1:
	encadapt ENC_TARGET_BOSS, 60, 2, 4, 5
	encjumpifvar CMP_NOT_EQUAL, 5, ENC_ADAPT_RESULT_HARDENED, EncScript_Genesect_File_Report
	encadapt ENC_TARGET_BOSS, 75, 2, 4, 5
	goto EncScript_Genesect_File_Report
EncScript_Genesect_File_P2:
	encadapt ENC_TARGET_BOSS, 70, 3, 4, 5
	encjumpifvar CMP_NOT_EQUAL, 5, ENC_ADAPT_RESULT_HARDENED, EncScript_Genesect_File_Report
	encadapt ENC_TARGET_BOSS, 90, 3, 4, 5
// The whole of guideline 14 for the damage layer: the player is told, by name, which type just
// stopped working and which one is working again.
EncScript_Genesect_File_Report:
	encjumpifvar CMP_EQUAL, 5, ENC_ADAPT_RESULT_EVICTED, EncScript_Genesect_File_Evicted
	encjumpifvar CMP_EQUAL, 5, ENC_ADAPT_RESULT_FILED, EncScript_Genesect_File_Filed
	encjumpifvar CMP_EQUAL, 5, ENC_ADAPT_RESULT_HARDENED, EncScript_Genesect_File_Hardened
	return            @ nothing landed - the command left the board and both vars alone
EncScript_Genesect_File_Filed:
	printstring STRINGID_ENCGENESECTFILED
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Genesect_File_Hardened:
	printstring STRINGID_ENCGENESECTHARDENED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return
EncScript_Genesect_File_Evicted:
	printstring STRINGID_ENCGENESECTEVICTED
	waitmessage B_WAIT_TIME_LONG
	return

// Every re-equip opens here. MoveGuard is raised to 2, which is the only thing Strain's decay reads:
// a turn that changed a Drive is a turn that does not decay. PrevDrive takes the outgoing Drive for
// DUAL DRIVE, and the lock breaks because the analysis has re-targeted.
EncScript_Genesect_EquipBegin:
	encsetvar 7, 2     @ MoveGuard: a Drive changed this turn
	enccopyvar 15, 1   @ PrevDrive = the outgoing Drive
	call EncScript_Genesect_LockRelease
	return

// A FIRE move landed: DOUSE. Rain halves the Fire damage that is Bug/Steel's 4x weakness, so the
// fight's sharpest answer is also the one thing that arms the counter to itself. Douse keeps Bug
// Buzz in slot 3 - Genesect has no Water move to learn.
EncScript_Genesect_EquipDouse:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Genesect_EquipDouse_Done
	call EncScript_Genesect_EquipBegin
	encsetvar 1, 3     @ Drive: Douse
	encformchange ENC_TARGET_BOSS, SPECIES_GENESECT_DOUSE, EncScript_Genesect_EquipDouse_Arm, B_ANIM_FORM_CHANGE
EncScript_Genesect_EquipDouse_Arm:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BUG_BUZZ
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCGENESECTDOUSE
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_RAIN, 0
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Genesect_EquipDouse_One
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	goto EncScript_Genesect_EquipDouse_Dual
EncScript_Genesect_EquipDouse_One:
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
EncScript_Genesect_EquipDouse_Dual:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Genesect_EquipDouse_Strain
	call EncScript_Genesect_DualDrive
EncScript_Genesect_EquipDouse_Strain:
	call EncScript_Genesect_Strain
EncScript_Genesect_EquipDouse_Done:
	return

// A PHYSICAL move landed: CHILL. No field effect of its own - it answers by taking a step of the
// player's Speed as well as raising its own Defense, so the physical attacker loses the trade twice.
EncScript_Genesect_EquipChill:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Genesect_EquipChill_Done
	call EncScript_Genesect_EquipBegin
	encsetvar 1, 2     @ Drive: Chill
	encformchange ENC_TARGET_BOSS, SPECIES_GENESECT_CHILL, EncScript_Genesect_EquipChill_Arm, B_ANIM_FORM_CHANGE
EncScript_Genesect_EquipChill_Arm:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_ICE_BEAM
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCGENESECTCHILL
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Genesect_EquipChill_One
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	goto EncScript_Genesect_EquipChill_Dual
EncScript_Genesect_EquipChill_One:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
EncScript_Genesect_EquipChill_Dual:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Genesect_EquipChill_Strain
	call EncScript_Genesect_DualDrive
EncScript_Genesect_EquipChill_Strain:
	call EncScript_Genesect_Strain
EncScript_Genesect_EquipChill_Done:
	return

// A SPECIAL move landed: SHOCK. Electric Terrain is the anti-abuse half of the fight's answer to
// sleep - a grounded Pokemon cannot be put to sleep on it, and Shock is exactly what equips against
// the special attackers who carry Spore and Hypnosis.
EncScript_Genesect_EquipShock:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Genesect_EquipShock_Done
	call EncScript_Genesect_EquipBegin
	encsetvar 1, 4     @ Drive: Shock
	encformchange ENC_TARGET_BOSS, SPECIES_GENESECT_SHOCK, EncScript_Genesect_EquipShock_Arm, B_ANIM_FORM_CHANGE
EncScript_Genesect_EquipShock_Arm:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_THUNDERBOLT
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCGENESECTSHOCK
	waitmessage B_WAIT_TIME_LONG
	encsetterrain ENC_TERRAIN_ELECTRIC, 0
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Genesect_EquipShock_One
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	goto EncScript_Genesect_EquipShock_Dual
EncScript_Genesect_EquipShock_One:
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1
EncScript_Genesect_EquipShock_Dual:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Genesect_EquipShock_Strain
	call EncScript_Genesect_DualDrive
EncScript_Genesect_EquipShock_Strain:
	call EncScript_Genesect_Strain
EncScript_Genesect_EquipShock_Done:
	return

// NOTHING landed on it all turn: BURN. The anti-stall arm - Protect, a miss, a sleep turn, a
// Substitute and a turn spent healing all resolve to the same measurable fact, and all of them make
// the machine hit harder.
EncScript_Genesect_EquipBurn:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Genesect_EquipBurn_Done
	call EncScript_Genesect_EquipBegin
	encsetvar 1, 1     @ Drive: Burn
	encformchange ENC_TARGET_BOSS, SPECIES_GENESECT_BURN, EncScript_Genesect_EquipBurn_Arm, B_ANIM_FORM_CHANGE
EncScript_Genesect_EquipBurn_Arm:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_FLAMETHROWER
	restoremovepp BS_OPPONENT1
	printstring STRINGID_ENCGENESECTBURN
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SUN, 0
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Genesect_EquipBurn_One
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	goto EncScript_Genesect_EquipBurn_Dual
EncScript_Genesect_EquipBurn_One:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
EncScript_Genesect_EquipBurn_Dual:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Genesect_EquipBurn_Strain
	call EncScript_Genesect_DualDrive
EncScript_Genesect_EquipBurn_Strain:
	call EncScript_Genesect_Strain
EncScript_Genesect_EquipBurn_Done:
	return

// DUAL DRIVE (Phase 2 only). A re-equip no longer discards the Drive it displaced: its field effect
// is simply left standing and its stat bump re-applied at +1, so the machine runs two loadouts at
// once. Which two is whatever the player's own last two attacks produced. Weather and terrain are
// separate channels, so a Shock pairing genuinely coexists; two weather Drives overwrite each other
// and only the stat halves stack, which is the honest behaviour of the field.
EncScript_Genesect_DualDrive:
	encjumpifvar CMP_EQUAL, 15, 0, EncScript_Genesect_DualDrive_Done
	printstring STRINGID_ENCGENESECTDUALDRIVE
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 15, 1, EncScript_Genesect_DualDrive_Burn
	encjumpifvar CMP_EQUAL, 15, 2, EncScript_Genesect_DualDrive_Chill
	encjumpifvar CMP_EQUAL, 15, 3, EncScript_Genesect_DualDrive_Douse
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 1     @ Shock
	return
EncScript_Genesect_DualDrive_Burn:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	return
EncScript_Genesect_DualDrive_Chill:
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 1
	return
EncScript_Genesect_DualDrive_Douse:
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
EncScript_Genesect_DualDrive_Done:
	return

// SYSTEM STRAIN. Announced on every change, because the only script that writes the value is the one
// that says it - so the callout re-fires whenever the number moves rather than being latched to a
// single showing. At 3 the machine gives out: an Overclock in flight breaks catastrophically,
// otherwise it simply overloads.
EncScript_Genesect_Strain:
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Genesect_Strain_Done
	encaddvar 3, 1
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Genesect_Strain_Three
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Genesect_Strain_Two
	printstring STRINGID_ENCGENESECTSTRAIN1
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Genesect_Strain_Two:
	printstring STRINGID_ENCGENESECTSTRAIN2
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Genesect_Strain_Three:
	printstring STRINGID_ENCGENESECTSTRAIN3
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_GREATER_THAN, 13, 0, EncScript_Genesect_Strain_Break
	call EncScript_Genesect_Overload
	return
EncScript_Genesect_Strain_Break:
	call EncScript_Genesect_OverclockBreak
EncScript_Genesect_Strain_Done:
	return

// SYSTEM OVERLOAD - what the player is playing for. Everything the machine built comes off at once:
// the file is wiped, the Drive ejected back to base Genesect, the Charge lost, and the flat guard
// collapses to 60 for two turns while it spends next turn's action recovering.
EncScript_Genesect_Overload:
	printstring STRINGID_ENCGENESECTOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_ALL, 4
	encformchange ENC_TARGET_BOSS, SPECIES_GENESECT, EncScript_Genesect_Overload_Eject, B_ANIM_FORM_CHANGE
EncScript_Genesect_Overload_Eject:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BUG_BUZZ
	restoremovepp BS_OPPONENT1
	encsetvar 1, 0     @ Drive
	encsetvar 15, 0    @ PrevDrive
	encsetvar 3, 0     @ Strain
	encsetvar 2, 0     @ Charge
	encsetvar 14, 3    @ Cooling: two full turns of window
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetrecharge ENC_TARGET_BOSS, 2
	call EncScript_Genesect_LockRelease
	printstring STRINGID_ENCGENESECTEXPOSED
	waitmessage B_WAIT_TIME_LONG
	return

// The catastrophic overheat: the Overclock interrupted, by damage or by a third System Strain. The
// fight's largest window, and the only place the guard goes below 60. Two actions lost, three turns
// of collapsed guard.
EncScript_Genesect_OverclockBreak:
	printstring STRINGID_ENCGENESECTOVERCLOCKBREAK
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_OVERHEAT
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_ALL, 4
	encformchange ENC_TARGET_BOSS, SPECIES_GENESECT, EncScript_Genesect_OverclockBreak_Eject, B_ANIM_FORM_CHANGE
EncScript_Genesect_OverclockBreak_Eject:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BUG_BUZZ
	restoremovepp BS_OPPONENT1
	encsetvar 1, 0     @ Drive
	encsetvar 15, 0    @ PrevDrive
	encsetvar 3, 0     @ Strain
	encsetvar 2, 0     @ Charge
	encsetvar 13, 0    @ Overclock
	encsetvar 14, 4    @ Cooling: three full turns of window
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetrecharge ENC_TARGET_BOSS, 3
	call EncScript_Genesect_LockRelease
	printstring STRINGID_ENCGENESECTEXPOSED
	waitmessage B_WAIT_TIME_LONG
	return

// TARGET LOCK's early break, reached from any Drive change, an overload and the catastrophic
// overheat. Values 4-6 are the cooldown half of the same variable, so a release that arrives while
// the lock is already down is a no-op. The player is never left guessing whether they may switch.
EncScript_Genesect_LockRelease:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Genesect_LockRelease_Done
	encjumpifvar CMP_GREATER_THAN, 11, 3, EncScript_Genesect_LockRelease_Done
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 11, 6    @ the cooldown half of the cycle
	printstring STRINGID_ENCGENESECTLOCKBROKEN
	waitmessage B_WAIT_TIME_LONG
EncScript_Genesect_LockRelease_Done:
	return

// The Target Lock cycle, ticked from _TurnOpen rather than from a trigger of its own: a countdown
// whose condition stays true after the decrement would re-select itself through every remaining
// value inside one dispatch. 1-3 are trap turns, 4-6 cooldown turns, 0 idle and re-armable.
EncScript_Genesect_LockTick:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Genesect_LockTick_Done
	encjumpifvar CMP_GREATER_THAN, 11, 4, EncScript_Genesect_LockTick_Cool
	encjumpifvar CMP_EQUAL, 11, 4, EncScript_Genesect_LockTick_Ready
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Genesect_LockTick_Expire
	encsubvar 11, 1
	printstring STRINGID_ENCGENESECTLOCKHOLD
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Genesect_LockTick_Cool:
	encsubvar 11, 1
	return
EncScript_Genesect_LockTick_Ready:
	encsetvar 11, 0
	return
EncScript_Genesect_LockTick_Expire:
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 11, 6
	printstring STRINGID_ENCGENESECTLOCKEXPIRES
	waitmessage B_WAIT_TIME_LONG
EncScript_Genesect_LockTick_Done:
	return

// The cooling window, ticked from _TurnOpen. Nothing else writes the guard, so the window can never
// be left stuck open: at 1 the value goes back to 86 absolutely. The heartbeat line is the standing
// "the mechanic is off" cue - a player who missed the overload still reads, every turn of the
// window, that it is still open.
EncScript_Genesect_CoolingTick:
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_Genesect_CoolingTick_Done
	encjumpifvar CMP_GREATER_THAN, 14, 1, EncScript_Genesect_CoolingTick_Open
	encsetvar 14, 0
	encsetdamagereduction ENC_TARGET_BOSS, 88
	printstring STRINGID_ENCGENESECTONLINE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	return
EncScript_Genesect_CoolingTick_Open:
	encsubvar 14, 1
	printstring STRINGID_ENCGENESECTEXPOSEDHOLD
	waitmessage B_WAIT_TIME_SHORT
EncScript_Genesect_CoolingTick_Done:
	return

// The Overclock's destabilisation countdown, ticked from _TurnOpen for the same re-selection reason
// as the lock, and resolved one checkpoint away at OnTurnEnd. Warned on every tick, escalating, so
// the three turns the player has to answer it are never silent.
EncScript_Genesect_OverclockTick:
	encjumpifvar CMP_LESS_THAN, 13, 2, EncScript_Genesect_OverclockTick_Done
	encsubvar 13, 1
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Genesect_OverclockTick_One
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Genesect_OverclockTick_Two
	printstring STRINGID_ENCGENESECTOVERCLOCK3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Genesect_OverclockTick_Two:
	printstring STRINGID_ENCGENESECTOVERCLOCK2
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Genesect_OverclockTick_One:
	printstring STRINGID_ENCGENESECTOVERCLOCK1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
EncScript_Genesect_OverclockTick_Done:
	return

// The Weapon Charge. _ChargeStep owns the counter and says nothing except when the cannon comes
// round; the two callers own the dialogue, so a heavy hit reads as one escalating line rather than
// the light line twice.
EncScript_Genesect_ChargeStep:
	encjumpifvar CMP_GREATER_THAN, 2, 4, EncScript_Genesect_ChargeStep_Done
	encaddvar 2, 1
	encjumpifvar CMP_NOT_EQUAL, 2, 5, EncScript_Genesect_ChargeStep_Done
	printstring STRINGID_ENCGENESECTCHARGEFULL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
EncScript_Genesect_ChargeStep_Done:
	return

EncScript_Genesect_ChargeLight:
	printstring STRINGID_ENCGENESECTCHARGE
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Genesect_ChargeStep
	return

// A turn that cost the player a quarter of their max HP, or a knockout. From Phase 1 it is worth two
// steps instead of one, which is what "heavy hits charge the cannon twice as fast" means.
EncScript_Genesect_ChargeHeavy:
	printstring STRINGID_ENCGENESECTCHARGEHEAVY
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Genesect_ChargeStep
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Genesect_ChargeHeavy_Done
	call EncScript_Genesect_ChargeStep
EncScript_Genesect_ChargeHeavy_Done:
	return

// The standing cues (guideline 14). Neither is latched to a single showing: the cannon line runs for
// as long as the cannon is nearly round, and the plating line for as long as the board is at the
// capacity this phase allows - which is exactly when the player's attacks have stopped working.
// Both stand down while the Overclock countdown or a cooling window is running: those say the same
// thing louder, and five message boxes at one turn open is more than anyone reads.
EncScript_Genesect_Cues:
	encjumpifvar CMP_GREATER_THAN, 13, 0, EncScript_Genesect_Cues_Done   @ the Overclock warning is louder
	encjumpifvar CMP_GREATER_THAN, 14, 0, EncScript_Genesect_Cues_Done   @ so is an open cooling window
	encjumpifvar CMP_LESS_THAN, 2, 4, EncScript_Genesect_Cues_Board
	printstring STRINGID_ENCGENESECTCANNONREMIND
	waitmessage B_WAIT_TIME_SHORT
EncScript_Genesect_Cues_Board:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Genesect_Cues_Three
	encjumpifvar CMP_LESS_THAN, 4, 2, EncScript_Genesect_Cues_Done
	goto EncScript_Genesect_Cues_Hum
EncScript_Genesect_Cues_Three:
	encjumpifvar CMP_LESS_THAN, 4, 3, EncScript_Genesect_Cues_Done
EncScript_Genesect_Cues_Hum:
	printstring STRINGID_ENCGENESECTPLATINGHUM
	waitmessage B_WAIT_TIME_SHORT
EncScript_Genesect_Cues_Done:
	return

// The Overclock's battlefield payload, chosen by the Drive that was equipped when it fired.
EncScript_Genesect_OverclockField:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Genesect_OverclockField_Burn
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Genesect_OverclockField_Chill
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Genesect_OverclockField_Douse
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Genesect_OverclockField_Shock
	return
EncScript_Genesect_OverclockField_Burn:
	encsetweather BATTLE_WEATHER_SUN, 0
	return
EncScript_Genesect_OverclockField_Chill:
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -2
	return
EncScript_Genesect_OverclockField_Douse:
	encsetweather BATTLE_WEATHER_RAIN, 0
	return
EncScript_Genesect_OverclockField_Shock:
	encsetterrain ENC_TERRAIN_ELECTRIC, 0
	return

// --- Trigger scripts ---

EncScript_Genesect_Intro::
	printstring STRINGID_ENCGENESECTINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCGENESECTINTRO2
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn. Leads with flushtextbox - an OnTurnStart script that animates before printing
// anything renders on top of the still-open action menu. TurnGuard is set immediately so no branch
// can leave it re-armed. Every countdown in the fight ticks here and resolves at OnTurnEnd, one
// checkpoint away, and the two HP marks the turn is measured against are taken as percentages - the
// only portable choice under Level: LevelCap and New Game Plus offsets.
EncScript_Genesect_TurnOpen::
	flushtextbox
	encsetvar 6, 1     @ TurnGuard
	encsetvar 7, 0     @ MoveGuard: the analysis arms are armed again
	encsetvar 12, 0    @ FaintGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Genesect_TurnOpen_Done   @ Phase 3: inert
	call EncScript_Genesect_CoolingTick
	call EncScript_Genesect_LockTick
	call EncScript_Genesect_OverclockTick
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 8, ENC_SNAP_SET
	encsnapshothp ENC_TARGET_BOSS, 9, ENC_SNAP_SET
	call EncScript_Genesect_Cues
EncScript_Genesect_TurnOpen_Done:
	return

// Phase 1+ anti-abuse: a non-volatile status is vented at turn open, so status control keeps working
// but costs a move every turn instead of once. Self-disabling - it cures the status its own trigger
// condition reads.
EncScript_Genesect_Vent::
	printstring STRINGID_ENCGENESECTPURGE
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// TARGET LOCK (Phase 1+, Charge 3+). Announced before the player picks a move, so the turn can be
// spent on the answer: any Drive change breaks it, and so does an overload. Switching cannot be the
// break when the lock's whole content is that you may not switch. Ghost-types walk out regardless,
// exactly as they do from Mean Look.
EncScript_Genesect_TargetLock::
	printstring STRINGID_ENCGENESECTTARGETLOCKED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	encsetvar 11, 3    @ three turns of trap
	return

// The three analysis arms. Each files first and only re-equips if something was actually filed, so a
// move that missed or had no effect teaches the machine nothing. MoveGuard is raised by _File's
// caller before anything else: an Event.* condition stays true for the whole dispatch and a script
// cannot move it, so an event-reacting trigger has to disable itself.
// FIRE is tested before category, so a Fire special move equips DOUSE rather than SHOCK - and a Fire
// PHYSICAL move equips Douse too, which is the rule and not a bug.
EncScript_Genesect_AnalyzeFire::
	encsetvar 7, 1     @ MoveGuard
	call EncScript_Genesect_File
	encjumpifvar CMP_EQUAL, 5, 255, EncScript_Genesect_AnalyzeFire_Done
	call EncScript_Genesect_EquipDouse
EncScript_Genesect_AnalyzeFire_Done:
	return

EncScript_Genesect_AnalyzePhysical::
	encsetvar 7, 1     @ MoveGuard
	call EncScript_Genesect_File
	encjumpifvar CMP_EQUAL, 5, 255, EncScript_Genesect_AnalyzePhysical_Done
	call EncScript_Genesect_EquipChill
EncScript_Genesect_AnalyzePhysical_Done:
	return

EncScript_Genesect_AnalyzeSpecial::
	encsetvar 7, 1     @ MoveGuard
	call EncScript_Genesect_File
	encjumpifvar CMP_EQUAL, 5, 255, EncScript_Genesect_AnalyzeSpecial_Done
	call EncScript_Genesect_EquipShock
EncScript_Genesect_AnalyzeSpecial_Done:
	return

// A player Pokemon went down. The ledger cannot see this for itself: by OnTurnEnd the replacement is
// already in at full HP, so the turn that cost the player a Pokemon would read as a turn that cost
// them nothing. A knockout always satisfies the heavy clause.
EncScript_Genesect_TheyFell::
	encsetvar 12, 1    @ FaintGuard
	call EncScript_Genesect_ChargeLight
	call EncScript_Genesect_ChargeHeavy
	return

// THE LEDGER, at OnTurnEnd priority 80, so it sees a finished turn.
//   1. What the turn cost the player -> the Weapon Charge, and the heavy clause at 25%.
//   2. What the turn cost Genesect. Nothing at all -> BURN, and the flame chip: the anti-stall arm.
//      Enough inside an Overclock window -> the weapon breaks.
//   3. Otherwise System Strain decays, on any turn no Drive was changed (MoveGuard 2 is the only
//      thing that says one was). That decay is what makes an overload cost three CONSECUTIVE
//      off-profile turns instead of being banked slowly.
EncScript_Genesect_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Genesect_TurnClose_Quiet   @ Phase 3: quiet
	enccopyvar 10, 8   @ Scratch = PlayerMark
	encsnapshothp ENC_TARGET_PLAYER_LEFT, 10, ENC_SNAP_DAMAGE, EncScript_Genesect_TurnClose_Boss
	call EncScript_Genesect_ChargeLight
	encjumpifvar CMP_LESS_THAN, 10, 25, EncScript_Genesect_TurnClose_Boss
	call EncScript_Genesect_ChargeHeavy
EncScript_Genesect_TurnClose_Boss:
	enccopyvar 10, 9   @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Genesect_TurnClose_Nothing
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Genesect_TurnClose_Decay
	encjumpifvar CMP_LESS_THAN, 10, 18, EncScript_Genesect_TurnClose_Decay
	call EncScript_Genesect_OverclockBreak
	goto EncScript_Genesect_TurnClose_Quiet
EncScript_Genesect_TurnClose_Nothing:
	printstring STRINGID_ENCGENESECTSCANNING
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Genesect_EquipBurn
	printstring STRINGID_ENCGENESECTFLAMECHIP
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FLAMETHROWER
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	goto EncScript_Genesect_TurnClose_Quiet   @ a Burn equip IS a re-equip: nothing decays
EncScript_Genesect_TurnClose_Decay:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Genesect_TurnClose_Quiet
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Genesect_TurnClose_Quiet
	encsubvar 3, 1     @ Strain
	printstring STRINGID_ENCGENESECTSTRAINEASE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Genesect_TurnClose_Quiet:
	encsetvar 6, 0     @ TurnGuard
	encsetvar 7, 0     @ MoveGuard
	encsetvar 12, 0    @ FaintGuard
	return

// TECHNO BLAST (Phase 0/1). Runs after _TurnClose (80), which is what raises Charge to 5, so it is
// selected on the re-evaluation pass that follows. Firing is itself a window: the weapon overheats,
// the newest adaptation is vented off the board and Genesect loses next turn's action.
EncScript_Genesect_TechnoBlast::
	printstring STRINGID_ENCGENESECTCANNONFIRES
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_TECHNO_BLAST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Genesect_TechnoBlast_Locked
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Genesect_TechnoBlast_Locked
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Genesect_TechnoBlast_Locked
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Genesect_TechnoBlast_Ph1
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	goto EncScript_Genesect_TechnoBlast_Overheat
EncScript_Genesect_TechnoBlast_Ph1:
	encchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Genesect_TechnoBlast_Overheat
EncScript_Genesect_TechnoBlast_Locked:
	printstring STRINGID_ENCGENESECTLOCKEDSHOT
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
EncScript_Genesect_TechnoBlast_Overheat:
	encsetvar 2, 0     @ Charge
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Genesect_TechnoBlast_Spent
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_NEWEST, 4
	printstring STRINGID_ENCGENESECTVENTED
	waitmessage B_WAIT_TIME_LONG
EncScript_Genesect_TechnoBlast_Spent:
	encsetrecharge ENC_TARGET_BOSS, 2
	printstring STRINGID_ENCGENESECTOVERHEATS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	return

// Phase 2: the cannon no longer fires on reaching 5. It destabilises for three warned turns instead,
// and the player has two ways to take it apart - 18% of Genesect's max HP inside the window, or a
// third System Strain. Those are the two verbs the fight has spent the whole battle teaching.
EncScript_Genesect_OverclockStart::
	encsetvar 13, 4    @ Overclock: three warned ticks
	printstring STRINGID_ENCGENESECTOVERCLOCK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	return

EncScript_Genesect_OverclockFire::
	encsetvar 13, 0    @ Overclock
	encsetvar 2, 0     @ Charge
	printstring STRINGID_ENCGENESECTOVERCLOCKFIRE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_TECHNO_BLAST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -38, ENC_AMOUNT_PERCENT
	call EncScript_Genesect_OverclockField
	encsetrecharge ENC_TARGET_BOSS, 2
	printstring STRINGID_ENCGENESECTOVERHEATS
	waitmessage B_WAIT_TIME_LONG
	return

// --- Phases ---

// 50% - COMBAT PROTOCOL. The board files at 60%, the Drive stat bumps double, a status is vented at
// turn open instead of carried, TARGET LOCK goes live and heavy hits charge the cannon twice as
// fast. Every one of those reads a Phase value, so the transition is the whole change.
EncScript_Genesect_CombatProtocol::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCGENESECTPROTOCOL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	printstring STRINGID_ENCGENESECTPROTOCOL2
	waitmessage B_WAIT_TIME_LONG
	return

// 20% - WEAPONIZED ADAPTATION. Three slots at 70%, DUAL DRIVE, and the cannon stops firing on
// reaching 5 - it overclocks instead.
EncScript_Genesect_Weaponized::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCGENESECTWEAPONIZED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	printstring STRINGID_ENCGENESECTWEAPONIZED2
	waitmessage B_WAIT_TIME_LONG
	return

// 1 HP - SYSTEMS DOWN, and the catch window (guideline 10). One last shot, then the file is wiped,
// the Drive ejected so a caught Genesect is its own base form, the trap released, and the guard, the
// immunities and Survive all come off. Every trigger that could still fire is gated Phase <= 2, so
// from here the fight is quiet. encsetdamagereduction writes the value the engine's automatic
// catch-window guard will RESTORE, not the live one.
EncScript_Genesect_Down::
	encsetvar 0, 3     @ Phase
	encsetvar 2, 0     @ Charge
	encsetvar 3, 0     @ Strain
	encsetvar 13, 0    @ Overclock
	encsetvar 14, 0    @ Cooling
	printstring STRINGID_ENCGENESECTFALLS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_TECHNO_BLAST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCGENESECTSTILL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encpurgeadapt ENC_TARGET_BOSS, ENC_ADAPT_ALL, 4
	encformchange ENC_TARGET_BOSS, SPECIES_GENESECT, EncScript_Genesect_Down_Ball, B_ANIM_FORM_CHANGE
EncScript_Genesect_Down_Ball:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BUG_BUZZ
	restoremovepp BS_OPPONENT1
	encsetvar 1, 0     @ Drive
	encsetvar 15, 0    @ PrevDrive
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 11, 0    @ Lock
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCGENESECTWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
