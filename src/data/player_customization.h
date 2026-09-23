// Palette index -> region mapping for the player customization screen.
// Index 0 (transparency), skin tones and pure black (15) are deliberately excluded.
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

// --- Stage P2: slot and group tables -------------------------------------
// Source: Appendix A of the plan doc. Confidence varies by row -- Brendan
// and May are split from the shipped sPlayerColorRegions data above (which
// was already verified in-game by Part A); Red and Leaf have no such prior
// art and are this trace's own best read, so some slots are missing or
// have no trainer-pic indices yet (numTrainerIndices == 0: not traced, not
// "confirmed absent" the way Brendan's HAIR trainer slot is).

// Emerald male (Brendan). Slot ids: 0 Hair, 1 Bandana white, 2 Bandana
// green, 3 Jacket, 4 Cap outline, 5 Bag.
static const u8 sColorIdx_Brendan_Hair[] = {4};
static const u8 sColorIdx_Brendan_BandanaWhite[] = {9, 14};
static const u8 sColorIdx_Brendan_BandanaGreen[] = {10, 11};
static const u8 sColorIdx_Brendan_Jacket[] = {6, 7, 8};
static const u8 sColorIdx_Brendan_CapOutline[] = {5};
static const u8 sColorIdx_Brendan_Bag[] = {12, 13};
static const u8 sGroupSlots_Brendan_Hair[] = {0};
static const u8 sGroupSlots_Brendan_Hat[] = {1, 2};
static const u8 sGroupSlots_Brendan_Outfit[] = {3, 4};
static const u8 sGroupSlots_Brendan_Accent[] = {5};

// Emerald female (May). Slot ids: 0 Hair, 1 Bandana, 2 Cap outline,
// 3 Shorts, 4 White trim, 5 Bag.
static const u8 sColorIdx_May_Hair[] = {7, 8};
static const u8 sColorIdx_May_Bandana[] = {10, 11};
static const u8 sColorIdx_May_CapOutline[] = {5};
static const u8 sColorIdx_May_Shorts[] = {6};
static const u8 sColorIdx_May_WhiteTrim[] = {9, 14};
static const u8 sColorIdx_May_Bag[] = {12, 13};
static const u8 sGroupSlots_May_Hair[] = {0};
static const u8 sGroupSlots_May_Hat[] = {1};
static const u8 sGroupSlots_May_Outfit[] = {2, 3, 4};
static const u8 sGroupSlots_May_Accent[] = {5};

// FRLG male (Red). No shipped baseline -- OW indices only, per Appendix A
// A.3; trainer-pic roles not traced yet (numTrainerIndices left at 0).
// Slot ids: 0 Hair, 1 Cap, 2 Jacket/jeans, 3 Shoe trim.
static const u8 sColorIdx_Red_Hair_Ow[] = {8};
static const u8 sColorIdx_Red_Cap_Ow[] = {9, 11, 12};
static const u8 sColorIdx_Red_Jacket_Ow[] = {6, 10};
static const u8 sColorIdx_Red_ShoeTrim_Ow[] = {7};
static const u8 sGroupSlots_Red_Hair[] = {0};
static const u8 sGroupSlots_Red_Hat[] = {1};
static const u8 sGroupSlots_Red_Outfit[] = {2};
static const u8 sGroupSlots_Red_Accent[] = {3};

// FRLG female (Leaf). No shipped baseline -- OW indices only, per Appendix A
// A.4; trainer-pic roles not traced, and no HAIR slot was found in the
// traced frame (may be fully hidden under her hat, unconfirmed). Slot ids
// start at 1, not 0 -- slot 0 (HAIR) is left unused rather than reindexing,
// so "slot N means the same logical colour" doesn't drift if HAIR turns up
// later. Slot ids: 1 Hat trim, 2 Hat brim, 3 Vest, 4 Accent.
static const u8 sColorIdx_Leaf_HatTrim_Ow[] = {9, 10};
static const u8 sColorIdx_Leaf_HatBrim_Ow[] = {12};
static const u8 sColorIdx_Leaf_Vest_Ow[] = {6};
static const u8 sColorIdx_Leaf_Accent_Ow[] = {5, 13};

