#include "global.h"
#include "route_tracker.h"
#include "battle_setup.h"
#include "event_data.h"
#include "pokedex.h"
#include "pokemon.h"
#include "wild_encounter.h"
#include "constants/event_bg.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/trainer_types.h"
#include "constants/trainers.h"

// Route Stat Tracker: per-map completion counts for the start menu box (see route_tracker.h).
// Wild Pokémon counting mirrors CapturedAllLandMons/CapturedAllWaterMons/CapturedAllHiddenMons
// in dexnav.c, generalized to all encounter methods and unioned across all times of day.

struct AreaSlots
{
    enum WildPokemonArea area;
    u8 count;
};

static const struct AreaSlots sAreaSlots[] =
{
    { WILD_AREA_LAND,    LAND_WILD_COUNT   },
    { WILD_AREA_WATER,   WATER_WILD_COUNT  },
    { WILD_AREA_ROCKS,   ROCK_WILD_COUNT   },
    { WILD_AREA_FISHING, FISH_WILD_COUNT   },
    { WILD_AREA_HIDDEN,  HIDDEN_WILD_COUNT },
};

static const struct WildPokemonInfo *GetAreaMonsInfo(u32 headerId, enum TimeOfDay timeOfDay, enum WildPokemonArea area)
{
    const struct WildEncounterTypes *types = &gWildMonHeaders[headerId].encounterTypes[timeOfDay];

    switch (area)
    {
    case WILD_AREA_LAND:
        return types->landMonsInfo;
    case WILD_AREA_WATER:
        return types->waterMonsInfo;
    case WILD_AREA_ROCKS:
        return types->rockSmashMonsInfo;
    case WILD_AREA_FISHING:
        return types->fishingMonsInfo;
    case WILD_AREA_HIDDEN:
        return types->hiddenMonsInfo;
    default:
        return NULL;
    }
}

static enum Species GetEncounterSpecies(u32 headerId, enum TimeOfDay timeOfDay, enum WildPokemonArea area, u8 slot)
{
    const struct WildPokemonInfo *info = GetAreaMonsInfo(headerId, timeOfDay, area);

    if (info == NULL)
        return SPECIES_NONE;

    // Same randomization point CreateMon uses, so the tracked species matches what's actually encountered.
    return GetRandomizedSpecies(info->wildPokemon[slot].species);
}

// Backwards scan through the canonical (timeOfDay, area, slot) enumeration order, stopping just
// before the given position. No storage needed since every entry can be regenerated on demand.
static bool32 SpeciesSeenEarlier(u32 headerId, enum Species species, u8 uptoTod, u8 uptoAreaIdx, u8 uptoSlot)
{
    u8 tod, areaIdx, slot, slotCount;

    for (tod = 0; tod <= uptoTod; tod++)
    {
        for (areaIdx = 0; areaIdx < ARRAY_COUNT(sAreaSlots); areaIdx++)
        {
            if (tod == uptoTod && areaIdx > uptoAreaIdx)
                break;

            slotCount = (tod == uptoTod && areaIdx == uptoAreaIdx) ? uptoSlot : sAreaSlots[areaIdx].count;
            for (slot = 0; slot < slotCount; slot++)
            {
                if (GetEncounterSpecies(headerId, tod, sAreaSlots[areaIdx].area, slot) == species)
                    return TRUE;
            }
        }
    }

    return FALSE;
}

static void CountEncounters(struct RouteProgress *progress)
{
    u32 headerId = GetCurrentMapWildMonHeaderId();
    u8 tod, areaIdx, slot;
    enum Species species;

    if (headerId == HEADER_NONE)
        return;

    for (tod = 0; tod < TIMES_OF_DAY_COUNT; tod++)
    {
        for (areaIdx = 0; areaIdx < ARRAY_COUNT(sAreaSlots); areaIdx++)
        {
            for (slot = 0; slot < sAreaSlots[areaIdx].count; slot++)
            {
                species = GetEncounterSpecies(headerId, tod, sAreaSlots[areaIdx].area, slot);
                if (species == SPECIES_NONE)
                    continue;
                if (SpeciesSeenEarlier(headerId, species, tod, areaIdx, slot))
                    continue;

                progress->speciesTotal++;
                if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    progress->speciesCaught++;
            }
        }
    }
}

static void CountItems(struct RouteProgress *progress)
{
    u32 i;
    const struct MapEvents *events = gMapHeader.events;

    if (events == NULL)
        return;

    for (i = 0; i < events->bgEventCount; i++)
    {
        const struct BgEvent *bgEvent = &events->bgEvents[i];
        if (bgEvent->kind != BG_EVENT_HIDDEN_ITEM)
            continue;

        progress->itemsTotal++;
        if (FlagGet(FLAG_HIDDEN_ITEMS_START + bgEvent->bgUnion.hiddenItem.hiddenItemId))
            progress->itemsCollected++;
    }

    for (i = 0; i < events->objectEventCount; i++)
    {
        const struct ObjectEventTemplate *objectEvent = &events->objectEvents[i];
        // flagId != 0 excludes flagless Battle Pyramid balls, which don't belong to a fixed map.
        if (objectEvent->graphicsId != OBJ_EVENT_GFX_ITEM_BALL || objectEvent->flagId == 0)
            continue;

        progress->itemsTotal++;
        if (FlagGet(objectEvent->flagId))
            progress->itemsCollected++;
    }
}

// Backwards scan for trainer ID dedup (twins/double-battle pairs share one ID).
static bool32 TrainerIdSeenEarlier(const struct ObjectEventTemplate *objectEvents, u32 uptoIndex, u16 trainerId)
{
    u32 i;
    u16 otherId;

    for (i = 0; i < uptoIndex; i++)
    {
        if (objectEvents[i].trainerType == TRAINER_TYPE_NONE || objectEvents[i].script == NULL)
            continue;

        otherId = GetTrainerFlagFromScript(objectEvents[i].script);
        if (otherId != TRAINER_NONE && otherId == trainerId)
            return TRUE;
    }

    return FALSE;
}

static void CountTrainers(struct RouteProgress *progress)
{
    u32 i;
    u16 trainerId;
    const struct MapEvents *events = gMapHeader.events;

    if (events == NULL)
        return;

    for (i = 0; i < events->objectEventCount; i++)
    {
        const struct ObjectEventTemplate *objectEvent = &events->objectEvents[i];
        if (objectEvent->trainerType == TRAINER_TYPE_NONE || objectEvent->script == NULL)
            continue;

        trainerId = GetTrainerFlagFromScript(objectEvent->script);
        if (trainerId == TRAINER_NONE)
            continue;
        if (TrainerIdSeenEarlier(events->objectEvents, i, trainerId))
            continue;

        progress->trainersTotal++;
        if (FlagGet(TRAINER_FLAGS_START + trainerId))
            progress->trainersDefeated++;
    }
}

bool32 GetCurrentRouteProgress(struct RouteProgress *progress)
{
    progress->speciesCaught = 0;
    progress->speciesTotal = 0;
    progress->itemsCollected = 0;
    progress->itemsTotal = 0;
    progress->trainersDefeated = 0;
    progress->trainersTotal = 0;

    CountEncounters(progress);
    CountItems(progress);
    CountTrainers(progress);

    return (progress->speciesTotal != 0 || progress->itemsTotal != 0 || progress->trainersTotal != 0);
}
