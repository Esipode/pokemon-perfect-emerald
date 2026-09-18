// Palkia, "The Spatial Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Palkia_Intro:
// 0 Phase (0 The First Step / 1 Spatial Collapse / 2 Origin Forme / 3 The Collapse),
// 1 Distortion (0-5), 2 LastDist (the last ANNOUNCED level), 3 Zone (1 COMPRESSED, 2 SEPARATED,
// 3 WARPED; doubles as the wheel cursor), 4 ZoneClock, 5 Anchor (0-3), 6 LastAnchor,
// 7 TurnGuard, 8 Hit, 9 Worm (0 none, 1 telegraphed, 2 open), 10 WormCd, 11 Rend (1-3 charge
// steps), 12 AskCd, 13 Displaced, 14 EventGuard.
//
// The engine has no positional model, so the fight asks about MOVE CATEGORY instead: a physical
// move is a body crossing a gap, a special move is something sent across one. Palkia announces how
// far away it is standing, and the player's answer each turn is which kind of attack can cross
// that distance. A committed damaging move that does not match the zone falls into the rift and
// comes back out behind the player's Pokemon.
// Distortion and Zone are the ONLY inputs to the guard, and EncScript_Palkia_SetGuard is its only
// writer outside the catch window, so the two numbers can never drift out of agreement with it.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. guard = {84,86,90,92,94,96}[Distortion], plus the
// zone modifier (COMPRESSED -6, SEPARATED +2, WARPED +0). A battle script has no arithmetic on
// constants, so this is eighteen leaves - verbose, and the only place in the encounter the number
// appears at all, which is the point.
EncScript_Palkia_SetGuard:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Palkia_SetGuard_D5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Palkia_SetGuard_D4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Palkia_SetGuard_D3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Palkia_SetGuard_D2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Palkia_SetGuard_D1
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Palkia_SetGuard_D0_Near
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_SetGuard_D0_Far
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Palkia_SetGuard_D0_Near:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	return
EncScript_Palkia_SetGuard_D0_Far:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	return
EncScript_Palkia_SetGuard_D1:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Palkia_SetGuard_D1_Near
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_SetGuard_D1_Far
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Palkia_SetGuard_D1_Near:
	encsetdamagereduction ENC_TARGET_BOSS, 78
	return
EncScript_Palkia_SetGuard_D1_Far:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Palkia_SetGuard_D2:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Palkia_SetGuard_D2_Near
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_SetGuard_D2_Far
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Palkia_SetGuard_D2_Near:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	return
EncScript_Palkia_SetGuard_D2_Far:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Palkia_SetGuard_D3:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Palkia_SetGuard_D3_Near
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_SetGuard_D3_Far
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Palkia_SetGuard_D3_Near:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	return
EncScript_Palkia_SetGuard_D3_Far:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Palkia_SetGuard_D4:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Palkia_SetGuard_D4_Near
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_SetGuard_D4_Far
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Palkia_SetGuard_D4_Near:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Palkia_SetGuard_D4_Far:
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Palkia_SetGuard_D5:
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Palkia_SetGuard_D5_Near
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_SetGuard_D5_Far
	encsetdamagereduction ENC_TARGET_BOSS, 96
	return
EncScript_Palkia_SetGuard_D5_Near:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Palkia_SetGuard_D5_Far:
	encsetdamagereduction ENC_TARGET_BOSS, 98
	return

// THE DISTORTION CALLOUT. LastDist holds the last ANNOUNCED level; each leaf compares it against
// its own level and stays quiet if that is what was last said, so the line prints only on a
// difference - in BOTH directions, so a level the player pushed DOWN speaks as loudly as one
// Palkia pushed up. Caller has already changed var 1.
EncScript_Palkia_Announce:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Palkia_Announce_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Palkia_Announce_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Palkia_Announce_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Palkia_Announce_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Palkia_Announce_4
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Palkia_Announce_Done
	printstring STRINGID_ENCPALKIADIST5
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Palkia_Announce_Latch
EncScript_Palkia_Announce_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Palkia_Announce_Done
	printstring STRINGID_ENCPALKIADIST0
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Palkia_Announce_Latch
EncScript_Palkia_Announce_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Palkia_Announce_Done
	printstring STRINGID_ENCPALKIADIST1
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Palkia_Announce_Latch
EncScript_Palkia_Announce_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Palkia_Announce_Done
	printstring STRINGID_ENCPALKIADIST2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Palkia_Announce_Latch
