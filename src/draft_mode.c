#include "global.h"
#include "draft_mode.h"
#include "battle_util.h"
#include "event_data.h"
#include "limited_party.h"
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

// Shared rules for the Draft challenge. See include/draft_mode.h.

bool32 Draft_IsEnabled(void)
{
    return gSaveBlock1Ptr->draftModeEnabled != 0;
}

bool32 Draft_IsActive(void)
{
    return Draft_IsEnabled() && FlagGet(FLAG_SYS_POKEMON_GET);
}

// Upper bound on scratch entries while a pool is being built: LAND_WILD_COUNT
// (12) + WATER_WILD_COUNT (5) with headroom for the fact that different
// times of day can point at different tables. Overflow beyond this is
// clamped rather than overrun - see AddSpeciesToScratch.
#define DRAFT_SCRATCH_CAPACITY 24

// Land + water info pointers can repeat across the four times of day (most
// maps point every time of day at the same tables); track which ones this
// call has already accumulated so the common case is one pass, not four.
#define DRAFT_MAX_VISITED_INFOS (TIMES_OF_DAY_COUNT * 2)

static void AddSpeciesToScratch(struct DraftChoice *scratch, u32 *count, u16 species, u8 level)
{
    u32 i;

    for (i = 0; i < *count; i++)
    {
        if (scratch[i].species == species)
        {
            if (level > scratch[i].level)
                scratch[i].level = level;
            return;
        }
    }

    if (*count < DRAFT_SCRATCH_CAPACITY)
    {
        scratch[*count].species = species;
        scratch[*count].level = level;
        (*count)++;
    }
    // else: pool is already at the scratch ceiling. Doesn't happen with real
    // encounter tables, but clamp rather than overrun if it ever does.
}

// Returns TRUE (without touching scratch) if `info` was already accumulated
// earlier in this Draft_BuildPool call.
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
        AddSpeciesToScratch(scratch, count, info->wildPokemon[i].species, info->wildPokemon[i].maxLevel);
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

// Reduces an overflowing scratch pool to DRAFT_MAX_CHOICES entries. The
// subset is chosen with a partial Fisher-Yates shuffle over the pool's
// indices, seeded deterministically from the trainer ID and the current
// area's MAPSEC - the same deterministic-seed idiom PickRandomSpecies uses
// in src/ui_birch_case.c. Stable across soft-resets, different every
// playthrough, and re-rolls on a New Game+ cycle via
// GetNewGamePlusLevelOffset(). The chosen indices are then sorted ascending
// so the result stays in encounter-table order.
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

u32 Draft_BuildPool(struct DraftChoice *out)
{
    struct DraftChoice scratch[DRAFT_SCRATCH_CAPACITY];
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

        AccumulateWildInfo(types->landMonsInfo, LAND_WILD_COUNT, scratch, &count, visited, &visitedCount);
        AccumulateWildInfo(types->waterMonsInfo, WATER_WILD_COUNT, scratch, &count, visited, &visitedCount);
    }

    if (count == 0)
        return 0;

    if (count <= DRAFT_MAX_CHOICES)
    {
        for (t = 0; t < count; t++)
            out[t] = scratch[t];
        return count;
    }

    return SelectDraftSubset(scratch, count, out);
}

bool32 Draft_IsAreaDraftable(void)
{
    mapsec_u8_t zone;
    struct DraftChoice pool[DRAFT_MAX_CHOICES];

    if (!Draft_IsActive())
        return FALSE;

    zone = GetCurrentRegionMapSectionId();
    if (zone == MAPSEC_NONE || zone == MAPSEC_DYNAMIC)
        return FALSE;

    if (GET_NUZLOCKE_ZONE_FLAG(zone))
        return FALSE;

    return Draft_BuildPool(pool) > 0;
}

// See the header comment above Draft_HasPendingMon/Draft_QueuePendingMon.
static EWRAM_DATA struct Pokemon sDraftPendingMon = {0};
static EWRAM_DATA bool8 sDraftPendingValid = FALSE;
// TRUE when sDraftPendingMon came from this area's own draft pick rather
// than a gift/egg funnelled in instead of going to the PC. Read only by
// Draft_MarkAreaSpent - see the fromDraft note on Draft_QueuePendingMon.
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

// ---------------------------------------------------------------------
// Script natives for the offer flow. See data/scripts/draft.inc and the
// declarations in include/draft_mode.h.
// ---------------------------------------------------------------------

// Mirrors GiveCapturedMonToPlayer's "find an empty slot" half
// (src/pokemon.c) - first empty slot below the Limited Party cap, or a
// declined placement if none is free. Never falls back to the PC: a Draft
// run has nowhere else for a Pokémon to go, so the caller (§5's
// Draft_EventScript_ResolvePending) routes a full party to the replace
// screen instead.
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
}

// gStringVar1 = the pending mon's species name (not its nickname - it
// doesn't have one yet). Also buffers gSpecialVar_0x8004 with the party
// slot the mon just joined, so Draft_EventScript_OfferNickname can hand it
// straight to `special ChangePokemonNickname`, the same convention
// Common_EventScript_GetGiftMonPartySlot uses (data/scripts/pc_transfer.inc).
// Only ever called right after Draft_TryGiveToEmptySlot succeeds, so the
// pending mon and the party count it reads are both guaranteed current.
void Draft_BufferPendingNickname(void)
{
    u16 species = GetMonData(&sDraftPendingMon, MON_DATA_SPECIES);

    StringCopy(gStringVar1, GetSpeciesName(species));
    gSpecialVar_0x8004 = gPartiesCount[B_TRAINER_PLAYER] - 1;
}

// gStringVar1 = the outgoing party mon at gSpecialVar_0x8004 (as chosen by
// `special ChoosePartyMon`), gStringVar2 = the pending mon's species name.
void Draft_BufferReplacementNames(void)
{
    u16 species = GetMonData(&sDraftPendingMon, MON_DATA_SPECIES);

    GetMonNickname(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], gStringVar1);
    StringCopy(gStringVar2, GetSpeciesName(species));
}

// Replaces the party mon at gSpecialVar_0x8004 with the pending mon.
// TryRevertPartyMonFormChange runs first, as Cmd_givecaughtmon does before
// releasing a mon to the PC (src/battle_script_commands.c) - a battle-form
// mon should always revert before it leaves the party for good.
void Draft_DoReplacement(void)
{
    u8 slot = gSpecialVar_0x8004;

    TryRevertPartyMonFormChange(slot);
    ZeroMonData(&gParties[B_TRAINER_PLAYER][slot]);
    CopyMon(&gParties[B_TRAINER_PLAYER][slot], &sDraftPendingMon, sizeof(sDraftPendingMon));
}

// Discards the pending mon without placing it anywhere - the decline path.
void Draft_DiscardPending(void)
{
    sDraftPendingValid = FALSE;
}

// Marks the current area's draft as spent. Draft_EventScript_Finish is the
// offer flow's single terminal node and its only caller - see the header
// comment on this function in include/draft_mode.h. No-ops for a gift/egg
// that got routed through the pending buffer instead of the PC (§6b) -
// otherwise accepting a gift on a fresh route would silently burn that
// area's own draft.
void Draft_MarkAreaSpent(void)
{
    if (!sDraftPendingFromDraft)
        return;

    SET_NUZLOCKE_ZONE_FLAG(GetCurrentRegionMapSectionId());
}

// Field hook for ProcessPlayerFieldInput (src/field_control_avatar.c), run
// every frame the player has field control. A pending mon (gift/egg) takes
// priority over a fresh draft so the two never race for the same frame - see
// the header comment on this function in include/draft_mode.h.
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
