// Manaphy, "The Prince of the Sea". Var indices, pinned by the always-true Conditions on
// EncScript_Manaphy_Intro:
// 0 Phase (0 Prince of the Sea / 1 Heart of the Ocean / 2 Ocean's Heart / 3 Weakened), 1 Bond (0-5),
// 2 LastBand (the last ANNOUNCED band index; 255 forces the first announcement), 3 Bonds (the network
// count), 4 Heartbreak (turns of the Heartbroken state left), 5 Echo (the depth of the bond that just
// broke), 6 TurnGuard, 7 SwapClock, 8 TideClock, 9 TideBroken, 10 BossHpMark, 11 ShareGuard,
// 12 Collapsed, 13 Pulse.
//
// It bonds with the player's Pokemon the moment the battle starts and it does not stop. The meter
// climbs by itself, one step a turn, and the only thing that ever moves it back is the player sending
// their Pokemon away - which drops the guard from 94 to 70 for three turns and wipes every stat stage
// Manaphy has. That window is the entire fight. The price is that the Pokemon coming in inherits the
// broken bond, and that Manaphy files the bond away for Phase 2, where the count turns the player's
// own damage back on them. Nothing states any of that. The fight only ever describes the bond.

// --- Shared subroutines (call/return) ---

// The Mesprit absolute reset: -12 clamps to -6, then +6 lands on neutral from wherever the stage had
// drifted. Every band writes its stance through this, so a fight that crosses six bands does not
// finish on a pile of accumulated steps and a band stays reachable from either direction.
EncScript_Manaphy_NeutralStance:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 6
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, -12
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 6
	return

// The stance the Bond meter alone dictates - shared by the five Bond bands and by Ocean's Heart,
// which keeps the bond's stance and only overrides the guard.
// The broken tide's Sp. Def drop is applied HERE rather than by EncScript_Manaphy_TideBreak, because
// this routine is the sole writer of Manaphy's stat stages: a stage written anywhere else would be
// wiped by the next ApplyBond.
EncScript_Manaphy_BondStance:
	call EncScript_Manaphy_NeutralStance
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Manaphy_BondStance_Tide
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Manaphy_BondStance_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Manaphy_BondStance_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Manaphy_BondStance_4
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	goto EncScript_Manaphy_BondStance_Tide
EncScript_Manaphy_BondStance_2:
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	goto EncScript_Manaphy_BondStance_Tide
EncScript_Manaphy_BondStance_3:
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	goto EncScript_Manaphy_BondStance_Tide
EncScript_Manaphy_BondStance_4:
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
EncScript_Manaphy_BondStance_Tide:
	encjumpifvar CMP_NOT_EQUAL, 9, 1, EncScript_Manaphy_BondStance_Done   @ TideBroken
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
EncScript_Manaphy_BondStance_Done:
	return

// THE SOLE OWNER of the stance, the guard and the callout. Nothing else writes any of the three,
// which is what makes it safe to call from a phase script and idempotent when the band did not move.
// Ladder, first match wins:
//   Heartbroken -> Shattered (collapsed) or Heartbroken   Bond 1-5 -> the five bands
//   Ocean's Heart (Phase 2, not collapsed, not heartbroken) -> the network band
// Once the collapse's Heartbreak expires, Collapsed == 1 and Phase == 2 fall through to the ordinary
// ladder: the network is gone, so the fight finishes on ordinary rules.
EncScript_Manaphy_ApplyBond:
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Manaphy_Band_Grief     @ Heartbreak
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Manaphy_Band_Ladder       @ Phase
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Manaphy_Band_Ladder          @ Collapsed
	goto EncScript_Manaphy_Band_Network
EncScript_Manaphy_Band_Grief:
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Manaphy_Band_Shattered
	goto EncScript_Manaphy_Band_Broken
EncScript_Manaphy_Band_Ladder:
	encjumpifvar CMP_LESS_THAN, 1, 2, EncScript_Manaphy_Band_1
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Manaphy_Band_2
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Manaphy_Band_3
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Manaphy_Band_4
	goto EncScript_Manaphy_Band_5

// Each band body: stance, then guard, then the callout - and the callout only speaks when LastBand
// says this band has not been announced yet. Pulse is zeroed alongside it so the standing line
// restarts its count from a real state change rather than talking over the transition.

