// Zacian, "The Warrior Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Zacian_Intro:
// 0 Phase (0 P0 / 1 Crowned Sword / 2 Master Swordsman / 3 Spent), 1 Technique (0-100),
// 2 TechBand (0-3, also the current Stance id), 3 MarkCount (0-3), 4 Chain (0-3+),
// 5 PerfectReady (0/1), 6 Predicted (0 none / 1 physical / 2 special / 3 status),
// 7 DefensiveStreak, 8 ArtIndex (0-3), 9 ArtCooldown, 10 ProtectHandled (0/1),
// 11 TurnGuard (0/1), 12 MoveGuard (0/1).
//
// TECHNIQUE IS THE GUARD, IN BOTH DIRECTIONS. It drives which Stance is announced (the band ranges
// on the trigger table) and Protect cuts it back down, so a band falling undoes a band rising with
// no separate teardown path.
//
// STANCE STATS HAVE A SINGLE WRITER: EncScript_Zacian_ApplyStanceStats reads TechBand and writes
// Speed/Attack/Accuracy/crit absolutely for every band, including the neutral write for the three
// bands that don't own a given stat. That's what stops switching Stances from stacking a stage left
// behind by the previous one - the Rayquaza idiom, +12 to clamp then one drop that lands on the
// stage that was meant.
//
// EVERY OnMoveEnd/OnTurnEnd TRIGGER WITHOUT Once/OnEnter IS SELF-DISABLING through a variable its
// own script writes on every path: HitResolver through MoveGuard, SwordArtResolver through
// ArtCooldown, ProtectInterrupt through ProtectHandled, TurnClose through TurnGuard. An Event.*
// condition or a plain state check stays true for the whole re-evaluation loop otherwise, and the
// dispatcher re-selects the trigger until the per-checkpoint script cap trips.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart/OnTurnEnd, so those scripts use
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER for every Stance-owned stat. Called by each Stance_* entry point after it sets
// TechBand, so the boss's stats always match the band just announced, whichever band it came from.
EncScript_Zacian_ApplyStanceStats:
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encjumpifvar CMP_NOT_EQUAL, 2, 0, EncScript_Zacian_ApplyStanceStats_SpeedOff
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -5
	goto EncScript_Zacian_ApplyStanceStats_Attack
EncScript_Zacian_ApplyStanceStats_SpeedOff:
	encchangestat ENC_TARGET_BOSS, STAT_ACC, 12
	encchangestat ENC_TARGET_BOSS, STAT_ACC, -6
EncScript_Zacian_ApplyStanceStats_Attack:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 12
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Zacian_ApplyStanceStats_AtkOff
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -5
	goto EncScript_Zacian_ApplyStanceStats_Acc
EncScript_Zacian_ApplyStanceStats_AtkOff:
	encchangestat ENC_TARGET_BOSS, STAT_ACC, 12
	encchangestat ENC_TARGET_BOSS, STAT_ACC, -6
EncScript_Zacian_ApplyStanceStats_Acc:
	encchangestat ENC_TARGET_BOSS, STAT_ACC, 12
	encjumpifvar CMP_NOT_EQUAL, 2, 2, EncScript_Zacian_ApplyStanceStats_AccOff
	encchangestat ENC_TARGET_BOSS, STAT_ACC, -5
	encsetcrit ENC_TARGET_BOSS, TRUE
	return
EncScript_Zacian_ApplyStanceStats_AccOff:
	encchangestat ENC_TARGET_BOSS, STAT_ACC, 12
	encchangestat ENC_TARGET_BOSS, STAT_ACC, -6
	encsetcrit ENC_TARGET_BOSS, FALSE
	return

// --- Trigger entry points ---

// OnBattleStart. TechBand starts at 9 - out of every real band - so the first Stance trigger after
// turn 1's move is a genuine announcement rather than a silent no-op edge.
EncScript_Zacian_Intro::
	encsetvar 2, 9    @ TechBand: nothing announced yet
	return

// The four Stances. Guideline 14: each fires only when the band actually changed (TechBand != n in
// the trigger's own condition), so the print is never a repeat.
EncScript_Zacian_Stance_Swift::
	encsetvar 2, 0
	printstring STRINGID_ENCZACIANSTANCESWIFT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Zacian_ApplyStanceStats
	return

EncScript_Zacian_Stance_Power::
	encsetvar 2, 1
	printstring STRINGID_ENCZACIANSTANCEPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Zacian_ApplyStanceStats
	return

