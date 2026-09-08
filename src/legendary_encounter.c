#include "global.h"

#include "event_data.h"
#include "field_specials.h"
#include "overworld.h"
#include "pokedex.h"
#include "rtc.h"

#include "constants/flags.h"
#include "constants/species.h"

/*
 * VAR_0x8004 = Legendary species
 * VAR_0x8005 = Alternate layout ID
 * VAR_0x8006 = Required weekday, 0 = Sunday ... 6 = Saturday
 * VAR_0x8007 = Daily-seed salt
 *
 * Result:
 *   TRUE  = alternate layout was selected
 *   FALSE = normal layout should be used
 *
 * The normal map layout is loaded first by LoadCurrentMapData().
 * This special is called by MAP_SCRIPT_ON_TRANSITION, before InitMap(),
 * so changing the layout here causes the alternate layout to actually
 * be rendered for this map visit.
 */
void CheckValidLegendaryEncounter(void)
{
    u16 species = gSpecialVar_0x8004;
    u16 alternateLayout = gSpecialVar_0x8005;
    u8 requiredWeekday = gSpecialVar_0x8006;
    u32 seedValue;

    /*
     * Start by assuming the normal map layout should remain active.
     */
    gSpecialVar_Result = FALSE;

    /*
     * Safety check.
     */
    if (species == SPECIES_NONE)
        return;

    if (alternateLayout == 0)
        return;

    /*
     * The alternate legendary maps are post-game only.
     */
    if (!FlagGet(FLAG_SYS_GAME_CLEAR))
        return;

    /*
     * Do not activate the special map after the legendary has already
     * been caught.
     */
    if (CheckPlayerOwnsSpecies(species))
        return;

    /*
     * Create a deterministic value that is different for every legendary
     * while remaining identical for the same legendary on the same day.
     *
     * The multiplication constant is simply a mixing constant; there is
     * no dependence on the normal battle RNG state.
     */
    seedValue = gSaveBlock1Ptr->dailySeed;
    seedValue ^= (u32)species * 0x45D9F3B;
    seedValue ^= (u32)gSpecialVar_0x8007 * 0x119DE1F3;

    /*
     * Select one of seven possible results.
     *
     * Because the result is derived entirely from the date and the
     * legendary-specific inputs, it remains stable for the entire day.
     */
    if ((seedValue % 7) != requiredWeekday)
        return;

    gSpecialVar_Result = TRUE;
}
