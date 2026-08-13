#include "global.h"
#include "test/test.h"
#include "trade_code.h"
#include "constants/characters.h"
#include "constants/moves.h"
#include "constants/items.h"
#include "pokemon.h"
#include "string_util.h"
#include "random.h"

// Stage 1 of "Trading Codes.md": the bit stream + Base32/Crockford codec.
// Pure functions, no save data, no UI.

#define TEST_BUF_BYTES 64 // 512 bits, well over the 400-bit stress case below
#define TEST_STR_CHARS 150 // 80 symbols + 15 hyphens + EOS, plus headroom

// Compares the first `nBits` bits of two buffers (MSB-first), ignoring
// whatever garbage sits below the low end of a partial final byte.
static bool32 BitsEqual(const u8 *a, const u8 *b, u32 nBits)
{
    u32 fullBytes = nBits / 8;
    u32 remBits = nBits % 8;

    if (fullBytes != 0 && memcmp(a, b, fullBytes) != 0)
        return FALSE;

    if (remBits != 0)
    {
        u8 mask = 0xFF << (8 - remBits);
        if ((a[fullBytes] & mask) != (b[fullBytes] & mask))
            return FALSE;
    }

    return TRUE;
}

TEST("TradeCode_Encode/Decode round-trip random bit patterns")
{
    u32 nBits;
    u32 i;

    // A byte-aligned length (8), a header-sized length (30), the three
    // spec'd payload sizes (221/277/387), and a stress length past the
    // largest spec'd payload (400) - plus a couple of odd, non-multiple-of-5
    // and non-multiple-of-8 lengths to catch off-by-one bit packing bugs.
    PARAMETRIZE { nBits = 1; }
    PARAMETRIZE { nBits = 5; }
    PARAMETRIZE { nBits = 8; }
    PARAMETRIZE { nBits = 9; }
    PARAMETRIZE { nBits = 30; }
    PARAMETRIZE { nBits = 99; }
    PARAMETRIZE { nBits = 221; }
    PARAMETRIZE { nBits = 277; }
    PARAMETRIZE { nBits = 387; }
    PARAMETRIZE { nBits = 400; }

    for (i = 0; i < 8; i++)
    {
        u8 src[TEST_BUF_BYTES];
        u8 decoded[TEST_BUF_BYTES];
        u8 str[TEST_STR_CHARS];
        u32 srcBytes = (nBits + 7) / 8;
        u32 j;
        struct TradeCodeBits out;
        enum TradeCodeStatus status;

        for (j = 0; j < srcBytes; j++)
            src[j] = Random() & 0xFF;

        TradeCode_Encode(src, nBits, str);

        memset(decoded, 0, sizeof(decoded));
        out.data = decoded;
        out.capacity = TEST_BUF_BYTES * 8;
        status = TradeCode_Decode(str, &out);

        EXPECT_EQ(status, TRADE_CODE_OK);
        EXPECT_EQ(out.capacity, ((nBits + 4) / 5) * 5); // rounded up to a whole symbol
        EXPECT_EQ(out.bitPos, 0);
        EXPECT(BitsEqual(src, decoded, nBits));
    }
}

TEST("TradeCode_WriteBits/ReadBits round-trip a sequence of fields")
{
    u8 buf[TEST_BUF_BYTES];
    struct TradeCodeBits stream;
    // Mimics the header + a few core fields from the payload spec: assorted
    // widths, none aligned to each other.
    const u32 widths[] = { 4, 2, 16, 8, 11, 10, 5, 2, 1, 2 };
    u32 values[ARRAY_COUNT(widths)];
    u32 i;

    memset(buf, 0, sizeof(buf));
    stream.data = buf;
    stream.bitPos = 0;
    stream.capacity = sizeof(buf) * 8;
    stream.error = FALSE;

    for (i = 0; i < ARRAY_COUNT(widths); i++)
    {
        values[i] = Random() & ((1 << widths[i]) - 1);
        TradeCode_WriteBits(&stream, values[i], widths[i]);
    }
    EXPECT(!stream.error);

    stream.capacity = stream.bitPos; // narrow to what was actually written
    stream.bitPos = 0;
    for (i = 0; i < ARRAY_COUNT(widths); i++)
    {
        u32 readBack = TradeCode_ReadBits(&stream, widths[i]);
        EXPECT_EQ(readBack, values[i]);
    }
    EXPECT(!stream.error);
}

TEST("TradeCode_ReadBits past the end latches error and returns 0")
{
    u8 buf[1] = { 0xFF };
    struct TradeCodeBits stream = { .data = buf, .bitPos = 0, .capacity = 8, .error = FALSE };

    EXPECT_EQ(TradeCode_ReadBits(&stream, 8), 0xFF);
    EXPECT(!stream.error);

    // Nothing left to read - must not touch memory past `buf`.
    EXPECT_EQ(TradeCode_ReadBits(&stream, 1), 0);
    EXPECT(stream.error);

    // Once latched, further reads keep returning 0.
    EXPECT_EQ(TradeCode_ReadBits(&stream, 4), 0);
    EXPECT(stream.error);
}

