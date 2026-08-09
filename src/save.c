#include "global.h"
#include "agb_flash.h"
#include "gba/flash_internal.h"
#include "fieldmap.h"
#include "save.h"
#include "task.h"
#include "decompress.h"
#include "load_save.h"
#include "overworld.h"
#include "hall_of_fame.h"
#include "pokemon_storage_system.h"
#include "trainer_hill.h"
#include "link.h"
#include "constants/game_stat.h"
#include "achievements.h"

static u16 CalculateChecksum(void *, u16);
static bool8 ReadFlashSector(u8, struct SaveSector *);
static u8 GetSaveValidStatus(const struct SaveSectorLocation *);
static u8 CopySaveSlotData(u16, struct SaveSectorLocation *);
static u8 TryWriteSector(u8, u8 *);
static u8 HandleWriteSector(u16, const struct SaveSectorLocation *);
static u8 HandleReplaceSector(u16, const struct SaveSectorLocation *);
static u8 WriteStorageSectorJournaled(u16 chunk);
static void WriteStorageSectorsIfDirty(void);
static void InvalidatePokemonStorageSectorCache(void);
static u8 LoadPokemonStorage(void);

// Divide save blocks into individual chunks to be written to flash sectors

/*
 * Sector Layout: see the big comment block in save.h for the full 32-sector
 * map. In short:
 *
 * Sectors 0-3:    Save Slot A (SaveBlock2 / SaveBlock1 / SaveBlock3)
 * Sectors 4-7:    Save Slot B (SaveBlock2 / SaveBlock1 / SaveBlock3)
 * Sectors 8-25:   PokemonStorage (single copy, not slot-rotated)
 * Sector  26:     PokemonStorage journal scratch
 * Sectors 27-29:  Spare
 * Sector  30:     Achievement Profile (primary)
 * Sector  31:     Achievement Profile (mirror)
 *
 * There are two save slots for saving the player's game data. We alternate between
 * them each time the game is saved, so that if the current save slot is corrupt,
 * we can load the previous one. We also rotate the sectors in each save slot
 * so that the same data is not always being written to the same sector. This
 * might be done to reduce wear on the flash memory, but I'm not sure, since all
 * 4 sectors get written anyway.
 *
 * PokemonStorage is deliberately NOT part of this slot rotation -- see the
 * PokemonStorage persistence section further down this file for why and how
 * its durability is handled instead.
 *
 * See SECTOR_ID_* constants in save.h
 */

#define SAVEBLOCK_CHUNK(structure, chunkNum)                                   \
{                                                                              \
    chunkNum * SECTOR_DATA_SIZE,                                               \
    sizeof(structure) >= chunkNum * SECTOR_DATA_SIZE ?                         \
    min(sizeof(structure) - chunkNum * SECTOR_DATA_SIZE, SECTOR_DATA_SIZE) : 0 \
}

struct
{
    u16 offset;
    u16 size;
} static const sSaveSlotLayout[NUM_SECTORS_PER_SLOT] =
{
    SAVEBLOCK_CHUNK(struct SaveBlock2, 0), // SECTOR_ID_SAVEBLOCK2

    SAVEBLOCK_CHUNK(struct SaveBlock1, 0), // SECTOR_ID_SAVEBLOCK1_START
    SAVEBLOCK_CHUNK(struct SaveBlock1, 1), // SECTOR_ID_SAVEBLOCK1_END

    SAVEBLOCK_CHUNK(struct SaveBlock3, 0), // SECTOR_ID_SAVEBLOCK3 -- its own sector now, no more tail-smearing
};

// PokemonStorage's own per-sector layout table, sized to the full reserved
// range (see NUM_PKMN_STORAGE_SECTORS in save.h) regardless of how many
// sectors the struct currently needs -- SAVEBLOCK_CHUNK already produces
// size 0 for chunks beyond sizeof(struct PokemonStorage), so unused reserve
// sectors are simply never read or written. Indexed 0..NUM_PKMN_STORAGE_SECTORS-1;
// add SECTOR_ID_PKMN_STORAGE_START to get the absolute flash sector.
// offset is u32, not u16 like sSaveSlotLayout's: chunkNum * SECTOR_DATA_SIZE
// is computed (then discarded via the size-0 case) even for reserve chunks
// far past sizeof(struct PokemonStorage), and at chunk 17 that's 67,456 --
// already past what a u16 can hold, regardless of the struct's current size.
struct
{
    u32 offset;
    u16 size;
} static const sPkmnStorageLayout[NUM_PKMN_STORAGE_SECTORS] =
{
    SAVEBLOCK_CHUNK(struct PokemonStorage, 0),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 1),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 2),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 3),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 4),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 5),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 6),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 7),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 8),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 9),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 10),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 11),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 12),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 13),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 14),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 15),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 16),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 17),
};

// These will produce an error if a save struct is larger than the space
// alloted for it in the flash.
STATIC_ASSERT(sizeof(struct SaveBlock3) <= SECTOR_DATA_SIZE, SaveBlock3FreeSpace);
STATIC_ASSERT(sizeof(struct SaveBlock2) <= SECTOR_DATA_SIZE, SaveBlock2FreeSpace);
STATIC_ASSERT(sizeof(struct SaveBlock1) <= SECTOR_DATA_SIZE * (SECTOR_ID_SAVEBLOCK1_END - SECTOR_ID_SAVEBLOCK1_START + 1), SaveBlock1FreeSpace);
STATIC_ASSERT(sizeof(struct PokemonStorage) <= SECTOR_DATA_SIZE * (SECTOR_ID_PKMN_STORAGE_END - SECTOR_ID_PKMN_STORAGE_START + 1), PokemonStorageFreeSpace);

