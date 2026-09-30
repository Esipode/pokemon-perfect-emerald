#include "global.h"
#include "field_chansey.h"
#include "achievements.h"
#include "event_data.h"
#include "constants/flags.h"
#include "constants/maps.h"
#include "data/field_chansey_sites.h"

static bool8 IsSiteActive(const struct FieldChanseySite *site)
{
    if (site->requiredFlag != 0 && !FlagGet(site->requiredFlag))
        return FALSE;
    if (site->blockingFlag != 0 && FlagGet(site->blockingFlag))
        return FALSE;
    if (site->var != 0)
    {
        u16 value = VarGet(site->var);
        if (value < site->varMin || value > site->varMax)
            return FALSE;
    }
    return TRUE;
}

void FieldChansey_RefreshVisibility(void)
{
    u32 i;
    u16 map = (gSaveBlock1Ptr->location.mapGroup << 8) | gSaveBlock1Ptr->location.mapNum;

    FlagSet(FLAG_HIDE_FIELD_CHANSEY);
    for (i = 0; i < ARRAY_COUNT(sFieldChanseySites); i++)
    {
        if (sFieldChanseySites[i].map != map)
            continue;
        if (IsSiteActive(&sFieldChanseySites[i]))
            FlagClear(FLAG_HIDE_FIELD_CHANSEY);
        return;
    }
}

void FieldChansey_RecordHeal(void)
{
    Achievement_RecordCenterHeal();
}