TEST("TradeCode_WriteBits past capacity latches error without corrupting data")
{
    u8 buf[1] = { 0 };
    struct TradeCodeBits stream = { .data = buf, .bitPos = 0, .capacity = 4, .error = FALSE };

    TradeCode_WriteBits(&stream, 0xF, 4);
    EXPECT(!stream.error);
    EXPECT_EQ(buf[0], 0xF0);

    // Would cross the 4-bit capacity - rejected, buffer left untouched.
    TradeCode_WriteBits(&stream, 1, 1);
    EXPECT(stream.error);
    EXPECT_EQ(buf[0], 0xF0);
}

TEST("TradeCode_Decode folds confusable characters to the same value")
{
    // "1O111" read canonically, vs. the same code with every confusable
    // substituted: lowercase, I/i/l -> 1, O/o -> 0, plus stray hyphens and
    // spaces that must be skipped, not rejected.
    u8 canonical[] = { CHAR_1, CHAR_0, CHAR_1, CHAR_1, CHAR_1, EOS };
    u8 confusable[] = { CHAR_I, CHAR_HYPHEN, CHAR_o, CHAR_SPACE, CHAR_l, CHAR_i, CHAR_1, EOS };
    u8 bufA[TEST_BUF_BYTES], bufB[TEST_BUF_BYTES];
    struct TradeCodeBits outA, outB;

    outA.data = bufA;
    outA.capacity = sizeof(bufA) * 8;
    EXPECT_EQ(TradeCode_Decode(canonical, &outA), TRADE_CODE_OK);

    outB.data = bufB;
    outB.capacity = sizeof(bufB) * 8;
    EXPECT_EQ(TradeCode_Decode(confusable, &outB), TRADE_CODE_OK);

    EXPECT_EQ(outA.capacity, outB.capacity);
    EXPECT(BitsEqual(bufA, bufB, outA.capacity));
}

TEST("TradeCode_Decode rejects U and other out-of-charset bytes")
{
    u8 withU[] = { CHAR_1, CHAR_U, CHAR_1, EOS };
    u8 withPunct[] = { CHAR_1, CHAR_EXCL_MARK, CHAR_1, EOS };
    u8 buf[TEST_BUF_BYTES];
    struct TradeCodeBits out;

    out.data = buf;
    out.capacity = sizeof(buf) * 8;
    EXPECT_EQ(TradeCode_Decode(withU, &out), TRADE_CODE_BAD_CHAR);

    out.data = buf;
    out.capacity = sizeof(buf) * 8;
    EXPECT_EQ(TradeCode_Decode(withPunct, &out), TRADE_CODE_BAD_CHAR);
}

TEST("TradeCode_Decode rejects empty or skip-only input as too short")
{
    u8 empty[] = { EOS };
    u8 hyphensOnly[] = { CHAR_HYPHEN, CHAR_HYPHEN, CHAR_SPACE, EOS };
    u8 buf[TEST_BUF_BYTES];
    struct TradeCodeBits out;

    out.data = buf;
    out.capacity = sizeof(buf) * 8;
    EXPECT_EQ(TradeCode_Decode(empty, &out), TRADE_CODE_TOO_SHORT);

    out.data = buf;
    out.capacity = sizeof(buf) * 8;
    EXPECT_EQ(TradeCode_Decode(hyphensOnly, &out), TRADE_CODE_TOO_SHORT);
}

TEST("TradeCode_Decode rejects a code too long for the caller's buffer")
{
    u8 str[TEST_STR_CHARS];
    u8 src[TEST_BUF_BYTES];
    u8 buf[1]; // room for one symbol (5 bits) at most
    struct TradeCodeBits out;

    memset(src, 0xAA, sizeof(src));
    TradeCode_Encode(src, 40, str); // 8 symbols - far more than `buf` can hold

    out.data = buf;
    out.capacity = 5; // exactly one symbol's worth
    EXPECT_EQ(TradeCode_Decode(str, &out), TRADE_CODE_TOO_LONG);
}

TEST("TradeCode_Encode groups symbols with a hyphen every TRADE_CODE_GROUP_SIZE")
{
    u8 zeros[4] = { 0, 0, 0, 0 };
    u8 str[TEST_STR_CHARS];

    // 25 bits = exactly 5 symbols = one full group, no hyphen yet.
    TradeCode_Encode(zeros, 25, str);
    EXPECT_EQ(str[0], CHAR_0);
    EXPECT_EQ(str[4], CHAR_0);
    EXPECT_EQ(str[5], EOS);

    // 30 bits = 6 symbols: a hyphen must separate the first group of 5
    // from the 6th symbol.
    TradeCode_Encode(zeros, 30, str);
    EXPECT_EQ(str[5], CHAR_HYPHEN);
    EXPECT_EQ(str[6], CHAR_0);
    EXPECT_EQ(str[7], EOS);

    // All-ones nibble-plus-one: the first 5 bits (11111) decode to symbol
    // value 31, the last letter in the alphabet.
    {
        u8 ones = 0xF8; // 11111000
        u8 out[TEST_STR_CHARS];
        TradeCode_Encode(&ones, 5, out);
        EXPECT_EQ(out[0], CHAR_Z);
        EXPECT_EQ(out[1], EOS);
    }
}

// ---------------------------------------------------------------------
// Stage 2 of "Trading Codes.md": the BoxPokemon serialiser/deserialiser.
// ---------------------------------------------------------------------

#define MON_TEST_BUF_BYTES 64 // headroom over the largest Stage 2 payload (~374 bits -> 47 bytes)

