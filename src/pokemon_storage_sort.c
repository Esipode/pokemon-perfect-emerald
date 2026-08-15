#include "global.h"
#include "pokemon_storage_sort.h"
#include "malloc.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "string_util.h"

// Sort engine for the PC boxes. See include/pokemon_storage_sort.h.

#define STORAGE_SLOT_COUNT (TOTAL_BOXES_COUNT * IN_BOX_COUNT)

// Marks an order[] entry whose destination slot has already been written
// during the permutation pass.
#define SLOT_PLACED 0x8000

STATIC_ASSERT(STORAGE_SLOT_COUNT < SLOT_PLACED, StorageSortSlotIndexMustFitBelowThePlacedBit);

// Compared before the mode's key in every mode.
enum
{
    SORT_GROUP_NORMAL,
    SORT_GROUP_EGG,
    SORT_GROUP_BAD_EGG,
};

struct SortSlotKey
{
    u16 key;      // dex no. / type id / level, per mode; unused for NAME
    u16 dexNum;   // first tie-break
    u16 species;  // second tie-break, so forms of one species order stably
    u8 group;
};

struct StorageSortWork
{
    // order[dest] = source slot. Occupied slots first, sorted; then the empty
    // slots, so the whole thing is a permutation of every slot.
    u16 order[STORAGE_SLOT_COUNT];
    u16 merged[STORAGE_SLOT_COUNT];
    struct SortSlotKey keys[STORAGE_SLOT_COUNT];
    // Uppercased nicknames, indexed by slot. Only allocated for STORAGE_SORT_NAME.
    u8 (*names)[POKEMON_NAME_LENGTH + 1];
    enum StorageSortType type;
    u16 count; // occupied slots
};

static struct BoxPokemon *SlotPtr(u32 slot)
{
    return GetBoxedMonPtr(slot / IN_BOX_COUNT, slot % IN_BOX_COUNT);
}

// One walk over every slot, splitting them into occupied (front of order[])
// and empty (back of order[]) and computing one key per occupied slot.
//
// The keys are precomputed rather than read inside the comparator because
// GetBoxMonData decrypts the secure substructs for any field past
// MON_DATA_ENCRYPT_SEPARATOR - which includes the nickname, whose last two
// characters live in substruct0. Reading them per comparison would decrypt
// thousands of times instead of once per mon.
static void BuildSortOrder(struct StorageSortWork *work)
{
    u32 slot, species;
    u32 head = 0;
    u32 tail = STORAGE_SLOT_COUNT;
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];

    for (slot = 0; slot < STORAGE_SLOT_COUNT; slot++)
    {
        struct BoxPokemon *boxMon = SlotPtr(slot);
        struct SortSlotKey *key = &work->keys[slot];

        // The unencrypted sanity bit, not MON_DATA_SPECIES: species reads
        // through the checksum path and reports SPECIES_NONE for a corrupted
        // mon, which would make the sort silently delete it.
        if (!GetBoxMonData(boxMon, MON_DATA_SANITY_HAS_SPECIES))
        {
            // Empty slots fill from the back in any order - all of them are
            // zeroed once the permutation has been applied.
            work->order[--tail] = slot;
            continue;
        }

        work->order[head++] = slot;

        // Eggs and bad eggs keep the zeroed key/dexNum/species they were
        // allocated with; group alone decides where they land, and equal
        // entries hold their original order.
        if (GetBoxMonData(boxMon, MON_DATA_SANITY_IS_BAD_EGG))
        {
            key->group = SORT_GROUP_BAD_EGG;
            continue;
        }
        if (GetBoxMonData(boxMon, MON_DATA_SANITY_IS_EGG))
        {
            key->group = SORT_GROUP_EGG;
            continue;
        }

        species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
        key->species = species;
        key->dexNum = SpeciesToNationalPokedexNum(species);

        switch (work->type)
        {
        case STORAGE_SORT_DEX:
            key->key = key->dexNum;
            break;
        case STORAGE_SORT_TYPE1:
            key->key = GetSpeciesType(species, 0);
            break;
        case STORAGE_SORT_TYPE2:
            key->key = GetSpeciesType(species, 1);
            break;
        case STORAGE_SORT_LEVEL:
            key->key = GetLevelFromBoxMonExp(boxMon);
            break;
        case STORAGE_SORT_NAME:
            // Folded to upper case on the way in: the game charset puts a-z
            // 0x1A above A-Z, so a raw StringCompare sorts "apple" after
            // "Zebra". item_menu.c's alphabetical sort compares raw only
            // because item names have a fixed house casing - player nicknames
            // do not.
            GetBoxMonData(boxMon, MON_DATA_NICKNAME, nickname);
            StringCopyUppercase(work->names[slot], nickname);
            break;
        default:
            break;
        }
    }

    work->count = head;
    AGB_ASSERT(head == tail);
}