EncScript_Palkia_Announce_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Palkia_Announce_Done
	printstring STRINGID_ENCPALKIADIST3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Palkia_Announce_Latch
EncScript_Palkia_Announce_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Palkia_Announce_Done
	printstring STRINGID_ENCPALKIADIST4
	waitmessage B_WAIT_TIME_LONG
EncScript_Palkia_Announce_Latch:
	enccopyvar 2, 1     @ LastDist = Distortion
EncScript_Palkia_Announce_Done:
	return

// THE ANCHOR CALLOUT. The same latch, on the meter the player owns. Rung 3 is the loud one - it is
// a state they can spend, so they have to know they are holding it.
EncScript_Palkia_AnnounceAnchor:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Palkia_AnnounceAnchor_0
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Palkia_AnnounceAnchor_1
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Palkia_AnnounceAnchor_2
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Palkia_AnnounceAnchor_Done
	printstring STRINGID_ENCPALKIAANCHOR3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Palkia_AnnounceAnchor_Latch
EncScript_Palkia_AnnounceAnchor_0:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Palkia_AnnounceAnchor_Done
	printstring STRINGID_ENCPALKIAANCHOR0
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Palkia_AnnounceAnchor_Latch
EncScript_Palkia_AnnounceAnchor_1:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Palkia_AnnounceAnchor_Done
	printstring STRINGID_ENCPALKIAANCHOR1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Palkia_AnnounceAnchor_Latch
EncScript_Palkia_AnnounceAnchor_2:
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Palkia_AnnounceAnchor_Done
	printstring STRINGID_ENCPALKIAANCHOR2
	waitmessage B_WAIT_TIME_SHORT
EncScript_Palkia_AnnounceAnchor_Latch:
	enccopyvar 6, 5     @ LastAnchor = Anchor
EncScript_Palkia_AnnounceAnchor_Done:
	return

// The three ways the Anchor moves. Each clamps its own end of the range, then announces.
EncScript_Palkia_RaiseAnchor:
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Palkia_RaiseAnchor_Done
	encaddvar 5, 1
	call EncScript_Palkia_AnnounceAnchor
EncScript_Palkia_RaiseAnchor_Done:
	return

EncScript_Palkia_LowerAnchor:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Palkia_LowerAnchor_Done
	encsubvar 5, 1
	call EncScript_Palkia_AnnounceAnchor
EncScript_Palkia_LowerAnchor_Done:
	return

EncScript_Palkia_ZeroAnchor:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Palkia_ZeroAnchor_Done
	encsetvar 5, 0
	call EncScript_Palkia_AnnounceAnchor
EncScript_Palkia_ZeroAnchor_Done:
	return

// Clamps Distortion (already changed by the caller) into the current phase's band, then announces
// it. Phase 0 runs 0-2, Phase 1 runs 3-4, Phase 2 runs 3-5 (the shift pins it at 5; only the
// player's own play brings it back down), Phase 3 is left wherever The Collapse put it.
EncScript_Palkia_ClampDistortion:
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Palkia_ClampDistortion_Ph1
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Palkia_ClampDistortion_Ph2
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Palkia_ClampDistortion_Announce
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Palkia_ClampDistortion_Announce
	encsetvar 1, 2
	goto EncScript_Palkia_ClampDistortion_Announce
EncScript_Palkia_ClampDistortion_Ph1:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Palkia_ClampDistortion_Ph1_Floor
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Palkia_ClampDistortion_Announce
	encsetvar 1, 4
	goto EncScript_Palkia_ClampDistortion_Announce
EncScript_Palkia_ClampDistortion_Ph1_Floor:
	encsetvar 1, 3
	goto EncScript_Palkia_ClampDistortion_Announce
EncScript_Palkia_ClampDistortion_Ph2:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Palkia_ClampDistortion_Ph1_Floor
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Palkia_ClampDistortion_Announce
	encsetvar 1, 5
