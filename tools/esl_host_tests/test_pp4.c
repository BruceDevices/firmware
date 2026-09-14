#include "esl_pp4.h"
#include "test_util.h"

/* Ticks at 80 MHz are cycles * 80/64 = cycles * 1.25, truncated by the
 * integer division in ESL_PP4_TICKS. */
#define RES 80000000u
#define T_BURST 3226u
#define T_GAP0 4838u
#define T_GAP1 19353u
#define T_GAP2 9676u
#define T_GAP3 14515u

static void test_tick_derivation(void) {
    CHECK_EQ(ESL_PP4_TICKS(ESL_PP4_BURST_CYCLES, RES), T_BURST);
    CHECK_EQ(ESL_PP4_TICKS(esl_pp4_gap_cycles[0], RES), T_GAP0);
    CHECK_EQ(ESL_PP4_TICKS(esl_pp4_gap_cycles[1], RES), T_GAP1);
    CHECK_EQ(ESL_PP4_TICKS(esl_pp4_gap_cycles[2], RES), T_GAP2);
    CHECK_EQ(ESL_PP4_TICKS(esl_pp4_gap_cycles[3], RES), T_GAP3);

    /* Identity at the source clock: ticks must equal the Flipper cycle counts. */
    CHECK_EQ(ESL_PP4_TICKS(ESL_PP4_BURST_CYCLES, ESL_PP4_SRC_CLOCK_HZ), 2581);
    CHECK_EQ(ESL_PP4_TICKS(esl_pp4_gap_cycles[1], ESL_PP4_SRC_CLOCK_HZ), 15483);

    /* The gap table must stay indexed by raw dibit value. */
    CHECK_EQ(esl_pp4_gap_cycles[0], 3871);
    CHECK_EQ(esl_pp4_gap_cycles[1], 15483);
    CHECK_EQ(esl_pp4_gap_cycles[2], 7741);
    CHECK_EQ(esl_pp4_gap_cycles[3], 11612);

    /* Everything must fit RMT's 15-bit duration field. */
    CHECK(T_GAP1 < 32768u);
}

static void test_symbol_count(void) {
    CHECK_EQ(esl_pp4_symbol_count(1), 5);
    CHECK_EQ(esl_pp4_symbol_count(34), 137);
    CHECK_EQ(esl_pp4_symbol_count(255), 1021);
    CHECK_EQ(esl_pp4_symbol_count(0), 0);
    CHECK_EQ(esl_pp4_symbol_count(256), 0);
}

static void test_dibit_order_and_gaps(void) {
    EslPp4Symbol sym[8];
    const uint8_t frame[1] = {0xE4}; /* dibits LSB-first: 0, 1, 2, 3 */

    CHECK_EQ(esl_pp4_encode(frame, 1, RES, sym, 8), 5);
    CHECK_EQ(sym[0].gap_ticks, T_GAP0);
    CHECK_EQ(sym[1].gap_ticks, T_GAP1);
    CHECK_EQ(sym[2].gap_ticks, T_GAP2);
    CHECK_EQ(sym[3].gap_ticks, T_GAP3);

    /* Every symbol carries the same burst, including the closing one. */
    for (int i = 0; i < 5; i++) CHECK_EQ(sym[i].burst_ticks, T_BURST);

    /* Closing symbol uses the tail gap. */
    CHECK_EQ(sym[4].gap_ticks, ESL_PP4_TICKS(ESL_PP4_TAIL_GAP_CYCLES, RES));
}

static void test_multibyte(void) {
    EslPp4Symbol sym[16];
    const uint8_t frame[2] = {0x00, 0xFF};

    CHECK_EQ(esl_pp4_encode(frame, 2, RES, sym, 16), 9);
    /* 0x00 -> four dibits of 0 */
    for (int i = 0; i < 4; i++) CHECK_EQ(sym[i].gap_ticks, T_GAP0);
    /* 0xFF -> four dibits of 3 */
    for (int i = 4; i < 8; i++) CHECK_EQ(sym[i].gap_ticks, T_GAP3);
}

static void test_guards(void) {
    EslPp4Symbol sym[8];
    const uint8_t frame[1] = {0x00};

    CHECK_EQ(esl_pp4_encode(NULL, 1, RES, sym, 8), 0);
    CHECK_EQ(esl_pp4_encode(frame, 1, RES, NULL, 8), 0);
    CHECK_EQ(esl_pp4_encode(frame, 0, RES, sym, 8), 0);
    CHECK_EQ(esl_pp4_encode(frame, 1, 0, sym, 8), 0);
    /* Refuses to overflow the caller's buffer. */
    CHECK_EQ(esl_pp4_encode(frame, 1, RES, sym, 4), 0);
}

int main(void) {
    test_tick_derivation();
    test_symbol_count();
    test_dibit_order_and_gaps();
    test_multibyte();
    test_guards();
    TEST_REPORT("test_pp4");
}
