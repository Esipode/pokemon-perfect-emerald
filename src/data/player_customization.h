// --- Stage P2: slot and group tables -------------------------------------
// Source: Appendix A of the plan doc. Confidence varies by row -- Brendan
// and May are split from the shipped per-region data Part A already
// verified in-game; Red and Leaf are traced from their sprite sheets.
// numTrainerIndices == 0 means the slot's colour has no safe front-pic
// equivalent (see the per-character notes below).
// Slots the menu does not show are either dropped entirely (name NULL,
// follows PLAYER_COLOR_SLOT_NONE -- they keep their ROM colour) or slaved to
// a visible slot (follows = that slot), which keeps ramps and outlines in
// step with the colour they shade instead of drifting apart.

// Emerald male (Brendan). Slot ids: 0 Shirt primary, 1 Hat, 2 Hat trim,
// 3 Jacket (hidden), 4 Cap outline (hidden), 5 Gloves and boots.
static const u8 sColorIdx_Brendan_Hair[] = {4};
static const u8 sColorIdx_Brendan_BandanaWhite[] = {9, 14};
static const u8 sColorIdx_Brendan_BandanaGreen[] = {10, 11};
static const u8 sColorIdx_Brendan_Jacket[] = {6, 7, 8};
static const u8 sColorIdx_Brendan_CapOutline[] = {5};
static const u8 sColorIdx_Brendan_Bag[] = {12, 13};
static const u8 sGroupSlots_Brendan_Hat[] = {1, 2};
static const u8 sGroupSlots_Brendan_Outfit[] = {0};
static const u8 sGroupSlots_Brendan_Accent[] = {5};

// Emerald female (May). Slot ids: 0 Hair, 1 Hat and bag, 2 Cap outline
// (hidden), 3 Shorts (hidden), 4 Hat accent, 5 Shirt. Hair is index 7 only
// -- index 8 is the whole sprite's inner outline, not a hair shade.
static const u8 sColorIdx_May_Hair[] = {7};
static const u8 sColorIdx_May_Bandana[] = {10, 11};
static const u8 sColorIdx_May_CapOutline[] = {5};
static const u8 sColorIdx_May_Shorts[] = {6};
static const u8 sColorIdx_May_WhiteTrim[] = {9, 14};
static const u8 sColorIdx_May_Bag[] = {12, 13};
static const u8 sGroupSlots_May_Hair[] = {0};
static const u8 sGroupSlots_May_Hat[] = {1};
static const u8 sGroupSlots_May_Outfit[] = {5};
static const u8 sGroupSlots_May_Accent[] = {4};

// FRLG male (Red). One OW index per slot: slots paint every index they own
// with one absolute colour, and Red's OW indices mostly pair distinct colours
// (e.g. white band vs red cap) rather than shades of one colour.
// Slot ids: 0 Hair + cap outline, 1 Cap top, 2 Cap/jacket red, 3 Cap band,
// 4 Brim highlight, 5 Sleeves/jeans shade, 6 Jeans, 7 Bag, 8 Bag shade.
// Visible rows are 1 (HAT, with 0 and 2 following), 3 (HIGHLIGHT, with 4
// following), 6 (PANTS, with 5 following) and 7 (BAG, with 8 following).
// Trainer indices are the front pic's (gTrainerPalette_Red); the back pic
// uses OW indices. Slots 0, 3 and 4 have no front indices: the front pic's
// hair navy (8) also outlines skin and jeans, and its white (14) and grey (9)
// are also the eye whites.
static const u8 sColorIdx_Red_Hair_Ow[] = {8};
static const u8 sColorIdx_Red_CapTop_Ow[] = {11};
static const u8 sColorIdx_Red_CapRed_Ow[] = {12};
static const u8 sColorIdx_Red_CapBand_Ow[] = {9};
static const u8 sColorIdx_Red_BrimHighlight_Ow[] = {10};
static const u8 sColorIdx_Red_Sleeves_Ow[] = {6};
static const u8 sColorIdx_Red_Jeans_Ow[] = {7};
static const u8 sColorIdx_Red_Bag_Ow[] = {13};
static const u8 sColorIdx_Red_BagShade_Ow[] = {14};
static const u8 sColorIdx_Red_CapTop_Trainer[] = {12};
static const u8 sColorIdx_Red_CapRed_Trainer[] = {13};
static const u8 sColorIdx_Red_Sleeves_Trainer[] = {7};
static const u8 sColorIdx_Red_Jeans_Trainer[] = {5, 6};
static const u8 sColorIdx_Red_Bag_Trainer[] = {10};
static const u8 sColorIdx_Red_BagShade_Trainer[] = {11};
static const u8 sGroupSlots_Red_Hat[] = {1, 3};
static const u8 sGroupSlots_Red_Outfit[] = {6};
static const u8 sGroupSlots_Red_Accent[] = {7};

