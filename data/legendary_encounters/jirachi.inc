// Jirachi, "The Wish Pokemon". Var indices, pinned by the
// always-true Conditions on EncScript_Jirachi_Intro:
// 0 Phase (0 Awakening / 1 after wish 1 / 2 after wish 2 / 3 Miracle / 4 Wish Granted),
// 1 Wishes (0-3, wishes spent), 2 Energy (Wish Energy, 0-3), 3 Heart (the read on the player,
// 1-15, starts 8), 4 GuardBias (guard ladder rung, 0-4, starts 2), 5 Miracle (0 none / 1 Power /
// 2 Life / 3 Stars), 6 LastGuard (reduction last ANNOUNCED, starts 90), 7 LastTier (Heart tier last
// announced: 0 will / 1 balance / 2 force, starts 3 = never announced), 8 TurnGuard, 9 MoveGuard
// (per-turn OnMoveEnd re-entry guard for the Heart tally), 10 Prev (ApplyGuard's before-value
// scratch), 11 Scratch.
//
// Jirachi reacts to HOW the player fights rather than to what they did to it. Every turn it gathers
// Wish Energy; at full energy it makes a wish, and WHICH wish is decided by the Heart meter - a
// hidden read of the player moved up by attacking and down by using status moves. Every wish costs
// it something, and then it asks whether the player wants the same boon: YES shares the wish and it
// pays nothing, NO makes it pay in full. It gets three wishes. The third is the Miracle, the Heart
// picks that one too, and surviving it puts Jirachi in the catch window.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. A live miracle overrides the ladder outright,
// which is why the miracle leaves are tested first. The ladder itself is symmetric around the
// opening 90: Peace wishes push it up (harder to reach), declined Power wishes pull it down.
// Prev holds the value last announced so each leaf can write LastGuard before comparing; there is
// no UI for damage reduction, so the line has to re-fire on every real change.
EncScript_Jirachi_ApplyGuard:
	enccopyvar 10, 6
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Jirachi_Guard80   @ Miracle of Power: a glass cannon
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Jirachi_Guard92   @ Miracle of Life: a wall
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Jirachi_Guard86   @ Miracle of Stars: a clock
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Jirachi_Guard84
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Jirachi_Guard88
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Jirachi_Guard92
	encjumpifvar CMP_GREATER_THAN, 4, 3, EncScript_Jirachi_Guard94
	goto EncScript_Jirachi_Guard90                            @ GuardBias 2, the opening rung

EncScript_Jirachi_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 6, 80
	encjumpifvar CMP_EQUAL, 10, 80, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 80, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 6, 84
	encjumpifvar CMP_EQUAL, 10, 84, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 84, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 6, 86
	encjumpifvar CMP_EQUAL, 10, 86, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 86, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 6, 88
	encjumpifvar CMP_EQUAL, 10, 88, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 88, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 6, 90
	encjumpifvar CMP_EQUAL, 10, 90, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 90, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 6, 92
	encjumpifvar CMP_EQUAL, 10, 92, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 92, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall
EncScript_Jirachi_Guard94:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encsetvar 6, 94
	encjumpifvar CMP_EQUAL, 10, 94, EncScript_Jirachi_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 10, 94, EncScript_Jirachi_ApplyGuard_Rise
	goto EncScript_Jirachi_ApplyGuard_Fall

EncScript_Jirachi_ApplyGuard_Rise:
	printstring STRINGID_ENCJIRACHIGUARDRISE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_ApplyGuard_Fall:
	printstring STRINGID_ENCJIRACHIGUARDFALL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_ApplyGuard_Done:
	return

