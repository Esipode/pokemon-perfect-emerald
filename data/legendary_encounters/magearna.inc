// Magearna, "The Artificial Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Magearna_Intro:
// 0 Phase (0 Soul-Heart / 1 Awakening / 2 Emergency / 3 Overdrive), 1 ActiveSystem (0 Defense /
// 1 Offense / 2 Repair / 3 Energy), 2 ActiveSystem2 (same domain, 255 = not live yet),
// 3 IntegrityA (0-3, 9 = shut off until next rotation), 4 IntegrityB (same, secondary slot),
// 5 Charge (0-100, flavour only), 6 GearTimer (turns to next Gear Shift / Sacrifice, repurposed
// per phase, frozen once Overdrive begins), 7 PulseTimer (0-2 counting down, 255 = idle),
// 8 ProgressCount (systems sacrificed in Emergency, reused as hits landed in Overdrive - phases
// never overlap), 9 MoveHandled (once-per-hit latch), 10 LastSystemBand (last announced primary
// system, 255 = nothing yet), 11 LastSystem2Band (same, secondary), 12 LastChargeBand (last
// announced Charge milestone, 255 = nothing yet), 13 TurnGuard.
//
// THE DAMAGE REDUCTION AND OFFENSE STANCE EACH HAVE ONE WRITER, EncScript_Magearna_ApplyPassives,
// which reads BOTH slots and writes both values absolutely every turn - so a system rotating out
// undoes what it set with no separate teardown path, and Haze can't strand the Offense stance.
//
// STAT STAGES ARE WRITTEN ABSOLUTELY: push to +12 (clamps at the real +6 ceiling), then a single
// delta lands on the stage meant. Survives a Haze, a scour, or a switch landing in between.
//
// EVERY OnMoveEnd TRIGGER IS SELF-DISABLING through MoveHandled, cleared at the top of the next
// turn by TurnOpen. OnTurnStart/OnTurnEnd bookkeeping with no natural edge to key off routes
// through the two ledgers (TurnOpen / TurnClose) instead of standalone triggers, because a
// standalone trigger with no self-disabling condition re-selects until the runaway-script guard
// trips - Var(ActiveSystem) doesn't change turn to turn just because a passive fired.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of both the damage reduction and the Offense stat stance. Called every turn
// from TurnOpen and again whenever a system overloads mid-turn, so it never depends on order
// against whichever slot last touched these values.
EncScript_Magearna_ApplyPassives:
	encjumpifvar CMP_NOT_EQUAL, 1, 0, EncScript_Magearna_ApplyPassives_DefCheckB
	encjumpifvar CMP_EQUAL, 3, 9, EncScript_Magearna_ApplyPassives_DefCheckB
	goto EncScript_Magearna_ApplyPassives_DefOn
EncScript_Magearna_ApplyPassives_DefCheckB:
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Magearna_ApplyPassives_DefOff
	encjumpifvar CMP_EQUAL, 4, 9, EncScript_Magearna_ApplyPassives_DefOff
EncScript_Magearna_ApplyPassives_DefOn:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	goto EncScript_Magearna_ApplyPassives_Offense
EncScript_Magearna_ApplyPassives_DefOff:
	encsetdamagereduction ENC_TARGET_BOSS, 86
EncScript_Magearna_ApplyPassives_Offense:
	encjumpifvar CMP_NOT_EQUAL, 1, 1, EncScript_Magearna_ApplyPassives_OffCheckB
	encjumpifvar CMP_EQUAL, 3, 9, EncScript_Magearna_ApplyPassives_OffCheckB
	goto EncScript_Magearna_ApplyPassives_OffOn
EncScript_Magearna_ApplyPassives_OffCheckB:
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Magearna_ApplyPassives_OffOff
	encjumpifvar CMP_EQUAL, 4, 9, EncScript_Magearna_ApplyPassives_OffOff
EncScript_Magearna_ApplyPassives_OffOn:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -5    @ lands at +1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -5  @ lands at +1
	return
