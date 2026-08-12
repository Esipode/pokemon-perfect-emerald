#include "global.h"
#include "test/test.h"
#include "trade_code.h"
#include "constants/characters.h"
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
