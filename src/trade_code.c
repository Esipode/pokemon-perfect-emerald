#include "global.h"
#include "trade_code.h"
#include "constants/characters.h"
#include "constants/pokemon.h"
#include "constants/moves.h"
#include "constants/items.h"
#include "constants/region_map_sections.h"
#include "string_util.h"

// Stage 1 of "Trading Codes.md": a pure bit stream + Base32/Crockford codec.
// No UI, no save data, no game state - see trade_code.h for the public
// surface and struct TradeCodeBits' contract.

// Crockford Base32 alphabet, "0123456789ABCDEFGHJKMNPQRSTVWXYZ", stored as
// game-charmap bytes so TradeCode_Encode's output can be handed straight to
// the text engine (Stage 5) with no ASCII conversion step. I, L, O and U are
// dropped: I/L fold to 1 and O folds to 0 on the way back in below (the
// commonest misreadings self-correct), and U is skipped entirely so a code
// can never spell a word.
static const u8 sTradeCodeAlphabet[32] =
{
    CHAR_0, CHAR_1, CHAR_2, CHAR_3, CHAR_4, CHAR_5, CHAR_6, CHAR_7, CHAR_8, CHAR_9,
    CHAR_A, CHAR_B, CHAR_C, CHAR_D, CHAR_E, CHAR_F, CHAR_G, CHAR_H, CHAR_J, CHAR_K,
    CHAR_M, CHAR_N, CHAR_P, CHAR_Q, CHAR_R, CHAR_S, CHAR_T, CHAR_V, CHAR_W, CHAR_X,
    CHAR_Y, CHAR_Z,
};

#define TRADE_CODE_CHAR_SKIP    0xFE // hyphen / space: ignored, not part of the payload
#define TRADE_CODE_CHAR_INVALID 0xFF // not a symbol, a fold, or a skip char

// Reverse lookup, indexed by the raw game-charmap byte the entry screen (or
// a hand-typed debug string) hands to TradeCode_Decode. Folds the common
// misreadings - I/i/l -> 1, O/o -> 0 - and lowercase -> uppercase. CHAR_U /
// CHAR_u are deliberately left TRADE_CODE_CHAR_INVALID: U isn't in the
// alphabet and doesn't stand in for anything else.
//
// The [0 ... 255] catch-all is deliberately then overridden per-symbol below
// - exactly the pattern -Woverride-init exists to flag, but it's what makes
// a 256-entry table buildable without hand-listing ~230 invalid entries.
// Suppressed locally rather than dropping the warning project-wide.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
static const u8 sTradeCodeReverse[256] =
{
    [0 ... 255] = TRADE_CODE_CHAR_INVALID,

    [CHAR_SPACE] = TRADE_CODE_CHAR_SKIP,
    [CHAR_HYPHEN] = TRADE_CODE_CHAR_SKIP,

    [CHAR_0] = 0,  [CHAR_1] = 1,  [CHAR_2] = 2,  [CHAR_3] = 3,  [CHAR_4] = 4,
    [CHAR_5] = 5,  [CHAR_6] = 6,  [CHAR_7] = 7,  [CHAR_8] = 8,  [CHAR_9] = 9,

    [CHAR_A] = 10, [CHAR_B] = 11, [CHAR_C] = 12, [CHAR_D] = 13, [CHAR_E] = 14,
    [CHAR_F] = 15, [CHAR_G] = 16, [CHAR_H] = 17, [CHAR_I] = 1,  [CHAR_J] = 18,
    [CHAR_K] = 19, [CHAR_L] = 1,  [CHAR_M] = 20, [CHAR_N] = 21, [CHAR_O] = 0,
    [CHAR_P] = 22, [CHAR_Q] = 23, [CHAR_R] = 24, [CHAR_S] = 25, [CHAR_T] = 26,
    [CHAR_V] = 27, [CHAR_W] = 28, [CHAR_X] = 29, [CHAR_Y] = 30, [CHAR_Z] = 31,

    [CHAR_a] = 10, [CHAR_b] = 11, [CHAR_c] = 12, [CHAR_d] = 13, [CHAR_e] = 14,
    [CHAR_f] = 15, [CHAR_g] = 16, [CHAR_h] = 17, [CHAR_i] = 1,  [CHAR_j] = 18,
    [CHAR_k] = 19, [CHAR_l] = 1,  [CHAR_m] = 20, [CHAR_n] = 21, [CHAR_o] = 0,
    [CHAR_p] = 22, [CHAR_q] = 23, [CHAR_r] = 24, [CHAR_s] = 25, [CHAR_t] = 26,
    [CHAR_v] = 27, [CHAR_w] = 28, [CHAR_x] = 29, [CHAR_y] = 30, [CHAR_z] = 31,
};
#pragma GCC diagnostic pop

