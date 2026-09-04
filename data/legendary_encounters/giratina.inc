// Giratina, "The Renegade Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Giratina_Intro:
// 0 Phase (0 Reality Intact / 1 The Distortion World / 2 Reality Break / 3 The Return),
// 1 Distortion (0-5), 2 LastDist (the last ANNOUNCED level), 3 Stability (0-3), 4 RotClock,
// 5 TurnGuard, 6 Inv1 (0 empty, 1-5 = which inversion), 7 Inv2, 8 Inv3, 9 AskCd, 10 Window,
// 11 GateCd, 12 BreakClock, 13 Wheel (the rotation wheel cursor, cycles 1-5), 14 EventGuard.
//
// The guard is deliberately boring - it moves on the two phase transitions and nowhere else.
// Distortion instead controls how many of five field-wide inversions are live and how fast they
// rotate: SPEED (Trick Room) -> GRAVITY -> DEFENCES (Wonder Room) -> HEALING (Heal Block) ->
// POSSESSIONS (Magic Room), a fixed wheel so an experienced player can plan two rotations ahead.
// One clock does everything: it clears the active set, sheds Giratina's own status, advances
// Distortion, and refills the wheel. Stability is the counter-meter - climb it to 3 and ANCHOR
// REALITY cancels every rule for a guard window, at the cost of resetting the meter and the clock.

// --- Shared subroutines (call/return) ---

// Clears every inversion unconditionally. Idempotent - clearing one that was not up is a no-op in
// the underlying commands, so no bookkeeping is needed to know which were actually active.
EncScript_Giratina_ClearInversions:
	encclearfieldstatus ENC_FIELD_TRICK_ROOM
	encclearfieldstatus ENC_FIELD_GRAVITY
	encclearfieldstatus ENC_FIELD_WONDER_ROOM
	encclearfieldstatus ENC_FIELD_MAGIC_ROOM
	encsethealblock ENC_TARGET_ALL_FOES, 0
	return

// THE CALLOUT. LastDist holds the last ANNOUNCED Distortion level; this prints only on a
// difference, in BOTH directions, so an ANCHOR that drops the level speaks as loudly as a rotation
// that raises it. Caller has already changed var 1 (Distortion).
EncScript_Giratina_Announce:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Giratina_Announce_Done
	enccopyvar 2, 1
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Giratina_Announce_5
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Giratina_Announce_4
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Giratina_Announce_3
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Giratina_Announce_2
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Giratina_Announce_1
	printstring STRINGID_ENCGIRATINADIST0
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Announce_1:
	printstring STRINGID_ENCGIRATINADIST1
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Announce_2:
	printstring STRINGID_ENCGIRATINADIST2
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Announce_3:
	printstring STRINGID_ENCGIRATINADIST3
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Announce_4:
	printstring STRINGID_ENCGIRATINADIST4
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Announce_5:
	printstring STRINGID_ENCGIRATINADIST5
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Announce_Done:
	return

// Clamps Distortion (already changed by the caller) to the current phase's ceiling, then
// announces it. Phase 0 caps at 2, Phase 1 caps at 5, Phase 2 is pinned at 5.
EncScript_Giratina_ClampDistortion:
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Giratina_ClampDistortion_Ph1
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Giratina_ClampDistortion_Announce
	encsetvar 1, 2
	goto EncScript_Giratina_ClampDistortion_Announce
EncScript_Giratina_ClampDistortion_Ph1:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Giratina_ClampDistortion_Ph2
	encjumpifvar CMP_LESS_THAN, 1, 6, EncScript_Giratina_ClampDistortion_Announce
	encsetvar 1, 5
	goto EncScript_Giratina_ClampDistortion_Announce
EncScript_Giratina_ClampDistortion_Ph2:
	encsetvar 1, 5
EncScript_Giratina_ClampDistortion_Announce:
	call EncScript_Giratina_Announce
	return

// The player's own lever on the meter: a switch (from Phase 1) or a faint (any phase) raises
// Distortion by one, same as a rotation would.
EncScript_Giratina_RaiseDistortion:
	encaddvar 1, 1
	call EncScript_Giratina_ClampDistortion
	return

// Advances the wheel cursor, wrapping 5 back to 1.
EncScript_Giratina_AdvanceWheel:
	encjumpifvar CMP_EQUAL, 13, 5, EncScript_Giratina_AdvanceWheel_Wrap
	encaddvar 13, 1
	return
EncScript_Giratina_AdvanceWheel_Wrap:
	encsetvar 13, 1
	return

