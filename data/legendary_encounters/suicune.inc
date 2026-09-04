// Suicune, "The Purifier". Var indices, pinned by the
// always-true Conditions on EncScript_Suicune_Intro:
// 0 Phase (0 Cleansing Current / 1 The Cleansing / 2 Sacred Beast / 3 Weakened), 1 Flow (0-5),
// 2 Rite (0 idle, else 3->2->1 resolved at 1), 3 Draw (0 idle, 2 telegraphed, 1 resolves),
// 4 Break (turns of open guard left), 5 LastGuard (reduction last ANNOUNCED), 6 Cleansed,
// 7 TurnGuard, 8 Shatter, 9 Calm, 10 Breach (1 broken draw / 2 shattered purity), 11 Prev,
// 12 Dirty, 13 Swept (a purification already ran this turn).
//
// Flow is fed ONLY by Suicune finding something on the field to wash away, and a turn that both
// purifies nothing and ends clean drains one back. Every effect the player puts down is a point of
// Flow waiting to be collected, so the whole fight is one question: is this worth giving Suicune
// power? The guard is a ladder rather than a locked door - draining Flow is visible progress on its
// own - and the fight inverts twice: Sacred Water only heals off a CLEAN field, and Sacred Beast is
// untouchable at any Flow above 0.

// --- Shared subroutines (call/return) ---

// Sole owner of the damage reduction AND its callout. Prev holds the value last announced so each
// leaf can write LastGuard before comparing; there is no UI for damage reduction, so a single
// showing would be trivially missed and the line has to re-fire on every real change.
EncScript_Suicune_ApplyGuard:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Suicune_ApplyGuard_Done   @ weakened owns its own guard
	enccopyvar 11, 5
	encjumpifvar CMP_GREATER_THAN, 4, 0, EncScript_Suicune_Guard_Open
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Suicune_Guard_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Suicune_Guard_P1
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Suicune_Guard93
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Guard91
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Guard88
	goto EncScript_Suicune_Guard84
EncScript_Suicune_Guard_P1:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Suicune_Guard94
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Guard92
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Guard90
	goto EncScript_Suicune_Guard86
// Sacred Beast at Flow 0 is deliberately the softest un-broken state in the fight: draining Flow has
// to feel like progress on its own, so the shatter reads as a plan paying off, not a lucky roll.
EncScript_Suicune_Guard_P2:
	encjumpifvar CMP_EQUAL, 1, 5, EncScript_Suicune_Guard95
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Guard93
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Guard90
	goto EncScript_Suicune_Guard80
EncScript_Suicune_Guard_Open:
	encjumpifvar CMP_EQUAL, 10, 2, EncScript_Suicune_Guard72
	goto EncScript_Suicune_Guard76

EncScript_Suicune_Guard72:
	encsetdamagereduction ENC_TARGET_BOSS, 72
	encsetvar 5, 72
	encjumpifvar CMP_EQUAL, 11, 72, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 72, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard76:
	encsetdamagereduction ENC_TARGET_BOSS, 76
	encsetvar 5, 76
	encjumpifvar CMP_EQUAL, 11, 76, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 76, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard80:
	encsetdamagereduction ENC_TARGET_BOSS, 80
	encsetvar 5, 80
	encjumpifvar CMP_EQUAL, 11, 80, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 80, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard84:
	encsetdamagereduction ENC_TARGET_BOSS, 84
	encsetvar 5, 84
	encjumpifvar CMP_EQUAL, 11, 84, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 84, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard86:
	encsetdamagereduction ENC_TARGET_BOSS, 86
	encsetvar 5, 86
	encjumpifvar CMP_EQUAL, 11, 86, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 86, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard88:
	encsetdamagereduction ENC_TARGET_BOSS, 88
	encsetvar 5, 88
	encjumpifvar CMP_EQUAL, 11, 88, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 88, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard90:
	encsetdamagereduction ENC_TARGET_BOSS, 90
	encsetvar 5, 90
	encjumpifvar CMP_EQUAL, 11, 90, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 90, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard91:
	encsetdamagereduction ENC_TARGET_BOSS, 91
	encsetvar 5, 91
	encjumpifvar CMP_EQUAL, 11, 91, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 91, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard92:
	encsetdamagereduction ENC_TARGET_BOSS, 92
	encsetvar 5, 92
	encjumpifvar CMP_EQUAL, 11, 92, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 92, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard93:
	encsetdamagereduction ENC_TARGET_BOSS, 93
	encsetvar 5, 93
	encjumpifvar CMP_EQUAL, 11, 93, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 93, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard94:
	encsetdamagereduction ENC_TARGET_BOSS, 94
	encsetvar 5, 94
	encjumpifvar CMP_EQUAL, 11, 94, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 94, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken
