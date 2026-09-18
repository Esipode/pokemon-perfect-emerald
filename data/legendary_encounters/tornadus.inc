// Tornadus, "The Cyclone Pokemon". Var indices, pinned by the always-true Conditions on
// EncScript_Tornadus_Intro:
// 0 Phase (0 Still Air / 1 Storm Front / 2 Tempest / 3 Grounded), 1 Wind (0-5), 2 LastBand (the last
// ANNOUNCED Wind), 3 TurnGuard, 4 Keyed (a type key counted this turn), 5 Eye (0 none / 1-2 turns
// remaining - an Eye stands while non-zero), 6 BossMark (Tornadus HP% at turn open), 7 Scratch (the
// copy ENC_SNAP_DAMAGE consumes), 8 Flew, 9 Gather (0 none / 1-3 turns left / 4 resolving this end
// of turn), 10 Window (turns left in the open-sky window), 11 EyeCd, 12 EventGuard, 13 Cyclones,
// 14 Displaced (0 none / 1 landed / 2 attempted and failed), 15 Windows.
//
// WIND IS THE GUARD, IN BOTH DIRECTIONS. EncScript_Tornadus_ApplyWinds is the damage reduction's
// only writer and reads Phase, Window, Wind and Eye absolutely, so a window closing or an Eye
// passing undoes one opening with no separate teardown path. The open-sky value is flat at 70 in
// every phase: it is the fight's single reward and it must not get worse as the fight goes on.
//
// THE WEATHER BRANCH IS MADE AT SELECTION, not in a script. jumpifweatheraffected reads
// gBattlerAttacker, which is stale at a turn checkpoint, so the turn-open scaffold ships as two
// triggers with mutually exclusive Weather conditions that share this one body through _Head/_Tail.
//
// STAT STAGES ARE WRITTEN ABSOLUTELY, the Rayquaza idiom: -12 clamps to the floor, then a single
// climb lands on the stage that was meant. That makes the Eye's grant and its revocation immune to a
// player Haze, Defog or accuracy drop landing in between.
//
// Animation note: gBattlerAttacker is stale at OnTurnStart and OnTurnEnd, so everything here uses
// playanimation with an explicit battler rather than playmoveanimation.

// --- Shared subroutines (call/return) ---

// THE SINGLE WRITER of the damage reduction. Phase 3 returns immediately: once balls are allowed the
// engine's own catch guard owns the number.
EncScript_Tornadus_ApplyWinds:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Tornadus_ApplyWinds_Done
	encjumpifvar CMP_GREATER_THAN, 10, 0, EncScript_Tornadus_ApplyWinds_Open
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Tornadus_ApplyWinds_Eye
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Tornadus_ApplyWinds_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Tornadus_ApplyWinds_P1
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Tornadus_ApplyWinds_P0Hot
	encsetdamagereduction ENC_TARGET_BOSS, 87
	return
EncScript_Tornadus_ApplyWinds_P0Hot:
	encsetdamagereduction ENC_TARGET_BOSS, 89
	return
EncScript_Tornadus_ApplyWinds_P1:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Tornadus_ApplyWinds_P1Hot
	encsetdamagereduction ENC_TARGET_BOSS, 90
	return
EncScript_Tornadus_ApplyWinds_P1Hot:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Tornadus_ApplyWinds_P2:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Tornadus_ApplyWinds_P2Hot
	encsetdamagereduction ENC_TARGET_BOSS, 93
	return
EncScript_Tornadus_ApplyWinds_P2Hot:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Tornadus_ApplyWinds_Eye:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Tornadus_ApplyWinds_EyeP2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Tornadus_ApplyWinds_EyeP1
	encsetdamagereduction ENC_TARGET_BOSS, 92
	return
EncScript_Tornadus_ApplyWinds_EyeP1:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	return
EncScript_Tornadus_ApplyWinds_EyeP2:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	return
EncScript_Tornadus_ApplyWinds_Open:
	encsetdamagereduction ENC_TARGET_BOSS, 80
EncScript_Tornadus_ApplyWinds_Done:
	return