// Defensive cap on how many raw characters TradeCode_Decode will scan
// looking for EOS. This is deliberately independent of TRADE_CODE_MAX_CHARS
// (the entry screen's display/entry limit, Stage 5/6's concern) - its only
// job is to stop a non-terminated buffer from being read past forever;
// TRADE_CODE_TOO_LONG for a legitimately-too-long *code* is already caught
// below by running out of `out->capacity` bits. Generous enough to cover
// every bit length Stage 1's own tests exercise (up to 400 bits -> 80
// symbols -> 95 characters with hyphens) plus headroom.
#define TRADE_CODE_DECODE_SCAN_LIMIT 512

u8 TradeCode_AlphabetSymbol(u32 index)
{
    return sTradeCodeAlphabet[index];
}

void TradeCode_WriteBits(struct TradeCodeBits *stream, u32 value, u32 nBits)
{
    u32 i;

    if (stream->error || nBits == 0)
        return;

    if (stream->bitPos + nBits > stream->capacity)
    {
        stream->error = TRUE;
        return;
    }

    for (i = 0; i < nBits; i++)
    {
        u32 bit = (value >> (nBits - 1 - i)) & 1;
        u32 bytePos = stream->bitPos >> 3;
        u32 bitInByte = 7 - (stream->bitPos & 7);

        if (bit)
            stream->data[bytePos] |= (1 << bitInByte);
        else
            stream->data[bytePos] &= ~(1 << bitInByte);

        stream->bitPos++;
    }
}

u32 TradeCode_ReadBits(struct TradeCodeBits *stream, u32 nBits)
{
    u32 i;
    u32 value = 0;

    if (nBits == 0)
        return 0;

    if (stream->error || stream->bitPos + nBits > stream->capacity)
    {
        stream->error = TRUE;
        return 0;
    }

    for (i = 0; i < nBits; i++)
    {
        u32 bytePos = stream->bitPos >> 3;
        u32 bitInByte = 7 - (stream->bitPos & 7);
        u32 bit = (stream->data[bytePos] >> bitInByte) & 1;

        value = (value << 1) | bit;
        stream->bitPos++;
    }

    return value;
}

void TradeCode_Encode(const u8 *bits, u32 nBits, u8 *outStr)
{
    u32 nSymbols = (nBits + 4) / 5; // round up to a whole symbol
    u32 bitPos = 0;
    u32 outPos = 0;
    u32 symbolIndex;

    for (symbolIndex = 0; symbolIndex < nSymbols; symbolIndex++)
    {
        u32 value = 0;
        u32 i;

        for (i = 0; i < 5; i++)
        {
            u32 bit = 0;

            if (bitPos < nBits)
            {
                u32 bytePos = bitPos >> 3;
                u32 bitInByte = 7 - (bitPos & 7);
                bit = (bits[bytePos] >> bitInByte) & 1;
            }
            value = (value << 1) | bit;
            bitPos++;
        }

        if (symbolIndex != 0 && symbolIndex % TRADE_CODE_GROUP_SIZE == 0)
            outStr[outPos++] = CHAR_HYPHEN;

        outStr[outPos++] = sTradeCodeAlphabet[value];
    }

    outStr[outPos] = EOS;
}

enum TradeCodeStatus TradeCode_Decode(const u8 *str, struct TradeCodeBits *out)
{
    u32 outCapacityBits = out->capacity; // caller-supplied buffer size, in bits
    u32 bitPos = 0;
    u32 nSymbols = 0;
    u32 rawPos = 0;
    u32 i;

    memset(out->data, 0, (outCapacityBits + 7) / 8);
    out->bitPos = 0;
    out->error = FALSE;

    while (str[rawPos] != EOS)
    {
        u32 value;

        if (rawPos >= TRADE_CODE_DECODE_SCAN_LIMIT)
            return TRADE_CODE_TOO_LONG;

        value = sTradeCodeReverse[str[rawPos]];
        rawPos++;

        if (value == TRADE_CODE_CHAR_SKIP)
            continue;
        if (value == TRADE_CODE_CHAR_INVALID)
            return TRADE_CODE_BAD_CHAR;

        if (bitPos + 5 > outCapacityBits)
            return TRADE_CODE_TOO_LONG;

        for (i = 0; i < 5; i++)
        {
            u32 bit = (value >> (4 - i)) & 1;
            u32 bytePos = bitPos >> 3;
            u32 bitInByte = 7 - (bitPos & 7);

            if (bit)
                out->data[bytePos] |= (1 << bitInByte);
            bitPos++;
        }
        nSymbols++;
    }

    if (nSymbols == 0)
        return TRADE_CODE_TOO_SHORT;

    out->capacity = bitPos; // narrow to the actual decoded length
    return TRADE_CODE_OK;
}

