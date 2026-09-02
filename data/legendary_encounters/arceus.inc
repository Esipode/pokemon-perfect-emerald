// Arceus, "The Original One". Var indices, pinned by the always-true Conditions on
// EncScript_Arceus_Intro:
// 0 Phase (0 The Creator / 1 The Plates / 2 The Creator's Judgment / 3 Creation / 4 The Original
// One / 5 The Final Moment / 6 Weakened), 1 Plate (0 none, 1-6 ACTIVE, 11-16 TELEGRAPHED),
// 2 PlateTimer, 3 Authority (0-12, the spine), 4 LastAuth (tier last ANNOUNCED - the re-fire
// latch), 5 Judge (0 none / 1 Marked / 2 Condemned / 3 falls at this turn's end), 6 Decree (0 none,
// 1-5), 7 DecreeTimer, 8 Phys / 9 Spec (Divine Observation; Spec becomes the stance flag after the
// Creation read), 10 Shift (per-turn one-move-driven-change guard), 11 Seen (per-turn Observation
// guard), 12 HpMark (boss HP% mark at turn open, for the disrupt tests), 13 Charge (Phase 4 finale
// clock), 14 Breaks (0 submitted, 10-15 the ladder), 15 TurnGuard.
//
// Every other legendary has one idea. Arceus is a rulebook that keeps rewriting itself, and each
// phase tests a different skill. Underneath all of it is AUTHORITY: Arceus regains 1 a turn and
// spends it to act, and it can never act on credit. Everything the player does to push it down is
// settled at one moment - the finale only offers the CHALLENGE option at Authority 3 or less.

// --- Authority: the spine ---

// Every path that moves Authority routes through these, so every change is bounded and announced.
// encaddvar has no ceiling and encsubvar underflows a u8 to 255, so both ends are tested first.
EncScript_Arceus_AuthGain1:
	encjumpifvar CMP_GREATER_THAN, 3, 11, EncScript_Arceus_AuthApply
	encaddvar 3, 1
	goto EncScript_Arceus_AuthApply

EncScript_Arceus_AuthGain2:
	encjumpifvar CMP_GREATER_THAN, 3, 10, EncScript_Arceus_AuthGain2_Cap
	encaddvar 3, 2
	goto EncScript_Arceus_AuthApply
EncScript_Arceus_AuthGain2_Cap:
	encsetvar 3, 12
	goto EncScript_Arceus_AuthApply

EncScript_Arceus_AuthDrain1:
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Arceus_AuthApply
	encsubvar 3, 1
	goto EncScript_Arceus_AuthApply

EncScript_Arceus_AuthDrain2:
	encjumpifvar CMP_LESS_THAN, 3, 2, EncScript_Arceus_AuthDrain_Floor
	encsubvar 3, 2
	goto EncScript_Arceus_AuthApply

EncScript_Arceus_AuthDrain5:
	encjumpifvar CMP_LESS_THAN, 3, 5, EncScript_Arceus_AuthDrain_Floor
	encsubvar 3, 5
	goto EncScript_Arceus_AuthApply

EncScript_Arceus_AuthDrain_Floor:
	encsetvar 3, 0
EncScript_Arceus_AuthApply:
	call EncScript_Arceus_AuthorityCue
	return

// The tier callout, latched on the tier last ANNOUNCED rather than on the raw value, so it fires on
// every real change in either direction and never drones on an unchanged tier. LastAuth holds the
// tier plus one, so the zeroed initial value can never match and the opening tier is always stated.
EncScript_Arceus_AuthorityCue:
	encjumpifvar CMP_GREATER_THAN, 3, 11, EncScript_Arceus_AuthCue_Three
	encjumpifvar CMP_GREATER_THAN, 3, 7, EncScript_Arceus_AuthCue_Two
	encjumpifvar CMP_GREATER_THAN, 3, 3, EncScript_Arceus_AuthCue_One
	encjumpifvar CMP_EQUAL, 4, 1, EncScript_Arceus_AuthCue_Done
	encsetvar 4, 1
	printstring STRINGID_ENCARCEUSAUTHORITYZERO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Arceus_AuthCue_One:
	encjumpifvar CMP_EQUAL, 4, 2, EncScript_Arceus_AuthCue_Done
	encsetvar 4, 2
	printstring STRINGID_ENCARCEUSAUTHORITYONE
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Arceus_AuthCue_Two:
	encjumpifvar CMP_EQUAL, 4, 3, EncScript_Arceus_AuthCue_Done
	encsetvar 4, 3
	printstring STRINGID_ENCARCEUSAUTHORITYTWO
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Arceus_AuthCue_Three:
	encjumpifvar CMP_EQUAL, 4, 4, EncScript_Arceus_AuthCue_Done
	encsetvar 4, 4
	printstring STRINGID_ENCARCEUSAUTHORITYTHREE
	waitmessage B_WAIT_TIME_LONG
EncScript_Arceus_AuthCue_Done:
	return

// Printed whenever Arceus is refused an action it cannot afford. This is the player's proof the
// meter is real and that pushing it down does something.
EncScript_Arceus_Wavers:
	printstring STRINGID_ENCARCEUSAUTHORITYWAVERS
	waitmessage B_WAIT_TIME_SHORT
	return

// --- The Plates ---

// Tears down whatever law is up, without needing to know which one it was. Every Plate's field
// effects are cleared unconditionally, so a rotation that runs 1-4-2-6 leaves no residue and no
// Plate has to remember how to undo itself.
EncScript_Arceus_ClearLaw:
	removeweather
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE
	encclearsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_AURORA_VEIL
	encclearsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST
	return

// Picks the next Plate and writes it TELEGRAPHED (1-6 plus the 10 offset). Weighted to a flat
// 1-in-6; a roll that lands on the Plate already up steps to the next one instead, so the rotation
// never stalls on a repeat. Plate can only equal one value, so at most one step ever happens and
// the chain cannot loop.
EncScript_Arceus_PickPlate:
	encjumpifchance 17, EncScript_Arceus_Pick_One
	encjumpifchance 20, EncScript_Arceus_Pick_Two
	encjumpifchance 25, EncScript_Arceus_Pick_Three
	encjumpifchance 33, EncScript_Arceus_Pick_Four
	encjumpifchance 50, EncScript_Arceus_Pick_Five
