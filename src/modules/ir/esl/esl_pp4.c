#include "esl_pp4.h"

const uint32_t esl_pp4_gap_cycles[4] = {
    3871u,  /* dibit 0 -> ~60.5 us  */
    15483u, /* dibit 1 -> ~241.9 us */
    7741u,  /* dibit 2 -> ~121.0 us */
    11612u, /* dibit 3 -> ~181.4 us */
};

size_t esl_pp4_symbol_count(size_t frame_len) {
    if (frame_len == 0u || frame_len > ESL_PP4_MAX_FRAME_LEN) return 0u;
    return frame_len * ESL_PP4_DIBITS_PER_BYTE + 1u;
}

size_t esl_pp4_encode(const uint8_t *frame, size_t len, uint32_t resolution_hz,
                      EslPp4Symbol *out, size_t out_cap) {
    const size_t need = esl_pp4_symbol_count(len);
    if (frame == NULL || out == NULL || need == 0u || out_cap < need ||
        resolution_hz == 0u) {
        return 0u;
    }

    const uint16_t burst =
        (uint16_t)ESL_PP4_TICKS(ESL_PP4_BURST_CYCLES, resolution_hz);
    uint16_t gaps[4];
    for (unsigned i = 0u; i < 4u; i++) {
        gaps[i] = (uint16_t)ESL_PP4_TICKS(esl_pp4_gap_cycles[i], resolution_hz);
    }

    size_t n = 0u;
    for (size_t byte_idx = 0u; byte_idx < len; byte_idx++) {
        uint8_t b = frame[byte_idx];
        for (unsigned s = 0u; s < ESL_PP4_DIBITS_PER_BYTE; s++) {
            out[n].burst_ticks = burst;
            out[n].gap_ticks = gaps[b & 0x03u]; /* LSB-first dibits */
            b = (uint8_t)(b >> 2);
            n++;
        }
    }

    /* Closing burst. The trailing low is well-defined rather than zero (a zero
     * RMT duration is an end marker), and is followed by at least 500 us of
     * idle before the next frame, so its exact length is not critical. */
    out[n].burst_ticks = burst;
    out[n].gap_ticks =
        (uint16_t)ESL_PP4_TICKS(ESL_PP4_TAIL_GAP_CYCLES, resolution_hz);
    n++;

    return n;
}