// Sets RotClock from the current Distortion level: 4/4/3/3/2/2. The fight's tempo, and the ceiling
// on how long a status lockout can last before EncScript_Giratina_Rotate sheds it.
EncScript_Giratina_SetRotClock:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Giratina_SetRotClock_Fast
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Giratina_SetRotClock_Mid
	encsetvar 4, 4
	return
EncScript_Giratina_SetRotClock_Mid:
	encsetvar 4, 3
	return
EncScript_Giratina_SetRotClock_Fast:
	encsetvar 4, 2
	return

// One subroutine per wheel value (1-5), each raising its inversion AND printing its own banner.
// Reads var 13 directly - the caller has already copied it into the Inv slot being filled, and has
// not advanced the cursor yet, so the value read here is exactly the one just recorded.
EncScript_Giratina_Raise:
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Giratina_Raise_Speed
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Giratina_Raise_Gravity
	encjumpifvar CMP_EQUAL, 13, 3, EncScript_Giratina_Raise_Defence
	encjumpifvar CMP_EQUAL, 13, 4, EncScript_Giratina_Raise_Healing
EncScript_Giratina_Raise_Possessions:   // pick 5
	encsetfieldstatus ENC_FIELD_MAGIC_ROOM
	printstring STRINGID_ENCGIRATINAINVITEMS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MAGIC_ROOM
	return
EncScript_Giratina_Raise_Speed:
	encsetfieldstatus ENC_FIELD_TRICK_ROOM
	printstring STRINGID_ENCGIRATINAINVSPEED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	return
EncScript_Giratina_Raise_Gravity:
	encsetfieldstatus ENC_FIELD_GRAVITY
	printstring STRINGID_ENCGIRATINAINVGRAVITY
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_GRAVITY
	return
EncScript_Giratina_Raise_Defence:
	encsetfieldstatus ENC_FIELD_WONDER_ROOM
	printstring STRINGID_ENCGIRATINAINVDEFENCE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WONDER_ROOM
	return
EncScript_Giratina_Raise_Healing:
	encsethealblock ENC_TARGET_ALL_FOES, B_HEAL_BLOCK_TIMER
	printstring STRINGID_ENCGIRATINAINVHEALING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_HEAL_BLOCK
	return

// While HEALING is anywhere in the active set, its Heal Block timer is re-armed every turn so it
// never lapses on its own - HandleEndTurnHealBlock ticks it down before this runs, so the tick
// always loses the race. This is also what makes a switch buy exactly one turn of real healing: a
// fresh Pokemon has no volatile until this reapplies it at the end of that turn.
EncScript_Giratina_RearmHealBlock:
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Giratina_RearmHealBlock_Do
	encjumpifvar CMP_EQUAL, 7, 4, EncScript_Giratina_RearmHealBlock_Do
	encjumpifvar CMP_EQUAL, 8, 4, EncScript_Giratina_RearmHealBlock_Do
	return
EncScript_Giratina_RearmHealBlock_Do:
	encsethealblock ENC_TARGET_ALL_FOES, B_HEAL_BLOCK_TIMER
	return

// THE SPINE. Clears the active set, sheds Giratina's own status, drops Stability, raises and
// clamps Distortion, then fills 0/1/1/2/2/3 wheel slots by the new level. Distortion rises BEFORE
// the slots are filled, so the level the player is told about is the level the rules they are
// about to be shown came from. Called from TurnClose when RotClock hits 0, from TheBreak, and from
// both phase transitions.
EncScript_Giratina_Rotate:
	printstring STRINGID_ENCGIRATINAROTATE
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Giratina_ClearInversions
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Giratina_Rotate_Shed
	goto EncScript_Giratina_Rotate_Stability
EncScript_Giratina_Rotate_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCGIRATINASHED
	waitmessage B_WAIT_TIME_LONG
EncScript_Giratina_Rotate_Stability:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Giratina_Rotate_Distortion
	encsubvar 3, 1
EncScript_Giratina_Rotate_Distortion:
	encaddvar 1, 1
	call EncScript_Giratina_ClampDistortion
	encjumpifchance 40, EncScript_Giratina_Rotate_Skip   // not every wheel cycle is the stable one
	goto EncScript_Giratina_Rotate_Fill
EncScript_Giratina_Rotate_Skip:
	call EncScript_Giratina_AdvanceWheel
EncScript_Giratina_Rotate_Fill:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Giratina_Rotate_End
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Giratina_Rotate_Fill1
	encjumpifvar CMP_LESS_THAN, 1, 5, EncScript_Giratina_Rotate_Fill2
	goto EncScript_Giratina_Rotate_Fill3