EncScript_Arceus_Pick_Six:
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Arceus_Pick_One
	encsetvar 1, 16
	return
EncScript_Arceus_Pick_One:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Arceus_Pick_Two
	encsetvar 1, 11
	return
EncScript_Arceus_Pick_Two:
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Arceus_Pick_Three
	encsetvar 1, 12
	return
EncScript_Arceus_Pick_Three:
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Arceus_Pick_Four
	encsetvar 1, 13
	return
EncScript_Arceus_Pick_Four:
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Arceus_Pick_Five
	encsetvar 1, 14
	return
EncScript_Arceus_Pick_Five:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Arceus_Pick_Six
	encsetvar 1, 15
	return

// Names the Plate that is coming, one action ahead of it landing. The player gets the announcement
// before choosing this turn's move, and the Plate itself arrives at this turn's end.
EncScript_Arceus_Telegraph:
	printstring STRINGID_ENCARCEUSPLATETURNS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	encjumpifvar CMP_EQUAL, 1, 16, EncScript_Arceus_Telegraph_Fist
	encjumpifvar CMP_EQUAL, 1, 15, EncScript_Arceus_Telegraph_Icicle
	encjumpifvar CMP_EQUAL, 1, 14, EncScript_Arceus_Telegraph_Meadow
	encjumpifvar CMP_EQUAL, 1, 13, EncScript_Arceus_Telegraph_Zap
	encjumpifvar CMP_EQUAL, 1, 12, EncScript_Arceus_Telegraph_Splash
	printstring STRINGID_ENCARCEUSNEXTFLAME
	goto EncScript_Arceus_Telegraph_Done
EncScript_Arceus_Telegraph_Splash:
	printstring STRINGID_ENCARCEUSNEXTSPLASH
	goto EncScript_Arceus_Telegraph_Done
EncScript_Arceus_Telegraph_Zap:
	printstring STRINGID_ENCARCEUSNEXTZAP
	goto EncScript_Arceus_Telegraph_Done
EncScript_Arceus_Telegraph_Meadow:
	printstring STRINGID_ENCARCEUSNEXTMEADOW
	goto EncScript_Arceus_Telegraph_Done
EncScript_Arceus_Telegraph_Icicle:
	printstring STRINGID_ENCARCEUSNEXTICICLE
	goto EncScript_Arceus_Telegraph_Done
EncScript_Arceus_Telegraph_Fist:
	printstring STRINGID_ENCARCEUSNEXTFIST
EncScript_Arceus_Telegraph_Done:
	waitmessage B_WAIT_TIME_LONG
	return

// Invokes the ACTIVE Plate held in Var(Plate) (1-6): the real form change, the law, and a slot-3
// move of the Plate's own type so the rule is backed by an attack that means it. encformchange
// takes stats, types and ability from the new species, so a Plate is a genuine type change that the
// damage calc, the AI and the effectiveness message all see. The fail label is the law itself, so a
// refused form change still leaves the rule standing.
EncScript_Arceus_InvokePlate:
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Arceus_Invoke_Fist
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Arceus_Invoke_Icicle
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Arceus_Invoke_Meadow
	encjumpifvar CMP_EQUAL, 1, 3, EncScript_Arceus_Invoke_Zap
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Arceus_Invoke_Splash
	encjumpifvar CMP_NOT_EQUAL, 1, 1, EncScript_Arceus_Invoke_None
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_FIRE, EncScript_Arceus_Invoke_Flame_Law, B_ANIM_FORM_CHANGE
EncScript_Arceus_Invoke_Flame_Law:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_FLAMETHROWER
	printstring STRINGID_ENCARCEUSLAWFLAME
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SUN, 0
	playanimation BS_OPPONENT1, B_ANIM_SUN_CONTINUES
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SEA_OF_FIRE, 0
	playanimation BS_PLAYER1, B_ANIM_SEA_OF_FIRE
	return
EncScript_Arceus_Invoke_Splash:
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_WATER, EncScript_Arceus_Invoke_Splash_Law, B_ANIM_FORM_CHANGE
EncScript_Arceus_Invoke_Splash_Law:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SURF
	printstring STRINGID_ENCARCEUSLAWWATER
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_RAIN, 0
	playanimation BS_OPPONENT1, B_ANIM_RAIN_CONTINUES
	encsetsidestatus ENC_TARGET_ALL_FOES, ENC_SIDE_SWAMP, 0
	playanimation BS_PLAYER1, B_ANIM_SWAMP
	return
EncScript_Arceus_Invoke_Zap:
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_ELECTRIC, EncScript_Arceus_Invoke_Zap_Law, B_ANIM_FORM_CHANGE
EncScript_Arceus_Invoke_Zap_Law:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_THUNDERBOLT
	printstring STRINGID_ENCARCEUSLAWLIGHTNING
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 0
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	return
EncScript_Arceus_Invoke_Meadow:
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_GRASS, EncScript_Arceus_Invoke_Meadow_Law, B_ANIM_FORM_CHANGE
EncScript_Arceus_Invoke_Meadow_Law:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_ENERGY_BALL
	printstring STRINGID_ENCARCEUSLAWLIFE
	waitmessage B_WAIT_TIME_LONG
	return
EncScript_Arceus_Invoke_Icicle:
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_ICE, EncScript_Arceus_Invoke_Icicle_Law, B_ANIM_FORM_CHANGE
EncScript_Arceus_Invoke_Icicle_Law:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_ICE_BEAM
	printstring STRINGID_ENCARCEUSLAWFROST
	waitmessage B_WAIT_TIME_LONG
	encsetweather BATTLE_WEATHER_SNOW, 0
	playanimation BS_OPPONENT1, B_ANIM_SNOW_CONTINUES
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_AURORA_VEIL, 0
	return
