// The contest minigame was removed (RAM reclamation, stage 3c). This file used to be the
// public API the contest engine exposed to scripts (via specials) and a few other systems.
// Every entry point below is now a safe no-op/default so nothing that still calls into them
// (scripts, the party menu, the trainer card) can hang or crash.
#include "global.h"
#include "contest.h"
#include "contest_util.h"
#include "event_data.h"
#include "link.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "string_util.h"
#include "constants/characters.h"

void BufferContestantTrainerName(void)
{
    gStringVar1[0] = EOS;
}

void BufferContestantMonNickname(void)
{
    gStringVar3[0] = EOS;
}

void BufferContestantMonSpecies(void)
{
    gSpecialVar_0x8004 = SPECIES_NONE;
}

void BufferContestTrainerAndMonNames(void)
{
    BufferContestantTrainerName();
    BufferContestantMonNickname();
    BufferContestantMonSpecies();
}

void BufferContestWinnerTrainerName(void)
{
    gStringVar3[0] = EOS;
}

void BufferContestWinnerMonName(void)
{
    gStringVar1[0] = EOS;
}

void GetContestWinnerId(void)
{
    gSpecialVar_0x8005 = 0;
}

void GetContestPlayerId(void)
{
    gSpecialVar_0x8004 = gContestPlayerMonIndex;
}

// Unused
void GetNpcContestantLocalId(void)
{
    gSpecialVar_0x8004 = 0;
}

void GetContestMonConditionRanking(void)
{
    gSpecialVar_0x8004 = 0;
}

void GetContestMonCondition(void)
{
    gSpecialVar_0x8004 = 0;
}

void SetContestTrainerGfxIds(void)
{
    // no-op: contests removed
}

void SetLinkContestPlayerGfx(void)
{
    // no-op: link contests removed
}

void LoadLinkContestPlayerPalettes(void)
{
    // no-op: link contests removed
}

void TryEnterContestMon(void)
{
    gSpecialVar_Result = CANT_ENTER_CONTEST;
}

// Unused
void GetContestantNamesAtRank(void)
{
    // no-op: contests removed
}

u16 HasMonWonThisContestBefore(void)
{
    return FALSE;
}

void GiveMonContestRibbon(void)
{
    // no-op: contest ribbons no longer exist (FREE_CONTESTS)
}

bool8 GiveMonArtistRibbon(void)
{
    return FALSE;
}

bool8 IsContestDebugActive(void)
{
    return FALSE;
}

void ShouldReadyContestArtist(void)
{
    gSpecialVar_0x8004 = FALSE;
}

void SaveMuseumContestPainting(void)
{
    // no-op: no museum painting slots remain (FREE_CONTESTS)
}

void DoesContestCategoryHaveMuseumPainting(void)
{
    gSpecialVar_0x8004 = FALSE;
}

u8 CountPlayerMuseumPaintings(void)
{
    return 0;
}

void ShowContestPainting(void)
{
    ScriptContext_Enable();
}

void ShowContestEntryMonPic(void)
{
    // no-op: contests removed
}

void HideContestEntryMonPic(void)
{
    // no-op: contests removed
}

void GetContestMultiplayerId(void)
{
    gSpecialVar_Result = MAX_LINK_PLAYERS;
}

void GenerateContestRand(void)
{
    gSpecialVar_Result = Random() % gSpecialVar_Result;
}

u16 GetContestRand(void)
{
    return Random();
}

bool8 LinkContestWaitForConnection(void)
{
    return FALSE;
}

void LinkContestTryShowWirelessIndicator(void)
{
    // no-op: link contests removed
}

void LinkContestTryHideWirelessIndicator(void)
{
    // no-op: link contests removed
}

bool8 IsContestWithRSPlayer(void)
{
    return FALSE;
}

void ClearLinkContestFlags(void)
{
    gLinkContestFlags = 0;
}

bool8 IsWirelessContest(void)
{
    return FALSE;
}

void StartContest(void)
{
    ScriptContext_Enable();
}

void ShowContestResults(void)
{
    ScriptContext_Enable();
}

void ContestLinkTransfer(u8 category)
{
    ScriptContext_Enable();
}
