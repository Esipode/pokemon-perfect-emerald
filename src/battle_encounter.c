#include "global.h"
#include "battle_encounter.h"
#include "caps.h"
#include "pokemon.h"
#include "constants/battle_ai.h"
#include "data/battle_encounters.h"

// firedTriggers is a u32 bitmap; one bit per trigger.
STATIC_ASSERT(MAX_ENCOUNTER_TRIGGERS == 32, EncounterFiredTriggersBitmapMismatch);
// A review tripwire, not a hardware limit: EncounterRuntime is embedded in BattleStruct and
// zero-cleared with it every battle, so growth here is silent and worth being made to notice. The
// 64 this started at was set when the struct held little more than an id and a trigger bitmap; the
// type-keyed adaptation board (adaptType/adaptPercent, 32 bytes) is what took it past that. 92 of
// 128 used - raise this again deliberately, not to get a build through.
STATIC_ASSERT(sizeof(struct EncounterRuntime) <= 128, EncounterRuntimeTooLarge);

// Which struct EncounterEvent fields a checkpoint's dispatcher actually populates. Grows as each
// checkpoint's call site is built out; a 0 row means the checkpoint has no event data at all
// (e.g. ENC_ON_BATTLE_START - nothing has happened yet).
static const u8 sCheckpointEventFields[ENC_CHECKPOINT_COUNT] =
{
    [ENC_ON_BATTLE_START] = 0,
    [ENC_ON_TURN_START]   = 0,
    [ENC_ON_MOVE_END]     = ENC_EVENT_BATTLER | ENC_EVENT_TARGET | ENC_EVENT_MOVE
                          | ENC_EVENT_CAUSE   | ENC_EVENT_VALUES,
    [ENC_ON_FAINT]        = ENC_EVENT_BATTLER | ENC_EVENT_CAUSE,
    [ENC_ON_SWITCH_IN]    = ENC_EVENT_BATTLER,
    [ENC_ON_TURN_END]     = ENC_EVENT_BATTLER | ENC_EVENT_CAUSE | ENC_EVENT_VALUES,
    [ENC_ON_BATTLE_END]   = 0,
};

// Resolves a battler reference to a concrete battler id. Shared with Stage 15's command targeting.
bool32 ResolveEncounterBattlerRef(u32 ref, u8 *battlerOut)
{
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    enum BattlerId battler;

    switch (ref)
    {
    case ENC_BOSS:
    case ENC_OPPONENT_LEFT:
        battler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        break;
    case ENC_SELF:
        battler = runtime->event.battler;
        break;
    case ENC_PLAYER_LEFT:
        battler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        break;
    case ENC_PLAYER_RIGHT:
        battler = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
        break;
    case ENC_OPPONENT_RIGHT:
        battler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
        break;
    default:
        errorf("encounter %d: unknown battler ref %d", runtime->id, ref);
        return FALSE;
    }

    // GetBattlerAtPosition returns gBattlersCount when no battler holds that position - the
    // _RIGHT refs in a singles battle, in particular. Never resolve to that; it isn't a battler.
    assertf(battler < gBattlersCount, "encounter %d: battler ref %d has no battler in this battle", runtime->id, ref)
    {
        return FALSE;
    }

    *battlerOut = battler;
    return TRUE;
}

// Resolves an EncounterTarget to a battler bitmask (Stage 15). See include/battle_encounter.h.
u32 ResolveEncounterTarget(enum EncounterTarget target)
{
    u8 battler;
    u8 boss;
    u32 mask;
    enum BattleSide bossSide;

    switch (target)
    {
    case ENC_TARGET_BOSS:
        return ResolveEncounterBattlerRef(ENC_BOSS, &battler) ? (1u << battler) : 0;
    case ENC_TARGET_SELF:
        return ResolveEncounterBattlerRef(ENC_SELF, &battler) ? (1u << battler) : 0;
    case ENC_TARGET_PLAYER_LEFT:
        return ResolveEncounterBattlerRef(ENC_PLAYER_LEFT, &battler) ? (1u << battler) : 0;
    case ENC_TARGET_PLAYER_RIGHT:
        return ResolveEncounterBattlerRef(ENC_PLAYER_RIGHT, &battler) ? (1u << battler) : 0;
    case ENC_TARGET_OPPONENT_LEFT:
        return ResolveEncounterBattlerRef(ENC_OPPONENT_LEFT, &battler) ? (1u << battler) : 0;
    case ENC_TARGET_OPPONENT_RIGHT:
        return ResolveEncounterBattlerRef(ENC_OPPONENT_RIGHT, &battler) ? (1u << battler) : 0;

    case ENC_TARGET_EVENT_TARGET:
    {
        s32 value;
        if (!GetEncounterEventField(ENC_EVENT_TARGET, &value))
            return 0;
        battler = value;
        // Mirrors ResolveEncounterBattlerRef's own guard: event.target is read as a raw battler id,
        // not looked up through GetBattlerAtPosition, so it needs the same bounds check here.
        assertf(battler < gBattlersCount,
                "encounter %d: event target %d has no battler in this battle", gBattleStruct->encounter.id, battler)
        {
            return 0;
        }
        return (1u << battler);
    }

    case ENC_TARGET_ALL_FOES:
    case ENC_TARGET_ALL_ALLIES:
        // Both are relative to the boss's side - there's no actor battler to be relative to
        // instead (Step 3: no actor field), so ENC_BOSS itself must resolve for either to mean
        // anything.
        if (!ResolveEncounterBattlerRef(ENC_BOSS, &boss))
            return 0;
        bossSide = GetBattlerSide(boss);
        mask = 0;
        for (battler = 0; battler < gBattlersCount; battler++)
        {
            bool32 onBossSide = (GetBattlerSide(battler) == bossSide);
            if (target == ENC_TARGET_ALL_ALLIES ? onBossSide : !onBossSide)
                mask |= (1u << battler);
        }
        return mask;

    case ENC_TARGET_ALL_BATTLERS:
        mask = 0;
        for (battler = 0; battler < gBattlersCount; battler++)
            mask |= (1u << battler);
        return mask;

    default:
        errorf("encounter %d: unknown target %d", gBattleStruct->encounter.id, target);
        return 0;
    }
}