EncScript_Zacian_Stance_Precision::
	encsetvar 2, 2
	printstring STRINGID_ENCZACIANSTANCEPRECISION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Zacian_ApplyStanceStats
	return

// Execution owns no stat of its own - ApplyStanceStats neutralises whatever the last band left
// behind. "Prioritises weakened targets" is flavour-only in a singles fight (guideline: honest gap,
// nothing to prioritise between with one target on the field).
EncScript_Zacian_Stance_Execution::
	encsetvar 2, 3
	printstring STRINGID_ENCZACIANSTANCEEXECUTION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Zacian_ApplyStanceStats
	return

// THE HIT RESOLVER. MoveGuard closes first, before anything else, so every path out - including a
// hit that changes nothing new - shuts the latch. Marks and Chain are independent checks: a hit can
// complete a Mark stack and arm/fire Perfect Strike in the same resolution.
EncScript_Zacian_HitResolver::
	encsetvar 12, 1    @ MoveGuard
	encaddvar 1, 3     @ Technique
	encjumpifvar CMP_LESS_THAN, 1, 101, EncScript_Zacian_HitResolver_Mark
	encsetvar 1, 100
EncScript_Zacian_HitResolver_Mark:
	encaddvar 3, 1     @ MarkCount
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Zacian_HitResolver_Chain
	printstring STRINGID_ENCZACIANMARKOFSWORD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	encsetvar 3, 0
EncScript_Zacian_HitResolver_Chain:
	encaddvar 4, 1     @ Chain
	encjumpifvar CMP_GREATER_THAN, 4, 2, EncScript_Zacian_HitResolver_ChainReady
	return
EncScript_Zacian_HitResolver_ChainReady:
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Zacian_HitResolver_Perfect
	encsetvar 5, 1     @ PerfectReady
	printstring STRINGID_ENCZACIANPERFECTWARNING
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Zacian_HitResolver_Perfect:
	printstring STRINGID_ENCZACIANPERFECTSTRIKE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -25, ENC_AMOUNT_PERCENT
	encsetvar 4, 0
	encsetvar 5, 0
	return

// SWORD ARTS. ArtCooldown is set on every path first, closing the latch before the branch even
// resolves. Meteor Slash deals its own payload immediately rather than telegraphing a separate
// follow-up turn - a plan simplification within the existing enc* primitives, not a deferred hit.
EncScript_Zacian_SwordArtResolver::
	encsetvar 9, 2     @ ArtCooldown
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Zacian_SwordArt_Rising
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Zacian_SwordArt_Cross
	encjumpifvar CMP_EQUAL, 8, 2, EncScript_Zacian_SwordArt_Meteor
	goto EncScript_Zacian_SwordArt_Royal
EncScript_Zacian_SwordArt_Rising:
	printstring STRINGID_ENCZACIANRISINGSLASH
	waitmessage B_WAIT_TIME_SHORT
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -4   @ +2 absolute, refreshed regardless of Stance
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	goto EncScript_Zacian_SwordArt_Advance
EncScript_Zacian_SwordArt_Cross:
	printstring STRINGID_ENCZACIANCROSSSLASH
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Zacian_SwordArt_Advance
	encaddvar 3, 1     @ MarkCount
	goto EncScript_Zacian_SwordArt_Advance
EncScript_Zacian_SwordArt_Meteor:
	printstring STRINGID_ENCZACIANMETEORSLASH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FOCUS_PUNCH_SETUP
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -14, ENC_AMOUNT_PERCENT
	goto EncScript_Zacian_SwordArt_Advance
EncScript_Zacian_SwordArt_Royal:
	printstring STRINGID_ENCZACIANROYALCUT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	encclearscreens ENC_TARGET_PLAYER_LEFT, EncScript_Zacian_SwordArt_Advance
EncScript_Zacian_SwordArt_Advance:
	encaddvar 8, 1     @ ArtIndex
	encjumpifvar CMP_NOT_EQUAL, 8, 4, EncScript_Zacian_SwordArt_Done
	encsetvar 8, 0
EncScript_Zacian_SwordArt_Done:
	return

// 50% - CROWNED SWORD. Passive Technique climb and Sword Arts both come online from here (the
// trigger table gates them on Phase >= 1); nothing further to arm in this script.
EncScript_Zacian_Phase1_CrownedSword::
	printstring STRINGID_ENCZACIANCROWNEDSWORD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 1     @ Phase
	return