EncScript_Giratina_Rotate_Fill1:
	enccopyvar 6, 13
	call EncScript_Giratina_Raise
	call EncScript_Giratina_AdvanceWheel
	goto EncScript_Giratina_Rotate_End
EncScript_Giratina_Rotate_Fill2:
	enccopyvar 6, 13
	call EncScript_Giratina_Raise
	call EncScript_Giratina_AdvanceWheel
	enccopyvar 7, 13
	call EncScript_Giratina_Raise
	call EncScript_Giratina_AdvanceWheel
	goto EncScript_Giratina_Rotate_End
EncScript_Giratina_Rotate_Fill3:
	enccopyvar 6, 13
	call EncScript_Giratina_Raise
	call EncScript_Giratina_AdvanceWheel
	enccopyvar 7, 13
	call EncScript_Giratina_Raise
	call EncScript_Giratina_AdvanceWheel
	enccopyvar 8, 13
	call EncScript_Giratina_Raise
	call EncScript_Giratina_AdvanceWheel
EncScript_Giratina_Rotate_End:
	call EncScript_Giratina_SetRotClock
	return

// Restores the phase guard once the ANCHOR window closes: 89/91/93 by Phase, read fresh so a phase
// transition that lands inside an open window still ends up at the right number when it closes.
EncScript_Giratina_RestoreGuard:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Giratina_RestoreGuard_Ph0
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Giratina_RestoreGuard_Ph1
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Giratina_RestoreGuard_Ph0:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	return
EncScript_Giratina_RestoreGuard_Ph1:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return

// THE YES ARM. Cancels every inversion, spends Stability, and buys a guard window - but costs
// Giratina's action this turn and Distortion never actually reaches zero once Phase 1's floor of 3
// (or Phase 2's pin at 5) is in force.
EncScript_Giratina_Anchor:
	printstring STRINGID_ENCGIRATINAANCHORYES
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Giratina_ClearInversions
	encsetvar 6, 0
	encsetvar 7, 0
	encsetvar 8, 0
	encsubvar 1, 2
	encjumpifvar CMP_NOT_EQUAL, 0, 0, EncScript_Giratina_Anchor_FloorPh1
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Giratina_Anchor_Announce
	encsetvar 1, 0
	goto EncScript_Giratina_Anchor_Announce
EncScript_Giratina_Anchor_FloorPh1:
	encjumpifvar CMP_NOT_EQUAL, 0, 1, EncScript_Giratina_Anchor_FloorPh2
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Giratina_Anchor_Announce
	encsetvar 1, 3
	goto EncScript_Giratina_Anchor_Announce
EncScript_Giratina_Anchor_FloorPh2:
	encsetvar 1, 5
EncScript_Giratina_Anchor_Announce:
	call EncScript_Giratina_Announce
	encsetrecharge ENC_TARGET_BOSS, 1
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 10, 3      @ Window
	encsetvar 3, 0        @ Stability
	call EncScript_Giratina_SetRotClock
	playanimation BS_PLAYER1, B_ANIM_RESTORE_BG
	return

// Distortion 5 -> 0 for THE RETURN, one step at a time so the announcer that has been speaking all
// fight is watched running backwards. A goto loop rather than recursive calls, so it costs no
// stack depth.
EncScript_Giratina_Return_StepDown:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Giratina_Return_StepDown_Done
	encsubvar 1, 1
	call EncScript_Giratina_Announce
	goto EncScript_Giratina_Return_StepDown
EncScript_Giratina_Return_StepDown_Done:
	return

// The catch-rate payoff for Stability, the same shape manaphy.inc scales its rate off Bond.
EncScript_Giratina_Return_CatchRate:
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Giratina_Return_CatchHigh
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Giratina_Return_CatchLow
	encsetcatchrate 30
	printstring STRINGID_ENCGIRATINACATCHSLIPPED
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Return_CatchHigh:
	encsetcatchrate 40
	printstring STRINGID_ENCGIRATINACATCHHELD
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Giratina_Return_CatchLow:
	encsetcatchrate 20
	printstring STRINGID_ENCGIRATINACATCHLOST
	waitmessage B_WAIT_TIME_LONG
	return

// --- Trigger scripts ---

EncScript_Giratina_Intro::
	printstring STRINGID_ENCGIRATINAINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCGIRATINAINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 4, 4       @ RotClock: the world turns over from turn 1, even with nothing to show yet
	encsetvar 13, 1      @ Wheel starts on SPEED
	return