// TRUE for the set-valued targets. A state-changing command applying one of these skips a
// fainted/absent battler in the resolved set silently - a group whose membership shifts as
// battlers faint (a foe KO'd by the same move that triggered an OnMoveEnd checkpoint, before its
// replacement is in) is normal, unlike naming one dead battler outright, which stays an assert.
bool32 IsEncounterGroupTarget(enum EncounterTarget target)
{
    return target == ENC_TARGET_ALL_FOES
        || target == ENC_TARGET_ALL_ALLIES
        || target == ENC_TARGET_ALL_BATTLERS;
}

// Reads one operand for condition evaluation. See include/battle_encounter.h.
s32 GetEncounterOperand(enum EncounterOperand operand, u32 arg, bool32 useSnapshot)
{
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    u8 battler;
    s32 value;

    switch (operand)
    {
    case ENC_OP_HP:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        return useSnapshot ? runtime->prevHp[battler] : gBattleMons[battler].hp;

    case ENC_OP_HP_PERCENT:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        if (gBattleMons[battler].maxHP == 0)
            return 0;
        return (s32)(useSnapshot ? runtime->prevHp[battler] : gBattleMons[battler].hp) * 100 / gBattleMons[battler].maxHP;

    case ENC_OP_MAX_HP:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        return gBattleMons[battler].maxHP;

    case ENC_OP_SPECIES:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        return gBattleMons[battler].species;

    case ENC_OP_ABILITY:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        return gBattleMons[battler].ability;

    case ENC_OP_STATUS:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        return gBattleMons[battler].status1;

    case ENC_OP_STAT_STAGE:
        if (!ResolveEncounterBattlerRef(ENC_UNPACK_STAT_BATTLER(arg), &battler))
            return 0;
        return gBattleMons[battler].statStages[ENC_UNPACK_STAT_ID(arg)];

    case ENC_OP_TYPE:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        return GetBattlerType(battler, 0, FALSE);

    // The whole-turn latch, not gProtectStructs - see struct EncounterRuntime. Protect is a
    // whole-turn shield, so "protected at any point this turn" and "is protected" say the same thing.
    case ENC_OP_PROTECTED:
        if (!ResolveEncounterBattlerRef(arg, &battler))
            return 0;
        return (runtime->protectedThisTurn >> battler) & 1;

    case ENC_OP_WEATHER:
        return gBattleWeather;

    case ENC_OP_TERRAIN:
        return gFieldStatuses & STATUS_FIELD_TERRAIN_ANY;

    case ENC_OP_TURN:
        return gBattleTurnCounter;

    case ENC_OP_BATTLER_COUNT:
        return gBattlersCount;

    case ENC_OP_VAR:
        assertf(arg < MAX_ENCOUNTER_VARS, "encounter %d: var index %d out of range", runtime->id, arg)
        {
            return 0;
        }
        return gEncounterVars[arg];

    case ENC_OP_EVENT_BATTLER:
        return GetEncounterEventField(ENC_EVENT_BATTLER, &value) ? value : 0;

    case ENC_OP_EVENT_TARGET:
        return GetEncounterEventField(ENC_EVENT_TARGET, &value) ? value : 0;

    case ENC_OP_EVENT_MOVE:
        return GetEncounterEventField(ENC_EVENT_MOVE, &value) ? value : 0;

    // Reads through ENC_EVENT_MOVE so it inherits that field's per-checkpoint validity mask, and
    // asserts at the same wrong checkpoints ENC_OP_EVENT_MOVE does. This is the move's base type,
    // not its runtime type after Normalize/-ate, Electrify or Tera.
    case ENC_OP_EVENT_MOVE_TYPE:
        return GetEncounterEventField(ENC_EVENT_MOVE, &value) ? GetMoveType(value) : 0;

    // Same validity mask as ENC_OP_EVENT_MOVE_TYPE above. This is the move's base category, not a
    // runtime flip (Photon Geyser, Tera Blast).
    case ENC_OP_EVENT_MOVE_CATEGORY:
        return GetEncounterEventField(ENC_EVENT_MOVE, &value) ? GetMoveCategory(value) : 0;

    case ENC_OP_EVENT_CAUSE:
        return GetEncounterEventField(ENC_EVENT_CAUSE, &value) ? value : 0;

    case ENC_OP_EVENT_OLD_VALUE:
        return GetEncounterEventField(ENC_EVENT_OLD_VALUE, &value) ? value : 0;

    case ENC_OP_EVENT_NEW_VALUE:
        return GetEncounterEventField(ENC_EVENT_NEW_VALUE, &value) ? value : 0;

    default:
        errorf("encounter %d: unknown operand %d", gBattleStruct->encounter.id, operand);
        return 0;
    }
}

