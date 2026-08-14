#include "global.h"
#include "draft_mode.h"
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

// Shared rules for the Draft challenge. See include/draft_mode.h.

bool32 Draft_IsEnabled(void)
{
    return gSaveBlock1Ptr->draftModeEnabled != 0;
}

bool32 Draft_IsActive(void)
{
    // Not FLAG_SYS_POKEMON_GET (set the moment the starter is chosen) - Draft
    // doesn't engage until the player is back in the lab with their Pokédex
    // in hand, after the first rival battle. FLAG_SYS_POKEDEX_GET is set at
    // LittlerootTown_ProfessorBirchsLab_EventScript_ReceivePokedex, the same
    // script node that sets FLAG_NUZLOCKE_CATCH_MODE - so this genuinely
    // mirrors when Nuzlocke's own catch restrictions start engaging, not
    // just the "player has a Pokémon" convention the old flag suggested.
    return Draft_IsEnabled() && FlagGet(FLAG_SYS_POKEDEX_GET);
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

// Accumulates the current map's raw wild-encounter species (land + water,
// unioned across all four times of day, deduped by species and keeping the
// highest maxLevel seen) into `scratch`. Not filtered by Mono Type/Mono
// Gen - this is "does this area have wild encounters at all", the ground
// truth Draft_IsAreaDraftable uses to decide whether the field hook engages
// here in the first place, independent of whether any of those species
// turn out to be legal under a Mono restriction. See Draft_BuildPool for
// the mono-filtered pool actually offered to the player.
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

        AccumulateWildInfo(types->landMonsInfo, LAND_WILD_COUNT, scratch, &count, visited, &visitedCount);
        AccumulateWildInfo(types->waterMonsInfo, WATER_WILD_COUNT, scratch, &count, visited, &visitedCount);
    }

    return count;
}

// Compacts `pool` in place down to the species legal under Mono Type/Mono
// Gen (only checked when those modes are actually enabled), returning the
// new count. A Draft pool otherwise offers whatever lives in the area
// regardless of those restrictions - this is what actually enforces them on
// a draft pick, the same job MonoType_SetGiveBlocked does for gifts
// (src/script_pokemon_util.c). A no-op copy when neither mode is enabled.
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

u32 Draft_BuildPool(struct DraftChoice *out)
{
    struct DraftChoice scratch[DRAFT_SCRATCH_CAPACITY];
    u32 t;
    u32 count = FilterPoolForMono(scratch, BuildRawPool(scratch));

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
    struct DraftChoice scratch[DRAFT_SCRATCH_CAPACITY];

    if (!Draft_IsActive())
        return FALSE;

    zone = GetCurrentRegionMapSectionId();
    if (zone == MAPSEC_NONE || zone == MAPSEC_DYNAMIC)
        return FALSE;

    if (GET_NUZLOCKE_ZONE_FLAG(zone))
        return FALSE;

    // Raw wild-encounter presence, NOT mono-filtered: an area whose pool
    // Mono Type/Gen reduces to zero still needs the field hook to fire so
    // Draft_EventScript_RouteDraft can show the "nothing eligible here"
    // message (Draft_CheckPoolEligible) instead of silently never engaging.
    return BuildRawPool(scratch) > 0;
}

// gSpecialVar_Result: TRUE if this area's Mono-filtered draft pool has at
// least one legal pick, FALSE if Mono Type/Mono Gen filtered out every
// species that lives here. Draft_IsAreaDraftable() already guarantees the
// area has *some* wild encounters before this runs (unfiltered) - this is
// what lets Draft_EventScript_RouteDraft pick between the normal offer and
// Draft_EventScript_NoEligiblePool.
void Draft_CheckPoolEligible(void)
{
    struct DraftChoice pool[DRAFT_MAX_CHOICES];

    gSpecialVar_Result = (Draft_BuildPool(pool) > 0);
}

// Marks the current area's draft as spent with no offer ever having been
// made - the Mono Type/Mono Gen "nothing eligible here" path
// (Draft_EventScript_NoEligiblePool) is the only caller. Unlike
// Draft_MarkAreaSpent, unconditional: there's no pending mon to key off of,
// since the case UI never opened.
void Draft_MarkAreaSpentNoOffer(void)
{
    SET_NUZLOCKE_ZONE_FLAG(GetCurrentRegionMapSectionId());
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
    sDraftPendingValid = FALSE;
}