// THE WIND CALLOUT (guideline 14). LastBand holds the last ANNOUNCED Wind; each arm compares it
// against its own literal and stays quiet if that is what was last said, so the line prints only on
// a difference - in BOTH directions. The falling lines are as load-bearing as the rising ones: a
// fall is the player's own work (a Rock key, an Electric key, a weather they set, a Cyclone they
// rode out) and the fight has no other way to confirm those did anything.
EncScript_Tornadus_AnnounceWind:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Tornadus_AnnounceWind_0
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Tornadus_AnnounceWind_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Tornadus_AnnounceWind_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Tornadus_AnnounceWind_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Tornadus_AnnounceWind_4
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Tornadus_AnnounceWind_Done
	printstring STRINGID_ENCTORNADUSWIND5
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Tornadus_AnnounceWind_Latch
EncScript_Tornadus_AnnounceWind_0:
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Tornadus_AnnounceWind_Done
	printstring STRINGID_ENCTORNADUSWIND0
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Tornadus_AnnounceWind_Latch
EncScript_Tornadus_AnnounceWind_1:
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Tornadus_AnnounceWind_Done
	printstring STRINGID_ENCTORNADUSWIND1
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Tornadus_AnnounceWind_Latch
EncScript_Tornadus_AnnounceWind_2:
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Tornadus_AnnounceWind_Done
	printstring STRINGID_ENCTORNADUSWIND2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Tornadus_AnnounceWind_Latch
EncScript_Tornadus_AnnounceWind_3:
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Tornadus_AnnounceWind_Done
	printstring STRINGID_ENCTORNADUSWIND3
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Tornadus_AnnounceWind_Latch
EncScript_Tornadus_AnnounceWind_4:
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Tornadus_AnnounceWind_Done
	printstring STRINGID_ENCTORNADUSWIND4
	waitmessage B_WAIT_TIME_LONG
EncScript_Tornadus_AnnounceWind_Latch:
	enccopyvar 2, 1     @ LastBand = Wind
EncScript_Tornadus_AnnounceWind_Done:
	return

// Wind climbs, clamped at 5. Two entry points so a caller never has to do the clamp itself.
EncScript_Tornadus_Wind2:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Tornadus_Wind2_Cap
	encaddvar 1, 2
	return
EncScript_Tornadus_Wind2_Cap:
	encsetvar 1, 5
	return

EncScript_Tornadus_Wind1:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Tornadus_Wind1_Done
	encaddvar 1, 1
EncScript_Tornadus_Wind1_Done:
	return

// Wind falls, floored at 0. Both are no-ops while an Eye stands - the calm centre ignores every Wind
// fall, which is the Eye's whole point and the reason the Rock key and the weather tick go quiet.
EncScript_Tornadus_Calm2:
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Tornadus_Calm2_Done
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Tornadus_Calm2_Floor
	encsubvar 1, 2
	return
EncScript_Tornadus_Calm2_Floor:
	encsetvar 1, 0
EncScript_Tornadus_Calm2_Done:
	return

EncScript_Tornadus_Calm1:
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Tornadus_Calm1_Done
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Tornadus_Calm1_Done
	encsubvar 1, 1
EncScript_Tornadus_Calm1_Done:
	return

// THE EYE PASSES. Takes the calm centre back and starts the cooldown. Silent - every caller prints
// its own line - and a no-op when no Eye stands, so it may be called unconditionally.
EncScript_Tornadus_EyeRevoke:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Tornadus_EyeRevoke_Done
	encsetvar 5, 0     @ Eye
	encsetvar 11, 3    @ EyeCd
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 6     @ absolute neutral, not a nudge
	encchangestat ENC_TARGET_BOSS, STAT_ACC, -12
	encchangestat ENC_TARGET_BOSS, STAT_ACC, 6
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Tornadus_ApplyWinds
EncScript_Tornadus_EyeRevoke_Done:
	return

// THE OPEN SKY - the fight's single reward, and the only thing that drops the guard to 70. Window 3
// buys two turns of play, because the decrement happens at the NEXT turn open. The Tempest break
// raises it to 4 on its own after calling this.
EncScript_Tornadus_OpenSky:
	encsetvar 10, 3    @ Window
	encaddvar 15, 1    @ Windows
	encsetrecharge ENC_TARGET_BOSS, 2
	printstring STRINGID_ENCTORNADUSOPENSKY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	call EncScript_Tornadus_ApplyWinds
	return

// THE GUST - the board wipe, from Wind 2. Each clear carries a fail label so a clean field costs
// nothing; the chip is the only part that always lands.
EncScript_Tornadus_Gusts:
	printstring STRINGID_ENCTORNADUSGUST
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_STRONG_WINDS
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Tornadus_Gusts_Screens
EncScript_Tornadus_Gusts_Screens:
	encclearscreens ENC_TARGET_PLAYER_LEFT, EncScript_Tornadus_Gusts_Chip
