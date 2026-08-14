#include "global.h"
#include "achievements.h"
#include "ai_battles.h"
#include "clock.h"
#include "new_game.h"
#include "new_game_settings_menu.h"
#include "random.h"
#include "pokemon.h"
#include "roamer.h"
#include "pokemon_size_record.h"
#include "script.h"
#include "lottery_corner.h"
#include "play_time.h"
#include "mauville_old_man.h"
#include "match_call.h"
#include "lilycove_lady.h"
#include "load_save.h"
#include "pokeblock.h"
#include "dewford_trend.h"
#include "berry.h"
#include "rtc.h"
#include "easy_chat.h"
#include "event_data.h"
#include "money.h"
#include "trainer_hill.h"
#include "trainer_tower.h"
#include "tv.h"
#include "coins.h"
#include "text.h"
#include "overworld.h"
#include "mail.h"
#include "battle_records.h"
#include "item.h"
#include "pokedex.h"
#include "apprentice.h"
#include "frontier_util.h"
#include "pokedex.h"
#include "save.h"
#include "link_rfu.h"
#include "main.h"
#include "contest.h"
#include "item_menu.h"
#include "pokemon_storage_system.h"
#include "pokemon_jump.h"
#include "decoration_inventory.h"
#include "secret_base.h"
#include "string_util.h"
#include "player_pc.h"
#include "field_specials.h"
#include "berry_powder.h"
#include "mystery_gift.h"
#include "union_room_chat.h"
#include "constants/map_groups.h"
#include "constants/items.h"
#include "constants/flags.h"
#include "difficulty.h"
#include "follower_npc.h"
#include "malloc.h"
#include "keep_storage_prompt.h"
#include "trade.h"
#include "constants/pokedex.h"

extern const u8 EventScript_ResetAllMapFlags[];
extern const u8 EventScript_ResetAllMapFlagsFrlg[];

static void ClearFrontierRecord(void);
static void WarpToTruck(void);
static void ResetMiniGamesRecords(void);
static void ResetItemFlags(void);
static void ResetDexNav(void);
static void CarryStorageIntoNewGame(void);
static void ReregisterCarriedOverDexEntries(void);

EWRAM_DATA bool8 gDifferentSaveFile = FALSE;
EWRAM_DATA bool8 gEnableContestDebugging = FALSE;
EWRAM_DATA bool8 gIsNewGamePlus = FALSE;

void SetTrainerId(u32 trainerId, u8 *dst)
{
    dst[0] = trainerId;
    dst[1] = trainerId >> 8;
    dst[2] = trainerId >> 16;
    dst[3] = trainerId >> 24;
}

u32 GetTrainerId(u8 *trainerId)
{
    return (trainerId[3] << 24) | (trainerId[2] << 16) | (trainerId[1] << 8) | (trainerId[0]);
}

void CopyTrainerId(u8 *dst, u8 *src)
{
    s32 i;
    for (i = 0; i < TRAINER_ID_LENGTH; i++)
        dst[i] = src[i];
}

static void InitPlayerTrainerId(void)
{
    // Deliberately not GetGeneratedTrainerIdLower() here: that returns a value
    // cached once by SeedRngAndSetTrainerId(), which only runs when the player
    // types their name on the naming screen -- i.e. once per boot, on a truly
    // fresh save. This function is also called on the Nuzlocke-restart path,
    // which reaches CB2_NewGame without ever visiting the naming screen (the
    // save was just continued, or this is a repeat restart within the same
    // boot), so that cache would still hold its EWRAM_DATA default of 0, or a
    // stale value from an earlier restart -- producing the same all-zero (or
    // repeated) lower half every time instead of a fresh ID. Drawing both
    // halves from Random() keeps every InitPlayerTrainerId() call self
    // contained and independent of naming-screen state.
    u32 trainerId = (Random() << 16) | (Random() & 0xFFFF);
    SetTrainerId(trainerId, gSaveBlock2Ptr->playerTrainerId);
}

// L=A isnt set here for some reason.
static void SetDefaultOptions(void)
{
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock2Ptr->optionsWindowFrameType = 0;
    gSaveBlock2Ptr->optionsSound = OPTIONS_SOUND_STEREO;
    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
    gSaveBlock2Ptr->optionsBattleSceneOff = FALSE;
    gSaveBlock2Ptr->regionMapZoom = FALSE;
    memset(gSaveBlock2Ptr->playerColors, 0, sizeof(gSaveBlock2Ptr->playerColors));
    // Offline trade codes (trade_code.h): explicit alongside the memset
    // above, even though ClearSav2() (called just before this, in
    // Sav2_ClearSetDefault()) already zeroes the whole SaveBlock2 -- kept
    // for the same "belt and suspenders, cheap and correct" reasoning as
    // the playerColors memset right above it, not because anything still
    // depends on it. TRADE_CODE_STATE_NONE is 0, so this doubles as "no
    // pending trade." NewGameInitData's own New Game+ path -- which does
    // NOT run ClearSav2() first, and Sav2_ClearSetDefault (so this
    // function) never runs for it -- gets its own separate clear of the
    // same field; see Trading Codes.md's Stage 11 status block for why
    // that one couldn't just live here.
    memset(&gSaveBlock2Ptr->pendingTrade, 0, sizeof(gSaveBlock2Ptr->pendingTrade));
}

