#include "global.h"
#include "draft_mode.h"
#include "achievements.h"
#include "battle_util.h"
#include "event_data.h"
#include "limited_party.h"
#include "mono_type.h"
#include "mono_gen.h"
#include "new_game.h"
#include "caps.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "string_util.h"
#include "wild_encounter.h"
#include "event_scripts.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/region_map_sections.h"


bool32 Draft_IsEnabled(void)
{
    return gSaveBlock1Ptr->draftModeEnabled != 0;
}

bool32 Draft_IsActive(void)
{
    // Not FLAG_SYS_POKEMON_GET (set when the starter is chosen): Draft waits for
    // the Pokédex, the script node that also sets FLAG_NUZLOCKE_CATCH_MODE.
    return Draft_IsEnabled() && FlagGet(FLAG_SYS_POKEDEX_GET);
}

// Scratch entry bound: NUM_LAND_MONS_ENCOUNTER_SLOTS (12) + NUM_WATER_MONS_ENCOUNTER_SLOTS (5),
// with headroom for times of day pointing at different tables. Overflow is
// clamped by AddSpeciesToScratch.
#define DRAFT_SCRATCH_CAPACITY 24

// Land/water info pointers usually repeat across times of day; track visited
// ones so the common case is one pass.
#define DRAFT_MAX_VISITED_INFOS (TIMES_OF_DAY_COUNT * 2)

static void AddSpeciesToScratch(struct DraftChoice *scratch, u32 *count, u16 species, u16 minLevel, u16 maxLevel)
{
    u32 i;

    for (i = 0; i < *count; i++)
    {
        if (scratch[i].species == species)
        {
            if (minLevel < scratch[i].minLevel)
                scratch[i].minLevel = minLevel;
            if (maxLevel > scratch[i].maxLevel)
                scratch[i].maxLevel = maxLevel;
            return;
        }
    }

    if (*count < DRAFT_SCRATCH_CAPACITY)
    {
        scratch[*count].species = species;
        scratch[*count].minLevel = minLevel;
        scratch[*count].maxLevel = maxLevel;
        (*count)++;
    }
    // Pool is at the scratch ceiling; clamp rather than overrun.
}

static bool32 InfoAlreadyVisited(const struct WildPokemonInfo *info, const struct WildPokemonInfo **visited, u32 *visitedCount)
{
    u32 i;

    for (i = 0; i < *visitedCount; i++)
    {
        if (visited[i] == info)
            return TRUE;
    }

    if (*visitedCount < DRAFT_MAX_VISITED_INFOS)
        visited[(*visitedCount)++] = info;

    return FALSE;
}

static void AccumulateWildInfo(const struct WildPokemonInfo *info, u32 wildCount, struct DraftChoice *scratch, u32 *count,
                                const struct WildPokemonInfo **visited, u32 *visitedCount)
{
    u32 i;

    if (info == NULL || info->wildPokemon == NULL)
        return;

    if (InfoAlreadyVisited(info, visited, visitedCount))
        return;

    for (i = 0; i < wildCount; i++)
        AddSpeciesToScratch(scratch, count, info->wildPokemon[i].species, info->wildPokemon[i].minLevel, info->wildPokemon[i].maxLevel);
}

static void SortIndicesAscending(u8 *indices, u32 n)
{
    u32 i, j;

    for (i = 1; i < n; i++)
    {
        u8 key = indices[i];
        j = i;
        while (j > 0 && indices[j - 1] > key)
        {
            indices[j] = indices[j - 1];
            j--;
        }
        indices[j] = key;
    }
}

