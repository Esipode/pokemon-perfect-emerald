// Palette index -> region mapping for the player customization screen
// (Customization.md). Derived by dumping
// graphics/object_events/pics/people/<gender>/walking.png and
// graphics/trainers/front_pics/<gender>.png in PIL 'P' mode and reading off
// each region's pixels. Index 0 (transparency) and skin tones are
// deliberately excluded, as is pure black (15). Included from
// src/player_customization.c only.
//
// May's overworld and trainer palettes are identical except indexes 10/11,
// so one FEMALE mapping serves both. Brendan's differ: the overworld
// palette packs skin into 1-3 with hair at 4, while the trainer palette
// uses 1-4 as a four-step skin ramp -- his hair is hidden under the cap, so
// MALE has no trainer-pic entry for HAIR.
static const u8 sPlayerColorIndices_MaleOwHair[] = {4}; // hair brown
static const u8 sPlayerColorIndices_MaleOwHat[] = {9, 10, 11, 14}; // cap red/shading
static const u8 sPlayerColorIndices_MaleOwOutfit[] = {5, 6, 7, 8}; // jacket blues, incl. cap outline (5)
static const u8 sPlayerColorIndices_MaleOwAccent[] = {12, 13}; // bag straps/trim

static const u8 sPlayerColorIndices_MaleTrainerHat[] = {9, 10, 11, 14}; // cap red/shading
static const u8 sPlayerColorIndices_MaleTrainerOutfit[] = {5, 6, 7, 8}; // jacket blues, incl. cap outline (5)
static const u8 sPlayerColorIndices_MaleTrainerAccent[] = {12, 13}; // bag straps/trim

static const u8 sPlayerColorIndices_FemaleHair[] = {7, 8}; // hair red
static const u8 sPlayerColorIndices_FemaleHat[] = {10, 11}; // bandana
static const u8 sPlayerColorIndices_FemaleOutfit[] = {5, 6, 9, 14}; // shirt/shorts
static const u8 sPlayerColorIndices_FemaleAccent[] = {12, 13}; // bag straps/trim

static const struct PlayerColorRegionInfo sPlayerColorRegions[GENDER_COUNT][PLAYER_COLOR_REGION_COUNT] =
{
    [MALE] = {
        [PLAYER_COLOR_REGION_HAIR] = {
            .name = COMPOUND_STRING("HAIR"),
            .owIndices = sPlayerColorIndices_MaleOwHair,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_MaleOwHair),
            .trainerIndices = NULL,
            .numTrainerIndices = 0, // hidden under the cap in the trainer pic
        },
        [PLAYER_COLOR_REGION_HAT] = {
            .name = COMPOUND_STRING("HAT"),
            .owIndices = sPlayerColorIndices_MaleOwHat,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_MaleOwHat),
            .trainerIndices = sPlayerColorIndices_MaleTrainerHat,
            .numTrainerIndices = ARRAY_COUNT(sPlayerColorIndices_MaleTrainerHat),
        },
        [PLAYER_COLOR_REGION_OUTFIT] = {
            .name = COMPOUND_STRING("OUTFIT"),
            .owIndices = sPlayerColorIndices_MaleOwOutfit,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_MaleOwOutfit),
            .trainerIndices = sPlayerColorIndices_MaleTrainerOutfit,
            .numTrainerIndices = ARRAY_COUNT(sPlayerColorIndices_MaleTrainerOutfit),
        },
        [PLAYER_COLOR_REGION_ACCENT] = {
            .name = COMPOUND_STRING("ACCENT"),
            .owIndices = sPlayerColorIndices_MaleOwAccent,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_MaleOwAccent),
            .trainerIndices = sPlayerColorIndices_MaleTrainerAccent,
            .numTrainerIndices = ARRAY_COUNT(sPlayerColorIndices_MaleTrainerAccent),
        },
    },
    [FEMALE] = {
        [PLAYER_COLOR_REGION_HAIR] = {
            .name = COMPOUND_STRING("HAIR"),
            .owIndices = sPlayerColorIndices_FemaleHair,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleHair),
            .trainerIndices = sPlayerColorIndices_FemaleHair,
            .numTrainerIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleHair),
        },
        [PLAYER_COLOR_REGION_HAT] = {
            .name = COMPOUND_STRING("HAT"),
            .owIndices = sPlayerColorIndices_FemaleHat,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleHat),
            .trainerIndices = sPlayerColorIndices_FemaleHat,
            .numTrainerIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleHat),
        },
        [PLAYER_COLOR_REGION_OUTFIT] = {
            .name = COMPOUND_STRING("OUTFIT"),
            .owIndices = sPlayerColorIndices_FemaleOutfit,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleOutfit),
            .trainerIndices = sPlayerColorIndices_FemaleOutfit,
            .numTrainerIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleOutfit),
        },
        [PLAYER_COLOR_REGION_ACCENT] = {
            .name = COMPOUND_STRING("ACCENT"),
            .owIndices = sPlayerColorIndices_FemaleAccent,
            .numOwIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleAccent),
            .trainerIndices = sPlayerColorIndices_FemaleAccent,
            .numTrainerIndices = ARRAY_COUNT(sPlayerColorIndices_FemaleAccent),
        },
    },
};