// ---------------------------------------------------------------------
// Stage 2 of "Trading Codes.md": the BoxPokemon serialiser/deserialiser.
// Still no UI, no save data - see trade_code.h for the payload's status
// enum. MON_DATA_HP_IV.. and MON_DATA_HP_EV.. are contiguous in HP, ATK,
// DEF, SPEED, SPATK, SPDEF order in enum MonData, so every 6-wide loop
// below walks that order via `+ i`.
// ---------------------------------------------------------------------

// Presence bitmap bits (see the payload spec's "Optional block" table).
// Bit 5 (met data) was dropped per the plan doc's own dev note: met data
// is a poor value-per-bit spend, and a fixed "obtained via trade" location
// is applied unconditionally below instead of transmitting a real one. Bit
// 7 stays reserved for a future format version. Both must decode as 0.
#define TRADE_CODE_PRESENCE_NICKNAME    (1 << 0)
#define TRADE_CODE_PRESENCE_EVS         (1 << 1)
#define TRADE_CODE_PRESENCE_MOVES       (1 << 2)
#define TRADE_CODE_PRESENCE_HELD_ITEM   (1 << 3)
#define TRADE_CODE_PRESENCE_FRIENDSHIP  (1 << 4)
#define TRADE_CODE_PRESENCE_POKERUS     (1 << 6)
#define TRADE_CODE_PRESENCE_RESERVED    ((1 << 5) | (1 << 7))

// The payload's "gender" field is a compact 2-bit enum of its own, not a
// MON_MALE(0x00)/MON_FEMALE(0xFE)/MON_GENDERLESS(0xFF) byte - those don't
// fit in 2 bits. Converted at the edges via TradeCode_GenderToField /
// TradeCode_FieldToGender.
#define TRADE_CODE_GENDER_MALE       0
#define TRADE_CODE_GENDER_FEMALE     1
#define TRADE_CODE_GENDER_GENDERLESS 2
// value 3 is reserved and rejected on decode.

static u8 TradeCode_GenderToField(u8 gender)
{
    switch (gender)
    {
    case MON_FEMALE:
        return TRADE_CODE_GENDER_FEMALE;
    case MON_GENDERLESS:
        return TRADE_CODE_GENDER_GENDERLESS;
    default:
        return TRADE_CODE_GENDER_MALE;
    }
}

static u8 TradeCode_FieldToGender(u32 field)
{
    switch (field)
    {
    case TRADE_CODE_GENDER_FEMALE:
        return MON_FEMALE;
    case TRADE_CODE_GENDER_GENDERLESS:
        return MON_GENDERLESS;
    default:
        return MON_MALE;
    }
}

// Whether `actualMoves` matches the moveset GiveBoxMonInitialMoveset would
// generate for `species` at `level` - the payload spec's definition of the
// move presence bit's default. Built via a scratch BoxPokemon rather than
// re-deriving the level-filtering/dedup logic by hand, so this can never
// drift from what the deserialiser's own "moves absent" path applies.
static bool32 TradeCode_MovesMatchDefault(enum Species species, u32 level, const enum Move actualMoves[MAX_MON_MOVES])
{
    struct BoxPokemon scratch;
    u32 i;

    CreateBoxMon(&scratch, species, level, 0, OTID_STRUCT_PRESET(0));
    GiveBoxMonInitialMoveset(&scratch);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetBoxMonData(&scratch, MON_DATA_MOVE1 + i) != actualMoves[i])
            return FALSE;
    }
    return TRUE;
}

// True if any of the first `len` bytes of `chars` is EOS. A real name
// character can never legitimately be EOS (it's the terminator, not part
// of any fork's charmap alphabet), so one appearing inside the declared
// length means the code is corrupt - decoding it further would desync the
// fixed-width nickname/otName arrays downstream.
static bool32 TradeCode_NameHasEosByte(const u8 *chars, u32 len)
{
    u32 i;

    for (i = 0; i < len; i++)
    {
        if (chars[i] == EOS)
            return TRUE;
    }
    return FALSE;
}

// Deterministic stand-in for the OT ID's high 16 bits, which the payload
// never transmits (see "otId high half" in the plan doc). Not cryptographic
// and doesn't need to be - its only job is to almost certainly differ from
// the receiving player's own OT ID, which the caller double-checks and
// perturbs on the rare exact collision.
static u32 TradeCode_SynthesizeOtIdHigh(const u8 *otName, u32 otNameLen, u16 otIdLow)
{
    u32 hash = otIdLow;
    u32 i;

    for (i = 0; i < otNameLen; i++)
        hash = hash * 33 + otName[i];

    return hash & 0xFFFF;
}