static void ClearPokedexFlags(void)
{
    gUnusedPokedexU8 = 0;
    memset(&gSaveBlock1Ptr->dexCaught, 0, sizeof(gSaveBlock1Ptr->dexCaught));
    memset(&gSaveBlock1Ptr->dexSeen, 0, sizeof(gSaveBlock1Ptr->dexSeen));
}

void ClearAllContestWinnerPics(void)
{
    ClearContestWinnerPicsInContestHall();

    // Museum paintings no longer have reserved slots (NUM_CONTEST_WINNERS == MUSEUM_CONTEST_WINNERS_START).
}

static void ClearFrontierRecord(void)
{
#if FREE_BATTLE_FRONTIER == FALSE
    CpuFill32(0, &gSaveBlock2Ptr->frontier, sizeof(gSaveBlock2Ptr->frontier));

    gSaveBlock2Ptr->frontier.opponentNames[0][0] = EOS;
    gSaveBlock2Ptr->frontier.opponentNames[1][0] = EOS;
#endif //FREE_BATTLE_FRONTIER
}

static void WarpToTruck(void)
{
    if (IS_FRLG)
        SetWarpDestination(MAP_GROUP(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), MAP_NUM(MAP_PALLET_TOWN_PLAYERS_HOUSE_2F), WARP_ID_NONE, 6, 6);
    else
        SetWarpDestination(MAP_GROUP(MAP_INSIDE_OF_TRUCK), MAP_NUM(MAP_INSIDE_OF_TRUCK), WARP_ID_NONE, -1, -1);
    WarpIntoMap();
}

void Sav2_ClearSetDefault(void)
{
    ClearSav2();
    SetDefaultOptions();
}

void ResetMenuAndMonGlobals(void)
{
    gDifferentSaveFile = FALSE;
    ResetPokedexScrollPositions();
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    ResetBagScrollPositions();
    ResetPokeblockScrollPositions();
}

