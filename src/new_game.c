#include "global.h"
#include "achievements.h"
#include "ai_battles.h"
#include "clock.h"
#include "new_game.h"
#include "new_game_settings_menu.h"
#include "random.h"
#include "clock.h"
#include "pokemon.h"
#include "roamer.h"
#include "pokemon_size_record.h"
#include "script.h"
#include "lottery_corner.h"
#include "play_time.h"
#include "mauville_old_man.h"
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
#include "main.h"
#include "contest.h"
#include "item_menu.h"
#include "pokemon_storage_system.h"
#include "decoration_inventory.h"
#include "string_util.h"
#include "player_pc.h"
#include "field_specials.h"
#include "berry_powder.h"
#include "mystery_gift.h"
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
    // Not GetGeneratedTrainerIdLower(): its value is cached by SeedRngAndSetTrainerId(),
    // which only runs on the naming screen. The Nuzlocke-restart path reaches CB2_NewGame
    // without it, so the cache would be 0 or stale and repeat the same lower half.
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
    gSaveBlock2Ptr->optionsExpShare = TRUE;
    gSaveBlock2Ptr->optionsBattleSpeed = OPTIONS_BATTLE_SPEED_1X;
    // Stored as mode + 1; 0 is reserved for "unset" on pre-change saves.
    gSaveBlock2Ptr->optionsHpDisplayPlayer = OPTIONS_HP_DISPLAY_BAR_NUMBERS + 1;
    gSaveBlock2Ptr->optionsHpDisplayOpponent = (B_HP_PERCENTAGE_DISPLAY ? OPTIONS_HP_DISPLAY_BAR_PERCENT : OPTIONS_HP_DISPLAY_BAR_ONLY) + 1;
    memset(gSaveBlock2Ptr->playerColors, 0, sizeof(gSaveBlock2Ptr->playerColors));
    memset(gSaveBlock2Ptr->playerColorSlots, 0, sizeof(gSaveBlock2Ptr->playerColorSlots));
    // Redundant with ClearSav2() in Sav2_ClearSetDefault(); kept alongside the
    // playerColors memset. TRADE_CODE_STATE_NONE is 0. The New Game+ path in
    // NewGameInitData never reaches this function and clears the field itself.
    memset(&gSaveBlock2Ptr->pendingTrade, 0, sizeof(gSaveBlock2Ptr->pendingTrade));
}

static void ClearPokedexFlags(void)
{
    memset(&gSaveBlock1Ptr->dexCaught, 0, sizeof(gSaveBlock1Ptr->dexCaught));
    memset(&gSaveBlock1Ptr->dexSeen, 0, sizeof(gSaveBlock1Ptr->dexSeen));
}

