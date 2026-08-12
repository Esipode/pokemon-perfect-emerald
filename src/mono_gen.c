#include "global.h"
#include "mono_gen.h"
#include "pokemon.h"
#include "ui_birch_case.h"
#include "constants/pokedex.h"
#include "constants/species.h"

// Shared rules for the Mono Gen challenge. See include/mono_gen.h.

bool32 MonoGen_IsEnabled(void)
{
    return gSaveBlock2Ptr->monoGenSetting != 0;
}

u8 MonoGen_GetGen(void)
{
    return gSaveBlock2Ptr->monoGenSetting;
}

u8 MonoGen_GetSpeciesGeneration(u16 species)
{
    const struct SpeciesInfo *info;
    enum NationalDexOrder dexNum;

    if (species == SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return 0;

    info = &gSpeciesInfo[species];

    // Regional and battle-only forms belong to the generation that introduced
    // the FORM, not to the base species' dex slot.
    if (info->isMegaEvolution || info->isPrimalReversion)
        return 6;
    if (info->isAlolanForm || info->isUltraBurst)
        return 7;
    if (info->isGalarianForm || info->isHisuianForm || info->isGigantamax)
        return 8;
    if (info->isPaldeanForm || info->isTeraForm)
        return 9;

    dexNum = SpeciesToNationalPokedexNum(species);
    if (dexNum == NATIONAL_DEX_NONE)
        return 0; // Unknown - never blocks.
    if (dexNum <= NATIONAL_DEX_MEW)
        return 1;
    if (dexNum <= NATIONAL_DEX_CELEBI)
        return 2;
    if (dexNum <= NATIONAL_DEX_DEOXYS)
        return 3;
    if (dexNum <= NATIONAL_DEX_ARCEUS)
        return 4;
    if (dexNum <= NATIONAL_DEX_GENESECT)
        return 5;
    if (dexNum <= NATIONAL_DEX_VOLCANION)
        return 6;
    if (dexNum <= NATIONAL_DEX_MELMETAL)
        return 7;
    if (dexNum <= NATIONAL_DEX_ENAMORUS) // Includes the Hisui additions.
        return 8;
    return 9;
}

bool32 MonoGen_IsSpeciesAllowed(u16 species)
{
    u8 gen = MonoGen_GetGen();
    u8 speciesGen;

    if (gen == 0)
        return TRUE;

    // Never block on something that isn't a real species; a caller passing
    // SPECIES_NONE or an out-of-range ID should fall through to whatever it
    // would have done with the mode off.
    if (species == SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return TRUE;

    speciesGen = MonoGen_GetSpeciesGeneration(species);

    // Generation 0 ("unknown") is treated as allowed rather than blocked.
    return speciesGen == 0 || speciesGen == gen;
}

void MonoGen_GetCanonicalStarters(u16 *out)
{
    u8 gen = MonoGen_GetGen();
    u32 i;

    for (i = 0; i < 3; i++)
        out[i] = SPECIES_NONE;

    if (gen == 0)
        return;

    for (i = 0; i < 3; i++)
        out[i] = GetCanonicalStarterSpecies(gen, i);
}

u8 MonoGen_CycleGen(u8 current, bool8 forward)
{
    if (forward)
        return (current + 1) % (MONO_GEN_COUNT + 1);
    else
        return (current + MONO_GEN_COUNT) % (MONO_GEN_COUNT + 1);
}