static const struct PlayerColorSlotInfo
    sPlayerColorSlots[PLAYER_SPRITE_STYLE_COUNT][GENDER_COUNT][PLAYER_COLOR_SLOT_COUNT] =
{
    [PLAYER_SPRITE_STYLE_EMERALD] = {
        [MALE] = {
            [0] = {COMPOUND_STRING("HAIR"), sColorIdx_Brendan_Hair, NULL, ARRAY_COUNT(sColorIdx_Brendan_Hair), 0},
            [1] = {COMPOUND_STRING("BANDANA"), sColorIdx_Brendan_BandanaWhite, sColorIdx_Brendan_BandanaWhite, ARRAY_COUNT(sColorIdx_Brendan_BandanaWhite), ARRAY_COUNT(sColorIdx_Brendan_BandanaWhite)},
            [2] = {COMPOUND_STRING("BANDANA TRIM"), sColorIdx_Brendan_BandanaGreen, sColorIdx_Brendan_BandanaGreen, ARRAY_COUNT(sColorIdx_Brendan_BandanaGreen), ARRAY_COUNT(sColorIdx_Brendan_BandanaGreen)},
            [3] = {COMPOUND_STRING("JACKET"), sColorIdx_Brendan_Jacket, sColorIdx_Brendan_Jacket, ARRAY_COUNT(sColorIdx_Brendan_Jacket), ARRAY_COUNT(sColorIdx_Brendan_Jacket)},
            [4] = {COMPOUND_STRING("CAP TRIM"), sColorIdx_Brendan_CapOutline, sColorIdx_Brendan_CapOutline, ARRAY_COUNT(sColorIdx_Brendan_CapOutline), ARRAY_COUNT(sColorIdx_Brendan_CapOutline)},
            [5] = {COMPOUND_STRING("BAG"), sColorIdx_Brendan_Bag, sColorIdx_Brendan_Bag, ARRAY_COUNT(sColorIdx_Brendan_Bag), ARRAY_COUNT(sColorIdx_Brendan_Bag)},
        },
        [FEMALE] = {
            [0] = {COMPOUND_STRING("HAIR"), sColorIdx_May_Hair, sColorIdx_May_Hair, ARRAY_COUNT(sColorIdx_May_Hair), ARRAY_COUNT(sColorIdx_May_Hair)},
            [1] = {COMPOUND_STRING("BANDANA"), sColorIdx_May_Bandana, sColorIdx_May_Bandana, ARRAY_COUNT(sColorIdx_May_Bandana), ARRAY_COUNT(sColorIdx_May_Bandana)},
            [2] = {COMPOUND_STRING("BANDANA TRIM"), sColorIdx_May_CapOutline, sColorIdx_May_CapOutline, ARRAY_COUNT(sColorIdx_May_CapOutline), ARRAY_COUNT(sColorIdx_May_CapOutline)},
            [3] = {COMPOUND_STRING("SHORTS"), sColorIdx_May_Shorts, sColorIdx_May_Shorts, ARRAY_COUNT(sColorIdx_May_Shorts), ARRAY_COUNT(sColorIdx_May_Shorts)},
            [4] = {COMPOUND_STRING("TRIM"), sColorIdx_May_WhiteTrim, sColorIdx_May_WhiteTrim, ARRAY_COUNT(sColorIdx_May_WhiteTrim), ARRAY_COUNT(sColorIdx_May_WhiteTrim)},
            [5] = {COMPOUND_STRING("BAG"), sColorIdx_May_Bag, sColorIdx_May_Bag, ARRAY_COUNT(sColorIdx_May_Bag), ARRAY_COUNT(sColorIdx_May_Bag)},
        },
    },
    [PLAYER_SPRITE_STYLE_FRLG] = {
        [MALE] = {
            [0] = {COMPOUND_STRING("HAIR"), sColorIdx_Red_Hair_Ow, NULL, ARRAY_COUNT(sColorIdx_Red_Hair_Ow), 0},
            [1] = {COMPOUND_STRING("CAP"), sColorIdx_Red_Cap_Ow, NULL, ARRAY_COUNT(sColorIdx_Red_Cap_Ow), 0},
            [2] = {COMPOUND_STRING("JACKET"), sColorIdx_Red_Jacket_Ow, NULL, ARRAY_COUNT(sColorIdx_Red_Jacket_Ow), 0},
            [3] = {COMPOUND_STRING("SHOES"), sColorIdx_Red_ShoeTrim_Ow, NULL, ARRAY_COUNT(sColorIdx_Red_ShoeTrim_Ow), 0},
        },
        [FEMALE] = {
            // No slot 0 (HAIR) -- not found in the traced frame, see Appendix A.4.
            [1] = {COMPOUND_STRING("HAT"), sColorIdx_Leaf_HatTrim_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_HatTrim_Ow), 0},
            [2] = {COMPOUND_STRING("HAT BRIM"), sColorIdx_Leaf_HatBrim_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_HatBrim_Ow), 0},
            [3] = {COMPOUND_STRING("VEST"), sColorIdx_Leaf_Vest_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_Vest_Ow), 0},
            [4] = {COMPOUND_STRING("ACCENT"), sColorIdx_Leaf_Accent_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_Accent_Ow), 0},
        },
    },
};