// Serialises `orig` and immediately decodes it back into `decoded`, as if
// it had round-tripped through a real offer code (minus the Base32 layer,
// which Stage 1 already covers on its own).
static enum TradeCodeMonStatus RoundTripMon(const struct BoxPokemon *orig, struct BoxPokemon *decoded)
{
    u8 buf[MON_TEST_BUF_BYTES];
    struct TradeCodeBits stream;

    memset(buf, 0, sizeof(buf));
    stream.data = buf;
    stream.bitPos = 0;
    stream.capacity = sizeof(buf) * 8;
    stream.error = FALSE;

    TradeCode_SerializeMon(orig, &stream);
    EXPECT(!stream.error);

    stream.capacity = stream.bitPos; // narrow to what was actually written
    stream.bitPos = 0;
    return TradeCode_DeserializeMon(&stream, decoded);
}

// Every field TradeCode_SerializeMon/DeserializeMon claim to preserve,
// compared directly - deliberately excludes personality and experience,
// the plan doc's two documented lossy fields. Reads exclusively through
// Get(Box)MonData, matching the module's own contract.
static void ExpectMonsMatch(struct BoxPokemon *orig, struct BoxPokemon *decoded)
{
    u8 origName[POKEMON_NAME_LENGTH + 1], decodedName[POKEMON_NAME_LENGTH + 1];
    u32 i;

    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_SPECIES), GetBoxMonData(orig, MON_DATA_SPECIES));
    EXPECT_EQ(GetLevelFromBoxMonExp(decoded), GetLevelFromBoxMonExp(orig));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_HIDDEN_NATURE), GetBoxMonData(orig, MON_DATA_HIDDEN_NATURE));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_IS_SHINY), GetBoxMonData(orig, MON_DATA_IS_SHINY));
    EXPECT_EQ(GetGenderFromSpeciesAndPersonality(GetBoxMonData(decoded, MON_DATA_SPECIES), decoded->personality),
              GetGenderFromSpeciesAndPersonality(GetBoxMonData(orig, MON_DATA_SPECIES), orig->personality));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_ABILITY_NUM), GetBoxMonData(orig, MON_DATA_ABILITY_NUM));
    for (i = 0; i < 6; i++)
        EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_HP_IV + i), GetBoxMonData(orig, MON_DATA_HP_IV + i));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_POKEBALL), GetBoxMonData(orig, MON_DATA_POKEBALL));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_TERA_TYPE), GetBoxMonData(orig, MON_DATA_TERA_TYPE));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_OT_ID) & 0xFFFF, GetBoxMonData(orig, MON_DATA_OT_ID) & 0xFFFF);
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_OT_GENDER), GetBoxMonData(orig, MON_DATA_OT_GENDER));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_LANGUAGE), GetBoxMonData(orig, MON_DATA_LANGUAGE));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_IS_EGG), GetBoxMonData(orig, MON_DATA_IS_EGG));

    GetBoxMonData(orig, MON_DATA_OT_NAME, origName);
    GetBoxMonData(decoded, MON_DATA_OT_NAME, decodedName);
    EXPECT_EQ(StringCompare(origName, decodedName), 0);

    GetBoxMonData(orig, MON_DATA_NICKNAME, origName);
    GetBoxMonData(decoded, MON_DATA_NICKNAME, decodedName);
    EXPECT_EQ(StringCompare(origName, decodedName), 0);

    for (i = 0; i < 6; i++)
        EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_HP_EV + i), GetBoxMonData(orig, MON_DATA_HP_EV + i));

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_MOVE1 + i), GetBoxMonData(orig, MON_DATA_MOVE1 + i));
        EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_PP1 + i), GetBoxMonData(orig, MON_DATA_PP1 + i));
    }
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_PP_BONUSES), GetBoxMonData(orig, MON_DATA_PP_BONUSES));

    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_HELD_ITEM), GetBoxMonData(orig, MON_DATA_HELD_ITEM));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_FRIENDSHIP), GetBoxMonData(orig, MON_DATA_FRIENDSHIP));
    EXPECT_EQ(GetBoxMonData(decoded, MON_DATA_POKERUS), GetBoxMonData(orig, MON_DATA_POKERUS));
}