// Stage 8 (Customization.md): index -> region mapping for the main-menu
// mugshot sprites (graphics/ui_main_menu/brendan_mugshot.png,
// may_mugshot.png). Derived the same way as sPlayerColorRegions -- dumping
// the indexed PNGs and reading off each region's pixels. These are separate
// 16-colour palettes from the overworld/trainer ones, laid out differently,
// so they get their own tables reusing the same struct (trainerIndices
// fields unused here).
//
// Brendan's mugshot hides his hair under the cap just like his trainer pic,
// so MALE has no HAIR entry. His jacket/cap-side blue-grey (index 2) and cap
// shadow (index 8) are grouped into OUTFIT alongside the collar trim (7).
static const u8 sMainMenuMugshotIndices_MaleHat[] = {5, 9}; // cap green crown + highlight crescent
static const u8 sMainMenuMugshotIndices_MaleOutfit[] = {2, 7, 8}; // jacket blue-grey body/shadow + collar trim
static const u8 sMainMenuMugshotIndices_MaleAccent[] = {10, 11}; // scarf/inner-shirt red + highlight

static const u8 sMainMenuMugshotIndices_FemaleHair[] = {2, 6}; // hair brown main + bangs highlight
static const u8 sMainMenuMugshotIndices_FemaleHat[] = {10, 11}; // bow ribbon + highlight
static const u8 sMainMenuMugshotIndices_FemaleOutfit[] = {7}; // collar trim
static const u8 sMainMenuMugshotIndices_FemaleAccent[] = {9, 12}; // shirt red + highlight

static const struct PlayerColorRegionInfo sMainMenuMugshotColorRegions[GENDER_COUNT][PLAYER_COLOR_REGION_COUNT] =
{
    [MALE] = {
        [PLAYER_COLOR_REGION_HAIR] = {0}, // hidden under the cap, same as the trainer pic
        [PLAYER_COLOR_REGION_HAT] = {
            .owIndices = sMainMenuMugshotIndices_MaleHat,
            .numOwIndices = ARRAY_COUNT(sMainMenuMugshotIndices_MaleHat),
        },
        [PLAYER_COLOR_REGION_OUTFIT] = {
            .owIndices = sMainMenuMugshotIndices_MaleOutfit,
            .numOwIndices = ARRAY_COUNT(sMainMenuMugshotIndices_MaleOutfit),
        },
        [PLAYER_COLOR_REGION_ACCENT] = {
            .owIndices = sMainMenuMugshotIndices_MaleAccent,
            .numOwIndices = ARRAY_COUNT(sMainMenuMugshotIndices_MaleAccent),
        },
    },
    [FEMALE] = {
        [PLAYER_COLOR_REGION_HAIR] = {
            .owIndices = sMainMenuMugshotIndices_FemaleHair,
            .numOwIndices = ARRAY_COUNT(sMainMenuMugshotIndices_FemaleHair),
        },
        [PLAYER_COLOR_REGION_HAT] = {
            .owIndices = sMainMenuMugshotIndices_FemaleHat,
            .numOwIndices = ARRAY_COUNT(sMainMenuMugshotIndices_FemaleHat),
        },
        [PLAYER_COLOR_REGION_OUTFIT] = {
            .owIndices = sMainMenuMugshotIndices_FemaleOutfit,
            .numOwIndices = ARRAY_COUNT(sMainMenuMugshotIndices_FemaleOutfit),
        },
        [PLAYER_COLOR_REGION_ACCENT] = {
            .owIndices = sMainMenuMugshotIndices_FemaleAccent,
            .numOwIndices = ARRAY_COUNT(sMainMenuMugshotIndices_FemaleAccent),
        },
    },
};