EncScript_Suicune_Guard95:
	encsetdamagereduction ENC_TARGET_BOSS, 95
	encsetvar 5, 95
	encjumpifvar CMP_EQUAL, 11, 95, EncScript_Suicune_ApplyGuard_Done
	encjumpifvar CMP_LESS_THAN, 11, 95, EncScript_Suicune_ApplyGuard_Thicken
	goto EncScript_Suicune_ApplyGuard_Slacken

EncScript_Suicune_ApplyGuard_Thicken:
	printstring STRINGID_ENCSUICUNETHICKENS
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Suicune_ApplyGuard_Slacken:
	printstring STRINGID_ENCSUICUNESLACKENS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Suicune_ApplyGuard_Done:
	return

// Clamped +1, silent. A sweep can pay out up to six points at once, so the callout is a separate
// step rather than one line per point.
EncScript_Suicune_GainFlow:
	encjumpifvar CMP_GREATER_THAN, 1, 4, EncScript_Suicune_GainFlow_Done
	encaddvar 1, 1
EncScript_Suicune_GainFlow_Done:
	return

// One line per sweep that washed something away, telling the player the resource moved without
// telling them what it is. The Flow 4 line is louder because that is where Suicune starts shedding
// status unprompted; it re-fires every sweep, so the cue keeps running while the mechanic is live.
EncScript_Suicune_FlowCallout:
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Suicune_FlowCallout_Surge
	printstring STRINGID_ENCSUICUNEFLOWRISES
	waitmessage B_WAIT_TIME_SHORT
	return
EncScript_Suicune_FlowCallout_Surge:
	printstring STRINGID_ENCSUICUNEFLOWSURGES
	waitmessage B_WAIT_TIME_SHORT
	return

EncScript_Suicune_LoseFlow:
	encjumpifvar CMP_EQUAL, 1, 0, EncScript_Suicune_LoseFlow_Done
	encsubvar 1, 1
	printstring STRINGID_ENCSUICUNEFLOWEBBS
	waitmessage B_WAIT_TIME_SHORT
EncScript_Suicune_LoseFlow_Done:
	return

// Stat stages off neutral on either side, into Dirty. Split out of the impurity sweep because the
// purification needs the same answer before deciding whether to Haze.
// The read-only twin of the purification: the same five impurities in the same order, answering into
// Dirty without touching anything. Sacred Water and the Flow decay both hang off this, so the two
// halves of the fight cannot disagree about what "clean" means.
//
// Stat stages are deliberately NOT on the list. They were, on both sides, and it made the player
// audit their own boosts every turn to avoid feeding Flow - too much to track for what it added.
// Suicune's own stages can't be the impurity on their own either: the only reset opcode is
// normalisebuffs, which Hazes every battler, so cleaning a debuff off Suicune would still wipe the
// player's boosts and leave them managing the same thing one step removed.
EncScript_Suicune_Impure:
	encsetvar 12, 1
	jumpifhalfword CMP_NOT_EQUAL, gBattleWeather, B_WEATHER_NONE, EncScript_Suicune_Impure_Done
	jumpifhalfword CMP_COMMON_BITS, gFieldStatuses, STATUS_FIELD_TERRAIN_ANY, EncScript_Suicune_Impure_Done
	jumpifsideaffecting BS_PLAYER1, SIDE_STATUS_BARRIER_ANY, EncScript_Suicune_Impure_Done
	jumpifsideaffecting BS_OPPONENT1, SIDE_STATUS_BARRIER_ANY, EncScript_Suicune_Impure_Done
	trytidyup FALSE, EncScript_Suicune_Impure_Done   @ hazards on either side, or any Substitute
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Suicune_Impure_Done
	encsetvar 12, 0
