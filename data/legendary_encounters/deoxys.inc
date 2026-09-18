// Deoxys, "The Alien Organism". Var indices, pinned by the
// always-true Conditions on EncScript_Deoxys_Intro:
// 0 Phase (0 The Organism / 1 Rapid Mutation / 2 Perfect Adaptation / 3 Cellular Collapse),
// 1 Form (0 Normal / 1 Attack / 2 Defense / 3 Speed), 2 Stress (Mutation Stress, 0-10),
// 3 Cooldown (turns until the next reconstruction), 4 Rebuild (0 idle / 3 queued / 1 reconstructing
// / 2 spent), 5 Lockout (Cellular Instability countdown, 3 -> 0), 6 Barrier (Defense Forme barrier,
// 0-3), 7 LastType (last attack bucket: 0 none / 1 Bug / 2 Ghost / 3 Dark / 4 other-physical /
// 5 other-special), 8 Assault (Attack Forme meter, 0-3), 9 LastGuard (reduction last ANNOUNCED),
// 10 TurnGuard, 11 MoveGuard (per-turn OnMoveEnd re-entry guard), 12 HpMark (boss HP percentage at
// the reconstruction open), 13 Scratch, 14 LastStress (stress TIER last announced), 15 Prev
// (ApplyGuard's before-value scratch).
//
// Deoxys reconstructs on a timer, and each reconstruction READS the player and builds the forme
// that answers what it read. Every real change costs it a point of Mutation Stress; at 5 it comes
// apart into CELLULAR INSTABILITY, collapsing to Normal Forme with a cratered guard for three
// turns. So the way to hurt it is to make it keep changing, and the way to make it keep changing is
// to keep changing what you do. Defense Forme's adaptive barrier teaches the same lesson a second
// time: it hardens against whatever kind of attack last landed and collapses the moment you switch.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. The ladder runs BACKWARDS relative to the
// formes' own bulk: Attack Forme's 20/20 defences behind a flat 90 guard would evaporate to one hit
// and Defense Forme's 160/160 would be unkillable, so the guard compensates for the shape rather
// than reinforcing it. Relative damage taken - (1 - guard/100) x (50 / defStat), against Normal
// Forme as 1.00: Attack (96) 1.43, Speed (86) 1.11, Defense (88-94) 0.54 falling to 0.27 at a full
// barrier, Instability (84) 2.29, Phase 3 (80, behind -30% defences) 4.08.
// Prev holds the value last announced so each leaf can write LastGuard before comparing; there is
// no UI for damage reduction, so the line has to re-fire on every real change.
EncScript_Deoxys_ApplyGuard:
	@ A reconstruction in flight owns the guard outright (97, set by RebuildBegin). Without this, a
	@ type-bucket script firing at OnMoveEnd during the telegraph turn would recompute the ladder and
	@ quietly undo the cover. Rebuild 2 (spent) is the resolution itself, and must recompute.
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Deoxys_ApplyGuard_Done
	enccopyvar 15, 9
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_Guard76      @ Phase 3: the collapse owns its guard
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Deoxys_Guard80  @ cellular instability
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Deoxys_Guard92      @ Attack Forme
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Guard82      @ Speed Forme
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_ApplyGuard_Def
	goto EncScript_Deoxys_Guard89                               @ Normal Forme, the reference
EncScript_Deoxys_ApplyGuard_Def:
	encjumpifvar CMP_GREATER_THAN, 6, 2, EncScript_Deoxys_Guard90
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Deoxys_Guard88
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Deoxys_Guard86
	goto EncScript_Deoxys_Guard84

EncScript_Deoxys_Guard76:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encsetvar 9, 76
	encjumpifvar CMP_EQUAL, 15, 76, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 76, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 9, 80
	encjumpifvar CMP_EQUAL, 15, 80, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 80, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard82:
	encsetdamagereduction ENC_TARGET_BOSS, 82
	encsetvar 9, 82
	encjumpifvar CMP_EQUAL, 15, 82, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 82, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 9, 84
	encjumpifvar CMP_EQUAL, 15, 84, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 84, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 9, 86
	encjumpifvar CMP_EQUAL, 15, 86, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 86, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 9, 88
	encjumpifvar CMP_EQUAL, 15, 88, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 88, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard89:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	encsetvar 9, 89
	encjumpifvar CMP_EQUAL, 15, 89, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 89, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 9, 90
	encjumpifvar CMP_EQUAL, 15, 90, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 90, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall
