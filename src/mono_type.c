#include "global.h"
#include "mono_type.h"
#include "caps.h"
#include "mono_gen.h"
#include "new_game.h"
#include "pokemon.h"
#include "random.h"
#include "randomization.h"
#include "constants/pokemon.h"
#include "constants/species.h"

// Shared rules for the Mono Type challenge. See include/mono_type.h.

// The order the settings menu cycles through. TYPE_NONE is the "OFF" entry --
// it is the only way to turn the mode off, so it stays in the cycle. The 18
// real types follow alphabetically; TYPE_MYSTERY (10) and TYPE_STELLAR (20) are
// deliberately absent because no obtainable species carries them.
static const u8 sMonoTypeCycle[] =
{
    TYPE_NONE, // OFF
    TYPE_BUG,
    TYPE_DARK,
    TYPE_DRAGON,
    TYPE_ELECTRIC,
    TYPE_FAIRY,
    TYPE_FIGHTING,
    TYPE_FIRE,
    TYPE_FLYING,
    TYPE_GHOST,
    TYPE_GRASS,
    TYPE_GROUND,
    TYPE_ICE,
    TYPE_NORMAL,
    TYPE_POISON,
    TYPE_PSYCHIC,
    TYPE_ROCK,
    TYPE_STEEL,
    TYPE_WATER,
};

// Fallback ladder for the starter pool, applied in order until at least
// MONO_TYPE_STARTER_COUNT candidates exist. Every real type should clear the
// strictest level; the rest are purely defensive.
enum MonoTypeStarterFilterLevel
{
    MONO_STARTER_FILTER_STRICT,
    MONO_STARTER_FILTER_NO_BST_CAP,
    MONO_STARTER_FILTER_ALLOW_EVOLVED,
    MONO_STARTER_FILTER_ALLOW_LEGENDARY,
    MONO_STARTER_FILTER_COUNT,
};

// One bit per species, ~190 bytes of stack. Cheap enough to build once per
// draw and far cheaper than calling GetSpeciesPreEvolution() per candidate,
// which is itself O(NUM_SPECIES).
#define EVOLVED_BITMAP_SIZE ((NUM_SPECIES + 7) / 8)
#define SetEvolvedBit(bitmap, species) ((bitmap)[(species) / 8] |= 1 << ((species) % 8))
#define GetEvolvedBit(bitmap, species) ((bitmap)[(species) / 8] & (1 << ((species) % 8)))

static void BuildEvolvedSpeciesBitmap(u8 *bitmap);
static bool32 IsStarterCandidate(u16 species, const u8 *evolvedBitmap, u32 filterLevel);
static u32 CountStarterCandidates(const u8 *evolvedBitmap, u32 filterLevel);
static u16 GetStarterCandidateAtIndex(const u8 *evolvedBitmap, u32 filterLevel, u32 index);
static bool32 WasSpeciesAlreadyPicked(const u16 *picks, u32 count, u16 species);

// Set by the givemon/giveegg hooks, consumed by the shared "no room" script.
// See MonoType_SetGiveBlocked/MonoType_ConsumeGiveBlockedFlag in the header.
static bool8 sGiveBlockedFlag = FALSE;

bool32 MonoType_IsEnabled(void)
{
    return gSaveBlock2Ptr->monoTypeSetting != TYPE_NONE;
}

u8 MonoType_GetType(void)
{
    return gSaveBlock2Ptr->monoTypeSetting;
}