TEST("TradeCode_SerializeMon/DeserializeMon round-trip a hand-built mon")
{
    struct BoxPokemon orig, decoded;
    enum TradeCodeMonStatus status;

    // A plain wild-caught mon: nothing set past CreateBoxMon's own
    // defaults, so every optional presence bit should end up clear and the
    // round trip should still reproduce every field exactly.
    PARAMETRIZE
    {
        CreateBoxMon(&orig, SPECIES_PIDGEY, 37, 0x1234, OTID_STRUCT_PRESET(0xCAFEBABE));
    }

    // A fully custom competitive mon: every optional field present at once
    // (nickname, EVs, custom moves + PP Ups, held item, friendship,
    // pokerus), maxing out the presence bitmap.
    PARAMETRIZE
    {
        u8 nickname[POKEMON_NAME_LENGTH + 1] = _("STRIKER");
        u8 ev, i;
        enum Move moves[MAX_MON_MOVES] = { MOVE_POUND, MOVE_KARATE_CHOP, MOVE_DOUBLE_SLAP, MOVE_COMET_PUNCH };
        u32 heldItem = ITEM_ORAN_BERRY;
        u32 friendship = 200;
        u32 pokerus = 0x31; // strain 3, 1 day left
        u32 ppBonuses = 0xE4; // 2 bits/slot, PP Up counts 3,2,1,0 across the four slots

        CreateBoxMon(&orig, SPECIES_PIDGEY, 50, 0x9E8D7C6B, OTID_STRUCT_PRESET(0x11223344));
        SetBoxMonData(&orig, MON_DATA_NICKNAME, nickname);
        ev = 252;
        SetBoxMonData(&orig, MON_DATA_HP_EV, &ev);
        ev = 4;
        SetBoxMonData(&orig, MON_DATA_ATK_EV, &ev);
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            u32 move = moves[i];
            u32 pp = CalculatePPWithBonus(moves[i], ppBonuses, i);
            SetBoxMonData(&orig, MON_DATA_MOVE1 + i, &move);
            SetBoxMonData(&orig, MON_DATA_PP1 + i, &pp);
        }
        SetBoxMonData(&orig, MON_DATA_PP_BONUSES, &ppBonuses);
        SetBoxMonData(&orig, MON_DATA_HELD_ITEM, &heldItem);
        SetBoxMonData(&orig, MON_DATA_FRIENDSHIP, &friendship);
        SetBoxMonData(&orig, MON_DATA_POKERUS, &pokerus);
    }

    status = RoundTripMon(&orig, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);
    ExpectMonsMatch(&orig, &decoded);
}

TEST("TradeCode round-trip preserves shininess independent of personality")
{
    struct BoxPokemon orig, decoded;
    bool32 isShiny;
    enum TradeCodeMonStatus status;

    PARAMETRIZE { isShiny = FALSE; }
    PARAMETRIZE { isShiny = TRUE; }

    CreateBoxMon(&orig, SPECIES_WOBBUFFET, 20, 0x55667788, OTID_STRUCT_PRESET(0x99AABBCC));
    SetBoxMonData(&orig, MON_DATA_IS_SHINY, &isShiny);

    status = RoundTripMon(&orig, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);
    EXPECT_EQ(GetBoxMonData(&decoded, MON_DATA_IS_SHINY), isShiny);
    ExpectMonsMatch(&orig, &decoded);
}

TEST("TradeCode round-trip preserves a full 12-character nickname")
{
    struct BoxPokemon orig, decoded;
    u8 nickname[POKEMON_NAME_LENGTH + 1] = _("ABCDEFGHIJKL"); // exactly 12 characters
    enum TradeCodeMonStatus status;

    CreateBoxMon(&orig, SPECIES_PIDGEY, 15, 0xABCDEF01, OTID_STRUCT_PRESET(0x22446688));
    SetBoxMonData(&orig, MON_DATA_NICKNAME, nickname);

    status = RoundTripMon(&orig, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);
    ExpectMonsMatch(&orig, &decoded);
}

TEST("TradeCode round-trip preserves maxed-out EVs, quantised to multiples of 4")
{
    struct BoxPokemon orig, decoded;
    u32 i;
    enum TradeCodeMonStatus status;

    CreateBoxMon(&orig, SPECIES_PIDGEY, 100, 0x13579BDF, OTID_STRUCT_PRESET(0x2468ACE0));
    // 84 * 6 = 504, within MAX_TOTAL_EVS (510), and 84 is already a
    // multiple of 4 so the /4 quantisation on the wire is lossless here.
    for (i = 0; i < 6; i++)
    {
        u8 ev = 84;
        SetBoxMonData(&orig, MON_DATA_HP_EV + i, &ev);
    }

    status = RoundTripMon(&orig, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);
    ExpectMonsMatch(&orig, &decoded);
}

TEST("TradeCode round-trip preserves an egg")
{
    struct BoxPokemon orig, decoded;
    bool32 isEgg = TRUE;
    enum TradeCodeMonStatus status;

    CreateBoxMon(&orig, SPECIES_TOGEPI, 5, 0x0F0F0F0F, OTID_STRUCT_PRESET(0x0C0C0C0C));
    SetBoxMonData(&orig, MON_DATA_IS_EGG, &isEgg);

    status = RoundTripMon(&orig, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);
    EXPECT_EQ(GetBoxMonData(&decoded, MON_DATA_IS_EGG), TRUE);
    ExpectMonsMatch(&orig, &decoded);
}

TEST("TradeCode round-trip preserves MAX_LEVEL")
{
    struct BoxPokemon orig, decoded;
    enum TradeCodeMonStatus status;

    CreateBoxMon(&orig, SPECIES_PIDGEY, MAX_LEVEL, 0x76543210, OTID_STRUCT_PRESET(0x01234567));

    status = RoundTripMon(&orig, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);
    EXPECT_EQ(GetLevelFromBoxMonExp(&decoded), MAX_LEVEL);
    ExpectMonsMatch(&orig, &decoded);
}