// 15% - MASTER SWORDSMAN. Technique snapped to 100 locks Execution on the very next Stance
// re-evaluation pass at this same checkpoint. ArtCooldown zeroed so a Sword Art fires on every hit
// from here.
EncScript_Zacian_Phase2_MasterSwordsman::
	printstring STRINGID_ENCZACIANMASTERSWORDSMAN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 0, 2     @ Phase
	encsetvar 1, 100   @ Technique
	encsetvar 9, 0     @ ArtCooldown
	return

// THE FINISHER. Once per battle (Flags: Once) - Technique resets to 0 afterward, which is what "the
// sword's light fades" means in the design doc, not a re-arm.
EncScript_Zacian_BehemothBlade::
	printstring STRINGID_ENCZACIANBEHEMOTHBLADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	encsetvar 1, 0     @ Technique
	return

// Top of every turn. TurnGuard closes immediately so no branch can leave it re-armed. MoveGuard is
// reset here too, arming HitResolver fresh for whatever the boss does this turn. ArtCooldown ticks
// here and resolves at SwordArtResolver, a different checkpoint, so a real turn passes in between.
EncScript_Zacian_TurnOpen::
	flushtextbox
	encsetvar 11, 1    @ TurnGuard
	encsetvar 12, 0    @ MoveGuard
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Zacian_TurnOpen_Predict
	encsubvar 9, 1     @ ArtCooldown
EncScript_Zacian_TurnOpen_Predict:
	encstoreprediction ENC_TARGET_PLAYER_LEFT, 6   @ Predicted
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Zacian_TurnOpen_Streak
	encsetvar 7, 0     @ DefensiveStreak: any non-status read breaks it
	return
EncScript_Zacian_TurnOpen_Streak:
	encaddvar 7, 1     @ DefensiveStreak
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Zacian_TurnOpen_Done
	printstring STRINGID_ENCZACIANANTICIPATED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	encaddvar 1, 10    @ Technique
	encjumpifvar CMP_LESS_THAN, 1, 101, EncScript_Zacian_TurnOpen_Clamp
	encsetvar 1, 100
EncScript_Zacian_TurnOpen_Clamp:
	encsetvar 7, 0     @ self-disabling: two-in-a-row consumed
EncScript_Zacian_TurnOpen_Done:
	return

// Split-checkpoint clear for ProtectHandled - a real turn passes between the OnTurnEnd that set it
// and this OnTurnStart clearing it, per the two-checkpoint countdown rule.
EncScript_Zacian_ProtectHandledReset::
	encsetvar 10, 0
	return

// BREAKING RHYTHM. ProtectHandled closes first. Phase 2's penalty is steeper (15 vs 5), both floored
// at 0 rather than left to wrap past 0 on a u8.
EncScript_Zacian_ProtectInterrupt::
	encsetvar 10, 1    @ ProtectHandled
	encsetvar 3, 0     @ MarkCount
	encsetvar 4, 0     @ Chain
	encsetvar 5, 0     @ PerfectReady
	printstring STRINGID_ENCZACIANMARKSBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Zacian_ProtectInterrupt_Heavy
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Zacian_ProtectInterrupt_Sub5
	encsetvar 1, 0
	return
EncScript_Zacian_ProtectInterrupt_Sub5:
	encsubvar 1, 5
	return
EncScript_Zacian_ProtectInterrupt_Heavy:
	encjumpifvar CMP_GREATER_THAN, 1, 14, EncScript_Zacian_ProtectInterrupt_Sub15
	encsetvar 1, 0
	return
EncScript_Zacian_ProtectInterrupt_Sub15:
	encsubvar 1, 15
	return

// THE PASSIVE LEDGER, Phase 1+. TurnGuard clears last, closing the once-per-turn scaffold it opened
// at TurnOpen.
EncScript_Zacian_TurnClose::
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Zacian_TurnClose_Guard
	encaddvar 1, 5     @ Technique
	encjumpifvar CMP_LESS_THAN, 1, 101, EncScript_Zacian_TurnClose_Guard
	encsetvar 1, 100
EncScript_Zacian_TurnClose_Guard:
	encsetvar 11, 0    @ TurnGuard
	return

// SPENT, the catch window (guideline 10). Reduction goes UP to 99 here - the boss must survive long
// enough to be caught once Balls unlock.
EncScript_Zacian_Spent::
	encsetvar 0, 3     @ Phase
	printstring STRINGID_ENCZACIANFADE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	encresetstats ENC_TARGET_ALL_BATTLERS, EncScript_Zacian_Spent_NoStats
EncScript_Zacian_Spent_NoStats:
	encchangehp ENC_TARGET_BOSS, 8, ENC_AMOUNT_TO_PERCENT
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCZACIANWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