EncScript_Magearna_ApplyPassives_OffOff:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -6    @ lands at neutral
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -6  @ lands at neutral
	return

// THE BAND CALLOUT for the primary slot (guideline 14): prints only when the live system actually
// changed from what was last announced.
EncScript_Magearna_AnnouncePrimary:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Magearna_AnnouncePrimary_Defense
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Magearna_AnnouncePrimary_Offense
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Magearna_AnnouncePrimary_Repair
	goto EncScript_Magearna_AnnouncePrimary_Energy
EncScript_Magearna_AnnouncePrimary_Defense:
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Magearna_AnnouncePrimary_Done
	encsetvar 10, 0
	printstring STRINGID_ENCMAGEARNADEFENSEON
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnouncePrimary_Offense:
	encjumpifvar CMP_EQUAL, 10, 1, EncScript_Magearna_AnnouncePrimary_Done
	encsetvar 10, 1
	printstring STRINGID_ENCMAGEARNAOFFENSEON
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnouncePrimary_Repair:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Magearna_AnnouncePrimary_Done
	encsetvar 10, 2
	printstring STRINGID_ENCMAGEARNAREPAIRON
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnouncePrimary_Energy:
	encjumpifvar CMP_EQUAL, 10, 3, EncScript_Magearna_AnnouncePrimary_Done
	encsetvar 10, 3
	printstring STRINGID_ENCMAGEARNAENERGYON
	waitmessage B_WAIT_TIME_LONG
EncScript_Magearna_AnnouncePrimary_Done:
	return

// Same idiom, secondary slot (Awakening onward; TurnOpen only calls this once Phase >= 1).
EncScript_Magearna_AnnounceSecondary:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Magearna_AnnounceSecondary_Defense
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Magearna_AnnounceSecondary_Offense
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Magearna_AnnounceSecondary_Repair
	goto EncScript_Magearna_AnnounceSecondary_Energy
EncScript_Magearna_AnnounceSecondary_Defense:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Magearna_AnnounceSecondary_Done
	encsetvar 11, 0
	printstring STRINGID_ENCMAGEARNADEFENSEON
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceSecondary_Offense:
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Magearna_AnnounceSecondary_Done
	encsetvar 11, 1
	printstring STRINGID_ENCMAGEARNAOFFENSEON
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceSecondary_Repair:
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Magearna_AnnounceSecondary_Done
	encsetvar 11, 2
	printstring STRINGID_ENCMAGEARNAREPAIRON
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceSecondary_Energy:
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Magearna_AnnounceSecondary_Done
	encsetvar 11, 3
	printstring STRINGID_ENCMAGEARNAENERGYON
	waitmessage B_WAIT_TIME_LONG
EncScript_Magearna_AnnounceSecondary_Done:
	return

// Prints which system just overloaded. Two copies (primary reads ActiveSystem, secondary reads
// ActiveSystem2) since there's no var-to-var indirection to share one.
EncScript_Magearna_AnnounceOverloadPrimary:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Magearna_AnnounceOverloadPrimary_Defense
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Magearna_AnnounceOverloadPrimary_Offense
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Magearna_AnnounceOverloadPrimary_Repair
	printstring STRINGID_ENCMAGEARNAENERGYOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceOverloadPrimary_Defense:
	printstring STRINGID_ENCMAGEARNADEFENSEOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceOverloadPrimary_Offense:
	printstring STRINGID_ENCMAGEARNAOFFENSEOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceOverloadPrimary_Repair:
	printstring STRINGID_ENCMAGEARNAREPAIROVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Magearna_AnnounceOverloadSecondary:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Magearna_AnnounceOverloadSecondary_Defense
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Magearna_AnnounceOverloadSecondary_Offense
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Magearna_AnnounceOverloadSecondary_Repair
	printstring STRINGID_ENCMAGEARNAENERGYOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceOverloadSecondary_Defense:
	printstring STRINGID_ENCMAGEARNADEFENSEOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceOverloadSecondary_Offense:
	printstring STRINGID_ENCMAGEARNAOFFENSEOVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_AnnounceOverloadSecondary_Repair:
	printstring STRINGID_ENCMAGEARNAREPAIROVERLOAD
	waitmessage B_WAIT_TIME_LONG
	return