TEST("TradeCode round-trip is stat-for-stat identical after CalculateMonStats")
{
    struct BoxPokemon origBox, decodedBox;
    struct Pokemon origMon, decodedMon;
    enum TradeCodeMonStatus status;
    u8 ev = 80; // 6*80 = 480, within MAX_TOTAL_EVS (510)
    u32 i;

    CreateBoxMon(&origBox, SPECIES_PIDGEY, 42, 0x87654321, OTID_STRUCT_PRESET(0xFEDCBA98));
    for (i = 0; i < 6; i++)
        SetBoxMonData(&origBox, MON_DATA_HP_EV + i, &ev);

    status = RoundTripMon(&origBox, &decodedBox);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);

    BoxMonToMon(&origBox, &origMon);
    BoxMonToMon(&decodedBox, &decodedMon);

    EXPECT_EQ(GetMonData(&decodedMon, MON_DATA_MAX_HP), GetMonData(&origMon, MON_DATA_MAX_HP));
    EXPECT_EQ(GetMonData(&decodedMon, MON_DATA_ATK), GetMonData(&origMon, MON_DATA_ATK));
    EXPECT_EQ(GetMonData(&decodedMon, MON_DATA_DEF), GetMonData(&origMon, MON_DATA_DEF));
    EXPECT_EQ(GetMonData(&decodedMon, MON_DATA_SPEED), GetMonData(&origMon, MON_DATA_SPEED));
    EXPECT_EQ(GetMonData(&decodedMon, MON_DATA_SPATK), GetMonData(&origMon, MON_DATA_SPATK));
    EXPECT_EQ(GetMonData(&decodedMon, MON_DATA_SPDEF), GetMonData(&origMon, MON_DATA_SPDEF));

    // The two documented lossy fields: personality is never transmitted at
    // all (no claim of equality either way), but experience is quantised
    // to the level, so unless the original mon happened to have zero
    // partial progress, the raw EXP values differ even though the level -
    // and therefore every stat above - does not.
    EXPECT_EQ(GetMonData(&decodedMon, MON_DATA_LEVEL), GetMonData(&origMon, MON_DATA_LEVEL));
}

TEST("TradeCode_SerializeMon omits optional fields already at their species/level default")
{
    struct BoxPokemon orig, decoded;
    enum TradeCodeMonStatus status;
    u8 buf[MON_TEST_BUF_BYTES];
    struct TradeCodeBits stream;

    CreateBoxMon(&orig, SPECIES_PIDGEY, 30, 0x22222222, OTID_STRUCT_PRESET(0x33333333));

    memset(buf, 0, sizeof(buf));
    stream.data = buf;
    stream.bitPos = 0;
    stream.capacity = sizeof(buf) * 8;
    stream.error = FALSE;
    TradeCode_SerializeMon(&orig, &stream);

    // The presence byte is the very first 8 bits written.
    EXPECT_EQ(buf[0], 0);

    stream.capacity = stream.bitPos;
    stream.bitPos = 0;
    status = TradeCode_DeserializeMon(&stream, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_OK);
    ExpectMonsMatch(&orig, &decoded);
}

TEST("TradeCode_DeserializeMon rejects malformed payloads and leaves outBoxMon untouched")
{
    struct BoxPokemon orig, decoded, sentinel;
    u8 buf[MON_TEST_BUF_BYTES];
    struct TradeCodeBits stream;
    enum TradeCodeMonStatus expectedStatus;

    CreateBoxMon(&orig, SPECIES_PIDGEY, 30, 0x44444444, OTID_STRUCT_PRESET(0x55555555));
    memset(&sentinel, 0xAA, sizeof(sentinel));

    memset(buf, 0, sizeof(buf));
    stream.data = buf;
    stream.bitPos = 0;
    stream.capacity = sizeof(buf) * 8;
    stream.error = FALSE;
    TradeCode_SerializeMon(&orig, &stream);
    stream.capacity = stream.bitPos;

    PARAMETRIZE
    {
        // Corrupt the species field (the 8 presence bits are first, so bits
        // 8-18 are species) to a value >= NUM_SPECIES.
        u32 i;
        for (i = 8; i < 19; i++)
            buf[i / 8] |= (1 << (7 - (i % 8)));
        expectedStatus = TRADE_CODE_MON_BAD_SPECIES;
    }
    PARAMETRIZE
    {
        // The presence byte is written MSB-first into a single, byte-aligned
        // byte (buf[0] == the presence value itself) - 0x80 is bit 7, one of
        // the two bits this format version reserves.
        buf[0] |= 0x80;
        expectedStatus = TRADE_CODE_MON_RESERVED_BITS_SET;
    }
    PARAMETRIZE
    {
        // Truncate the stream to fewer bits than the core fields need.
        stream.capacity = 10;
        expectedStatus = TRADE_CODE_MON_TRUNCATED;
    }

    decoded = sentinel;
    stream.bitPos = 0;
    EXPECT_EQ(TradeCode_DeserializeMon(&stream, &decoded), expectedStatus);
    EXPECT_EQ(memcmp(&decoded, &sentinel, sizeof(decoded)), 0);
}