void TradeCode_SerializeMon(const struct BoxPokemon *boxMon, struct TradeCodeBits *stream)
{
    // GetBoxMonData decrypts/re-encrypts its argument in place (restoring
    // it before returning) - working on a local copy keeps the caller's
    // struct untouched for the duration, matching how BoxMonToMon (Saveblock
    // Shrinking-era code) already handles a const source.
    struct BoxPokemon mon = *boxMon;
    enum Species species = GetBoxMonData(&mon, MON_DATA_SPECIES);
    u32 level = GetLevelFromBoxMonExp(&mon);
    u8 presence = 0;
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u32 nicknameLen, otNameLen;
    u32 evs[6];
    bool32 hasEVs = FALSE;
    enum Move moves[MAX_MON_MOVES];
    u32 ppBonuses = GetBoxMonData(&mon, MON_DATA_PP_BONUSES);
    bool32 hasMoves;
    u32 heldItem = GetBoxMonData(&mon, MON_DATA_HELD_ITEM);
    u32 friendship = GetBoxMonData(&mon, MON_DATA_FRIENDSHIP);
    u32 pokerus = GetBoxMonData(&mon, MON_DATA_POKERUS);
    u32 i;

    GetBoxMonData(&mon, MON_DATA_NICKNAME, nickname);
    nicknameLen = StringLength(nickname);
    if (StringCompare(nickname, GetSpeciesName(species)) != 0)
        presence |= TRADE_CODE_PRESENCE_NICKNAME;

    for (i = 0; i < 6; i++)
    {
        evs[i] = GetBoxMonData(&mon, MON_DATA_HP_EV + i);
        if (evs[i] != 0)
            hasEVs = TRUE;
    }
    if (hasEVs)
        presence |= TRADE_CODE_PRESENCE_EVS;

    for (i = 0; i < MAX_MON_MOVES; i++)
        moves[i] = GetBoxMonData(&mon, MON_DATA_MOVE1 + i);
    // PP Ups are carried in the same 52-bit block as the moves themselves
    // (see the payload spec), so a mon that knows only its default moveset
    // but has spent PP Ups on it still needs the block written - otherwise
    // those PP Ups would silently vanish across the trade.
    hasMoves = (ppBonuses != 0) || !TradeCode_MovesMatchDefault(species, level, moves);
    if (hasMoves)
        presence |= TRADE_CODE_PRESENCE_MOVES;

    if (heldItem != ITEM_NONE)
        presence |= TRADE_CODE_PRESENCE_HELD_ITEM;
    if (friendship != gSpeciesInfo[species].friendship)
        presence |= TRADE_CODE_PRESENCE_FRIENDSHIP;
    if (pokerus != 0)
        presence |= TRADE_CODE_PRESENCE_POKERUS;

    TradeCode_WriteBits(stream, presence, 8);

    // -- core (always present) --
    TradeCode_WriteBits(stream, species, 11);
    TradeCode_WriteBits(stream, level, 10);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_HIDDEN_NATURE), 5);
    TradeCode_WriteBits(stream, TradeCode_GenderToField(GetGenderFromSpeciesAndPersonality(species, GetBoxMonData(&mon, MON_DATA_PERSONALITY))), 2);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_IS_SHINY) ? 1 : 0, 1);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_ABILITY_NUM), 2);
    for (i = 0; i < 6; i++)
        TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_HP_IV + i), 5);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_POKEBALL), 6);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_TERA_TYPE), 5);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_OT_ID) & 0xFFFF, 16);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_OT_GENDER), 1);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_LANGUAGE), 3);
    TradeCode_WriteBits(stream, GetBoxMonData(&mon, MON_DATA_IS_EGG) ? 1 : 0, 1);

    // -- OT name (always present) --
    // Deviation from the plan doc: it specs 7 bits/character ("straight
    // from the game's charmap"), but this fork's charmap puts CHAR_0 at
    // 0xA1 and runs past 0xEE for lowercase letters - every real name
    // character is outside 7 bits' 0-127 range, so a literal 7-bit pack
    // would silently truncate every letter and digit. Using the full raw
    // byte (8 bits/character) here and for the nickname below instead;
    // flagged in the Stage 2 status block since it changes the spec's
    // published code-length table.
    GetBoxMonData(&mon, MON_DATA_OT_NAME, otName);
    otNameLen = StringLength(otName);
    TradeCode_WriteBits(stream, otNameLen, 3);
    for (i = 0; i < otNameLen; i++)
        TradeCode_WriteBits(stream, otName[i], 8);

    // -- optional block, gated by `presence` --
    if (presence & TRADE_CODE_PRESENCE_NICKNAME)
    {
        TradeCode_WriteBits(stream, nicknameLen, 4);
        for (i = 0; i < nicknameLen; i++)
            TradeCode_WriteBits(stream, nickname[i], 8);
    }
    if (presence & TRADE_CODE_PRESENCE_EVS)
    {
        for (i = 0; i < 6; i++)
            TradeCode_WriteBits(stream, evs[i] / 4, 6); // stat calc floors EV/4 - not lossy
    }
    if (presence & TRADE_CODE_PRESENCE_MOVES)
    {
        for (i = 0; i < MAX_MON_MOVES; i++)
            TradeCode_WriteBits(stream, moves[i], 11);
        TradeCode_WriteBits(stream, ppBonuses, 8);
    }
    if (presence & TRADE_CODE_PRESENCE_HELD_ITEM)
        TradeCode_WriteBits(stream, heldItem, 10);
    if (presence & TRADE_CODE_PRESENCE_FRIENDSHIP)
        TradeCode_WriteBits(stream, friendship, 8);
    if (presence & TRADE_CODE_PRESENCE_POKERUS)
        TradeCode_WriteBits(stream, pokerus, 8);
}