EncScript_Arceus_Invoke_Fist:
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_FIGHTING, EncScript_Arceus_Invoke_Fist_Law, B_ANIM_FORM_CHANGE
EncScript_Arceus_Invoke_Fist_Law:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_BRICK_BREAK
	printstring STRINGID_ENCARCEUSLAWFORCE
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_MIST, 0
EncScript_Arceus_Invoke_None:
	return

// Re-arm values by phase. The timer ticks at TurnOpen and resolves at OnTurnEnd on 1, so a re-arm
// of N is a new Plate every N-1 turns: 4 is every third turn (Phase 1), 3 every second (Phase 2),
// 2 every turn (Phase 3). The telegraph fires when the tick lands on 1, which is the same turn the
// Plate arrives at - and it is suppressed outright in Phase 3, so the rapid cycling comes with no
// warning at all.
EncScript_Arceus_PlateArm:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Arceus_PlateArm_Fast
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Arceus_PlateArm_Mid
	encsetvar 2, 4
	return
EncScript_Arceus_PlateArm_Mid:
	encsetvar 2, 3
	return
EncScript_Arceus_PlateArm_Fast:
	encsetvar 2, 2
	return

// A broken Plate costs Arceus tempo as well as Authority.
EncScript_Arceus_PlateArmSlow:
	call EncScript_Arceus_PlateArm
	encaddvar 2, 1
	return

// TurnOpen step: the countdown, plus the telegraph when it lands on 1. Runs after the law upkeep so
// the turn the Plate rotates still gets its predecessor's upkeep line first.
EncScript_Arceus_PlateTick:
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Arceus_PlateTick_Done
	encsubvar 2, 1
	encjumpifvar CMP_NOT_EQUAL, 2, 1, EncScript_Arceus_PlateTick_Done
	encjumpifvar CMP_GREATER_THAN, 0, 2, EncScript_Arceus_PlateTick_Done   @ Phase 3: no warning
	call EncScript_Arceus_PickPlate
	call EncScript_Arceus_Telegraph
EncScript_Arceus_PlateTick_Done:
	return

// TurnOpen step: what the active law does every turn, plus the recurring "the rule is still here"
// line guideline 14 asks for. A telegraphed Plate (11-16) has no upkeep - the telegraph line is
// that turn's cue instead.
EncScript_Arceus_LawUpkeep:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Arceus_LawUpkeep_Done
	encjumpifvar CMP_GREATER_THAN, 1, 10, EncScript_Arceus_LawUpkeep_Done
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Arceus_LawUpkeep_Meadow
	encjumpifvar CMP_EQUAL, 1, 6, EncScript_Arceus_LawUpkeep_Fist
EncScript_Arceus_LawUpkeep_Line:
	printstring STRINGID_ENCARCEUSLAWHOLDS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Arceus_LawUpkeep_Done:
	return
EncScript_Arceus_LawUpkeep_Meadow:
	enchangehp ENC_TARGET_ALL_FOES, -5, ENC_AMOUNT_PERCENT
	playanimation BS_PLAYER1, B_ANIM_LEECH_SEED_DRAIN
	goto EncScript_Arceus_LawUpkeep_Line
EncScript_Arceus_LawUpkeep_Fist:
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	goto EncScript_Arceus_LawUpkeep_Line

// --- Judgment and the Divine Mark ---

// TurnOpen step, Phases 2-3 only. Judgment escalates one stage a turn and falls at the end of the
// third: MARKED, then CONDEMNED (where Arceus's attacks visibly converge), then it lands.
EncScript_Arceus_JudgeStep:
	encjumpifvar CMP_LESS_THAN, 0, 2, EncScript_Arceus_JudgeStep_Done
	encjumpifvar CMP_GREATER_THAN, 0, 3, EncScript_Arceus_JudgeStep_Done
	encjumpifvar CMP_EQUAL, 5, 1, EncScript_Arceus_JudgeStep_Mark
	encjumpifvar CMP_EQUAL, 5, 2, EncScript_Arceus_JudgeStep_Condemn
	encjumpifvar CMP_NOT_EQUAL, 5, 0, EncScript_Arceus_JudgeStep_Done
	encjumpifchance 45, EncScript_Arceus_JudgeStep_Try
EncScript_Arceus_JudgeStep_Done:
	return
EncScript_Arceus_JudgeStep_Try:
	encjumpifvar CMP_GREATER_THAN, 3, 2, EncScript_Arceus_JudgeStep_Begin
	call EncScript_Arceus_Wavers
	return
EncScript_Arceus_JudgeStep_Begin:
	call EncScript_Arceus_AuthDrain3
	encsetvar 5, 1
	printstring STRINGID_ENCARCEUSJUDGES
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	return
EncScript_Arceus_JudgeStep_Mark:
	encsetvar 5, 2
	printstring STRINGID_ENCARCEUSMARKED
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Arceus_JudgeStep_Condemn:
	encsetvar 5, 3
	printstring STRINGID_ENCARCEUSCONDEMNED
	waitmessage B_WAIT_TIME_LONG
	encchangestat ENC_TARGET_BOSS, STAT_ATK, 1
	encchangestat ENC_TARGET_BOSS, STAT_SPATK, 1
	return

// Begins a Judgment for 3 Authority. Split out because the drain routines are shared and this is
// the only 3-point spend.
EncScript_Arceus_AuthDrain3:
	encjumpifvar CMP_LESS_THAN, 3, 3, EncScript_Arceus_AuthDrain_Floor
	encsubvar 3, 3
	goto EncScript_Arceus_AuthApply