// The player's only window onto the Heart meter - the number itself is never shown, only the colour
// of the tags. Latched on LastTier so it prints when the tier actually moves; LastTier starts on the
// out-of-range sentinel 3, so the opening evaluation always prints and the mechanic is on screen
// from turn one. This is the whole discovery mechanism: the colour answers the player's own play,
// and then the wish matches the colour.
EncScript_Jirachi_HeartTier:
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Jirachi_HeartTier_Done   @ Miracle locked in: silent
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Jirachi_HeartTier_Force
	encjumpifvar CMP_GREATER_THAN, 3, 5, EncScript_Jirachi_HeartTier_Balance
	encjumpifvar CMP_EQUAL, 7, 0, EncScript_Jirachi_HeartTier_Done
	encsetvar 7, 0
	printstring STRINGID_ENCJIRACHIHEARTWILL
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_HeartTier_Balance:
	encjumpifvar CMP_EQUAL, 7, 1, EncScript_Jirachi_HeartTier_Done
	encsetvar 7, 1
	printstring STRINGID_ENCJIRACHIHEARTBALANCE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_HeartTier_Force:
	encjumpifvar CMP_EQUAL, 7, 2, EncScript_Jirachi_HeartTier_Done
	encsetvar 7, 2
	printstring STRINGID_ENCJIRACHIHEARTFORCE
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_HeartTier_Done:
	return

// The wish clock, recurring rather than latched: energy is the one mechanic the player has to be
// able to count down every single turn. Silent at 0-1 so the early turns are quiet.
EncScript_Jirachi_EnergyCue:
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Jirachi_EnergyCue_Done   @ no wish left to promise
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Jirachi_EnergyCue_Ring
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Jirachi_EnergyCue_Done
	printstring STRINGID_ENCJIRACHITAGSCHIME
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Jirachi_EnergyCue_Ring:
	printstring STRINGID_ENCJIRACHITAGSRING
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_EnergyCue_Done:
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so this only has to write the four non-zero defaults.
// The opening HeartTier call puts the tag colour on screen before the player's first move, which is
// what makes the meter learnable rather than invisible.
EncScript_Jirachi_Intro::
	encsetvar 3, 8     @ Heart: dead centre of 1-15
	encsetvar 4, 2     @ GuardBias: the middle rung, matching the Properties reduction
	encsetvar 6, 90    @ LastGuard: seeded so the first real move reads as a change
	encsetvar 7, 3     @ LastTier: out-of-range sentinel, so the first evaluation always prints
	printstring STRINGID_ENCJIRACHIAWAKENS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	call EncScript_Jirachi_HeartTier
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Owns the per-turn guards,
// the Miracle of Stars chip clock and the recurring miracle line.
EncScript_Jirachi_TurnOpen::
	flushtextbox
	encsetvar 8, 1    @ TurnGuard
	encsetvar 9, 0    @ MoveGuard
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Jirachi_TurnOpen_Done   @ wishes spent: inert
	encjumpifvar CMP_EQUAL, 5, 3, EncScript_Jirachi_TurnOpen_Star
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Jirachi_TurnOpen_Burning
	goto EncScript_Jirachi_TurnOpen_Clings
	@ The star clock. Fires here rather than at the turn close so it lands BEFORE the player's
	@ action, and because gBattlerAttacker is deterministically the opponent at OnTurnStart, which is
	@ what makes playmoveanimation animate from Jirachi in the right direction.
	@ ALL_FOES rather than PLAYER_LEFT so the tick is doubles-safe and skips a fainted battler.
EncScript_Jirachi_TurnOpen_Star:
	printstring STRINGID_ENCJIRACHISTARFALLS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_SWIFT
	enchangehp ENC_TARGET_ALL_FOES, -10, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_TurnOpen_Done
EncScript_Jirachi_TurnOpen_Burning:
	printstring STRINGID_ENCJIRACHIMIRACLEBURNS
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Jirachi_TurnOpen_Done
	@ Survive is on until the Miracle has played, so a player who has already earned the kill needs
	@ telling why it isn't landing. Only at a sliver of HP, so it is a rare line rather than a drone.
EncScript_Jirachi_TurnOpen_Clings:
	encsnapshothp ENC_TARGET_BOSS, 11, ENC_SNAP_SET
	encjumpifvar CMP_GREATER_THAN, 11, 5, EncScript_Jirachi_TurnOpen_Done
	printstring STRINGID_ENCJIRACHICLINGS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Jirachi_TurnOpen_Done:
	return

