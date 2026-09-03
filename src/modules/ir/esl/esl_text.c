#include "esl_text.h"

#include <stddef.h>

bool esl_text_should_send_full(uint16_t width, uint16_t height, bool second_plane) {
    size_t pixel_count = (size_t)width * height;
    if(second_plane) pixel_count *= 2U;
    return pixel_count <= ESL_TEXT_FULL_JOB_PIXEL_LIMIT;
}

uint32_t esl_text_chunk_settle_ms(uint16_t width, uint16_t height, bool color_clear) {
    /* Tag needs time to process each chunk before accepting the next one.
     * Keep this as short as possible while remaining reliable. */
    (void)color_clear;
    size_t work_pixels = (size_t)width * height;
    uint32_t delay_ms = 500U + (uint32_t)(work_pixels / 20U);
    if(delay_ms < 800U) delay_ms = 800U;
    if(delay_ms > 2000U) delay_ms = 2000U;
    return delay_ms;
}

uint16_t esl_text_chunk_height(uint16_t width, uint16_t height, bool second_plane) {
    (void)second_plane;
    if(width == 0U || height == 0U) return 1U;

    /*
     * Keep each plane buffer ≤ 8 KB so two planes + encode overhead fits
     * in the Flipper's heap even right after BLE teardown.
     * 8 KB (down from 12 KB) leaves extra headroom for the encoder's
     * internal allocations and any lingering BLE buffers.
     */
    size_t per_plane_budget = 8192U; /* 8 KB */
    uint16_t chunk_h = (uint16_t)(per_plane_budget / width);
    if(chunk_h == 0U) chunk_h = 1U;
    if(chunk_h > height) chunk_h = height;

    /* Round down to 8-row boundary for alignment, but keep at least 1 */
    if(chunk_h >= 16U) {
        chunk_h = (uint16_t)(chunk_h & ~7U);
        if(chunk_h == 0U) chunk_h = 8U;
    }

    if(chunk_h > height) chunk_h = height;
    if(chunk_h == 0U) chunk_h = 1U;
    return chunk_h;
}