// FRLG female (Leaf). Same one-OW-index-per-slot model as Red.
// Slot ids: 0 Hair, 1 Hair shade, 2 Hair highlight, 3 Hat white,
// 4 Hat shade + socks, 5 Hat band + skirt, 6 Hat logo,
// 7 Top shade + hat outline, 8 Top, 9 Bag, 10 Bag shade.
// Visible rows are 2 (HAIR, with 0 and 1 following), 3 (HAT, with 4
// following), 5 (HIGHLIGHT, with 6 following), 8 (SHIRT, with 7 following)
// and 9 (BAG, with 10 following).
// Trainer indices are the front pic's (gTrainerPalette_Leaf, same palette as
// Red's front); the back pic uses OW indices. The front pic's hair browns
// (3, 4, 8) also outline or shade skin, and its white (14) and grey (9) are
// also the eye whites and shoes, so hair and hat white/shade have no front
// indices. Front 6/7 are also her irises, so the top slots recolour them.
static const u8 sColorIdx_Leaf_Hair_Ow[] = {4};
static const u8 sColorIdx_Leaf_HairShade_Ow[] = {8};
static const u8 sColorIdx_Leaf_HairHighlight_Ow[] = {1};
static const u8 sColorIdx_Leaf_HatWhite_Ow[] = {9};
static const u8 sColorIdx_Leaf_HatShade_Ow[] = {10};
static const u8 sColorIdx_Leaf_HatBand_Ow[] = {12};
static const u8 sColorIdx_Leaf_HatLogo_Ow[] = {11};
static const u8 sColorIdx_Leaf_TopShade_Ow[] = {6};
static const u8 sColorIdx_Leaf_Top_Ow[] = {7};
static const u8 sColorIdx_Leaf_Bag_Ow[] = {13};
static const u8 sColorIdx_Leaf_BagShade_Ow[] = {14};
static const u8 sColorIdx_Leaf_HatBand_Trainer[] = {12, 13};
static const u8 sColorIdx_Leaf_TopShade_Trainer[] = {7};
static const u8 sColorIdx_Leaf_Top_Trainer[] = {5, 6};
static const u8 sColorIdx_Leaf_Bag_Trainer[] = {10};
static const u8 sGroupSlots_Leaf_Hair[] = {2};
static const u8 sGroupSlots_Leaf_Hat[] = {3, 5};
static const u8 sGroupSlots_Leaf_Outfit[] = {8};
static const u8 sGroupSlots_Leaf_Accent[] = {9};

