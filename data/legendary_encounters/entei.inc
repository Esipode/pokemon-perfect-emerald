// Entei, "The Walking Volcano". Var indices, pinned by
// the always-true Conditions on EncScript_Entei_Intro:
// 0 Phase (0 dormant / 1 eruption cycle / 2 cataclysm / 3 weakened), 1 Pressure (0-5), 2 Fuse
// (0 unlit, else 3->2->1 resolved at 1), 3 Scorch (0-3), 4 Window (turns of open guard left),
// 5 LastGuard (reduction last ANNOUNCED), 6 VentCool, 7 TurnGuard, 8 PMove, 9 BMoveGuard,
// 10 SwitchGuard, 11 FaintGuard.
//
// Pressure never falls on its own - it only rises, and is only spent by erupting. The player does
// not choose whether the volcano goes off, only when, how big, and what they are standing on. The
// guard is a locked door rather than a ladder: flat for the whole phase (90/92/94), and only an
// eruption opens it. Scorch is a switch tax and the eruption's fuel gauge, cooled by Water moves
// and Rain, and locked at 3 once the Cataclysm begins.

// --- Shared subroutines (call/return) ---

// Sole owner of the flat phase guard AND its callout. Returns without touching anything while a
// window owns the reduction, or in Weakened. Otherwise it writes the phase's base value and compares
// it against the last ANNOUNCED value, printing on every change - there is no UI for damage
// reduction, so a single showing would be trivially missed. Coming back from a window (any value
// below 80) reads as the moment closing; anything else is the mountain hardening.
EncScript_Entei_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_ApplyGuard_Done          @ weakened owns its own guard
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Entei_ApplyGuard_Done   @ a window owns the guard
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Entei_ApplyGuard_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Entei_ApplyGuard_P1
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 5, 90, EncScript_Entei_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 5, 80, EncScript_Entei_ApplyGuard_P0_Shut
	encsetvar 5, 90
	goto EncScript_Entei_ApplyGuard_Harden
EncScript_Entei_ApplyGuard_P0_Shut:
	encsetvar 5, 90
	goto EncScript_Entei_ApplyGuard_Shut
EncScript_Entei_ApplyGuard_P1:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encjumpifvar CMP_EQUAL, 5, 92, EncScript_Entei_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 5, 80, EncScript_Entei_ApplyGuard_P1_Shut
	encsetvar 5, 92
	goto EncScript_Entei_ApplyGuard_Harden
EncScript_Entei_ApplyGuard_P1_Shut:
	encsetvar 5, 92
	goto EncScript_Entei_ApplyGuard_Shut
EncScript_Entei_ApplyGuard_P2:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	encjumpifvar CMP_EQUAL, 5, 94, EncScript_Entei_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 5, 80, EncScript_Entei_ApplyGuard_P2_Shut
	encsetvar 5, 94
	goto EncScript_Entei_ApplyGuard_Harden
EncScript_Entei_ApplyGuard_P2_Shut:
	encsetvar 5, 94
EncScript_Entei_ApplyGuard_Shut:
	printstring STRINGID_ENCENTEIRECOVERS
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Entei_ApplyGuard_Harden:
	printstring STRINGID_ENCENTEIHARDENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_ApplyGuard_Done:
	return

// The only place Pressure ever rises. A lit fuse locks it out entirely, so the countdown can be
// neither accelerated nor stalled once it starts.
EncScript_Entei_GainPressure:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_GainPressure_Done       @ weakened: the mountain is out
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Entei_GainPressure_Done   @ the fuse is already burning
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Entei_GainPressure_Done
	encaddvar 1, 1
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Entei_GainPressure_Arm
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Entei_GainPressure_Pours
	printstring STRINGID_ENCENTEIGROUNDWARMS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Entei_GainPressure_Pours:
	printstring STRINGID_ENCENTEIHEATPOURS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Entei_GainPressure_Arm:
	call EncScript_Entei_ArmFuse