EncScript_Manaphy_Band_1:
	call EncScript_Manaphy_BondStance
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encjumpifvar CMP_EQUAL, 2, 1, EncScript_Manaphy_Band_Done
	encsetvar 2, 1      @ LastBand
	encsetvar 13, 0     @ Pulse
	printstring STRINGID_ENCMANAPHYBAND1
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_Manaphy_Band_2:
	call EncScript_Manaphy_BondStance
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encjumpifvar CMP_EQUAL, 2, 2, EncScript_Manaphy_Band_Done
	encsetvar 2, 2
	encsetvar 13, 0
	printstring STRINGID_ENCMANAPHYBAND2
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_AQUA_RING_HEAL
	return

EncScript_Manaphy_Band_3:
	call EncScript_Manaphy_BondStance
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encjumpifvar CMP_EQUAL, 2, 3, EncScript_Manaphy_Band_Done
	encsetvar 2, 3
	encsetvar 13, 0
	printstring STRINGID_ENCMANAPHYBAND3
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_AQUA_RING_HEAL
	return

EncScript_Manaphy_Band_4:
	call EncScript_Manaphy_BondStance
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encjumpifvar CMP_EQUAL, 2, 4, EncScript_Manaphy_Band_Done
	encsetvar 2, 4
	encsetvar 13, 0
	printstring STRINGID_ENCMANAPHYBAND4
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_INGRAIN_HEAL
	return

// AS ONE. 94% reduction, and from Phase 1 the tide heal outruns almost anything that gets through it.
// The health bar visibly going the wrong way is the least subtle cue in the fight and is meant to be:
// this is where the player is supposed to work out that they are doing it wrong.
EncScript_Manaphy_Band_5:
	call EncScript_Manaphy_BondStance
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encjumpifvar CMP_EQUAL, 2, 5, EncScript_Manaphy_Band_Done
	encsetvar 2, 5
	encsetvar 13, 0
	printstring STRINGID_ENCMANAPHYBAND5
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAINBOW
	return

// HEARTBROKEN. The window, and the spec's best idea: it hits far harder, moves far faster and stops
// defending itself. Survivable rather than lethal only because severing wipes its accumulated stages
// on the way in - the fight hands the player a clean slate at the same moment it hands them the
// danger.
EncScript_Manaphy_Band_Broken:
	call EncScript_Manaphy_NeutralStance
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPEED, 2
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encjumpifvar CMP_EQUAL, 2, 0, EncScript_Manaphy_Band_Done
	encsetvar 2, 0
	encsetvar 13, 0
	printstring STRINGID_ENCMANAPHYBANDBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// OCEAN'S HEART. The guard stops listening to the Bond and is pinned at 90 whatever the meter says -
// the network holds it up, not the bond with the player. So severing in this phase buys nothing on
// its own, which is what makes the collapse the only move.
EncScript_Manaphy_Band_Network:
	call EncScript_Manaphy_BondStance
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encjumpifvar CMP_EQUAL, 2, 6, EncScript_Manaphy_Band_Done
	encsetvar 2, 6
	encsetvar 13, 0
	printstring STRINGID_ENCMANAPHYBANDNETWORK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	return

// SHATTERED. Guard 60, no rain, no sharing, no stat stages, at 20% health. The kill window, and the
// spec's own counterplay: break the primary bond and the entire network comes apart with it.
EncScript_Manaphy_Band_Shattered:
	call EncScript_Manaphy_NeutralStance
	encsetdamagereduction ENC_TARGET_BOSS, 74
	encjumpifvar CMP_EQUAL, 2, 7, EncScript_Manaphy_Band_Done
	encsetvar 2, 7
	encsetvar 13, 0
	printstring STRINGID_ENCMANAPHYBANDSHATTERED
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
EncScript_Manaphy_Band_Done:
	return

// A byte var running past 5, or underflowing 0 to 255, would land in a band that does not exist.
// CMP_GREATER_THAN catches both ends.
EncScript_Manaphy_ClampBond:
	encjumpifvar CMP_GREATER_THAN, 1, 5, EncScript_Manaphy_ClampBond_High
	return
EncScript_Manaphy_ClampBond_High:
	encsetvar 1, 5      @ Bond
	return