STATIC_ASSERT(SECTOR_ID_ACHIEVEMENTS >= NUM_SAVE_SLOT_SECTORS, AchievementSectorOutsideSaveSlots);
// Guard rails for the Stage 6 sector remap: PokemonStorage and its journal
// scratch sector must sit strictly between the rotating slots and the
// achievement profile, and the whole map must still fit the physical chip.
STATIC_ASSERT(SECTOR_ID_PKMN_STORAGE_START >= NUM_SAVE_SLOT_SECTORS, StorageSectorsOutsideSaveSlots);
STATIC_ASSERT(SECTOR_ID_STORAGE_JOURNAL > SECTOR_ID_PKMN_STORAGE_END, JournalSectorAfterStorage);
STATIC_ASSERT(SECTOR_ID_ACHIEVEMENTS > SECTOR_ID_STORAGE_JOURNAL, AchievementSectorAfterJournal);
STATIC_ASSERT(SECTORS_COUNT == 32, SectorsCountMatchesFlashChip); // gFlash->sector.count is always 32 on this hardware (see src/agb_flash_mx.c / agb_flash_le.c)

// The achievement profile lives outside the save slots and must survive every
// save wipe. Nothing in the save-slot code path may erase it.
static u16 EraseSaveSlotSector(u16 sector)
{
    if (sector >= NUM_SAVE_SLOT_SECTORS)
        return 0x80FF; // refuse
    return EraseFlashSector(sector);
}

COMMON_DATA u16 gLastWrittenSector = 0;
COMMON_DATA u32 gLastSaveCounter = 0;
COMMON_DATA u16 gLastKnownGoodSector = 0;
COMMON_DATA u32 gDamagedSaveSectors = 0;
COMMON_DATA u32 gSaveCounter = 0;
COMMON_DATA struct SaveSector *gReadWriteSector = NULL; // Pointer to a buffer for reading/writing a sector
COMMON_DATA u16 gIncrementalSectorId = 0;
COMMON_DATA u16 gSaveFileStatus = 0;
COMMON_DATA u16 gPokemonStorageFileStatus = 0;
COMMON_DATA MainCallback gGameContinueCallback = NULL;
COMMON_DATA struct SaveSectorLocation gRamSaveSectorLocations[NUM_SECTORS_PER_SLOT] = {0};
COMMON_DATA u16 gSaveAttemptStatus = 0;

EWRAM_DATA struct SaveSector gSaveDataBuffer = {0}; // Buffer used for reading/writing sectors

// Per-sector dirty-tracking cache for PokemonStorage -- see the persistence
// section further down this file. Indexed the same way as sPkmnStorageLayout.
static u16 sPkmnStorageSectorChecksum[NUM_PKMN_STORAGE_SECTORS];
// Whether sPkmnStorageSectorChecksum[i] reflects data that is actually known
// to be safely on flash right now (set on a successful load or write, and
// deliberately zero-initialized to FALSE so the very first save of a fresh
// game unconditionally writes every sector instead of assuming a match).
static bool8 sPkmnStorageSectorValid[NUM_PKMN_STORAGE_SECTORS];

void ClearSaveData(void)
{
    u16 i;

    // Erases every sector except the achievement profile and its mirror
    // (SECTOR_ID_ACHIEVEMENTS / _BACKUP, 30-31), which must survive every
    // save wipe. This covers the two rotating SaveBlock1/2/3 slots (0-7),
    // PokemonStorage (8-25), and its journal scratch sector (26) -- unlike
    // pre-Stage-6, where storage only ever lived inside the rotating slots
    // and the old NUM_SAVE_SLOT_SECTORS-bounded loop already covered it for
    // free, storage now needs its own sectors explicitly included here.
    // Deliberately calls EraseFlashSector directly rather than
    // EraseSaveSlotSector: that helper's gate is intentionally narrower (see
    // its own comment) and would refuse everything from sector 8 up.
    for (i = 0; i < SECTOR_ID_ACHIEVEMENTS; i++)
        EraseFlashSector(i);
}

void Save_ResetSaveCounters(void)
{
    gSaveCounter = 0;
    gLastWrittenSector = 0;
    gDamagedSaveSectors = 0;
}

static bool32 SetDamagedSectorBits(u8 op, u8 sectorId)
{
    bool32 retVal = FALSE;

    switch (op)
    {
    case ENABLE:
        gDamagedSaveSectors |= (1 << sectorId);
        break;
    case DISABLE:
        gDamagedSaveSectors &= ~(1 << sectorId);
        break;
    case CHECK: // unused
        if (gDamagedSaveSectors & (1 << sectorId))
            retVal = TRUE;
        break;
    }

    return retVal;
}

static u8 WriteSaveSectorOrSlot(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u32 status;
    u16 i;

    gReadWriteSector = &gSaveDataBuffer;

    if (sectorId != FULL_SAVE_SLOT)
    {
        // A sector was specified, just write that sector.
        // This is never reached, FULL_SAVE_SLOT is always used instead.
        status = HandleWriteSector(sectorId, locations);
    }
    else
    {
        // No sector was specified, write full save slot.
        gLastKnownGoodSector = gLastWrittenSector; // backup the current written sector before attempting to write.
        gLastSaveCounter = gSaveCounter;
        gLastWrittenSector++;
        gLastWrittenSector = gLastWrittenSector % NUM_SECTORS_PER_SLOT;
        gSaveCounter++;
        status = SAVE_STATUS_OK;

        for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
            HandleWriteSector(i, locations);

        if (gDamagedSaveSectors)
        {
            // At least one sector save failed
            status = SAVE_STATUS_ERROR;
            gLastWrittenSector = gLastKnownGoodSector;
            gSaveCounter = gLastSaveCounter;
        }
    }

    return status;
}

static u8 HandleWriteSector(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 i;
    u16 sector;
    u8 *data;
    u16 size;

    // Adjust sector id for current save slot
    sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Get current save data
    data = locations[sectorId].data;
    size = locations[sectorId].size;

    // Clear temp save sector
    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)gReadWriteSector)[i] = 0;

    // Set footer data
    gReadWriteSector->id = sectorId;
    gReadWriteSector->signature = SECTOR_SIGNATURE;
    gReadWriteSector->counter = gSaveCounter;

    // Copy current data to temp buffer for writing
    for (i = 0; i < size; i++)
        gReadWriteSector->data[i] = data[i];

    gReadWriteSector->checksum = CalculateChecksum(data, size);

    return TryWriteSector(sector, gReadWriteSector->data);
}