EncScript_Suicune_Impure_Done:
	return

// The sweep. Walks the five impurities in a fixed order, counting each removal into Cleansed, then
// pays Suicune one point of Flow per thing washed away. The count is left standing afterwards
// because TurnClose reads it to decide whether the turn earned a Flow decay.
EncScript_Suicune_Purify:
	encsetvar 13, 1   @ Swept: one purification per turn, whichever trigger asked for it
	printstring STRINGID_ENCSUICUNEPURIFICATION
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SURF
	waitanimation
	jumpifhalfword CMP_EQUAL, gBattleWeather, B_WEATHER_NONE, EncScript_Suicune_Purify_Terrain
	removeweather
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESWEATHER
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_DEFOG
	waitanimation
EncScript_Suicune_Purify_Terrain:
	jumpifhalfword CMP_NO_COMMON_BITS, gFieldStatuses, STATUS_FIELD_TERRAIN_ANY, EncScript_Suicune_Purify_Screens
	removeterrain
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESTERRAIN
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_DEFOG
	waitanimation
EncScript_Suicune_Purify_Screens:
	encclearscreens ENC_TARGET_ALL_BATTLERS, EncScript_Suicune_Purify_Hazards
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESSCREENS
	waitmessage B_WAIT_TIME_SHORT
	playmoveanimation MOVE_BRICK_BREAK
	waitanimation
EncScript_Suicune_Purify_Hazards:
	trytidyup FALSE, EncScript_Suicune_Purify_HazardsFound
	goto EncScript_Suicune_Purify_Status
EncScript_Suicune_Purify_HazardsFound:
	encaddvar 6, 1
	@ trytidyup TRUE does not advance while it still has something to clear, so this one instruction
	@ is the whole loop; the engine supplies its own removal and Substitute-fade messaging.
	trytidyup TRUE, EncScript_Suicune_Purify_Status
EncScript_Suicune_Purify_Status:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Suicune_Purify_StatusFound
	goto EncScript_Suicune_Purify_Undertow
EncScript_Suicune_Purify_StatusFound:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	encaddvar 6, 1
	printstring STRINGID_ENCSUICUNEWASHESSTATUS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	@ From The Cleansing on, the sweep drags a stage of Speed off the player. This is Suicune's only
	@ hold on the player's stat line now that stages are not impurities, and the drops accumulate -
	@ the sweep's old Haze used to reset them to neutral immediately before reapplying -1, which
	@ pinned the undertow at one stage forever and then paid Suicune a point of Flow next sweep for
	@ the very stage it had just applied itself.
EncScript_Suicune_Purify_Undertow:
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Suicune_Purify_Flow
	encchangestat ENC_TARGET_ALL_FOES, STAT_SPEED, -1
	printstring STRINGID_ENCSUICUNEUNDERTOW
	waitmessage B_WAIT_TIME_SHORT
	@ One point of Flow per thing washed away, five impurities at most. Unrolled rather than looped so
	@ Cleansed survives for TurnClose, which reads it to decide whether this turn earned a Flow decay.
EncScript_Suicune_Purify_Flow:
	encjumpifvar CMP_EQUAL, 6, 0, EncScript_Suicune_Purify_Guard
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 2, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 3, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 4, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
	encjumpifvar CMP_LESS_THAN, 6, 5, EncScript_Suicune_Purify_Paid
	call EncScript_Suicune_GainFlow
EncScript_Suicune_Purify_Paid:
	call EncScript_Suicune_FlowCallout
EncScript_Suicune_Purify_Guard:
	call EncScript_Suicune_ApplyGuard
	return

// From Flow 4, and unconditionally in Sacred Beast, Suicune sheds status on its own - which is what
// makes locking it down self-defeating rather than impossible. Silent no-op when it has none.
EncScript_Suicune_ShedStatus:
	jumpifstatus BS_OPPONENT1, STATUS1_ANY, EncScript_Suicune_ShedStatus_Cure
	return
EncScript_Suicune_ShedStatus_Cure:
	curestatus BS_OPPONENT1
	updatestatusicon BS_OPPONENT1
	printstring STRINGID_ENCSUICUNESHEDS
	waitmessage B_WAIT_TIME_SHORT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return