// Soul-Heart Charge milestone callout - flavour only; HP thresholds own the real phase gates.
// Band-gated the same way, through LastChargeBand.
EncScript_Magearna_ChargeBand:
	encjumpifvar CMP_LESS_THAN, 5, 25, EncScript_Magearna_ChargeBand_Done
	encjumpifvar CMP_LESS_THAN, 5, 50, EncScript_Magearna_ChargeBand_Band1
	encjumpifvar CMP_LESS_THAN, 5, 75, EncScript_Magearna_ChargeBand_Band2
	encjumpifvar CMP_LESS_THAN, 5, 100, EncScript_Magearna_ChargeBand_Band3
	encjumpifvar CMP_EQUAL, 12, 4, EncScript_Magearna_ChargeBand_Done
	encsetvar 12, 4
	printstring STRINGID_ENCMAGEARNAOVERDRIVE100
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_ChargeBand_Band3:
	encjumpifvar CMP_EQUAL, 12, 3, EncScript_Magearna_ChargeBand_Done
	encsetvar 12, 3
	printstring STRINGID_ENCMAGEARNAEMERGENCY75
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_ChargeBand_Band2:
	encjumpifvar CMP_EQUAL, 12, 2, EncScript_Magearna_ChargeBand_Done
	encsetvar 12, 2
	printstring STRINGID_ENCMAGEARNASHIFT50
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Magearna_ChargeBand_Band1:
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Magearna_ChargeBand_Done
	encsetvar 12, 1
	printstring STRINGID_ENCMAGEARNAUPGRADE25
	waitmessage B_WAIT_TIME_LONG
EncScript_Magearna_ChargeBand_Done:
	return

// Gear Shift: rotates the primary system +1 (Defense->Offense->Repair->Energy->repeat) and, from
// Awakening on, keeps the secondary pinned opposite it (+2, so Defense pairs with Repair and
// Offense with Energy). Resets both Integrity trackers and re-arms the 4-turn countdown.
EncScript_Magearna_GearShift:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Magearna_GearShift_PrimaryWrap
	encaddvar 1, 1
	goto EncScript_Magearna_GearShift_Secondary
EncScript_Magearna_GearShift_PrimaryWrap:
	encsetvar 1, 0
EncScript_Magearna_GearShift_Secondary:
	encsetvar 3, 0    @ IntegrityA
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Magearna_GearShift_HasSecondary
	goto EncScript_Magearna_GearShift_ResetTimer
EncScript_Magearna_GearShift_HasSecondary:
	encsetvar 4, 0    @ IntegrityB
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Magearna_GearShift_Sec2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Magearna_GearShift_Sec3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Magearna_GearShift_Sec0
	encsetvar 2, 1    @ primary == 3 (Energy) -> secondary = 1 (Offense)
	goto EncScript_Magearna_GearShift_ResetTimer
EncScript_Magearna_GearShift_Sec2:
	encsetvar 2, 2
	goto EncScript_Magearna_GearShift_ResetTimer
EncScript_Magearna_GearShift_Sec3:
	encsetvar 2, 3
	goto EncScript_Magearna_GearShift_ResetTimer
EncScript_Magearna_GearShift_Sec0:
	encsetvar 2, 0
EncScript_Magearna_GearShift_ResetTimer:
	encsetvar 6, 4
	call EncScript_Magearna_ApplyPassives
	return

// Emergency Protocol: sacrifices the next system in fixed order (Defense, Offense, Repair,
// Energy) via ProgressCount, permanently trading it for a one-time surge. The first two grants
// are a stat stage each (guideline 7's +/-2 cap split across both); the last two are pure Charge.
EncScript_Magearna_Sacrifice:
	encsetvar 6, 3    @ next sacrifice in 3 turns
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Magearna_Sacrifice_Defense
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Magearna_Sacrifice_Offense
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Magearna_Sacrifice_Repair
	encjumpifvar CMP_EQUAL, 8, 3, EncScript_Magearna_Sacrifice_Energy
	return    @ all four already given up; nothing left to sacrifice