EncScript_Entei_GainPressure_Done:
	return

// Two full turns of warning, cut to one once the Cataclysm begins. TurnClose steps it 3->2->1,
// TurnStart resolves it at 1.
EncScript_Entei_ArmFuse:
	encsetvar 2, 3
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Entei_ArmFuse_Print
	encsetvar 2, 2
EncScript_Entei_ArmFuse_Print:
	printstring STRINGID_ENCENTEITREMBLES
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MAGNITUDE
	waitanimation
	return

EncScript_Entei_Scorch1:
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Entei_Scorch1_Done
	encaddvar 3, 1
	printstring STRINGID_ENCENTEISCORCHED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_Scorch1_Done:
	return

// The player's half of the field battle. Dead once the Cataclysm locks the ground at 3.
EncScript_Entei_Cool1:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Entei_Cool1_Done
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Entei_Cool1_Done   @ Cataclysm: the field is locked
	encsubvar 3, 1
	printstring STRINGID_ENCENTEICOOLS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_WATER_GUN
	waitanimation
EncScript_Entei_Cool1_Done:
	return

// The eruption body, shared by the normal and the below-15% forms. Percentage HP loss that ignores
// typing entirely, so nothing walls the volcano - only Entei's own moveset. Every eruption slams the
// field back to full Scorch, scours the stat changes off both sides, and throws Entei awake, then
// leaves it wide open for the only real damage window in the fight.
EncScript_Entei_EruptCore:
	encsetvar 2, 0   @ the fuse is spent
	printstring STRINGID_ENCENTEIERUPTS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ERUPTION
	waitanimation
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Entei_EruptCore_S3
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Entei_EruptCore_S2
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Entei_EruptCore_S1
	encchangehp ENC_TARGET_ALL_FOES, -20, ENC_AMOUNT_PERCENT
	goto EncScript_Entei_EruptCore_After
EncScript_Entei_EruptCore_S1:
	encchangehp ENC_TARGET_ALL_FOES, -25, ENC_AMOUNT_PERCENT
	goto EncScript_Entei_EruptCore_After
EncScript_Entei_EruptCore_S2:
	encchangehp ENC_TARGET_ALL_FOES, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Entei_EruptCore_After
EncScript_Entei_EruptCore_S3:
	encchangehp ENC_TARGET_ALL_FOES, -35, ENC_AMOUNT_PERCENT
EncScript_Entei_EruptCore_After:
	normalisebuffs
	encsetvar 3, 3
	printstring STRINGID_ENCENTEIFIELDSCORCHED
	waitmessage B_WAIT_TIME_LONG
	curestatus BS_OPPONENT1        @ thrown awake by its own blast
	updatestatusicon BS_OPPONENT1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Entei_EruptCore_Cata
	encsetvar 4, 2   @ Window: two full turns of open guard
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 5, 65
	encsetvar 1, 0   @ Pressure floor, Dormant
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Entei_EruptCore_Spent
	encsetvar 1, 2   @ Pressure floor, eruption cycle
	goto EncScript_Entei_EruptCore_Spent
EncScript_Entei_EruptCore_Cata:
	encsetvar 1, 3   @ Pressure floor, Cataclysm
	encsetvar 4, 3   @ three full turns
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 5, 55
EncScript_Entei_EruptCore_Spent:
	printstring STRINGID_ENCENTEISPENT
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger scripts ---

