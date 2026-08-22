#include "global.h"
#include "battle_encounter.h"
#include "data/battle_encounters.h"

// firedTriggers is a u32 bitmap; one bit per trigger.
STATIC_ASSERT(MAX_ENCOUNTER_TRIGGERS == 32, EncounterFiredTriggersBitmapMismatch);
STATIC_ASSERT(sizeof(struct EncounterRuntime) <= 64, EncounterRuntimeTooLarge);

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

    // Entering a new checkpoint resets the runaway guard and clears the event context; repeated
    // calls for the same checkpoint (the re-evaluation loop) keep accumulating against the guard
    // and must NOT re-clear the event, or a later pass loses what an earlier pass was reacting to.
    if (checkpoint != runtime->checkpoint)
    {
        runtime->checkpoint = checkpoint;
        runtime->scriptsThisCheckpoint = 0;
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
    }

    assertf(runtime->scriptsThisCheckpoint < MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT,
            "encounter %d: %d scripts ran at checkpoint %d - runaway trigger chain?",
            runtime->id, runtime->scriptsThisCheckpoint, checkpoint)
    {
        return NULL;
    }

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