enum TradeCodeMonStatus TradeCode_DeserializeMon(struct TradeCodeBits *stream, struct BoxPokemon *outBoxMon)
{
    u8 presence;
    u32 species, level, nature, genderField, shiny, abilityNum;
    u32 ivs[6];
    u32 pokeball, teraType, otIdLow, otGender, language, isEgg;
    u32 otNameLen;
    u8 otName[PLAYER_NAME_LENGTH + 1];
    bool32 hasNickname;
    u32 nicknameLen = 0;
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    bool32 hasEVs;
    u32 evs[6] = {0};
    bool32 hasMoves;
    enum Move moves[MAX_MON_MOVES] = {MOVE_NONE};
    u32 ppBonuses = 0;
    bool32 hasHeldItem;
    u32 heldItem = ITEM_NONE;
    bool32 hasFriendship;
    u32 friendship = 0;
    bool32 hasPokerus;
    u32 pokerus = 0;
    u32 i;

    presence = TradeCode_ReadBits(stream, 8);
    if (presence & TRADE_CODE_PRESENCE_RESERVED)
        return TRADE_CODE_MON_RESERVED_BITS_SET;

    species = TradeCode_ReadBits(stream, 11);
    level = TradeCode_ReadBits(stream, 10);
    nature = TradeCode_ReadBits(stream, 5);
    genderField = TradeCode_ReadBits(stream, 2);
    shiny = TradeCode_ReadBits(stream, 1);
    abilityNum = TradeCode_ReadBits(stream, 2);
    for (i = 0; i < 6; i++)
        ivs[i] = TradeCode_ReadBits(stream, 5);
    pokeball = TradeCode_ReadBits(stream, 6);
    teraType = TradeCode_ReadBits(stream, 5);
    otIdLow = TradeCode_ReadBits(stream, 16);
    otGender = TradeCode_ReadBits(stream, 1);
    language = TradeCode_ReadBits(stream, 3);
    isEgg = TradeCode_ReadBits(stream, 1);

    otNameLen = TradeCode_ReadBits(stream, 3); // 3 bits can't exceed PLAYER_NAME_LENGTH (7)
    for (i = 0; i < otNameLen; i++)
        otName[i] = TradeCode_ReadBits(stream, 8);
    // SetBoxMonData(MON_DATA_OT_NAME) below copies all PLAYER_NAME_LENGTH
    // bytes unconditionally, so every slot past the terminator needs a
    // defined value too, not just otName[otNameLen] - otherwise this reads
    // as uninitialised stack memory into save data.
    for (i = otNameLen; i < PLAYER_NAME_LENGTH; i++)
        otName[i] = EOS;
    otName[PLAYER_NAME_LENGTH] = EOS;

    hasNickname = (presence & TRADE_CODE_PRESENCE_NICKNAME) != 0;
    if (hasNickname)
    {
        nicknameLen = TradeCode_ReadBits(stream, 4);
        // Unlike otNameLen, this 4-bit field's max (15) DOES exceed
        // POKEMON_NAME_LENGTH (12) - must bounds-check before using it to
        // index `nickname`, or a corrupt/hostile code overflows the array.
        if (nicknameLen > POKEMON_NAME_LENGTH)
            return TRADE_CODE_MON_BAD_NAME;
        for (i = 0; i < nicknameLen; i++)
            nickname[i] = TradeCode_ReadBits(stream, 8);
        // Same reasoning as otName above: SetBoxMonData(MON_DATA_NICKNAME)
        // reads all POKEMON_NAME_LENGTH bytes unconditionally.
        for (i = nicknameLen; i <= POKEMON_NAME_LENGTH; i++)
            nickname[i] = EOS;
    }