// One echo per Judgment, rotating off the active Plate so the power Arceus recalls is tied to the
// law that was up. Unconditional - encounter conditions cannot read overworld flags, so whether the
// player has met Dialga, Palkia or Giratina is not something this fight can know.
EncScript_Arceus_Echo:
	encjumpifvar CMP_EQUAL, 1, 1, EncScript_Arceus_Echo_Temporal
	encjumpifvar CMP_EQUAL, 1, 4, EncScript_Arceus_Echo_Temporal
	encjumpifvar CMP_EQUAL, 1, 2, EncScript_Arceus_Echo_Spatial
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Arceus_Echo_Spatial
	printstring STRINGID_ENCARCEUSECHODISTORTION
	waitmessage B_WAIT_TIME_LONG
	removeweather
	encclearscreens ENC_TARGET_ALL_FOES, EncScript_Arceus_Echo_Done
EncScript_Arceus_Echo_Done:
	return
EncScript_Arceus_Echo_Temporal:
	printstring STRINGID_ENCARCEUSECHOTEMPORAL
	waitmessage B_WAIT_TIME_LONG
	encsetsidestatus ENC_TARGET_BOSS, ENC_SIDE_TAILWIND, 4
	playanimation BS_OPPONENT1, B_ANIM_TAILWIND
	return
EncScript_Arceus_Echo_Spatial:
	printstring STRINGID_ENCARCEUSECHOSPATIAL
	waitmessage B_WAIT_TIME_LONG
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	playanimation BS_PLAYER1, B_ANIM_TURN_TRAP
	return

// --- Decrees ---

// Undoes whatever a Decree did, shared by the timed expiry and the early collapse. Stat-stage
// Decrees use STAGES precisely so the reversal is exact; a Haze in between costs both sides up to
// two stages symmetrically, which is noted rather than engineered around.
EncScript_Arceus_DecreeEnd:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Arceus_DecreeEnd_Strength
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Arceus_DecreeEnd_Mind
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Arceus_DecreeEnd_Binding
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Arceus_DecreeEnd_Balance
	goto EncScript_Arceus_DecreeEnd_Clear
EncScript_Arceus_DecreeEnd_Strength:
	encchangestat ENC_TARGET_ALL_BATTLERS, STAT_ATK, -2
	goto EncScript_Arceus_DecreeEnd_Clear
EncScript_Arceus_DecreeEnd_Mind:
	encchangestat ENC_TARGET_ALL_BATTLERS, STAT_SPATK, -2
	goto EncScript_Arceus_DecreeEnd_Clear
EncScript_Arceus_DecreeEnd_Binding:
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	goto EncScript_Arceus_DecreeEnd_Clear
EncScript_Arceus_DecreeEnd_Balance:
	encclearsidestatus ENC_TARGET_ALL_BATTLERS, ENC_SIDE_MIST
EncScript_Arceus_DecreeEnd_Clear:
	encsetvar 6, 0
	encsetvar 7, 0
	return

// The recurring reminder of which rule holds, so a player who tabbed past the declaration still
// learns it.
EncScript_Arceus_DecreeLine:
	encjumpifvar CMP_EQUAL, 6, 1, EncScript_Arceus_DecreeLine_Strength
	encjumpifvar CMP_EQUAL, 6, 2, EncScript_Arceus_DecreeLine_Mind
	encjumpifvar CMP_EQUAL, 6, 3, EncScript_Arceus_DecreeLine_Binding
	encjumpifvar CMP_EQUAL, 6, 4, EncScript_Arceus_DecreeLine_Balance
	printstring STRINGID_ENCARCEUSHOLDSTOLL
	goto EncScript_Arceus_DecreeLine_Done
EncScript_Arceus_DecreeLine_Strength:
	printstring STRINGID_ENCARCEUSHOLDSSTRENGTH
	goto EncScript_Arceus_DecreeLine_Done
EncScript_Arceus_DecreeLine_Mind:
	printstring STRINGID_ENCARCEUSHOLDSMIND
	goto EncScript_Arceus_DecreeLine_Done
EncScript_Arceus_DecreeLine_Binding:
	printstring STRINGID_ENCARCEUSHOLDSBINDING
	goto EncScript_Arceus_DecreeLine_Done
EncScript_Arceus_DecreeLine_Balance:
	printstring STRINGID_ENCARCEUSHOLDSBALANCE
EncScript_Arceus_DecreeLine_Done:
	waitmessage B_WAIT_TIME_SHORT
	return

// Three turns in Phase 0, two in Phase 4 - the same rule, thrown faster.
EncScript_Arceus_DecreeArm:
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Arceus_DecreeArm_Fast
	encsetvar 7, 3
	return
EncScript_Arceus_DecreeArm_Fast:
	encsetvar 7, 2
	return

// The FIRST TurnOpen step, ahead of the fresh HP mark: HpMark still holds last turn's opening
// percentage, so ENC_SNAP_DAMAGE turns it into "percent of Arceus's max HP taken off over the last
// turn". 4% through a 90% reduction is a low bar deliberately - a Decree is the fight's earliest
// lever on Authority, and it should answer to any real attack rather than to a burst check.
// Decrees run only in Phases 0 and 4 and Judgment only in Phases 2 and 3, so the two disrupt tests
// can share one mark without ever colliding.
EncScript_Arceus_DecreeDisrupt:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Arceus_DecreeDisrupt_Done
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE, EncScript_Arceus_DecreeDisrupt_Done
	encjumpifvar CMP_LESS_THAN, 12, 4, EncScript_Arceus_DecreeDisrupt_Done
	printstring STRINGID_ENCARCEUSDECREECRACKS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_DecreeEnd
	call EncScript_Arceus_AuthDrain1
EncScript_Arceus_DecreeDisrupt_Done:
	return

// TurnOpen step, Phases 0 and 4 only. A Decree and a Plate Law are the same shape of thing - a
// temporary field-wide rule - so they never run together; splitting them gives the two bookend
// phases their own identity.
EncScript_Arceus_DecreeStep:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Arceus_DecreeStep_Live
	encjumpifvar CMP_EQUAL, 0, 4, EncScript_Arceus_DecreeStep_Live
	return