// Returns the number of array entries node spans, without evaluating any leaf comparison. Lets
// ENC_OP_ALL/ENC_OP_ANY's short-circuit still walk (but never evaluate) the siblings it skips,
// which is what keeps the parent's consumed length correct without duplicating EvalNode's walk.
static u32 SkipNode(const struct EncounterCondition *node, u32 depth)
{
    u32 consumed;
    u32 i;

    switch (node->operand)
    {
    case ENC_OP_ALL:
    case ENC_OP_ANY:
        assertf(depth < MAX_ENCOUNTER_COND_DEPTH,
                "encounter %d: condition nesting deeper than %d",
                gBattleStruct->encounter.id, MAX_ENCOUNTER_COND_DEPTH)
        {
            return 1;
        }
        consumed = 1;
        for (i = 0; i < node->arg; i++)
        {
            assertf(node[consumed].operand != ENC_OP_COUNT,
                    "encounter %d: condition group claims more children than exist",
                    gBattleStruct->encounter.id)
            {
                return consumed;
            }
            consumed += SkipNode(node + consumed, depth + 1);
        }
        return consumed;

    case ENC_OP_NOT:
        assertf(depth < MAX_ENCOUNTER_COND_DEPTH,
                "encounter %d: condition nesting deeper than %d",
                gBattleStruct->encounter.id, MAX_ENCOUNTER_COND_DEPTH)
        {
            return 1;
        }
        assertf(node[1].operand != ENC_OP_COUNT,
                "encounter %d: NOT has no child to invert",
                gBattleStruct->encounter.id)
        {
            return 1;
        }
        return 1 + SkipNode(node + 1, depth + 1);

    default: // leaf
        return 1;
    }
}

// Returns the number of array entries node spans; writes node's result to *out. depth counts group
// nesting (ALL/ANY/NOT), bounded against the stack budget in Free Space.md Sec0.1 - a leaf never
// recurses, so it isn't itself subject to the bound.
static u32 EvalNode(const struct EncounterCondition *node, bool32 useSnapshot, u32 depth, bool32 *out)
{
    switch (node->operand)
    {
    case ENC_OP_ALL:
    case ENC_OP_ANY:
    {
        // ALL starts TRUE and short-circuits on the first FALSE child; ANY starts FALSE and
        // short-circuits on the first TRUE child - one loop serves both by short-circuiting the
        // moment a child's result matches the operand's own "deciding" value. Once decided, the
        // remaining children are walked via SkipNode (measured, not evaluated) so the consumed
        // length is still correct and a skipped child's assertion never trips.
        bool32 decidesOn = (node->operand == ENC_OP_ANY);
        bool32 result = !decidesOn;
        bool32 decided = FALSE;
        u32 consumed = 1;
        u32 i;

        assertf(depth < MAX_ENCOUNTER_COND_DEPTH,
                "encounter %d: condition nesting deeper than %d",
                gBattleStruct->encounter.id, MAX_ENCOUNTER_COND_DEPTH)
        {
            *out = FALSE;
            return 1;
        }

        for (i = 0; i < node->arg; i++)
        {
            assertf(node[consumed].operand != ENC_OP_COUNT,
                    "encounter %d: condition group claims more children than exist",
                    gBattleStruct->encounter.id)
            {
                *out = FALSE;
                return consumed;
            }

            if (decided)
            {
                consumed += SkipNode(node + consumed, depth + 1);
            }
            else
            {
                bool32 childResult;
                consumed += EvalNode(node + consumed, useSnapshot, depth + 1, &childResult);
                if (childResult == decidesOn)
                {
                    result = decidesOn;
                    decided = TRUE;
                }
            }
        }

        *out = result;
        return consumed;
    }

    case ENC_OP_NOT:
    {
        bool32 childResult;
        u32 consumed;

        assertf(depth < MAX_ENCOUNTER_COND_DEPTH,
                "encounter %d: condition nesting deeper than %d",
                gBattleStruct->encounter.id, MAX_ENCOUNTER_COND_DEPTH)
        {
            *out = FALSE;
            return 1;
        }
        assertf(node[1].operand != ENC_OP_COUNT,
                "encounter %d: NOT has no child to invert",
                gBattleStruct->encounter.id)
        {
            *out = FALSE;
            return 1;
        }

        consumed = 1 + EvalNode(node + 1, useSnapshot, depth + 1, &childResult);
        *out = !childResult;
        return consumed;
    }

    default: // leaf - Stage 11's comparison
    {
        s32 lhs = GetEncounterOperand(node->operand, node->arg, useSnapshot);

        switch (node->cmp)
        {
        case ENC_CMP_EQ: *out = (lhs == node->value); break;
        case ENC_CMP_NE: *out = (lhs != node->value); break;
        case ENC_CMP_LT: *out = (lhs <  node->value); break;
        case ENC_CMP_LE: *out = (lhs <= node->value); break;
        case ENC_CMP_GT: *out = (lhs >  node->value); break;
        case ENC_CMP_GE: *out = (lhs >= node->value); break;
        default:
            errorf("encounter %d: unknown comparison %d", gBattleStruct->encounter.id, node->cmp);
            *out = FALSE;
            break;
        }
        return 1;
    }
    }
}