// gStringVar1 = the pending mon's display name. Goes through GetMonNickname
// rather than a MON_DATA_SPECIES + GetSpeciesName lookup on purpose: a fresh
// draft pick has no custom nickname yet, so GetMonNickname just returns its
// species name anyway, but a gift/egg routed in by §6b (GiveScriptedMonToPlayer
// can hand over an unhatched egg, e.g. from the daycare) needs this to read
// "EGG" rather than spoil the species hidden inside - GetMonNickname already
// special-cases MON_DATA_NICKNAME to do that (src/pokemon.c). Also buffers
// gSpecialVar_0x8004 with the party slot the mon just joined, so
// Draft_EventScript_OfferNickname can hand it straight to
// `special ChangePokemonNickname`, the same convention
// Common_EventScript_GetGiftMonPartySlot uses (data/scripts/pc_transfer.inc).
// Only ever called right after Draft_TryGiveToEmptySlot succeeds, so the
// pending mon and the party count it reads are both guaranteed current.
void Draft_BufferPendingNickname(void)
{
    GetMonNickname(&sDraftPendingMon, gStringVar1);
    gSpecialVar_0x8004 = gPartiesCount[B_TRAINER_PLAYER] - 1;
}

// gStringVar1 = the outgoing party mon at gSpecialVar_0x8004 (as chosen by
// `special ChoosePartyMon`), gStringVar2 = the pending mon's display name.
// See the GetMonNickname note on Draft_BufferPendingNickname above - the
// same egg-safety applies here, since this is the confirmation prompt an
// egg offered against a full party would otherwise leak its species through.
void Draft_BufferReplacementNames(void)
{
    GetMonNickname(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], gStringVar1);
    GetMonNickname(&sDraftPendingMon, gStringVar2);
}

// Replaces the party mon at gSpecialVar_0x8004 with the pending mon.
// TryRevertPartyMonFormChange runs first, as Cmd_givecaughtmon does before
// releasing a mon to the PC (src/battle_script_commands.c) - a battle-form
// mon should always revert before it leaves the party for good.
void Draft_DoReplacement(void)
{
    u8 slot = gSpecialVar_0x8004;

    // §5's Draft_EventScript_PartyFull only reaches this native once the
    // party was already full (that's what routed the offer here instead of
    // Draft_TryGiveToEmptySlot), and LIMITED_PARTY_BASE_SIZE (3) is the
    // smallest a full party can ever be, so this never removes the last mon.
    // No IsRemovingLastPartyMon-style guard is needed - this is just a
    // trip wire in case that precondition is ever violated.
    AGB_ASSERT(gPartiesCount[B_TRAINER_PLAYER] > 1);

    TryRevertPartyMonFormChange(slot);
    ZeroMonData(&gParties[B_TRAINER_PLAYER][slot]);
    CopyMon(&gParties[B_TRAINER_PLAYER][slot], &sDraftPendingMon, sizeof(sDraftPendingMon));
    // See the matching comment in Draft_TryGiveToEmptySlot - the mon has a
    // home now, so it must stop reading as pending or the field hook loops.
    sDraftPendingValid = FALSE;
}

// Discards the pending mon without placing it anywhere - the decline path.
void Draft_DiscardPending(void)
{
    sDraftPendingValid = FALSE;
}

// Marks the current area's draft as spent. Draft_EventScript_Finish is the
// offer flow's single terminal node and its only caller - see the header
// comment on this function in include/draft_mode.h. No-ops the zone lock
// for a gift/egg that got routed through the pending buffer instead of the
// PC (§6b) - otherwise accepting a gift on a fresh route would silently
// burn that area's own draft.
void Draft_MarkAreaSpent(void)
{
    // Draft_EventScript_Finish is reached by every path through the offer
    // flow - a route's own pick, a decline, and a gift/egg resolution
    // (joined or swapped in) alike. Unlike Nuzlocke, Draft doesn't force
    // autosave on (IsAutosaveHidden, src/option_menu.c leaves the option
    // alone for Draft) - it's just another permanent-choice trigger point,
    // same as any other autosave-eligible event for a regular player, so it
    // only arms one if the player has actually opted into the option.
    if (gSaveBlock1Ptr->autosaveModeEnabled)
        gDoAutosave = TRUE;

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