EncScript_Tornadus_Gusts_Chip:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Tornadus_Gusts_Hard
	encchangehp ENC_TARGET_PLAYER_LEFT, -4, ENC_AMOUNT_PERCENT
	return
EncScript_Tornadus_Gusts_Hard:
	encchangehp ENC_TARGET_PLAYER_LEFT, -7, ENC_AMOUNT_PERCENT
	return

// --- Trigger scripts ---

// LastBand is parked on a value Wind can never hold so the callout's latch fires on turn 0: the
// player is told the air is Calm before anything happens, and every later change reads as a change.
EncScript_Tornadus_Intro::
	printstring STRINGID_ENCTORNADUSINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCTORNADUSINTRO2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	call EncScript_Tornadus_ApplyWinds
	encsetvar 2, 9     @ LastBand: nothing announced yet
	call EncScript_Tornadus_AnnounceWind
	return

// Top of every turn, in the clear-sky copy. _Head and _Tail carry the whole body; the weather copy
// below is the same two calls with the weather bite wedged between them.
EncScript_Tornadus_TurnOpen::
	call EncScript_Tornadus_TurnOpen_Head
	goto EncScript_Tornadus_TurnOpen_Tail

// Top of every turn while a weather stands. THE SKY IS ALREADY TAKEN: Wind loses its hold, a
// standing Eye is torn open, and a gathering Hurricane disperses.
EncScript_Tornadus_TurnOpenWx::
	call EncScript_Tornadus_TurnOpen_Head
	call EncScript_Tornadus_WeatherBites
	goto EncScript_Tornadus_TurnOpen_Tail

// Leads with flushtextbox - an OnTurnStart script that animates before any dialogue renders on top
// of the still-open action menu (the BG0 gotcha). TurnGuard is set immediately so no branch can
// leave it re-armed. Ticks the Eye cooldown and the open-sky window, then the Eye itself.
EncScript_Tornadus_TurnOpen_Head:
	flushtextbox
	encsetvar 3, 1     @ TurnGuard
	encsetvar 4, 0     @ Keyed
	encsetvar 8, 0     @ Flew
	encsetvar 12, 0    @ EventGuard - a faint or a drag that landed after _TurnClose left this set
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Tornadus_TurnOpen_Head_Done   @ Phase 3: quiet
	encjumpifvar CMP_EQUAL, 11, 0, EncScript_Tornadus_TurnOpen_Head_Window
	encsubvar 11, 1    @ EyeCd
EncScript_Tornadus_TurnOpen_Head_Window:
	encjumpifvar CMP_EQUAL, 10, 0, EncScript_Tornadus_TurnOpen_Head_Eye
	encsubvar 10, 1    @ Window: the open sky closes on its own
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Tornadus_TurnOpen_Head_Eye
	printstring STRINGID_ENCTORNADUSGATHERAGAIN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
EncScript_Tornadus_TurnOpen_Head_Eye:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Tornadus_TurnOpen_Head_Done
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Tornadus_TurnOpen_Head_EyePasses
	encsubvar 5, 1
	printstring STRINGID_ENCTORNADUSEYEHOLDS   @ the recurring cue while the calm centre stands
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Tornadus_TurnOpen_Head_Done
EncScript_Tornadus_TurnOpen_Head_EyePasses:
	call EncScript_Tornadus_EyeRevoke
	printstring STRINGID_ENCTORNADUSEYEPASSES
	waitmessage B_WAIT_TIME_LONG
EncScript_Tornadus_TurnOpen_Head_Done:
	return

// THE SKY IS ALREADY TAKEN. Runs between _Head and _Tail, so the Hurricane it disperses is gone
// before _Tail would have counted the turn down.
EncScript_Tornadus_WeatherBites:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Tornadus_WeatherBites_Done
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Tornadus_WeatherBites_Eye
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Tornadus_WeatherBites_Gather
	call EncScript_Tornadus_Calm1
	printstring STRINGID_ENCTORNADUSWEATHERBITES
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Tornadus_WeatherBites_Gather
EncScript_Tornadus_WeatherBites_Eye:
	call EncScript_Tornadus_EyeRevoke
	printstring STRINGID_ENCTORNADUSEYETORN
	waitmessage B_WAIT_TIME_LONG
