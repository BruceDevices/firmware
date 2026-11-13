#include "esl_tx.h"
#include "test_util.h"
#include <stdlib.h>

#define MAX_REC 64

typedef struct {
    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    size_t len;
    uint16_t repeats;
    uint8_t delay;
} Rec;

#define MAX_SETTLE 128

typedef struct {
    Rec rec[MAX_REC];
    size_t count;
    uint32_t settle[MAX_SETTLE]; /* ordered log of settle durations */
    size_t settle_calls;
    size_t progress_calls;
    size_t last_done;
    size_t last_total;
    int abort_after; /* -1 = never */
    size_t fail_at;  /* SIZE_MAX = never */
} Fake;

static bool fake_send(void *ctx, const uint8_t *f, size_t len, uint16_t repeats,
                      uint8_t delay) {
    Fake *k = (Fake *)ctx;
    if (k->count == k->fail_at) return false;
    if (k->count < MAX_REC) {
        memcpy(k->rec[k->count].frame, f, len);
        k->rec[k->count].len = len;
        k->rec[k->count].repeats = repeats;
        k->rec[k->count].delay = delay;
    }
    k->count++;
    return true;
}

static void fake_settle(void *ctx, uint32_t ms) {
    Fake *k = (Fake *)ctx;
    if (k->settle_calls < MAX_SETTLE) k->settle[k->settle_calls] = ms;
    k->settle_calls++;
}

static bool fake_aborted(void *ctx) {
    Fake *k = (Fake *)ctx;
    return k->abort_after >= 0 && k->count >= (size_t)k->abort_after;
}

static void fake_progress(void *ctx, size_t done, size_t total) {
    Fake *k = (Fake *)ctx;
    k->progress_calls++;
    k->last_done = done;
    k->last_total = total;
}

static void fake_init(Fake *k, EslTxOps *ops) {
    memset(k, 0, sizeof(*k));
    k->abort_after = -1;
    k->fail_at = (size_t)-1;
    ops->send = fake_send;
    ops->settle_ms = fake_settle;
    ops->aborted = fake_aborted;
    ops->progress = fake_progress;
    ops->ctx = k;
}

/* Payload of exactly 3 data frames, with recognisable content. */
static void payload_init(TagTinkerImagePayload *p, size_t frames) {
    p->byte_count = frames * TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME;
    p->data = (uint8_t *)malloc(p->byte_count);
    for (size_t i = 0; i < p->byte_count; i++) p->data[i] = (uint8_t)i;
    p->comp_type = 2u;
}

static const uint8_t PLID[4] = {0x10, 0x06, 0x9E, 0x40};