    hasEVs = (presence & TRADE_CODE_PRESENCE_EVS) != 0;
    if (hasEVs)
    {
        for (i = 0; i < 6; i++)
            evs[i] = TradeCode_ReadBits(stream, 6) * 4;
    }

    hasMoves = (presence & TRADE_CODE_PRESENCE_MOVES) != 0;
    if (hasMoves)
    {
        for (i = 0; i < MAX_MON_MOVES; i++)
            moves[i] = TradeCode_ReadBits(stream, 11);
        ppBonuses = TradeCode_ReadBits(stream, 8);
    }

    hasHeldItem = (presence & TRADE_CODE_PRESENCE_HELD_ITEM) != 0;
    if (hasHeldItem)
        heldItem = TradeCode_ReadBits(stream, 10);

    hasFriendship = (presence & TRADE_CODE_PRESENCE_FRIENDSHIP) != 0;
    if (hasFriendship)
        friendship = TradeCode_ReadBits(stream, 8);

    hasPokerus = (presence & TRADE_CODE_PRESENCE_POKERUS) != 0;
    if (hasPokerus)
        pokerus = TradeCode_ReadBits(stream, 8);

    // A truncated stream latches TradeCode_ReadBits' error flag and hands
    // back zeros for every field past the cut - checked once here, rather
    // than after every single read, but before any of those zeros can leak
    // into a validation decision below.
    if (stream->error)
        return TRADE_CODE_MON_TRUNCATED;

    // -- validation, before anything is materialised --
    if (species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return TRADE_CODE_MON_BAD_SPECIES;
    if (level == 0 || level > MAX_LEVEL)
        return TRADE_CODE_MON_BAD_LEVEL;
    if (nature >= NUM_NATURES)
        return TRADE_CODE_MON_BAD_NATURE;
    if (genderField > TRADE_CODE_GENDER_GENDERLESS)
        return TRADE_CODE_MON_BAD_GENDER;
    // IVs are already bounded to 0-31 by their 5-bit field width.
    if (hasEVs)
    {
        u32 evTotal = 0;
        for (i = 0; i < 6; i++)
            evTotal += evs[i];
        if (evTotal > MAX_TOTAL_EVS)
            return TRADE_CODE_MON_BAD_EV_TOTAL;
    }
    if (hasMoves)
    {
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (moves[i] != MOVE_NONE && moves[i] >= MOVES_COUNT)
                return TRADE_CODE_MON_BAD_MOVE;
        }
    }
    if (hasHeldItem && heldItem != ITEM_NONE && heldItem >= ITEMS_COUNT)
        return TRADE_CODE_MON_BAD_ITEM;
    if (isEgg && hasNickname)
        return TRADE_CODE_MON_EGG_WITH_NICKNAME;
    if (TradeCode_NameHasEosByte(otName, otNameLen)
        || (hasNickname && TradeCode_NameHasEosByte(nickname, nicknameLen)))
        return TRADE_CODE_MON_BAD_NAME;