EncScript_Tornadus_WeatherBites_Gather:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Tornadus_WeatherBites_Done
	encsetvar 9, 0     @ Gather: the hurricane finds no sky to gather in
	printstring STRINGID_ENCTORNADUSGATHERDISPERSED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
EncScript_Tornadus_WeatherBites_Done:
	return

// Counts the Hurricane down, holds the Tempest's pins in place, takes the mark this turn's damage is
// measured against, then writes the guard and says the band. Ends the turn-open script, so its
// return is the one that leaves the trigger.
EncScript_Tornadus_TurnOpen_Tail:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Tornadus_TurnOpen_Tail_Quiet
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Tornadus_TurnOpen_Tail_P2
	encjumpifvar CMP_EQUAL, 9, 4, EncScript_Tornadus_TurnOpen_Tail_P2
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Tornadus_TurnOpen_Tail_Breaks
	encsubvar 9, 1
	encjumpifvar CMP_EQUAL, 9, 2, EncScript_Tornadus_TurnOpen_Tail_G2
	printstring STRINGID_ENCTORNADUSGATHER1
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Tornadus_TurnOpen_Tail_P2
EncScript_Tornadus_TurnOpen_Tail_G2:
	printstring STRINGID_ENCTORNADUSGATHER2
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Tornadus_TurnOpen_Tail_P2
EncScript_Tornadus_TurnOpen_Tail_Breaks:
	encsetvar 9, 4     @ resolving at this end of turn
	printstring STRINGID_ENCTORNADUSGATHERBREAKS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
EncScript_Tornadus_TurnOpen_Tail_P2:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Tornadus_TurnOpen_Tail_Mark
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Tornadus_TurnOpen_Tail_Mark   @ a window is open
	encsetvar 1, 5     @ Wind, pinned for as long as the Tempest holds
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	printstring STRINGID_ENCTORNADUSTRAPPEDCUE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Tornadus_TurnOpen_Tail_Mark:
	encsnapshothp ENC_TARGET_BOSS, 6, ENC_SNAP_SET
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
EncScript_Tornadus_TurnOpen_Tail_Quiet:
	return

// THE EYE OF THE STORM. The line names the calm centre and says the stone and the cold are lost in
// it - pointing at the cause without naming the counter (guideline 14).
EncScript_Tornadus_FormEye::
	encsetvar 5, 2     @ Eye: self-disabling
	printstring STRINGID_ENCTORNADUSEYEFORMS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, -12
	encchangestat ENC_TARGET_BOSS, STAT_EVASION, 8     @ absolute +2, not a nudge
	encchangestat ENC_TARGET_BOSS, STAT_ACC, -12
	encchangestat ENC_TARGET_BOSS, STAT_ACC, 7         @ absolute +1
	playanimation BS_OPPONENT1, B_ANIM_STATS_CHANGE
	call EncScript_Tornadus_ApplyWinds
	return

// TORNADUS GATHERS A HURRICANE. _TurnOpen_Tail reprints the countdown every turn after this, so the
// beat cannot be missed.
EncScript_Tornadus_Gather::
	encsetvar 9, 3     @ Gather: self-disabling
	printstring STRINGID_ENCTORNADUSGATHERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	printstring STRINGID_ENCTORNADUSGATHER3
	waitmessage B_WAIT_TIME_LONG
	return

// THE WIND FEEDS ON WIND. Nothing but the latch - the +1 is paid at _TurnClose, so a multi-hit
// Flying move cannot vault the meter and the accounting stays in one place.
EncScript_Tornadus_Flew::
	encsetvar 8, 1     @ Flew
	return

// THE STONE BREAKS THE WIND'S SHAPE - Wind -2, and nothing at all while the Eye stands.
EncScript_Tornadus_KeyRock::
	encsetvar 4, 1     @ Keyed
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Tornadus_KeyRock_Normal
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Tornadus_KeyRock_Normal
	goto EncScript_Tornadus_Subside
EncScript_Tornadus_KeyRock_Normal:
	encjumpifvar CMP_GREATER_THAN, 5, 0, EncScript_Tornadus_KeyRock_Eye
	call EncScript_Tornadus_Calm2
	printstring STRINGID_ENCTORNADUSKEYROCK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	goto EncScript_Tornadus_KeyRock_Fin