// Top of every turn. Leads with flushtextbox: an OnTurnStart script that animates before printing
// anything renders on top of the still-open action menu. TurnGuard is set immediately so no branch
// out of this script can leave it re-armed.
EncScript_Giratina_TurnOpen::
	flushtextbox
	encsetvar 5, 1      @ TurnGuard
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Giratina_TurnOpen_Window
	encsubvar 4, 1      @ RotClock
EncScript_Giratina_TurnOpen_Window:
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Giratina_TurnOpen_GateCd
	encsubvar 10, 1     @ Window
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Giratina_TurnOpen_GateCd
	call EncScript_Giratina_RestoreGuard
	printstring STRINGID_ENCGIRATINAANCHORCLOSE
	waitmessage B_WAIT_TIME_LONG
EncScript_Giratina_TurnOpen_GateCd:
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Giratina_TurnOpen_AskCd
	encsubvar 11, 1     @ GateCd
EncScript_Giratina_TurnOpen_AskCd:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Giratina_TurnOpen_BreakClock
	encsubvar 9, 1      @ AskCd
EncScript_Giratina_TurnOpen_BreakClock:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Giratina_TurnOpen_Done   @ BreakClock only matters Ph2
	encjumpifvar CMP_EQUAL, 12, 0, EncScript_Giratina_TurnOpen_Done
	encsubvar 12, 1     @ BreakClock
EncScript_Giratina_TurnOpen_Done:
	return

EncScript_Giratina_AnchorPrompt::
	encaskyesno STRINGID_ENCGIRATINAANCHORASK, EncScript_Giratina_Anchor
	encsetvar 9, 2       @ AskCd: quiet for two turns; Stability is NOT spent on a NO
	printstring STRINGID_ENCGIRATINAANCHORNO
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Giratina_Gate::
	printstring STRINGID_ENCGIRATINAGATE
	waitmessage B_WAIT_TIME_LONG
	encsetprotect ENC_TARGET_BOSS
	encsetvar 11, 3      @ GateCd
	return

// THE REALITY BREAK. flushtextbox leads even though printstring follows immediately - OnTurnStart
// runs before the turn's first message, and this is the storyboard as specced, in order.
// playmoveanimation MOVE_PROTECT, not MOVE_POLTERGEIST: Shadow Force is a two-turn move whose own
// animation darkens the background and sets the attacker sprite invisible, expecting its own second
// half to undo both - neither ever runs when the animation is only borrowed as one-shot flavor here,
// which corrupted the field into every screen drawn afterward (the action menu, the "send out next
// Pokemon?" prompt, ...). Protect's animation is a plain shield sprite with no background or
// palette state to leave behind, and pairing it with encsetprotect is the same idiom
// cresselia.inc/mesprit.inc/regirock.inc already use for their own OnTurnStart braces.
EncScript_Giratina_TheBreak::
	flushtextbox
	printstring STRINGID_ENCGIRATINABREAK
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Giratina_Rotate
	encchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	playmoveanimation MOVE_PROTECT
	encsetprotect ENC_TARGET_BOSS
	encsetvar 12, 4      @ BreakClock
	return

EncScript_Giratina_TheyFled::
	encsetvar 14, 1      @ EventGuard
	printstring STRINGID_ENCGIRATINAFLED
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Giratina_TheyFled_NoStab
	encsetvar 3, 0
	printstring STRINGID_ENCGIRATINASTABGONE
	waitmessage B_WAIT_TIME_LONG
EncScript_Giratina_TheyFled_NoStab:
	call EncScript_Giratina_RaiseDistortion
	return

EncScript_Giratina_TheyFell::
	encsetvar 14, 1      @ EventGuard
	printstring STRINGID_ENCGIRATINAFELL
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Giratina_TheyFell_NoStab
	encsetvar 3, 0
	printstring STRINGID_ENCGIRATINASTABGONE
	waitmessage B_WAIT_TIME_LONG
EncScript_Giratina_TheyFell_NoStab:
	call EncScript_Giratina_RaiseDistortion
	return

// Runs last so it sees the finished turn. TurnGuard/EventGuard are cleared first - this trigger's
// own self-disabling write, and the switch/faint guard alongside it - so next turn's reactions
// arrive re-armed.
EncScript_Giratina_TurnClose::
	encsetvar 5, 0       @ TurnGuard
	encsetvar 14, 0      @ EventGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Giratina_TurnClose_Done   @ The Return: inert
	jumpiffainted BS_PLAYER1, TRUE, EncScript_Giratina_TurnClose_Stability_Done
	encjumpifvar CMP_EQUAL, 3, 3, EncScript_Giratina_TurnClose_Stability_Done
	encaddvar 3, 1
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Giratina_TurnClose_Stability_Done
	printstring STRINGID_ENCGIRATINASTABREADY
	waitmessage B_WAIT_TIME_SHORT