static const struct PlayerColorSlotInfo
    sPlayerColorSlots[PLAYER_SPRITE_STYLE_COUNT][GENDER_COUNT][PLAYER_COLOR_SLOT_COUNT] =
{
    [PLAYER_SPRITE_STYLE_EMERALD] = {
        [MALE] = {
            [0] = {COMPOUND_STRING("SHIRT PRIMARY"), sColorIdx_Brendan_Hair, NULL, ARRAY_COUNT(sColorIdx_Brendan_Hair), 0, PLAYER_COLOR_SLOT_NONE},
            [1] = {COMPOUND_STRING("HAT"), sColorIdx_Brendan_BandanaWhite, sColorIdx_Brendan_BandanaWhite, ARRAY_COUNT(sColorIdx_Brendan_BandanaWhite), ARRAY_COUNT(sColorIdx_Brendan_BandanaWhite), PLAYER_COLOR_SLOT_NONE},
            [2] = {COMPOUND_STRING("HAT TRIM"), sColorIdx_Brendan_BandanaGreen, sColorIdx_Brendan_BandanaGreen, ARRAY_COUNT(sColorIdx_Brendan_BandanaGreen), ARRAY_COUNT(sColorIdx_Brendan_BandanaGreen), PLAYER_COLOR_SLOT_NONE},
            [3] = {NULL, sColorIdx_Brendan_Jacket, sColorIdx_Brendan_Jacket, ARRAY_COUNT(sColorIdx_Brendan_Jacket), ARRAY_COUNT(sColorIdx_Brendan_Jacket), PLAYER_COLOR_SLOT_NONE},
            [4] = {NULL, sColorIdx_Brendan_CapOutline, sColorIdx_Brendan_CapOutline, ARRAY_COUNT(sColorIdx_Brendan_CapOutline), ARRAY_COUNT(sColorIdx_Brendan_CapOutline), PLAYER_COLOR_SLOT_NONE},
            [5] = {COMPOUND_STRING("GLOVES & BOOTS"), sColorIdx_Brendan_Bag, sColorIdx_Brendan_Bag, ARRAY_COUNT(sColorIdx_Brendan_Bag), ARRAY_COUNT(sColorIdx_Brendan_Bag), PLAYER_COLOR_SLOT_NONE},
        },
        [FEMALE] = {
            [0] = {COMPOUND_STRING("HAIR"), sColorIdx_May_Hair, sColorIdx_May_Hair, ARRAY_COUNT(sColorIdx_May_Hair), ARRAY_COUNT(sColorIdx_May_Hair), PLAYER_COLOR_SLOT_NONE},
            [1] = {COMPOUND_STRING("HAT & BAG"), sColorIdx_May_Bandana, sColorIdx_May_Bandana, ARRAY_COUNT(sColorIdx_May_Bandana), ARRAY_COUNT(sColorIdx_May_Bandana), PLAYER_COLOR_SLOT_NONE},
            [2] = {NULL, sColorIdx_May_CapOutline, sColorIdx_May_CapOutline, ARRAY_COUNT(sColorIdx_May_CapOutline), ARRAY_COUNT(sColorIdx_May_CapOutline), PLAYER_COLOR_SLOT_NONE},
            [3] = {NULL, sColorIdx_May_Shorts, sColorIdx_May_Shorts, ARRAY_COUNT(sColorIdx_May_Shorts), ARRAY_COUNT(sColorIdx_May_Shorts), PLAYER_COLOR_SLOT_NONE},
            [4] = {COMPOUND_STRING("HAT ACCENT"), sColorIdx_May_WhiteTrim, sColorIdx_May_WhiteTrim, ARRAY_COUNT(sColorIdx_May_WhiteTrim), ARRAY_COUNT(sColorIdx_May_WhiteTrim), PLAYER_COLOR_SLOT_NONE},
            [5] = {COMPOUND_STRING("SHIRT"), sColorIdx_May_Bag, sColorIdx_May_Bag, ARRAY_COUNT(sColorIdx_May_Bag), ARRAY_COUNT(sColorIdx_May_Bag), PLAYER_COLOR_SLOT_NONE},
        },
    },
    [PLAYER_SPRITE_STYLE_FRLG] = {
        [MALE] = {
            [0] = {NULL, sColorIdx_Red_Hair_Ow, NULL, ARRAY_COUNT(sColorIdx_Red_Hair_Ow), 0, 1},
            [1] = {COMPOUND_STRING("HAT"), sColorIdx_Red_CapTop_Ow, sColorIdx_Red_CapTop_Trainer, ARRAY_COUNT(sColorIdx_Red_CapTop_Ow), ARRAY_COUNT(sColorIdx_Red_CapTop_Trainer), PLAYER_COLOR_SLOT_NONE},
            [2] = {NULL, sColorIdx_Red_CapRed_Ow, sColorIdx_Red_CapRed_Trainer, ARRAY_COUNT(sColorIdx_Red_CapRed_Ow), ARRAY_COUNT(sColorIdx_Red_CapRed_Trainer), 1},
            [3] = {COMPOUND_STRING("HIGHLIGHT"), sColorIdx_Red_CapBand_Ow, NULL, ARRAY_COUNT(sColorIdx_Red_CapBand_Ow), 0, PLAYER_COLOR_SLOT_NONE},
            [4] = {NULL, sColorIdx_Red_BrimHighlight_Ow, NULL, ARRAY_COUNT(sColorIdx_Red_BrimHighlight_Ow), 0, 3},
            [5] = {NULL, sColorIdx_Red_Sleeves_Ow, sColorIdx_Red_Sleeves_Trainer, ARRAY_COUNT(sColorIdx_Red_Sleeves_Ow), ARRAY_COUNT(sColorIdx_Red_Sleeves_Trainer), 6},
            [6] = {COMPOUND_STRING("PANTS"), sColorIdx_Red_Jeans_Ow, sColorIdx_Red_Jeans_Trainer, ARRAY_COUNT(sColorIdx_Red_Jeans_Ow), ARRAY_COUNT(sColorIdx_Red_Jeans_Trainer), PLAYER_COLOR_SLOT_NONE},
            [7] = {COMPOUND_STRING("BAG"), sColorIdx_Red_Bag_Ow, sColorIdx_Red_Bag_Trainer, ARRAY_COUNT(sColorIdx_Red_Bag_Ow), ARRAY_COUNT(sColorIdx_Red_Bag_Trainer), PLAYER_COLOR_SLOT_NONE},
            [8] = {NULL, sColorIdx_Red_BagShade_Ow, sColorIdx_Red_BagShade_Trainer, ARRAY_COUNT(sColorIdx_Red_BagShade_Ow), ARRAY_COUNT(sColorIdx_Red_BagShade_Trainer), 7},
        },
        [FEMALE] = {
            [0] = {NULL, sColorIdx_Leaf_Hair_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_Hair_Ow), 0, 2},
            [1] = {NULL, sColorIdx_Leaf_HairShade_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_HairShade_Ow), 0, 2},
            [2] = {COMPOUND_STRING("HAIR"), sColorIdx_Leaf_HairHighlight_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_HairHighlight_Ow), 0, PLAYER_COLOR_SLOT_NONE},
            [3] = {COMPOUND_STRING("HAT"), sColorIdx_Leaf_HatWhite_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_HatWhite_Ow), 0, PLAYER_COLOR_SLOT_NONE},
            [4] = {NULL, sColorIdx_Leaf_HatShade_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_HatShade_Ow), 0, 3},
            [5] = {COMPOUND_STRING("HIGHLIGHT"), sColorIdx_Leaf_HatBand_Ow, sColorIdx_Leaf_HatBand_Trainer, ARRAY_COUNT(sColorIdx_Leaf_HatBand_Ow), ARRAY_COUNT(sColorIdx_Leaf_HatBand_Trainer), PLAYER_COLOR_SLOT_NONE},
            [6] = {NULL, sColorIdx_Leaf_HatLogo_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_HatLogo_Ow), 0, 5},
            [7] = {NULL, sColorIdx_Leaf_TopShade_Ow, sColorIdx_Leaf_TopShade_Trainer, ARRAY_COUNT(sColorIdx_Leaf_TopShade_Ow), ARRAY_COUNT(sColorIdx_Leaf_TopShade_Trainer), 8},
            [8] = {COMPOUND_STRING("SHIRT"), sColorIdx_Leaf_Top_Ow, sColorIdx_Leaf_Top_Trainer, ARRAY_COUNT(sColorIdx_Leaf_Top_Ow), ARRAY_COUNT(sColorIdx_Leaf_Top_Trainer), PLAYER_COLOR_SLOT_NONE},
            [9] = {COMPOUND_STRING("BAG"), sColorIdx_Leaf_Bag_Ow, sColorIdx_Leaf_Bag_Trainer, ARRAY_COUNT(sColorIdx_Leaf_Bag_Ow), ARRAY_COUNT(sColorIdx_Leaf_Bag_Trainer), PLAYER_COLOR_SLOT_NONE},
            [10] = {NULL, sColorIdx_Leaf_BagShade_Ow, NULL, ARRAY_COUNT(sColorIdx_Leaf_BagShade_Ow), 0, 9},
        },
    },
};