TEST("TradeCode_DeserializeMon rejects an EV total above MAX_TOTAL_EVS")
{
    struct TradeCodeBits stream;
    u8 buf[MON_TEST_BUF_BYTES];
    struct BoxPokemon decoded;
    u32 i;

    memset(buf, 0, sizeof(buf));
    stream.data = buf;
    stream.bitPos = 0;
    stream.capacity = sizeof(buf) * 8;
    stream.error = FALSE;

    TradeCode_WriteBits(&stream, 0x02, 8); // presence: EVs bit (bit 1) set, nothing else
    TradeCode_WriteBits(&stream, SPECIES_PIDGEY, 11);
    TradeCode_WriteBits(&stream, 50, 10); // level
    TradeCode_WriteBits(&stream, 0, 5);   // nature
    TradeCode_WriteBits(&stream, 0, 2);   // gender
    TradeCode_WriteBits(&stream, 0, 1);   // shiny
    TradeCode_WriteBits(&stream, 0, 2);   // ability num
    for (i = 0; i < 6; i++)
        TradeCode_WriteBits(&stream, 0, 5); // IVs
    TradeCode_WriteBits(&stream, 0, 6);   // pokeball
    TradeCode_WriteBits(&stream, 0, 5);   // tera type
    TradeCode_WriteBits(&stream, 0, 16);  // otId low
    TradeCode_WriteBits(&stream, 0, 1);   // ot gender
    TradeCode_WriteBits(&stream, 0, 3);   // language
    TradeCode_WriteBits(&stream, 0, 1);   // isEgg
    TradeCode_WriteBits(&stream, 0, 3);   // OT name length (0)
    // Optional EVs block: every stat at the max 6-bit value (63 -> 252
    // actual), for a total of 1512 - well past MAX_TOTAL_EVS (510).
    for (i = 0; i < 6; i++)
        TradeCode_WriteBits(&stream, 63, 6);
    EXPECT(!stream.error);

    stream.capacity = stream.bitPos;
    stream.bitPos = 0;
    EXPECT_EQ(TradeCode_DeserializeMon(&stream, &decoded), TRADE_CODE_MON_BAD_EV_TOTAL);
}

TEST("TradeCode_DeserializeMon rejects an unknown move and an unknown held item")
{
    struct TradeCodeBits stream;
    u8 buf[MON_TEST_BUF_BYTES];
    struct BoxPokemon decoded;
    u32 i;
    u8 presence;
    enum TradeCodeMonStatus expectedStatus;

    PARAMETRIZE { presence = 0x04; expectedStatus = TRADE_CODE_MON_BAD_MOVE; }   // presence bit 2: moves
    PARAMETRIZE { presence = 0x08; expectedStatus = TRADE_CODE_MON_BAD_ITEM; }   // presence bit 3: held item

    memset(buf, 0, sizeof(buf));
    stream.data = buf;
    stream.bitPos = 0;
    stream.capacity = sizeof(buf) * 8;
    stream.error = FALSE;

    TradeCode_WriteBits(&stream, presence, 8);
    TradeCode_WriteBits(&stream, SPECIES_PIDGEY, 11);
    TradeCode_WriteBits(&stream, 50, 10);
    TradeCode_WriteBits(&stream, 0, 5);
    TradeCode_WriteBits(&stream, 0, 2);
    TradeCode_WriteBits(&stream, 0, 1);
    TradeCode_WriteBits(&stream, 0, 2);
    for (i = 0; i < 6; i++)
        TradeCode_WriteBits(&stream, 0, 5);
    TradeCode_WriteBits(&stream, 0, 6);
    TradeCode_WriteBits(&stream, 0, 5);
    TradeCode_WriteBits(&stream, 0, 16);
    TradeCode_WriteBits(&stream, 0, 1);
    TradeCode_WriteBits(&stream, 0, 3);
    TradeCode_WriteBits(&stream, 0, 1);
    TradeCode_WriteBits(&stream, 0, 3); // OT name length

    if (presence & 0x04)
    {
        // 2047 is the 11-bit field's max representable value - guaranteed
        // >= MOVES_COUNT without risking an overflow wraparound the way
        // "MOVES_COUNT + N" could if MOVES_COUNT is already close to 2047.
        for (i = 0; i < MAX_MON_MOVES; i++)
            TradeCode_WriteBits(&stream, 2047, 11);
        TradeCode_WriteBits(&stream, 0, 8); // PP bonuses
    }
    if (presence & 0x08)
        TradeCode_WriteBits(&stream, 1023, 10); // the 10-bit field's max - same reasoning
    EXPECT(!stream.error);

    stream.capacity = stream.bitPos;
    stream.bitPos = 0;
    EXPECT_EQ(TradeCode_DeserializeMon(&stream, &decoded), expectedStatus);
}

TEST("TradeCode_DeserializeMon rejects an oversized nickname length")
{
    struct TradeCodeBits stream;
    u8 buf[MON_TEST_BUF_BYTES];
    struct BoxPokemon decoded;
    u32 i;

    memset(buf, 0, sizeof(buf));
    stream.data = buf;
    stream.bitPos = 0;
    stream.capacity = sizeof(buf) * 8;
    stream.error = FALSE;

    TradeCode_WriteBits(&stream, 0x01, 8); // presence: nickname bit (bit 0) set
    TradeCode_WriteBits(&stream, SPECIES_PIDGEY, 11);
    TradeCode_WriteBits(&stream, 50, 10);
    TradeCode_WriteBits(&stream, 0, 5);
    TradeCode_WriteBits(&stream, 0, 2);
    TradeCode_WriteBits(&stream, 0, 1);
    TradeCode_WriteBits(&stream, 0, 2);
    for (i = 0; i < 6; i++)
        TradeCode_WriteBits(&stream, 0, 5);
    TradeCode_WriteBits(&stream, 0, 6);
    TradeCode_WriteBits(&stream, 0, 5);
    TradeCode_WriteBits(&stream, 0, 16);
    TradeCode_WriteBits(&stream, 0, 1);
    TradeCode_WriteBits(&stream, 0, 3);
    TradeCode_WriteBits(&stream, 0, 1);
    TradeCode_WriteBits(&stream, 0, 3); // OT name length
    TradeCode_WriteBits(&stream, 15, 4); // nickname length: the 4-bit field's max, well past POKEMON_NAME_LENGTH (12)
    EXPECT(!stream.error);

    stream.capacity = stream.bitPos;
    stream.bitPos = 0;
    EXPECT_EQ(TradeCode_DeserializeMon(&stream, &decoded), TRADE_CODE_MON_BAD_NAME);
}