// Boxes the outgoing party (so ZeroPlayerPartyMons() doesn't delete it) and marks every
// mon that survives into the new run's storage as legacy-carry-over-locked (Trading
// Codes.md Stage 11 -- struct BoxPokemon's own legacyCarryOverLocked bit, include/
// pokemon.h) so IsBoxMonWithdrawLocked (src/pokemon_storage_system.c) keeps it out of
// reach until the player beats the league again. Must run before InitPlayerTrainerId() --
// the boxed party needs to keep its old OT ID for the discardRandomizedMons comparisons
// below, which still key off it. Only called when keepStorage is set.
static void CarryStorageIntoNewGame(void)
{
    u32 i, boxId, boxPosition;
    u32 outgoingOtId = READ_OTID_FROM_SAVE;
    u32 partyCount = CalculatePlayerPartyCount();
    // FLAG_RANDOMIZE_MON bakes a randomized species
    // straight into MON_DATA_SPECIES at CreateMon time (see GetRandomizedSpecies in
    // pokemon.c) -- unlike the type/move randomizers, which randomization.c resolves live
    // off the real species and never touch the saved data, so those are fine to carry
    // over. A Pokémon caught -- or received as an in-game trade, which also runs through
    // CreateMon -- under that flag this run is just a randomized species tied to this
    // run's OT id, so it gets discarded below instead of carried into the next one. Must
    // be read here, before ClearSav1() wipes the flag later in NewGameInitData().
    bool32 discardRandomizedMons = FlagGet(FLAG_RANDOMIZE_MON);

    // Pass 1 -- move the party into the first free storage slots. Copied verbatim; Pass 2
    // below (which runs over the whole of storage, so it naturally covers these too) is
    // what actually sets the legacy-carry-over lock bit, not anything in this pass.
    for (i = 0; i < partyCount; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        enum Item heldItem;
        bool32 placed = FALSE;

        if (discardRandomizedMons)
        {
            u32 otId = GetMonData(mon, MON_DATA_OT_ID);
            if (otId == outgoingOtId || IsIngameTradeOtId(otId))
                continue;
        }

        heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);

        // ClearAllMail() runs later in NewGameInitData(), so a boxed mon still
        // holding a mail item would be left pointing at wiped mail data.
        if (ItemIsMail(heldItem))
        {
            enum Item none = ITEM_NONE;
            u8 mailNone = MAIL_NONE;
            SetMonData(mon, MON_DATA_HELD_ITEM, &none);
            SetMonData(mon, MON_DATA_MAIL, &mailNone);
        }

        // Full-heal it so it isn't sitting in the box fainted.
        HealPokemon(mon);

        for (boxId = 0; boxId < TOTAL_BOXES_COUNT && !placed; boxId++)
        {
            for (boxPosition = 0; boxPosition < IN_BOX_COUNT; boxPosition++)
            {
                struct BoxPokemon *slot = GetBoxedMonPtr(boxId, boxPosition);
                if (!GetBoxMonData(slot, MON_DATA_SANITY_HAS_SPECIES))
                {
                    CopyMon(slot, &mon->box, sizeof(mon->box));
                    placed = TRUE;
                    break;
                }
            }
        }
        // If storage is full (TOTAL_BOXES_COUNT * IN_BOX_COUNT slots), stop placing --
        // the remaining party mons are simply lost, which is no worse than the
        // previous wipe-everything behaviour.
        if (!placed)
            break;
    }

    // Pass 2 -- lock every mon that survives into the new run's storage (Trading Codes.md
    // Stage 11), or (Part 3e) discard any already-boxed Pokémon this run caught or traded
    // for while FLAG_RANDOMIZE_MON was on. Running this after pass 1 means a mon that was
    // sitting in the party at restart time gets locked too, with no special-casing.
    // Pokémon from an even earlier run were already locked (this bit, once set, is never
    // cleared short of FLAG_SYS_GAME_CLEAR -- see IsBoxMonWithdrawLocked, src/pokemon_
    // storage_system.c) so re-setting it here is a harmless no-op for them.
    //
    // Pre-Stage-11 this pass instead re-stamped IsIngameTradeOtId() mons' OT ID (via
    // UpdateBoxMonOtId, src/pokemon.c) to outgoingOtId, purely so they'd keep *looking*
    // foreign to an OT-ID-mismatch-based lock check after this restart -- otherwise they'd
    // ride that function's own hardcoded whitelist forever and stay withdrawable. An
    // explicit lock bit needs no such trick: nothing about a mon's real OT ID matters to
    // this pass anymore, so in-game-trade mons now keep their genuine OT ID forever, same
    // as a real trade always would. UpdateBoxMonOtId itself is left in place (a legitimate
    // general-purpose "rewrite this box mon's OT ID, correctly re-encrypting the substructs
    // under the new key" utility, src/pokemon.c) even though this was its only caller.
    for (boxId = 0; boxId < TOTAL_BOXES_COUNT; boxId++)
    {
        for (boxPosition = 0; boxPosition < IN_BOX_COUNT; boxPosition++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(boxId, boxPosition);
            u32 otId;

            if (!GetBoxMonData(boxMon, MON_DATA_SANITY_HAS_SPECIES))
                continue;

            otId = GetBoxMonData(boxMon, MON_DATA_OT_ID);
            if (discardRandomizedMons && (otId == outgoingOtId || IsIngameTradeOtId(otId)))
            {
                ZeroBoxMonData(boxMon);
                continue;
            }
            boxMon->legacyCarryOverLocked = TRUE;
        }
    }
}

// Must run after ClearSav1() wipes dexCaught/dexSeen.
// Re-registers every carried-over box mon as seen + caught so the dex progress the
// player kept storage for is visible from turn one.
static void ReregisterCarriedOverDexEntries(void)
{
    u32 boxId, boxPosition;

    for (boxId = 0; boxId < TOTAL_BOXES_COUNT; boxId++)
    {
        for (boxPosition = 0; boxPosition < IN_BOX_COUNT; boxPosition++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(boxId, boxPosition);
            enum NationalDexOrder dexNum;

            if (!GetBoxMonData(boxMon, MON_DATA_SANITY_HAS_SPECIES)
             || GetBoxMonData(boxMon, MON_DATA_SANITY_IS_EGG))
                continue;

            dexNum = SpeciesToNationalPokedexNum(GetBoxMonData(boxMon, MON_DATA_SPECIES));
            GetSetPokedexFlag(dexNum, FLAG_SET_SEEN);
            GetSetPokedexFlag(dexNum, FLAG_SET_CAUGHT);
        }
    }
}