EncScript_Palkia_ClampDistortion_Announce:
	call EncScript_Palkia_Announce
	return

// Distortion -1 / -2, floored before the subtraction so the byte can never wrap, then clamped and
// announced. The player's only lever on the meter.
EncScript_Palkia_LowerDistortion:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Palkia_LowerDistortion_Done
	encsubvar 1, 1
	call EncScript_Palkia_ClampDistortion
EncScript_Palkia_LowerDistortion_Done:
	return

EncScript_Palkia_LowerDistortionTwo:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Palkia_LowerDistortionTwo_Floor
	encsubvar 1, 2
	call EncScript_Palkia_ClampDistortion
	return
EncScript_Palkia_LowerDistortionTwo_Floor:
	encsetvar 1, 0
	call EncScript_Palkia_ClampDistortion
	return

// THE RIFT CHIP - the price of an attack that could not cross the distance. 7% / 9% / 11% of the
// player's max HP by phase, and double that for attacking into an open wormhole.
EncScript_Palkia_RiftChip:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_RiftChip_Ph0
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Palkia_RiftChip_Ph1
	encchangehp ENC_TARGET_ALL_FOES, -11, ENC_AMOUNT_PERCENT
	return
EncScript_Palkia_RiftChip_Ph0:
	encchangehp ENC_TARGET_ALL_FOES, -7, ENC_AMOUNT_PERCENT
	return
EncScript_Palkia_RiftChip_Ph1:
	encchangehp ENC_TARGET_ALL_FOES, -9, ENC_AMOUNT_PERCENT
	return

EncScript_Palkia_RiftChipDouble:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_RiftChipDouble_Ph0
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Palkia_RiftChipDouble_Ph1
	encchangehp ENC_TARGET_ALL_FOES, -22, ENC_AMOUNT_PERCENT
	return
EncScript_Palkia_RiftChipDouble_Ph0:
	encchangehp ENC_TARGET_ALL_FOES, -14, ENC_AMOUNT_PERCENT
	return
EncScript_Palkia_RiftChipDouble_Ph1:
	encchangehp ENC_TARGET_ALL_FOES, -18, ENC_AMOUNT_PERCENT
	return

// Entering a zone: the banner that states the rule in flavour, plus the zone's own field effects.
// COMPRESSED is Fairy Lock and trapping - there is nowhere to step back to; SEPARATED releases
// both; WARPED is a +2 evasion stance, so attacking at all is the wrong play there.
EncScript_Palkia_ZoneEnter:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_ZoneEnter_Far
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Palkia_ZoneEnter_Warped
	encsetfieldstatus ENC_FIELD_FAIRY_LOCK
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCPALKIAZONENEAR
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_FAIRY_LOCK
	return
EncScript_Palkia_ZoneEnter_Far:
	encclearfieldstatus ENC_FIELD_FAIRY_LOCK
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	printstring STRINGID_ENCPALKIAZONEFAR
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	return
EncScript_Palkia_ZoneEnter_Warped:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 2
	printstring STRINGID_ENCPALKIAZONEWARPED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	return

// The mirror of _ZoneEnter, read BEFORE the wheel advances. Clearing a field status that is not up
// and untrapping a battler that is not trapped are both documented no-ops, so only the WARPED
// evasion stage needs a paired write. A player who Hazed the +2 away first leaves Palkia at -2 for
// one zone - player-favourable drift, and not worth a var to track.
EncScript_Palkia_ZoneExit:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Palkia_ZoneExit_Warped
	encclearfieldstatus ENC_FIELD_FAIRY_LOCK
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	return
EncScript_Palkia_ZoneExit_Warped:
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -2
	return

// The standing zone reminder (guideline 14): one line, the turn before the shift, for as long as
// the wheel is turning. A player who came back to the fight after a break is never guessing.
EncScript_Palkia_ZoneRemind:
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_ZoneRemind_Far
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Palkia_ZoneRemind_Warped
	printstring STRINGID_ENCPALKIAREMINDNEAR
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Palkia_ZoneRemind_Far:
	printstring STRINGID_ENCPALKIAREMINDFAR
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Palkia_ZoneRemind_Warped:
	printstring STRINGID_ENCPALKIAREMINDWARPED
	waitmessage B_WAIT_TIME_SHORT
	return

