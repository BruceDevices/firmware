/* PP4 symbol builder.
 *
 * TagTinker's IR line code sends two bits per symbol: a fixed carrier burst
 * followed by a gap whose length encodes the dibit value. This file turns
 * frame bytes into (burst, gap) tick pairs and has no hardware dependencies,
 * so it can be unit-tested on the host. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* The upstream Flipper driver expresses its timings as CPU cycle counts at
 * 64 MHz. Those counts stay the single source of truth; tick values for any
 * RMT resolution are derived from them so no converted number is hardcoded. */
#define ESL_PP4_SRC_CLOCK_HZ 64000000u
#define ESL_PP4_BURST_CYCLES 2581u
#define ESL_PP4_TAIL_GAP_CYCLES 3871u
#define ESL_PP4_DIBITS_PER_BYTE 4u
#define ESL_PP4_MAX_FRAME_LEN 255u

#define ESL_PP4_TICKS(cycles, res_hz)                                          \
    ((uint32_t)(((uint64_t)(cycles) * (uint64_t)(res_hz)) /                    \
                (uint64_t)ESL_PP4_SRC_CLOCK_HZ))

/* Gap cycle counts indexed by the raw 2-bit dibit value (0..3). */
extern const uint32_t esl_pp4_gap_cycles[4];

typedef struct {
    uint16_t burst_ticks; /* carrier on */
    uint16_t gap_ticks;   /* carrier off */
} EslPp4Symbol;

/* Symbols needed for a frame: four per byte plus one closing burst.
 * Returns 0 if frame_len is 0 or above ESL_PP4_MAX_FRAME_LEN. */
size_t esl_pp4_symbol_count(size_t frame_len);

/* Encodes frame into out. Returns the number of symbols written, or 0 on any
 * invalid argument or insufficient out_cap. */
size_t esl_pp4_encode(const uint8_t *frame, size_t len, uint32_t resolution_hz,
                      EslPp4Symbol *out, size_t out_cap);

#ifdef __cplusplus
}
#endif