// Reduces an overflowing pool to DRAFT_MAX_CHOICES with a partial Fisher-Yates
// shuffle seeded from trainer ID and the area's MAPSEC (same idiom as
// PickRandomSpecies in src/ui_birch_case.c; re-rolls on New Game+ via
// GetNewGamePlusLevelOffset()). Chosen indices are sorted to keep encounter-table order.
static u32 SelectDraftSubset(const struct DraftChoice *scratch, u32 count, struct DraftChoice *out)
{
    u8 indices[DRAFT_SCRATCH_CAPACITY];
    u32 i;
    u32 trainerId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    mapsec_u8_t zone = GetCurrentRegionMapSectionId();
    rng_value_t rngState = LocalRandomSeed(trainerId + (u32)zone * 131 + GetNewGamePlusLevelOffset());

    for (i = 0; i < count; i++)
        indices[i] = i;

    for (i = 0; i < DRAFT_MAX_CHOICES; i++)
    {
        u32 j = i + (LocalRandom(&rngState) % (count - i));
        u8 temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    SortIndicesAscending(indices, DRAFT_MAX_CHOICES);

    for (i = 0; i < DRAFT_MAX_CHOICES; i++)
        out[i] = scratch[indices[i]];

    return DRAFT_MAX_CHOICES;
}

// Accumulates the current map's raw wild species (land + water, all times of
// day, deduped, highest maxLevel kept) into `scratch`. Not Mono-filtered: this is
// the ground truth for Draft_IsAreaDraftable. See Draft_BuildPool for the offered pool.
static u32 BuildRawPool(struct DraftChoice *scratch)
{
    const struct WildPokemonInfo *visited[DRAFT_MAX_VISITED_INFOS];
    u32 visitedCount = 0;
    u32 count = 0;
    u32 t;
    u16 headerId = GetCurrentMapWildMonHeaderId();

    if (headerId == HEADER_NONE)
        return 0;

    for (t = 0; t < TIMES_OF_DAY_COUNT; t++)
    {
        const struct WildEncounterTypes *types = &gWildMonHeaders[headerId].encounterTypes[t];

        AccumulateWildInfo(types->landMonsInfo, NUM_LAND_MONS_ENCOUNTER_SLOTS, scratch, &count, visited, &visitedCount);
        AccumulateWildInfo(types->waterMonsInfo, NUM_WATER_MONS_ENCOUNTER_SLOTS, scratch, &count, visited, &visitedCount);
    }

    return count;
}

// Compacts `pool` in place to species legal under Mono Type/Mono Gen (when
// enabled), returning the new count. Enforces those modes on a draft pick, as
// MonoType_SetGiveBlocked does for gifts (src/script_pokemon_util.c).
static u32 FilterPoolForMono(struct DraftChoice *pool, u32 count)
{
    u32 i, kept;
    bool32 monoOn = MonoType_IsEnabled();
    bool32 genOn = MonoGen_IsEnabled();

    if (!monoOn && !genOn)
        return count;

    for (i = 0, kept = 0; i < count; i++)
    {
        if ((!monoOn || MonoType_IsSpeciesAllowed(pool[i].species))
         && (!genOn || MonoGen_IsSpeciesAllowed(pool[i].species)))
            pool[kept++] = pool[i];
    }

    return kept;
}

// Compacts `pool` in place to species whose minLevel doesn't exceed the level
// cap, returning the new count. A species partly above the cap is kept;
// ResolveLevelsForCap clamps its roll.
static u32 FilterPoolForLevelCap(struct DraftChoice *pool, u32 count)
{
    u32 i, kept;
    u32 levelCap = GetCurrentLevelCap();

    for (i = 0, kept = 0; i < count; i++)
    {
        if (pool[i].minLevel <= levelCap)
            pool[kept++] = pool[i];
    }

    return kept;
}

// Rolls each entry's offered `level` in [minLevel, min(maxLevel, level cap)].
// Runs after FilterPoolForLevelCap, so minLevel <= levelCap holds.
static void ResolveLevelsForCap(struct DraftChoice *pool, u32 count)
{
    u32 i;
    u32 levelCap = GetCurrentLevelCap();

    for (i = 0; i < count; i++)
    {
        u32 loLevel = pool[i].minLevel;
        u32 hiLevel = pool[i].maxLevel < levelCap ? pool[i].maxLevel : levelCap;

        if (hiLevel < loLevel)
            hiLevel = loLevel;

        pool[i].level = loLevel + (Random() % (hiLevel - loLevel + 1));
    }
}

u32 Draft_BuildPool(struct DraftChoice *out)
{
    struct DraftChoice scratch[DRAFT_SCRATCH_CAPACITY];
    u32 t;
    u32 count = FilterPoolForMono(scratch, BuildRawPool(scratch));

    count = FilterPoolForLevelCap(scratch, count);

    if (count == 0)
        return 0;

    if (count <= DRAFT_MAX_CHOICES)
    {
        for (t = 0; t < count; t++)
            out[t] = scratch[t];
        ResolveLevelsForCap(out, count);
        return count;
    }

    count = SelectDraftSubset(scratch, count, out);
    ResolveLevelsForCap(out, count);
    return count;
}

bool32 Draft_IsAreaDraftable(void)
{
    mapsec_u8_t zone;
    struct DraftChoice scratch[DRAFT_SCRATCH_CAPACITY];

    if (!Draft_IsActive())
        return FALSE;

    zone = GetCurrentRegionMapSectionId();
    if (zone == MAPSEC_NONE || zone == MAPSEC_DYNAMIC)
        return FALSE;

    if (GET_NUZLOCKE_ZONE_FLAG(zone))
        return FALSE;

    // Not mono-filtered: a pool that Mono Type/Gen empties must still fire the
    // field hook so Draft_EventScript_RouteDraft can report "nothing eligible".
    return BuildRawPool(scratch) > 0;
}

// gSpecialVar_Result: TRUE if the Mono-filtered pool has at least one legal
// pick. Draft_IsAreaDraftable() already guarantees unfiltered encounters exist.
void Draft_CheckPoolEligible(void)
{
    struct DraftChoice pool[DRAFT_MAX_CHOICES];

    gSpecialVar_Result = (Draft_BuildPool(pool) > 0);
}

// Marks the area spent with no offer made (Mono "nothing eligible" path).
// Unconditional, unlike Draft_MarkAreaSpent: no pending mon exists.
void Draft_MarkAreaSpentNoOffer(void)
{
    SET_NUZLOCKE_ZONE_FLAG(GetCurrentRegionMapSectionId());
}

static EWRAM_DATA struct Pokemon sDraftPendingMon = {0};
static EWRAM_DATA bool8 sDraftPendingValid = FALSE;
// TRUE when sDraftPendingMon is this area's own pick rather than a gift/egg.
// Read only by Draft_MarkAreaSpent.
static EWRAM_DATA bool8 sDraftPendingFromDraft = FALSE;

bool32 Draft_HasPendingMon(void)
{
    return sDraftPendingValid;
}

void Draft_QueuePendingMon(struct Pokemon *mon, bool32 fromDraft)
{
    CopyMon(&sDraftPendingMon, mon, sizeof(struct Pokemon));
    sDraftPendingValid = TRUE;
    sDraftPendingFromDraft = fromDraft;
}


// Mirrors the "find an empty slot" half of GiveCapturedMonToPlayer
// (src/pokemon.c): first empty slot below the Limited Party cap, else declined.
// Never falls back to the PC; a full party goes to the replace screen instead.
void Draft_TryGiveToEmptySlot(void)
{
    u8 maxSize = LimitedParty_GetMaxPartySize();
    u8 i;

    for (i = 0; i < maxSize; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == SPECIES_NONE)
            break;
    }

    if (i >= maxSize)
    {
        gSpecialVar_Result = 1;
        return;
    }

    CopyMon(&gParties[B_TRAINER_PLAYER][i], &sDraftPendingMon, sizeof(sDraftPendingMon));
    gPartiesCount[B_TRAINER_PLAYER] = i + 1;
    gSpecialVar_Result = 0;
    sDraftPendingValid = FALSE;
}

