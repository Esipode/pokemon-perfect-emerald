#include "global.h"
#include "test/test.h"

TEST("uq4_12_add adds 4.12 numbers") {
    EXPECT_EQ(uq4_12_add(UQ_4_12(3.5), UQ_4_12(2.5)), UQ_4_12(6.0));
}

TEST("uq4_12_subtract subtracts 4.12 numbers") {
    EXPECT_EQ(uq4_12_subtract(UQ_4_12(3.5), UQ_4_12(2.0)), UQ_4_12(1.5));
}

TEST("uq4_12_multiply multiplies 4.12 numbers") {
    EXPECT_EQ(uq4_12_multiply(UQ_4_12(3.5), UQ_4_12(2.0)), UQ_4_12(7.0));
}

TEST("uq4_12_divide divides 4.12 numbers") {
    EXPECT_EQ(uq4_12_divide(UQ_4_12(5.0), UQ_4_12(2.0)), UQ_4_12(2.5));
}

TEST("uq4_12_multiply_by_int_half_down multiplies an int by a 4.12 number") {
    EXPECT_EQ(uq4_12_multiply_by_int_half_down(UQ_4_12(1.5), 100), 150);
    EXPECT_EQ(uq4_12_multiply_by_int_half_down(UQ_4_12(0.5), 100), 50);
    // 101 * 1.5 = 151.5 exactly; ties round down.
    EXPECT_EQ(uq4_12_multiply_by_int_half_down(UQ_4_12(1.5), 101), 151);
}

TEST("uq4_12_multiply_by_int_half_up multiplies an int by a 4.12 number") {
    EXPECT_EQ(uq4_12_multiply_by_int_half_up(UQ_4_12(1.5), 100), 150);
    EXPECT_EQ(uq4_12_multiply_by_int_half_up(UQ_4_12(0.5), 100), 50);
    // 101 * 1.5 = 151.5 exactly; ties round up.
    EXPECT_EQ(uq4_12_multiply_by_int_half_up(UQ_4_12(1.5), 101), 152);
}

// Regression tests for the modifier-multiply overflow at MAX_LEVEL.
//
// Both helpers form `modifier * value` before shifting the fixed-point result back down.
// `uq4_12_t` is 32-bit, so an unwidened product wraps once `modifier * value` exceeds
// UINT32_MAX -- reachable at MAX_LEVEL, where the running damage total handed to these
// helpers can sit in the hundreds of thousands while modifiers run up to 2.0x.
//
//   UQ_4_12(2.0) * 1000000 = 8192 * 1000000 = 8,192,000,000  (> UINT32_MAX)
//     64-bit: 8,192,000,000 / 4096      = 2,000,000  <- correct
//     32-bit: wraps to 3,897,032,704    =   951,424  <- what the bug produced
//
// A battle-level test that drives real damage this high is not writable yet: the value
// would still be truncated by the s16 `moveDamage` field (Bug C, Stage 4) before any
// assertion could observe it. These unit tests pin the helpers themselves instead.
TEST("uq4_12_multiply_by_int_half_down does not overflow above UINT32_MAX") {
    EXPECT_EQ(uq4_12_multiply_by_int_half_down(UQ_4_12(2.0), 1000000), 2000000);
    // 1000001 * 1.5 = 1,500,001.5 exactly, from a 6,144,006,144 product; ties round down.
    EXPECT_EQ(uq4_12_multiply_by_int_half_down(UQ_4_12(1.5), 1000001), 1500001);
}

TEST("uq4_12_multiply_by_int_half_up does not overflow above UINT32_MAX") {
    EXPECT_EQ(uq4_12_multiply_by_int_half_up(UQ_4_12(2.0), 1000000), 2000000);
    // Same product as above; ties round up.
    EXPECT_EQ(uq4_12_multiply_by_int_half_up(UQ_4_12(1.5), 1000001), 1500002);
}