TEST("TradeCode_DeserializeMon rejects isEgg combined with a custom nickname")
{
    struct BoxPokemon orig, decoded;
    bool32 isEgg = TRUE;
    u8 nickname[POKEMON_NAME_LENGTH + 1] = _("NOTANEGG");
    enum TradeCodeMonStatus status;

    // The encoder itself never produces this combination - it just reports
    // the mon's actual state honestly. A hostile/hand-edited code could
    // still carry it, so the deserialiser has to catch it.
    CreateBoxMon(&orig, SPECIES_TOGEPI, 5, 0x66666666, OTID_STRUCT_PRESET(0x77777777));
    SetBoxMonData(&orig, MON_DATA_IS_EGG, &isEgg);
    SetBoxMonData(&orig, MON_DATA_NICKNAME, nickname);

    status = RoundTripMon(&orig, &decoded);
    EXPECT_EQ(status, TRADE_CODE_MON_EGG_WITH_NICKNAME);
}

// ---------------------------------------------------------------------
// Stage 3 of "Trading Codes.md": sealing, nonces, replay protection.
// ---------------------------------------------------------------------

TEST("TradeCode_SealOffer changes when any single bit of the payload is flipped")
{
    struct BoxPokemon mon;
    u8 original[MON_TEST_BUF_BYTES];
    struct TradeCodeBits stream;
    u32 nBits;
    u32 originalSeal;
    u32 bit;

    CreateBoxMon(&mon, SPECIES_PIDGEY, 37, 0x1234, OTID_STRUCT_PRESET(0xCAFEBABE));

    memset(original, 0, sizeof(original));
    stream.data = original;
    stream.bitPos = 0;
    stream.capacity = sizeof(original) * 8;
    stream.error = FALSE;
    TradeCode_SerializeMon(&mon, &stream);
    nBits = stream.bitPos;

    originalSeal = TradeCode_SealOffer(original, nBits);

    // Every single bit of the payload, one at a time - not just a sample -
    // per the plan doc's own acceptance test.
    for (bit = 0; bit < nBits; bit++)
    {
        u8 flipped[MON_TEST_BUF_BYTES];
        u32 byteIdx = bit / 8;
        u32 bitInByte = 7 - (bit % 8);
        u32 flippedSeal;

        memcpy(flipped, original, sizeof(flipped));
        flipped[byteIdx] ^= (1 << bitInByte);

        flippedSeal = TradeCode_SealOffer(flipped, nBits);
        EXPECT(flippedSeal != originalSeal);
    }
}

TEST("TradeCode_SealOffer differs for the same mon behind a different nonce")
{
    struct BoxPokemon mon;
    u8 bufA[MON_TEST_BUF_BYTES], bufB[MON_TEST_BUF_BYTES];
    struct TradeCodeBits streamA, streamB;
    u32 sealA, sealB;

    // A stand-in 16-bit nonce prefix ahead of the mon payload - Stage 7
    // assembles the real header, but sealing itself only cares about "every
    // preceding bit", so prefixing here exercises the same property without
    // needing the header layer.
    CreateBoxMon(&mon, SPECIES_PIDGEY, 37, 0x1234, OTID_STRUCT_PRESET(0xCAFEBABE));

    memset(bufA, 0, sizeof(bufA));
    streamA.data = bufA;
    streamA.bitPos = 0;
    streamA.capacity = sizeof(bufA) * 8;
    streamA.error = FALSE;
    TradeCode_WriteBits(&streamA, 0x1111, 16);
    TradeCode_SerializeMon(&mon, &streamA);

    memset(bufB, 0, sizeof(bufB));
    streamB.data = bufB;
    streamB.bitPos = 0;
    streamB.capacity = sizeof(bufB) * 8;
    streamB.error = FALSE;
    TradeCode_WriteBits(&streamB, 0x2222, 16);
    TradeCode_SerializeMon(&mon, &streamB);

    sealA = TradeCode_SealOffer(bufA, streamA.bitPos);
    sealB = TradeCode_SealOffer(bufB, streamB.bitPos);

    EXPECT(sealA != sealB);
}