// gStringVar1 = the pending mon's display name. Uses GetMonNickname rather
// than a species-name lookup so a gift egg (e.g. from the daycare) reads "EGG"
// instead of revealing its species. Also sets gSpecialVar_0x8004 to the joined
// slot for `special ChangePokemonNickname`, as Common_EventScript_GetGiftMonPartySlot
// does (data/scripts/pc_transfer.inc). Only called after Draft_TryGiveToEmptySlot succeeds.
void Draft_BufferPendingNickname(void)
{
    GetMonNickname(&sDraftPendingMon, gStringVar1);
    gSpecialVar_0x8004 = gPartiesCount[B_TRAINER_PLAYER] - 1;
}

// gStringVar1 = outgoing party mon at gSpecialVar_0x8004 (from
// `special ChoosePartyMon`), gStringVar2 = the pending mon's display name.
// Nickname-safe for the same egg reason as Draft_BufferPendingNickname.
void Draft_BufferReplacementNames(void)
{
    GetMonNickname(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], gStringVar1);
    GetMonNickname(&sDraftPendingMon, gStringVar2);
}

// Replaces the party mon at gSpecialVar_0x8004 with the pending mon. Reverts
// battle forms first, as Cmd_givecaughtmon does (src/battle_script_commands.c).
void Draft_DoReplacement(void)
{
    u8 slot = gSpecialVar_0x8004;

    // The party was full to reach here and LIMITED_PARTY_BASE_SIZE (3) is the
    // smallest a full party can be, so the last mon is never removed. Trip wire
    // only.
    AGB_ASSERT(gPartiesCount[B_TRAINER_PLAYER] > 1);

    TryRevertPartyMonFormChange(slot);
    ZeroMonData(&gParties[B_TRAINER_PLAYER][slot]);
    CopyMon(&gParties[B_TRAINER_PLAYER][slot], &sDraftPendingMon, sizeof(sDraftPendingMon));
    // The mon has a home now; stop it reading as pending or the field hook loops.
    sDraftPendingValid = FALSE;

    Achievement_RecordDraftReplacement();
}

void Draft_DiscardPending(void)
{
    sDraftPendingValid = FALSE;
}

// Marks the area's draft as spent. Draft_EventScript_Finish is the only caller.
// No-ops for a gift/egg routed through the pending buffer, so accepting one on a
// fresh route doesn't burn that area's draft.
void Draft_MarkAreaSpent(void)
{
    // Finish is reached by every offer path (pick, decline, gift/egg). Draft
    // doesn't force autosave (see IsAutosaveHidden, src/option_menu.c), so only
    // arm one if the player opted in.
    if (gSaveBlock1Ptr->autosaveModeEnabled)
        gDoAutosave = TRUE;

    if (!sDraftPendingFromDraft)
        return;

    SET_NUZLOCKE_ZONE_FLAG(GetCurrentRegionMapSectionId());
    Achievement_RecordDraftCompleted();
}

// Field hook for ProcessPlayerFieldInput (src/field_control_avatar.c). A
// pending gift/egg takes priority over a fresh draft.
bool32 Draft_TryStartFieldScript(void)
{
    if (Draft_HasPendingMon())
    {
        ScriptContext_SetupScript(Draft_EventScript_ResolvePending);
        return TRUE;
    }

    if (Draft_IsAreaDraftable())
    {
        ScriptContext_SetupScript(Draft_EventScript_RouteDraft);
        return TRUE;
    }

    return FALSE;
}