EncScript_Giratina_TurnClose_Stability_Done:
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Giratina_TurnClose_Telegraph
	call EncScript_Giratina_Rotate
	goto EncScript_Giratina_TurnClose_Rearm
EncScript_Giratina_TurnClose_Telegraph:
	encjumpifvar CMP_NOT_EQUAL, 4, 1, EncScript_Giratina_TurnClose_Rearm
	printstring STRINGID_ENCGIRATINAROTATESOON
	waitmessage B_WAIT_TIME_SHORT
EncScript_Giratina_TurnClose_Rearm:
	call EncScript_Giratina_RearmHealBlock
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Giratina_TurnClose_Done
	encjumpifvar CMP_NOT_EQUAL, 12, 1, EncScript_Giratina_TurnClose_Done
	printstring STRINGID_ENCGIRATINABREAKSOON
	waitmessage B_WAIT_TIME_SHORT
EncScript_Giratina_TurnClose_Done:
	return

// --- Phases ---

// 50% - THE DISTORTION WORLD. Origin Forme hands over Levitate and the 120/100/120/100 line for
// free; Shadow Force is the spec's Distortion Gate storyboard on Giratina's own learnset. Distortion
// is set to 3 (the floor from here) and Rotate's own +1 brings it to 4, filling two slots
// immediately - "the world changes the moment it changes forme."
EncScript_Giratina_DistortionWorld::
	encsetvar 0, 1       @ Phase
	printstring STRINGID_ENCGIRATINAPHASE1
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_GIRATINA_ORIGIN, EncScript_Giratina_DistortionWorld_Body, B_ANIM_FORM_CHANGE
EncScript_Giratina_DistortionWorld_Body:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_POLTERGEIST
	encsetmove ENC_TARGET_BOSS, 1, MOVE_DRACO_METEOR
	printstring STRINGID_ENCGIRATINAPHASE1GROUND
	waitmessage B_WAIT_TIME_LONG
	encsetdamagereduction ENC_TARGET_BOSS, 93
	encsetvar 1, 3       @ Distortion, floored at 3 from here
	call EncScript_Giratina_Rotate
	return

// 25% - REALITY BREAK. Earthquake is the payoff for GRAVITY COLLAPSED, which is also the rule that
// grounds Giratina itself. Distortion pins at 5 and every slot fills at once.
EncScript_Giratina_RealityBreak::
	encsetvar 0, 2       @ Phase
	printstring STRINGID_ENCGIRATINAPHASE2
	waitmessage B_WAIT_TIME_LONG
	encsetdamagereduction ENC_TARGET_BOSS, 95
	encsetmove ENC_TARGET_BOSS, 2, MOVE_EARTHQUAKE
	encsetvar 1, 5       @ Distortion, pinned from here
	call EncScript_Giratina_Rotate
	encsetvar 12, 2      @ BreakClock: the first break comes fast
	return

// 10% - THE RETURN, and the catch window. The ladder-down speaks the meter running backwards; the
// automatic catch-window damage guard owns the reduction from here, so the 99 below sets the value
// that guard restores, not the live one.
EncScript_Giratina_Return::
	encsetvar 0, 3       @ Phase
	printstring STRINGID_ENCGIRATINARETURN1
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Giratina_Return_StepDown
	call EncScript_Giratina_ClearInversions
	encsetvar 6, 0
	encsetvar 7, 0
	encsetvar 8, 0
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Giratina_Return_Shed
	goto EncScript_Giratina_Return_FormChange
EncScript_Giratina_Return_Shed:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
EncScript_Giratina_Return_FormChange:
	encformchange ENC_TARGET_BOSS, SPECIES_GIRATINA_ALTERED, EncScript_Giratina_Return_Body, B_ANIM_FORM_CHANGE_INSTANT
EncScript_Giratina_Return_Body:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_SHADOW_CLAW
	encsetmove ENC_TARGET_BOSS, 1, MOVE_DRAGON_CLAW
	encsetmove ENC_TARGET_BOSS, 2, MOVE_TOXIC
	encsetmove ENC_TARGET_BOSS, 3, MOVE_HEX
	printstring STRINGID_ENCGIRATINARETURN2
	waitmessage B_WAIT_TIME_LONG
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCGIRATINAWEAKENED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Giratina_Return_CatchRate
	encsetvar 4, 0       @ RotClock
	encsetvar 9, 0       @ AskCd
	encsetvar 10, 0      @ Window
	encsetvar 11, 0      @ GateCd
	encsetvar 12, 0      @ BreakClock
	return