EncScript_Deoxys_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 9, 92
	encjumpifvar CMP_EQUAL, 15, 92, EncScript_Deoxys_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 15, 92, EncScript_Deoxys_ApplyGuard_Rise
	goto EncScript_Deoxys_ApplyGuard_Fall

EncScript_Deoxys_ApplyGuard_Rise:
	printstring STRINGID_ENCDEOXYSGUARDRISE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Deoxys_ApplyGuard_Fall:
	printstring STRINGID_ENCDEOXYSGUARDFALL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_ApplyGuard_Done:
	return

// The player's only window onto Mutation Stress - the number itself is never shown. Three tiers
// (0-3 stable, 4-7 straining, 8-10 critical) rather than eleven keeps the fight from narrating
// every point, and the latch on LastStress means it prints only when the tier actually moves.
// Silent once the cells have collapsed for good.
EncScript_Deoxys_StressTier:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_StressTier_Done
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_StressTier_T2
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Deoxys_StressTier_T1
	encjumpifvar CMP_EQUAL, 14, 0, EncScript_Deoxys_StressTier_Done
	encsetvar 14, 0
	printstring STRINGID_ENCDEOXYSSTRESSSTABLE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Deoxys_StressTier_T1:
	encjumpifvar CMP_EQUAL, 14, 1, EncScript_Deoxys_StressTier_Done
	encsetvar 14, 1
	printstring STRINGID_ENCDEOXYSSTRESSSTRAIN
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Deoxys_StressTier_T2:
	encjumpifvar CMP_EQUAL, 14, 2, EncScript_Deoxys_StressTier_Done
	encsetvar 14, 2
	printstring STRINGID_ENCDEOXYSSTRESSCRITICAL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_StressTier_Done:
	return

// Clamped at 10, which is the point Instability reads. Announces through the tier latch rather than
// per point, so a gain inside a band is silent.
EncScript_Deoxys_GainStress:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_GainStress_Done
	encaddvar 2, 1
EncScript_Deoxys_GainStress_Done:
	call EncScript_Deoxys_StressTier
	return

// Stress scales the stance PAYLOADS, never the guard - the guard has to stay a pure function of
// forme plus barrier or the LastGuard latch starts announcing moves the line doesn't describe.
// Six bands of literals rather than a loop of small encchangehp calls, which would play six
// health-bar animations for one hit. Stress runs 0-10, so each band is two points wide except the
// top one, which is the ceiling Instability fires at:
//   0-1 / 2-3 / 4-5 / 6-7 / 8-9 / 10
// Tested in descending order with 0-1 as the fall-through, so the ladder needs no equality leaves.
EncScript_Deoxys_AssaultStrike:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_AssaultStrike_S5
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_AssaultStrike_S4
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Deoxys_AssaultStrike_S3
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Deoxys_AssaultStrike_S2
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_Deoxys_AssaultStrike_S1
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S1:
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S2:
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S3:
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S4:
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_AssaultStrike_S5:
	encchangehp ENC_TARGET_ALL_FOES, -16, ENC_AMOUNT_PERCENT
	return

EncScript_Deoxys_BlitzChip:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_BlitzChip_S5
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_BlitzChip_S4
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Deoxys_BlitzChip_S3
	encjumpifvar CMP_GREATER_THAN, 2, 3, EncScript_Deoxys_BlitzChip_S2
	encjumpifvar CMP_GREATER_THAN, 2, 1, EncScript_Deoxys_BlitzChip_S1
	encchangehp ENC_TARGET_ALL_FOES, -2, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S1:
	encchangehp ENC_TARGET_ALL_FOES, -3, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S2:
	encchangehp ENC_TARGET_ALL_FOES, -4, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S3:
	encchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S4:
	encchangehp ENC_TARGET_ALL_FOES, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_BlitzChip_S5:
	encchangehp ENC_TARGET_ALL_FOES, -8, ENC_AMOUNT_PERCENT
	return