// Wish Energy income, and the sleep inversion that is this encounter's anti-cheese. Jirachi is the
// Pokemon that sleeps for a thousand years: putting it to sleep does not shut it down, it makes it
// DREAM, and dreams are where wishes come from. Nothing is blocked and nothing is silently ignored -
// a player who reaches for the standard boss lock simply finds the Miracle on a two-turn clock, and
// can reverse it by waking it up. Energy is a plain turn counter besides, so Protect/recover
// stalling accelerates the fight into the Miracle rather than avoiding it.
EncScript_Jirachi_TurnClose::
	encsetvar 8, 0    @ TurnGuard
	encsetvar 9, 0    @ MoveGuard
	@ Phase 3 and 4 stop the clock entirely: the third wish is the last one, so an energy cue past
	@ that point would be promising a fourth that never arrives.
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Jirachi_TurnClose_Done
	encjumpifvar CMP_GREATER_THAN, 2, 2, EncScript_Jirachi_TurnClose_Cue
	jumpifstatus BS_OPPONENT1, STATUS1_SLEEP, EncScript_Jirachi_TurnClose_Dream
	encaddvar 2, 1
	goto EncScript_Jirachi_TurnClose_Cue
EncScript_Jirachi_TurnClose_Dream:
	printstring STRINGID_ENCJIRACHIDREAMS
	waitmessage B_WAIT_TIME_SHORT
	encaddvar 2, 2
	encjumpifvar CMP_LESS_THAN, 2, 4, EncScript_Jirachi_TurnClose_Cue
	encsetvar 2, 3
EncScript_Jirachi_TurnClose_Cue:
	call EncScript_Jirachi_EnergyCue
EncScript_Jirachi_TurnClose_Done:
	return

// The Heart tally. Jirachi reads INTENT, not results: a move that missed still counts, because
// swinging and missing is still swinging - and Event.OldValue/NewValue are unreliable after a miss
// anyway. MoveGuard makes a five-hit move worth one point rather than five. The clamps are explicit
// skips because encaddvar/encsubvar do not saturate.
EncScript_Jirachi_HeartForce::
	encsetvar 9, 1    @ MoveGuard
	encjumpifvar CMP_GREATER_THAN, 3, 14, EncScript_Jirachi_HeartForce_Tier
	encaddvar 3, 1
EncScript_Jirachi_HeartForce_Tier:
	call EncScript_Jirachi_HeartTier
	return

EncScript_Jirachi_HeartWill::
	encsetvar 9, 1    @ MoveGuard
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Jirachi_HeartWill_Tier
	encsubvar 3, 1
EncScript_Jirachi_HeartWill_Tier:
	call EncScript_Jirachi_HeartTier
	return

// 30% HP fills the meter outright, so whichever wish is next arrives within a turn of the player
// pushing Jirachi low rather than waiting out the clock. A hurt Jirachi wishes harder, and the
// Miracle lands as a climax rather than on a timer.
EncScript_Jirachi_TagsBlaze::
	encsetvar 2, 3    @ Energy
	printstring STRINGID_ENCJIRACHITAGSBLAZE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// Wishes one and two. The tier branch decides WHAT it wishes for; the prompt decides whether the
// player wishes alongside it. The right answer differs per wish, which is what stops the prompt
// collapsing into a default: refusing Peace or Wonder buys a whole free turn, refusing Power buys
// softened defences and a permanent rung off the guard ladder, and accepting anything hands the
// player the same boon at the price of a faster clock.
// Every player-facing effect targets ALL_FOES rather than PLAYER_LEFT. A group target drops a
// fainted battler; a single-slot one asserts on it - and the Wonder falling-star outcome can KO the
// player's active in the middle of this very script, before the prompt is even answered.
EncScript_Jirachi_Wish::
	flushtextbox
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Jirachi_Wish_BannerTwo
	printstring STRINGID_ENCJIRACHIWISHONE
	goto EncScript_Jirachi_Wish_Open
EncScript_Jirachi_Wish_BannerTwo:
	printstring STRINGID_ENCJIRACHIWISHTWO
EncScript_Jirachi_Wish_Open:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Jirachi_Wish_Power
	encjumpifvar CMP_GREATER_THAN, 3, 5, EncScript_Jirachi_Wish_Wonder
	@ WISH OF PEACE - the answer to a player who fights with status moves. It walls up, and the
	@ guard ladder climbs a rung whether or not the player wishes alongside it: the screens are the
	@ visible half of the boon, the ladder is the half they have to infer from the callout.