void NewGameInitData(void)
{
    bool8 isNewGamePlus = gIsNewGamePlus;
    struct Pokemon *playerPartyBackup = NULL;
    u8 playerPartyCountBackup = 0;
    void *pcStorageBackup = NULL;
    void *pcItemsBackup = NULL;
    void *bagItemsBackup = NULL;
    // void *bagKeyItemsBackup = NULL;
    void *bagPokeBallsBackup = NULL;
    void *bagTMHMsBackup = NULL;
    void *bagBerriesBackup = NULL;
    void *dexCaughtBackup = NULL;
    void *dexSeenBackup = NULL;
    void *flagsBackup = NULL;
    u32 aiBattlesBackup = 0;
    void *optionsBackup = NULL;
    void *playerSettingsBackup = NULL;
    void *itemFlagsBackup = NULL;
    u8 savedTrainerId[TRAINER_ID_LENGTH];
    u32 moneyBackup = 0;
    u16 coinsBackup = 0;
    void *roamersBackup = NULL;
    void *locationHistoryBackup = NULL;
    void *roamerLocationBackup = NULL;
    // Never TRUE for New Game+ -- that path already
    // preserves the PC via its own backup/restore below.
    bool32 keepStorage = !isNewGamePlus && gKeepStorageOnNewGame && gSaveFileStatus == SAVE_STATUS_OK;

#if IS_FRLG
    u8 rivalName[PLAYER_NAME_LENGTH + 1];
#endif
    gKeepStorageOnNewGame = FALSE; // consume, same as gIsNewGamePlus below

    if (gSaveFileStatus == SAVE_STATUS_EMPTY || gSaveFileStatus == SAVE_STATUS_CORRUPT)
        RtcReset();

    if (isNewGamePlus)
    {
        /* Backup things we want to preserve */
        CopyTrainerId(savedTrainerId, gSaveBlock2Ptr->playerTrainerId);

        playerPartyBackup = Alloc(sizeof(gSaveBlock1Ptr->playerParty));
        memcpy(playerPartyBackup, gSaveBlock1Ptr->playerParty, sizeof(gSaveBlock1Ptr->playerParty));
        playerPartyCountBackup = gSaveBlock1Ptr->playerPartyCount;

        pcStorageBackup = Alloc(sizeof(*gPokemonStoragePtr));
        memcpy(pcStorageBackup, gPokemonStoragePtr, sizeof(*gPokemonStoragePtr));

        pcItemsBackup = Alloc(sizeof(gSaveBlock1Ptr->pcItems));
        memcpy(pcItemsBackup, gSaveBlock1Ptr->pcItems, sizeof(gSaveBlock1Ptr->pcItems));

        bagItemsBackup = Alloc(sizeof(gSaveBlock1Ptr->bag.items));
        memcpy(bagItemsBackup, gSaveBlock1Ptr->bag.items, sizeof(gSaveBlock1Ptr->bag.items));

        // bagKeyItemsBackup = Alloc(sizeof(gSaveBlock1Ptr->bag.keyItems));
        // memcpy(bagKeyItemsBackup, gSaveBlock1Ptr->bag.keyItems, sizeof(gSaveBlock1Ptr->bag.keyItems));

        bagPokeBallsBackup = Alloc(sizeof(gSaveBlock1Ptr->bag.pokeBalls));
        memcpy(bagPokeBallsBackup, gSaveBlock1Ptr->bag.pokeBalls, sizeof(gSaveBlock1Ptr->bag.pokeBalls));

        bagTMHMsBackup = Alloc(sizeof(gSaveBlock1Ptr->bag.TMsHMs));
        memcpy(bagTMHMsBackup, gSaveBlock1Ptr->bag.TMsHMs, sizeof(gSaveBlock1Ptr->bag.TMsHMs));

        bagBerriesBackup = Alloc(sizeof(gSaveBlock1Ptr->bag.berries));
        memcpy(bagBerriesBackup, gSaveBlock1Ptr->bag.berries, sizeof(gSaveBlock1Ptr->bag.berries));

        dexCaughtBackup = Alloc(sizeof(gSaveBlock1Ptr->dexCaught));
        memcpy(dexCaughtBackup, gSaveBlock1Ptr->dexCaught, sizeof(gSaveBlock1Ptr->dexCaught));

        dexSeenBackup = Alloc(sizeof(gSaveBlock1Ptr->dexSeen));
        memcpy(dexSeenBackup, gSaveBlock1Ptr->dexSeen, sizeof(gSaveBlock1Ptr->dexSeen));
        /* Backup money and coins so they persist through ClearSav1 */
        moneyBackup = GetMoney(&gSaveBlock1Ptr->money);
        coinsBackup = GetCoins();

        roamersBackup = Alloc(sizeof(gSaveBlock1Ptr->roamer));
        memcpy(roamersBackup, gSaveBlock1Ptr->roamer, sizeof(gSaveBlock1Ptr->roamer));

        locationHistoryBackup = Alloc(sizeof(sLocationHistory));
        memcpy(locationHistoryBackup, sLocationHistory, sizeof(sLocationHistory));

        roamerLocationBackup = Alloc(sizeof(sRoamerLocation));
        memcpy(roamerLocationBackup, sRoamerLocation, sizeof(sRoamerLocation));

        /* Backup only option-related flag bytes (minimize restoring unrelated flags) */
        flagsBackup = Alloc(2);
        ((u8 *)flagsBackup)[0] = gSaveBlock1Ptr->flags[FLAG_AUTO_SCROLL_TEXT / 8];
        ((u8 *)flagsBackup)[1] = gSaveBlock1Ptr->flags[FLAG_RANDOMIZE_TYPE / 8];
        /* FLAG_AI_BATTLES / FLAG_AI_WILD_BATTLES go through ai_battles.h, not the raw-byte scheme above */
        aiBattlesBackup = AiBattles_BackupSettings();

        /* Backup SaveBlock2 options (packed bitfields occupy 2 bytes at offset 0x14) */
        optionsBackup = Alloc(sizeof(u16));
        memcpy(optionsBackup, (u8 *)gSaveBlock2Ptr + 0x14, sizeof(u16));
        /* Backup a few SaveBlock1 player settings stored in SaveBlock1 */
        playerSettingsBackup = Alloc(7);
        ((u8 *)playerSettingsBackup)[0] = gSaveBlock1Ptr->nuzlockeModeEnabled;
        ((u8 *)playerSettingsBackup)[1] = gSaveBlock1Ptr->autosaveModeEnabled;
        ((u8 *)playerSettingsBackup)[2] = gSaveBlock1Ptr->difficulty;
        // achievementsBlocked lives in
        // SaveBlock1 and ClearSav1() below wipes it back to FALSE like
        // everything else that isn't explicitly preserved here -- without
        // this, a run that got permanently blocked by opening the debug menu
        // would come back clean (unblocked) on its next NG+ cycle.
        ((u8 *)playerSettingsBackup)[3] = gSaveBlock1Ptr->achievementsBlocked;
        // monoTypeSetting/monoGenSetting/limitedPartySetting actually live in
        // SaveBlock2, which ClearSav1() below never touches, so this isn't
        // strictly needed for correctness -- kept here anyway so the
        // challenge run's settings are backed up as one group.
        ((u8 *)playerSettingsBackup)[4] = gSaveBlock2Ptr->monoTypeSetting;
        ((u8 *)playerSettingsBackup)[5] = gSaveBlock2Ptr->monoGenSetting;
        ((u8 *)playerSettingsBackup)[6] = gSaveBlock2Ptr->limitedPartySetting;

        gIsNewGamePlus = FALSE; // consume flag
    }

#if IS_FRLG
    StringCopy(rivalName, gSaveBlock1Ptr->rivalName);
#endif
    gDifferentSaveFile = TRUE;
    /* Keep existing encryptionKey when doing New Game+ to avoid re-encryption issues */
    if (!isNewGamePlus)
    {
        gSaveBlock2Ptr->encryptionKey = 0;
        // Must run before ZeroPlayerPartyMons() wipes the party below, 
        // and before InitPlayerTrainerId() a few lines later
        // issues a new trainer ID -- the boxed party needs to keep its old OT ID,
        // and the trade-mon re-stamp needs the outgoing one to stamp with.
        if (keepStorage)
            CarryStorageIntoNewGame();
        ZeroPlayerPartyMons();
        ResetPokedex();
        InitPlayerTrainerId();
        PlayTimeCounter_Reset();
        ClearPokedexFlags();
        // Skip the wipe when carrying storage over.
        // Box names, wallpapers, currentBox and fusions[] are deliberately
        // left as-is.
        if (!keepStorage)
            ResetPokemonStorageSystem();
        gPartiesCount[B_TRAINER_PLAYER] = 0;
        NewGameInitPCItems();
        // SetCurrentDifficultyLevel(DIFFICULTY_NORMAL); // OLD DIFFICULTY IMPLEMENTATION
        gSaveBlock2Ptr->newGamePlus = 0;
        ResetItemFlags();
        ResetDexNav();
        // Gates the OT-ID lock in pokemon_storage_system.c, 
        // and is what CB2_NewGame reads on the Nuzlocke restart path.
        // Set unconditionally so a run started without keep-storage
        // explicitly clears any earlier run's answer.
        gSaveBlock2Ptr->keepStorageOnRestart = keepStorage;
    }

    if (isNewGamePlus)
    {
#if OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_FIRST_TIME
        itemFlagsBackup = Alloc(sizeof(gSaveBlock3Ptr->itemFlags));
        memcpy(itemFlagsBackup, gSaveBlock3Ptr->itemFlags, sizeof(gSaveBlock3Ptr->itemFlags));
#endif
    }
    ZeroEnemyPartyMons();
    ClearFrontierRecord();
    ClearSav1();
    if (!isNewGamePlus)
    {
        gSaveBlock1Ptr->difficulty = 1;
        SetMoney(&gSaveBlock1Ptr->money, 5000);
        DeactivateAllRoamers();
        SetCoins(0);
    }
    ClearSav3();
    ClearAllMail();
    gSaveBlock2Ptr->specialSaveWarpFlags = 0;
    gSaveBlock2Ptr->gcnLinkFlags = 0;
    // Trading Codes.md Stage 11 ("New Game Plus / keep-storage" bullet):
    // pendingTrade must be cleared on NG+ so a committed trade can't
    // survive into a fresh file. SetDefaultOptions() (above in this file)
    // already does this same memset, but only runs via Sav2_ClearSetDefault
    // - reached from a truly fresh save (src/intro.c) or a corrupted-reload
    // fallback (src/reload_save.c), both of which already ClearSav2() the
    // whole SaveBlock2 anyway, making that particular memset redundant.
    // This exact NG+ restart path (isNewGamePlus == TRUE) never calls
    // either one - confirmed by reading this whole function before relying
    // on the other one - so without this line, a trade COMMITTED right
    // before restarting into New Game+ would carry its confirm-code-
    // waiting state into the new file, and Stage 9's boot hook (src/
    // overworld.c) would drop a fresh NG+ save straight into "enter your
    // partner's confirm code" for a trade from the previous playthrough.
    // Unconditional (not gated on isNewGamePlus) to match specialSaveWarp
    // Flags/gcnLinkFlags immediately above - harmless on the non-NG+ path,
    // where pendingTrade is already TRADE_CODE_STATE_NONE from ClearSav2().
    memset(&gSaveBlock2Ptr->pendingTrade, 0, sizeof(gSaveBlock2Ptr->pendingTrade));
    InitEventData();
    // Must run after ClearSav1() above wiped dexCaught/dexSeen.
    // Re-registers every carried-over box mon so the dex
    // progress the player kept storage for is visible from turn one.
    if (keepStorage)
        ReregisterCarriedOverDexEntries();
    if (!isNewGamePlus)
        ApplyPendingNewGameSettings();
    ClearTVShowData();
    ResetGabbyAndTy();
    ClearSecretBases();
    ClearBerryTrees();
    ResetLinkContestBoolean();
    ResetGameStats();
    ClearAllContestWinnerPics();
    ClearPlayerLinkBattleRecords();
    InitSeedotSizeRecord();
    InitLotadSizeRecord();
    gSaveBlock1Ptr->registeredItem = ITEM_NONE;
    gSaveBlock1Ptr->registeredLongItem = ITEM_NONE;
    ClearBag();
    // BOOST_STARTER_KIT. Must come after ClearBag() above or the
    // grant is wiped, and is guarded on !isNewGamePlus because the New Game+
    // path restores the previous save's bag and money further down anyway.
    if (!isNewGamePlus && AchievementBoost_HasStarterKit())
    {
        AddBagItem(ITEM_POTION, 5);
        AddBagItem(ITEM_FULL_HEAL, 2);
        AddBagItem(ITEM_REPEL, 3);
        AddBagItem(ITEM_ESCAPE_ROPE, 2);
        AddMoney(&gSaveBlock1Ptr->money, 3000); // on top of the 5000 set above
    }
    ClearPokeblocks();
    ClearDecorationInventories();
    InitEasyChatPhrases();
    SetMauvilleOldMan();
    InitDewfordTrend();
    ResetFanClub();
    ResetLotteryCorner();
    UpdateDailySeed();
    WarpToTruck();
    if (IS_FRLG)
        RunScriptImmediately(EventScript_ResetAllMapFlagsFrlg);
    else
        RunScriptImmediately(EventScript_ResetAllMapFlags);
#if IS_FRLG
        StringCopy(gSaveBlock1Ptr->rivalName, rivalName);
#endif
    ResetMiniGamesRecords();
    InitUnionRoomChatRegisteredTexts();
    InitLilycoveLady();
    ResetAllApprenticeData();
    ClearRankingHallRecords();
    InitMatchCallCounters();
    ClearMysteryGift();
    WipeTrainerNameRecords();
    ResetTrainerHillResults();
    ResetTrainerTowerResults();
    ResetContestLinkResults();
    ClearFollowerNPCData();

    /* Restore preserved data for New Game+ */
    if (isNewGamePlus)
    {
        if (playerPartyBackup != NULL)
        {
            memcpy(gSaveBlock1Ptr->playerParty, playerPartyBackup, sizeof(gSaveBlock1Ptr->playerParty));
            gSaveBlock1Ptr->playerPartyCount = playerPartyCountBackup;
            memcpy(gPokemonStoragePtr, pcStorageBackup, sizeof(*gPokemonStoragePtr));
            memcpy(gSaveBlock1Ptr->pcItems, pcItemsBackup, sizeof(gSaveBlock1Ptr->pcItems));
            memcpy(gSaveBlock1Ptr->bag.items, bagItemsBackup, sizeof(gSaveBlock1Ptr->bag.items));
            // memcpy(gSaveBlock1Ptr->bag.keyItems, bagKeyItemsBackup, sizeof(gSaveBlock1Ptr->bag.keyItems));
            memcpy(gSaveBlock1Ptr->bag.pokeBalls, bagPokeBallsBackup, sizeof(gSaveBlock1Ptr->bag.pokeBalls));
            if (bagTMHMsBackup != NULL)
            {
                struct ItemSlot *backupSlots = bagTMHMsBackup;
                for (u32 i = 0; i < BAG_TMHM_COUNT; i++)
                {
                    enum Item itemId = backupSlots[i].itemId;
                    if (itemId != ITEM_NONE && GetItemTMHMIndex(itemId) <= NUM_TECHNICAL_MACHINES)
                        gSaveBlock1Ptr->bag.TMsHMs[i] = backupSlots[i];
                    else
                        gSaveBlock1Ptr->bag.TMsHMs[i] = (struct ItemSlot){ITEM_NONE, 0};
                }
            }
            else
            {
                CpuFastFill16(0, gSaveBlock1Ptr->bag.TMsHMs, sizeof(gSaveBlock1Ptr->bag.TMsHMs));
            }
            memcpy(gSaveBlock1Ptr->bag.berries, bagBerriesBackup, sizeof(gSaveBlock1Ptr->bag.berries));
            CopyTrainerId(gSaveBlock2Ptr->playerTrainerId, savedTrainerId);

            /* Restore Pokédex flags preserved across ClearSav1 */
            if (dexCaughtBackup != NULL)
                memcpy(gSaveBlock1Ptr->dexCaught, dexCaughtBackup, sizeof(gSaveBlock1Ptr->dexCaught));
            if (dexSeenBackup != NULL)
                memcpy(gSaveBlock1Ptr->dexSeen, dexSeenBackup, sizeof(gSaveBlock1Ptr->dexSeen));

            /* Restore option-related flags from backup (only these specific settings) */
            if (flagsBackup != NULL)
            {
                u8 *fb = (u8 *)flagsBackup;
                (fb[0] & (1 << (FLAG_AUTO_SCROLL_TEXT % 8))) ? FlagSet(FLAG_AUTO_SCROLL_TEXT) : FlagClear(FLAG_AUTO_SCROLL_TEXT);
                (fb[0] & (1 << (FLAG_RANDOMIZE_MON % 8))) ? FlagSet(FLAG_RANDOMIZE_MON) : FlagClear(FLAG_RANDOMIZE_MON);
                (fb[1] & (1 << (FLAG_RANDOMIZE_TYPE % 8))) ? FlagSet(FLAG_RANDOMIZE_TYPE) : FlagClear(FLAG_RANDOMIZE_TYPE);
                (fb[1] & (1 << (FLAG_RANDOMIZE_MOVES % 8))) ? FlagSet(FLAG_RANDOMIZE_MOVES) : FlagClear(FLAG_RANDOMIZE_MOVES);
                (fb[1] & (1 << (FLAG_LEVEL_CAP_OFF % 8))) ? FlagSet(FLAG_LEVEL_CAP_OFF) : FlagClear(FLAG_LEVEL_CAP_OFF);
                (fb[1] & (1 << (FLAG_ALLOW_STAT_EDITOR % 8))) ? FlagSet(FLAG_ALLOW_STAT_EDITOR) : FlagClear(FLAG_ALLOW_STAT_EDITOR);
            }
            /* FLAG_AI_BATTLES / FLAG_AI_WILD_BATTLES go through ai_battles.h, not the raw-byte scheme above */
            AiBattles_RestoreSettings(aiBattlesBackup);

            if (optionsBackup != NULL)
                memcpy((u8 *)gSaveBlock2Ptr + 0x14, optionsBackup, sizeof(u16));

            if (playerSettingsBackup != NULL)
            {
                gSaveBlock1Ptr->nuzlockeModeEnabled = ((u8 *)playerSettingsBackup)[0];
                gSaveBlock1Ptr->autosaveModeEnabled = ((u8 *)playerSettingsBackup)[1];
                gSaveBlock1Ptr->difficulty = ((u8 *)playerSettingsBackup)[2];
                gSaveBlock1Ptr->achievementsBlocked = ((u8 *)playerSettingsBackup)[3];
                gSaveBlock2Ptr->monoTypeSetting = ((u8 *)playerSettingsBackup)[4];
                gSaveBlock2Ptr->monoGenSetting = ((u8 *)playerSettingsBackup)[5];
                gSaveBlock2Ptr->limitedPartySetting = ((u8 *)playerSettingsBackup)[6];
            }

            if (roamersBackup != NULL)
                memcpy(gSaveBlock1Ptr->roamer, roamersBackup, sizeof(gSaveBlock1Ptr->roamer));
            if (locationHistoryBackup != NULL)
                memcpy(sLocationHistory, locationHistoryBackup, sizeof(sLocationHistory));
            if (roamerLocationBackup != NULL)
                memcpy(sRoamerLocation, roamerLocationBackup, sizeof(sRoamerLocation));

#if OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_FIRST_TIME
            if (itemFlagsBackup != NULL)
                memcpy(gSaveBlock3Ptr->itemFlags, itemFlagsBackup, sizeof(gSaveBlock3Ptr->itemFlags));
#endif

            /* Restore money and coins preserved across ClearSav1 */
            SetMoney(&gSaveBlock1Ptr->money, moneyBackup);
            SetCoins(coinsBackup);

            /* Load restored party into runtime structures so follower code has mons available. */
            LoadPlayerParty();

            /* Increase New Game+ counter in save (0-255) */
            gSaveBlock2Ptr->newGamePlus++;
            // highestNgPlusCycle is a high-water mark in the
            // achievement profile (outside SaveBlock2, so it survives even a
            // corrupted/reset save) -- newGamePlus itself is already the live
            // counter, this just remembers the furthest the player has gone.
            Achievement_OnNewGamePlusStarted(gSaveBlock2Ptr->newGamePlus);
        }

        if (playerPartyBackup != NULL)
            Free(playerPartyBackup);
        if (pcStorageBackup != NULL)
            Free(pcStorageBackup);
        if (pcItemsBackup != NULL)
            Free(pcItemsBackup);
        if (bagItemsBackup != NULL)
            Free(bagItemsBackup);
        if (dexCaughtBackup != NULL)
            Free(dexCaughtBackup);
        if (dexSeenBackup != NULL)
            Free(dexSeenBackup);
        if (flagsBackup != NULL)
            Free(flagsBackup);
        if (optionsBackup != NULL)
            Free(optionsBackup);
        if (playerSettingsBackup != NULL)
            Free(playerSettingsBackup);
        if (roamersBackup != NULL)
            Free(roamersBackup);
        if (locationHistoryBackup != NULL)
            Free(locationHistoryBackup);
        if (roamerLocationBackup != NULL)
            Free(roamerLocationBackup);
        // if (bagKeyItemsBackup != NULL)
        //     Free(bagKeyItemsBackup);
        if (bagPokeBallsBackup != NULL)
            Free(bagPokeBallsBackup);
        if (bagTMHMsBackup != NULL)
            Free(bagTMHMsBackup);
        if (bagBerriesBackup != NULL)
            Free(bagBerriesBackup);
    }
}