// Advances the wheel cursor. COMPRESSED -> SEPARATED -> WARPED -> wraps, except in Phase 0, where
// WARPED is not on the wheel yet and the two zones simply alternate.
EncScript_Palkia_AdvanceZone:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Palkia_AdvanceZone_Wrap
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_AdvanceZone_ToWarped
	encsetvar 3, 2
	return
EncScript_Palkia_AdvanceZone_ToWarped:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_AdvanceZone_Wrap
	encsetvar 3, 3
	return
EncScript_Palkia_AdvanceZone_Wrap:
	encsetvar 3, 1
	return

// ZoneClock by phase: 4 / 3 / 2. The fight's tempo, and the ceiling on how long a status can hold
// on Palkia from Phase 1, since every shift sheds it.
EncScript_Palkia_SetZoneClock:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_SetZoneClock_Slow
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Palkia_SetZoneClock_Mid
	encsetvar 4, 2
	return
EncScript_Palkia_SetZoneClock_Slow:
	encsetvar 4, 4
	return
EncScript_Palkia_SetZoneClock_Mid:
	encsetvar 4, 3
	return

// THE SPACE SHIFT. Leaves the old zone, advances the wheel, raises Distortion on Palkia's own
// clock, resets the guard, sheds Palkia's own status (Phase 1+), enters the new zone, and only
// then considers displacement - so the player is told where they are standing before they are
// moved. Called from _TurnClose when ZoneClock reaches 0.
EncScript_Palkia_Shift:
	call EncScript_Palkia_ZoneExit
	call EncScript_Palkia_AdvanceZone
	encaddvar 1, 1
	call EncScript_Palkia_ClampDistortion
	call EncScript_Palkia_SetGuard
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_Shift_Enter
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Palkia_Shift_Shed
	goto EncScript_Palkia_Shift_Enter
EncScript_Palkia_Shift_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCPALKIASHED
	waitmessage B_WAIT_TIME_LONG
EncScript_Palkia_Shift_Enter:
	call EncScript_Palkia_ZoneEnter
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_Shift_Clock
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Palkia_Shift_Clock
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Palkia_Shift_Anchored
	call EncScript_Palkia_Displace
	goto EncScript_Palkia_Shift_Clock
EncScript_Palkia_Shift_Anchored:
	printstring STRINGID_ENCPALKIAANCHORHOLDS
	waitmessage B_WAIT_TIME_LONG
EncScript_Palkia_Shift_Clock:
	call EncScript_Palkia_SetZoneClock
	return

// DISPLACEMENT - the spec's Spatial Shift, kept literally. encswitchout is the one engine addition
// this encounter makes; it is only ever called from OnTurnEnd, after every action has resolved, so
// the Pokemon dragged in does not act this turn. A player down to their last Pokemon has nowhere
// to be sent, so the tear closes on them instead.
EncScript_Palkia_Displace:
	printstring STRINGID_ENCPALKIATEARSSPACE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SPACIAL_REND
	encswitchout ENC_TARGET_PLAYER_LEFT, EncScript_Palkia_Displace_Nowhere
	encaddvar 13, 1     @ Displaced
	encjumpifvar CMP_GREATER_THAN, 13, 1, EncScript_Palkia_Displace_Again
	printstring STRINGID_ENCPALKIADISPLACED
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Palkia_Displace_Cost
EncScript_Palkia_Displace_Again:
	printstring STRINGID_ENCPALKIADISPLACEDAGAIN
	waitmessage B_WAIT_TIME_LONG
EncScript_Palkia_Displace_Cost:
	call EncScript_Palkia_ZeroAnchor
	return
EncScript_Palkia_Displace_Nowhere:
	printstring STRINGID_ENCPALKIANOWHERETOSEND
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	call EncScript_Palkia_RiftChip
	return

