// Species list for the regional-form Pokédex flag slots. Generated from
// FOREACH_DEX_VARIANT_FLAG_SLOT (include/constants/pokedex.h) so this array
// and DEX_VARIANT_FLAG_SLOT_COUNT can never drift apart. Slot index is this
// array's position, offset by DEX_FLAG_SLOT_VARIANT_START -- append only.
static const u16 sDexVariantFlagSlotSpecies[] =
{
    #define DEX_VARIANT_SLOT_SPECIES(name) SPECIES_ ##name,
    FOREACH_DEX_VARIANT_FLAG_SLOT(DEX_VARIANT_SLOT_SPECIES)
    #undef DEX_VARIANT_SLOT_SPECIES
};