// End-of-turn mending, on a percentage so it scales with whatever LevelCap built. The Flow threshold
// drops a rung each phase; Sacred Beast mends every turn regardless.
EncScript_Suicune_Regen:
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Suicune_Regen_P2
	encjumpifvar CMP_EQUAL, 0, 1, EncScript_Suicune_Regen_P1
	encjumpifvar CMP_GREATER_THAN, 1, 3, EncScript_Suicune_Regen_Big
	encjumpifvar CMP_GREATER_THAN, 1, 1, EncScript_Suicune_Regen_Small
	return
EncScript_Suicune_Regen_P1:
	encjumpifvar CMP_GREATER_THAN, 1, 2, EncScript_Suicune_Regen_Big
	encjumpifvar CMP_GREATER_THAN, 1, 0, EncScript_Suicune_Regen_Small
	return
EncScript_Suicune_Regen_P2:
	encchangehp ENC_TARGET_BOSS, 2, ENC_AMOUNT_PERCENT
	goto EncScript_Suicune_Regen_Show
EncScript_Suicune_Regen_Big:
	encchangehp ENC_TARGET_BOSS, 3, ENC_AMOUNT_PERCENT
	goto EncScript_Suicune_Regen_Show
EncScript_Suicune_Regen_Small:
	encchangehp ENC_TARGET_BOSS, 1, ENC_AMOUNT_PERCENT
EncScript_Suicune_Regen_Show:
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCSUICUNEMENDS
	waitmessage B_WAIT_TIME_SHORT
	return

// Shared body of the two shatter triggers. Reached by goto, not call, so its return pops the trigger
// dispatch frame like any other trigger script's would.
EncScript_Suicune_ShatterCore:
	printstring STRINGID_ENCSUICUNESHATTERED
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_HYDRO_PUMP
	waitanimation
	encsetvar 1, 0    @ Flow
	encsetvar 3, 0    @ Draw
	encsetvar 4, 2    @ Break: two turns of open guard
	encsetvar 10, 2   @ Breach: shattered purity
	call EncScript_Suicune_ApplyGuard
	return

// --- Trigger scripts ---

// LastGuard is seeded to the Properties reduction so the first real change reads as a change. The
// second line is the hint that the player's own effects, not their attacks, are what Suicune is
// reacting to.
EncScript_Suicune_Intro::
	encsetvar 5, 86
	encsetvar 2, 3   @ Rite armed
	printstring STRINGID_ENCSUICUNEINTRO
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_ENCSUICUNEWATCHES
	waitmessage B_WAIT_TIME_LONG
	return

// Top of every turn (TurnGuard gate). Leads with flushtextbox: an OnTurnStart script that animates
// before printing anything renders on top of the still-open action menu. Clears the per-turn scratch
// and guards, sheds status where Flow or the phase says to, and keeps the two recurring callouts
// running - the open window, and the standing "the water is still" cue that Flow is draining.
EncScript_Suicune_TurnOpen::
	flushtextbox
	encsetvar 7, 1    @ TurnGuard
	encsetvar 6, 0    @ Cleansed
	encsetvar 8, 0    @ Shatter
	encsetvar 13, 0   @ Swept
	encjumpifvar CMP_NOT_EQUAL, 3, 3, EncScript_Suicune_TurnOpen_Phase
	encsetvar 3, 0    @ a spent draw rests one turn before Suicune may draw again
EncScript_Suicune_TurnOpen_Phase:
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Suicune_TurnOpen_Done   @ weakened: inert
	encjumpifvar CMP_EQUAL, 0, 2, EncScript_Suicune_TurnOpen_Shed
	encjumpifvar CMP_LESS_THAN, 1, 4, EncScript_Suicune_TurnOpen_Break
EncScript_Suicune_TurnOpen_Shed:
	call EncScript_Suicune_ShedStatus
EncScript_Suicune_TurnOpen_Break:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Suicune_TurnOpen_Calm
	printstring STRINGID_ENCSUICUNEUNSTEADY
	waitmessage B_WAIT_TIME_SHORT
	goto EncScript_Suicune_TurnOpen_Done
EncScript_Suicune_TurnOpen_Calm:
	encjumpifvar CMP_LESS_THAN, 9, 2, EncScript_Suicune_TurnOpen_Done
	printstring STRINGID_ENCSUICUNESTILL
	waitmessage B_WAIT_TIME_SHORT