// NULL -> always eligible. Otherwise ANDs every top-level node in conds - a leaf or a group tree
// (Stage 12) - short-circuiting on the first failure. A conditions array with no group node is
// exactly Stage 11's flat AND-list, evaluated identically.
bool32 EvaluateConditions(const struct EncounterCondition *conds, bool32 useSnapshot)
{
    const struct EncounterCondition *node = conds;

    if (conds == NULL)
        return TRUE;

    while (node->operand != ENC_OP_COUNT)
    {
        bool32 result;
        node += EvalNode(node, useSnapshot, 0, &result);
        if (!result)
            return FALSE;
    }

    return TRUE;
}

#if TESTING
static const struct Encounter *sTestEncounter;

void TestSetEncounter(const struct Encounter *encounter)
{
    sTestEncounter = encounter;
}
#endif

const struct Encounter *GetEncounter(enum EncounterId id)
{
#if TESTING
    if (sTestEncounter != NULL)
        return sTestEncounter;
#endif
    assertf(id > ENCOUNTER_NONE && id < ENCOUNTER_COUNT, "invalid encounter id %d", id)
    {
        return NULL;
    }
    assertf(gEncounters[id].triggerCount <= MAX_ENCOUNTER_TRIGGERS, "encounter %d has too many triggers", id)
    {
        return NULL;
    }
    return &gEncounters[id];
}

static void UpdateEncounterCatchGuard(void);

// Selects the highest-priority (lowest value) eligible trigger for checkpoint, marks it
// fired if it's ENC_TRIGGER_ONCE, and returns its script. Ties break by table order.
// Call sites re-invoke this after each returned script runs, re-evaluating against the
// new state, until it returns NULL - see include/battle_encounter.h for the ordering
// guarantees that loop relies on.
const u8 *TryRunEncounterCheckpoint(enum EncounterCheckpoint checkpoint)
{
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    const struct EncounterTrigger *best = NULL;
    u32 bestIndex = 0;

    if (!IsEncounterActive())
        return NULL;

    // Entering a new checkpoint resets the runaway guard. Event-carrying checkpoints have their
    // context set by the call site immediately before the first dispatch (so pass 1's conditions
    // can read it), so it must NOT be cleared here for those - only for checkpoints that carry no
    // event, where a stale field left by a prior checkpoint could otherwise leak into a read.
    if (checkpoint != runtime->checkpoint)
    {
        runtime->checkpoint = checkpoint;
        runtime->scriptsThisCheckpoint = 0;
        if (checkpoint >= ENC_CHECKPOINT_COUNT || sCheckpointEventFields[checkpoint] == 0)
            memset(&runtime->event, 0, sizeof(runtime->event));
    }

    // ENC_ON_BATTLE_START is dispatched exactly once, and this is its first pass (nothing has
    // fired at it yet this battle) - snapshot pre-battle HP now, before any trigger's script can
    // change it. Without this, prevHp[] would still read its zeroed initial value and a boss
    // starting below a threshold would look like it just crossed it.
    if (checkpoint == ENC_ON_BATTLE_START && runtime->scriptsThisCheckpoint == 0)
    {
        for (u32 i = 0; i < MAX_BATTLERS_COUNT; i++)
            runtime->prevHp[i] = gBattleMons[i].hp;

        // Seed the encounter's properties here too - the earliest point every battler exists, and
        // before any trigger's script can run and change what the properties would have set.
        ApplyEncounterBattlerProperties();
    }

    // Re-check the catch-window damage guard against HP as of the last checkpoint, before triggers
    // run: a script this checkpoint may change the boss's reduction, and the guard owns that value
    // while a Poke Ball can be thrown.
    UpdateEncounterCatchGuard();

    const struct Encounter *encounter = GetEncounter(runtime->id);
    if (encounter == NULL)
        return NULL;

    for (u32 i = 0; i < encounter->triggerCount; i++)
    {
        const struct EncounterTrigger *trigger = &encounter->triggers[i];
        bool32 invalid = FALSE;

        assertf(trigger->checkpoint < ENC_CHECKPOINT_COUNT, "encounter %d trigger %d: invalid checkpoint %d", runtime->id, i, trigger->checkpoint)
        {
            invalid = TRUE;
        }
        assertf(trigger->script != NULL, "encounter %d trigger %d: NULL script", runtime->id, i)
        {
            invalid = TRUE;
        }
        if (invalid)
            continue;

        if (trigger->checkpoint != checkpoint)
            continue;
        if (runtime->firedTriggers & (1u << i))
            continue;
        if (!EvaluateConditions(trigger->conditions, FALSE))
            continue;

        if (trigger->flags & ENC_TRIGGER_ON_ENTER)
        {
            // Edge semantics: eligible only on the FALSE -> TRUE transition. useSnapshot = TRUE
            // evaluates the same conditions against prevHp - "as of the last checkpoint" - so a
            // level condition that was already true last checkpoint doesn't re-fire just because
            // it's still true now.
            if (EvaluateConditions(trigger->conditions, TRUE))
                continue;
        }

        if (best == NULL || trigger->priority < best->priority)
        {
            best = trigger;
            bestIndex = i;
        }
    }

    if (best == NULL)
    {
        // All of this checkpoint's dispatch passes are done - snapshot HP now, "since the last
        // checkpoint" being the granularity an author reasons about. Must happen at the END of
        // dispatch, not the start: updating here instead of on checkpoint entry is what lets a
        // trigger's own script change HP without that change being invisible to ON_ENTER at the
        // very next checkpoint.
        for (u32 i = 0; i < MAX_BATTLERS_COUNT; i++)
            runtime->prevHp[i] = gBattleMons[i].hp;
        return NULL;
    }

    // Runaway guard: a trigger that never disables itself would be re-selected forever and hang the
    // game, so turn that into a loud assert instead. Checked here rather than on dispatch entry so
    // it only fires when a trigger is actually eligible - a checkpoint that legitimately runs the
    // full budget would otherwise trip on the trailing discovery pass, which found nothing and was
    // about to return NULL on its own.
    assertf(runtime->scriptsThisCheckpoint < MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT,
            "encounter %d: %d scripts ran at checkpoint %d - runaway trigger chain?",
            runtime->id, runtime->scriptsThisCheckpoint, checkpoint)
    {
        return NULL;
    }

    if (best->flags & ENC_TRIGGER_ONCE)
        runtime->firedTriggers |= (1u << bestIndex);

    runtime->scriptsThisCheckpoint++;
    return best->script;
}