void ClearAllContestWinnerPics(void)
{
    ClearContestWinnerPicsInContestHall();

    // Museum paintings have no reserved slots (NUM_CONTEST_WINNERS == MUSEUM_CONTEST_WINNERS_START).
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
// surviving mon with BoxPokemon's legacyCarryOverLocked bit, so IsBoxMonWithdrawLocked
// (src/pokemon_storage_system.c) keeps it out of reach until the player beats the league
// again. Must run before InitPlayerTrainerId(): the discardRunLocalMons comparisons key
// off the old OT ID. Only called when keepStorage is set.
static void CarryStorageIntoNewGame(void)
{
    u32 i, boxId, boxPosition;
    u32 outgoingOtId = READ_OTID_FROM_SAVE;
    u32 partyCount = CalculatePlayerPartyCount();
    // FLAG_RANDOMIZE_MON bakes a randomized species into MON_DATA_SPECIES at CreateMon time
    // (see GetRandomizedSpecies), unlike the type/move randomizers, which resolve live and
    // are safe to carry over. Mons caught or in-game-traded under that flag, or under
    // FLAG_DEBUG (which can conjure any mon), are discarded below. Must be read before
    // ClearSav1() wipes the flags.
    bool32 discardRunLocalMons = FlagGet(FLAG_RANDOMIZE_MON) || FlagGet(FLAG_DEBUG);

    // Pass 1 -- move the party into the first free storage slots. Pass 2 sets the lock bit.
    for (i = 0; i < partyCount; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        enum Item heldItem;
        bool32 placed = FALSE;

        if (discardRunLocalMons)
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
        // Storage is full; the remaining party mons are lost.
        if (!placed)
            break;
    }

    // Pass 2 -- lock every surviving mon, or discard boxed mons this run caught or traded
    // for under FLAG_RANDOMIZE_MON / FLAG_DEBUG. Running after pass 1 also locks the boxed
    // party. Mons from earlier runs are already locked (the bit is never cleared short of
    // FLAG_SYS_GAME_CLEAR; see IsBoxMonWithdrawLocked), so re-setting it is a no-op.
    for (boxId = 0; boxId < TOTAL_BOXES_COUNT; boxId++)
    {
        for (boxPosition = 0; boxPosition < IN_BOX_COUNT; boxPosition++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(boxId, boxPosition);
            u32 otId;
            u8 recruitBattles = 0;

            if (!GetBoxMonData(boxMon, MON_DATA_SANITY_HAS_SPECIES))
                continue;

            otId = GetBoxMonData(boxMon, MON_DATA_OT_ID);
            if (discardRunLocalMons && (otId == outgoingOtId || IsIngameTradeOtId(otId)))
            {
                ZeroBoxMonData(boxMon);
                continue;
            }
            boxMon->legacyCarryOverLocked = TRUE;
            // The Recruits battle counter is per-run; reset it so the mon doesn't arrive
            // partway toward retiring.
            SetBoxMonData(boxMon, MON_DATA_RECRUIT_BATTLES, &recruitBattles);
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
            enum Species species;

            if (!GetBoxMonData(boxMon, MON_DATA_SANITY_HAS_SPECIES)
             || GetBoxMonData(boxMon, MON_DATA_SANITY_IS_EGG))
                continue;

            species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
            GetSetPokedexFlagBySpecies(species, FLAG_SET_SEEN);
            GetSetPokedexFlagBySpecies(species, FLAG_SET_CAUGHT);
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
    // Never TRUE for New Game+, which preserves the PC via its own backup/restore.
    bool32 keepStorage = !isNewGamePlus && gKeepStorageOnNewGame && gSaveFileStatus == SAVE_STATUS_OK;
    // Read before ClearSav1() wipes SaveBlock1 flags; restored after it for a plain
    // New Game so Auto-Scroll Text stays on (New Game+ restores it separately).
    bool8 autoScrollTextBackup = FlagGet(FLAG_AUTO_SCROLL_TEXT);

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
        playerSettingsBackup = Alloc(10);
        ((u8 *)playerSettingsBackup)[0] = gSaveBlock1Ptr->nuzlockeModeEnabled;
        ((u8 *)playerSettingsBackup)[1] = gSaveBlock1Ptr->autosaveModeEnabled;
        ((u8 *)playerSettingsBackup)[2] = gSaveBlock1Ptr->difficulty;
        // ClearSav1() would reset achievementsBlocked, unblocking a run blocked via the debug menu.
        ((u8 *)playerSettingsBackup)[3] = gSaveBlock1Ptr->achievementsBlocked;
        // These live in SaveBlock2, which ClearSav1() doesn't touch; backed up as a group with the other challenge settings.
        ((u8 *)playerSettingsBackup)[4] = gSaveBlock2Ptr->monoTypeSetting;
        ((u8 *)playerSettingsBackup)[5] = gSaveBlock2Ptr->monoGenSetting;
        ((u8 *)playerSettingsBackup)[6] = gSaveBlock2Ptr->limitedPartySetting;
        // SaveBlock1 fields wiped by ClearSav1(), like nuzlockeModeEnabled.
        ((u8 *)playerSettingsBackup)[7] = gSaveBlock1Ptr->draftModeEnabled;
        ((u8 *)playerSettingsBackup)[8] = gSaveBlock1Ptr->recruitsModeEnabled;
        ((u8 *)playerSettingsBackup)[9] = gSaveBlock2Ptr->rotationModeSetting;

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
        // Must run before ZeroPlayerPartyMons() and InitPlayerTrainerId(); the boxed
        // party needs its old OT ID.
        if (keepStorage)
            CarryStorageIntoNewGame();
        ZeroPlayerPartyMons();
        ResetPokedex();
        InitPlayerTrainerId();
        PlayTimeCounter_Reset();
        ClearPokedexFlags();
        // Skip the wipe when carrying storage over; box names, wallpapers,
        // currentBox and fusions[] are left as-is.
        if (!keepStorage)
            ResetPokemonStorageSystem();
        gPartiesCount[B_TRAINER_PLAYER] = 0;
        NewGameInitPCItems();
        // Exp Share defaults on for every fresh playthrough; SetDefaultOptions() only
        // covers a blank save, not a New Game over an existing one.
        gSaveBlock2Ptr->optionsExpShare = TRUE;
        // SetCurrentDifficultyLevel(DIFFICULTY_NORMAL); // OLD DIFFICULTY IMPLEMENTATION
        gSaveBlock2Ptr->newGamePlus = 0;
        ResetItemFlags();
        ResetDexNav();
        // Gates the OT-ID lock in pokemon_storage_system.c and is read by CB2_NewGame on the
        // Nuzlocke restart path. Set unconditionally to clear any earlier run's answer.
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
    // The New Game+ path never runs ClearSav2(), so SaveBlock2 state that must not carry
    // over is cleared here. Unconditional; harmless on the non-NG+ path.
    // A committed pendingTrade would otherwise make the boot hook in overworld.c prompt for
    // the previous playthrough's confirm code.
    memset(&gSaveBlock2Ptr->pendingTrade, 0, sizeof(gSaveBlock2Ptr->pendingTrade));
    // Doubles as "this area's draft is spent" in Draft runs (src/draft_mode.c).
    memset(gSaveBlock2Ptr->nuzlockeZoneCaughtFlags, 0, sizeof(gSaveBlock2Ptr->nuzlockeZoneCaughtFlags));
    // Style and colour slots are not reset here: this runs after the Birch speech's
    // colours menu. PlayerCustomization_ResetForNewGame() clears them where the
    // protagonist is picked; paths that skip that step keep the current look.
    InitEventData();
    // Must run after InitEventData(), which memsets the whole flags array again.
    if (!isNewGamePlus)
    {
        if (autoScrollTextBackup)
            FlagSet(FLAG_AUTO_SCROLL_TEXT);
        else
            FlagClear(FLAG_AUTO_SCROLL_TEXT);
    }
    // Must run after ClearSav1() wipes dexCaught/dexSeen.
    if (keepStorage)
        ReregisterCarriedOverDexEntries();
    if (!isNewGamePlus)
        ApplyPendingNewGameSettings();
    ClearTVShowData();
    ResetGabbyAndTy();
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
    // BOOST_STARTER_KIT. Must come after ClearBag(); skipped for New Game+, which restores
    // the previous bag and money below.
    if (!isNewGamePlus && AchievementBoost_HasStarterKit())
    {
        AddBagItem(ITEM_POTION, 5);
        AddBagItem(ITEM_FULL_HEAL, 2);
        AddBagItem(ITEM_REPEL, 3);
        AddBagItem(ITEM_ESCAPE_ROPE, 1);
        AddMoney(&gSaveBlock1Ptr->money, 3000); // on top of the 5000 set above
    }
    // BOOST_SHINY_CHARM_START/BOOST_ABILITY_CAPSULE_START/BOOST_ABILITY_PATCH_START.
    if (!isNewGamePlus && AchievementBoost_HasShinyCharmStart())
        AddBagItem(ITEM_SHINY_CHARM, 1);
    if (!isNewGamePlus && AchievementBoost_HasAbilityCapsuleStart())
        AddBagItem(ITEM_ABILITY_CAPSULE, 1);
    if (!isNewGamePlus && AchievementBoost_HasAbilityPatchStart())
        AddBagItem(ITEM_ABILITY_PATCH, 1);
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
    InitLilycoveLady();
    ResetAllApprenticeData();
    ClearRankingHallRecords();
    ClearMysteryGift();
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
                gSaveBlock1Ptr->draftModeEnabled = ((u8 *)playerSettingsBackup)[7];
                gSaveBlock2Ptr->limitedPartySetting = ((u8 *)playerSettingsBackup)[6];
                gSaveBlock1Ptr->recruitsModeEnabled = ((u8 *)playerSettingsBackup)[8];
                gSaveBlock2Ptr->rotationModeSetting = ((u8 *)playerSettingsBackup)[9];
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
            // Records the high-water mark in the achievement profile, which survives a corrupted or reset save.
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
