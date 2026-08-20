#include "global.h"
#include "pokedex.h"
#include "test/test.h"
#include "constants/pokedex.h"

// Same source of truth pokedex_variant_slots.h builds its table from, so this
// test can't drift from the real variant list.
static const u16 sTestDexVariantSpecies[] =
{
    #define DEX_VARIANT_SLOT_SPECIES(name) SPECIES_ ##name,
    FOREACH_DEX_VARIANT_FLAG_SLOT(DEX_VARIANT_SLOT_SPECIES)
    #undef DEX_VARIANT_SLOT_SPECIES
};

static bool32 IsTestDexVariantSpecies(u32 species)
{
    for (u32 i = 0; i < ARRAY_COUNT(sTestDexVariantSpecies); i++)
    {
        if (sTestDexVariantSpecies[i] == species)
            return TRUE;
    }
    return FALSE;
}

// Save compatibility for Feature 2 (Stage 4/5) hinges on this: every species
// that had a Pokedex flag bit before the regional-form slots existed must
// still map to that same slot (its National Dex number), or an existing
// save's caught/seen bits would silently point at the wrong species.
TEST("Non-variant species keep their National Dex number as their flag slot")
{
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        if (IsTestDexVariantSpecies(species))
            continue;
        // Totem/Zen forms are deliberately remapped onto their base form's
        // slot instead of keeping their own natDexNum -- see
        // SpeciesToDexFlagSlot.
        if (species == SPECIES_RATICATE_ALOLA_TOTEM
         || species == SPECIES_MAROWAK_ALOLA_TOTEM
         || species == SPECIES_DARMANITAN_GALAR_ZEN)
            continue;

        EXPECT_EQ(SpeciesToDexFlagSlot(species), SpeciesToNationalPokedexNum(species));
    }
}

// Variant slots must sit above NATIONAL_DEX_COUNT so they can never collide
// with a pre-existing dex-number-keyed bit (the append-only invariant).
TEST("Regional-form flag slots never collide with a National Dex number")
{
    for (u32 i = 0; i < ARRAY_COUNT(sTestDexVariantSpecies); i++)
    {
        u32 slot = SpeciesToDexFlagSlot(sTestDexVariantSpecies[i]);
        EXPECT(slot > NATIONAL_DEX_COUNT);
    }
}