EncScript_Magearna_Sacrifice_Defense:
	printstring STRINGID_ENCMAGEARNASACRIFICE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -5    @ lands at +1
	encaddvar 8, 1
	return
EncScript_Magearna_Sacrifice_Offense:
	printstring STRINGID_ENCMAGEARNASACRIFICE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -5  @ lands at +1
	encaddvar 8, 1
	return
EncScript_Magearna_Sacrifice_Repair:
	printstring STRINGID_ENCMAGEARNASACRIFICE
	waitmessage B_WAIT_TIME_LONG
	encaddvar 5, 15
	encaddvar 8, 1
	return
EncScript_Magearna_Sacrifice_Energy:
	printstring STRINGID_ENCMAGEARNASACRIFICE
	waitmessage B_WAIT_TIME_LONG
	encaddvar 5, 15
	encaddvar 8, 1
	return

// Heart Pulse resolves keyed on whichever system is currently primary - it began under Energy,
// but the 2-turn countdown can outlast a Gear Shift.
EncScript_Magearna_PulseResolve:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Magearna_PulseResolve_Defense
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Magearna_PulseResolve_Offense
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Magearna_PulseResolve_Repair
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	return
EncScript_Magearna_PulseResolve_Defense:
	encchangehp ENC_TARGET_BOSS, 4, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return
EncScript_Magearna_PulseResolve_Offense:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, -1
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	return
EncScript_Magearna_PulseResolve_Repair:
	encresetstats ENC_TARGET_ALL_FOES, EncScript_Magearna_PulseResolve_Done
EncScript_Magearna_PulseResolve_Done:
	return

// --- Trigger entry points ---

EncScript_Magearna_Intro::
	printstring STRINGID_ENCMAGEARNAINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCMAGEARNAINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 2, 255    @ ActiveSystem2: not live yet
	encsetvar 6, 4      @ GearTimer
	encsetvar 7, 255    @ PulseTimer: idle
	encsetvar 10, 255   @ LastSystemBand: nothing announced
	encsetvar 11, 255   @ LastSystem2Band
	encsetvar 12, 255   @ LastChargeBand
	call EncScript_Magearna_ApplyPassives
	call EncScript_Magearna_AnnouncePrimary
	return

// Top of every turn. TurnGuard is set immediately so no branch out can leave it re-armed.
// GearTimer is frozen at 99 once Overdrive begins (Phase 3), so the telegraph/decrement block
// skips itself there rather than drifting forever.
EncScript_Magearna_TurnOpen::
	encsetvar 13, 1     @ TurnGuard
	encsetvar 9, 0      @ MoveHandled
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Magearna_TurnOpen_Pulse
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Magearna_TurnOpen_Pulse
	encjumpifvar CMP_NOT_EQUAL, 6, 1, EncScript_Magearna_TurnOpen_Decrement
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Magearna_TurnOpen_TelegraphSacrifice
	printstring STRINGID_ENCMAGEARNAGEARWARN
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Magearna_TurnOpen_Decrement
EncScript_Magearna_TurnOpen_TelegraphSacrifice:
	printstring STRINGID_ENCMAGEARNASACRIFICEWARN
	waitmessage B_WAIT_TIME_SHORT
EncScript_Magearna_TurnOpen_Decrement:
	encsubvar 6, 1
EncScript_Magearna_TurnOpen_Pulse:
	encjumpifvar CMP_EQUAL, 7, 255, EncScript_Magearna_TurnOpen_Systems
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Magearna_TurnOpen_Systems
	encsubvar 7, 1
EncScript_Magearna_TurnOpen_Systems:
	call EncScript_Magearna_ApplyPassives
	call EncScript_Magearna_AnnouncePrimary
	encjumpifvar CMP_LESS_THAN, 0, 1, EncScript_Magearna_TurnOpen_Done
	call EncScript_Magearna_AnnounceSecondary