static u8 TryWriteSector(u8 sector, u8 *data)
{
    if (ProgramFlashSectorAndVerify(sector, data)) // is damaged?
    {
        // Failed
        SetDamagedSectorBits(ENABLE, sector);
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u32 RestoreSaveBackupVarsAndIncrement(const struct SaveSectorLocation *locations)
{
    gReadWriteSector = &gSaveDataBuffer;
    gLastKnownGoodSector = gLastWrittenSector;
    gLastSaveCounter = gSaveCounter;
    gLastWrittenSector++;
    gLastWrittenSector %= NUM_SECTORS_PER_SLOT;
    gSaveCounter++;
    gIncrementalSectorId = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

static u32 RestoreSaveBackupVars(const struct SaveSectorLocation *locations)
{
    gReadWriteSector = &gSaveDataBuffer;
    gLastKnownGoodSector = gLastWrittenSector;
    gLastSaveCounter = gSaveCounter;
    gIncrementalSectorId = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

static u8 HandleWriteIncrementalSector(u16 numSectors, const struct SaveSectorLocation *locations)
{
    u8 status;

    if (gIncrementalSectorId < numSectors - 1)
    {
        status = SAVE_STATUS_OK;
        HandleWriteSector(gIncrementalSectorId, locations);
        gIncrementalSectorId++;
        if (gDamagedSaveSectors)
        {
            status = SAVE_STATUS_ERROR;
            gLastWrittenSector = gLastKnownGoodSector;
            gSaveCounter = gLastSaveCounter;
        }
    }
    else
    {
        // Exceeded max sector, finished
        status = SAVE_STATUS_ERROR;
    }

    return status;
}

static u8 HandleReplaceSectorAndVerify(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u8 status = SAVE_STATUS_OK;

    HandleReplaceSector(sectorId - 1, locations);

    if (gDamagedSaveSectors)
    {
        status = SAVE_STATUS_ERROR;
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
    }
    return status;
}

// Similar to HandleWriteSector, but fully erases the sector first, and skips writing the first signature byte
static u8 HandleReplaceSector(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 i;
    u16 sector;
    u8 *data;
    u16 size;
    u8 status;

    // Adjust sector id for current save slot
    sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Get current save data
    data = locations[sectorId].data;
    size = locations[sectorId].size;

    // Clear temp save sector.
    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)gReadWriteSector)[i] = 0;

    // Set footer data
    gReadWriteSector->id = sectorId;
    gReadWriteSector->signature = SECTOR_SIGNATURE;
    gReadWriteSector->counter = gSaveCounter;

    // Copy current data to temp buffer for writing
    for (i = 0; i < size; i++)
        gReadWriteSector->data[i] = data[i];

    gReadWriteSector->checksum = CalculateChecksum(data, size);

    // Erase old save data
    EraseSaveSlotSector(sector);

    status = SAVE_STATUS_OK;

    // Write new save data up to signature field
    for (i = 0; i < SECTOR_SIGNATURE_OFFSET; i++)
    {
        if (ProgramFlashByte(sector, i, ((u8 *)gReadWriteSector)[i]))
        {
            status = SAVE_STATUS_ERROR;
            break;
        }
    }

    if (status == SAVE_STATUS_ERROR)
    {
        // Writing save data failed
        SetDamagedSectorBits(ENABLE, sector);
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Writing save data succeeded, write signature and counter
        status = SAVE_STATUS_OK;

        // Write signature (skipping the first byte) and counter fields.
        // The byte of signature that is skipped is instead written by WriteSectorSignatureByte or WriteSectorSignatureByte_NoOffset
        for (i = 0; i < SECTOR_SIZE - (SECTOR_SIGNATURE_OFFSET + 1); i++)
        {
            if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET + 1 + i, ((u8 *)gReadWriteSector)[SECTOR_SIGNATURE_OFFSET + 1 + i]))
            {
                status = SAVE_STATUS_ERROR;
                break;
            }
        }

        if (status == SAVE_STATUS_ERROR)
        {
            // Writing signature/counter failed
            SetDamagedSectorBits(ENABLE, sector);
            return SAVE_STATUS_ERROR;
        }
        else
        {
            // Succeeded
            SetDamagedSectorBits(DISABLE, sector);
            return SAVE_STATUS_OK;
        }
    }
}

static u8 WriteSectorSignatureByte_NoOffset(u16 sectorId, const struct SaveSectorLocation *locations)
{
    // Adjust sector id for current save slot
    // This first line lacking -1 is the only difference from WriteSectorSignatureByte
    u16 sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Write just the first byte of the signature field, which was skipped by HandleReplaceSector
    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, SECTOR_SIGNATURE & 0xFF))
    {
        // Sector is damaged, so enable the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 CopySectorSignatureByte(u16 sectorId, const struct SaveSectorLocation *locations)
{
    // Adjust sector id for current save slot
    u16 sector = sectorId + gLastWrittenSector - 1;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Copy just the first byte of the signature field from the read/write buffer
    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, ((u8 *)gReadWriteSector)[SECTOR_SIGNATURE_OFFSET]))
    {
        // Sector is damaged, so enable the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 WriteSectorSignatureByte(u16 sectorId, const struct SaveSectorLocation *locations)
{
    // Adjust sector id for current save slot
    u16 sector = sectorId + gLastWrittenSector - 1;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Write just the first byte of the signature field, which was skipped by HandleReplaceSector
    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, SECTOR_SIGNATURE & 0xFF))
    {
        // Sector is damaged, so enable the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 TryLoadSaveSlot(u16 sectorId, struct SaveSectorLocation *locations)
{
    u8 status;
    gReadWriteSector = &gSaveDataBuffer;
    if (sectorId != FULL_SAVE_SLOT)
    {
        // This function may not be used with a specific sector id
        status = SAVE_STATUS_ERROR;
    }
    else
    {
        status = GetSaveValidStatus(locations);
        CopySaveSlotData(FULL_SAVE_SLOT, locations);
    }

    return status;
}

// sectorId arg is ignored, this always reads the full save slot
static u8 CopySaveSlotData(u16 sectorId, struct SaveSectorLocation *locations)
{
    u16 i;
    u16 checksum;
    u16 slotOffset = NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);
    u16 id;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + slotOffset, gReadWriteSector);

        id = gReadWriteSector->id;
        if (id == 0)
            gLastWrittenSector = i;

        checksum = CalculateChecksum(gReadWriteSector->data, locations[id].size);

        // Only copy data for sectors whose signature and checksum fields are correct
        if (gReadWriteSector->signature == SECTOR_SIGNATURE && gReadWriteSector->checksum == checksum)
        {
            u16 j;
            for (j = 0; j < locations[id].size; j++)
                ((u8 *)locations[id].data)[j] = gReadWriteSector->data[j];
        }
    }

    return SAVE_STATUS_OK;
}