TEST("TradeCode_ConfirmTag is deterministic, differs by role, and fits 28 bits")
{
    struct BoxPokemon monA, monB;
    u8 bufA[MON_TEST_BUF_BYTES], bufB[MON_TEST_BUF_BYTES];
    struct TradeCodeBits streamA, streamB;
    u32 otIdA, otIdB;
    u16 nonceA, nonceB;
    u32 tagSelfA, tagSelfARepeat, tagSelfB;

    // Three relative orderings: A's otId below B's, above B's, and tied
    // (nonce breaks the tie) - the canonical-order logic must keep A's tag
    // distinct from B's tag in every case, not just the common one.
    PARAMETRIZE { otIdA = 0x0000AAAA; otIdB = 0x0000BBBB; nonceA = 0x1111; nonceB = 0x2222; }
    PARAMETRIZE { otIdA = 0x0000BBBB; otIdB = 0x0000AAAA; nonceA = 0x1111; nonceB = 0x2222; }
    PARAMETRIZE { otIdA = 0x0000CCCC; otIdB = 0x0000CCCC; nonceA = 0x1111; nonceB = 0x2222; }

    CreateBoxMon(&monA, SPECIES_PIDGEY, 10, 0x11111111, OTID_STRUCT_PRESET(otIdA));
    CreateBoxMon(&monB, SPECIES_WOBBUFFET, 20, 0x22222222, OTID_STRUCT_PRESET(otIdB));

    memset(bufA, 0, sizeof(bufA));
    streamA.data = bufA;
    streamA.bitPos = 0;
    streamA.capacity = sizeof(bufA) * 8;
    streamA.error = FALSE;
    TradeCode_SerializeMon(&monA, &streamA);

    memset(bufB, 0, sizeof(bufB));
    streamB.data = bufB;
    streamB.bitPos = 0;
    streamB.capacity = sizeof(bufB) * 8;
    streamB.error = FALSE;
    TradeCode_SerializeMon(&monB, &streamB);

    tagSelfA = TradeCode_ConfirmTag(bufA, streamA.bitPos, otIdA, nonceA,
                                     bufB, streamB.bitPos, otIdB, nonceB);
    tagSelfARepeat = TradeCode_ConfirmTag(bufA, streamA.bitPos, otIdA, nonceA,
                                           bufB, streamB.bitPos, otIdB, nonceB);
    EXPECT_EQ(tagSelfA, tagSelfARepeat); // pure function of its inputs

    tagSelfB = TradeCode_ConfirmTag(bufB, streamB.bitPos, otIdB, nonceB,
                                     bufA, streamA.bitPos, otIdA, nonceA);
    // A's revealed code must differ from B's - "each player enters the
    // other's, not their own" only means something if they're distinct.
    EXPECT(tagSelfA != tagSelfB);

    EXPECT_EQ(tagSelfA & ~0x0FFFFFFF, 0);
    EXPECT_EQ(tagSelfB & ~0x0FFFFFFF, 0);
}

TEST("TradeCode_IsOfferSealUsed/RecordOfferSeal: the ring rejects a repeat and evicts the oldest")
{
    u32 ring[TRADE_CODE_REPLAY_RING];
    u32 i;

    memset(ring, 0, sizeof(ring));

    EXPECT(!TradeCode_IsOfferSealUsed(ring, 0x12345678));

    TradeCode_RecordOfferSeal(ring, 0x12345678);
    EXPECT(TradeCode_IsOfferSealUsed(ring, 0x12345678));
    // A seal that was never recorded must not collide with one that was.
    EXPECT(!TradeCode_IsOfferSealUsed(ring, 0x87654321));

    // Push TRADE_CODE_REPLAY_RING more distinct seals through - the very
    // first one recorded above must fall off the back once the ring is
    // full, pinning down the drop-oldest eviction policy.
    for (i = 0; i < TRADE_CODE_REPLAY_RING; i++)
        TradeCode_RecordOfferSeal(ring, 0xA0000000 + i);

    EXPECT(!TradeCode_IsOfferSealUsed(ring, 0x12345678));
    for (i = 0; i < TRADE_CODE_REPLAY_RING; i++)
        EXPECT(TradeCode_IsOfferSealUsed(ring, 0xA0000000 + i));
}

// Stage 9 of "Trading Codes.md": reset-resistance. Unlike Stage 4/8's own
// UI/save-flow-orchestration logic (which the plan doc's own status blocks
// for those stages note has no headless-testable unit beyond what Stages
// 1-3 already cover), TradeCode_ValidatePendingBoxMon is a small pure
// function over an already-materialised BoxPokemon - exactly the shape
// every other Stage 1-3 primitive in this file already gets tests for.

TEST("TradeCode_ValidatePendingBoxMon accepts a normal mon and rejects a bad checksum")
{
    struct BoxPokemon mon;

    CreateBoxMon(&mon, SPECIES_PIDGEY, 30, 0x13572468, OTID_STRUCT_PRESET(0x87654321));
    EXPECT(TradeCode_ValidatePendingBoxMon(&mon));

    // Corrupt the checksum directly - the same kind of on-disk bit rot or
    // hand-editing this function exists to catch. Poking the struct like
    // this is exactly what TradeCode_SerializeMon itself is never allowed
    // to do (it only reads through GetBoxMonData), but simulating real
    // save-file corruption in a test has no other way to do it.
    mon.checksum ^= 0xFFFF;
    EXPECT(!TradeCode_ValidatePendingBoxMon(&mon));
}

TEST("TradeCode_ValidatePendingBoxMon rejects a species outside NUM_SPECIES")
{
    struct BoxPokemon mon;
    u16 badSpecies = NUM_SPECIES; // one past the last valid species

    CreateBoxMon(&mon, SPECIES_PIDGEY, 30, 0x13572468, OTID_STRUCT_PRESET(0x87654321));
    // SetBoxMonData recomputes the checksum after the write, so this stays
    // checksum-valid - isolates the species-range check from the checksum
    // check the previous test already covers.
    SetBoxMonData(&mon, MON_DATA_SPECIES, &badSpecies);
    EXPECT(!TradeCode_ValidatePendingBoxMon(&mon));
}