// THE ECHO - the party as the network. Applied to the Pokemon that has just arrived, so severing a
// deep bond is paid for by the bench before it has taken a single action. Bounded at 15% on purpose:
// encchangehp faints a battler it takes to 0, and one-shotting the Pokemon sent in to save the player
// would read as a bug rather than a cost.
// The lesson is legible from the first lap - a shallow bond is nearly free, a Bond-5 bond costs a
// sixth of a Pokemon's health - which means the right time to sever is EARLY, and severing early
// means fewer turns of damage per lap. That tension is the fight.
EncScript_Manaphy_Echo:
	encjumpifvar CMP_LESS_THAN, 5, 2, EncScript_Manaphy_Echo_Done       @ Echo <= 1: it barely knew you
	encjumpifvar CMP_GREATER_THAN, 5, 3, EncScript_Manaphy_Echo_Hard
	printstring STRINGID_ENCMANAPHYECHOSOFT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -8, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -1
	return
EncScript_Manaphy_Echo_Hard:
	printstring STRINGID_ENCMANAPHYECHOHARD
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encchangehp ENC_TARGET_PLAYER_LEFT, -15, ENC_AMOUNT_PERCENT
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPDEF, -1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPEED, -1
EncScript_Manaphy_Echo_Done:
	return

// TIDAL BOND's heal. At Bond 4+ this is more than a 92%-guarded Manaphy loses to almost any attack,
// which is the point. Not while heartbroken - a grieving Manaphy is not drawing on the sea - and not
// once the network has shattered, which took the rain with it.
// Lives inside TurnClose rather than in a trigger of its own: a level trigger whose conditions do not
// change when it runs would re-select until the checkpoint cap asserted.
EncScript_Manaphy_TidalDraw:
	encjumpifvar CMP_LESS_THAN, 0, 1, EncScript_Manaphy_TidalDraw_Done    @ Phase 0: no rain yet
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Manaphy_TidalDraw_Done       @ Collapsed
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Manaphy_TidalDraw_Done        @ TideBroken
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Manaphy_TidalDraw_Done    @ Bond < 4
	printstring STRINGID_ENCMANAPHYDRAWS
	waitmessage B_WAIT_TIME_SHORT
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
EncScript_Manaphy_TidalDraw_Done:
	return

// SwapClock is armed at Bond 3 and ticked here; the OnTurnEnd trigger fires it at 1 and re-arms it.
// The clock is left alone at 1 so that trigger is the only thing that can consume it.
EncScript_Manaphy_SwapTick:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Manaphy_SwapTick_Done     @ Bond < 3: nothing to give
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Manaphy_SwapTick_Arm
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Manaphy_SwapTick_Done
	encsubvar 7, 1      @ SwapClock
	return
EncScript_Manaphy_SwapTick_Arm:
	encsetvar 7, 3
EncScript_Manaphy_SwapTick_Done:
	return

// Ticked at OnTurnStart and resolved at OnTurnEnd, two distinct checkpoints, so a tick can never eat
// a step off a clock armed the same turn. Left alone at 1 for EncScript_Manaphy_TideReturn.
EncScript_Manaphy_TideTick:
	encjumpifvar CMP_NOT_EQUAL, 9, 1, EncScript_Manaphy_TideTick_Done     @ TideBroken
	encjumpifvar CMP_LESS_THAN, 8, 2, EncScript_Manaphy_TideTick_Done
	encsubvar 8, 1      @ TideClock
EncScript_Manaphy_TideTick_Done:
	return

// THE RECURRING CHANNEL. The band transitions announce a change; this is what a player who missed one
// eventually reads. One line, every other turn, and only while the state is not neutral: the most
// urgent live state speaks and the rest stay quiet.
EncScript_Manaphy_Standing:
	encjumpifvar CMP_NOT_EQUAL, 13, 0, EncScript_Manaphy_Standing_Done    @ Pulse: every other turn
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Manaphy_Standing_Broken
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Manaphy_Standing_Bond
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Manaphy_Standing_Bond        @ Collapsed: the network is gone
	printstring STRINGID_ENCMANAPHYSTANDINGNETWORK
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Manaphy_Standing_Broken:
	printstring STRINGID_ENCMANAPHYSTANDINGBROKEN
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Manaphy_Standing_Bond:
	encjumpifvar CMP_LESS_THAN, 1, 3, EncScript_Manaphy_Standing_Done
	printstring STRINGID_ENCMANAPHYSTANDINGBOND
	waitmessage B_WAIT_TIME_SHORT
