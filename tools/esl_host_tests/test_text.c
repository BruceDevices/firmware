#include "esl_text.h"
#include "font/tagtinker_font.h"
#include "test_util.h"
#include <stdint.h>

static void test_should_send_full(void) {
    /* 208×112×2 = 46592 ≤ 49152 (most common color DM). */
    CHECK(esl_text_should_send_full(208, 112, true));
    /* 296×152×2 = 89984 > 49152. */
    CHECK(!esl_text_should_send_full(296, 152, true));
    CHECK(esl_text_should_send_full(296, 152, false));
    CHECK_EQ(ESL_TEXT_FULL_JOB_PIXEL_LIMIT, 49152u);
}

static void test_chunk_height(void) {
    /* 8 KB / width, round down to 8-row if ≥16, min 1. */
    CHECK_EQ(esl_text_chunk_height(208, 112, true), 32u); /* 8192/208=39 → 32 */
    CHECK_EQ(esl_text_chunk_height(296, 152, true), 24u); /* 8192/296=27 → 24 */
    CHECK_EQ(esl_text_chunk_height(152, 296, false), 48u); /* 8192/152=53 → 48 */
    CHECK_EQ(esl_text_chunk_height(800, 480, false), 10u); /* 8192/800=10, <16 */
    CHECK_EQ(esl_text_chunk_height(0, 112, false), 1u);
    CHECK_EQ(esl_text_chunk_height(208, 0, false), 1u);
    CHECK_EQ(esl_text_chunk_height(208, 8, true), 8u); /* cannot exceed height */
}

static void test_chunk_settle(void) {
    /* 500 + pixels/20, clamp 800–2000. color_clear is unused. */
    CHECK_EQ(esl_text_chunk_settle_ms(208, 32, false), 832u);
    CHECK_EQ(esl_text_chunk_settle_ms(208, 32, true), 832u);
    CHECK_EQ(esl_text_chunk_settle_ms(1, 1, false), 800u);
    CHECK_EQ(esl_text_chunk_settle_ms(800, 480, false), 2000u);
}

static void test_render_short_string(void) {
    uint8_t buf[32 * 16];
    render_text_ex(buf, 32, 16, "A", 1, 0, 0);
    CHECK_EQ(buf[0], 1u); /* background is clear */
    /* scale=2, start=(10,1); glyph col0 bit1 → (10, 1+2)=(10,3) is ink. */
    CHECK_EQ(buf[3 * 32 + 10], 0u);
    CHECK_EQ(buf[1 * 32 + 10], 1u); /* gy=0 of 'A' is empty (0x7e) */
}

int main(void) {
    test_should_send_full();
    test_chunk_height();
    test_chunk_settle();
    test_render_short_string();
    TEST_REPORT("test_text");
}
