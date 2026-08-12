#include "global.h"
#include "trade_code.h"
#include "constants/characters.h"

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

// Defensive cap on how many raw characters TradeCode_Decode will scan
// looking for EOS. This is deliberately independent of TRADE_CODE_MAX_CHARS
// (the entry screen's display/entry limit, Stage 5/6's concern) - its only
// job is to stop a non-terminated buffer from being read past forever;
// TRADE_CODE_TOO_LONG for a legitimately-too-long *code* is already caught
// below by running out of `out->capacity` bits. Generous enough to cover
// every bit length Stage 1's own tests exercise (up to 400 bits -> 80
// symbols -> 95 characters with hyphens) plus headroom.
#define TRADE_CODE_DECODE_SCAN_LIMIT 512

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