// The reconstruction cadence, in turns of cooldown before the telegraph turn. Phase 0 is 2 (1 for
// Normal Forme, the observing state, which never sits still for long); Phases 1 and 2 are 1, which
// is the structural floor - RebuildBegin telegraphs at one turn open and RebuildResolve fires at
// the next, so a full cycle can never be shorter than two turns. Phase 0's Normal Forme therefore
// already runs at that floor, which is why it has no separate leaf in the late phases.
EncScript_Deoxys_ResetCooldown:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Deoxys_ResetCooldown_P0
	encsetvar 3, 1
	return
EncScript_Deoxys_ResetCooldown_P0:
	encsetvar 3, 2
	encjumpifvar CMP_NOT_EQUAL, 1, 0, EncScript_Deoxys_ResetCooldown_Done
	encsetvar 3, 1
EncScript_Deoxys_ResetCooldown_Done:
	return

// The raw defensive floor, and the reason this encounter is fightable at all. Deoxys has 50 base HP
// - about 210 at level 100, against 322 for Ho-Oh, Lugia and Mewtwo - so every hit is a far bigger
// slice of a far smaller bar, and Attack Forme's 20 base Def makes a super-effective STAB hit worth
// SEVEN TIMES its own max HP before reduction. A damage-reduction percentage cannot fix that: the
// last three points of the 0-99 scale are each worth a 25-50% swing, which is not a tunable knob.
// So the bulk is bought in raw stats instead, and the guard ladder above is left free to express
// the differences between the formes on a stable part of its range.
//
// Called EXACTLY ONCE after every encformchange: RecalcBattlerStats rebuilds the battle stats from
// the new species and wipes anything raw applied before it, so this neither accumulates across
// changes nor survives one. A path that did not actually change form must not call it.
// Defense Forme takes a smaller multiplier because its 160 base defences are already doing the job;
// the same one would make a full barrier take 40+ hits to break through.
EncScript_Deoxys_Bulk:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Bulk_Defense
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 180, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 180, ENC_AMOUNT_PERCENT
	return
EncScript_Deoxys_Bulk_Defense:
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 110, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 110, ENC_AMOUNT_PERCENT
	return

// The shared transformation body. The caller has already written the destination into Form. Strips
// the Tailwind first so it belongs to Speed Forme rather than to the battle, then changes shape,
// pays the stress, applies the forme's entry effect and resets the timer.
EncScript_Deoxys_ApplyForm:
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encsetvar 6, 0    @ Barrier
	encsetvar 8, 0    @ Assault
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Deoxys_Form_Attack
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Form_Defense
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Form_Speed
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_NORMAL, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMNORMAL
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Deoxys_Form_Gain
EncScript_Deoxys_Form_Attack:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_ATTACK, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMATTACK
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Deoxys_Form_Gain
EncScript_Deoxys_Form_Defense:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_DEFENSE, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMDEFENSE
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Deoxys_Form_Gain
EncScript_Deoxys_Form_Speed:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_SPEED, EncScript_Deoxys_Form_Gain, B_ANIM_FORM_CHANGE
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSFORMSPEED
	waitmessage B_WAIT_TIME_LONG
EncScript_Deoxys_Form_Gain:
	call EncScript_Deoxys_GainStress
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Form_EntrySpeed
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Form_EntryDefense
	encchangehp ENC_TARGET_BOSS, 5, ENC_AMOUNT_PERCENT
	goto EncScript_Deoxys_Form_Cooldown
	@ Permanent - a 0 timer never ticks - and stripped again by the next form change.
EncScript_Deoxys_Form_EntrySpeed:
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	goto EncScript_Deoxys_Form_Cooldown
	@ A straining organism arrives already braced.
EncScript_Deoxys_Form_EntryDefense:
	encjumpifvar CMP_GREATER_THAN, 2, 9, EncScript_Deoxys_Form_Braced2
	encjumpifvar CMP_GREATER_THAN, 2, 5, EncScript_Deoxys_Form_Braced1
	goto EncScript_Deoxys_Form_Cooldown