// Populates the current checkpoint's event context. Caller only needs to pass the fields its
// checkpoint's row in sCheckpointEventFields actually marks valid; the rest are ignored by
// GetEncounterEventField regardless of what's passed here.
void SetEncounterEvent(u8 battler, u8 target, u16 move, enum EncounterEventCause cause, s16 oldValue, s16 newValue)
{
    struct EncounterEvent *event = &gBattleStruct->encounter.event;

    event->battler = battler;
    event->target = target;
    event->move = move;
    event->cause = cause;
    event->oldValue = oldValue;
    event->newValue = newValue;
}

// Reads one field of the current checkpoint's event context. See include/battle_encounter.h.
bool32 GetEncounterEventField(u32 field, s32 *out)
{
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    u32 validityBit = (field == ENC_EVENT_OLD_VALUE || field == ENC_EVENT_NEW_VALUE) ? ENC_EVENT_VALUES : field;

    assertf(runtime->checkpoint < ENC_CHECKPOINT_COUNT, "invalid checkpoint %d", runtime->checkpoint)
    {
        return FALSE;
    }
    assertf(sCheckpointEventFields[runtime->checkpoint] & validityBit,
            "event field %d not valid at checkpoint %d", field, runtime->checkpoint)
    {
        return FALSE;
    }

    switch (field)
    {
    case ENC_EVENT_BATTLER:
        *out = runtime->event.battler;
        break;
    case ENC_EVENT_TARGET:
        *out = runtime->event.target;
        break;
    case ENC_EVENT_MOVE:
        *out = runtime->event.move;
        break;
    case ENC_EVENT_CAUSE:
        *out = runtime->event.cause;
        break;
    case ENC_EVENT_OLD_VALUE:
        *out = runtime->event.oldValue;
        break;
    case ENC_EVENT_NEW_VALUE:
        *out = runtime->event.newValue;
        break;
    default:
        errorf("unknown encounter event field %d", field);
        return FALSE;
    }
    return TRUE;
}

// Author-defined script variables. Fixed EWRAM array outside gBattleStruct so sENCOUNTER_VAR
// (constants/battle_encounter.h) is a link-time constant address a battle script can encode - see
// the struct EncounterRuntime comment (battle.h). 16 bytes, unconditional.
EWRAM_DATA u8 gEncounterVars[MAX_ENCOUNTER_VARS] = {0};

void ResetEncounterVars(void)
{
    memset(gEncounterVars, 0, sizeof(gEncounterVars));
}

// --- Encounter properties ------------------------------------------------------------------------

// The properties of the encounter this battle is running, or NULL when there isn't one. Every
// property hook goes through this, so an inactive/invalid encounter costs one comparison and
// changes nothing.
static const struct EncounterProperties *GetEncounterProperties(void)
{
    const struct Encounter *encounter;

    if (!IsEncounterActive())
        return NULL;

    encounter = GetEncounter(gBattleStruct->encounter.id);
    if (encounter == NULL)
        return NULL;

    return &encounter->properties;
}

// Rebuilds one party Pokemon at level. Experience is reset to the exact threshold for that level
// (the same GetExperienceAtLevel call CreateBoxMon uses) so the level and the experience bar agree,
// then stats are recalculated and HP refilled - the mon is being defined at this level, not healed
// or levelled up mid-run.
static void SetPartyMonLevel(struct Pokemon *mon, u16 level)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u32 exp = GetExperienceAtLevel(gSpeciesInfo[species].growthRate, level);
    u16 hp;

    SetMonData(mon, MON_DATA_EXP, &exp);
    SetMonData(mon, MON_DATA_LEVEL, &level);
    CalculateMonStats(mon);

    hp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
    SetMonData(mon, MON_DATA_HP, &hp);
}