EncScript_Tornadus_KeyRock_Eye:
	printstring STRINGID_ENCTORNADUSEYESWALLOWS
	waitmessage B_WAIT_TIME_LONG
EncScript_Tornadus_KeyRock_Fin:
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// THE COLD STAGGERS IT. encsetrecharge at OnMoveEnd costs NEXT turn's action. No Wind change, and
// the Eye does not stop it - the Eye ignores Wind falls, and a stagger is not one.
EncScript_Tornadus_KeyIce::
	encsetvar 4, 1     @ Keyed
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Tornadus_KeyIce_Normal
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Tornadus_KeyIce_Normal
	goto EncScript_Tornadus_Subside
EncScript_Tornadus_KeyIce_Normal:
	encsetrecharge ENC_TARGET_BOSS, 2
	printstring STRINGID_ENCTORNADUSKEYICE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	return

// DISRUPTION. Branches on state, widest case first: the Tempest break, then a gathering Hurricane
// (interrupt - and an interrupt is what opens the sky), then a standing Eye, then the plain fall.
EncScript_Tornadus_KeyElectric::
	encsetvar 4, 1     @ Keyed
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Tornadus_KeyElectric_Gather
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Tornadus_KeyElectric_Gather
	goto EncScript_Tornadus_Subside
EncScript_Tornadus_KeyElectric_Gather:
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Tornadus_KeyElectric_Eye
	encsetvar 9, 0     @ Gather: interrupted
	call EncScript_Tornadus_Calm2
	printstring STRINGID_ENCTORNADUSGATHERBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Tornadus_OpenSky
	goto EncScript_Tornadus_KeyElectric_Fin
EncScript_Tornadus_KeyElectric_Eye:
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Tornadus_KeyElectric_Plain
	call EncScript_Tornadus_EyeRevoke
	call EncScript_Tornadus_Calm1
	printstring STRINGID_ENCTORNADUSEYEBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
	goto EncScript_Tornadus_KeyElectric_Fin
EncScript_Tornadus_KeyElectric_Plain:
	call EncScript_Tornadus_Calm1
	printstring STRINGID_ENCTORNADUSKEYELECTRIC
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_HIT
EncScript_Tornadus_KeyElectric_Fin:
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// THE WINDS SUBSIDE. Reached from all three keys while the Tempest holds and no window is open, so
// the endgame beat is earned rather than timed. Everything the Tempest pinned comes off at once and
// the window runs one turn longer than a normal one.
EncScript_Tornadus_Subside:
	encsetvar 1, 0     @ Wind
	encsetvar 9, 0     @ Gather
	call EncScript_Tornadus_EyeRevoke
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	removeweather
	printstring STRINGID_ENCTORNADUSSUBSIDE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	call EncScript_Tornadus_OpenSky
	encsetvar 10, 4    @ Window: a Tempest break buys three turns, not two
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// One of the player's Pokemon fell - it moved you, so it moved itself. EventGuard stops the
// replacement's switch-in being processed a second time.
EncScript_Tornadus_TheyFell::
	encsetvar 12, 1    @ EventGuard
	call EncScript_Tornadus_Wind1
	printstring STRINGID_ENCTORNADUSTHEYFELL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// Somebody walked in on their own terms. Says so, and says that it changed nothing.
EncScript_Tornadus_TheyEntered::
	encsetvar 12, 1    @ EventGuard
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Tornadus_TheyEntered_Done
	printstring STRINGID_ENCTORNADUSTHEYENTERED
	waitmessage B_WAIT_TIME_SHORT
EncScript_Tornadus_TheyEntered_Done:
	return

// THE HURRICANE LANDS. Runs before _TurnClose so its own damage is never counted as the player's
// damage for the "it rode the attacks out" test. Self-disabling: it clears Gather.
// It does NOT drag on its own. At priority 79 it sits one script ahead of the ledger, so a Displaced
// written here would be paid and cleared at 80 and _Displace would re-select at 82 and drag a second
// time. Putting Wind back to 5 instead makes that drag certain - one drag, one script later, with
// the accounting intact.
EncScript_Tornadus_Hurricane::
	encsetvar 9, 0     @ Gather
	printstring STRINGID_ENCTORNADUSHURRICANE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_STRONG_WINDS
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Tornadus_Hurricane_P2
	encchangehp ENC_TARGET_PLAYER_LEFT, -30, ENC_AMOUNT_PERCENT
	goto EncScript_Tornadus_Hurricane_Slow