EncScript_Deoxys_Form_Braced2:
	encsetvar 6, 2
	goto EncScript_Deoxys_Form_Cooldown
EncScript_Deoxys_Form_Braced1:
	encsetvar 6, 1
	@ Every path, the form-change failure branches included, reaches this label: a script that left
	@ without setting Cooldown would be re-selected on the very next pass.
EncScript_Deoxys_Form_Cooldown:
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// The adaptive barrier, shared by all five type-bucket triggers. The caller has written the
// incoming bucket into Scratch. LastType is recorded in EVERY forme, not just Defense - so a player
// who nukes it with one move and then lets it build Defense Forme finds the barrier growing from
// the first turn. MoveGuard makes this once per turn, so a multi-hit move is one adaptation rather
// than five. The five compare leaves exist because encjumpifvar only ever tests a var against a
// literal; there is no var-to-var comparison.
EncScript_Deoxys_Adapt:
	encsetvar 11, 1   @ MoveGuard
	encjumpifvar CMP_NOT_EQUAL, 1, 2, EncScript_Deoxys_Adapt_Record
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Deoxys_Adapt_Break
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Deoxys_Adapt_C1
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Deoxys_Adapt_C2
	encjumpifvar CMP_EQUAL, 13, 3, EncScript_Deoxys_Adapt_C3
	encjumpifvar CMP_EQUAL, 13, 4, EncScript_Deoxys_Adapt_C4
	encjumpifvar CMP_EQUAL, 7, 5, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C1:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C2:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C3:
	encjumpifvar CMP_EQUAL, 7, 3, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_C4:
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Deoxys_Adapt_Harden
	goto EncScript_Deoxys_Adapt_Break
EncScript_Deoxys_Adapt_Harden:
	encjumpifvar CMP_GREATER_THAN, 6, 2, EncScript_Deoxys_Adapt_Record
	encaddvar 6, 1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCDEOXYSHARDEN
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Deoxys_Adapt_Record
EncScript_Deoxys_Adapt_Break:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Deoxys_Adapt_Record
	encsetvar 6, 0
	printstring STRINGID_ENCDEOXYSBARRIERBREAK
	waitmessage B_WAIT_TIME_SHORT
	@ The strings never name the type - "that attack" is both better writing and honest about the
	@ five-bucket approximation.
EncScript_Deoxys_Adapt_Record:
	enccopyvar 7, 13
	call EncScript_Deoxys_ApplyGuard
	return

// --- Trigger scripts ---

// Opens in Normal Forme with a four-turn timer. LastGuard is seeded to the Properties reduction so
// the first real move in it reads as a change. The second line is the whole hint the fight rests
// on: it is rebuilding to answer YOU, and every rebuild costs it something.
EncScript_Deoxys_Intro::
	encsetvar 3, 2     @ Cooldown
	encsetvar 9, 93    @ LastGuard
	encsetvar 12, 100  @ HpMark
	@ The opening Normal Forme has never been through a form change, so it needs the floor applied
	@ here; every later forme gets it from its own encformchange.
	call EncScript_Deoxys_Bulk
	printstring STRINGID_ENCDEOXYSINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSADAPTS
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Owns the status shrug, the
// instability countdown, the reconstruction timer and the two forme stances.
EncScript_Deoxys_TurnOpen::
	flushtextbox
	encsetvar 10, 1   @ TurnGuard
	encsetvar 11, 0   @ MoveGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_TurnOpen_Done   @ collapsed: inert
	@ Sleep and freeze are refused rather than wasted - and each refusal costs a point of stress, so
	@ status is the fight's second lever. Capped at Stress <= 7 on purpose: sleeping it every turn
	@ would otherwise walk the meter to 10 on its own and hand over a free damage window. The last
	@ two points can only come from forcing mutations.
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Deoxys_TurnOpen_Shrug
	jumpifstatus BS_OPPONENT1, STATUS1_FREEZE, EncScript_Deoxys_TurnOpen_Shrug
	goto EncScript_Deoxys_TurnOpen_Lockout