// THE VERDICT, resolved. The attack matched the zone: it lands at that zone's own guard, which is
// the reward, and the read is paid in Anchor.
EncScript_Palkia_Landed:
	printstring STRINGID_ENCPALKIALANDED
	waitmessage B_WAIT_TIME_SHORT
	call EncScript_Palkia_RaiseAnchor
	return

// It did not: the blow goes into the torn space and comes back out behind the player's Pokemon.
// playanimation BS_PLAYER1 rather than playmoveanimation - gBattlerAttacker is unreliable at a
// checkpoint, so anything direction-sensitive needs an explicit battler. Phase 0 is a free lesson
// and costs no Anchor.
EncScript_Palkia_Rift:
	printstring STRINGID_ENCPALKIARIFTSWALLOWS
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCPALKIARIFTRETURNS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	call EncScript_Palkia_RiftChip
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_Rift_Done
	call EncScript_Palkia_LowerAnchor
EncScript_Palkia_Rift_Done:
	return

// THE SPATIAL REND CHARGE (Phase 2). One step per turn, each with its own line and animation, then
// it fires for 35% of the player's max HP and costs Palkia the following turn. Rend resets to 1
// afterwards, so Phase 2 is a repeating deadline rather than a single event - and the player who
// cannot break the charge still gets a free turn out of every cycle.
EncScript_Palkia_RendTick:
	encjumpifvar CMP_EQUAL, 11, 1, EncScript_Palkia_RendTick_1
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Palkia_RendTick_2
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Palkia_RendTick_Fire
	encsetvar 11, 1     @ the charge was collapsed; it starts again
	return
EncScript_Palkia_RendTick_1:
	printstring STRINGID_ENCPALKIAREND1
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SPACIAL_REND
	encsetvar 11, 2
	return
EncScript_Palkia_RendTick_2:
	printstring STRINGID_ENCPALKIAREND2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encsetvar 11, 3
	return
EncScript_Palkia_RendTick_Fire:
	printstring STRINGID_ENCPALKIAREND3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_SCARED
	printstring STRINGID_ENCPALKIARENDFIRE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SPACIAL_REND
	encchangehp ENC_TARGET_ALL_FOES, -35, ENC_AMOUNT_PERCENT
	encsetrecharge ENC_TARGET_BOSS, 2   @ OnTurnEnd: the timer has already ticked for this turn
	printstring STRINGID_ENCPALKIARENDSPENT
	waitmessage B_WAIT_TIME_LONG
	encsetvar 11, 1
	return

// THE YES ARM of the anchor prompt. Spending the meter collapses the rift, drops the guard two
// rungs and takes Palkia's action this turn - but leaves the player unanchored and displaceable.
EncScript_Palkia_Anchor:
	printstring STRINGID_ENCPALKIAANCHORDRIVE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_INGRAIN_HEAL
	encsetvar 11, 0     @ the charge collapses
	encsetvar 5, 0      @ Anchor spent
	call EncScript_Palkia_AnnounceAnchor
	call EncScript_Palkia_LowerDistortionTwo
	call EncScript_Palkia_SetGuard
	encsetrecharge ENC_TARGET_BOSS, 1   @ OnTurnStart: costs Palkia THIS turn's action
	printstring STRINGID_ENCPALKIARIFTCOLLAPSES
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger scripts ---

EncScript_Palkia_Intro::
	printstring STRINGID_ENCPALKIAINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCPALKIAINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 3, 1      @ Zone: the wheel starts on COMPRESSED
	call EncScript_Palkia_SetGuard
	call EncScript_Palkia_ZoneEnter
	call EncScript_Palkia_SetZoneClock
	return

// Top of every turn. Leads with flushtextbox: an OnTurnStart script that animates before printing
// anything renders on top of the still-open action menu. TurnGuard is set immediately so no branch
// out of this script can leave it re-armed. ZoneClock and Rend are NOT ticked here - a shift always
// lands at the end of a turn, so the player sees the new zone before choosing a move in it.
EncScript_Palkia_TurnOpen::
	flushtextbox
	encsetvar 7, 1      @ TurnGuard
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Palkia_TurnOpen_AskCd
	encsubvar 10, 1     @ WormCd