    // -- materialise --
    {
        u8 gender = TradeCode_FieldToGender(genderField);
        u32 personality = GetMonPersonality(species, gender, NATURE_RANDOM, RANDOM_UNOWN_LETTER);
        u32 otIdHigh = TradeCode_SynthesizeOtIdHigh(otName, otNameLen, otIdLow);
        u32 otId = (otIdHigh << 16) | otIdLow;
        bool32 isShinyFlag = shiny;
        bool32 isEggFlag = isEgg;
        u32 metLocation = METLOC_IN_GAME_TRADE;

        // otId only has to differ from the receiver's own - a 16-bit-low-half
        // match is 1-in-65536, so this is a defensive perturb, not a hot path.
        if (otId == READ_OTID_FROM_SAVE)
        {
            otIdHigh ^= 1;
            otId = (otIdHigh << 16) | otIdLow;
        }

        // CreateBoxMon sets species/personality/otId, encrypts, then guesses
        // shininess/nature/ability/tera/pokeball/met-data from them. Every
        // guess it makes is overwritten below with the transmitted value -
        // the shiny/nature overwrites must come after personality/otId are
        // set (they're read back through the personality-XOR-modifier
        // trick), so nothing here can move above this call.
        CreateBoxMon(outBoxMon, species, level, personality, OTID_STRUCT_PRESET(otId));

        SetBoxMonData(outBoxMon, MON_DATA_IS_SHINY, &isShinyFlag);
        SetBoxMonData(outBoxMon, MON_DATA_HIDDEN_NATURE, &nature);
        SetBoxMonData(outBoxMon, MON_DATA_ABILITY_NUM, &abilityNum);
        for (i = 0; i < 6; i++)
            SetBoxMonData(outBoxMon, MON_DATA_HP_IV + i, &ivs[i]);
        SetBoxMonData(outBoxMon, MON_DATA_POKEBALL, &pokeball);
        SetBoxMonData(outBoxMon, MON_DATA_TERA_TYPE, &teraType);
        SetBoxMonData(outBoxMon, MON_DATA_OT_GENDER, &otGender);
        SetBoxMonData(outBoxMon, MON_DATA_LANGUAGE, &language);
        SetBoxMonData(outBoxMon, MON_DATA_OT_NAME, otName);
        SetBoxMonData(outBoxMon, MON_DATA_IS_EGG, &isEggFlag);

        if (hasNickname)
            SetBoxMonData(outBoxMon, MON_DATA_NICKNAME, nickname);
        // else: CreateBoxMon already set the nickname to the species name -
        // exactly the spec's default.

        if (hasEVs)
        {
            for (i = 0; i < 6; i++)
                SetBoxMonData(outBoxMon, MON_DATA_HP_EV + i, &evs[i]);
        }
        // else: ZeroBoxMonData (via CreateBoxMon) already left EVs at zero.

        if (hasMoves)
        {
            for (i = 0; i < MAX_MON_MOVES; i++)
            {
                u32 move = moves[i];
                u32 pp = CalculatePPWithBonus(moves[i], ppBonuses, i);
                SetBoxMonData(outBoxMon, MON_DATA_MOVE1 + i, &move);
                SetBoxMonData(outBoxMon, MON_DATA_PP1 + i, &pp);
            }
            SetBoxMonData(outBoxMon, MON_DATA_PP_BONUSES, &ppBonuses);
        }
        else
        {
            GiveBoxMonInitialMoveset(outBoxMon); // the spec's default: level-up moveset at `level`
        }

        if (hasHeldItem)
            SetBoxMonData(outBoxMon, MON_DATA_HELD_ITEM, &heldItem);
        // else: CreateBoxMon already left it at ITEM_NONE.

        if (hasFriendship)
            SetBoxMonData(outBoxMon, MON_DATA_FRIENDSHIP, &friendship);
        // else: CreateBoxMon already set the species' base friendship.

        if (hasPokerus)
            SetBoxMonData(outBoxMon, MON_DATA_POKERUS, &pokerus);

        // Met data was dropped from the payload (see the presence bitmap
        // comment above) - CreateBoxMon's met level/game defaults (this
        // level, this game version) stand as-is; only the location is
        // overridden, to the same sentinel the existing in-game-trade path
        // already uses for "no real overworld location" (src/trade.c).
        SetBoxMonData(outBoxMon, MON_DATA_MET_LOCATION, &metLocation);
    }

    return TRADE_CODE_MON_OK;
}

// ---------------------------------------------------------------------
// Stage 3 of "Trading Codes.md": sealing, nonces, replay protection.
// Still no UI, no save data - the replay ring's storage is Stage 4's
// struct PendingTrade; TradeCode_IsOfferSealUsed/RecordOfferSeal below are
// the shared check/insert primitives operating on a caller-owned array.
// ---------------------------------------------------------------------

// Salts passed to TradeCode_Hash, purely to keep an offer seal, "I'm the
// canonically-first offer" confirm tag and "I'm second" confirm tag from
// ever landing on the same value for coincidentally-identical input bytes.
// Not secret in themselves - TRADE_CODE_SECRET is what does the keying.
#define TRADE_CODE_HASH_SALT_OFFER          0x00
#define TRADE_CODE_HASH_SALT_CONFIRM_FIRST  0x01
#define TRADE_CODE_HASH_SALT_CONFIRM_SECOND 0x02

// Two full offer codes (header + mon payload + seal) concatenated. Sized
// with the same "generous headroom over the largest real payload" approach
// as test/trade_code.c's MON_TEST_BUF_BYTES, doubled for two payloads plus
// each one's header/seal. If a future format version ever needs more, the
// excess is silently truncated (TradeCode_WriteBits' existing capacity
// latching, see Stage 1) rather than overflowed - both carts truncate
// identically, so the tag would just cover less of the input, not desync.
#define TRADE_CODE_CONFIRM_COMBINE_BYTES 128

u32 TradeCode_Hash(const u8 *data, u32 len, u32 salt)
{
    u32 hash = 2166136261u ^ (TRADE_CODE_SECRET ^ salt); // FNV-1a offset basis, keyed
    u32 i;

    for (i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= 16777619u; // FNV-1a prime
    }

    // murmur3 fmix32: two xor-shift + multiply rounds, then a final
    // xor-shift - the "avalanche mix (xor-shift-multiply, twice)" from the
    // plan doc. FNV-1a alone leaves a short input's high bits weakly mixed;
    // this spreads a single flipped input bit across roughly half the
    // output bits.
    hash ^= hash >> 16;
    hash *= 0x85EBCA6Bu;
    hash ^= hash >> 13;
    hash *= 0xC2B2AE35u;
    hash ^= hash >> 16;

    return hash;
}

