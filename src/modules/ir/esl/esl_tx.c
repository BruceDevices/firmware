#include "esl_tx.h"

static bool tx_ok(const EslTxOps *ops) {
    return ops != NULL && ops->send != NULL;
}

static bool tx_aborted(const EslTxOps *ops) {
    return ops->aborted != NULL && ops->aborted(ops->ctx);
}

static bool tx_frame(const EslTxOps *ops, const uint8_t *frame, size_t len,
                     uint16_t repeats) {
    if (tx_aborted(ops)) return false;
    return ops->send(ops->ctx, frame, len, repeats, ESL_FRAME_DELAY_UNITS);
}

static void tx_settle_ms(const EslTxOps *ops, uint32_t ms) {
    if (ops->settle_ms != NULL) ops->settle_ms(ops->ctx, ms);
}

static void tx_settle(const EslTxOps *ops) {
    tx_settle_ms(ops, ESL_SETTLE_MS);
}

static void tx_step(const EslTxOps *ops, size_t *done, size_t total) {
    (*done)++;
    if (ops->progress != NULL) ops->progress(ops->ctx, *done, total);
}

static size_t data_frame_count(const TagTinkerImagePayload *payload) {
    return payload->byte_count / TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME;
}

size_t esl_tx_step_count(const TagTinkerImagePayload *payload) {
    if (payload == NULL) return 0u;
    return data_frame_count(payload) + 3u; /* wake/ping + param + refresh */
}

static bool args_ok(const EslTxOps *ops, const uint8_t plid[4],
                    const TagTinkerImagePayload *payload) {
    return tx_ok(ops) && plid != NULL && payload != NULL &&
           payload->data != NULL && payload->byte_count > 0u &&
           data_frame_count(payload) > 0u;
}

/* Repeat counts and data-frame pacing for one tag family. */
typedef struct {
    uint16_t param_repeats;
    uint16_t data_repeats;
    uint16_t refresh_repeats;
    uint16_t data_pace_every; /* pause after every Nth data frame */
    uint32_t data_pace_ms;
} EslTxProfile;

/* Sends the param frame, all data frames, then the refresh frame. The stage
 * order is common to both families; the repeat counts and the data pacing come
 * from the caller's profile, because upstream paces the two families
 * differently. */
static bool tx_payload_stages(const EslTxOps *ops, const uint8_t plid[4],
                              const TagTinkerImagePayload *payload,
                              uint8_t page, uint16_t width, uint16_t height,
                              uint16_t pos_x, uint16_t pos_y,
                              const EslTxProfile *prof, size_t *done,
                              size_t total) {
    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];

    size_t len = tagtinker_make_image_param_frame(
        frame, plid, (uint16_t)payload->byte_count, payload->comp_type, page,
        width, height, pos_x, pos_y);
    if (!tx_frame(ops, frame, len, prof->param_repeats)) return false;
    tx_step(ops, done, total);
    tx_settle(ops);

    const size_t frames = data_frame_count(payload);
    const uint16_t every =
        (prof->data_pace_every == 0u) ? 1u : prof->data_pace_every;
    for (size_t i = 0u; i < frames; i++) {
        len = tagtinker_make_image_data_frame(
            frame, plid, (uint16_t)i,
            &payload->data[i * TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME]);
        if (!tx_frame(ops, frame, len, prof->data_repeats)) return false;
        tx_step(ops, done, total);
        if (((i + 1u) % every) == 0u && (i + 1u) < frames) {
            tx_settle_ms(ops, prof->data_pace_ms);
        }
    }
    tx_settle(ops);

    len = tagtinker_make_refresh_frame(frame, plid);
    if (!tx_frame(ops, frame, len, prof->refresh_repeats)) return false;
    tx_step(ops, done, total);
    return true;
}

bool esl_tx_send_color26(const EslTxOps *ops, const uint8_t plid[4],
                         const TagTinkerImagePayload *payload, uint8_t page) {
    if (!args_ok(ops, plid, payload)) return false;

    const size_t total = esl_tx_step_count(payload);
    size_t done = 0u;
    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];

    const size_t len = tagtinker_make_wake_frame(frame, plid);
    if (!tx_frame(ops, frame, len, ESL_COLOR26_WAKE_REPEATS)) return false;
    tx_step(ops, &done, total);
    tx_settle(ops);

    const EslTxProfile prof = {
        .param_repeats = ESL_COLOR26_STAGE_REPEATS,
        .data_repeats = ESL_COLOR26_STAGE_REPEATS,
        .refresh_repeats = ESL_COLOR26_STAGE_REPEATS,
        .data_pace_every = ESL_COLOR26_DATA_PACE_EVERY,
        .data_pace_ms = ESL_COLOR26_DATA_PACE_MS,
    };

    /* Page arrives already-resolved-as-a-default from the caller. An explicit
     * user pick must reach the wire unmodified — do not resolve again. */
    return tx_payload_stages(ops, plid, payload, page, TAGTINKER_COLOR26_WIRE_W,
                             TAGTINKER_COLOR26_WIRE_H, 0u, 0u, &prof, &done,
                             total);
}

bool esl_tx_send_generic(const EslTxOps *ops, const uint8_t plid[4],
                         const TagTinkerImagePayload *payload, uint8_t page,
                         uint16_t width, uint16_t height, uint16_t pos_x,
                         uint16_t pos_y, uint16_t data_repeats) {
    if (!args_ok(ops, plid, payload)) return false;

    const size_t total = esl_tx_step_count(payload);
    size_t done = 0u;
    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];

    const size_t len = tagtinker_make_ping_frame(frame, plid);
    if (!tx_frame(ops, frame, len, ESL_GENERIC_PING_REPEATS)) return false;
    tx_step(ops, &done, total);
    tx_settle(ops);

    const EslTxProfile prof = {
        .param_repeats = ESL_GENERIC_PARAM_REPEATS,
        .data_repeats = data_repeats,
        .refresh_repeats = ESL_GENERIC_REFRESH_REPEATS,
        .data_pace_every = ESL_GENERIC_DATA_PACE_EVERY,
        .data_pace_ms = ESL_GENERIC_DATA_PACE_MS,
    };

    return tx_payload_stages(ops, plid, payload, page, width, height, pos_x,
                             pos_y, &prof, &done, total);
}