EncScript_Palkia_TurnOpen_AskCd:
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Palkia_TurnOpen_Remind
	encsubvar 12, 1     @ AskCd
EncScript_Palkia_TurnOpen_Remind:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_TurnOpen_Done
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Palkia_TurnOpen_Done
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Palkia_TurnOpen_Done
	call EncScript_Palkia_ZoneRemind
EncScript_Palkia_TurnOpen_Done:
	return

// A real decision, not a free button: YES stops the rend, drops the guard two rungs and takes
// Palkia's turn, but leaves the player unanchored; NO keeps the anchor and eats up to 35% of their
// max HP. The Anchor is NOT spent on a NO.
EncScript_Palkia_AnchorPrompt::
	encaskyesno STRINGID_ENCPALKIAANCHORASK, EncScript_Palkia_Anchor
	encsetvar 12, 2     @ AskCd: do not nag every turn
	printstring STRINGID_ENCPALKIAANCHORHELD
	waitmessage B_WAIT_TIME_SHORT
	return

// The wormhole, one turn after its telegraph. OnTurnStart is the only checkpoint where this works:
// gProtectStructs is cleared after end-of-turn effects, so a protect set anywhere else evaporates
// before it can matter. Resolved at _TurnClose - see it, decide, find out.
EncScript_Palkia_OpenWormhole::
	flushtextbox
	printstring STRINGID_ENCPALKIAWORMWAITS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_PROTECT
	encsetprotect ENC_TARGET_BOSS
	encsetvar 9, 2      @ Worm: open this turn
	return

// THE CORE MECHANIC. Hit is set first, so this pair can only speak once per turn however many
// moves resolve, then the zone decides which verdict the player gets. Physical crosses a crushed
// distance; special crosses a stretched one; in WARPED neither does.
EncScript_Palkia_StruckPhysical::
	encsetvar 8, 1      @ Hit
	encjumpifvar CMP_EQUAL, 3, 1, EncScript_Palkia_Landed
	goto EncScript_Palkia_Rift

EncScript_Palkia_StruckSpecial::
	encsetvar 8, 1      @ Hit
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_Landed
	goto EncScript_Palkia_Rift

// A voluntary switch costs the Anchor - except in SEPARATED, the one zone where stepping back is
// the zone's own rule rather than a retreat from it.
EncScript_Palkia_TheyFled::
	encsetvar 14, 1     @ EventGuard
	encjumpifvar CMP_EQUAL, 3, 2, EncScript_Palkia_TheyFled_Free
	call EncScript_Palkia_ZeroAnchor
EncScript_Palkia_TheyFled_Free:
	return

EncScript_Palkia_TheyFell::
	encsetvar 14, 1     @ EventGuard
	call EncScript_Palkia_ZeroAnchor
	return

// Runs last, so it sees the finished turn: the wormhole's answer, the WARPED reward, the zone
// clock, the rend charge, the next telegraph, and the per-turn state clear.
EncScript_Palkia_TurnClose::
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Palkia_TurnClose_Clear   @ The Collapse: inert
	encjumpifvar CMP_NOT_EQUAL, 9, 2, EncScript_Palkia_TurnClose_Warped
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Palkia_TurnClose_WormPunish
	printstring STRINGID_ENCPALKIAWORMEMPTY
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Palkia_RaiseAnchor
	call EncScript_Palkia_LowerDistortion
	call EncScript_Palkia_SetGuard
	goto EncScript_Palkia_TurnClose_WormDone
EncScript_Palkia_TurnClose_WormPunish:
	printstring STRINGID_ENCPALKIAWORMPUNISH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	call EncScript_Palkia_RiftChipDouble
EncScript_Palkia_TurnClose_WormDone:
	encsetvar 9, 0      @ Worm
	encsetvar 10, 4     @ WormCd
EncScript_Palkia_TurnClose_Warped:
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Palkia_TurnClose_Clock
	encjumpifvar CMP_EQUAL, 8, 1, EncScript_Palkia_TurnClose_Clock
	call EncScript_Palkia_RaiseAnchor   @ the only place a non-attacking turn pays