static u8 GetSaveValidStatus(const struct SaveSectorLocation *locations)
{
    u16 i;
    u16 checksum;
    u32 saveSlot1Counter = 0;
    u32 saveSlot2Counter = 0;
    u32 validSectorFlags = 0;
    bool8 signatureValid = FALSE;
    u8 saveSlot1Status;
    u8 saveSlot2Status;

    // Check save slot 1
    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i, gReadWriteSector);
        if (gReadWriteSector->signature == SECTOR_SIGNATURE)
        {
            signatureValid = TRUE;
            checksum = CalculateChecksum(gReadWriteSector->data, locations[gReadWriteSector->id].size);
            if (gReadWriteSector->checksum == checksum)
            {
                saveSlot1Counter = gReadWriteSector->counter;
                validSectorFlags |= 1 << gReadWriteSector->id;
            }
        }
    }

    if (signatureValid)
    {
        if (validSectorFlags == (1 << NUM_SECTORS_PER_SLOT) - 1)
            saveSlot1Status = SAVE_STATUS_OK;
        else
            saveSlot1Status = SAVE_STATUS_ERROR;
    }
    else
    {
        // No sectors in slot 1 have the correct signature, treat it as empty
        saveSlot1Status = SAVE_STATUS_EMPTY;
    }

    validSectorFlags = 0;
    signatureValid = FALSE;

    // Check save slot 2
    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + NUM_SECTORS_PER_SLOT, gReadWriteSector);
        if (gReadWriteSector->signature == SECTOR_SIGNATURE)
        {
            signatureValid = TRUE;
            checksum = CalculateChecksum(gReadWriteSector->data, locations[gReadWriteSector->id].size);
            if (gReadWriteSector->checksum == checksum)
            {
                saveSlot2Counter = gReadWriteSector->counter;
                validSectorFlags |= 1 << gReadWriteSector->id;
            }
        }
    }

    if (signatureValid)
    {
        if (validSectorFlags == (1 << NUM_SECTORS_PER_SLOT) - 1)
            saveSlot2Status = SAVE_STATUS_OK;
        else
            saveSlot2Status = SAVE_STATUS_ERROR;
    }
    else
    {
        // No sectors in slot 2 have the correct signature, treat it as empty.
        saveSlot2Status = SAVE_STATUS_EMPTY;
    }

    if (saveSlot1Status == SAVE_STATUS_OK && saveSlot2Status == SAVE_STATUS_OK)
    {
        if ((saveSlot1Counter == -1 && saveSlot2Counter ==  0)
         || (saveSlot1Counter ==  0 && saveSlot2Counter == -1))
        {
            if ((unsigned)(saveSlot1Counter + 1) < (unsigned)(saveSlot2Counter + 1))
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        else
        {
            if (saveSlot1Counter < saveSlot2Counter)
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        return SAVE_STATUS_OK;
    }

    // One or both save slots are not OK

    if (saveSlot1Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot1Counter;
        if (saveSlot2Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR; // Slot 2 errored
        return SAVE_STATUS_OK; // Slot 1 is OK, slot 2 is empty
    }

    if (saveSlot2Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot2Counter;
        if (saveSlot1Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR; // Slot 1 errored
        return SAVE_STATUS_OK; // Slot 2 is OK, slot 1 is empty
    }

    // Neither slot is OK, check if both are empty
    if (saveSlot1Status == SAVE_STATUS_EMPTY
     && saveSlot2Status == SAVE_STATUS_EMPTY)
    {
        gSaveCounter = 0;
        gLastWrittenSector = 0;
        return SAVE_STATUS_EMPTY;
    }

    // Both slots errored
    gSaveCounter = 0;
    gLastWrittenSector = 0;
    return SAVE_STATUS_CORRUPT;
}

// Return value always ignored
static bool8 ReadFlashSector(u8 sectorId, struct SaveSector *sector)
{
    ReadFlash(sectorId, 0, sector->data, SECTOR_SIZE);
    return TRUE;
}

static u16 CalculateChecksum(void *data, u16 size)
{
    u16 i;
    u32 checksum = 0;

    for (i = 0; i < (size / 4); i++)
    {
        checksum += *((u32 *)data);
        data += sizeof(u32);
    }

    return ((checksum >> 16) + checksum);
}

// Populates gRamSaveSectorLocations for the rotating slot only (SaveBlock2,
// SaveBlock1, SaveBlock3). PokemonStorage is deliberately not part of this
// table -- it's addressed directly via sPkmnStorageLayout + gPokemonStoragePtr
// in the PokemonStorage persistence section further down this file, since it
// isn't slot-rotated and needs none of gRamSaveSectorLocations' relative
// addressing.
static void UpdateSaveAddresses(void)
{
    int i = SECTOR_ID_SAVEBLOCK2;
    gRamSaveSectorLocations[i].data = (void *)(gSaveBlock2Ptr) + sSaveSlotLayout[i].offset;
    gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;

    for (i = SECTOR_ID_SAVEBLOCK1_START; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
    {
        gRamSaveSectorLocations[i].data = (void *)(gSaveBlock1Ptr) + sSaveSlotLayout[i].offset;
        gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;
    }

    i = SECTOR_ID_SAVEBLOCK3;
    gRamSaveSectorLocations[i].data = (void *)(gSaveBlock3Ptr) + sSaveSlotLayout[i].offset;
    gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;
}

u8 HandleSavingData(u8 saveType)
{
    u8 i;
    u32 *backupVar = gTrainerHillVBlankCounter;

    gTrainerHillVBlankCounter = NULL;
    UpdateSaveAddresses();
    switch (saveType)
    {
    case SAVE_HALL_OF_FAME_ERASE_BEFORE:
        // Unused. Hall of Fame no longer has dedicated flash sectors (see the
        // save.h sector remap), so there is nothing left to erase before saving it.
        // fallthrough
    case SAVE_HALL_OF_FAME:
        if (GetGameStat(GAME_STAT_ENTERED_HOF) < 999)
            IncrementGameStat(GAME_STAT_ENTERED_HOF);
        // fallthrough - Hall of Fame team data is no longer written to its own
        // sectors; only the current save slot is written, same as SAVE_NORMAL.
    case SAVE_NORMAL:
    default:
        CopyPartyAndObjectsToSave();
        WriteSaveSectorOrSlot(FULL_SAVE_SLOT, gRamSaveSectorLocations);
        WriteStorageSectorsIfDirty();
        break;
    case SAVE_LINK:
    case SAVE_EREADER: // Dummied, now duplicate of SAVE_LINK
        // Used by link / Battle Frontier
        // Write only SaveBlocks 1 and 2 (skips the PC) -- PokemonStorage was
        // never included here even before Stage 6's sector remap, so this
        // stays as-is.
        CopyPartyAndObjectsToSave();
        for (i = SECTOR_ID_SAVEBLOCK2; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
            HandleReplaceSector(i, gRamSaveSectorLocations);
        for (i = SECTOR_ID_SAVEBLOCK2; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
            WriteSectorSignatureByte_NoOffset(i, gRamSaveSectorLocations);
        break;
    case SAVE_OVERWRITE_DIFFERENT_FILE:
        // Overwrite save slot. Previously also erased the Hall of Fame sectors
        // first; those sectors no longer exist (see the save.h sector remap).
        // The storage dirty-tracking cache is invalidated first: it may still
        // reflect whatever save file was previously loaded, which is not a
        // safe baseline to diff a different logical save file against.
        InvalidatePokemonStorageSectorCache();
        CopyPartyAndObjectsToSave();
        WriteSaveSectorOrSlot(FULL_SAVE_SLOT, gRamSaveSectorLocations);
        WriteStorageSectorsIfDirty();
        break;
    }
    gTrainerHillVBlankCounter = backupVar;
    return 0;
}

u8 TrySavingData(u8 saveType)
{
    // Independent of the save slots below: flushes the achievement profile
    // whenever a normal save happens, success or not.
    Achievement_FlushProfile();

    if (gFlashMemoryPresent != TRUE)
    {
        gSaveAttemptStatus = SAVE_STATUS_ERROR;
        return SAVE_STATUS_ERROR;
    }

    HandleSavingData(saveType);
    if (!gDamagedSaveSectors)
    {
        gSaveAttemptStatus = SAVE_STATUS_OK;
        return SAVE_STATUS_OK;
    }
    else
    {
        DoSaveFailedScreen(saveType);
        gSaveAttemptStatus = SAVE_STATUS_ERROR;
        return SAVE_STATUS_ERROR;
    }
}

bool8 LinkFullSave_Init(void)
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;
    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    RestoreSaveBackupVarsAndIncrement(gRamSaveSectorLocations);
    return FALSE;
}

// Called repeatedly (once per frame-ish) until it returns TRUE. Writes
// SaveBlock2 and SaveBlock1's sectors first (SaveBlock3, the slot's final
// sector, is committed afterward by LinkFullSave_ReplaceLastSector /
// LinkFullSave_SetLastSectorSignature's deferred-signature-byte trick,
// unchanged by Stage 6), then continues into PokemonStorage's own dirty,
// journaled sectors -- this is what the callers' "does save the PC data"
// comments (see e.g. src/trade.c) refer to. Unlike the rotating slot,
// storage has no second copy to fall back on, so each of its sectors commits
// via WriteStorageSectorJournaled instead of the slot's atomic-last-sector
// trick; per-sector journaling is what makes that safe (see the
// PokemonStorage persistence section further down this file).
bool8 LinkFullSave_WriteSector(void)
{
    u8 status;

    if (gIncrementalSectorId < NUM_SECTORS_PER_SLOT - 1)
    {
        status = HandleWriteIncrementalSector(NUM_SECTORS_PER_SLOT, gRamSaveSectorLocations);
    }
    else
    {
        u16 chunk = gIncrementalSectorId - (NUM_SECTORS_PER_SLOT - 1);
        if (chunk < NUM_PKMN_STORAGE_SECTORS)
        {
            WriteStorageSectorJournaled(chunk);
            gIncrementalSectorId++;
            status = SAVE_STATUS_OK;
        }
        else
        {
            status = SAVE_STATUS_ERROR; // Exceeded max sector, finished (mirrors HandleWriteIncrementalSector's own sentinel).
        }
    }

    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);

    // In this case "error" either means that an actual error was encountered
    // or that the given max sector has been reached (meaning it has finished successfully).
    // If there was an actual error the save failed screen above will also be shown.
    if (status == SAVE_STATUS_ERROR)
        return TRUE;
    else
        return FALSE;
}

bool8 LinkFullSave_ReplaceLastSector(void)
{
    HandleReplaceSectorAndVerify(NUM_SECTORS_PER_SLOT, gRamSaveSectorLocations);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

bool8 LinkFullSave_SetLastSectorSignature(void)
{
    CopySectorSignatureByte(NUM_SECTORS_PER_SLOT, gRamSaveSectorLocations);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

bool8 WriteSaveBlock2(void)
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;

    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    RestoreSaveBackupVars(gRamSaveSectorLocations);

    // Because RestoreSaveBackupVars is called immediately prior, gIncrementalSectorId will always be 0 below,
    // so this function only saves the first sector (SECTOR_ID_SAVEBLOCK2)
    HandleReplaceSectorAndVerify(gIncrementalSectorId + 1, gRamSaveSectorLocations);
    return FALSE;
}

// Used in conjunction with WriteSaveBlock2 to write both for certain link saves.
// This will be called repeatedly in a task, writing each sector of SaveBlock1,
// and now also SaveBlock3 (its own sector as of the Stage 6 remap, rather than
// a tail smeared across every sector -- see save.h), incrementally.
// It returns TRUE when finished.
bool8 WriteSaveBlock1Sector(void)
{
    bool32 finished = FALSE;
    u16 sectorId = ++gIncrementalSectorId; // Because WriteSaveBlock2 will have been called prior, this will be SECTOR_ID_SAVEBLOCK1_START
    if (sectorId <= NUM_SECTORS_PER_SLOT - 1) // walks SaveBlock1's chunks, then SaveBlock3, the slot's final sector
    {
        // Write a single sector of SaveBlock1/SaveBlock3
        HandleReplaceSectorAndVerify(gIncrementalSectorId + 1, gRamSaveSectorLocations);
        WriteSectorSignatureByte(sectorId, gRamSaveSectorLocations);
    }
    else
    {
        // Beyond SaveBlock3, don't write the sector.
        // Does write 1 byte of the next sector's signature field, but as these
        // are the same for all valid sectors it doesn't matter.
        WriteSectorSignatureByte(sectorId, gRamSaveSectorLocations);
        finished = TRUE;
    }

    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_LINK);

    return finished;
}

u8 LoadGameSave(u8 saveType)
{
    u8 status;

    if (gFlashMemoryPresent != TRUE)
    {
        gSaveFileStatus = SAVE_STATUS_NO_FLASH;
        return SAVE_STATUS_ERROR;
    }

    UpdateSaveAddresses();
    switch (saveType)
    {
    case SAVE_NORMAL:
    default:
        status = TryLoadSaveSlot(FULL_SAVE_SLOT, gRamSaveSectorLocations);
        // Independent of the slot's own status -- see gPokemonStorageFileStatus's
        // declaration in save.h for why a storage read failure can no longer
        // make the whole save look invalid.
        gPokemonStorageFileStatus = LoadPokemonStorage();
        CopyPartyAndObjectsFromSave();
        gSaveFileStatus = status;
        gGameContinueCallback = NULL;
        break;
    case SAVE_HALL_OF_FAME:
        // Hall of Fame team data no longer has a dedicated flash sector (see the
        // save.h sector remap). Always report failure so callers (Sav2_HallOfFame
        // and friends) treat the record list as empty, matching the pre-existing
        // (already broken) Hall of Fame persistence behavior on this fork.
        status = SAVE_STATUS_ERROR;
        break;
    }

    return status;
}

u16 GetSaveBlocksPointersBaseOffset(void)
{
    u16 i, slotOffset;
    struct SaveSector *sector;

    sector = gReadWriteSector = &gSaveDataBuffer;
    if (gFlashMemoryPresent != TRUE)
        return 0;
    UpdateSaveAddresses();
    GetSaveValidStatus(gRamSaveSectorLocations);
    slotOffset = NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);
    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + slotOffset, gReadWriteSector);

        // Base offset for SaveBlock2 is calculated using the trainer id
        if (gReadWriteSector->id == SECTOR_ID_SAVEBLOCK2)
            return sector->data[offsetof(struct SaveBlock2, playerTrainerId[0])] +
                   sector->data[offsetof(struct SaveBlock2, playerTrainerId[1])] +
                   sector->data[offsetof(struct SaveBlock2, playerTrainerId[2])] +
                   sector->data[offsetof(struct SaveBlock2, playerTrainerId[3])];
    }
    return 0;
}

// Trainer Hill and Recorded Battle previously used dedicated flash sectors
// (34 and 35) that already fell outside the physical 32-sector chip, so their
// persistence was already non-functional. Those sector IDs are gone now that
// sectors 30-31 are reserved for the achievement profile (see save.h), so
// these always report failure, preserving the existing (broken) behavior
// without touching flash sectors that don't belong to them.
u32 TryReadSpecialSaveSector(u8 sector, u8 *dst)
{
    return SAVE_STATUS_ERROR;
}

u32 TryWriteSpecialSaveSector(u8 sector, u8 *src)
{
    return SAVE_STATUS_ERROR;
}

#define tState         data[0]
#define tTimer         data[1]
#define tInBattleTower data[2]

// Note that this is very different from TrySavingData(SAVE_LINK).
// Most notably it does save the PC data.
void Task_LinkFullSave(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        gSoftResetDisabled = TRUE;
        tState = 1;
        break;
    case 1:
        SetLinkStandbyCallback();
        tState = 2;
        break;
    case 2:
        if (IsLinkTaskFinished())
        {
            if (!tInBattleTower)
                SaveMapView();
            tState = 3;
        }
        break;
    case 3:
        if (!tInBattleTower)
            SetContinueGameWarpStatusToDynamicWarp();
        LinkFullSave_Init();
        tState = 4;
        break;
    case 4:
        if (++tTimer == 5)
        {
            tTimer = 0;
            tState = 5;
        }
        break;
    case 5:
        if (LinkFullSave_WriteSector())
            tState = 6;
        else
            tState = 4; // Not finished, delay again
        break;
    case 6:
        LinkFullSave_ReplaceLastSector();
        tState = 7;
        break;
    case 7:
        if (!tInBattleTower)
            ClearContinueGameWarpStatus2();
        SetLinkStandbyCallback();
        tState = 8;
        break;
    case 8:
        if (IsLinkTaskFinished())
        {
            LinkFullSave_SetLastSectorSignature();
            tState = 9;
        }
        break;
    case 9:
        SetLinkStandbyCallback();
        tState = 10;
        break;
    case 10:
        if (IsLinkTaskFinished())
            tState++;
        break;
    case 11:
        if (++tTimer > 5)
        {
            gSoftResetDisabled = FALSE;
            DestroyTask(taskId);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// PokemonStorage persistence (single copy, journaled, dirty-tracked)
// ---------------------------------------------------------------------------
//
// PokemonStorage is not part of the SaveBlock1/2/3 slot rotation above: it
// lives once, at a fixed absolute sector range (SECTOR_ID_PKMN_STORAGE_START
// .. _END), addressed the same way regardless of gSaveCounter/gLastWrittenSector.
// A second full copy would cost as many flash sectors as the rest of the
// entire save combined, which is the opposite of what this whole project is
// trying to buy back (see Saveblock Shrinking.md).
//
// Durability instead comes from two things working together:
//
//   1. Per-sector dirty tracking (sPkmnStorageSectorChecksum/Valid, declared
//      near the top of this file): a save only ever rewrites sectors whose
//      contents actually changed since the last successful read or write of
//      that sector. Untouched boxes are never rewritten, which keeps flash
//      wear down and -- more importantly for the point below -- keeps the
//      window where a mid-write power loss can matter as small as possible.
//
//   2. A copy-on-write journal (SECTOR_ID_STORAGE_JOURNAL). Before a changed
//      sector's home is ever touched, the exact candidate contents are
//      written to the scratch sector and verified there first (this is what
//      ProgramFlashSectorAndVerify already does per call). Only once that
//      succeeds does the home sector get overwritten:
//        - A power loss during the *journal* write leaves the home sector
//          completely untouched -- still whatever it was before, still
//          valid. The half-written journal sector simply fails its own
//          checksum on the next boot and is ignored.
//        - A power loss during the *home* write is the only way the home
//          sector itself can end up corrupt. On the next boot,
//          LoadPokemonStorageSector notices the home sector's checksum no
//          longer matches, finds the journal sector still holds a verified,
//          complete candidate for that exact sector id, and replays it back
//          into the home sector to repair it.
//      A single storage box spans at most two sectors, so this per-sector
//      atomicity is sufficient -- there is never a need to make a write span
//      multiple sectors atomically as one unit.
//
// Unlike gSaveFileStatus (which only reflects the small SaveBlock1/2/3 slot),
// a corrupt or unrecoverable storage sector can no longer make the game
// treat the whole save as invalid -- see gPokemonStorageFileStatus in save.h.

// Assembles a full sector buffer (in gSaveDataBuffer) for storage chunk
// `chunk`, ready to hand to ProgramFlashSectorAndVerify. `id` is stamped with
// the chunk's absolute home sector -- unlike the rotating slot's sector `id`
// field (which only ever holds a small 0..3 logical chunk index, because
// slot sectors physically move between the two slots), storage never moves,
// so its own `id` doubling as "which physical sector this candidate belongs
// to" is unambiguous and is exactly what LoadPokemonStorageSector needs to
// recognize a matching journal entry on replay.
static void BuildStorageSectorBuffer(u16 chunk, const u8 *data, u16 size)
{
    u16 i;

    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)&gSaveDataBuffer)[i] = 0;

    gSaveDataBuffer.id = SECTOR_ID_PKMN_STORAGE_START + chunk;
    gSaveDataBuffer.signature = SECTOR_SIGNATURE;
    gSaveDataBuffer.counter = gSaveCounter;
    for (i = 0; i < size; i++)
        gSaveDataBuffer.data[i] = data[i];
    gSaveDataBuffer.checksum = CalculateChecksum((void *)data, size);
}

// Journals and commits one storage sector if -- and only if -- it has
// actually changed since the last successful read or write. Called once per
// chunk, either in a tight loop (a normal full save) or one chunk per call
// across many frames (the incremental link-save path -- see
// LinkFullSave_WriteSector). Failures set gDamagedSaveSectors bits exactly
// like the slot-sector writers above, so they participate in the same
// DoSaveFailedScreen / retry-wipe flow as everything else (see
// save_failed_screen.c's WipeSectors).
static u8 WriteStorageSectorJournaled(u16 chunk)
{
    u8 *data;
    u16 size;
    u16 checksum;
    u16 absoluteSector;

    if (chunk >= NUM_PKMN_STORAGE_SECTORS)
        return SAVE_STATUS_OK;

    size = sPkmnStorageLayout[chunk].size;
    if (size == 0)
        return SAVE_STATUS_OK; // Reserved sector, not needed at the current box count.

    data = (u8 *)gPokemonStoragePtr + sPkmnStorageLayout[chunk].offset;
    checksum = CalculateChecksum(data, size);

    if (sPkmnStorageSectorValid[chunk] && sPkmnStorageSectorChecksum[chunk] == checksum)
        return SAVE_STATUS_OK; // Unchanged since the last successful read/write -- nothing to do.

    absoluteSector = SECTOR_ID_PKMN_STORAGE_START + chunk;

    // Copy-on-write: land the candidate in the scratch sector and verify it
    // before ever touching the home sector (see the section comment above).
    BuildStorageSectorBuffer(chunk, data, size);
    if (ProgramFlashSectorAndVerify(SECTOR_ID_STORAGE_JOURNAL, (u8 *)&gSaveDataBuffer) != 0)
    {
        SetDamagedSectorBits(ENABLE, SECTOR_ID_STORAGE_JOURNAL);
        return SAVE_STATUS_ERROR;
    }
    SetDamagedSectorBits(DISABLE, SECTOR_ID_STORAGE_JOURNAL);

    // The journal write already verified this buffer byte-for-byte, so the
    // home write can reuse it as-is.
    if (ProgramFlashSectorAndVerify(absoluteSector, (u8 *)&gSaveDataBuffer) != 0)
    {
        SetDamagedSectorBits(ENABLE, absoluteSector);
        sPkmnStorageSectorValid[chunk] = FALSE; // Cache is now unknown, not just stale -- force a retry next time.
        return SAVE_STATUS_ERROR;
    }
    SetDamagedSectorBits(DISABLE, absoluteSector);

    sPkmnStorageSectorChecksum[chunk] = checksum;
    sPkmnStorageSectorValid[chunk] = TRUE;
    return SAVE_STATUS_OK;
}

// Walks every storage sector, journaling and committing whichever ones are
// actually dirty. Used by the ordinary (non-link) save paths in
// HandleSavingData, which don't need the one-sector-per-call pacing the
// incremental link-save path (LinkFullSave_WriteSector) uses instead.
static void WriteStorageSectorsIfDirty(void)
{
    u16 chunk;

    for (chunk = 0; chunk < NUM_PKMN_STORAGE_SECTORS; chunk++)
        WriteStorageSectorJournaled(chunk);
}

// Forces every storage sector to be treated as unknown, so the next
// WriteStorageSectorsIfDirty/WriteStorageSectorJournaled call rewrites all of
// them unconditionally instead of trusting a cached checksum. Used by
// SAVE_OVERWRITE_DIFFERENT_FILE: the cache may have been seeded from
// whatever save file was previously loaded, which is not a safe baseline to
// diff a different logical save file against.
static void InvalidatePokemonStorageSectorCache(void)
{
    u16 chunk;

    for (chunk = 0; chunk < NUM_PKMN_STORAGE_SECTORS; chunk++)
        sPkmnStorageSectorValid[chunk] = FALSE;
}

// Loads one storage sector's data into gPokemonStoragePtr, replaying the
// journal scratch sector if the home sector is corrupt (see the section
// comment above). Returns TRUE if the chunk's data in RAM is now known-good,
// whether it came from the home sector or from a successful journal replay.
// Returns FALSE if neither is usable, in which case RAM is left untouched --
// the same thing happens, harmlessly, on a brand new save with nothing valid
// on flash yet, since new_game.c already initializes PokemonStorage in RAM
// before any of this ever runs.
static bool8 LoadPokemonStorageSector(u16 chunk)
{
    u16 size = sPkmnStorageLayout[chunk].size;
    u16 absoluteSector = SECTOR_ID_PKMN_STORAGE_START + chunk;
    u8 *dest;
    u16 checksum;

    if (size == 0)
        return TRUE; // Reserved sector, nothing to load.

    dest = (u8 *)gPokemonStoragePtr + sPkmnStorageLayout[chunk].offset;

    ReadFlashSector(absoluteSector, &gSaveDataBuffer);
    if (gSaveDataBuffer.signature == SECTOR_SIGNATURE)
    {
        checksum = CalculateChecksum(gSaveDataBuffer.data, size);
        if (gSaveDataBuffer.checksum == checksum)
        {
            memcpy(dest, gSaveDataBuffer.data, size);
            sPkmnStorageSectorChecksum[chunk] = checksum;
            sPkmnStorageSectorValid[chunk] = TRUE;
            return TRUE;
        }
    }

    // Home sector missing or corrupt -- see if the journal scratch sector
    // holds a verified candidate for this exact sector and replay it.
    ReadFlashSector(SECTOR_ID_STORAGE_JOURNAL, &gSaveDataBuffer);
    if (gSaveDataBuffer.signature == SECTOR_SIGNATURE && gSaveDataBuffer.id == absoluteSector)
    {
        checksum = CalculateChecksum(gSaveDataBuffer.data, size);
        if (gSaveDataBuffer.checksum == checksum)
        {
            // Repair the home sector so future saves aren't relying on the
            // journal sector staying intact, then load from the same buffer.
            ProgramFlashSectorAndVerify(absoluteSector, (u8 *)&gSaveDataBuffer);
            memcpy(dest, gSaveDataBuffer.data, size);
            sPkmnStorageSectorChecksum[chunk] = checksum;
            sPkmnStorageSectorValid[chunk] = TRUE;
            return TRUE;
        }
    }

    // Neither home nor journal has a usable copy. Mark the cache unknown so
    // the next save unconditionally (re)writes this sector rather than
    // assuming it still matches whatever is on flash.
    sPkmnStorageSectorValid[chunk] = FALSE;
    return FALSE;
}

// Loads all of PokemonStorage from flash. Returns SAVE_STATUS_OK if every
// sector loaded cleanly, SAVE_STATUS_EMPTY if none did (a brand new save,
// not corruption -- there is nothing valid on flash yet for any sector),
// or SAVE_STATUS_CORRUPT if some sectors loaded and others didn't (a
// genuinely partial read). Deliberately independent of gSaveFileStatus --
// see gPokemonStorageFileStatus's declaration in save.h for why.
static u8 LoadPokemonStorage(void)
{
    u16 chunk;
    bool8 anyValid = FALSE;
    bool8 anyInvalid = FALSE;

    for (chunk = 0; chunk < NUM_PKMN_STORAGE_SECTORS; chunk++)
    {
        if (sPkmnStorageLayout[chunk].size == 0)
            continue;

        if (LoadPokemonStorageSector(chunk))
            anyValid = TRUE;
        else
            anyInvalid = TRUE;
    }

    if (!anyInvalid)
        return anyValid ? SAVE_STATUS_OK : SAVE_STATUS_EMPTY;
    if (anyValid)
        return SAVE_STATUS_CORRUPT;
    return SAVE_STATUS_EMPTY; // Nothing loaded anywhere: a fresh cart, not corruption.
}
