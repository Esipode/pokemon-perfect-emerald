#include "global.h"
#include "constants/trainers.h"
#include "player_customization.h"

static enum TrainerPicID GetEmeraldTrainerPic(enum Gender gender)
{
    return gender == MALE ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY;
}
static enum TrainerPicID GetRSTrainerPic(enum Gender gender)
{
    return gender == MALE ? TRAINER_PIC_RS_BRENDAN : TRAINER_PIC_RS_MAY;
}

static enum TrainerPicID GetKantoTrainerPic(enum Gender gender)
{
    return gender == MALE ? TRAINER_PIC_RED : TRAINER_PIC_LEAF;
}

enum TrainerPicID GetPlayerTrainerPic(enum Gender gender, enum GameVersion version)
{
    switch (version)
    {
        case VERSION_SAPPHIRE:
        case VERSION_RUBY:
            return GetRSTrainerPic(gender);
        case VERSION_LEAF_GREEN:
        case VERSION_FIRE_RED:
            return GetKantoTrainerPic(gender);
        case VERSION_EMERALD:
        default:
            return GetEmeraldTrainerPic(gender);
    }
}

// The player's own back pic. Link and recorded battles must keep using GetPlayerTrainerPic, so a
// remote player's pic never follows the local sprite style.
enum TrainerPicID GetLocalPlayerTrainerPic(void)
{
    if (Player_GetSpriteStyle() == PLAYER_SPRITE_STYLE_FRLG)
        return GetKantoTrainerPic(gSaveBlock2Ptr->playerGender);
    return GetPlayerTrainerPic(gSaveBlock2Ptr->playerGender, GAME_VERSION);
}