EncScript_Deoxys_TurnOpen_Shrug:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	printstring STRINGID_ENCDEOXYSSHRUG
	waitmessage B_WAIT_TIME_SHORT
	encjumpifvar CMP_GREATER_THAN, 2, 7, EncScript_Deoxys_TurnOpen_Lockout
	call EncScript_Deoxys_GainStress
EncScript_Deoxys_TurnOpen_Lockout:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Deoxys_TurnOpen_Cooldown
	encsubvar 5, 1
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Deoxys_TurnOpen_StillUnstable
	printstring STRINGID_ENCDEOXYSSTABILIZED
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	goto EncScript_Deoxys_TurnOpen_Stance
EncScript_Deoxys_TurnOpen_StillUnstable:
	printstring STRINGID_ENCDEOXYSUNSTABLE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Deoxys_TurnOpen_Tier
	@ Skipped while a reconstruction is in flight, so the telegraph turn does not also tick toward
	@ the next cycle. encsubvar wraps at 0, so the floor is an explicit skip rather than a clamp.
EncScript_Deoxys_TurnOpen_Cooldown:
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Deoxys_TurnOpen_Stance
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Deoxys_TurnOpen_Stance
	encsubvar 3, 1
EncScript_Deoxys_TurnOpen_Stance:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_TurnOpen_Blitz
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Deoxys_TurnOpen_Studies
	goto EncScript_Deoxys_TurnOpen_Tier
	@ The Blitz. OnTurnStart is after turn order is locked but before any action resolves, so this
	@ genuinely lands before the player's move - the spec's "acts multiple times" delivered honestly
	@ rather than faked. Scripted damage, so Protect and screens do not stop it.
EncScript_Deoxys_TurnOpen_Blitz:
	printstring STRINGID_ENCDEOXYSBLITZ
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	call EncScript_Deoxys_BlitzChip
	@ A real Protect, so Feint, never-miss moves and contact punishment all resolve exactly as they
	@ should. gProtectStructs is wiped after end-of-turn effects, so it expires on its own.
	encjumpifchance 25, EncScript_Deoxys_TurnOpen_Evade
	goto EncScript_Deoxys_TurnOpen_Tier
EncScript_Deoxys_TurnOpen_Evade:
	encsetprotect ENC_TARGET_BOSS
	printstring STRINGID_ENCDEOXYSEVADE
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Deoxys_TurnOpen_Tier
EncScript_Deoxys_TurnOpen_Studies:
	printstring STRINGID_ENCDEOXYSSTUDIES
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_TurnOpen_Tier:
	call EncScript_Deoxys_StressTier
EncScript_Deoxys_TurnOpen_Done:
	return

// OnTurnEnd, priority 90 - runs after every phase transition, hybrid and type-bucket script at the
// same checkpoint. Owns the Rebuild state machine and the Attack Forme meter.
EncScript_Deoxys_TurnClose::
	encsetvar 10, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Deoxys_TurnClose_Done   @ collapsed: inert
	@ Rebuild 3 -> 1 -> 2 -> 0. RebuildBegin writes 3 rather than 1 precisely so RebuildResolve,
	@ gated on 1 at the same checkpoint, cannot fire in the same dispatch: priority orders the first
	@ pass only, it is not a guard. Stepping it here means a real turn always passes between the
	@ telegraph and the transformation.
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Deoxys_TurnClose_Queue
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Deoxys_TurnClose_Spent
	goto EncScript_Deoxys_TurnClose_Assault
EncScript_Deoxys_TurnClose_Queue:
	encsetvar 4, 1
	goto EncScript_Deoxys_TurnClose_Assault
EncScript_Deoxys_TurnClose_Spent:
	encsetvar 4, 0
	@ Assault. The reason not to simply park in front of Attack Forme and race it: it is the forme
	@ the player most wants it in, and the one that punishes them hardest for staying there.
	@ playanimation BS_PLAYER1 rather than playmoveanimation - gBattlerAttacker is stale at
	@ OnTurnEnd, so anything direction-sensitive needs an explicit battler.