EncScript_Suicune_TurnOpen_Done:
	return

// The scheduled purification. TurnClose steps Rite 3->2->1 and this resolves it at 1, one checkpoint
// later, so a real turn always passes between a tick and the sweep.
EncScript_Suicune_Rite::
	flushtextbox
	call EncScript_Suicune_Purify
	encsetvar 2, 3
	encjumpifvar CMP_EQUAL, 0, 0, EncScript_Suicune_Rite_Done
	encsetvar 2, 2   @ The Cleansing: the rite comes round faster
EncScript_Suicune_Rite_Done:
	return

// Sacred Water, telegraphed a full turn ahead. The field is wiped first so the counterplay is
// legible: anything standing on it next turn is unambiguously the player's own doing.
EncScript_Suicune_SacredDraw::
	flushtextbox
	call EncScript_Suicune_Purify
	encsetvar 3, 2
	printstring STRINGID_ENCSUICUNEDRAWS
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_AQUA_RING
	waitanimation
	return

// The inversion: a 10% heal that only lands off a clean field, so on this one turn the player has to
// spend the Flow they have starved all fight. Dirtying it instead breaks the draw wide open.
EncScript_Suicune_SacredResolve::
	flushtextbox
	@ Draw 3 is the "spent this turn" sentinel, cleared by the next TurnOpen. Priority alone would not
	@ hold: every trigger is re-evaluated after each script, so plain 0 would let the Flow-5 draw
	@ re-telegraph in this same dispatch.
	encsetvar 3, 3
	call EncScript_Suicune_Impure
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Suicune_SacredResolve_Break
	printstring STRINGID_ENCSUICUNESACREDWATER
	waitmessage B_WAIT_TIME_LONG
	encchangehp ENC_TARGET_BOSS, 10, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	return
EncScript_Suicune_SacredResolve_Break:
	printstring STRINGID_ENCSUICUNEBROKEN
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_MUDDY_WATER
	waitanimation
	encsetvar 1, 2    @ Flow knocked back
	encsetvar 4, 1    @ Break: this turn only
	encsetvar 10, 1   @ Breach: broken draw
	call EncScript_Suicune_ApplyGuard
	return

// Grass or Electric into a Flow-0 Sacred Beast. Gating the type door behind Flow 0 is what turns
// "bring the counter-type" into the conclusion of the Purity puzzle rather than a checklist item.
EncScript_Suicune_ShatterType::
	encsetvar 8, 1   @ Shatter: one shatter attempt per turn
	goto EncScript_Suicune_ShatterCore

// The lucky second door. The crit test cannot be expressed as a trigger condition, so it is checked
// in-script; a non-crit still consumes the guard rather than letting the trigger re-fire.
EncScript_Suicune_ShatterCrit::
	encsetvar 8, 1
	jumpifnotcriticalhit EncScript_Suicune_ShatterCrit_Done
	goto EncScript_Suicune_ShatterCore
EncScript_Suicune_ShatterCrit_Done:
	return

// Sacred Beast purifies at the end of every turn instead of on the Rite timer. Purify sets Swept,
// which is this trigger's self-disabling gate; TurnOpen re-arms it.
EncScript_Suicune_StillWater::
	call EncScript_Suicune_Purify
	return

// OnTurnEnd, priority 90 - runs after any phase transition at the same checkpoint. Owns every
// countdown in one place, then the Flow decay. Gating decay on Cleansed == 0 is load-bearing: a
// purification turn ends with a clean field by definition, and without the guard it would instantly
// refund the Flow it had just paid out.
EncScript_Suicune_TurnClose::
	encsetvar 7, 0   @ TurnGuard
	encjumpifvar CMP_EQUAL, 0, 3, EncScript_Suicune_TurnClose_Done   @ weakened: inert
	encjumpifvar CMP_LESS_THAN, 2, 2, EncScript_Suicune_TurnClose_Draw
	encsubvar 2, 1
EncScript_Suicune_TurnClose_Draw:
	encjumpifvar CMP_NOT_EQUAL, 3, 2, EncScript_Suicune_TurnClose_Break   @ 3 is the spent sentinel
	encsubvar 3, 1