EncScript_Tornadus_Hurricane_P2:
	encchangehp ENC_TARGET_PLAYER_LEFT, -40, ENC_AMOUNT_PERCENT
EncScript_Tornadus_Hurricane_Slow:
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -2
	playanimation BS_PLAYER1, B_ANIM_STATS_CHANGE
	encsetvar 1, 5     @ Wind
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// THE LEDGER, and the only place per-turn state is cleared. The gust first, then the four ways the
// wind climbs, then the turn tax. Self-disables by clearing TurnGuard.
EncScript_Tornadus_TurnClose::
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Tornadus_TurnClose_Quiet   @ Phase 3: quiet
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Tornadus_TurnClose_Damage
	encjumpifvar CMP_NOT_EQUAL, 10, 0, EncScript_Tornadus_TurnClose_Damage
	call EncScript_Tornadus_Gusts
EncScript_Tornadus_TurnClose_Damage:
	encsetvar 7, 0     @ Scratch
	enccopyvar 7, 6    @ Scratch = BossMark
	encsnapshothp ENC_TARGET_BOSS, 7, ENC_SNAP_DAMAGE, EncScript_Tornadus_TurnClose_NoDamage
	goto EncScript_Tornadus_TurnClose_Rode
EncScript_Tornadus_TurnClose_NoDamage:
	encsetvar 7, 0     @ nothing taken off Tornadus this turn
EncScript_Tornadus_TurnClose_Rode:
	encjumpifvar CMP_GREATER_THAN, 7, 4, EncScript_Tornadus_TurnClose_Flew
	call EncScript_Tornadus_Wind1   @ under 5% - it rode the attacks out
EncScript_Tornadus_TurnClose_Flew:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Tornadus_TurnClose_Moved
	call EncScript_Tornadus_Wind1   @ something flew on the field
EncScript_Tornadus_TurnClose_Moved:
	encjumpifvar CMP_NOT_EQUAL, 14, 1, EncScript_Tornadus_TurnClose_Tax
	call EncScript_Tornadus_Wind1   @ last turn's drag landed; a failed one (2) pays nothing
EncScript_Tornadus_TurnClose_Tax:
	encsetvar 14, 0    @ Displaced, consumed
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Tornadus_TurnClose_Tax1
	call EncScript_Tornadus_Wind2   @ the storm feeds itself from Storm Front on
	goto EncScript_Tornadus_TurnClose_Settle
EncScript_Tornadus_TurnClose_Tax1:
	call EncScript_Tornadus_Wind1
EncScript_Tornadus_TurnClose_Settle:
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
EncScript_Tornadus_TurnClose_Quiet:
	encsetvar 4, 0     @ Keyed
	encsetvar 8, 0     @ Flew
	encsetvar 12, 0    @ EventGuard
	encsetvar 3, 0     @ TurnGuard
	return

// DISPLACEMENT - the wind decides who is standing in front of it. Certain from Wind 4 and through
// the whole Tempest, a 40% roll at Wind 2-3. Displaced is set on every path so the trigger latches
// off either way, but only the value 1 is worth a +1 at the next ledger.
EncScript_Tornadus_Displace::
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Tornadus_Displace_Go
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Tornadus_Displace_Go
	encjumpifchance 40, EncScript_Tornadus_Displace_Go
	encsetvar 14, 2    @ the gust did not take hold this turn
	return
EncScript_Tornadus_Displace_Go:
	encsetvar 14, 1    @ Displaced
	printstring STRINGID_ENCTORNADUSDISPLACE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SLIDE_OFFSCREEN
	encswitchout ENC_TARGET_PLAYER_LEFT, EncScript_Tornadus_Displace_Fail
	return
EncScript_Tornadus_Displace_Fail:
	encsetvar 14, 2    @ latched off, but no +1 is owed for a drag that moved nobody
	printstring STRINGID_ENCTORNADUSDISPLACEFAIL
	waitmessage B_WAIT_TIME_LONG
	return

// THE CYCLONE. Runs last, so a Wind that reached 5 anywhere in the turn spends itself the same turn.
// This is the ONLY thing in the fight that clears weather - a sky the player took is worth several
// turns, not one.
EncScript_Tornadus_Cyclone::
	encaddvar 13, 1    @ Cyclones
	encjumpifvar CMP_GREATER_THAN, 13, 2, EncScript_Tornadus_Cyclone_Line3
	encjumpifvar CMP_EQUAL, 13, 2, EncScript_Tornadus_Cyclone_Line2
	printstring STRINGID_ENCTORNADUSCYCLONE1
	goto EncScript_Tornadus_Cyclone_Hit