EncScript_Manaphy_Standing_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start. Manaphy has already bonded by the time the player sees
// it, so Bond and the network count both start at 1, and LastBand starts at 255 so the first
// ApplyBond announces whatever band it lands in.
EncScript_Manaphy_Intro::
	printstring STRINGID_ENCMANAPHYINTRO1
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCMANAPHYINTRO2
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 1      @ Bond
	encsetvar 3, 1      @ Bonds
	encsetvar 2, 255    @ LastBand: force the first announcement
	call EncScript_Manaphy_ApplyBond
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set and
// ShareGuard cleared immediately rather than at the tail, so no branch out of this script can leave
// one wrong.
EncScript_Manaphy_TurnOpen::
	flushtextbox
	encsetvar 6, 1      @ TurnGuard
	encsetvar 11, 0     @ ShareGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Manaphy_TurnOpen_Done   @ Weakened: inert
	@ Pulse alternates 1, 0, 1, 0 - the turn parity the standing line paces itself against. It belongs
	@ to the turn, not to the meter; only the band callout may reset it.
	encaddvar 13, 1
	encjumpifvar CMP_LESS_THAN, 13, 2, EncScript_Manaphy_TurnOpen_Mark
	encsetvar 13, 0
EncScript_Manaphy_TurnOpen_Mark:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Manaphy_TurnOpen_Ticks
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_SET     @ BossHpMark, for the sharing tick
EncScript_Manaphy_TurnOpen_Ticks:
	call EncScript_Manaphy_SwapTick
	call EncScript_Manaphy_TideTick
	call EncScript_Manaphy_Standing
EncScript_Manaphy_TurnOpen_Done:
	return

// SEVERING - the loop, and the only thing in the fight that moves the meter down. Heartbreak is
// written first: it is this trigger's self-disabling condition, and it also means switching AGAIN
// during a Heartbreak is free - no second echo, no reset of the window.
// Manaphy's stat stages are wiped by the Heartbroken band ApplyBond lands on; that is the second,
// quieter reward for severing.
EncScript_Manaphy_Sever::
	enccopyvar 5, 1     @ Echo = Bond: how deep the bond was when it broke
	encsetvar 1, 0      @ Bond
	encsetvar 4, 3      @ Heartbreak
	encsetvar 7, 0      @ SwapClock: a grieving Manaphy gives nothing
	encaddvar 3, 1      @ Bonds
	encjumpifvar CMP_LESS_THAN, 3, 6, EncScript_Manaphy_Sever_Body
	encsetvar 3, 5      @ capped, so the Phase 2 ladder has a bounded top
EncScript_Manaphy_Sever_Body:
	printstring STRINGID_ENCMANAPHYSEVER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Manaphy_Echo
	call EncScript_Manaphy_ApplyBond
	return

// THE COLLAPSE. One switch, once, in Ocean's Heart, and the whole network comes apart: the rain goes,
// the sharing stops, every stage is wiped and the guard falls to 60 for four turns. It also asks the
// question the fight has been building to - is there still a Pokemon to switch to? A player who spent
// their bench on the sever loop may not, and will have to grind Manaphy down at guard 90 while it
// hands their own damage back to them. The bill was always coming.
EncScript_Manaphy_Collapse::
	encsetvar 12, 1     @ Collapsed
	encsetvar 3, 0      @ Bonds: the network count is what shattered
	encsetvar 1, 0      @ Bond
	encsetvar 4, 4      @ Heartbreak: one turn longer than an ordinary sever
	encsetvar 7, 0      @ SwapClock
	encsetvar 8, 0      @ TideClock
	encsetvar 9, 0      @ TideBroken
	removeweather
	printstring STRINGID_ENCMANAPHYCOLLAPSE1
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	printstring STRINGID_ENCMANAPHYCOLLAPSE2
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Manaphy_ApplyBond
	return

// The weather lever. TideClock 4 rather than 3 because it is ticked at OnTurnStart and resolved at
// OnTurnEnd: 4 buys the player the turn the weather changed plus two clean ones after it.
// The Sp. Def drop is not written here - EncScript_Manaphy_BondStance reads TideBroken and applies
// it, so the next ApplyBond cannot wipe it.
EncScript_Manaphy_TideBreak::
	encsetvar 9, 1      @ TideBroken
	encsetvar 8, 4      @ TideClock
	printstring STRINGID_ENCMANAPHYTIDEBROKEN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	call EncScript_Manaphy_ApplyBond
	return

EncScript_Manaphy_TideReturn::
	encsetvar 9, 0      @ TideBroken
	encsetvar 8, 0      @ TideClock
	removeweather
	encsetweather BATTLE_WEATHER_RAIN, 0
	printstring STRINGID_ENCMANAPHYTIDERETURN
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	call EncScript_Manaphy_ApplyBond
	return

