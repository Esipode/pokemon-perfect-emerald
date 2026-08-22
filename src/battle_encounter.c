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
        // conditions always pass here; Stage 11 adds real evaluation.

        if (best == NULL || trigger->priority < best->priority)
        {
            best = trigger;
            bestIndex = i;
        }
    }

    if (best == NULL)
        return NULL;

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