EncScript_Magearna_TurnOpen_Done:
	return

// Defense/Offense Integrity from a landed hit or the boss's own attack.
EncScript_Magearna_IntegrityHitPrimary::
	encsetvar 9, 1      @ MoveHandled
	encaddvar 3, 1      @ IntegrityA
	return

EncScript_Magearna_IntegrityHitSecondary::
	encsetvar 9, 1
	encaddvar 4, 1      @ IntegrityB
	return

EncScript_Magearna_PulseBegin::
	encsetvar 7, 2      @ PulseTimer
	printstring STRINGID_ENCMAGEARNAPULSECHARGE
	waitmessage B_WAIT_TIME_LONG
	return

// The ledger. Integrity passive gain, overload, Charge callout, Gear Shift/Sacrifice resolution,
// Heart Pulse resolve/interrupt - all of it, so nothing here needs its own self-disabling latch
// beyond the TurnGuard this trigger already reads.
EncScript_Magearna_TurnClose::
	encjumpifvar CMP_EQUAL, 3, 9, EncScript_Magearna_TurnClose_PassiveB
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Magearna_TurnClose_PrimaryRepair
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Magearna_TurnClose_PrimaryEnergy
	goto EncScript_Magearna_TurnClose_PassiveB
EncScript_Magearna_TurnClose_PrimaryRepair:
	encaddvar 3, 1
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	goto EncScript_Magearna_TurnClose_PassiveB
EncScript_Magearna_TurnClose_PrimaryEnergy:
	encaddvar 3, 1
	encaddvar 5, 4
EncScript_Magearna_TurnClose_PassiveB:
	encjumpifvar CMP_LESS_THAN, 0, 1, EncScript_Magearna_TurnClose_Overload
	encjumpifvar CMP_EQUAL, 4, 9, EncScript_Magearna_TurnClose_Overload
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Magearna_TurnClose_SecondaryRepair
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Magearna_TurnClose_SecondaryEnergy
	goto EncScript_Magearna_TurnClose_Overload
EncScript_Magearna_TurnClose_SecondaryRepair:
	encaddvar 4, 1
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	goto EncScript_Magearna_TurnClose_Overload
EncScript_Magearna_TurnClose_SecondaryEnergy:
	encaddvar 4, 1
	encaddvar 5, 4
EncScript_Magearna_TurnClose_Overload:
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Magearna_TurnClose_OverloadB
	encsetvar 3, 9
	encaddvar 5, 5
	call EncScript_Magearna_AnnounceOverloadPrimary
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Magearna_ApplyPassives
EncScript_Magearna_TurnClose_OverloadB:
	encjumpifvar CMP_NOT_EQUAL, 4, 3, EncScript_Magearna_TurnClose_Charge
	encsetvar 4, 9
	encaddvar 5, 5
	call EncScript_Magearna_AnnounceOverloadSecondary
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Magearna_ApplyPassives
EncScript_Magearna_TurnClose_Charge:
	call EncScript_Magearna_ChargeBand
	encjumpifvar CMP_GREATER_THAN, 6, 0, EncScript_Magearna_TurnClose_Pulse
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Magearna_TurnClose_Sacrifice
	encjumpifvar CMP_GREATER_THAN, 0, 1, EncScript_Magearna_TurnClose_Pulse
	call EncScript_Magearna_GearShift
	goto EncScript_Magearna_TurnClose_Pulse
EncScript_Magearna_TurnClose_Sacrifice:
	call EncScript_Magearna_Sacrifice
EncScript_Magearna_TurnClose_Pulse:
	encjumpifvar CMP_EQUAL, 7, 255, EncScript_Magearna_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 3, 9, EncScript_Magearna_TurnClose_PulseTick
	encsetvar 7, 255
	printstring STRINGID_ENCMAGEARNAPULSEDISRUPT
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Magearna_TurnClose_Done
EncScript_Magearna_TurnClose_PulseTick:
	encjumpifvar CMP_NOT_EQUAL, 7, 0, EncScript_Magearna_TurnClose_Done
	encsetvar 7, 255
	call EncScript_Magearna_PulseResolve