// The opponent parties, and only those - enum BattleTrainer interleaves B_TRAINER_PARTNER between
// the two opponent slots, so this can't be a plain range over the enum.
static const enum BattleTrainer sOpponentTrainers[] = { B_TRAINER_OPPONENT_A, B_TRAINER_OPPONENT_B };

void ApplyEncounterLevelOverride(void)
{
    const struct EncounterProperties *properties = GetEncounterProperties();
    u32 level;

    if (properties == NULL || properties->level == ENC_LEVEL_NONE)
        return;

    // GetProgressionLevelCap, not GetCurrentLevelCap: a player who turned their own cap off gets
    // MAX_LEVEL back from the latter, which would put the boss thousands of levels above them.
    // The progression cap is what the encounter is actually being balanced against (caps.c).
    level = (properties->level == ENC_LEVEL_CAP) ? GetProgressionLevelCap() : properties->level;
    if (level < 1)
        level = 1;
    else if (level > MAX_LEVEL)
        level = MAX_LEVEL;

    for (u32 t = 0; t < ARRAY_COUNT(sOpponentTrainers); t++)
    {
        for (u32 i = 0; i < PARTY_SIZE; i++)
        {
            struct Pokemon *mon = &gParties[sOpponentTrainers[t]][i];
            if (GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE
             || GetMonData(mon, MON_DATA_IS_EGG, NULL))
                continue;
            SetPartyMonLevel(mon, level);
        }
    }
}

void ApplyEncounterMoveOverride(void)
{
    const struct EncounterProperties *properties = GetEncounterProperties();
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];

    if (properties == NULL)
        return;

    if (GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE
     || GetMonData(mon, MON_DATA_IS_EGG, NULL))
        return;

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = properties->moves[i];
        u8 pp;

        if (move == MOVE_NONE)
            continue;

        // PP is set from the move's own maximum rather than carried over from whatever occupied the
        // slot, and PP Ups aren't applied - a boss is being defined here, not taught a move.
        pp = GetMovePP(move);
        SetMonData(mon, MON_DATA_MOVE1 + i, &move);
        SetMonData(mon, MON_DATA_PP1 + i, &pp);
    }
}

void ApplyEncounterAbilityOverride(void)
{
    const struct EncounterProperties *properties = GetEncounterProperties();
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    enum Species species;

    if (properties == NULL || properties->ability == ENC_ABILITY_NONE)
        return;

    species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    if (species == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG, NULL))
        return;

    // A party mon stores an ability slot, not an ability, so this only works for an ability the
    // species actually has. Anything else falls through to ApplyEncounterBattlerAbilityOverride,
    // which writes it onto the battler once one exists.
    for (u32 slot = 0; slot < NUM_ABILITY_SLOTS; slot++)
    {
        if (GetSpeciesAbility(species, slot) == properties->ability)
        {
            SetMonData(mon, MON_DATA_ABILITY_NUM, &slot);
            return;
        }
    }
}

void ApplyEncounterBattlerAbilityOverride(enum BattlerId battler)
{
    const struct EncounterProperties *properties = GetEncounterProperties();

    if (properties == NULL || properties->ability == ENC_ABILITY_NONE)
        return;

    // Scoped to the mon, not to the boss position: after a switch a different Pokemon stands in the
    // opponent's left slot, and it isn't the one the encounter's Ability: describes.
    if (GetBattlerMon(battler) != &gParties[B_TRAINER_OPPONENT_A][0])
        return;

    // The species owns this ability, so the party's ability slot already carried it here.
    if (gBattleMons[battler].ability == properties->ability)
        return;

    // overwrittenAbility is set alongside it so readers that would otherwise re-derive the ability
    // from the species' slots - the AI's ability guessing above all - see the real one.
    gBattleMons[battler].ability = properties->ability;
    gBattleMons[battler].volatiles.overwrittenAbility = properties->ability;
}

u64 GetEncounterAiFlags(void)
{
    const struct EncounterProperties *properties = GetEncounterProperties();

    return (properties != NULL) ? properties->aiFlags : 0;
}

void ApplyEncounterBattlerProperties(void)
{
    const struct EncounterProperties *properties = GetEncounterProperties();
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    u8 boss;

    if (properties == NULL)
        return;

    runtime->ballPolicy = properties->ballPolicy;
    runtime->catchRate = properties->catchRate;

    if (properties->damageReduction == 0 && properties->immunities == 0
     && !properties->capTypeEffectiveness && !properties->flatToxicDamage && !properties->survive)
        return;

    // Properties describe the encounter's subject, so they seed the boss and nobody else. A script
    // that wants a modifier on any other battler sets it with encsetdamagereduction/encsetimmunity/
    // encsetcaptypeeffectiveness/encsetflattoxicdamage.
    if (!ResolveEncounterBattlerRef(ENC_BOSS, &boss))
        return;

    SetEncounterDamageReduction(boss, properties->damageReduction);
    SetEncounterImmunities(boss, properties->immunities);
    SetEncounterCapTypeEffectiveness(boss, properties->capTypeEffectiveness);
    SetEncounterFlatToxicDamage(boss, properties->flatToxicDamage);
    SetEncounterSurvive(boss, properties->survive);
}