EncScript_Arceus_DecreeStep_Live:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Arceus_DecreeDeclare
	call EncScript_Arceus_DecreeLine
	encjumpifvar CMP_NOT_EQUAL, 6, 5, EncScript_Arceus_DecreeStep_Tick
	enchangehp ENC_TARGET_ALL_BATTLERS, -6, ENC_AMOUNT_PERCENT
EncScript_Arceus_DecreeStep_Tick:
	encjumpifvar CMP_LESS_THAN, 7, 2, EncScript_Arceus_DecreeStep_Done
	encsubvar 7, 1
EncScript_Arceus_DecreeStep_Done:
	return

// Costs 2 Authority. The affordability test sits AFTER the roll, so the "authority wavers" line
// fires only on a turn Arceus actually wanted to declare something - the refusal is a callout, not
// a metronome.
EncScript_Arceus_DecreeDeclare:
	encjumpifchance 50, EncScript_Arceus_DecreeDeclare_Try
	return
EncScript_Arceus_DecreeDeclare_Try:
	encjumpifvar CMP_GREATER_THAN, 3, 1, EncScript_Arceus_DecreeDeclare_Roll
	call EncScript_Arceus_Wavers
	return
EncScript_Arceus_DecreeDeclare_Roll:
	call EncScript_Arceus_AuthDrain2
	call EncScript_Arceus_DecreeArm
	encjumpifchance 20, EncScript_Arceus_Decree_Strength
	encjumpifchance 25, EncScript_Arceus_Decree_Mind
	encjumpifchance 33, EncScript_Arceus_Decree_Binding
	encjumpifchance 50, EncScript_Arceus_Decree_Balance
	encsetvar 6, 5
	printstring STRINGID_ENCARCEUSDECREETOLL
	goto EncScript_Arceus_Decree_Shown
EncScript_Arceus_Decree_Strength:
	encsetvar 6, 1
	encchangestat ENC_TARGET_ALL_BATTLERS, STAT_ATK, 2
	printstring STRINGID_ENCARCEUSDECREESTRENGTH
	goto EncScript_Arceus_Decree_Shown
EncScript_Arceus_Decree_Mind:
	encsetvar 6, 2
	encchangestat ENC_TARGET_ALL_BATTLERS, STAT_SPATK, 2
	printstring STRINGID_ENCARCEUSDECREEMIND
	goto EncScript_Arceus_Decree_Shown
EncScript_Arceus_Decree_Binding:
	encsetvar 6, 3
	encsettrapped ENC_TARGET_ALL_FOES, TRUE
	printstring STRINGID_ENCARCEUSDECREEBINDING
	goto EncScript_Arceus_Decree_Shown
EncScript_Arceus_Decree_Balance:
	encsetvar 6, 4
	normalisebuffs
	encsetsidestatus ENC_TARGET_ALL_BATTLERS, ENC_SIDE_MIST, 0
	printstring STRINGID_ENCARCEUSDECREEBALANCE
EncScript_Arceus_Decree_Shown:
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	return

// --- Judgment: Creation, the five-turn finale ---

// TurnOpen step, Phase 4 only. Turns 8-6 of the clock are plain Original One turns; the charge
// becomes visible at 5, which is where the response is offered.
EncScript_Arceus_FinaleStep:
	encjumpifvar CMP_NOT_EQUAL, 0, 4, EncScript_Arceus_FinaleStep_Done
	encjumpifvar CMP_LESS_THAN, 13, 2, EncScript_Arceus_FinaleStep_Done
	encsubvar 13, 1
	encjumpifvar CMP_GREATER_THAN, 13, 5, EncScript_Arceus_FinaleStep_Done
	encjumpifvar CMP_EQUAL, 13, 5, EncScript_Arceus_FinaleStep_Prompt
EncScript_Arceus_FinaleStep_Rung:
	playanimation BS_OPPONENT1, B_ANIM_TERA_CHARGE
	call EncScript_Arceus_Ladder
EncScript_Arceus_FinaleStep_Done:
	return
EncScript_Arceus_FinaleStep_Prompt:
	printstring STRINGID_ENCARCEUSPREPARESJUDGMENT
	waitmessage B_WAIT_TIME_LONG
	encjumpifvar CMP_GREATER_THAN, 3, 3, EncScript_Arceus_FinaleStep_Absolute
	encaskyesno STRINGID_ENCARCEUSWORLDAWAITS, EncScript_Arceus_FinaleStep_Challenge
	goto EncScript_Arceus_FinaleStep_Rung
EncScript_Arceus_FinaleStep_Challenge:
	encsetvar 14, 10
	printstring STRINGID_ENCARCEUSREFUSESTOYIELD
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Arceus_FinaleStep_Rung
EncScript_Arceus_FinaleStep_Absolute:
	printstring STRINGID_ENCARCEUSABSOLUTE
	waitmessage B_WAIT_TIME_LONG
	goto EncScript_Arceus_FinaleStep_Rung

// Var(Breaks) doubles as the ladder counter, using a sentinel offset so it needs no second
// variable: 0 means the player submitted or was never offered the choice, 10-15 is the ladder
// itself. Printing the rung every charge turn is what lets the player watch their own progress
// instead of guessing.
EncScript_Arceus_Ladder:
	encjumpifvar CMP_LESS_THAN, 14, 10, EncScript_Arceus_Ladder_None
	encjumpifvar CMP_EQUAL, 14, 11, EncScript_Arceus_Ladder_Twelve
	encjumpifvar CMP_EQUAL, 14, 12, EncScript_Arceus_Ladder_Eight
	encjumpifvar CMP_EQUAL, 14, 13, EncScript_Arceus_Ladder_Four
	encjumpifvar CMP_GREATER_THAN, 14, 13, EncScript_Arceus_Ladder_One
	printstring STRINGID_ENCARCEUSLADDEREIGHTEEN
	goto EncScript_Arceus_Ladder_Done
EncScript_Arceus_Ladder_Twelve:
	printstring STRINGID_ENCARCEUSLADDERTWELVE
	goto EncScript_Arceus_Ladder_Done