EncScript_Deoxys_TurnClose_Assault:
	encjumpifvar CMP_NOT_EQUAL, 1, 1, EncScript_Deoxys_TurnClose_Done
	encaddvar 8, 1
	encjumpifvar CMP_LESS_THAN, 8, 3, EncScript_Deoxys_TurnClose_Charging
	encsetvar 8, 0
	printstring STRINGID_ENCDEOXYSASSAULTFIRE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	call EncScript_Deoxys_AssaultStrike
	goto EncScript_Deoxys_TurnClose_Done
EncScript_Deoxys_TurnClose_Charging:
	encjumpifvar CMP_LESS_THAN, 8, 2, EncScript_Deoxys_TurnClose_Done
	printstring STRINGID_ENCDEOXYSASSAULTCHARGE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Deoxys_TurnClose_Done:
	return

// The reconstruction turn. The player is NOT locked out here - they act normally, and what they do
// is the input the selection ladder reads next turn. That makes the telegraph and the decision the
// same turn, which is what turns baiting into a skill rather than a guess.
// encsetprotect only has an effect at OnTurnStart, which is exactly where this is; the 97 guard is
// belt-and-braces against a Feint or a never-miss move, and is lifted again at resolution.
EncScript_Deoxys_RebuildBegin::
	flushtextbox
	printstring STRINGID_ENCDEOXYSRECONSTRUCT
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSRECONSTRUCTING
	waitmessage B_WAIT_TIME_LONG
	encsetprotect ENC_TARGET_BOSS
	encsetdamagereduction ENC_TARGET_BOSS, 96
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_SET
	encsetvar 4, 3   @ Rebuild: queued; TurnClose steps it to 1
	return

// The selection ladder, walked in order. Every rung is something the player controls:
//   1. your active outspeeds it            -> SPEED FORME  (and then nothing outspeeds 180 base, so
//                                             the rule goes false next cycle and it leaves again)
//   2. it lost 10%+ of max HP that turn    -> DEFENSE FORME (save your biggest hit for the telegraph)
//   3. it is below 50% HP                  -> ATTACK FORME  (the glass cannon that takes 1.8x)
//   4. otherwise                           -> NORMAL FORME  (and Normal recycles fastest)
// Rule 3 rides Var(Phase) rather than a live HP test: Phase 1's threshold IS the 50% mark, so the
// spec's rule and the phase boundary are the same line and one variable serves both.
EncScript_Deoxys_RebuildResolve::
	flushtextbox
	enccomparestat ENC_TARGET_PLAYER_LEFT, ENC_TARGET_BOSS, STAT_SPEED, 13
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Deoxys_Resolve_Speed
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE, EncScript_Deoxys_Resolve_Rule3
	encjumpifvar CMP_GREATER_THAN, 12, 9, EncScript_Deoxys_Resolve_Defense
EncScript_Deoxys_Resolve_Rule3:
	encjumpifvar CMP_GREATER_THAN, 0, 0, EncScript_Deoxys_Resolve_Attack
	goto EncScript_Deoxys_Resolve_Normal
EncScript_Deoxys_Resolve_Speed:
	encsetvar 13, 3
	goto EncScript_Deoxys_Resolve_Apply
EncScript_Deoxys_Resolve_Defense:
	encsetvar 13, 2
	goto EncScript_Deoxys_Resolve_Apply
EncScript_Deoxys_Resolve_Attack:
	encsetvar 13, 1
	goto EncScript_Deoxys_Resolve_Apply
EncScript_Deoxys_Resolve_Normal:
	encsetvar 13, 0
	@ Rebuild 2 (spent) is written before anything else can go wrong, so this trigger's own gate is
	@ already false when the re-evaluation pass comes round.
EncScript_Deoxys_Resolve_Apply:
	encsetvar 4, 2
	encjumpifvar CMP_EQUAL, 13, 0, EncScript_Deoxys_Resolve_TryNormal
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Deoxys_Resolve_TryAttack
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Deoxys_Resolve_TryDefense
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Deoxys_Resolve_NoChange
	goto EncScript_Deoxys_Resolve_Change
