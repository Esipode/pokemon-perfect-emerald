#include "global.h"

#include "event_data.h"
#include "field_specials.h"
#include "overworld.h"
#include "pokedex.h"
#include "rtc.h"

#include "constants/flags.h"
#include "constants/siirtc.h"
#include "constants/species.h"

/*
 * VAR_0x8004 = Legendary species
 * VAR_0x8005 = Alternate layout ID
 * VAR_0x8006 = Required weekday (WEEKDAY_SUN..WEEKDAY_SAT), or WEEKDAY_ANY for no requirement
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
    enum Weekday currentWeekday = GetDayOfWeek();

    gSpecialVar_Result = FALSE;

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

    if (requiredWeekday != WEEKDAY_ANY && currentWeekday != requiredWeekday)
        return;

    gSpecialVar_Result = TRUE;
}