void SetEncounterDamageReduction(enum BattlerId battler, u32 percent)
{
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    u8 boss = 0;

    assertf(percent <= ENC_MAX_DAMAGE_REDUCTION,
            "encounter %d: damage reduction %d above the maximum of %d percent",
            gBattleStruct->encounter.id, percent, ENC_MAX_DAMAGE_REDUCTION)
    {
        percent = ENC_MAX_DAMAGE_REDUCTION;
    }

    // While the catch-window guard is holding the boss's reduction at the maximum, a script setting
    // the boss's reduction changes the value the guard will restore, not the live one.
    if (runtime->catchGuard && ResolveEncounterBattlerRef(ENC_BOSS, &boss) && battler == boss)
    {
        runtime->catchGuardDr = percent;
        return;
    }

    runtime->damageReduction[battler] = percent;
}

// The catch-window damage guard, absolute for every encounter rather than a property. While an
// encounter allows Poke Balls the boss takes the maximum reduced damage so a stray hit - an attack,
// a status tick, an ability - can't kill the Pokemon the player is trying to catch. Any HP the boss
// recovers, for any reason, lifts the guard and restores the encounter's own reduction; the guard
// re-arms once the boss is back in the catchable state at a checkpoint where it hasn't just healed.
static void UpdateEncounterCatchGuard(void)
{
    struct EncounterRuntime *runtime = &gBattleStruct->encounter;
    bool32 catchable = (runtime->ballPolicy == ENC_BALLS_ALLOWED);
    bool32 healed;
    u8 boss;

    // Nothing to arm until Poke Balls are allowed, and nothing to lift unless the guard is holding.
    if (!catchable && !runtime->catchGuard)
        return;

    if (!ResolveEncounterBattlerRef(ENC_BOSS, &boss) || gBattleMons[boss].hp == 0)
        return;

    healed = (gBattleMons[boss].hp > runtime->prevHp[boss]);

    if (runtime->catchGuard)
    {
        if (healed || !catchable)
        {
            runtime->damageReduction[boss] = runtime->catchGuardDr;
            runtime->catchGuard = FALSE;
        }
    }
    else if (catchable && !healed)
    {
        runtime->catchGuardDr = runtime->damageReduction[boss];
        runtime->damageReduction[boss] = ENC_MAX_DAMAGE_REDUCTION;
        runtime->catchGuard = TRUE;
    }
}

void SetEncounterImmunities(enum BattlerId battler, u32 immunities)
{
    assertf((immunities & ~ENC_IMMUNE_ALL) == 0,
            "encounter %d: unknown immunity bits %d", gBattleStruct->encounter.id, immunities & ~ENC_IMMUNE_ALL)
    {
        immunities &= ENC_IMMUNE_ALL;
    }
    gBattleStruct->encounter.immunities[battler] = immunities;
}

void SetEncounterCapTypeEffectiveness(enum BattlerId battler, bool32 cap)
{
    gBattleStruct->encounter.capTypeEffectiveness[battler] = (cap != FALSE);
}

bool32 DoesEncounterCapTypeEffectiveness(enum BattlerId battler)
{
    if (!IsEncounterActive() || battler >= MAX_BATTLERS_COUNT)
        return FALSE;

    return gBattleStruct->encounter.capTypeEffectiveness[battler];
}

void SetEncounterFlatToxicDamage(enum BattlerId battler, bool32 flat)
{
    gBattleStruct->encounter.flatToxicDamage[battler] = (flat != FALSE);
}

bool32 DoesEncounterFlattenToxicDamage(enum BattlerId battler)
{
    if (!IsEncounterActive() || battler >= MAX_BATTLERS_COUNT)
        return FALSE;

    return gBattleStruct->encounter.flatToxicDamage[battler];
}

void SetEncounterSurvive(enum BattlerId battler, bool32 survive)
{
    gBattleStruct->encounter.survive[battler] = (survive != FALSE);
}

bool32 DoesEncounterSurvive(enum BattlerId battler)
{
    if (!IsEncounterActive() || battler >= MAX_BATTLERS_COUNT)
        return FALSE;

    return gBattleStruct->encounter.survive[battler];
}

s32 ApplyEncounterDamageReduction(enum BattlerId battler, s32 damage)
{
    u32 percent;

    // A non-positive amount is a heal (SetPassiveDamageAmount's callers encode healing as a
    // negative), and reducing a heal is never what "takes less damage" means. The bounds check
    // covers struct DamageContext's 3-bit battler fields, which are wider than MAX_BATTLERS_COUNT.
    if (damage <= 0 || battler >= MAX_BATTLERS_COUNT || !IsEncounterActive())
        return damage;

    percent = gBattleStruct->encounter.damageReduction[battler];
    if (percent > ENC_MAX_DAMAGE_REDUCTION)
        percent = ENC_MAX_DAMAGE_REDUCTION;
    if (percent != 0)
    {
        damage = damage * (100 - percent) / 100;
        if (damage < 1)
            damage = 1;
    }

    // Survive: this battler's HP can't be taken below 1. Clamped after the reduction so it judges the
    // damage actually about to land, and before GetAdjustedDamage, whose `hp > damage` guard then
    // short-circuits. At 1 HP this yields 0 - the same value False Swipe already produces there.
    if (gBattleStruct->encounter.survive[battler] && damage >= gBattleMons[battler].hp)
        damage = gBattleMons[battler].hp - 1;

    return damage;
}

