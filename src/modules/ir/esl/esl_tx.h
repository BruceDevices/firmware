/* ESL transmit sequences.
 *
 * Builds and orders the frames for an image upload. The radio is injected as a
 * set of callbacks so the sequence logic can be unit-tested on the host with a
 * recording fake instead of real hardware. */
#pragma once

/* Via esl_proto.h, never the vendored header directly: that keeps the vendor
 * symbols' linkage independent of include order in C++ callers. */
#include "esl_proto.h"

/* Repeat counts are load-bearing: they are what makes a tag latch. Values come
 * from TagTinker PR #53. Remember repeats = N means N+1 transmissions. */
#define ESL_COLOR26_WAKE_REPEATS 400u
#define ESL_COLOR26_STAGE_REPEATS 1u
#define ESL_GENERIC_PING_REPEATS 80u
#define ESL_GENERIC_PARAM_REPEATS 15u
#define ESL_GENERIC_DATA_REPEATS 2u /* upstream default; Settings exposes 1-10 (M2) */
#define ESL_GENERIC_REFRESH_REPEATS 20u

#define ESL_SETTLE_MS 50u        /* between stages, both families */
#define ESL_FRAME_DELAY_UNITS 1u /* 1 unit = 500 us between repeats */

/* Data-frame pacing, as (every_n, ms): pause ms after every every_n-th data
 * frame. The two families differ and must not share a policy — Color 2.6
 * pauses 50 ms after every frame, generic pauses 1 ms after every 32nd. */
#define ESL_COLOR26_DATA_PACE_EVERY 1u
#define ESL_COLOR26_DATA_PACE_MS 50u
#define ESL_GENERIC_DATA_PACE_EVERY 32u
#define ESL_GENERIC_DATA_PACE_MS 1u

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Required. Sends one frame, repeated repeats+1 times. */
    bool (*send)(void *ctx, const uint8_t *frame, size_t len, uint16_t repeats,
                 uint8_t delay);
    /* Optional. Blocking settle between stages. */
    void (*settle_ms)(void *ctx, uint32_t ms);
    /* Optional. Returning true stops the sequence. */
    bool (*aborted)(void *ctx);
    /* Optional. done/total step counters for a progress bar. */
    void (*progress)(void *ctx, size_t done, size_t total);
    void *ctx;
} EslTxOps;

/* Total frames a sequence will send: data frames plus the three fixed stages. */
size_t esl_tx_step_count(const TagTinkerImagePayload *payload);

/* SmartTAG Color 2.6 (type 1626): wake -> param(152x296) -> data -> refresh.
 * The page is remapped so the image never lands on the barcode page. */
bool esl_tx_send_color26(const EslTxOps *ops, const uint8_t plid[4],
                         const TagTinkerImagePayload *payload, uint8_t page);

/* Generic dot-matrix tags: ping -> param -> data -> refresh. The page is passed
 * straight through (only Color 2.6 remaps). data_repeats mirrors upstream's
 * app->data_frame_repeats; pass ESL_GENERIC_DATA_REPEATS for the default.
 * NOTE: unverified against real hardware; the project has no generic tag. */
bool esl_tx_send_generic(const EslTxOps *ops, const uint8_t plid[4],
                         const TagTinkerImagePayload *payload, uint8_t page,
                         uint16_t width, uint16_t height, uint16_t pos_x,
                         uint16_t pos_y, uint16_t data_repeats);

#ifdef __cplusplus
}
#endif