u32 TradeCode_SealOffer(const u8 *payload, u32 nBits)
{
    u32 nBytes = (nBits + 7) / 8;
    return TradeCode_Hash(payload, nBytes, TRADE_CODE_HASH_SALT_OFFER);
}

// Copies `nBits` bits from an external byte buffer into `dest`, MSB-first,
// via TradeCode_WriteBits - keeps all bit-packing logic in Stage 1's own
// functions rather than duplicating it here. Chunked at 24 bits so the
// per-chunk value can never approach a u32 shift overflow.
static void TradeCode_AppendBits(struct TradeCodeBits *dest, const u8 *src, u32 nBits)
{
    u32 pos = 0;

    while (pos < nBits && !dest->error)
    {
        u32 remaining = nBits - pos;
        u32 chunk = (remaining < 24) ? remaining : 24;
        u32 value = 0;
        u32 i;

        for (i = 0; i < chunk; i++)
        {
            u32 bitIndex = pos + i;
            u32 byteIdx = bitIndex >> 3;
            u32 bitInByte = 7 - (bitIndex & 7);
            u32 bit = (src[byteIdx] >> bitInByte) & 1;
            value = (value << 1) | bit;
        }

        TradeCode_WriteBits(dest, value, chunk);
        pos += chunk;
    }
}

u32 TradeCode_ConfirmTag(const u8 *offerSelf, u32 lenSelfBits, u32 otIdSelf, u16 nonceSelf,
                          const u8 *offerPartner, u32 lenPartnerBits, u32 otIdPartner, u16 noncePartner)
{
    u8 buf[TRADE_CODE_CONFIRM_COMBINE_BYTES];
    struct TradeCodeBits combined;
    bool32 selfIsFirst = (otIdSelf != otIdPartner) ? (otIdSelf < otIdPartner) : (nonceSelf < noncePartner);
    u32 salt = selfIsFirst ? TRADE_CODE_HASH_SALT_CONFIRM_FIRST : TRADE_CODE_HASH_SALT_CONFIRM_SECOND;
    u32 hash;

    memset(buf, 0, sizeof(buf));
    combined.data = buf;
    combined.bitPos = 0;
    combined.capacity = sizeof(buf) * 8;
    combined.error = FALSE;

    // Canonical order: lower otId (tie-broken by lower nonce) always goes
    // first, regardless of which mon was passed in as "self" - this is what
    // lets both carts agree on the same concatenated bytes.
    if (selfIsFirst)
    {
        TradeCode_AppendBits(&combined, offerSelf, lenSelfBits);
        TradeCode_AppendBits(&combined, offerPartner, lenPartnerBits);
    }
    else
    {
        TradeCode_AppendBits(&combined, offerPartner, lenPartnerBits);
        TradeCode_AppendBits(&combined, offerSelf, lenSelfBits);
    }

    hash = TradeCode_Hash(buf, (combined.bitPos + 7) / 8, salt);
    return hash & 0x0FFFFFFF; // 28 bits, per the payload spec's confirm code
}

bool32 TradeCode_IsOfferSealUsed(const u32 ring[TRADE_CODE_REPLAY_RING], u32 seal)
{
    u32 i;

    for (i = 0; i < TRADE_CODE_REPLAY_RING; i++)
    {
        if (ring[i] == seal)
            return TRUE;
    }
    return FALSE;
}

void TradeCode_RecordOfferSeal(u32 ring[TRADE_CODE_REPLAY_RING], u32 seal)
{
    u32 i;

    for (i = TRADE_CODE_REPLAY_RING - 1; i > 0; i--)
        ring[i] = ring[i - 1];
    ring[0] = seal;
}

// ---------------------------------------------------------------------
// Stage 9 of "Trading Codes.md": reset-resistance. See this function's own
// declaration in trade_code.h for exactly what it checks and why.
// ---------------------------------------------------------------------

bool32 TradeCode_ValidatePendingBoxMon(const struct BoxPokemon *boxMon)
{
    struct BoxPokemon copy;
    u32 species;

    // GetBoxMonData(MON_DATA_SANITY_IS_BAD_EGG) lazily writes back into its
    // argument's own isBadEgg field the first time it notices a checksum
    // mismatch (see IsBadEgg, src/pokemon.c) - working on a local copy
    // keeps `boxMon` itself a pure read as far as this function's own
    // caller is concerned.
    memcpy(&copy, boxMon, sizeof(copy));

    if (GetBoxMonData(&copy, MON_DATA_SANITY_IS_BAD_EGG))
        return FALSE;

    species = GetBoxMonData(&copy, MON_DATA_SPECIES);
    if (species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return FALSE;

    return TRUE;
}