static void ResetMiniGamesRecords(void)
{
    CpuFill16(0, &gSaveBlock2Ptr->berryCrush, sizeof(struct BerryCrush));
    SetBerryPowder(&gSaveBlock2Ptr->berryCrush.berryPowderAmount, 0);
    ResetPokemonJumpRecords();
    CpuFill16(0, &gSaveBlock2Ptr->berryPick, sizeof(struct BerryPickingResults));
}

static void ResetItemFlags(void)
{
#if OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_FIRST_TIME
    memset(&gSaveBlock3Ptr->itemFlags, 0, sizeof(gSaveBlock3Ptr->itemFlags));
#endif
}

static void ResetDexNav(void)
{
#if USE_DEXNAV_SEARCH_LEVELS == TRUE
    memset(gSaveBlock3Ptr->dexNavSearchLevels, 0, sizeof(gSaveBlock3Ptr->dexNavSearchLevels));
#endif
    gSaveBlock3Ptr->dexNavChain = 0;
}

// Script-native: sets VAR_RESULT to 1 if save's newGamePlus counter is > 0, otherwise 0
void CheckNewGamePlus(struct ScriptContext *ctx)
{
    u16 val = (gSaveBlock2Ptr->newGamePlus > 0) ? 1 : 0;
    VarSet(VAR_RESULT, val);
}