EncScript_Magearna_TurnClose_Done:
	encsetvar 13, 0     @ TurnGuard
	return

// --- Phase transitions ---

// 50% HP - SOUL-HEART AWAKENING. Establishes the secondary system opposite the primary (+2) and
// leaves Gear Shift to rotate both together from here on.
EncScript_Magearna_Phase2Awaken::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCMAGEARNAAWAKENBEGIN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 4, 0      @ IntegrityB
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Magearna_Phase2Awaken_Sec2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Magearna_Phase2Awaken_Sec3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Magearna_Phase2Awaken_Sec0
	encsetvar 2, 1
	goto EncScript_Magearna_Phase2Awaken_Done
EncScript_Magearna_Phase2Awaken_Sec2:
	encsetvar 2, 2
	goto EncScript_Magearna_Phase2Awaken_Done
EncScript_Magearna_Phase2Awaken_Sec3:
	encsetvar 2, 3
	goto EncScript_Magearna_Phase2Awaken_Done
EncScript_Magearna_Phase2Awaken_Sec0:
	encsetvar 2, 0
EncScript_Magearna_Phase2Awaken_Done:
	call EncScript_Magearna_ApplyPassives
	call EncScript_Magearna_AnnounceSecondary
	return

// 25% HP - EMERGENCY PROTOCOL. Balls stay blocked; the sacrifice cadence starts at 3 turns.
EncScript_Magearna_Phase3aEmergency::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCMAGEARNAEMERGENCYBEGIN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encmegaevolve ENC_TARGET_BOSS, EncScript_Magearna_Phase3aEmergency_MegaFail
EncScript_Magearna_Phase3aEmergency_MegaFail:
	encsetvar 6, 3      @ GearTimer repurposed as the sacrifice countdown
	encsetvar 8, 0      @ ProgressCount: sacrifices so far
	return

// 10% HP - SOUL-HEART OVERDRIVE, the catch window (guideline 10). The Mega form gives out - it's
// spent, not empowered - so this reverts it (encformchange back to base) instead of evolving it.
// Forces Charge to 100 for the banner's sake, freezes GearTimer, and opens Balls immediately - the
// automatic catch-window damage guard takes it from here. ProgressCount is reused as hits landed.
EncScript_Magearna_Phase3bOverdrive::
	encsetvar 0, 3      @ Phase
	encsetvar 5, 100    @ Charge
	encsetvar 8, 0      @ ProgressCount: hits landed this phase
	encsetvar 6, 99     @ GearTimer frozen
	printstring STRINGID_ENCMAGEARNAMEGAREVERT
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_MAGEARNA, EncScript_Magearna_Phase3bOverdrive_RevertFail, B_ANIM_FORM_CHANGE
EncScript_Magearna_Phase3bOverdrive_RevertFail:
	printstring STRINGID_ENCMAGEARNAOVERDRIVEBEGIN
	waitmessage B_WAIT_TIME_LONG
	encsetballs ENC_BALLS_ALLOWED
	return

// Every landed hit in Overdrive collapses one more system. At four, Fleur Cannon fires.
EncScript_Magearna_OverdriveHit::
	encsetvar 9, 1      @ MoveHandled
	encaddvar 8, 1      @ ProgressCount
	encjumpifvar CMP_EQUAL, 8, 4, EncScript_Magearna_FleurCannonFinish
	printstring STRINGID_ENCMAGEARNACOLLAPSE
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	return

// Fleur Cannon fires as the finisher, then the exact mirror of the Properties: block so nothing
// the fight imposed survives into the ordinary knockout/catch that follows (guideline 10).
EncScript_Magearna_FleurCannonFinish::
	printstring STRINGID_ENCMAGEARNAFLEURCANNON
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Magearna_FleurCannonFinish_NoStats
EncScript_Magearna_FleurCannonFinish_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	printstring STRINGID_ENCMAGEARNAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