EncScript_Suicune_TurnClose_Break:
	encjumpifvar CMP_EQUAL, 4, 0, EncScript_Suicune_TurnClose_Shed
	encsubvar 4, 1
	encjumpifvar CMP_NOT_EQUAL, 4, 0, EncScript_Suicune_TurnClose_Shed
	encsetvar 10, 0   @ Breach cleared: ApplyGuard re-announces the guard coming back up
	call EncScript_Suicune_ApplyGuard
EncScript_Suicune_TurnClose_Shed:
	encjumpifvar CMP_NOT_EQUAL, 0, 2, EncScript_Suicune_TurnClose_Regen
	call EncScript_Suicune_ShedStatus
EncScript_Suicune_TurnClose_Regen:
	call EncScript_Suicune_Regen
	encjumpifvar CMP_NOT_EQUAL, 6, 0, EncScript_Suicune_TurnClose_Dirty
	call EncScript_Suicune_Impure
	encjumpifvar CMP_EQUAL, 12, 1, EncScript_Suicune_TurnClose_Dirty
	call EncScript_Suicune_LoseFlow
	encjumpifvar CMP_GREATER_THAN, 9, 9, EncScript_Suicune_TurnClose_Calm
	encaddvar 9, 1   @ Calm, capped so a long fight cannot wrap the byte
EncScript_Suicune_TurnClose_Calm:
	call EncScript_Suicune_ApplyGuard
	goto EncScript_Suicune_TurnClose_Done
EncScript_Suicune_TurnClose_Dirty:
	encsetvar 9, 0   @ Calm
EncScript_Suicune_TurnClose_Done:
	return

// Phase 1 at 50% HP. The rite comes round faster and every sweep from here also drags a stage of
// Speed off the player, so a clean, patient game is no longer a free one.
EncScript_Suicune_Cleansing::
	encsetvar 0, 1   @ Phase 1
	printstring STRINGID_ENCSUICUNECLEANSING
	waitmessage B_WAIT_TIME_LONG
	playmoveanimation MOVE_SURF
	waitanimation
	@ The banner beat always plays, but the sweep is skipped if a Rite already swept this turn - the
	@ threshold can be crossed by a move on a turn that opened with a scheduled purification, and
	@ running the sequence twice back to back reads as a bug and takes two stages of Speed.
	encjumpifvar CMP_EQUAL, 13, 1, EncScript_Suicune_Cleansing_Rite
	call EncScript_Suicune_Purify
EncScript_Suicune_Cleansing_Rite:
	encsetvar 2, 2
	return

// Phase 2 at 20% HP. Purification moves to the end of every turn and the guard shuts at any Flow
// above 0, which is the moment the player learns the door only opens from empty. Per the spec there
// is no aggressive animation here - it simply stops, and everything goes calm.
EncScript_Suicune_SacredBeast::
	encsetvar 0, 2   @ Phase 2
	encsetvar 2, 0   @ Rite: StillWater takes over
	encsetvar 3, 0   @ Draw
	encsetsurvive ENC_TARGET_BOSS, TRUE
	printstring STRINGID_ENCSUICUNESACREDBEAST
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	call EncScript_Suicune_ApplyGuard
	return

// Below 10% in phase 2. Everything off, and Survive released so the catch window is a real one.
// CapTypeEffectiveness and FlatToxicDamage stay on as insurance for the window.
EncScript_Suicune_Weakened::
	encsetvar 0, 3   @ Phase 3
	encsetvar 1, 0   @ Flow
	encsetvar 2, 0   @ Rite
	encsetvar 3, 0   @ Draw
	encsetvar 4, 0   @ Break
	encsetvar 9, 0   @ Calm
	encsetvar 10, 0  @ Breach
	encsetvar 13, 0  @ Swept
	encsetsurvive ENC_TARGET_BOSS, FALSE
	encsetimmunity ENC_TARGET_BOSS, 0
	encsetdamagereduction ENC_TARGET_BOSS, 78
	encsetvar 5, 78
	encsetcatchrate 30
	encsetballs ENC_BALLS_ALLOWED
	printstring STRINGID_ENCSUICUNEWEAKENED
	waitmessage B_WAIT_TIME_LONG
	return