EncScript_Jirachi_Wish_Peace:
	printstring STRINGID_ENCJIRACHIWISHPEACE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT, 5
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN, 5
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 5
	encjumpifvar CMP_GREATER_THAN, 4, 3, EncScript_Jirachi_Wish_PeaceAsk
	encaddvar 4, 1
EncScript_Jirachi_Wish_PeaceAsk:
	encaskyesno STRINGID_ENCJIRACHILISTENING, EncScript_Jirachi_Wish_PeaceShared
	printstring STRINGID_ENCJIRACHIPEACECOST
	waitmessage B_WAIT_TIME_LONG
	encsetrecharge ENC_TARGET_BOSS, 1
	goto EncScript_Jirachi_Wish_Spend
EncScript_Jirachi_Wish_PeaceShared:
	printstring STRINGID_ENCJIRACHISHAREDPEACE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_WISH_HEAL
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_REFLECT, 5
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_LIGHT_SCREEN, 5
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SAFEGUARD, 5
	goto EncScript_Jirachi_Wish_Share
	@ WISH OF POWER - the answer to a player who only attacks. Refusing it is the aggressive line:
	@ its defences drop two stages AND the guard ladder loses a rung for good.
EncScript_Jirachi_Wish_Power:
	printstring STRINGID_ENCJIRACHIWISHPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 2
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 2
	encaskyesno STRINGID_ENCJIRACHILISTENING, EncScript_Jirachi_Wish_PowerShared
	printstring STRINGID_ENCJIRACHIPOWERCOST
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_DEF, -1
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, -1
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Jirachi_Wish_Spend
	encsubvar 4, 1
	goto EncScript_Jirachi_Wish_Spend
EncScript_Jirachi_Wish_PowerShared:
	printstring STRINGID_ENCJIRACHISHAREDPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	encchangestat ENC_TARGET_ALL_FOES, STAT_ATK, 2
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPATK, 2
	goto EncScript_Jirachi_Wish_Share
	@ WISH OF WONDER - the answer to a player who does a bit of everything, and the only wish whose
	@ payload is a roll. Uniform 1-in-4 across the four outcomes (25 / 33 / 50 / fall-through).
EncScript_Jirachi_Wish_Wonder:
	printstring STRINGID_ENCJIRACHIWISHWONDER
	waitmessage B_WAIT_TIME_LONG
	encjumpifchance 25, EncScript_Jirachi_Wonder_Strength
	encjumpifchance 33, EncScript_Jirachi_Wonder_Star
	encjumpifchance 50, EncScript_Jirachi_Wonder_Still
	@ It gains nothing at all. One beat in the fight is purely a gift, and this is it.
	printstring STRINGID_ENCJIRACHIWONDERPLAY
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	enchangehp ENC_TARGET_ALL_FOES, 25, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wonder_Ask
EncScript_Jirachi_Wonder_Strength:
	printstring STRINGID_ENCJIRACHIWONDERSTRENGTH
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wonder_Ask
EncScript_Jirachi_Wonder_Star:
	printstring STRINGID_ENCJIRACHIWONDERSTAR
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 4
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	playmoveanimation MOVE_SWIFT
	enchangehp ENC_TARGET_ALL_FOES, -12, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wonder_Ask
	@ Cuts both ways - it wipes the player's setup AND Jirachi's own boosts - which is why this
	@ outcome needs no drawback of its own.
EncScript_Jirachi_Wonder_Still:
	printstring STRINGID_ENCJIRACHIWONDERSTILL
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TRICK_ROOM
	normalisebuffs
EncScript_Jirachi_Wonder_Ask:
	encaskyesno STRINGID_ENCJIRACHILISTENING, EncScript_Jirachi_Wish_WonderShared
	printstring STRINGID_ENCJIRACHIWONDERCOST
	waitmessage B_WAIT_TIME_LONG
	encsetrecharge ENC_TARGET_BOSS, 1
	goto EncScript_Jirachi_Wish_Spend
