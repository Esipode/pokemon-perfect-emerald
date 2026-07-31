#include "global.h"
#include "gba/flash_internal.h"
#include "agb_flash.h"
#include "load_save.h"
#include "save.h"
#include "achievements.h"

// The whole struct is written as one blob to a sector (see WriteAchievementProfile).
STATIC_ASSERT(sizeof(struct AchievementProfile) <= SECTOR_SIZE, AchievementProfileFreeSpace);

EWRAM_DATA struct AchievementProfile gAchievementProfile = {0};

// Separate from gDamagedSaveSectors on purpose (see achievements.h): a profile
// read/write failure must never be able to trigger the save-failed screen.
// Must default to FALSE (zero-initialized): non-zero static initializers land
// in a plain .data section that ld_script_modern.ld has no rule for and the
// trailing /DISCARD/ silently drops, leaving .text with a dangling reference.
static bool8 sAchievementProfileWriteFailed = FALSE;

// Set by mutators in this file (Stage 1.4's Achievement_TryComplete, boost
// purchase/reset, etc.) and cleared once Achievement_FlushProfile writes the
// profile out. Keeps flash writes off the hot path: a mutation only costs a
// flash write once, at the next safe flush point, not on every call.
static bool8 sAchievementProfileDirty = FALSE;

static u16 CalculateProfileChecksum(const struct AchievementProfile *profile)
{
    u16 offset = offsetof(struct AchievementProfile, totalPointsEarned);
    const u32 *data = (const u32 *)((const u8 *)profile + offset);
    u16 size = sizeof(*profile) - offset;
    u32 checksum = 0;
    u16 i;

    for (i = 0; i < size / 4; i++)
        checksum += data[i];

    return (checksum >> 16) + checksum;
}

static void InitDefaultAchievementProfile(void)
{
    memset(&gAchievementProfile, 0, sizeof(gAchievementProfile));
    gAchievementProfile.magic = ACHIEVEMENT_PROFILE_MAGIC;
    gAchievementProfile.version = ACHIEVEMENT_PROFILE_VERSION;
    gAchievementProfile.boostsEnabled = TRUE;
}

// Reads directly into a scratch struct rather than gAchievementProfile so a
// bad sector can never partially clobber the live profile.
static bool8 TryLoadAchievementProfileSector(u16 sector)
{
    struct AchievementProfile buffer;

    ReadFlash(sector, 0, (u8 *)&buffer, sizeof(buffer));

    if (buffer.magic != ACHIEVEMENT_PROFILE_MAGIC)
        return FALSE;
    if (buffer.version != ACHIEVEMENT_PROFILE_VERSION)
        return FALSE;
    if (buffer.checksum != CalculateProfileChecksum(&buffer))
        return FALSE;

    gAchievementProfile = buffer;
    return TRUE;
}

void ReadAchievementProfile(void)
{
    if (gFlashMemoryPresent != TRUE)
    {
        InitDefaultAchievementProfile();
        return;
    }

    if (!TryLoadAchievementProfileSector(SECTOR_ID_ACHIEVEMENTS)
     && !TryLoadAchievementProfileSector(SECTOR_ID_ACHIEVEMENTS_BACKUP))
    {
        InitDefaultAchievementProfile();
    }

    // NOTE: once Stage 7 adds struct AchievementBoost (with its per-boost
    // maxLevel), clamp each boostLevels[i] to that ceiling here so corrupt
    // flash data can never hand back an out-of-range boost level.
}

// ProgramFlashSectorAndVerify always writes a full flash sector's worth of
// bytes from src, so it's given a full SECTOR_SIZE scratch buffer rather than
// &gAchievementProfile directly. gSaveDataBuffer is save.c's own sector-sized
// scratch space, already reused the same way by TryWriteSpecialSaveSector.
void WriteAchievementProfile(void)
{
    if (gFlashMemoryPresent != TRUE)
        return;

    gAchievementProfile.checksum = CalculateProfileChecksum(&gAchievementProfile);

    memset(&gSaveDataBuffer, 0, SECTOR_SIZE);
    memcpy(&gSaveDataBuffer, &gAchievementProfile, sizeof(gAchievementProfile));

    sAchievementProfileWriteFailed = FALSE;

    if (ProgramFlashSectorAndVerify(SECTOR_ID_ACHIEVEMENTS, (u8 *)&gSaveDataBuffer))
        sAchievementProfileWriteFailed = TRUE;

    // Mirror. Either sector failing marks the whole write as an error.
    if (ProgramFlashSectorAndVerify(SECTOR_ID_ACHIEVEMENTS_BACKUP, (u8 *)&gSaveDataBuffer))
        sAchievementProfileWriteFailed = TRUE;
}

bool8 Achievement_ProfileWriteFailed(void)
{
    return sAchievementProfileWriteFailed;
}

// Call sites (design doc §1.3): the overworld at the same safe point the
// achievement popup task runs, TrySavingData (so a normal save always
// flushes), and immediately after a boost purchase or boost reset. Achievements
// earned mid-battle flush on return to the field, not in-battle: this fails in
// the safe direction, where a hard reset can lose an award but never double-award it.
void Achievement_FlushProfile(void)
{
    if (!sAchievementProfileDirty)
        return;

    WriteAchievementProfile();
    sAchievementProfileDirty = FALSE;
}