// HEART SWAP - the poisoned gift, and the loudest thing in the fight. Both halves are real: Manaphy
// genuinely loses 8% and the player genuinely gains a quarter of their health, a cure and two stat
// stages. At Bond 3 that is a straightforwardly good trade for them.
// The only cost is the extra Bond step below, which is invisible, and which is why by the fourth swap
// the player is at Bond 5 watching their attacks do nothing at all. TurnClose adds the other step.
// The re-arm shortens as the bond deepens - the closer it is, the more often it helps.
EncScript_Manaphy_HeartSwap::
	printstring STRINGID_ENCMANAPHYSWAPGIVE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	encchangehp ENC_TARGET_PLAYER_LEFT, 25, ENC_AMOUNT_PERCENT
	curestatus BS_PLAYER1
	updatestatusicon BS_PLAYER1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_ATK, 1
	encchangestat ENC_TARGET_PLAYER_LEFT, STAT_SPATK, 1
	printstring STRINGID_ENCMANAPHYSWAPTAKE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_LEECH_SEED_DRAIN
	encchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	encaddvar 1, 1      @ Bond: the extra step
	call EncScript_Manaphy_ClampBond
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Manaphy_HeartSwap_Fast
	encsetvar 7, 4      @ SwapClock: every third turn at Bond 3
	return
EncScript_Manaphy_HeartSwap_Fast:
	encsetvar 7, 3      @ every second turn from Bond 4
	return

// OCEAN'S HEART's damage sharing. encsnapshothp rewrites BossHpMark as "percentage of max HP lost
// since the turn opened" and jumps its fail label when that is zero, so one command is both the test
// and the measurement.
// Two columns, because a byte var cannot be divided: Bonds is the count of bonds Manaphy has formed
// over the whole battle. A player who never switched barely notices; a player who ran four laps of
// the sever loop takes a sixth of their health back every time they land a real hit. THAT IS THE BILL
// FOR THE FIRST TWO PHASES ARRIVING AT ONCE, and it is the only place the count is ever used.
EncScript_Manaphy_OceanShare::
	encsetvar 11, 1     @ ShareGuard
	encsnapshothp ENC_TARGET_BOSS, 10, ENC_SNAP_DAMAGE, EncScript_Manaphy_OceanShare_Done
	encjumpifvar CMP_LESS_THAN, 10, 3, EncScript_Manaphy_OceanShare_Done
	printstring STRINGID_ENCMANAPHYSHARE
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Manaphy_OceanShare_Deep
	encjumpifvar CMP_GREATER_THAN, 10, 14, EncScript_Manaphy_OceanShare_Wide
	encjumpifvar CMP_GREATER_THAN, 10, 7, EncScript_Manaphy_OceanShare_Mid
	encchangehp ENC_TARGET_PLAYER_LEFT, -3, ENC_AMOUNT_PERCENT
	return
EncScript_Manaphy_OceanShare_Mid:
	encchangehp ENC_TARGET_PLAYER_LEFT, -6, ENC_AMOUNT_PERCENT
	return
EncScript_Manaphy_OceanShare_Wide:
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Manaphy_OceanShare_Deep:
	encjumpifvar CMP_GREATER_THAN, 10, 14, EncScript_Manaphy_OceanShare_DeepWide
	encjumpifvar CMP_GREATER_THAN, 10, 7, EncScript_Manaphy_OceanShare_DeepMid
	encchangehp ENC_TARGET_PLAYER_LEFT, -5, ENC_AMOUNT_PERCENT
	return
EncScript_Manaphy_OceanShare_DeepMid:
	encchangehp ENC_TARGET_PLAYER_LEFT, -10, ENC_AMOUNT_PERCENT
	return
EncScript_Manaphy_OceanShare_DeepWide:
	encchangehp ENC_TARGET_PLAYER_LEFT, -16, ENC_AMOUNT_PERCENT
EncScript_Manaphy_OceanShare_Done:
	return