bool32 MonoType_IsSpeciesAllowed(u16 species)
{
    u8 monoType = MonoType_GetType();
    u8 type1, type2;

    if (monoType == TYPE_NONE)
        return TRUE;

    // Never block on something that isn't a real species; a caller passing
    // SPECIES_NONE or an out-of-range ID should fall through to whatever it
    // would have done with the mode off.
    if (species == SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return TRUE;

    // Must go through the resolver so the check agrees with FLAG_RANDOMIZE_TYPE
    // rather than the species' printed types.
    GetResolvedTypePair(species, &type1, &type2);

    return type1 == monoType || type2 == monoType;
}

u8 MonoType_CycleType(u8 current, bool8 forward)
{
    u32 index = 0;

    for (u32 i = 0; i < ARRAY_COUNT(sMonoTypeCycle); i++)
    {
        if (sMonoTypeCycle[i] == current)
        {
            index = i;
            break;
        }
    }

    if (forward)
        index = (index + 1) % ARRAY_COUNT(sMonoTypeCycle);
    else
        index = (index + ARRAY_COUNT(sMonoTypeCycle) - 1) % ARRAY_COUNT(sMonoTypeCycle);

    return sMonoTypeCycle[index];
}

void MonoType_PickStarterSpecies(u16 *out)
{
    u8 evolvedBitmap[EVOLVED_BITMAP_SIZE];
    u32 filterLevel;
    u32 candidateCount = 0;
    u32 trainerId;

    for (u32 i = 0; i < MONO_TYPE_STARTER_COUNT; i++)
        out[i] = SPECIES_NONE;

    if (!MonoType_IsEnabled())
        return;

    // Pass A: mark every species that something else evolves into.
    BuildEvolvedSpeciesBitmap(evolvedBitmap);

    // Pass B: find the strictest filter level that still yields a full trio.
    for (filterLevel = MONO_STARTER_FILTER_STRICT; filterLevel < MONO_STARTER_FILTER_COUNT; filterLevel++)
    {
        candidateCount = CountStarterCandidates(evolvedBitmap, filterLevel);
        if (candidateCount >= MONO_TYPE_STARTER_COUNT)
            break;
    }

    if (candidateCount == 0)
        return; // Nothing of this type exists at all; leave every slot empty.

    if (filterLevel == MONO_STARTER_FILTER_COUNT)
        filterLevel = MONO_STARTER_FILTER_ALLOW_LEGENDARY; // Fewer than 3 exist even fully relaxed.

    trainerId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);

    for (u32 slot = 0; slot < MONO_TYPE_STARTER_COUNT; slot++)
    {
        rng_value_t rngState;
        u16 species;

        if (slot >= candidateCount)
            break; // Pool smaller than the trio; the remaining slots stay SPECIES_NONE.

        // Fixed seed per slot, mirroring PickRandomSpecies(). Backing out of
        // the case with B re-runs InitializeStarterChoices(), so an unseeded
        // draw would let the player reroll the trio indefinitely.
        rngState = LocalRandomSeed(trainerId + slot + GetNewGamePlusLevelOffset());

        // Re-draws advance this slot's stream, so the loop terminates and the
        // result is still fixed for the save.
        do
        {
            species = GetStarterCandidateAtIndex(evolvedBitmap, filterLevel, LocalRandom(&rngState) % candidateCount);
        } while (WasSpeciesAlreadyPicked(out, slot, species));

        out[slot] = species;
    }
}

void MonoType_SetGiveBlocked(void)
{
    sGiveBlockedFlag = TRUE;
}

bool32 MonoType_ConsumeGiveBlockedFlag(void)
{
    bool32 wasBlocked = sGiveBlockedFlag;
    sGiveBlockedFlag = FALSE;
    return wasBlocked;
}

static void BuildEvolvedSpeciesBitmap(u8 *bitmap)
{
    memset(bitmap, 0, EVOLVED_BITMAP_SIZE);

    for (u32 species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
    {
        const struct Evolution *evolutions;

        // GetSpeciesEvolutions() sanitizes its argument, which asserts on a
        // disabled species, so gate on IsSpeciesEnabled() first.
        if (!IsSpeciesEnabled(species))
            continue;

        evolutions = GetSpeciesEvolutions(species);
        if (evolutions == NULL)
            continue;

        for (u32 i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
        {
            enum Species target = evolutions[i].targetSpecies;

            if (target != SPECIES_NONE && target < NUM_SPECIES)
                SetEvolvedBit(bitmap, target);
        }
    }
}

static bool32 IsStarterCandidate(u16 species, const u8 *evolvedBitmap, u32 filterLevel)
{
    const struct SpeciesInfo *info;

    if (!IsSpeciesEnabled(species))
        return FALSE;

    // No alternate forms, megas or regional variants in the pool.
    if (GET_BASE_SPECIES_ID(species) != species)
        return FALSE;

    if (!MonoType_IsSpeciesAllowed(species))
        return FALSE;

    if (!MonoGen_IsSpeciesAllowed(species))
        return FALSE;

    if (filterLevel < MONO_STARTER_FILTER_NO_BST_CAP
     && GetSpeciesBaseStatTotal(species) > MONO_TYPE_STARTER_MAX_BST)
        return FALSE;

    if (filterLevel < MONO_STARTER_FILTER_ALLOW_EVOLVED && GetEvolvedBit(evolvedBitmap, species))
        return FALSE;

    info = &gSpeciesInfo[species];

    // Totems are never a sensible starter, so they stay excluded at every level.
    if (info->isTotem)
        return FALSE;

    if (filterLevel < MONO_STARTER_FILTER_ALLOW_LEGENDARY
     && (info->isRestrictedLegendary
      || info->isSubLegendary
      || info->isMythical
      || info->isUltraBeast
      || info->isParadox))
        return FALSE;

    return TRUE;
}

static u32 CountStarterCandidates(const u8 *evolvedBitmap, u32 filterLevel)
{
    u32 count = 0;

    for (u32 species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
    {
        if (IsStarterCandidate(species, evolvedBitmap, filterLevel))
            count++;
    }

    return count;
}

static u16 GetStarterCandidateAtIndex(const u8 *evolvedBitmap, u32 filterLevel, u32 index)
{
    for (u32 species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
    {
        if (!IsStarterCandidate(species, evolvedBitmap, filterLevel))
            continue;
        if (index == 0)
            return species;
        index--;
    }

    return SPECIES_NONE;
}

static bool32 WasSpeciesAlreadyPicked(const u16 *picks, u32 count, u16 species)
{
    for (u32 i = 0; i < count; i++)
    {
        if (picks[i] == species)
            return TRUE;
    }

    return FALSE;
}