static s32 CompareSlots(struct StorageSortWork *work, u32 slotA, u32 slotB)
{
    const struct SortSlotKey *a = &work->keys[slotA];
    const struct SortSlotKey *b = &work->keys[slotB];

    if (a->group != b->group)
        return a->group < b->group ? -1 : 1;

    if (work->type == STORAGE_SORT_NAME)
    {
        s32 cmp = StringCompare(work->names[slotA], work->names[slotB]);
        if (cmp != 0)
            return cmp;
    }
    else if (a->key != b->key)
    {
        if (work->type == STORAGE_SORT_LEVEL)
            return a->key > b->key ? -1 : 1;
        return a->key < b->key ? -1 : 1;
    }

    // Without a tie-break, TYPE and LEVEL leave huge runs ordered by wherever
    // the mons happened to sit, which reads as random.
    if (a->dexNum != b->dexNum)
        return a->dexNum < b->dexNum ? -1 : 1;
    if (a->species != b->species)
        return a->species < b->species ? -1 : 1;

    return 0;
}

static void Merge(struct StorageSortWork *work, u32 iLeft, u32 iRight, u32 iEnd)
{
    u32 i = iLeft, j = iRight, k;

    for (k = iLeft; k < iEnd; k++)
    {
        // <= keeps the sort stable, so fully equal entries hold their original
        // flat order. (item_menu.c's Merge uses < here and is not stable.)
        if (i < iRight && (j >= iEnd || CompareSlots(work, work->order[i], work->order[j]) <= 0))
            work->merged[k] = work->order[i++];
        else
            work->merged[k] = work->order[j++];
    }
}

// Bottom-up stable merge sort over the occupied prefix of order[], the same
// shape as MergeSort in src/item_menu.c but sorting slot indices against the
// precomputed keys instead of item slots.
// Source: https://en.wikipedia.org/wiki/Merge_sort#Bottom-up_implementation
static void SortOrder(struct StorageSortWork *work)
{
    u32 width, i, j;

    for (width = 1; width < work->count; width *= 2)
    {
        for (i = 0; i < work->count; i += 2 * width)
            Merge(work, i, min(i + width, work->count), min(i + 2 * width, work->count));

        for (j = 0; j < work->count; j++)
            work->order[j] = work->merged[j];
    }
}

// Applies order[] to the boxes in place by walking each permutation cycle
// once, carrying a single mon in a stack temp.
static void ApplyOrder(struct StorageSortWork *work)
{
    struct BoxPokemon temp;
    u32 start, cur, src;

    for (start = 0; start < STORAGE_SLOT_COUNT; start++)
    {
        if (work->order[start] & SLOT_PLACED)
            continue;

        if (work->order[start] == start)
        {
            work->order[start] |= SLOT_PLACED;
            continue;
        }

        // Lift the mon this cycle will displace, then pull each slot's source
        // into it; the carried mon closes the cycle.
        temp = *SlotPtr(start);
        cur = start;
        while ((src = work->order[cur]) != start)
        {
            *SlotPtr(cur) = *SlotPtr(src);
            work->order[cur] |= SLOT_PLACED;
            cur = src;
        }
        *SlotPtr(cur) = temp;
        work->order[cur] |= SLOT_PLACED;
    }

    for (start = work->count; start < STORAGE_SLOT_COUNT; start++)
        ZeroBoxMonAt(start / IN_BOX_COUNT, start % IN_BOX_COUNT);
}

bool32 SortPokemonStorage(enum StorageSortType type)
{
    struct StorageSortWork *work;

    if (type >= STORAGE_SORT_COUNT)
        return FALSE;

    // Unchecked: plain AllocZeroed calls fatalf on failure instead of
    // returning NULL, and a full PC is not worth crashing over.
    work = AllocZeroedUnchecked(sizeof(*work));
    if (work == NULL)
        return FALSE;

    work->type = type;
    if (type == STORAGE_SORT_NAME)
    {
        work->names = AllocZeroedUnchecked(STORAGE_SLOT_COUNT * sizeof(*work->names));
        if (work->names == NULL)
        {
            Free(work);
            return FALSE;
        }
    }

    BuildSortOrder(work);
    if (work->count != 0)
    {
        SortOrder(work);
        ApplyOrder(work);
    }

    if (work->names != NULL)
        Free(work->names);
    Free(work);
    return TRUE;
}