static const struct PlayerColorGroupInfo
    sPlayerColorGroups[PLAYER_SPRITE_STYLE_COUNT][GENDER_COUNT][PLAYER_COLOR_REGION_COUNT] =
{
    [PLAYER_SPRITE_STYLE_EMERALD] = {
        [MALE] = {
            [PLAYER_COLOR_REGION_HAIR] = {COMPOUND_STRING("HAIR"), NULL, 0}, // Brendan has no hair row
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
            [PLAYER_COLOR_REGION_HAIR] = {COMPOUND_STRING("HAIR"), NULL, 0}, // Red's hair follows his hat
            [PLAYER_COLOR_REGION_HAT] = {COMPOUND_STRING("HAT"), sGroupSlots_Red_Hat, ARRAY_COUNT(sGroupSlots_Red_Hat)},
            [PLAYER_COLOR_REGION_OUTFIT] = {COMPOUND_STRING("OUTFIT"), sGroupSlots_Red_Outfit, ARRAY_COUNT(sGroupSlots_Red_Outfit)},
            [PLAYER_COLOR_REGION_ACCENT] = {COMPOUND_STRING("ACCENT"), sGroupSlots_Red_Accent, ARRAY_COUNT(sGroupSlots_Red_Accent)},
        },
        [FEMALE] = {
            [PLAYER_COLOR_REGION_HAIR] = {COMPOUND_STRING("HAIR"), sGroupSlots_Leaf_Hair, ARRAY_COUNT(sGroupSlots_Leaf_Hair)},
            [PLAYER_COLOR_REGION_HAT] = {COMPOUND_STRING("HAT"), sGroupSlots_Leaf_Hat, ARRAY_COUNT(sGroupSlots_Leaf_Hat)},
            [PLAYER_COLOR_REGION_OUTFIT] = {COMPOUND_STRING("OUTFIT"), sGroupSlots_Leaf_Outfit, ARRAY_COUNT(sGroupSlots_Leaf_Outfit)},
            [PLAYER_COLOR_REGION_ACCENT] = {COMPOUND_STRING("ACCENT"), sGroupSlots_Leaf_Accent, ARRAY_COUNT(sGroupSlots_Leaf_Accent)},
        },
    },
};