EncScript_Arceus_Ladder_Eight:
	printstring STRINGID_ENCARCEUSLADDEREIGHT
	goto EncScript_Arceus_Ladder_Done
EncScript_Arceus_Ladder_Four:
	printstring STRINGID_ENCARCEUSLADDERFOUR
	goto EncScript_Arceus_Ladder_Done
EncScript_Arceus_Ladder_One:
	printstring STRINGID_ENCARCEUSLADDERONE
	goto EncScript_Arceus_Ladder_Done
EncScript_Arceus_Ladder_None:
	printstring STRINGID_ENCARCEUSCHARGEDEEPENS
EncScript_Arceus_Ladder_Done:
	waitmessage B_WAIT_TIME_SHORT
	return

// --- Trigger scripts ---

// gEncounterVars is zeroed at battle start, so only Authority and the HP mark need seeding.
// Slot 3 is Cosmic Power for the whole of Phase 0: a slow, watchful move that gives the observation
// phase its feel and gives the player something worth Hazing.
EncScript_Arceus_Intro::
	encsetvar 3, 6    @ Authority
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_SET
	printstring STRINGID_ENCARCEUSGAZES
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCARCEUSSILENCE
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. TurnGuard is set
// immediately rather than at the tail so no branch out of this script can leave it re-armed.
// Order matters. The Decree disrupt test reads last turn's mark before anything else can move
// Arceus's HP; the fresh mark is taken next; Authority regains and is announced before any spend
// is attempted; the law's upkeep runs before the countdown that may rotate it; and each of the last
// four steps opens with a Phase test and falls straight through when the phase is wrong.
EncScript_Arceus_TurnOpen::
	flushtextbox
	encsetvar 15, 1    @ TurnGuard
	encjumpifvar CMP_GREATER_THAN, 0, 4, EncScript_Arceus_TurnOpen_Done   @ Final Moment: inert
	call EncScript_Arceus_DecreeDisrupt
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_SET
	call EncScript_Arceus_AuthGain1
	call EncScript_Arceus_LawUpkeep
	call EncScript_Arceus_PlateTick
	call EncScript_Arceus_JudgeStep
	call EncScript_Arceus_DecreeStep
	call EncScript_Arceus_FinaleStep
EncScript_Arceus_TurnOpen_Done:
	return

// TurnClose owns nothing but the per-turn guard resets. Every countdown lives at TurnOpen instead,
// one checkpoint away from the OnTurnEnd resolutions that re-arm them.
EncScript_Arceus_TurnClose::
	encsetvar 15, 0    @ TurnGuard
	encsetvar 10, 0    @ Shift
	encsetvar 11, 0    @ Seen
	return

// Anti-cheese. curestatus takes an explicit battler - BS_TARGET/BS_ATTACKER are stale at
// OnTurnStart - and the attempt feeds the meter, so stilling the Creator is not free. Leads with
// flushtextbox for the same reason TurnOpen does: this runs one priority earlier.
EncScript_Arceus_WillNotBeStilled::
	flushtextbox
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCARCEUSNOTSTILLED
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_AuthGain2
	return

// The six Plate breaks share this one script, which reads Var(Plate) only through ClearLaw - the
// teardown is identity-free, so nothing here has to know which Plate fell. Breaking a Plate is
// always the right answer to something: it also clears an in-progress Judgment.
EncScript_Arceus_PlateBreak::
	encsetvar 10, 1    @ Shift
	call EncScript_Arceus_ClearLaw
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_NORMAL, EncScript_Arceus_PlateBreak_Told, B_ANIM_ILLUSION_OFF
EncScript_Arceus_PlateBreak_Told:
	encsetvar 1, 0    @ Plate
	encsetmove ENC_TARGET_BOSS, 3, MOVE_COSMIC_POWER
	printstring STRINGID_ENCARCEUSPLATEBREAKS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_AuthDrain2
	call EncScript_Arceus_PlateArmSlow
	encjumpifvar CMP_EQUAL, 5, 0, EncScript_Arceus_PlateBreak_Done
	encsetvar 5, 0    @ Judge
	printstring STRINGID_ENCARCEUSCONCENTRATIONBROKEN
	waitmessage B_WAIT_TIME_LONG
EncScript_Arceus_PlateBreak_Done:
	return

// Divine Observation. Silent by design - the player is told that Arceus adapted, never to what.
// Capped at 99 so a u8 cannot wrap in a long fight; both counters only ever move once per turn.
EncScript_Arceus_SeePhysical::
	encsetvar 11, 1    @ Seen
	encjumpifvar CMP_GREATER_THAN, 8, 98, EncScript_Arceus_SeePhysical_Done
	encaddvar 8, 1
EncScript_Arceus_SeePhysical_Done:
	return

EncScript_Arceus_SeeSpecial::
	encsetvar 11, 1    @ Seen
	encjumpifvar CMP_GREATER_THAN, 9, 98, EncScript_Arceus_SeeSpecial_Done
	encaddvar 9, 1
EncScript_Arceus_SeeSpecial_Done:
	return

// Switching out drops the Judgment back to its first stage. You bought two turns and paid a
// Pokemon's worth of tempo.
EncScript_Arceus_MarkTransfers::
	encsetvar 5, 1    @ Judge
	printstring STRINGID_ENCARCEUSMARKTRANSFERS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_TOTEM_FLARE
	return

// The Plate rotation, resolved. A telegraphed Plate (11-16) is already chosen; in Phase 3 there was
// no telegraph, so the pick and the teardown happen here instead. The re-arm is taken up front so
// every path leaves the clock armed - including the one where Arceus cannot afford the Plate at
// all, which is where the whole meter becomes visible.
EncScript_Arceus_PlateResolve::
	call EncScript_Arceus_PlateArm
	encjumpifvar CMP_GREATER_THAN, 1, 10, EncScript_Arceus_PlateResolve_Invoke
	call EncScript_Arceus_PickPlate