EncScript_Palkia_TurnClose_Clock:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Palkia_TurnClose_Shift
	encsubvar 4, 1      @ ZoneClock
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Palkia_TurnClose_Rend
EncScript_Palkia_TurnClose_Shift:
	call EncScript_Palkia_Shift
EncScript_Palkia_TurnClose_Rend:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Palkia_TurnClose_Telegraph
	call EncScript_Palkia_RendTick
EncScript_Palkia_TurnClose_Telegraph:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Palkia_TurnClose_Clear
	encjumpifvar CMP_NOT_EQUAL, 9, 0, EncScript_Palkia_TurnClose_Clear
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Palkia_TurnClose_Clear
	encjumpifvar CMP_EQUAL, 11, 2, EncScript_Palkia_TurnClose_Clear   @ not on top of a loud rend turn
	encjumpifvar CMP_EQUAL, 11, 3, EncScript_Palkia_TurnClose_Clear
	printstring STRINGID_ENCPALKIAWORMOPENS
	waitmessage B_WAIT_TIME_SHORT
	encsetvar 9, 1      @ Worm: telegraphed, opens next turn
EncScript_Palkia_TurnClose_Clear:
	encsetvar 7, 0      @ TurnGuard
	encsetvar 8, 0      @ Hit
	encsetvar 14, 0     @ EventGuard
	return

// --- Phases ---

// 50% - SPATIAL COLLAPSE. WARPED joins the wheel, the clock tightens to 3, Distortion runs 3-4,
// wrong answers start costing an Anchor rung, wormholes are armed, and displacement is armed at
// Distortion 3+.
EncScript_Palkia_SpatialCollapse::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCPALKIACOLLAPSE1
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SPACIAL_REND
	printstring STRINGID_ENCPALKIACOLLAPSE2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 3      @ Distortion, floored at 3 from here
	call EncScript_Palkia_Announce
	call EncScript_Palkia_SetGuard
	encsetvar 4, 3      @ ZoneClock: the clock tightens
	encsetvar 10, 2     @ WormCd: the first wormhole comes soon, not instantly
	return

// 25% - ORIGIN FORME. encformchange keeps HP and the moveset and takes stats, types and ability
// from the new species, so Origin Forme's own ability replaces Pressure from this moment;
// encsetmove writes only the battle mon, so a Palkia caught afterwards keeps the moveset it was
// built with. The rend begins charging immediately.
EncScript_Palkia_OriginForme::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCPALKIAORIGIN1
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_PALKIA_ORIGIN, EncScript_Palkia_OriginForme_Body, B_ANIM_FORM_CHANGE
	printstring STRINGID_ENCPALKIAORIGIN2
	waitmessage B_WAIT_TIME_LONG
EncScript_Palkia_OriginForme_Body:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_SPACIAL_REND
	encsetvar 1, 5      @ Distortion, pinned at 5 by every shift from here
	call EncScript_Palkia_Announce
	call EncScript_Palkia_SetGuard
	encsetvar 4, 2      @ ZoneClock
	encsetvar 11, 1     @ Rend: the charge starts now
	return

// 10% - THE COLLAPSE, and the catch window (guideline 10). Everything the fight was running is
// cleared, the guard falls, and balls are unlocked. The automatic catch-window damage guard owns
// the reduction from here, so the value set below is the one that guard restores, not the live one.
EncScript_Palkia_Collapse::
	encsetvar 0, 3      @ Phase
	printstring STRINGID_ENCPALKIANOWHERETOHIDE
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SPACIAL_REND
	printstring STRINGID_ENCPALKIACOLLAPSEALL
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Palkia_ZoneExit
	encclearfieldstatus ENC_FIELD_FAIRY_LOCK
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetvar 9, 0      @ Worm
	encsetvar 11, 0     @ Rend
	encsetvar 4, 0      @ ZoneClock
	encsetvar 12, 0     @ AskCd
	encsetvar 1, 0      @ Distortion
	call EncScript_Palkia_Announce
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetballs ENC_BALLS_ALLOWED
	encsetcatchrate 30
	printstring STRINGID_ENCPALKIAEXHAUSTED
	waitmessage B_WAIT_TIME_LONG
	return
