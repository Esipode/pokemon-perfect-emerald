#ifndef GUARD_SAVE_H
#define GUARD_SAVE_H

#include "main.h"

// Each 4 KiB flash sector contains 3968 bytes of actual data followed by a
// reserved region and then 12 bytes of footer. The reserved region used to
// carry a smeared-out SaveBlock3 chunk. SaveBlock3 now has its own dedicated sector,
// so this is just padding -- SECTOR_SIZE must stay exactly 4096 regardless,
// because it's the physical flash sector size (see the `sector.size` field
// of MX29L010 / LE26FV10N1TS / DefaultFlash in src/agb_flash_mx.c and
// src/agb_flash_le.c) and ProgramFlashSectorAndVerify/VerifyFlashSector
// always read/write that many bytes from whatever buffer they're given.
#define SECTOR_DATA_SIZE 3968
#define SECTOR_RESERVED_SIZE 116
#define SECTOR_FOOTER_SIZE 12
#define SECTOR_SIZE (SECTOR_DATA_SIZE + SECTOR_RESERVED_SIZE + SECTOR_FOOTER_SIZE)

#define NUM_SAVE_SLOTS 2

// If the sector's signature field is not this value then the sector is either invalid or empty.
#define SECTOR_SIGNATURE 0x8012025

#define SPECIAL_SECTOR_SENTINEL 0xB39D

// Sector map (32 sectors total, matching gFlash->sector.count -- see
// SectorsCountMatchesFlashChip below):
//
//   0 / 1-2 / 3    Slot A: SaveBlock2 / SaveBlock1 / SaveBlock3
//   4 / 5-6 / 7    Slot B: SaveBlock2 / SaveBlock1 / SaveBlock3
//   8-25           PokemonStorage -- single copy, not slot-rotated
//   26             PokemonStorage copy-on-write journal scratch
//   27-29          Spare (headroom for future box-count growth)
//   30-31          Achievement profile (primary + mirror), unchanged
//
// Unlike SaveBlock1/2/3, PokemonStorage is NOT duplicated across the two
// rotating slots: at up to 18 sectors it would cost as much flash as the
// rest of the save combined for a second copy. Its durability instead comes
// from per-sector dirty tracking plus the journal sector -- see the
// PokemonStorage persistence section of src/save.c for the full scheme.
#define SECTOR_ID_SAVEBLOCK2          0
#define SECTOR_ID_SAVEBLOCK1_START    1
#define SECTOR_ID_SAVEBLOCK1_END      2
#define SECTOR_ID_SAVEBLOCK3          3
#define NUM_SECTORS_PER_SLOT          4
// Save Slot A: 0-3;  Save Slot B: 4-7
#define SECTOR_ID_PKMN_STORAGE_START  8
#define SECTOR_ID_PKMN_STORAGE_END   25
#define NUM_PKMN_STORAGE_SECTORS     (SECTOR_ID_PKMN_STORAGE_END - SECTOR_ID_PKMN_STORAGE_START + 1)
#define SECTOR_ID_STORAGE_JOURNAL    26
#define SECTOR_ID_ACHIEVEMENTS        30
#define SECTOR_ID_ACHIEVEMENTS_BACKUP 31
#define SECTORS_COUNT                 32

// Sectors 0 - (NUM_SAVE_SLOT_SECTORS - 1) belong to the two rotating save
// slots (SaveBlock1/2/3 only, post Stage 6 -- PokemonStorage lives outside
// this range and is never touched by the slot-rotation code path).
#define NUM_SAVE_SLOT_SECTORS (NUM_SAVE_SLOTS * NUM_SECTORS_PER_SLOT) // 8

#define NUM_HOF_SECTORS 2

#define SAVE_STATUS_EMPTY    0
#define SAVE_STATUS_OK       1
#define SAVE_STATUS_CORRUPT  2
#define SAVE_STATUS_NO_FLASH 4
#define SAVE_STATUS_ERROR    0xFF

// Special sector id value for certain save functions to
// indicate that no specific sector should be used.
#define FULL_SAVE_SLOT 0xFFFF

// SetDamagedSectorBits states
enum
{
    ENABLE,
    DISABLE,
    CHECK // unused
};

// Do save types
enum
{
    SAVE_NORMAL,
    SAVE_LINK, // Link / Battle Frontier
    SAVE_EREADER, // deprecated in Emerald
    SAVE_HALL_OF_FAME,
    SAVE_OVERWRITE_DIFFERENT_FILE,
    SAVE_HALL_OF_FAME_ERASE_BEFORE // unused
};

// A save sector location holds a pointer to the data for a particular sector
// and the size of that data. Size cannot be greater than SECTOR_DATA_SIZE.
struct SaveSectorLocation
{
    void *data;
    u16 size;
};

struct SaveSector
{
    u8 data[SECTOR_DATA_SIZE];
    u8 reserved[SECTOR_RESERVED_SIZE]; // see the comment on SECTOR_RESERVED_SIZE above
    u16 id;
    u16 checksum;
    u32 signature;
    u32 counter;
}; // size is SECTOR_SIZE (0x1000)

#define SECTOR_SIGNATURE_OFFSET offsetof(struct SaveSector, signature)
#define SECTOR_COUNTER_OFFSET   offsetof(struct SaveSector, counter)

extern u16 gLastWrittenSector;
extern u32 gLastSaveCounter;
extern u16 gLastKnownGoodSector;
extern u32 gDamagedSaveSectors;
extern u32 gSaveCounter;
extern struct SaveSector *gFastSaveSector;
extern u16 gIncrementalSectorId;
extern u16 gSaveFileStatus;
// Separate from gSaveFileStatus on purpose: PokemonStorage is no longer part
// of the SaveBlock1/2/3 rotating slot, so its own read health (see
// LoadPokemonStorage in save.c) is no longer able to affect -- or be masked
// by -- whether the core save slot is considered valid.
extern u16 gPokemonStorageFileStatus;
extern MainCallback gGameContinueCallback;
extern struct SaveSectorLocation gRamSaveSectorLocations[];

extern struct SaveSector gSaveDataBuffer;

void ClearSaveData(void);
void Save_ResetSaveCounters(void);
u8 HandleSavingData(u8 saveType);
u8 TrySavingData(u8 saveType);
bool8 LinkFullSave_Init(void);
bool8 LinkFullSave_WriteSector(void);
bool8 LinkFullSave_ReplaceLastSector(void);
bool8 LinkFullSave_SetLastSectorSignature(void);
bool8 WriteSaveBlock2(void);
bool8 WriteSaveBlock1Sector(void);
u8 LoadGameSave(u8 saveType);
u16 GetSaveBlocksPointersBaseOffset(void);
u32 TryReadSpecialSaveSector(u8 sector, u8 *dst);
u32 TryWriteSpecialSaveSector(u8 sector, u8 *src);
void Task_LinkFullSave(u8 taskId);

// save_failed_screen.c
void DoSaveFailedScreen(u8 saveType);

#endif // GUARD_SAVE_H