EncScript_Arceus_PlateResolve_Invoke:
	call EncScript_Arceus_ClearLaw
	encjumpifvar CMP_EQUAL, 3, 0, EncScript_Arceus_PlateResolve_Cant
	call EncScript_Arceus_AuthDrain1
	encsubvar 1, 10
	call EncScript_Arceus_InvokePlate
	return
EncScript_Arceus_PlateResolve_Cant:
	encsetvar 1, 0    @ Plate: nothing rises, and the next rotation picks fresh
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_NORMAL, EncScript_Arceus_PlateResolve_Bare, B_ANIM_ILLUSION_OFF
EncScript_Arceus_PlateResolve_Bare:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_COSMIC_POWER
	call EncScript_Arceus_Wavers
	return

// Judgment falling. The turn's damage is measured against the mark taken at turn open first, so
// breaking Arceus's concentration is a live out right up to the last moment.
EncScript_Arceus_JudgmentFalls::
	encsetvar 5, 0    @ Judge
	encsnapshothp ENC_TARGET_BOSS, 12, ENC_SNAP_DAMAGE, EncScript_Arceus_JudgmentFalls_Land
	encjumpifvar CMP_LESS_THAN, 12, 4, EncScript_Arceus_JudgmentFalls_Land
	printstring STRINGID_ENCARCEUSCONCENTRATIONBROKEN
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_AuthDrain2
	return
EncScript_Arceus_JudgmentFalls_Land:
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	printstring STRINGID_ENCARCEUSJUDGMENT
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_PLAYER1, B_ANIM_MON_HIT
	enchangehp ENC_TARGET_ALL_FOES, -35, ENC_AMOUNT_PERCENT
	call EncScript_Arceus_Echo
	return

// A Decree running out on its own clock.
EncScript_Arceus_DecreeEnds::
	printstring STRINGID_ENCARCEUSDECREEENDS
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_DecreeEnd
	return

// 78% - THE PLATES AWAKEN. Phase 0's Decrees are torn down and the rotation starts immediately, so
// the phase arrives as a rule change rather than an announcement.
EncScript_Arceus_PlatesAwaken::
	encsetvar 0, 1    @ Phase
	printstring STRINGID_ENCARCEUSPLATESAWAKEN
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_AuthGain2
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Arceus_PlatesAwaken_Plate
	call EncScript_Arceus_DecreeEnd
EncScript_Arceus_PlatesAwaken_Plate:
	call EncScript_Arceus_PickPlate
	encsubvar 1, 10
	call EncScript_Arceus_InvokePlate
	encsetvar 2, 4    @ PlateTimer
	return

// 55% - THE CREATOR'S JUDGMENT. The rotation tightens by one turn and the Judgment system arms.
// The "recalls the powers of creation" line is printed once, here, so the first echo lands with
// context rather than arriving out of nowhere.
EncScript_Arceus_Judgment::
	encsetvar 0, 2    @ Phase
	printstring STRINGID_ENCARCEUSDECIDESTOJUDGE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TOTEM_FLARE
	call EncScript_Arceus_AuthGain2
	call EncScript_Arceus_PlateArm
	printstring STRINGID_ENCARCEUSRECALLSCREATION
	waitmessage B_WAIT_TIME_LONG
	return

// 34% - CREATION. The scripted moment, and a genuine hard reset the player has to rebuild from:
// weather, screens, Pledge statuses, trapping and every stat stage on both sides go at once.
// Costs 5 Authority and takes it even from a meter that cannot pay - this one is not a choice.
// Then Divine Observation is cashed in, the rotation drops to a new Plate every turn, and the
// telegraph stops for good.
EncScript_Arceus_Creation::
	encsetvar 0, 3    @ Phase
	printstring STRINGID_ENCARCEUSCREATIONBEGINS
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TERA_CHARGE
	printstring STRINGID_ENCARCEUSTHEREISNOTHING
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_ClearLaw
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Arceus_Creation_Haze
EncScript_Arceus_Creation_Haze:
	normalisebuffs
	playanimation BS_OPPONENT1, B_ANIM_RESTORE_BG
	printstring STRINGID_ENCARCEUSASPARK
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_TERA_ACTIVATE
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_NORMAL, EncScript_Arceus_Creation_Remade, B_ANIM_FORM_CHANGE
EncScript_Arceus_Creation_Remade:
	encsetmove ENC_TARGET_BOSS, 3, MOVE_COSMIC_POWER
	printstring STRINGID_ENCARCEUSWORLDREMADE
	waitmessage B_WAIT_TIME_LONG
	encsetvar 1, 0     @ Plate
	encsetvar 5, 0     @ Judge
	encsetvar 3, 12    @ Authority
	encsetvar 4, 0     @ LastAuth: the new tier is announced from scratch
	call EncScript_Arceus_AuthorityCue
	printstring STRINGID_ENCARCEUSSEENENOUGH
	waitmessage B_WAIT_TIME_LONG
	call EncScript_Arceus_ReadTheParty
	call EncScript_Arceus_InvokePlate
	encsetvar 2, 2     @ PlateTimer: a new Plate every turn from here
	return

// Divine Observation, cashed in. Conditions and encjumpifvar only ever compare a variable against a
// literal, so "which counter is bigger" is expressed as a decrement loop - both counters move at
// most once a turn, so it is bounded by the turn count.
// The answer picks a permanent RAW stat gain (unbounded by the +-6 clamp, immune to Haze) and the
// opening Plate of the second half: the counter to whatever the player has been leaning on.
EncScript_Arceus_ReadTheParty:
	encjumpifvar CMP_EQUAL, 8, 0, EncScript_Arceus_LeansSpecial
	encjumpifvar CMP_EQUAL, 9, 0, EncScript_Arceus_LeansPhysical
	encsubvar 8, 1
	encsubvar 9, 1
	goto EncScript_Arceus_ReadTheParty
EncScript_Arceus_LeansPhysical:
	encsetvar 9, 1    @ Spec: now the stance flag - 1 means the party leans physical
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, 40, ENC_AMOUNT_PERCENT
	encsetvar 1, 6    @ Fist: Mist refuses your stat drops, and its Attack climbs
	return