static void test_color26_sequence(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    payload_init(&p, 3);

    CHECK(esl_tx_send_color26(&ops, PLID, &p, 0));

    /* wake + param + 3 data + refresh */
    CHECK_EQ(k.count, 6);

    /* Wake: cmd 0x17, repeated 400. */
    CHECK_EQ(k.rec[0].repeats, ESL_COLOR26_WAKE_REPEATS);
    CHECK_EQ(k.rec[0].repeats, 400);
    CHECK_EQ(k.rec[0].frame[5], 0x17);
    CHECK_EQ(k.rec[0].len, 34);

    /* Param: MCU 0x05, wire dims 152x296, page 0 reaches the wire
     * verbatim (the defect was a second resolve that rewrote 0 to 2). */
    CHECK_EQ(k.rec[1].frame[9], 0x05);
    CHECK_EQ(k.rec[1].frame[14], 0); /* explicit page 0 is not remapped */
    CHECK_EQ(k.rec[1].frame[15], 0x00);
    CHECK_EQ(k.rec[1].frame[16], 0x98); /* 152 */
    CHECK_EQ(k.rec[1].frame[17], 0x01);
    CHECK_EQ(k.rec[1].frame[18], 0x28); /* 296 */
    CHECK_EQ(k.rec[1].repeats, ESL_COLOR26_STAGE_REPEATS);
    CHECK_EQ(k.rec[1].repeats, 1);

    /* Data frames: MCU 0x20, ascending index, repeats 1. */
    for (int i = 0; i < 3; i++) {
        CHECK_EQ(k.rec[2 + i].frame[9], 0x20);
        CHECK_EQ(k.rec[2 + i].frame[10], 0x00);
        CHECK_EQ(k.rec[2 + i].frame[11], (uint8_t)i);
        CHECK_EQ(k.rec[2 + i].repeats, 1);
    }

    /* Refresh: MCU 0x01, repeats 1. */
    CHECK_EQ(k.rec[5].frame[9], 0x01);
    CHECK_EQ(k.rec[5].repeats, 1);

    /* Every frame uses the 500 us inter-repeat delay unit. */
    for (size_t i = 0; i < k.count; i++)
        CHECK_EQ(k.rec[i].delay, ESL_FRAME_DELAY_UNITS);

    /* Color 2.6 pacing: 50 ms after wake, after param, after each data frame
     * except the last, then once more before refresh. */
    CHECK_EQ(k.settle_calls, 5);
    for (size_t i = 0; i < 5; i++) CHECK_EQ(k.settle[i], 50);

    CHECK_EQ(k.last_done, k.last_total);
    CHECK_EQ(k.last_total, esl_tx_step_count(&p));
    CHECK_EQ(k.last_total, 6);

    free(p.data);
}

static void test_explicit_page_preserved(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    payload_init(&p, 1);

    CHECK(esl_tx_send_color26(&ops, PLID, &p, 5));
    CHECK_EQ(k.rec[1].frame[14], 5); /* explicit pages 2-7 pass through */
    free(p.data);
}

static void test_explicit_page_one_preserved(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    payload_init(&p, 1);

    CHECK(esl_tx_send_color26(&ops, PLID, &p, 1));
    CHECK_EQ(k.rec[1].frame[14], 1); /* resolve_page used to rewrite 1 to 2 */
    free(p.data);
}

static void test_generic_sequence(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    /* 40 data frames so the every-32nd pacing rule is actually exercised. */
    payload_init(&p, 40);

    CHECK(esl_tx_send_generic(&ops, PLID, &p, 0, 296, 128, 0, 0,
                              ESL_GENERIC_DATA_REPEATS));

    /* ping + param + 40 data + refresh */
    CHECK_EQ(k.count, 43);
    CHECK_EQ(k.rec[0].frame[5], 0x97); /* ping */
    CHECK_EQ(k.rec[0].repeats, ESL_GENERIC_PING_REPEATS);
    CHECK_EQ(k.rec[0].repeats, 80);
    CHECK_EQ(k.rec[1].frame[9], 0x05); /* param */
    CHECK_EQ(k.rec[1].repeats, ESL_GENERIC_PARAM_REPEATS);
    CHECK_EQ(k.rec[1].repeats, 15);
    /* Page 0 is remapping-sensitive (resolve_page would emit 2). */
    CHECK_EQ(k.rec[1].frame[14], 0);
    CHECK_EQ(k.rec[2].repeats, ESL_GENERIC_DATA_REPEATS);
    CHECK_EQ(k.rec[2].repeats, 2);
    CHECK_EQ(k.rec[42].frame[9], 0x01); /* refresh */
    CHECK_EQ(k.rec[42].repeats, ESL_GENERIC_REFRESH_REPEATS);
    CHECK_EQ(k.rec[42].repeats, 20);

    /* Every frame uses the 500 us inter-repeat delay unit. */
    for (size_t i = 0; i < k.count; i++)
        CHECK_EQ(k.rec[i].delay, ESL_FRAME_DELAY_UNITS);

    /* Generic pacing is NOT the Color 2.6 policy: 50 ms after ping, 50 ms
     * after param, a single 1 ms pause after the 32nd data frame, then 50 ms
     * before refresh. Four settles total, not one per data frame. */
    CHECK_EQ(k.settle_calls, 4);
    CHECK_EQ(k.settle[0], 50);
    CHECK_EQ(k.settle[1], 50);
    CHECK_EQ(k.settle[2], ESL_GENERIC_DATA_PACE_MS);
    CHECK_EQ(k.settle[2], 1);
    CHECK_EQ(k.settle[3], 50);

    free(p.data);
}