EncScript_Jirachi_Wish_WonderShared:
	printstring STRINGID_ENCJIRACHISHAREDWONDER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_SIMPLE_HEAL
	enchangehp ENC_TARGET_ALL_FOES, 25, ENC_AMOUNT_PERCENT
	goto EncScript_Jirachi_Wish_Share
	@ Refused: the meter empties, so the next wish is a full three turns away.
EncScript_Jirachi_Wish_Spend:
	encsetvar 2, 0
	goto EncScript_Jirachi_Wish_Advance
	@ Shared: the wish cost it nothing and it comes round a turn sooner. The price the player will
	@ not see coming on a first run - accept all three and the Miracle arrives early, against a
	@ Jirachi that never paid for any of them.
EncScript_Jirachi_Wish_Share:
	encsetvar 2, 1
	@ Both spend paths write Energy to a value this trigger's own condition rejects, so it cannot
	@ re-select inside one dispatch. Phase tracks wishes spent, which is what opens the Miracle.
EncScript_Jirachi_Wish_Advance:
	encaddvar 1, 1
	enccopyvar 0, 1
	call EncScript_Jirachi_ApplyGuard
	return

// The third wish, and Jirachi's alone - no prompt. Taking the choice away at the climax is the
// point. The Heart is read one last time, so the fight the player brought is the fight they get:
// attackers race a glass cannon, stallers have to break a wall, and everyone else runs a clock.
EncScript_Jirachi_Miracle::
	flushtextbox
	printstring STRINGID_ENCJIRACHIFINALWISH
	waitmessage B_WAIT_TIME_LONG
	encsetvar 0, 3    @ Phase 3
	encsetvar 1, 3    @ Wishes
	encsetvar 2, 0    @ Energy: this trigger's own gate, closed before anything else can run
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Jirachi_Miracle_Power
	encjumpifvar CMP_GREATER_THAN, 3, 5, EncScript_Jirachi_Miracle_Stars
	@ MIRACLE OF LIFE. The staller's own game handed back to them, and the answer is the setup they
	@ have spent the fight accumulating and now finally have to cash in.
	printstring STRINGID_ENCJIRACHIMIRACLELIFE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_WISH_HEAL
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_REFLECT, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_LIGHT_SCREEN, 0
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_SAFEGUARD, 0
	encsetvar 5, 2
	goto EncScript_Jirachi_Miracle_Done
	@ MIRACLE OF POWER. A race - it can end the player in two moves, they can end it in three.
EncScript_Jirachi_Miracle_Power:
	printstring STRINGID_ENCJIRACHIMIRACLEPOWER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encchangestatvalue ENC_TARGET_BOSS, STAT_ATK, 50, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 50, ENC_AMOUNT_PERCENT
	encsetvar 5, 1
	goto EncScript_Jirachi_Miracle_Done
	@ MIRACLE OF STARS. The softest guard of the three, permanent Tailwind, and a star falling on the
	@ player every turn from here (TurnOpen). Finish it or lose.
EncScript_Jirachi_Miracle_Stars:
	printstring STRINGID_ENCJIRACHIMIRACLESTARS
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encsetvar 5, 3
EncScript_Jirachi_Miracle_Done:
	call EncScript_Jirachi_ApplyGuard
	return

// Survive the Miracle and the tags go dark: every encounter rule comes off and the fight becomes an
// ordinary Pokemon battle. Clearing Miracle is load-bearing rather than tidiness - it is what stops
// the star clock chipping the player's Pokemon down while they are trying to land a ball.
// From here the engine's automatic catch-window damage guard keeps the catch target alive.
EncScript_Jirachi_WishGranted::
	encsetvar 0, 4    @ Phase 4
	encsetvar 2, 0    @ Energy
	encsetvar 5, 0    @ Miracle: stops the star clock and every recurring line
	encclearscreens ENC_TARGET_BOSS, EncScript_Jirachi_WishGranted_Release
EncScript_Jirachi_WishGranted_Release:
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	@ The live reduction and the LastGuard latch are the same state; a mismatch would make the next
	@ callout announce a move that never happened.
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 6, 0
	encsetcatchrate 40
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCJIRACHIWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