EncScript_Arceus_LeansSpecial:
	encsetvar 9, 0    @ Spec: special-leaning, or tied
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPDEF, 40, ENC_AMOUNT_PERCENT
	encsetvar 1, 2    @ Splash: Swamp quarters your Speed, and the rain feeds it
	return

// 18% - THE ORIGINAL ONE. No more Plates, ever. Survive is set here rather than in Properties: so
// the AI reads Arceus as killable for three quarters of the fight, and from this point it
// guarantees the finale plays and the catch window opens. The reduction drops - it has stopped
// defending - and all four move slots are rewritten with the power of a different type each.
EncScript_Arceus_OriginalOne::
	encsetvar 0, 4    @ Phase
	call EncScript_Arceus_ClearLaw
	encformchange ENC_TARGET_BOSS, SPECIES_ARCEUS_NORMAL, EncScript_Arceus_OriginalOne_Shatter, B_ANIM_ILLUSION_OFF
EncScript_Arceus_OriginalOne_Shatter:
	printstring STRINGID_ENCARCEUSPLATESSHATTER
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_FORM_CHANGE
	encsetvar 1, 0    @ Plate
	encsetvar 2, 0    @ PlateTimer
	encsetvar 5, 0    @ Judge
	encsetsurvive ENC_TARGET_BOSS, TRUE
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPATK, 30, ENC_AMOUNT_PERCENT
	encchangestatvalue ENC_TARGET_BOSS, STAT_SPEED, 20, ENC_AMOUNT_PERCENT
	encsetmove ENC_TARGET_BOSS, 0, MOVE_JUDGMENT
	encsetmove ENC_TARGET_BOSS, 1, MOVE_EARTH_POWER
	encsetmove ENC_TARGET_BOSS, 2, MOVE_ICE_BEAM
	encsetmove ENC_TARGET_BOSS, 3, MOVE_SHADOW_BALL
	encsetvar 13, 8   @ Charge: three plain turns, then five charge turns
	call EncScript_Arceus_AuthGain2
	printstring STRINGID_ENCARCEUSORIGINALONE
	waitmessage B_WAIT_TIME_LONG
	return

// JUDGMENT: CREATION resolves. Shattering it takes all five breaks - one damaging hit on every
// single charge turn, with no setup turn, no healing turn and no free switch - and is only ever on
// the table if the player pushed Authority to 3 or less by the time the charge began.
EncScript_Arceus_JudgmentCreation::
	encsetvar 13, 0   @ Charge: the finale never repeats
	encsetvar 0, 5    @ Phase
	encjumpifvar CMP_EQUAL, 14, 15, EncScript_Arceus_JudgmentCreation_Shattered
	printstring STRINGID_ENCARCEUSJUDGMENTCREATION
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ULTRA_BURST
	enchangehp ENC_TARGET_ALL_FOES, -55, ENC_AMOUNT_PERCENT
	normalisebuffs
	removeweather
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Arceus_JudgmentCreation_Reset
EncScript_Arceus_JudgmentCreation_Reset:
	encsetvar 3, 12   @ Authority: not just a huge hit - Arceus remakes the battle
	encsetvar 4, 0    @ LastAuth
	call EncScript_Arceus_AuthorityCue
	goto EncScript_Arceus_JudgmentCreation_Final
EncScript_Arceus_JudgmentCreation_Shattered:
	printstring STRINGID_ENCARCEUSFINALPLATE
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	printstring STRINGID_ENCARCEUSREELS
	waitmessage B_WAIT_TIME_LONG
	enchangehp ENC_TARGET_BOSS, -8, ENC_AMOUNT_PERCENT
	encsetrecharge ENC_TARGET_BOSS, 2
	encsetdamagereduction ENC_TARGET_BOSS, 84
	call EncScript_Arceus_AuthDrain5
EncScript_Arceus_JudgmentCreation_Final:
	encsetvar 14, 0   @ Breaks
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Arceus_JudgmentCreation_Quiet
	call EncScript_Arceus_DecreeEnd
EncScript_Arceus_JudgmentCreation_Quiet:
	printstring STRINGID_ENCARCEUSFINALMOMENT
	waitmessage B_WAIT_TIME_LONG
	return

// One qualifying hit per charge turn advances the ladder, and the rung is printed straight back so
// the player sees their own progress land.
EncScript_Arceus_FinaleBreak::
	encsetvar 10, 1   @ Shift
	encaddvar 14, 1
	printstring STRINGID_ENCARCEUSCHARGECRACKS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_ILLUSION_OFF
	call EncScript_Arceus_Ladder
	return

// 10% - the catch window, the mirror image of the Properties: block. Phase 6 makes TurnOpen return
// through its leading branch, so nothing can restart after this point. From here the engine's
// automatic catch-window damage guard keeps the catch target alive; encsetdamagereduction sets the
// value that guard will restore, not the live one.
// Catch rate 20 is the lowest of any shipped legendary - Arceus is the hardest thing in the game
// and stays hard right up to the ball.
EncScript_Arceus_Weakened::
	encsetvar 0, 6    @ Phase
	encsetvar 1, 0    @ Plate
	encsetvar 2, 0    @ PlateTimer
	encsetvar 5, 0    @ Judge
	encsetvar 13, 0   @ Charge
	encsetvar 14, 0   @ Breaks
	call EncScript_Arceus_ClearLaw
	encclearscreens ENC_TARGET_BOSS, EncScript_Arceus_Weakened_Release
EncScript_Arceus_Weakened_Release:
	encsettrapped ENC_TARGET_ALL_FOES, FALSE
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetcaptypeeffectiveness ENC_TARGET_BOSS, FALSE
	encsetflattoxicdamage ENC_TARGET_BOSS, FALSE
	encsetdamagereduction ENC_TARGET_BOSS, 99
	encsetcatchrate 20
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCARCEUSWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