// Leaf's slot ids shift by one (no slot 0) because her HAIR slot is missing
// rather than reindexed -- keeps "slot N means the same logical colour
// across styles" from silently drifting if HAIR is found later.
static const u8 sGroupSlots_Leaf_HatShifted[] = {1, 2};
static const u8 sGroupSlots_Leaf_OutfitShifted[] = {3};
static const u8 sGroupSlots_Leaf_AccentShifted[] = {4};

static const struct PlayerColorGroupInfo
    sPlayerColorGroups[PLAYER_SPRITE_STYLE_COUNT][GENDER_COUNT][PLAYER_COLOR_REGION_COUNT] =
{
    [PLAYER_SPRITE_STYLE_EMERALD] = {
        [MALE] = {
            [PLAYER_COLOR_REGION_HAIR] = {COMPOUND_STRING("HAIR"), sGroupSlots_Brendan_Hair, ARRAY_COUNT(sGroupSlots_Brendan_Hair)},
            [PLAYER_COLOR_REGION_HAT] = {COMPOUND_STRING("HAT"), sGroupSlots_Brendan_Hat, ARRAY_COUNT(sGroupSlots_Brendan_Hat)},
            [PLAYER_COLOR_REGION_OUTFIT] = {COMPOUND_STRING("OUTFIT"), sGroupSlots_Brendan_Outfit, ARRAY_COUNT(sGroupSlots_Brendan_Outfit)},
            [PLAYER_COLOR_REGION_ACCENT] = {COMPOUND_STRING("ACCENT"), sGroupSlots_Brendan_Accent, ARRAY_COUNT(sGroupSlots_Brendan_Accent)},
        },
        [FEMALE] = {
            [PLAYER_COLOR_REGION_HAIR] = {COMPOUND_STRING("HAIR"), sGroupSlots_May_Hair, ARRAY_COUNT(sGroupSlots_May_Hair)},
            [PLAYER_COLOR_REGION_HAT] = {COMPOUND_STRING("HAT"), sGroupSlots_May_Hat, ARRAY_COUNT(sGroupSlots_May_Hat)},
            [PLAYER_COLOR_REGION_OUTFIT] = {COMPOUND_STRING("OUTFIT"), sGroupSlots_May_Outfit, ARRAY_COUNT(sGroupSlots_May_Outfit)},
            [PLAYER_COLOR_REGION_ACCENT] = {COMPOUND_STRING("ACCENT"), sGroupSlots_May_Accent, ARRAY_COUNT(sGroupSlots_May_Accent)},
        },
    },
    [PLAYER_SPRITE_STYLE_FRLG] = {
        [MALE] = {
            [PLAYER_COLOR_REGION_HAIR] = {COMPOUND_STRING("HAIR"), sGroupSlots_Red_Hair, ARRAY_COUNT(sGroupSlots_Red_Hair)},
            [PLAYER_COLOR_REGION_HAT] = {COMPOUND_STRING("HAT"), sGroupSlots_Red_Hat, ARRAY_COUNT(sGroupSlots_Red_Hat)},
            [PLAYER_COLOR_REGION_OUTFIT] = {COMPOUND_STRING("OUTFIT"), sGroupSlots_Red_Outfit, ARRAY_COUNT(sGroupSlots_Red_Outfit)},
            [PLAYER_COLOR_REGION_ACCENT] = {COMPOUND_STRING("ACCENT"), sGroupSlots_Red_Accent, ARRAY_COUNT(sGroupSlots_Red_Accent)},
        },
        [FEMALE] = {
            // No PLAYER_COLOR_REGION_HAIR entry -- Leaf has no HAIR slot yet (see above).
            [PLAYER_COLOR_REGION_HAT] = {COMPOUND_STRING("HAT"), sGroupSlots_Leaf_HatShifted, ARRAY_COUNT(sGroupSlots_Leaf_HatShifted)},
            [PLAYER_COLOR_REGION_OUTFIT] = {COMPOUND_STRING("OUTFIT"), sGroupSlots_Leaf_OutfitShifted, ARRAY_COUNT(sGroupSlots_Leaf_OutfitShifted)},
            [PLAYER_COLOR_REGION_ACCENT] = {COMPOUND_STRING("ACCENT"), sGroupSlots_Leaf_AccentShifted, ARRAY_COUNT(sGroupSlots_Leaf_AccentShifted)},
        },
    },
};

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
