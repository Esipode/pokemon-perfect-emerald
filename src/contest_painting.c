// The contest minigame was removed (RAM reclamation, stage 3c). There are no more contest-won
// paintings to show; both entry points just safely hand control back to the field/script.
#include "global.h"
#include "contest_painting.h"
#include "main.h"
#include "overworld.h"
#include "script.h"

void SetContestWinnerForPainting(int contestWinnerId)
{
    // no-op: contests removed
}

void CB2_ContestPainting(void)
{
    ScriptContext_Enable();
    SetMainCallback2(CB2_ReturnToField);
}