// THE METER, and the sole owner of it. Heartbroken pins Bond at 0 and ticks the window down instead;
// when the window closes Manaphy re-bonds at 1 and the climb starts over. Otherwise the meter climbs
// by itself - twice as fast while Manaphy's rain is standing, which is what makes Phase 1 a clock.
EncScript_Manaphy_TurnClose::
	encsetvar 6, 0      @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Manaphy_TurnClose_Done   @ Weakened: inert
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Manaphy_TurnClose_Grief  @ Heartbreak
	call EncScript_Manaphy_TidalDraw
	encaddvar 1, 1      @ Bond
	encjumpifvar CMP_LESS_THAN, 0, 1, EncScript_Manaphy_TurnClose_Apply     @ Phase 0: one step
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Manaphy_TurnClose_Apply        @ Collapsed: no rain left
	encjumpifvar CMP_EQUAL, 9, 1, EncScript_Manaphy_TurnClose_Apply         @ TideBroken
	encaddvar 1, 1      @ the rain doubles the climb
EncScript_Manaphy_TurnClose_Apply:
	call EncScript_Manaphy_ClampBond
	call EncScript_Manaphy_ApplyBond
	return
EncScript_Manaphy_TurnClose_Grief:
	encsubvar 4, 1      @ Heartbreak
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Manaphy_TurnClose_Done
	encsetvar 1, 1      @ Bond: it finds them again
	printstring STRINGID_ENCMANAPHYREBOND
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Manaphy_ApplyBond
EncScript_Manaphy_TurnClose_Done:
	return

// --- Phases ---

// 50% - HEART OF THE OCEAN. TIDAL BOND: permanent rain, which does four things at once. The Bond
// climbs two steps a turn instead of one; Manaphy heals 3% a turn at Bond 4+; its own HYDRATION
// starts scrubbing status off it every turn; and Surf gets its rain boost. The last two are native
// and free, which is the whole anti-abuse answer for this fight - status works completely for the
// first half of the battle and the player has to break the weather to make it stick again.
EncScript_Manaphy_HeartOfTheOcean::
	encsetvar 0, 1      @ Phase
	printstring STRINGID_ENCMANAPHYPHASE1
	waitmessage B_WAIT_TIME_LONG
	removeweather
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	printstring STRINGID_ENCMANAPHYTIDALBOND
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Manaphy_ApplyBond
	return

// 20% - OCEAN'S HEART. The guard is pinned at 90 regardless of the meter and everything dealt to
// Manaphy comes back at the player, scaled by how many of their Pokemon it has held. Severing alone
// buys nothing here; EncScript_Manaphy_Collapse is the only move.
EncScript_Manaphy_OceansHeart::
	encsetvar 0, 2      @ Phase
	printstring STRINGID_ENCMANAPHYOCEANSHEART
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	call EncScript_Manaphy_ApplyBond
	return

// 10% - the catch window, the mirror image of the Properties: block plus everything the fight built.
// Phase 3 retires every remaining trigger; all of them are gated Phase <= 2. From here the engine's
// automatic catch-window damage guard owns the reduction - encsetdamagereduction is setting the value
// that guard will restore, not the live one. The status is cured because a Toxic standing on a boss
// that no longer has Survive: True is the one thing that could still kill the catch target.
// THE CATCH RATE IS READ OFF THE FINAL BOND, and it is the last real decision of the fight. Burst
// Manaphy down inside a Heartbreak window and it falls at Bond 0 - the easy kill, the hard catch. Let
// it re-bond first and finish it at Bond 4+ - much harder to do, and it comes almost willingly. The
// window itself is identical on every path; what the Bond buys is how many balls it takes.
EncScript_Manaphy_Weakened::
	encsetvar 0, 3      @ Phase
	call EncScript_Manaphy_NeutralStance
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	removeweather
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCMANAPHYWEAKENED
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Manaphy_Weakened_Close
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Manaphy_Weakened_Watching
	encsetcatchrate 30
	printstring STRINGID_ENCMANAPHYCATCHDISTANT
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Manaphy_Weakened_Clear
EncScript_Manaphy_Weakened_Watching:
	printstring STRINGID_ENCMANAPHYCATCHWATCHING
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Manaphy_Weakened_Clear
EncScript_Manaphy_Weakened_Close:
	printstring STRINGID_ENCMANAPHYCATCHCLOSE
	waitmessage B_WAIT_TIME_LONG
EncScript_Manaphy_Weakened_Clear:
	encsetvar 1, 0      @ Bond
	encsetvar 2, 0      @ LastBand
	encsetvar 3, 0      @ Bonds
	encsetvar 4, 0      @ Heartbreak
	encsetvar 7, 0      @ SwapClock
	encsetvar 8, 0      @ TideClock
	encsetvar 9, 0      @ TideBroken
	encsetvar 13, 0     @ Pulse
	return