// LastGuard is seeded to the Properties reduction so the first change reads as a change.
EncScript_Entei_Intro::
	encsetvar 5, 90
	printstring STRINGID_ENCENTEIINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCENTEIGROUNDHOT
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn guards,
// lets Rain cool the ground, converts any status Entei is carrying into Pressure, and keeps the
// open-window callout running for as long as the window lasts.
EncScript_Entei_TurnOpen::
	flushtextbox
	encsetvar 7, 1    @ TurnGuard
	encsetvar 8, 0    @ PMove
	encsetvar 9, 0    @ BMoveGuard
	encsetvar 10, 0   @ SwitchGuard
	encsetvar 11, 0   @ FaintGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_TurnOpen_Done   @ weakened: inert
	jumpifhalfword CMP_NO_COMMON_BITS, gBattleWeather, B_WEATHER_RAIN, EncScript_Entei_TurnOpen_Status
	call EncScript_Entei_Cool1
EncScript_Entei_TurnOpen_Status:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Entei_TurnOpen_Statused
	goto EncScript_Entei_TurnOpen_Window
EncScript_Entei_TurnOpen_Statused:
	printstring STRINGID_ENCENTEINOWHERETOGO
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Entei_GainPressure
EncScript_Entei_TurnOpen_Window:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Entei_TurnOpen_Done
	printstring STRINGID_ENCENTEISTILLOPEN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_TurnOpen_Done:
	return

EncScript_Entei_Erupt::
	flushtextbox
	call EncScript_Entei_EruptCore
	encchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	printstring STRINGID_ENCENTEIRECOIL
	waitmessage B_WAIT_TIME_SHORT
	return

// Below 15% the blast costs Entei nothing: encchangehp bypasses the damage-reduction path, so the
// recoil would otherwise faint it before the catch window ever opens.
EncScript_Entei_EruptSafe::
	flushtextbox
	call EncScript_Entei_EruptCore
	return

// Pressure Break. Holding a stat boost while Entei is loaded buys a small, early, weak eruption and
// a one-turn window - the mirror of letting Pressure run to 5 for a devastating one and the biggest
// window in the fight.
EncScript_Entei_Vent::
	flushtextbox
	encsubvar 1, 2
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Entei_Vent_Cool
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Entei_Vent_Cool
	encsetvar 1, 2   @ the eruption cycle's Pressure floor
EncScript_Entei_Vent_Cool:
	encsetvar 6, 3   @ VentCool: it cannot be farmed
	printstring STRINGID_ENCENTEIVENTS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_LAVA_PLUME
	waitanimation
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	normalisebuffs
	printstring STRINGID_ENCENTEISCOURED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Entei_Scorch1
	encsetvar 4, 1   @ Window: this turn only
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 5, 78
	printstring STRINGID_ENCENTEIOFFBALANCE
	waitmessage B_WAIT_TIME_LONG
	return

// Water cools the ground - and still feeds Pressure. Being the cooling lever does not exempt it from
// "Pressure cannot be prevented"; the player is choosing which of the two things they need more.
EncScript_Entei_PlayerWater::
	encsetvar 8, 2   @ PMove
	call EncScript_Entei_Cool1
	call EncScript_Entei_GainPressure
	return

EncScript_Entei_PlayerAttack::
	encsetvar 8, 2   @ PMove
	call EncScript_Entei_GainPressure
	return

// Entei stoking its own fire. OnMoveEnd fires whether or not the move connected - the heat goes into
// the ground either way.
EncScript_Entei_BossFire::
	encsetvar 9, 1   @ BMoveGuard
	call EncScript_Entei_GainPressure
	return

EncScript_Entei_Faint::
	encsetvar 11, 1   @ FaintGuard
	printstring STRINGID_ENCENTEIUNMOVED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Entei_GainPressure
	return

// The transition tax. Scorch never chips on a timer; it charges for pivoting, which is what makes
// letting it climb a real decision rather than a drip of damage.
EncScript_Entei_SwitchIn::
	encsetvar 10, 1   @ SwitchGuard
	printstring STRINGID_ENCENTEISEARS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_WILL_O_WISP
	waitanimation
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Entei_SwitchIn_S3
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Entei_SwitchIn_S2
	encchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	return
EncScript_Entei_SwitchIn_S2:
	encchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return