EncScript_Tornadus_Cyclone_Line2:
	printstring STRINGID_ENCTORNADUSCYCLONE2
	goto EncScript_Tornadus_Cyclone_Hit
EncScript_Tornadus_Cyclone_Line3:
	printstring STRINGID_ENCTORNADUSCYCLONE3
EncScript_Tornadus_Cyclone_Hit:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	playanimation BS_PLAYER1, B_ANIM_STRONG_WINDS
	removeweather
	encclearhazards ENC_TARGET_ALL_BATTLERS, EncScript_Tornadus_Cyclone_Screens
EncScript_Tornadus_Cyclone_Screens:
	encclearscreens ENC_TARGET_PLAYER_LEFT, EncScript_Tornadus_Cyclone_Chip
EncScript_Tornadus_Cyclone_Chip:
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -12, ENC_AMOUNT_PERCENT
	encjumpifvar CMP_NOT_EQUAL, 14, 0, EncScript_Tornadus_Cyclone_Spent   @ one drag a turn
	playanimation BS_PLAYER1, B_ANIM_SLIDE_OFFSCREEN
	encswitchout ENC_TARGET_PLAYER_LEFT, EncScript_Tornadus_Cyclone_Spent
EncScript_Tornadus_Cyclone_Spent:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Tornadus_Cyclone_Reset0
	encsetvar 1, 3     @ Wind, from Storm Front on
	goto EncScript_Tornadus_Cyclone_Fin
EncScript_Tornadus_Cyclone_Reset0:
	encsetvar 1, 2     @ Wind
EncScript_Tornadus_Cyclone_Fin:
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// --- Phases ---

// 50% - STORM FRONT. The moveset is untouched: the escalation here is the doubled turn tax and the
// Hurricane, not new attacks.
EncScript_Tornadus_StormFront::
	encsetvar 0, 1     @ Phase
	printstring STRINGID_ENCTORNADUSSTORMFRONT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_STRONG_WINDS
	call EncScript_Tornadus_Wind1
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// 15% - TEMPEST. The Therian forme, Bleakwind Storm in Air Slash's slot, a tailwind and a raw Speed
// bump - "higher Wind makes Tornadus faster", made visible. Wind is pinned at 5 and the player is
// trapped, both re-applied at every turn open, so a key that breaks the Tempest has to be landed
// again for each window.
EncScript_Tornadus_Tempest::
	encsetvar 0, 2     @ Phase
	printstring STRINGID_ENCTORNADUSTEMPEST
	waitmessage B_WAIT_TIME_LONG
	encformchange ENC_TARGET_BOSS, SPECIES_TORNADUS_THERIAN, EncScript_Tornadus_Tempest_Move, B_ANIM_FORM_CHANGE
EncScript_Tornadus_Tempest_Move:
	encsetmove ENC_TARGET_BOSS, 0, MOVE_BLEAKWIND_STORM
	restoremovepp BS_OPPONENT1
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 20, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encsettrapped ENC_TARGET_PLAYER_LEFT, TRUE
	printstring STRINGID_ENCTORNADUSTRAPPED
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 5     @ Wind, pinned
	encsetvar 9, 0     @ Gather
	encsetvar 11, 0    @ EyeCd
	call EncScript_Tornadus_ApplyWinds
	call EncScript_Tornadus_AnnounceWind
	return

// 1 HP - GROUNDED, and the catch window (guideline 10). Every guard comes off in one place and the
// ball is allowed. Every trigger that could still fire is gated Phase <= 2, so from here the fight
// is quiet. encsetdamagereduction 99 writes the value the automatic catch guard will RESTORE, not
// the live one.
EncScript_Tornadus_Grounded::
	encsetvar 0, 3     @ Phase
	call EncScript_Tornadus_EyeRevoke
	encsetvar 1, 0     @ Wind
	encsetvar 9, 0     @ Gather
	encsetvar 10, 0    @ Window
	call EncScript_Tornadus_AnnounceWind
	encsettrapped ENC_TARGET_PLAYER_LEFT, FALSE
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	removeweather
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	playanimation BS_OPPONENT1, B_ANIM_MON_SCARED
	printstring STRINGID_ENCTORNADUSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