EncScript_Deoxys_Resolve_TryNormal:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Deoxys_Resolve_NoChange
	goto EncScript_Deoxys_Resolve_Change
EncScript_Deoxys_Resolve_TryAttack:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Deoxys_Resolve_NoChange
	goto EncScript_Deoxys_Resolve_Change
EncScript_Deoxys_Resolve_TryDefense:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Deoxys_Resolve_NoChange
EncScript_Deoxys_Resolve_Change:
	enccopyvar 1, 13
	call EncScript_Deoxys_ApplyForm
	return
	@ A read that comes back the same costs nothing, and the line that says so is the whole tutorial
	@ for the mechanic: play the same way twice and it tells you, in as many words, that the cycle
	@ was wasted. ApplyGuard here is what lifts the reconstruction's 97 cover.
EncScript_Deoxys_Resolve_NoChange:
	printstring STRINGID_ENCDEOXYSNOCHANGE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// The one spend, and it is not voluntary. Stress resets rather than latching, so instability
// REPEATS - it is a rhythm the player learns to drive, not a one-time phase. Self-disabling: the
// gate is Stress >= 10 and the first thing this does is zero it.
EncScript_Deoxys_Instability::
	flushtextbox
	encsetvar 2, 0    @ Stress
	encsetvar 3, 0    @ Cooldown
	encsetvar 4, 0    @ Rebuild: pre-empts a reconstruction due the same turn
	encsetvar 5, 3    @ Lockout: three turns unable to rebuild
	encsetvar 6, 0    @ Barrier
	encsetvar 8, 0    @ Assault
	encsetvar 14, 0   @ LastStress: back to the stable tier without re-announcing it
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	printstring STRINGID_ENCDEOXYSINSTABILITY
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Deoxys_Instability_Guard
	encsetvar 1, 0    @ Form: collapses to Normal
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_NORMAL, EncScript_Deoxys_Instability_Guard, B_ANIM_FORM_CHANGE_INSTANT
	@ Only on the branch that actually changed shape - the already-Normal jump and the failure label
	@ both skip it, because neither rebuilt the stats this would be restoring.
	call EncScript_Deoxys_Bulk
EncScript_Deoxys_Instability_Guard:
	call EncScript_Deoxys_ApplyGuard
	return

// Phase 1 at 50%: the cycle tightens from four turns to two, and selection rule 3 comes online.
// An in-flight reconstruction keeps its own timing - only a countdown longer than the new cadence
// is pulled in, so the transition can never lengthen the wait.
EncScript_Deoxys_RapidMutation::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCDEOXYSRAPID
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Deoxys_RapidMutation_Done
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Deoxys_RapidMutation_Done
	call EncScript_Deoxys_ResetCooldown
EncScript_Deoxys_RapidMutation_Done:
	return

// Phase 2 at 25%. The cadence is already 2, so this only opens the two Hybrid Mutation beats.
EncScript_Deoxys_PerfectAdaptation::
	encsetvar 0, 2   @ Phase 2
	printstring STRINGID_ENCDEOXYSPERFECT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Hybrid I - ATTACK + SPEED. Attack Forme's offences behind Speed Forme's turn order.
// The raw Speed is applied AFTER the form change: a form change rebuilds the battle stats from the
// new species and wipes anything raw applied before it. That is the intended lifetime here - the
// hybrid's edge survives until the next ordinary reconstruction, which is exactly how long a
// temporary hybrid should last.
EncScript_Deoxys_HybridOne::
	printstring STRINGID_ENCDEOXYSHYBRID
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSHYBRIDONE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1   @ Form: Attack
	encsetvar 4, 0   @ Rebuild: the hybrid IS the mutation, so it cancels any reconstruction in flight
	encsetvar 6, 0   @ Barrier
	encsetvar 8, 0   @ Assault
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_ATTACK, EncScript_Deoxys_HybridOne_Velocity, B_ANIM_ULTRA_BURST
	encchangehp ENC_TARGET_BOSS, 5, ENC_AMOUNT_PERCENT