/* data_frame_repeats is a Settings knob upstream (1-10), so it must be
 * threaded through rather than baked in. */
static void test_generic_honours_data_repeats(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    payload_init(&p, 2);

    CHECK(esl_tx_send_generic(&ops, PLID, &p, 0, 296, 128, 0, 0, 7));
    CHECK_EQ(k.rec[2].repeats, 7);
    CHECK_EQ(k.rec[3].repeats, 7);
    free(p.data);
}

static void test_abort_stops_early(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    payload_init(&p, 5);
    k.abort_after = 2; /* abort once wake + param are out */

    CHECK(!esl_tx_send_color26(&ops, PLID, &p, 0));
    CHECK_EQ(k.count, 2); /* no data frames sent */
    free(p.data);
}

static void test_send_failure_propagates(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    payload_init(&p, 5);
    k.fail_at = 3;

    CHECK(!esl_tx_send_color26(&ops, PLID, &p, 0));
    /* fake_send returns false at count==fail_at before incrementing, so
     * wake + param + first data are recorded and the second data send fails. */
    CHECK_EQ(k.count, 3);
    /* Refresh (MCU 0x01) was never recorded as a successful final frame. */
    CHECK(k.rec[2].frame[9] != 0x01);
    free(p.data);
}

static void test_guards(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    payload_init(&p, 1);

    CHECK(!esl_tx_send_color26(NULL, PLID, &p, 0));
    CHECK(!esl_tx_send_color26(&ops, NULL, &p, 0));
    CHECK(!esl_tx_send_color26(&ops, PLID, NULL, 0));

    TagTinkerImagePayload empty = {NULL, 0u, 0u};
    CHECK(!esl_tx_send_color26(&ops, PLID, &empty, 0));

    /* Optional callbacks may be NULL. */
    EslTxOps minimal = {fake_send, NULL, NULL, NULL, &k};
    k.count = 0;
    CHECK(esl_tx_send_color26(&minimal, PLID, &p, 0));

    free(p.data);
}

static void test_led_test_sequence(void) {
    Fake k;
    EslTxOps ops;
    fake_init(&k, &ops);

    uint8_t want_ping[TAGTINKER_MAX_FRAME_SIZE];
    const size_t ping_len = tagtinker_make_ping_frame(want_ping, PLID);
    const uint8_t blink[6] = {0x06, 0x49, 0x00, 0x00, 0x00, 0x05};
    uint8_t want_blink[TAGTINKER_MAX_FRAME_SIZE];
    const size_t blink_len =
        tagtinker_make_addressed_frame(want_blink, PLID, blink, sizeof(blink));

    CHECK(esl_tx_send_led_test(&ops, PLID));
    CHECK_EQ(k.count, 2);

    CHECK_EQ(k.rec[0].repeats, ESL_LED_PING_REPEATS);
    CHECK_EQ(k.rec[0].repeats, 160);
    CHECK_EQ(k.rec[0].len, ping_len);
    CHECK_MEM(k.rec[0].frame, want_ping, ping_len);

    CHECK_EQ(k.rec[1].repeats, ESL_LED_BLINK_REPEATS);
    CHECK_EQ(k.rec[1].repeats, 80);
    CHECK_EQ(k.rec[1].len, blink_len);
    CHECK_MEM(k.rec[1].frame, want_blink, blink_len);

    for (size_t i = 0; i < k.count; i++)
        CHECK_EQ(k.rec[i].delay, ESL_FRAME_DELAY_UNITS);

    /* Flipper queues both frames then transmits — no inter-frame settle. */
    CHECK_EQ(k.settle_calls, 0);
    CHECK_EQ(k.progress_calls, 2);
    CHECK_EQ(k.last_done, 2);
    CHECK_EQ(k.last_total, 2);
}

