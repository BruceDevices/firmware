#include "esl_text.h"
#include "esl_proto.h"

#include <stddef.h>

uint8_t esl_text_color26_pixel(size_t idx, void *ctx) {
    const EslTextColor26Ctx *c = (const EslTextColor26Ctx *)ctx;
    if (c == NULL || c->primary == NULL) return 0u;

    const size_t plane_count = (size_t)TAGTINKER_COLOR26_WIRE_W *
                               (size_t)TAGTINKER_COLOR26_WIRE_H;
    uint8_t plane = 0u;
    if (idx >= plane_count) {
        plane = 1u;
        idx -= plane_count;
    }
    if (idx >= plane_count) return 0u;

    const uint16_t px = (uint16_t)(idx % TAGTINKER_COLOR26_WIRE_W);
    const uint16_t py = (uint16_t)(idx / TAGTINKER_COLOR26_WIRE_W);
    uint16_t gx = 0u, gy = 0u;
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, px, py, &gx, &gy);
    if (gx >= TAGTINKER_COLOR26_GLASS_W || gy >= TAGTINKER_COLOR26_GLASS_H) {
        return 0u;
    }

    const uint8_t *src = (plane == 0u) ? c->primary : c->secondary;
    if (src == NULL) return 0u;
    return src[(size_t)gy * TAGTINKER_COLOR26_GLASS_W + gx] ? 0u : 1u;
}

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