// Scales healing the boss drains OUT OF another battler by the encounter's AUTHORED damage
// reduction. DamageReduction: cuts what reaches the boss, but nothing cuts what the boss deals - and
// a drain move turns that undiminished damage straight into healing. At DamageReduction: 90 the
// exchange runs ten to one against the player: Leech Seed alone takes 1/8 of their max HP every turn
// and hands all of it to a boss they can only chip one or two percent off, so the fight quietly
// stops being winnable and never looks like a bug. This puts the boss's healing on the same footing
// as its guard - it keeps the same fraction of what it drains that it lets through of what it takes.
//
// Reads the AUTHORED property rather than the live per-battler value on purpose. The live number
// moves with a phase, a stance or a form, and the catch-window guard pins it to
// ENC_MAX_DAMAGE_REDUCTION - balance would swing with all of that, and the catch window would zero
// out drain healing outright. The property is the fight's fixed balance constant, so this is too.
//
// Self-healing is deliberately untouched (sourceBattler == battler): Ingrain, Aqua Ring, Synthesis
// and Recover are a fraction of the boss's OWN max HP, already balanced against how long the fight
// runs, and are not the asymmetry. Nor is a Liquid Ooze punish, which is damage rather than healing
// and is already scaled once by ApplyEncounterDamageReduction on the way in - so the call sites
// apply this only on the branch that actually heals.
s32 ApplyEncounterDrainReduction(enum BattlerId battler, enum BattlerId sourceBattler, s32 heal)
{
    const struct Encounter *encounter;
    u8 boss;
    u32 percent;

    if (heal <= 0 || battler == sourceBattler || battler >= MAX_BATTLERS_COUNT || !IsEncounterActive())
        return heal;

    // Boss-scoped because DamageReduction: is. The boss is the only battler a Properties: block
    // configures, so it is the only one with an authored number to read; a reduction a script hands
    // some other battler has no property behind it and leaves its drains alone.
    if (!ResolveEncounterBattlerRef(ENC_BOSS, &boss) || battler != boss)
        return heal;

    encounter = GetEncounter(gBattleStruct->encounter.id);
    if (encounter == NULL)
        return heal;

    percent = encounter->properties.damageReduction;
    if (percent > ENC_MAX_DAMAGE_REDUCTION)
        percent = ENC_MAX_DAMAGE_REDUCTION;
    if (percent != 0)
    {
        heal = heal * (100 - percent) / 100;
        if (heal < 1)
            heal = 1;
    }

    return heal;
}

// ANALYSIS. Scales damage aimed at battler by the adaptation it holds for this move's type, if any.
// The board is a short unordered array, so a linear scan is cheaper than any index would be.
s32 ApplyEncounterTypeAdaptation(enum BattlerId battler, enum Type moveType, s32 damage)
{
    u32 i, percent;

    // TYPE_NONE is the empty-slot marker, so a typeless hit must never be allowed to match one.
    if (damage <= 0 || moveType == TYPE_NONE || battler >= MAX_BATTLERS_COUNT || !IsEncounterActive())
        return damage;

    for (i = 0; i < ENC_MAX_ADAPTATIONS; i++)
    {
        if (gBattleStruct->encounter.adaptType[battler][i] != moveType)
            continue;

        percent = gBattleStruct->encounter.adaptPercent[battler][i];
        if (percent > ENC_MAX_ADAPT_PERCENT)
            percent = ENC_MAX_ADAPT_PERCENT;
        damage = damage * (100 - percent) / 100;
        if (damage < 1)
            damage = 1;   // floored the same way the flat reduction is - never a silent no-op
        break;
    }

    return damage;
}

bool32 DoesEncounterGrantImmunity(enum BattlerId battler, u32 immunity)
{
    if (!IsEncounterActive() || battler >= MAX_BATTLERS_COUNT)
        return FALSE;

    return (gBattleStruct->encounter.immunities[battler] & immunity) == immunity;
}

bool32 IsEncounterBlockingBalls(void)
{
    if (!IsEncounterActive())
        return FALSE;

    return gBattleStruct->encounter.ballPolicy == ENC_BALLS_BLOCKED;
}

u32 GetEncounterCatchRate(void)
{
    if (!IsEncounterActive())
        return ENC_CATCH_RATE_NONE;

    return gBattleStruct->encounter.catchRate;
}

// Set by the overworld script that starts the battle, taken exactly once by battle start.
EWRAM_DATA static enum EncounterId sPendingEncounter = ENCOUNTER_NONE;

void SetPendingBattleEncounter(enum EncounterId id)
{
    sPendingEncounter = id;
}

enum EncounterId TakePendingBattleEncounter(void)
{
    enum EncounterId id = sPendingEncounter;
    sPendingEncounter = ENCOUNTER_NONE;
    return id;
}