EncScript_Deoxys_HybridOne_Velocity:
	call EncScript_Deoxys_Bulk
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 80, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	call EncScript_Deoxys_GainStress
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// Hybrid II - DEFENSE + ATTACK. Defense Forme's bulk with offences that actually threaten, and it
// arrives with two thirds of a barrier already up.
EncScript_Deoxys_HybridTwo::
	printstring STRINGID_ENCDEOXYSHYBRID
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCDEOXYSHYBRIDTWO
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 2   @ Form: Defense
	encsetvar 4, 0   @ Rebuild: cancels any reconstruction in flight
	encsetvar 8, 0   @ Assault
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_DEFENSE, EncScript_Deoxys_HybridTwo_Armor, B_ANIM_ULTRA_BURST
	encchangehp ENC_TARGET_BOSS, 5, ENC_AMOUNT_PERCENT
EncScript_Deoxys_HybridTwo_Armor:
	call EncScript_Deoxys_Bulk
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 70, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 70, ENC_AMOUNT_PERCENT
	encsetvar 6, 2   @ Barrier
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	call EncScript_Deoxys_GainStress
	call EncScript_Deoxys_ResetCooldown
	call EncScript_Deoxys_ApplyGuard
	return

// The Final Twist. B_ANIM_FORM_CHANGE_INSTANT rather than the full fanfare: four of the long
// version would take most of a minute, and the instant variant reads as RAPID, which is the point.
// Each change's failure label is the next change's position, so a failure cascades toward Normal
// rather than parking it mid-cycle.
// Ending on Normal Forme is load-bearing: encformchange writes the party Pokemon's species, so the
// forme Deoxys is in when it is caught is the forme the player keeps.
EncScript_Deoxys_Collapse::
	printstring STRINGID_ENCDEOXYSDESTABILIZE
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_ATTACK, EncScript_Deoxys_Collapse_Two, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Two:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_DEFENSE, EncScript_Deoxys_Collapse_Three, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Three:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_SPEED, EncScript_Deoxys_Collapse_Four, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Four:
	encformchange ENC_TARGET_BOSS, SPECIES_DEOXYS_NORMAL, EncScript_Deoxys_Collapse_Done, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Deoxys_Collapse_Done:
	printstring STRINGID_ENCDEOXYSNOADAPT
	waitmessage B_WAIT_TIME_LONG
	@ Phase 3 gates every stance, countdown and trigger that could still fire, so the catch window is
	@ quiet. CapTypeEffectiveness and FlatToxicDamage stay on as insurance; Survive comes off, after
	@ which the automatic catch-window damage guard keeps the catch target alive.
	encsetvar 0, 3    @ Phase 3
	encsetvar 1, 0    @ Form: Normal
	encsetvar 2, 0    @ Stress
	encsetvar 3, 0    @ Cooldown
	encsetvar 4, 0    @ Rebuild
	encsetvar 5, 0    @ Lockout
	encsetvar 6, 0    @ Barrier
	encsetvar 8, 0    @ Assault
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	@ The four-forme cycle above ended on a freshly rebuilt Normal Forme, so the floor goes back on
	@ before the collapse's own cut is taken out of it.
	call EncScript_Deoxys_Bulk
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -10, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, -10, ENC_AMOUNT_PERCENT
	@ Must match ApplyGuard's Phase 3 leaf exactly - the live reduction and the LastGuard latch are
	@ the same state, and a mismatch makes the next callout announce a move that never happened.
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 9, 80
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCDEOXYSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return

// The five type buckets. Bug, Ghost and Dark are Deoxys' own weaknesses and so what the player will
// actually reach for; the physical/special split on the remainder keeps neutral coverage moves
// meaningfully different from each other. Five triggers is far cheaper than the eighteen a per-type
// bucket would need.
EncScript_Deoxys_HitBug::
	encsetvar 13, 1
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitGhost::
	encsetvar 13, 2
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitDark::
	encsetvar 13, 3
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitOtherPhysical::
	encsetvar 13, 4
	call EncScript_Deoxys_Adapt
	return

EncScript_Deoxys_HitOtherSpecial::
	encsetvar 13, 5
	call EncScript_Deoxys_Adapt
	return