EncScript_Entei_SwitchIn_S3:
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown. The fuse is stepped here and resolved at the next TurnStart, so a real turn always
// passes between a tick and the eruption. Pressure gains come after the step, which is what keeps a
// fuse armed this turn from being stepped in the same breath.
EncScript_Entei_TurnClose::
	encsetvar 7, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Entei_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Entei_TurnClose_Fuse3
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Entei_TurnClose_Fuse2
	goto EncScript_Entei_TurnClose_Heat
EncScript_Entei_TurnClose_Fuse3:
	encsetvar 2, 2
	printstring STRINGID_ENCENTEIMAGMA
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Entei_TurnClose_Heat
EncScript_Entei_TurnClose_Fuse2:
	encsetvar 2, 1
	printstring STRINGID_ENCENTEISPLITS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MAGNITUDE
	waitanimation
EncScript_Entei_TurnClose_Heat:
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Entei_TurnClose_Climb
	printstring STRINGID_ENCENTEIDRINKSHEAT
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Entei_GainPressure
EncScript_Entei_TurnClose_Climb:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Entei_TurnClose_Cool
	call EncScript_Entei_GainPressure   @ from the eruption cycle on, Pressure climbs unconditionally
EncScript_Entei_TurnClose_Cool:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Entei_TurnClose_Window
	encsubvar 6, 1
EncScript_Entei_TurnClose_Window:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Entei_TurnClose_Glow
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Entei_TurnClose_Done   @ window still open
	call EncScript_Entei_ApplyGuard   @ closed: announces the guard coming back up
	goto EncScript_Entei_TurnClose_Done
EncScript_Entei_TurnClose_Glow:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Entei_TurnClose_Done
	printstring STRINGID_ENCENTEIGROUNDGLOWS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_TurnClose_Done:
	return

// Phase 1 at 50% HP. Pressure can never fall below 2 again and now climbs every turn on its own, so
// the eruptions come whatever the player does. The guard hardens a notch to pay for the fact that
// the windows are about to arrive more often.
EncScript_Entei_Cycle::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCENTEICYCLE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_ERUPTION
	waitanimation
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Entei_Cycle_Guard
	encsetvar 1, 2   @ Pressure floor
	printstring STRINGID_ENCENTEIHEATPOURS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Entei_Cycle_Guard:
	call EncScript_Entei_ApplyGuard
	return

// Phase 2 at 25% HP. The field locks at full Scorch and cooling stops working, the fuse shortens to
// one turn of warning, and every eruption becomes a Cataclysm. A fuse lit under the old rules is
// abandoned - Pressure is left one short of full so it re-arms on the shorter timer instead of
// stalling at 5 with nothing to spend it on.
EncScript_Entei_Cataclysm::
	encsetvar 0, 2   @ Phase 2
	encsetvar 2, 0   @ Fuse
	encsetvar 3, 3   @ Scorch locked
	printstring STRINGID_ENCENTEICATACLYSM
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_EARTHQUAKE
	waitanimation
	printstring STRINGID_ENCENTEIFIELDSCORCHED
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Entei_Cataclysm_Rearm
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Entei_Cataclysm_Guard
	encsetvar 1, 3   @ Pressure floor
	goto EncScript_Entei_Cataclysm_Guard
EncScript_Entei_Cataclysm_Rearm:
	encsetvar 1, 4
EncScript_Entei_Cataclysm_Guard:
	call EncScript_Entei_ApplyGuard
	return

// Below 10% in phase 2. Everything off - a lit fuse or a scorched field landing after Entei has
// visibly guttered out would be incoherent, and a team-wide tick during the catch phase is pure
// frustration. CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the catch window.
EncScript_Entei_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Pressure
	encsetvar 2, 0   @ Fuse
	encsetvar 3, 0   @ Scorch
	encsetvar 4, 0   @ Window
	encsetvar 6, 0   @ VentCool
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 5, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCENTEIWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
