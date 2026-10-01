#include "global.h"
#include "event_data.h"
#include "test/test.h"

TEST("(Johto storage) Johto flags set, read and clear at both ends")
{
    u32 i;
    static const u16 flags[] = {JOHTO_FLAGS_START, JOHTO_HIDDEN_ITEMS_END, JOHTO_TRAINER_FLAGS_START, JOHTO_FLAGS_END};

    for (i = 0; i < ARRAY_COUNT(flags); i++)
    {
        EXPECT(!FlagGet(flags[i]));
        FlagSet(flags[i]);
        EXPECT(FlagGet(flags[i]));
        FlagClear(flags[i]);
        EXPECT(!FlagGet(flags[i]));
    }
}

TEST("(Johto storage) Johto flags do not alias Kanto flags")
{
    FlagSet(JOHTO_FLAGS_START);
    EXPECT(!FlagGet(KANTO_FLAGS_START));
    FlagClear(JOHTO_FLAGS_START);
    FlagSet(KANTO_FLAGS_END);
    EXPECT(!FlagGet(JOHTO_FLAGS_START));
    FlagClear(KANTO_FLAGS_END);
}

TEST("(Johto storage) Johto vars set and read at both ends")
{
    VarSet(JOHTO_VARS_START, 0x1234);
    VarSet(JOHTO_VARS_END, 0xABCD);
    EXPECT_EQ(VarGet(JOHTO_VARS_START), 0x1234);
    EXPECT_EQ(VarGet(JOHTO_VARS_END), 0xABCD);
    EXPECT_EQ(VarGet(KANTO_VARS_END), 0);
    VarSet(JOHTO_VARS_START, 0);
    VarSet(JOHTO_VARS_END, 0);
}