static void test_led_test_guards(void) {
    Fake k;
    EslTxOps ops;
    fake_init(&k, &ops);

    CHECK(!esl_tx_send_led_test(NULL, PLID));
    CHECK(!esl_tx_send_led_test(&ops, NULL));
    CHECK_EQ(k.count, 0);
}

static void test_led_test_abort_after_first(void) {
    Fake k;
    EslTxOps ops;
    fake_init(&k, &ops);
    k.abort_after = 1; /* abort once ping is out */

    CHECK(!esl_tx_send_led_test(&ops, PLID));
    CHECK_EQ(k.count, 1); /* blink not sent */
}

static void test_broadcast_page_golden(void) {
    Fake k;
    EslTxOps ops;
    fake_init(&k, &ops);

    uint8_t want[TAGTINKER_MAX_FRAME_SIZE];
    const size_t want_len =
        tagtinker_build_broadcast_page_frame(want, 3, true, 15);

    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    const size_t len =
        tagtinker_build_broadcast_page_frame(frame, 3, true, 15);

    CHECK(esl_tx_send_raw(&ops, frame, len, 200, false));
    CHECK_EQ(k.count, 1); /* spam=false sends once */
    CHECK_EQ(k.rec[0].repeats, 200);
    CHECK_EQ(k.rec[0].len, want_len);
    CHECK_MEM(k.rec[0].frame, want, want_len);
    CHECK_EQ(k.rec[0].delay, ESL_FRAME_DELAY_UNITS);
    CHECK_EQ(k.settle_calls, 0);
}

static void test_broadcast_debug_golden(void) {
    Fake k;
    EslTxOps ops;
    fake_init(&k, &ops);

    uint8_t want[TAGTINKER_MAX_FRAME_SIZE];
    const size_t want_len = tagtinker_build_broadcast_debug_frame(want);

    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    const size_t len = tagtinker_build_broadcast_debug_frame(frame);

    CHECK(esl_tx_send_raw(&ops, frame, len, 200, false));
    CHECK_EQ(k.count, 1);
    CHECK_EQ(k.rec[0].repeats, 200);
    CHECK_EQ(k.rec[0].len, want_len);
    CHECK_MEM(k.rec[0].frame, want, want_len);
}

static void test_send_raw_spam_stops_on_abort(void) {
    Fake k;
    EslTxOps ops;
    fake_init(&k, &ops);
    k.abort_after = 2; /* abort after two recorded sends */

    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    const size_t len = tagtinker_build_broadcast_debug_frame(frame);

    CHECK(!esl_tx_send_raw(&ops, frame, len, 200, true));
    CHECK_EQ(k.count, 2);
}

static void test_send_raw_guards(void) {
    Fake k;
    EslTxOps ops;
    fake_init(&k, &ops);

    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    const size_t len = tagtinker_build_broadcast_debug_frame(frame);

    CHECK(!esl_tx_send_raw(NULL, frame, len, 200, false));
    CHECK(!esl_tx_send_raw(&ops, NULL, len, 200, false));
    CHECK(!esl_tx_send_raw(&ops, frame, 0, 200, false));
    CHECK_EQ(k.count, 0);
}

int main(void) {
    test_color26_sequence();
    test_explicit_page_preserved();
    test_explicit_page_one_preserved();
    test_generic_sequence();
    test_generic_honours_data_repeats();
    test_abort_stops_early();
    test_send_failure_propagates();
    test_guards();
    test_led_test_sequence();
    test_led_test_guards();
    test_led_test_abort_after_first();
    test_broadcast_page_golden();
    test_broadcast_debug_golden();
    test_send_raw_spam_stops_on_abort();
    test_send_raw_guards();
    TEST_REPORT("test_tx");
}
