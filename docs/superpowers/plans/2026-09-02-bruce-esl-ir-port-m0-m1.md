# Bruce ESL IR Port (M0 + M1) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port TagTinker's infrared ESL transmit flow into Bruce ESP32 firmware so a prepared BMP on the SD card can be pushed to a SmartTAG Color 2.6 tag from the Bruce IR menu.

**Architecture:** The wire protocol is vendored byte-identical from TagTinker PR #53 and compiled as C. All decision logic (PP4 symbol timing, frame sequencing, BMP pixel mapping) lives in pure C files with no hardware dependencies, so it is unit-tested on the host with `cc`. Hardware touches only two thin C++ files: an ESP-IDF v5 RMT driver that turns PP4 symbols into a carrier-modulated waveform, and a Bruce UI layer.

**Tech Stack:** C11 (protocol + pure logic), C++ (Arduino-ESP32 3.3.x / ESP-IDF v5 for RMT + UI), PlatformIO (`lilygo-t-embed-cc1101` env), `make` + `cc` for host tests.

**Design doc:** `docs/superpowers/specs/2026-09-02-tagtinker-esl-ir-port-design.md`

## Global Constraints

- `src/modules/ir/esl/protocol/tagtinker_proto.{c,h}` are **VENDORED — never edit**. They must stay byte-identical to `git show pr-53:protocol/tagtinker_proto.c` in the TagTinker clone at `../TagTinker`.
- The vendored `tagtinker_proto.c` contains `#include "../tagtinker_app.h"`. That path **must** resolve to our shim, which is why the vendored files live in an `esl/protocol/` subdirectory with the shim at `esl/tagtinker_app.h`.
- C++ translation units must include the vendored header through `esl_proto.h`, which wraps it in `extern "C"`. Including `protocol/tagtinker_proto.h` directly from C++ will fail to link.
- **Every C header we author under `esl/` carries its own `extern "C"` guard** — `esl_pp4.h`, `esl_tx.h`, `esl_bmp.h`, `esl_ir_driver.h`. Their `.c` files compile as C while `esl_ir_driver.cpp` and `esl_app.cpp` compile as C++; without the guard the C++ side mangles the names and the link fails. Only the *vendored* header is exempt, because it must not be edited — that one is wrapped externally by `esl_proto.h`.
- **Authored headers reach protocol symbols via `esl_proto.h`, never `protocol/tagtinker_proto.h` directly.** Because the vendored header has `#pragma once`, whichever include wins first fixes its linkage for the whole translation unit. If an authored header pulls in the vendored header raw and a C++ file includes that header before `esl_proto.h`, the wrapper becomes a no-op and the vendor symbols get C++ linkage — a link failure that depends on include order. Routing every authored header through `esl_proto.h` makes linkage order-independent. `esl_proto.h` is safe to include from C, where its guards collapse to a plain include.
- **Do not call** `tagtinker_make_mcu_frame`, `tagtinker_rle_compress`, or `tagtinker_build_image_sequence`. They are declared in the vendored header but have no implementation in PR #53 — calling them is a link error.
- PP4 tick values are **derived at compile time** from the Flipper's 64 MHz cycle counts via `ESL_PP4_TICKS`. Never hardcode converted tick numbers.
- Carrier: `frequency_hz = 1250000`, `duty_cycle = 0.49f`, `flags.polarity_active_low = false`, `flags.always_on = false`.
- RMT: `resolution_hz = 80000000`, `rmt_new_copy_encoder`, and `rmt_transmit_config_t.flags.eot_level = 0`.
- Always `digitalWrite(bruceConfigPins.irTx, LED_OFF)` on teardown so the IR LED never idles energised.
- **Encode-then-transmit invariant:** the complete `TagTinkerImagePayload` must be in RAM before the IR pin is claimed. No SD reads may occur after `setup_ir_pin()`.
- **This is a 1:1 port.** Bruce must behave as TagTinker behaves. Never substitute a cleaner-looking constant, threshold, or policy for the upstream one, and never hardcode something upstream obtains at runtime. Every intentional divergence is already listed in the spec's "Deliberate adaptations" table — if your change is not in that table, it must match upstream.
- **Nothing is hardcoded that upstream asks the user for.** The tag barcode always comes from a prompt. There is no compiled-in barcode, PLID, or profile anywhere in the port.
- Frame repeat counts are fixed: Color 2.6 = wake `400`, param/data/refresh `1`; generic = ping `80`, param `15`, data `data_frame_repeats` (default `2`), refresh `20`. `delay` is always `1` (500 µs). Remember `repeats = N` means **N+1** transmissions.
- **Data-frame pacing differs per family and must not be shared:** Color 2.6 waits **50 ms after every data frame**; generic waits **1 ms after every 32nd data frame**. Stage settles are 50 ms for both.
- BMP header parsing accepts bpp **1, 2, 24, 32** (as `tx_bmp_open` does). Rejecting non-1/2 is the Color 2.6 *send path's* job, not the parser's.
- Color 2.6 BMP file cap is **24576** bytes (`TX_COLOR26_BMP_MAX`). Do not raise it.
- The image page is **user-selectable 0–7**, mirroring `scene_image_options`. Position (0,0), compression (auto RLE) and frame-repeat (×2) stay fixed at upstream's defaults — upstream deliberately exposes only the page.
- **All dot-matrix profiles are supported**, not just Color 2.6. Segment-kind profiles are rejected (they have no image page).
- **Do not add chunked streaming to the image path.** `tx_should_send_full_job`, `tx_pick_chunk_height` and `tx_chunk_settle_delay_ms` are used only by the text path upstream; `tx_stream_bmp_image` never chunks. Chunking is M2 work.
- Use `ps_malloc` for BMP and payload buffers.
- Host tests must pass with `-std=c11 -Wall -Wextra -Werror` (our code) and `-Wall -Wextra` (vendored code).
- Firmware build target: `lilygo-t-embed-cc1101`.
- Verification hardware: LilyGo T-Embed CC1101 (ESP32-S3), IR TX default GPIO 2, `LED_ON = HIGH`. Tag barcode `A4165420155216265` → PLID `10 06 9E 40`, type `1626`.

---

## File Structure

| File | Responsibility |
|------|----------------|
| `src/modules/ir/esl/tagtinker_app.h` | Compat shim. Supplies `COUNT_OF` so the vendored `.c` compiles unchanged. |
| `src/modules/ir/esl/protocol/tagtinker_proto.h` | VENDORED protocol header (frames, profiles, RLE, transpose helpers). |
| `src/modules/ir/esl/protocol/tagtinker_proto.c` | VENDORED protocol implementation. |
| `src/modules/ir/esl/esl_proto.h` | `extern "C"` wrapper so C++ can use the vendored header. |
| `src/modules/ir/esl/esl_pp4.h/.c` | Pure PP4 symbol builder: frame bytes → burst/gap tick pairs. Host-tested. |
| `src/modules/ir/esl/esl_tx.h/.c` | Pure frame sequencing with injectable ops (send/settle/abort/progress). Host-tested. |
| `src/modules/ir/esl/esl_bmp.h/.c` | Pure BMP header parse + pixel callbacks (Color 2.6 transpose, generic rescale). Host-tested. |
| `src/modules/ir/esl/esl_ir_driver.h/.cpp` | ESP32 RMT hardware shim. Not host-testable; verified by scope + tag. |
| `src/modules/ir/esl/esl_app.h/.cpp` | Bruce UI: barcode entry, BMP picker, progress, abort, error paths. |
| `src/core/menu_items/IRMenu.cpp` | Adds the `ESL Image` menu entry. |
| `tools/esl_host_tests/Makefile` | Host test runner (outside `src/`, so never compiled into firmware). |
| `tools/esl_host_tests/test_util.h` | Dependency-free assert macros. |
| `tools/esl_host_tests/test_proto.c` | Golden-vector tests for the vendored protocol. |
| `tools/esl_host_tests/test_pp4.c` | PP4 timing/ordering tests. |
| `tools/esl_host_tests/test_tx.c` | Frame-sequence tests with a recording fake. |
| `tools/esl_host_tests/test_bmp.c` | BMP parse + pixel mapping tests. |

`tools/` sits outside `src/`, and Bruce's `build_src_filter` is `+<*>` relative to `src/`, so host test code is never compiled into the firmware. New files under `src/modules/ir/esl/` are picked up automatically — no build file changes needed.

---

## Task 1: Vendor the protocol core and stand up host tests

**Files:**
- Create: `src/modules/ir/esl/tagtinker_app.h`
- Create: `src/modules/ir/esl/protocol/tagtinker_proto.h` (copied)
- Create: `src/modules/ir/esl/protocol/tagtinker_proto.c` (copied)
- Create: `src/modules/ir/esl/esl_proto.h`
- Create: `tools/esl_host_tests/Makefile`
- Create: `tools/esl_host_tests/test_util.h`
- Test: `tools/esl_host_tests/test_proto.c`

**Interfaces:**
- Consumes: nothing (first task).
- Produces: the vendored protocol API used by every later task — `tagtinker_barcode_to_plid(const char*, uint8_t[4]) -> bool`, `tagtinker_barcode_to_profile(const char*, TagTinkerTagProfile*) -> bool`, `tagtinker_crc16(const uint8_t*, size_t) -> uint16_t`, `tagtinker_make_wake_frame/ping_frame/refresh_frame(uint8_t*, const uint8_t[4]) -> size_t`, `tagtinker_make_image_param_frame(uint8_t*, const uint8_t[4], uint16_t byte_count, uint8_t comp_type, uint8_t page, uint16_t w, uint16_t h, uint16_t x, uint16_t y) -> size_t`, `tagtinker_make_image_data_frame(uint8_t*, const uint8_t[4], uint16_t index, const uint8_t[20]) -> size_t`, `tagtinker_encode_fn_payload(TagTinkerPixelAtFn, void*, size_t, TagTinkerCompressionMode, TagTinkerImagePayload*) -> bool`, `tagtinker_free_image_payload(TagTinkerImagePayload*)`, plus `TAGTINKER_COLOR26_WIRE_W/H` (152/296), `TAGTINKER_COLOR26_GLASS_W/H` (296/152), `TAGTINKER_MAX_FRAME_SIZE` (96), `TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME` (20), `tagtinker_color26_proto_to_glass`, `tagtinker_color26_resolve_page`, `tagtinker_profile_glass_size`. Also produces the host test harness (`make test`) that all later tasks extend.

- [ ] **Step 1: Create the test harness assert macros**

Create `tools/esl_host_tests/test_util.h`:

```c
#pragma once
#include <stdio.h>
#include <string.h>

static int esl_checks = 0;
static int esl_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        esl_checks++;                                                          \
        if (!(cond)) {                                                         \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                      \
    } while (0)

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        esl_checks++;                                                          \
        unsigned long long _a = (unsigned long long)(actual);                  \
        unsigned long long _e = (unsigned long long)(expected);                \
        if (_a != _e) {                                                        \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: %s (got %llu, want %llu)\n",          \
                    __FILE__, __LINE__, #actual, _a, _e);                      \
        }                                                                      \
    } while (0)

#define CHECK_STR(actual, expected)                                            \
    do {                                                                       \
        esl_checks++;                                                          \
        const char *_a = (actual);                                             \
        if (_a == NULL || strcmp(_a, (expected)) != 0) {                       \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: %s (got '%s', want '%s')\n",          \
                    __FILE__, __LINE__, #actual, _a ? _a : "(null)",           \
                    (expected));                                               \
        }                                                                      \
    } while (0)

#define CHECK_MEM(actual, expected, n)                                         \
    do {                                                                       \
        esl_checks++;                                                          \
        if (memcmp((actual), (expected), (size_t)(n)) != 0) {                  \
            esl_failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: bytes differ\n  got : ",              \
                    __FILE__, __LINE__);                                       \
            for (size_t _i = 0; _i < (size_t)(n); _i++)                        \
                fprintf(stderr, "%02X ",                                       \
                        ((const unsigned char *)(actual))[_i]);                \
            fprintf(stderr, "\n  want: ");                                     \
            for (size_t _i = 0; _i < (size_t)(n); _i++)                        \
                fprintf(stderr, "%02X ",                                       \
                        ((const unsigned char *)(expected))[_i]);              \
            fprintf(stderr, "\n");                                             \
        }                                                                      \
    } while (0)

#define TEST_REPORT(name)                                                      \
    do {                                                                       \
        printf("%s: %d checks, %d failures\n", (name), esl_checks,             \
               esl_failures);                                                  \
        return esl_failures == 0 ? 0 : 1;                                      \
    } while (0)
```

- [ ] **Step 2: Create the Makefile**

Create `tools/esl_host_tests/Makefile`. Vendored code is compiled without `-Werror` so a future compiler's new warning can never force an edit to a file we are forbidden to change:

```make
CC ?= cc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O0 -g
VENDOR_CFLAGS := -std=c11 -Wall -Wextra -O0 -g

ESL := ../../src/modules/ir/esl
INC := -I$(ESL) -I.

VENDOR_SRC := $(ESL)/protocol/tagtinker_proto.c
PURE_SRC := $(wildcard $(ESL)/esl_*.c)
TEST_SRC := $(wildcard test_*.c)
TESTS := $(TEST_SRC:.c=)

.PHONY: test clean
test: $(TESTS)
	@fail=0; \
	for t in $(TESTS); do printf '\n--- %s ---\n' "$$t"; ./$$t || fail=1; done; \
	if [ $$fail -ne 0 ]; then printf '\nSOME TESTS FAILED\n'; exit 1; fi; \
	printf '\nALL TESTS PASSED\n'

vendor.o: $(VENDOR_SRC)
	$(CC) $(VENDOR_CFLAGS) $(INC) -c $< -o $@

$(TESTS): %: %.c vendor.o $(PURE_SRC)
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(PURE_SRC) vendor.o

clean:
	rm -f $(TESTS) vendor.o
```

- [ ] **Step 3: Write the failing golden-vector test**

Create `tools/esl_host_tests/test_proto.c`. Every expected value here was produced by compiling the real PR #53 protocol on the host, so these are true golden vectors, not hand-derived guesses:

```c
#include "protocol/tagtinker_proto.h"
#include "test_util.h"

static const char *BARCODE = "A4165420155216265";

static uint8_t zero_pixel(size_t idx, void *ctx) {
    (void)idx; (void)ctx;
    return 0U;
}

static uint8_t alt_pixel(size_t idx, void *ctx) {
    (void)ctx;
    return (uint8_t)(idx & 1U);
}

static void test_barcode(void) {
    uint8_t plid[4] = {0};
    const uint8_t want[4] = {0x10, 0x06, 0x9E, 0x40};
    CHECK(tagtinker_barcode_to_plid(BARCODE, plid));
    CHECK_MEM(plid, want, 4);

    uint16_t type = 0;
    CHECK(tagtinker_barcode_to_type(BARCODE, &type));
    CHECK_EQ(type, 1626);

    /* Wrong length must be rejected. */
    CHECK(!tagtinker_barcode_to_plid("A416542015521626", plid));
    CHECK(!tagtinker_is_barcode_valid("too-short"));
    CHECK(tagtinker_is_barcode_valid(BARCODE));
}

static void test_profile(void) {
    TagTinkerTagProfile p;
    CHECK(tagtinker_barcode_to_profile(BARCODE, &p));
    CHECK_STR(p.model_name, "SmartTAG Color 2.6");
    CHECK_EQ(p.type_code, 1626);
    CHECK_EQ(p.width, 152);   /* wire dims live in the profile */
    CHECK_EQ(p.height, 296);
    CHECK_EQ(p.kind, TagTinkerTagKindDotMatrix);
    CHECK_EQ(p.color, TagTinkerTagColorRed);
    CHECK(p.known);

    uint16_t gw = 0, gh = 0;
    tagtinker_profile_glass_size(&p, &gw, &gh);
    CHECK_EQ(gw, 296);
    CHECK_EQ(gh, 152);
    CHECK(tagtinker_profile_needs_wh_swap(&p));
    CHECK(tagtinker_profile_uses_ui_page(&p));

    /* An unknown type code must fail rather than silently guess. */
    TagTinkerTagProfile unknown;
    CHECK(!tagtinker_barcode_to_profile("A4165420155299995", &unknown));
}

static void test_page_and_transpose(void) {
    CHECK_EQ(tagtinker_color26_resolve_page(0), 2);
    CHECK_EQ(tagtinker_color26_resolve_page(1), 2);
    CHECK_EQ(tagtinker_color26_resolve_page(2), 2);
    CHECK_EQ(tagtinker_color26_resolve_page(5), 5);
    CHECK_EQ(tagtinker_color26_resolve_page(9), 7);

    uint16_t bx = 0, by = 0;
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 0, 0, &bx, &by);
    CHECK_EQ(bx, 0);   CHECK_EQ(by, 151);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 0, 1, &bx, &by);
    CHECK_EQ(bx, 1);   CHECK_EQ(by, 151);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 1, 0, &bx, &by);
    CHECK_EQ(bx, 0);   CHECK_EQ(by, 150);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 151, 0, &bx, &by);
    CHECK_EQ(bx, 0);   CHECK_EQ(by, 0);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 151, 295, &bx, &by);
    CHECK_EQ(bx, 295); CHECK_EQ(by, 0);
}

static void test_crc(void) {
    const uint8_t vec[] = {0x85, 0x10, 0x06, 0x9E, 0x40, 0x17};
    CHECK_EQ(tagtinker_crc16(vec, sizeof(vec)), 0xD3D8);
}

static void test_frames(void) {
    uint8_t plid[4];
    tagtinker_barcode_to_plid(BARCODE, plid);
    uint8_t buf[TAGTINKER_MAX_FRAME_SIZE];

    const uint8_t want_wake[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x17, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x85, 0x7A};
    CHECK_EQ(tagtinker_make_wake_frame(buf, plid), 34);
    CHECK_MEM(buf, want_wake, 34);

    const uint8_t want_ping[32] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x97, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x6B, 0x81};
    CHECK_EQ(tagtinker_make_ping_frame(buf, plid), 32);
    CHECK_MEM(buf, want_ping, 32);

    const uint8_t want_refresh[30] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB9, 0xEB};
    CHECK_EQ(tagtinker_make_refresh_frame(buf, plid), 30);
    CHECK_MEM(buf, want_refresh, 30);

    /* byte_count=20, comp_type=2 (RLE), page=2, 152x296, pos 0,0 */
    const uint8_t want_param[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x05,
        0x00, 0x14, 0x00, 0x02, 0x02, 0x00, 0x98, 0x01, 0x28, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xBF, 0x64};
    CHECK_EQ(tagtinker_make_image_param_frame(
                 buf, plid, 20, 2, 2, TAGTINKER_COLOR26_WIRE_W,
                 TAGTINKER_COLOR26_WIRE_H, 0, 0),
             34);
    CHECK_MEM(buf, want_param, 34);

    uint8_t data20[TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME];
    for (int i = 0; i < 20; i++) data20[i] = (uint8_t)i;

    const uint8_t want_d0[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
        0x12, 0x13, 0x95, 0x20};
    CHECK_EQ(tagtinker_make_image_data_frame(buf, plid, 0, data20), 34);
    CHECK_MEM(buf, want_d0, 34);

    const uint8_t want_d7[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x07, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
        0x12, 0x13, 0x91, 0xA4};
    CHECK_EQ(tagtinker_make_image_data_frame(buf, plid, 7, data20), 34);
    CHECK_MEM(buf, want_d7, 34);

    /* Every frame must fit the protocol's own buffer bound. */
    CHECK(34 <= TAGTINKER_MAX_FRAME_SIZE);
}

static void test_encode(void) {
    TagTinkerImagePayload p;

    /* One long run of zeros: RLE wins, so auto picks comp_type 2.
     * Bits: [0] + 7 zeros + 8 count bits of 160 (10100000) = 00 A0. */
    const uint8_t want_zeros[20] = {0x00, 0xA0};
    CHECK(tagtinker_encode_fn_payload(zero_pixel, NULL, 160,
                                      TagTinkerCompressionAuto, &p));
    CHECK_EQ(p.comp_type, 2);
    CHECK_EQ(p.byte_count, 20);
    CHECK_MEM(p.data, want_zeros, 20);
    tagtinker_free_image_payload(&p);
    CHECK(p.data == NULL);

    /* Alternating pixels: RLE would be worse, so auto must fall back to raw. */
    uint8_t want_alt[20];
    memset(want_alt, 0x55, sizeof(want_alt));
    CHECK(tagtinker_encode_fn_payload(alt_pixel, NULL, 160,
                                      TagTinkerCompressionAuto, &p));
    CHECK_EQ(p.comp_type, 0);
    CHECK_EQ(p.byte_count, 20);
    CHECK_MEM(p.data, want_alt, 20);
    tagtinker_free_image_payload(&p);

    /* Forcing RLE on 8 alternating pixels: [0] then eight 1-bit runs. */
    const uint8_t want_forced[20] = {0x7F, 0x80};
    CHECK(tagtinker_encode_fn_payload(alt_pixel, NULL, 8,
                                      TagTinkerCompressionRle, &p));
    CHECK_EQ(p.comp_type, 2);
    CHECK_EQ(p.byte_count, 20);
    CHECK_MEM(p.data, want_forced, 20);
    tagtinker_free_image_payload(&p);

    /* Payloads are always a whole number of 20-byte data frames. */
    size_t total = (size_t)TAGTINKER_COLOR26_WIRE_W *
                   TAGTINKER_COLOR26_WIRE_H * 2U;
    CHECK_EQ(total, 89984);
    CHECK(tagtinker_encode_fn_payload(zero_pixel, NULL, total,
                                      TagTinkerCompressionAuto, &p));
    CHECK_EQ(p.byte_count % TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME, 0);
    tagtinker_free_image_payload(&p);

    /* Degenerate inputs must be rejected, not crash. */
    CHECK(!tagtinker_encode_fn_payload(NULL, NULL, 16,
                                       TagTinkerCompressionAuto, &p));
    CHECK(!tagtinker_encode_fn_payload(zero_pixel, NULL, 0,
                                       TagTinkerCompressionAuto, &p));
}

int main(void) {
    test_barcode();
    test_profile();
    test_page_and_transpose();
    test_crc();
    test_frames();
    test_encode();
    TEST_REPORT("test_proto");
}
```

- [ ] **Step 4: Run the test to verify it fails**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: FAIL. `make` cannot build `vendor.o` because `../../src/modules/ir/esl/protocol/tagtinker_proto.c` does not exist yet — an error like `No rule to make target '../../src/modules/ir/esl/protocol/tagtinker_proto.c'`.

- [ ] **Step 5: Create the compat shim**

Create `src/modules/ir/esl/tagtinker_app.h`. The vendored `.c` includes `../tagtinker_app.h` and needs exactly one thing from it:

```c
/* Compat shim for the vendored TagTinker protocol core.
 *
 * protocol/tagtinker_proto.c includes "../tagtinker_app.h". On the Flipper that
 * is the full app header; here the protocol code only needs COUNT_OF, so this
 * stands in for it and lets the vendored file compile with zero edits. */
#pragma once

#ifndef COUNT_OF
#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))
#endif
```

- [ ] **Step 6: Vendor the protocol files unchanged**

Copy them straight out of the PR branch. Do not hand-edit:

```bash
cd firmware && mkdir -p src/modules/ir/esl/protocol
git -C ../TagTinker show pr-53:protocol/tagtinker_proto.h > src/modules/ir/esl/protocol/tagtinker_proto.h
git -C ../TagTinker show pr-53:protocol/tagtinker_proto.c > src/modules/ir/esl/protocol/tagtinker_proto.c
```

If the `pr-53` ref is missing, recreate it: `git -C ../TagTinker fetch upstream pull/53/head:pr-53`.

- [ ] **Step 7: Create the `extern "C"` wrapper for C++ consumers**

Create `src/modules/ir/esl/esl_proto.h`:

```c
/* Include this instead of protocol/tagtinker_proto.h from C++.
 *
 * The vendored header has no extern "C" guard of its own (and must not be
 * edited), so its non-inline functions would otherwise be name-mangled by the
 * C++ compiler and fail to link against the C-compiled protocol object. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol/tagtinker_proto.h"

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 8: Run the test to verify it passes**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: `test_proto: N checks, 0 failures` then `ALL TESTS PASSED`.

- [ ] **Step 9: Confirm the vendored files are byte-identical to the PR**

This guard is the whole point of the vendoring rule, so verify it explicitly:

```bash
cd firmware && git -C ../TagTinker show pr-53:protocol/tagtinker_proto.c | diff - src/modules/ir/esl/protocol/tagtinker_proto.c && git -C ../TagTinker show pr-53:protocol/tagtinker_proto.h | diff - src/modules/ir/esl/protocol/tagtinker_proto.h && echo "VENDOR CLEAN"
```

Expected: `VENDOR CLEAN` with no diff output.

- [ ] **Step 10: Commit**

```bash
cd firmware
git add src/modules/ir/esl tools/esl_host_tests
git commit -m "feat(esl): vendor TagTinker protocol core with host test harness"
```

---

## Task 2: PP4 symbol builder

**Files:**
- Create: `src/modules/ir/esl/esl_pp4.h`
- Create: `src/modules/ir/esl/esl_pp4.c`
- Test: `tools/esl_host_tests/test_pp4.c`

**Interfaces:**
- Consumes: nothing from Task 1 (deliberately standalone — the timing layer knows nothing about frame contents).
- Produces: `typedef struct { uint16_t burst_ticks; uint16_t gap_ticks; } EslPp4Symbol;`, `size_t esl_pp4_symbol_count(size_t frame_len)`, `size_t esl_pp4_encode(const uint8_t *frame, size_t len, uint32_t resolution_hz, EslPp4Symbol *out, size_t out_cap)`, `extern const uint32_t esl_pp4_gap_cycles[4]`, and the macros `ESL_PP4_TICKS(cycles, res_hz)`, `ESL_PP4_SRC_CLOCK_HZ` (64000000), `ESL_PP4_BURST_CYCLES` (2581), `ESL_PP4_TAIL_GAP_CYCLES` (3871), `ESL_PP4_MAX_FRAME_LEN` (255), `ESL_PP4_DIBITS_PER_BYTE` (4). Task 4 uses all of these.

- [ ] **Step 1: Write the failing test**

Create `tools/esl_host_tests/test_pp4.c`. `0xE4` is chosen because its four LSB-first dibits are exactly `0, 1, 2, 3`, so one byte exercises every gap value in a known order:

```c
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
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: FAIL — `fatal error: 'esl_pp4.h' file not found`.

- [ ] **Step 3: Write the header**

Create `src/modules/ir/esl/esl_pp4.h`:

```c
/* PP4 symbol builder.
 *
 * TagTinker's IR line code sends two bits per symbol: a fixed carrier burst
 * followed by a gap whose length encodes the dibit value. This file turns
 * frame bytes into (burst, gap) tick pairs and has no hardware dependencies,
 * so it can be unit-tested on the host. */
#pragma once

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
```

- [ ] **Step 4: Write the implementation**

Create `src/modules/ir/esl/esl_pp4.c`:

```c
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
```

- [ ] **Step 5: Run tests to verify they pass**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: both `test_proto` and `test_pp4` report `0 failures`, then `ALL TESTS PASSED`.

- [ ] **Step 6: Commit**

```bash
cd firmware
git add src/modules/ir/esl/esl_pp4.h src/modules/ir/esl/esl_pp4.c tools/esl_host_tests/test_pp4.c
git commit -m "feat(esl): add host-tested PP4 symbol builder"
```

---

## Task 3: Frame sequences with injectable ops

**Files:**
- Create: `src/modules/ir/esl/esl_tx.h`
- Create: `src/modules/ir/esl/esl_tx.c`
- Test: `tools/esl_host_tests/test_tx.c`

**Interfaces:**
- Consumes: from Task 1 — `tagtinker_make_wake_frame`, `tagtinker_make_ping_frame`, `tagtinker_make_refresh_frame`, `tagtinker_make_image_param_frame`, `tagtinker_make_image_data_frame`, `tagtinker_color26_resolve_page`, `TagTinkerImagePayload`, `TAGTINKER_MAX_FRAME_SIZE`, `TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME`, `TAGTINKER_COLOR26_WIRE_W/H`.
- Produces: `EslTxOps` (fields `send`, `settle_ms`, `aborted`, `progress`, `ctx`), `bool esl_tx_send_color26(const EslTxOps*, const uint8_t plid[4], const TagTinkerImagePayload*, uint8_t page)`, `bool esl_tx_send_generic(const EslTxOps*, const uint8_t plid[4], const TagTinkerImagePayload*, uint8_t page, uint16_t width, uint16_t height, uint16_t pos_x, uint16_t pos_y, uint16_t data_repeats)`, `size_t esl_tx_step_count(const TagTinkerImagePayload*)`, the repeat-count macros, and the pacing macros `ESL_COLOR26_DATA_PACE_EVERY/MS` (1 / 50) and `ESL_GENERIC_DATA_PACE_EVERY/MS` (32 / 1). Tasks 5 and 7 call these.

- [ ] **Step 1: Write the failing test**

Create `tools/esl_host_tests/test_tx.c`. A recording fake stands in for the radio, which lets the exact frame order and repeat counts be asserted with no hardware:

```c
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

    /* Param: MCU 0x05, wire dims 152x296, page 0 resolved to 2, repeats 1. */
    CHECK_EQ(k.rec[1].frame[9], 0x05);
    CHECK_EQ(k.rec[1].frame[14], 2); /* resolved page */
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

static void test_generic_sequence(void) {
    Fake k;
    EslTxOps ops;
    TagTinkerImagePayload p;
    fake_init(&k, &ops);
    /* 40 data frames so the every-32nd pacing rule is actually exercised. */
    payload_init(&p, 40);

    CHECK(esl_tx_send_generic(&ops, PLID, &p, 3, 296, 128, 0, 0,
                              ESL_GENERIC_DATA_REPEATS));

    /* ping + param + 40 data + refresh */
    CHECK_EQ(k.count, 43);
    CHECK_EQ(k.rec[0].frame[5], 0x97); /* ping */
    CHECK_EQ(k.rec[0].repeats, ESL_GENERIC_PING_REPEATS);
    CHECK_EQ(k.rec[0].repeats, 80);
    CHECK_EQ(k.rec[1].frame[9], 0x05); /* param */
    CHECK_EQ(k.rec[1].repeats, ESL_GENERIC_PARAM_REPEATS);
    CHECK_EQ(k.rec[1].repeats, 15);
    CHECK_EQ(k.rec[1].frame[14], 3); /* generic path does not remap the page */
    CHECK_EQ(k.rec[2].repeats, ESL_GENERIC_DATA_REPEATS);
    CHECK_EQ(k.rec[2].repeats, 2);
    CHECK_EQ(k.rec[42].frame[9], 0x01); /* refresh */
    CHECK_EQ(k.rec[42].repeats, ESL_GENERIC_REFRESH_REPEATS);
    CHECK_EQ(k.rec[42].repeats, 20);

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

int main(void) {
    test_color26_sequence();
    test_explicit_page_preserved();
    test_generic_sequence();
    test_generic_honours_data_repeats();
    test_abort_stops_early();
    test_send_failure_propagates();
    test_guards();
    TEST_REPORT("test_tx");
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: FAIL — `fatal error: 'esl_tx.h' file not found`.

- [ ] **Step 3: Write the header**

Create `src/modules/ir/esl/esl_tx.h`:

```c
/* ESL transmit sequences.
 *
 * Builds and orders the frames for an image upload. The radio is injected as a
 * set of callbacks so the sequence logic can be unit-tested on the host with a
 * recording fake instead of real hardware. */
#pragma once

#include "protocol/tagtinker_proto.h"

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
```

- [ ] **Step 4: Write the implementation**

Create `src/modules/ir/esl/esl_tx.c`:

```c
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

    /* resolve_page is idempotent for 2..7, so resolving here is safe even if
     * the caller already resolved it. */
    return tx_payload_stages(
        ops, plid, payload, tagtinker_color26_resolve_page(page),
        TAGTINKER_COLOR26_WIRE_W, TAGTINKER_COLOR26_WIRE_H, 0u, 0u, &prof,
        &done, total);
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
```

- [ ] **Step 5: Run tests to verify they pass**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: `test_proto`, `test_pp4`, `test_tx` all `0 failures`, then `ALL TESTS PASSED`.

- [ ] **Step 6: Commit**

```bash
cd firmware
git add src/modules/ir/esl/esl_tx.h src/modules/ir/esl/esl_tx.c tools/esl_host_tests/test_tx.c
git commit -m "feat(esl): add host-tested image transmit sequences"
```

---

## Task 4: ESP32 RMT driver

**Files:**
- Create: `src/modules/ir/esl/esl_ir_driver.h`
- Create: `src/modules/ir/esl/esl_ir_driver.cpp`

**Interfaces:**
- Consumes: from Task 2 — `esl_pp4_encode`, `EslPp4Symbol`, `esl_pp4_symbol_count`, `ESL_PP4_MAX_FRAME_LEN`. From Bruce — `bruceConfigPins.irTx`, `setup_ir_pin(int, uint8_t)` (`modules/ir/ir_utils.h`), `LED_OFF`.
- Produces: `bool esl_ir_init(int gpio)`, `void esl_ir_deinit(void)`, `bool esl_ir_transmit(const uint8_t *data, size_t len, uint16_t repeats, uint8_t delay)`, `void esl_ir_stop(void)`, `bool esl_ir_is_busy(void)`, `#define ESL_IR_MAX_FRAME_LEN 96`. Tasks 5 and 7 call these. The signature of `esl_ir_transmit` deliberately matches `EslTxOps::send` minus the `ctx` argument.

There is no host test here: this file is a hardware shim, and all of its decision logic already lives in the host-tested `esl_pp4`. It is verified by a compile plus an on-device scope capture.

- [ ] **Step 1: Write the header**

Create `src/modules/ir/esl/esl_ir_driver.h`:

```c
/* ESP32 RMT driver for TagTinker's PP4 infrared line code.
 *
 * The RMT peripheral generates the ~1.25 MHz carrier in hardware and clocks
 * out the burst/gap envelope, so symbol timing is immune to FreeRTOS, WiFi and
 * BT activity. That is why this needs no interrupt-disabling critical section,
 * unlike the Flipper implementation it replaces. */
#pragma once

#include <stddef.h>
#include <stdint.h>

/* ESL protocol frames never exceed TAGTINKER_MAX_FRAME_SIZE (96). Bounding the
 * driver here keeps the static symbol buffers small. */
#define ESL_IR_MAX_FRAME_LEN 96

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Claims the IR pin and sets up the RMT channel, carrier and encoder.
 * Safe to call twice. Returns false if any RMT resource cannot be allocated. */
bool esl_ir_init(int gpio);

/* Releases the RMT channel and parks the IR LED off. */
void esl_ir_deinit(void);

/* Sends one frame repeats+1 times, waiting delay*500us between repeats.
 * Blocks until done. Returns false on error or if stopped. */
bool esl_ir_transmit(const uint8_t *data, size_t len, uint16_t repeats,
                     uint8_t delay);

/* Asks an in-flight transmit to stop at the next frame boundary. */
void esl_ir_stop(void);

bool esl_ir_is_busy(void);

/* Abort poll. esl_ir_transmit blocks the calling task for repeats+1 frames —
 * 401 frames (~10 s) for a wake burst — so without this the caller has no way
 * to interrupt it and any "Esc aborts" prompt would be a lie. The hook is
 * polled between repeats, which keeps transmit synchronous on one task (no TX
 * thread, so the missing re-entry guard stays safe) while still giving the user
 * upstream's ability to cancel a send. Pass NULL to clear. */
typedef bool (*EslIrAbortFn)(void *ctx);
void esl_ir_set_abort_hook(EslIrAbortFn fn, void *ctx);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Write the implementation**

Create `src/modules/ir/esl/esl_ir_driver.cpp`:

```cpp
#include "esl_ir_driver.h"

#include "esl_pp4.h"
#include "modules/ir/ir_utils.h"
#include <Arduino.h>
#include <driver/rmt_tx.h>
#include <esp_rom_sys.h>
#include <globals.h>

/* 80 MHz resolution gives 12.5 ns ticks: fine enough that the derived PP4
 * durations stay well inside RMT's 15-bit duration field (max gap ~19353). */
#define ESL_RMT_RESOLUTION_HZ 80000000u

/* 80 MHz / 64 = 1.25 MHz, within 0.4% of the Flipper's 64 MHz / 51. */
#define ESL_CARRIER_HZ 1250000u
#define ESL_CARRIER_DUTY 0.49f

#define ESL_RMT_MEM_BLOCK_SYMBOLS 64
#define ESL_RMT_QUEUE_DEPTH 2
#define ESL_RMT_DONE_TIMEOUT_MS 1000

#define ESL_IR_MAX_SYMBOLS (ESL_IR_MAX_FRAME_LEN * 4 + 1) /* 385 */

static rmt_channel_handle_t s_channel = nullptr;
static rmt_encoder_handle_t s_encoder = nullptr;
static int s_gpio = -1;
static volatile bool s_stop = false;
static volatile bool s_busy = false;
static EslIrAbortFn s_abort_fn = nullptr;
static void *s_abort_ctx = nullptr;

static EslPp4Symbol s_symbols[ESL_IR_MAX_SYMBOLS];
static rmt_symbol_word_t s_words[ESL_IR_MAX_SYMBOLS];

bool esl_ir_init(int gpio) {
    if (s_channel != nullptr) return true;

    s_gpio = gpio;
    setup_ir_pin(gpio, OUTPUT);
    digitalWrite(gpio, LED_OFF);

    rmt_tx_channel_config_t chan_cfg = {};
    chan_cfg.gpio_num = (gpio_num_t)gpio;
    chan_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
    chan_cfg.resolution_hz = ESL_RMT_RESOLUTION_HZ;
    chan_cfg.mem_block_symbols = ESL_RMT_MEM_BLOCK_SYMBOLS;
    chan_cfg.trans_queue_depth = ESL_RMT_QUEUE_DEPTH;
    if (rmt_new_tx_channel(&chan_cfg, &s_channel) != ESP_OK) {
        s_channel = nullptr;
        return false;
    }

    rmt_carrier_config_t carrier_cfg = {};
    carrier_cfg.frequency_hz = ESL_CARRIER_HZ;
    carrier_cfg.duty_cycle = ESL_CARRIER_DUTY;
    /* Carrier rides the high (mark) level, matching the active-high IR LED. */
    carrier_cfg.flags.polarity_active_low = false;
    carrier_cfg.flags.always_on = false;
    if (rmt_apply_carrier(s_channel, &carrier_cfg) != ESP_OK) {
        esl_ir_deinit();
        return false;
    }

    rmt_copy_encoder_config_t enc_cfg = {};
    if (rmt_new_copy_encoder(&enc_cfg, &s_encoder) != ESP_OK) {
        esl_ir_deinit();
        return false;
    }

    if (rmt_enable(s_channel) != ESP_OK) {
        esl_ir_deinit();
        return false;
    }

    s_stop = false;
    s_busy = false;
    return true;
}

void esl_ir_deinit(void) {
    if (s_channel != nullptr) {
        rmt_disable(s_channel);
    }
    if (s_encoder != nullptr) {
        rmt_del_encoder(s_encoder);
        s_encoder = nullptr;
    }
    if (s_channel != nullptr) {
        rmt_del_channel(s_channel);
        s_channel = nullptr;
    }
    if (s_gpio >= 0) {
        pinMode(s_gpio, OUTPUT);
        digitalWrite(s_gpio, LED_OFF);
    }
    s_busy = false;
    s_stop = false;
}

bool esl_ir_transmit(const uint8_t *data, size_t len, uint16_t repeats,
                     uint8_t delay) {
    if (s_channel == nullptr || s_encoder == nullptr || data == nullptr) {
        return false;
    }
    if (len == 0u || len > ESL_IR_MAX_FRAME_LEN) return false;

    const size_t n = esl_pp4_encode(data, len, ESL_RMT_RESOLUTION_HZ, s_symbols,
                                    ESL_IR_MAX_SYMBOLS);
    if (n == 0u) return false;

    for (size_t i = 0; i < n; i++) {
        s_words[i].level0 = 1;
        s_words[i].duration0 = s_symbols[i].burst_ticks;
        s_words[i].level1 = 0;
        s_words[i].duration1 = s_symbols[i].gap_ticks;
    }

    rmt_transmit_config_t tx_cfg = {};
    tx_cfg.loop_count = 0;
    /* Park the line low after each frame so the IR LED never idles on. */
    tx_cfg.flags.eot_level = 0;

    const uint32_t reps = (uint32_t)(repeats & 0x7FFFu);
    s_stop = false;
    s_busy = true;
    bool ok = true;

    for (uint32_t r = 0; r <= reps; r++) {
        /* Poll the caller's abort hook between repeats so a long burst can be
         * cancelled without moving TX off this task. */
        if (s_abort_fn != nullptr && s_abort_fn(s_abort_ctx)) s_stop = true;
        if (s_stop) {
            ok = false;
            break;
        }
        /* The copy encoder takes a byte count, not a symbol count. */
        if (rmt_transmit(s_channel, s_encoder, s_words,
                         n * sizeof(rmt_symbol_word_t), &tx_cfg) != ESP_OK) {
            ok = false;
            break;
        }
        /* Blocks on a semaphore, so this yields to FreeRTOS and cannot starve
         * the watchdog even across hundreds of repeats. */
        if (rmt_tx_wait_all_done(s_channel, ESL_RMT_DONE_TIMEOUT_MS) != ESP_OK) {
            ok = false;
            break;
        }
        if (r < reps && delay > 0u) {
            esp_rom_delay_us((uint32_t)delay * 500u);
        }
    }

    s_busy = false;
    if (s_gpio >= 0) digitalWrite(s_gpio, LED_OFF);
    return ok;
}

void esl_ir_stop(void) { s_stop = true; }

bool esl_ir_is_busy(void) { return s_busy; }

void esl_ir_set_abort_hook(EslIrAbortFn fn, void *ctx) {
    s_abort_fn = fn;
    s_abort_ctx = ctx;
}
```

- [ ] **Step 3: Verify the static buffer bound is consistent**

`ESL_IR_MAX_FRAME_LEN` (96) must match the protocol's own frame bound, or long frames would be silently rejected at runtime. Add this compile-time check to the top of `esl_ir_driver.cpp`, just after the includes:

```cpp
#include "esl_proto.h"
static_assert(ESL_IR_MAX_FRAME_LEN >= TAGTINKER_MAX_FRAME_SIZE,
              "IR driver frame bound must cover every ESL protocol frame");
```

- [ ] **Step 4: Build the firmware**

PlatformIO is not on `PATH` and the bundled virtualenv at `~/.platformio/penv` is broken (`bad interpreter`). Establish a working build first:

```bash
/opt/homebrew/bin/python3.12 -m venv /tmp/pio-venv && /tmp/pio-venv/bin/pip install -q platformio && /tmp/pio-venv/bin/pio --version
```

**Use Python 3.12 explicitly.** The default `python3` on this machine is 3.14, which PlatformIO's library installer rejects.

Known first-build trap: Bruce's `patch.py` renames `libnet80211.a` to `.old`, and if the Xtensa toolchain package is not yet installed its `objcopy` step silently fails while still writing `.patched`, so it never retries. The link then fails with `cannot find -lnet80211`. Repair the *installed framework package* (never the repo) using the toolchain's own objcopy:

```bash
xtensa-esp32s3-elf-objcopy --weaken-symbol=ieee80211_raw_frame_sanity_check \
  <framework>/esp32s3/lib/libnet80211.a.old <framework>/esp32s3/lib/libnet80211.a
```

Then build:

```bash
cd firmware && /tmp/pio-venv/bin/pio run -e lilygo-t-embed-cc1101
```

Expected: `SUCCESS`. The first run downloads the toolchain and takes several minutes. If the build fails on `rmt_carrier_config_t` field names, check the installed ESP-IDF version's `driver/rmt_tx.h` and adjust the designated initialisers — the flag names differ between IDF 5.0 and 5.1+.

- [ ] **Step 5: Commit**

```bash
cd firmware
git add src/modules/ir/esl/esl_ir_driver.h src/modules/ir/esl/esl_ir_driver.cpp
git commit -m "feat(esl): add ESP32 RMT PP4 infrared driver"
```

---

## Task 5: M0 timing checkpoint on real hardware

**Files:**
- Create: `src/modules/ir/esl/esl_app.h`
- Create: `src/modules/ir/esl/esl_app.cpp`
- Modify: `src/core/menu_items/IRMenu.cpp:1-29`

**Interfaces:**
- Consumes: Task 1 (`esl_proto.h`, `tagtinker_barcode_to_plid`, `tagtinker_barcode_to_profile`, `tagtinker_make_wake_frame`, `TAGTINKER_MAX_FRAME_SIZE`), Task 3 (`ESL_COLOR26_WAKE_REPEATS`, `ESL_FRAME_DELAY_UNITS`), Task 4 (`esl_ir_init`, `esl_ir_deinit`, `esl_ir_transmit`, `esl_ir_stop`). From Bruce: `bruceConfigPins.irTx`, `checkIrTxPin()`, `keyboard()`, `drawMainBorderWithTitle`, `displayError`, `displaySuccess`, `displayTextLine`, `check(EscPress)`, `returnToMenu`, `delay`.
- Produces: `void startEslTx(void)` — the menu entry point, expanded in Task 7. Also `esl_prompt_target()`, reused unchanged by Task 7.

**This is a timing checkpoint, not a feature.** Its purpose is to put a real, correctly-timed frame on the wire so the waveform can be measured. Two fidelity rules bind it:

- **Nothing is hardcoded.** The barcode comes from the same `keyboard()` prompt Task 7 uses, so the real addressing path is exercised from the first run. There must be no compiled-in barcode, PLID, or profile.
- **No invented features.** Upstream has no "send a blank image" operation, so this task sends *only* the wake frame. Making the tag visibly refresh needs a real image payload, which is Task 7's job and Task 7's gate.

- [ ] **Step 1: Write the header**

Create `src/modules/ir/esl/esl_app.h`. `esl_prompt_target` is declared here because Task 7 reuses it verbatim:

```cpp
/* Bruce UI entry point for the ESL infrared transmitter. */
#pragma once

#include "esl_proto.h"

void startEslTx();

/* Prompts for a tag barcode; fills plid + profile. False if the user's entry
 * is unusable (wrong length, unknown type, or a segment tag). */
bool esl_prompt_target(uint8_t plid[4], TagTinkerTagProfile *profile);
```

- [ ] **Step 2: Write the M0 checkpoint implementation**

Create `src/modules/ir/esl/esl_app.cpp`. `esl_prompt_target` is written in its final form here because Task 7 reuses it verbatim:

```cpp
#include "esl_app.h"

#include "esl_ir_driver.h"
#include "esl_proto.h"
#include "esl_tx.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "modules/ir/TV-B-Gone.h"
#include <Arduino.h>
#include <globals.h>

#define ESL_BARCODE_LEN 17

/* Settle before blasting IR, mirroring the upstream app's pre-TX pause. */
#define ESL_PRE_TX_SETTLE_MS 500

/* Latches once pressed so the outcome can be reported as an abort rather than
 * a transmit failure. */
static bool s_ui_aborted = false;

/* Polled by the driver between repeats so a long burst is interruptible while
 * transmit stays synchronous on this task. */
static bool ui_abort_poll(void *ctx) {
    (void)ctx;
    if (!s_ui_aborted && check(EscPress)) s_ui_aborted = true;
    return s_ui_aborted;
}

/* Prompts for the tag barcode and derives its address and profile. The barcode
 * is always entered by the user — upstream never compiles one in. */
bool esl_prompt_target(uint8_t plid[4], TagTinkerTagProfile *profile) {
    String entered = keyboard("", ESL_BARCODE_LEN, "Tag barcode (17 chars):");
    entered.trim();

    /* Bruce's keyboard returns ESC when the user backs out. Treat that as a
     * silent cancel rather than scolding them about a length they never
     * entered. (ESC is not whitespace, so it survives trim().) */
    if (entered.length() == 0 || entered == "\x1B") return false;

    if (entered.length() != ESL_BARCODE_LEN) {
        displayError("Barcode must be 17 chars", true);
        return false;
    }
    if (!tagtinker_barcode_to_plid(entered.c_str(), plid)) {
        displayError("Bad barcode", true);
        return false;
    }
    if (!tagtinker_barcode_to_profile(entered.c_str(), profile)) {
        displayError("Unknown tag type", true);
        return false;
    }
    /* Segment tags have no image page, matching supports_graphics upstream. */
    if (profile->kind != TagTinkerTagKindDotMatrix) {
        displayError("Tag has no image page", true);
        return false;
    }
    return true;
}

void startEslTx() {
    drawMainBorderWithTitle("ESL Image");

    uint8_t plid[4] = {0};
    TagTinkerTagProfile profile;
    if (!esl_prompt_target(plid, &profile)) {
        returnToMenu = true;
        return;
    }

    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        displayError("IR init failed", true);
        returnToMenu = true;
        return;
    }

    /* Install the abort poll before transmitting so the status line below is
     * truthful: the driver checks this between repeats. */
    s_ui_aborted = false;
    esl_ir_set_abort_hook(ui_abort_poll, nullptr);

    drawMainBorderWithTitle("ESL Image");
    displayTextLine("Wake frames, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    /* M0 sends only the wake frame, so the waveform can be measured against a
     * real addressed frame. Image payloads arrive in Task 7. */
    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    const size_t len = tagtinker_make_wake_frame(frame, plid);
    const bool ok = esl_ir_transmit(frame, len, ESL_COLOR26_WAKE_REPEATS,
                                    ESL_FRAME_DELAY_UNITS);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();

    if (s_ui_aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Wake sent", true);
    } else {
        displayError("TX failed", true);
    }
    returnToMenu = true;
}
```


- [ ] **Step 3: Wire it into the IR menu**

Modify `src/core/menu_items/IRMenu.cpp`. Add the include after the existing IR includes (line 8):

```cpp
#include "modules/ir/esl/esl_app.h"
```

Then add the entry to the `options` vector in `optionsMenu()`, after `"IR Read"`:

```cpp
        {"ESL Image", startEslTx                },
```

The vector becomes:

```cpp
    options = {
        {"TV-B-Gone", StartTvBGone              },
        {"Custom IR", otherIRcodes              },
        {"IR Read",   [=]() { IrRead(); }       },
        {"ESL Image", startEslTx                },
#if !defined(LITE_VERSION)
        {"IR Jammer", startIrJammer             }, // Simple frequency-adjustable jammer
#endif
        {"Config",    [this]() { configMenu(); }},
    };
```

- [ ] **Step 4: Build**

```bash
cd firmware && /tmp/pio-venv/bin/pio run -e lilygo-t-embed-cc1101
```

Expected: `SUCCESS`.

- [ ] **Step 5: Verify the waveform on a scope or logic analyzer**

Flash the device, probe the IR TX pin (GPIO 2), run `IR -> ESL Image`, and enter the tag's barcode at the prompt.

```bash
cd firmware && /tmp/pio-venv/bin/pio run -e lilygo-t-embed-cc1101 -t upload
```

This is the task's exit criterion. Confirm against the golden timing:
- Carrier inside a burst: **1.25 MHz ± 1%**, duty ~48%.
- Burst width: **40.3 µs ± 2%**.
- Gaps take exactly four distinct values: **60.5, 121.0, 181.4, 241.9 µs** (± 2%).
- The line rests **low** between frames and after the sequence completes.
- 401 identical wake frames, 500 µs apart.

Diagnostics: if the four gap values are not distinct, the dibit ordering is wrong — re-check `test_pp4`. If there is no carrier, re-check `polarity_active_low`. If the line idles high, re-check `eot_level`.

The tag will **not** visibly change here — a wake frame alone does not repaint it. That is expected; the tag refresh is Task 7's gate.

- [ ] **Step 6: Verify the error and abort paths**

Confirm each leaves the device back at the menu with the IR LED off:
- Enter a 5-character barcode → `Barcode must be 17 chars`.
- Enter `A4165420155299995` (valid shape, unknown type) → `Unknown tag type`.

- [ ] **Step 7: Commit**

```bash
cd firmware
git add src/modules/ir/esl/esl_app.h src/modules/ir/esl/esl_app.cpp src/core/menu_items/IRMenu.cpp
git commit -m "feat(esl): add M0 timing checkpoint and IR menu entry"
```

---

## Task 6: BMP parsing and pixel callbacks

**Files:**
- Create: `src/modules/ir/esl/esl_bmp.h`
- Create: `src/modules/ir/esl/esl_bmp.c`
- Test: `tools/esl_host_tests/test_bmp.c`

**Interfaces:**
- Consumes: from Task 1 — `TAGTINKER_COLOR26_WIRE_W/H`, `TAGTINKER_COLOR26_GLASS_W/H`, `tagtinker_color26_proto_to_glass`, `TagTinkerPixelAtFn`.
- Produces: `EslBmpInfo` (fields `data_offset`, `row_stride`, `width`, `height`, `bpp`, `top_down`), `bool esl_bmp_parse(const uint8_t *file, size_t len, EslBmpInfo *info)`, `uint16_t esl_bmp_map_x(uint16_t out_x, uint16_t tx_w, uint16_t src_w)`, `uint16_t esl_bmp_map_y(uint16_t out_y, uint16_t tx_h, uint16_t src_h)`, `EslColor26BmpCtx` (fields `file`, `file_len`, `info`) with `uint8_t esl_color26_bmp_pixel(size_t idx, void *ctx)`, and `EslGenericBmpCtx` (fields `file`, `file_len`, `info`, `out_w`, `out_h`, `second_plane`) with `uint8_t esl_generic_bmp_pixel(size_t idx, void *ctx)`. Both callbacks match `TagTinkerPixelAtFn`, so Task 7 passes them straight into `tagtinker_encode_fn_payload`.

Two conventions this task must reproduce exactly:

**Plane marker.** `bpp` is a marker, not a bit depth. `bpp = 1` is a single 1-bit plane; `bpp = 2` means **two stacked 1-bit planes** with the *same* row stride, where the header height is the per-plane height and the accent plane starts at row offset `height`. A BMP bit of `1` means white, which is ESL `0`.

**Accepted bpp set.** `esl_bmp_parse` accepts **1, 2, 24 and 32**, exactly as upstream's `tx_bmp_open` does, using upstream's per-bpp strides (`((w+31)/32)*4` for 1 and 2, `((w*3)+3) & ~3` for 24, `w*4` for 32). Rejecting non-1/2 is the *Color 2.6 send path's* job, not the parser's. The faithful consequence: upstream's pixel reader takes one bit per pixel regardless of bpp, so a 24/32-bpp source is accepted by the parser but not meaningfully decoded. That is upstream's behaviour and we reproduce it rather than diverging — the web prep tool always emits 1 or 2, which is the supported flow.

- [ ] **Step 1: Write the failing test**

Create `tools/esl_host_tests/test_bmp.c`:

```c
#include "esl_bmp.h"
#include "test_util.h"
#include <stdlib.h>

/* Builds a minimal 54-byte-header BMP. planes=2 stacks a second plane. */
static uint8_t *make_bmp(uint16_t w, uint16_t h, uint16_t bpp, int top_down,
                         unsigned planes, size_t *out_len) {
    const uint32_t stride = ((uint32_t)(w + 31u) / 32u) * 4u;
    const uint32_t data = 54u;
    const size_t len = data + (size_t)stride * h * planes;
    uint8_t *f = (uint8_t *)calloc(len, 1);

    f[0] = 'B';
    f[1] = 'M';
    f[10] = (uint8_t)(data & 0xFF);
    f[18] = (uint8_t)(w & 0xFF);
    f[19] = (uint8_t)(w >> 8);
    int32_t hh = top_down ? -(int32_t)h : (int32_t)h;
    f[22] = (uint8_t)(hh & 0xFF);
    f[23] = (uint8_t)((hh >> 8) & 0xFF);
    f[24] = (uint8_t)((hh >> 16) & 0xFF);
    f[25] = (uint8_t)((hh >> 24) & 0xFF);
    f[28] = (uint8_t)(bpp & 0xFF);
    f[29] = (uint8_t)(bpp >> 8);

    *out_len = len;
    return f;
}

static void set_bit(uint8_t *f, const EslBmpInfo *i, unsigned plane,
                    uint16_t row, uint16_t x) {
    uint32_t off = i->data_offset +
                   ((uint32_t)row + (uint32_t)plane * i->height) * i->row_stride;
    f[off + x / 8u] |= (uint8_t)(1u << (7u - (x % 8u)));
}

static void test_parse(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(296, 152, 1, 0, 1, &len);
    EslBmpInfo info;

    CHECK(esl_bmp_parse(f, len, &info));
    CHECK_EQ(info.width, 296);
    CHECK_EQ(info.height, 152);
    CHECK_EQ(info.bpp, 1);
    CHECK_EQ(info.data_offset, 54);
    CHECK_EQ(info.row_stride, 40); /* (296+31)/32*4 */
    CHECK(!info.top_down);
    free(f);

    /* Negative header height means top-down. */
    f = make_bmp(152, 296, 2, 1, 2, &len);
    CHECK(esl_bmp_parse(f, len, &info));
    CHECK(info.top_down);
    CHECK_EQ(info.height, 296);
    CHECK_EQ(info.bpp, 2);
    CHECK_EQ(info.row_stride, 20); /* (152+31)/32*4 */
    free(f);

    /* 24 and 32 bpp are accepted, with upstream's per-bpp strides. Rejecting
     * non-1/2 is the Color 2.6 send path's job, not the parser's. */
    f = make_bmp(100, 10, 24, 0, 1, &len);
    f[28] = 24;
    CHECK(esl_bmp_parse(f, len, &info));
    CHECK_EQ(info.bpp, 24);
    CHECK_EQ(info.row_stride, 300); /* (100*3 + 3) & ~3 */
    free(f);

    f = make_bmp(100, 10, 32, 0, 1, &len);
    f[28] = 32;
    CHECK(esl_bmp_parse(f, len, &info));
    CHECK_EQ(info.bpp, 32);
    CHECK_EQ(info.row_stride, 400); /* 100*4 */
    free(f);

    /* Rejections. Each case is isolated so it can only fail for its own
     * reason: the header is restored to valid between mutations. */
    f = make_bmp(8, 8, 1, 0, 1, &len);
    CHECK(esl_bmp_parse(f, len, &info)); /* valid baseline */

    f[0] = 'X';
    CHECK(!esl_bmp_parse(f, len, &info)); /* bad magic */
    f[0] = 'B';

    f[28] = 16;
    CHECK(!esl_bmp_parse(f, len, &info)); /* bpp outside the accepted set */
    f[28] = 1;

    CHECK(!esl_bmp_parse(f, 53, &info)); /* truncated header, bpp valid */
    CHECK(!esl_bmp_parse(NULL, len, &info));
    CHECK(!esl_bmp_parse(f, len, NULL));
    free(f);
}

/* Generic profiles: plain nearest-neighbour rescale, no transpose. */
static void test_generic_pixel(void) {
    size_t len = 0;
    /* 8x8 source, top-down, scaled up to a 16x16 output. */
    uint8_t *f = make_bmp(8, 8, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 0, 0); /* source (0,0) is white */

    EslGenericBmpCtx ctx = {f, len, &info, 16, 16, false};

    /* Upscale x2: output (0,0), (1,0), (0,1), (1,1) all map to source (0,0). */
    CHECK_EQ(esl_generic_bmp_pixel(0, &ctx), 0);
    CHECK_EQ(esl_generic_bmp_pixel(1, &ctx), 0);
    CHECK_EQ(esl_generic_bmp_pixel(16, &ctx), 0);
    CHECK_EQ(esl_generic_bmp_pixel(17, &ctx), 0);
    /* Output (2,0) maps to source (1,0), which is untouched. */
    CHECK_EQ(esl_generic_bmp_pixel(2, &ctx), 1);

    /* With second_plane on a 1-plane source, the accent plane reads clear. */
    EslGenericBmpCtx accent_ctx = {f, len, &info, 16, 16, true};
    CHECK_EQ(esl_generic_bmp_pixel(256, &accent_ctx), 1);
    CHECK_EQ(esl_generic_bmp_pixel(257, &accent_ctx), 1);

    /* Guards. */
    CHECK_EQ(esl_generic_bmp_pixel(0, NULL), 1);
    EslGenericBmpCtx zero_ctx = {f, len, &info, 0, 0, false};
    CHECK_EQ(esl_generic_bmp_pixel(0, &zero_ctx), 1);
    free(f);
}

/* A stacked 2-plane generic source feeds the accent plane. */
static void test_generic_accent_plane(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(8, 8, 2, 1, 2, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 1, 0, 0); /* accent plane, source (0,0) */

    EslGenericBmpCtx ctx = {f, len, &info, 8, 8, true};
    CHECK_EQ(esl_generic_bmp_pixel(0, &ctx), 1);  /* primary untouched */
    CHECK_EQ(esl_generic_bmp_pixel(64, &ctx), 0); /* accent set */
    CHECK_EQ(esl_generic_bmp_pixel(65, &ctx), 1);
    free(f);
}

static void test_map(void) {
    /* Identity when sizes match. */
    CHECK_EQ(esl_bmp_map_x(0, 100, 100), 0);
    CHECK_EQ(esl_bmp_map_x(99, 100, 100), 99);
    /* Downscale by 2. */
    CHECK_EQ(esl_bmp_map_x(0, 50, 100), 0);
    CHECK_EQ(esl_bmp_map_x(49, 50, 100), 98);
    /* Upscale by 2 repeats source columns. */
    CHECK_EQ(esl_bmp_map_x(0, 100, 50), 0);
    CHECK_EQ(esl_bmp_map_x(1, 100, 50), 0);
    CHECK_EQ(esl_bmp_map_x(2, 100, 50), 1);
    /* Clamped, never out of range. */
    CHECK_EQ(esl_bmp_map_x(200, 100, 50), 49);
    CHECK_EQ(esl_bmp_map_y(200, 100, 50), 49);
    /* Degenerate inputs return 0 rather than dividing by zero. */
    CHECK_EQ(esl_bmp_map_x(5, 0, 50), 0);
    CHECK_EQ(esl_bmp_map_y(5, 50, 0), 0);
}

/* A glass-oriented (296x152) top-down 1bpp BMP with a single white pixel at
 * glass (0,151). The wire index that maps there is wire(0,0) -> glass(0,151),
 * which is wire index 0. */
static void test_color26_glass_orientation(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(296, 152, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 151, 0); /* top_down: row index == glass y */

    EslColor26BmpCtx ctx = {f, len, &info};

    /* BMP bit 1 (white) -> ESL 0. */
    CHECK_EQ(esl_color26_bmp_pixel(0, &ctx), 0);
    /* Neighbouring wire pixels are untouched -> ESL 1. */
    CHECK_EQ(esl_color26_bmp_pixel(1, &ctx), 1);
    CHECK_EQ(esl_color26_bmp_pixel(TAGTINKER_COLOR26_WIRE_W, &ctx), 1);

    /* A 1bpp source has no accent plane, so plane 2 reads all clear. */
    const size_t plane = (size_t)TAGTINKER_COLOR26_WIRE_W *
                         TAGTINKER_COLOR26_WIRE_H;
    CHECK_EQ(esl_color26_bmp_pixel(plane, &ctx), 1);
    free(f);
}

/* A wire-oriented (152x296) BMP is consumed without any transpose. */
static void test_color26_wire_orientation(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(152, 296, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 0, 0); /* wire (0,0) */

    EslColor26BmpCtx ctx = {f, len, &info};
    CHECK_EQ(esl_color26_bmp_pixel(0, &ctx), 0);
    CHECK_EQ(esl_color26_bmp_pixel(1, &ctx), 1);
    free(f);
}

/* A stacked 2-plane BMP feeds the accent plane from the second stack. */
static void test_color26_accent_plane(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(152, 296, 2, 1, 2, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 1, 0, 0); /* accent plane, wire (0,0) */

    EslColor26BmpCtx ctx = {f, len, &info};
    const size_t plane = (size_t)TAGTINKER_COLOR26_WIRE_W *
                         TAGTINKER_COLOR26_WIRE_H;

    CHECK_EQ(esl_color26_bmp_pixel(0, &ctx), 1);         /* primary untouched */
    CHECK_EQ(esl_color26_bmp_pixel(plane, &ctx), 0);     /* accent set */
    CHECK_EQ(esl_color26_bmp_pixel(plane + 1, &ctx), 1);
    free(f);
}

/* Reads past the end of the buffer must be clamped, never out of bounds. */
static void test_color26_bounds(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(152, 296, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    EslColor26BmpCtx ctx = {f, len, &info};
    const size_t total = (size_t)TAGTINKER_COLOR26_WIRE_W *
                         TAGTINKER_COLOR26_WIRE_H * 2U;
    for (size_t i = 0; i < total; i += 997) {
        uint8_t v = esl_color26_bmp_pixel(i, &ctx);
        CHECK(v == 0u || v == 1u);
    }
    free(f);
}

int main(void) {
    test_parse();
    test_map();
    test_color26_glass_orientation();
    test_color26_wire_orientation();
    test_color26_accent_plane();
    test_color26_bounds();
    test_generic_pixel();
    test_generic_accent_plane();
    TEST_REPORT("test_bmp");
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: FAIL — `fatal error: 'esl_bmp.h' file not found`.

- [ ] **Step 3: Write the header**

Create `src/modules/ir/esl/esl_bmp.h`:

```c
/* BMP reading for ESL image uploads.
 *
 * Consumes the BMPs produced by TagTinker's web image prep tool. Note that bpp
 * is a plane marker, not a real bit depth: 1 means one 1-bit plane, and 2 means
 * two stacked 1-bit planes (identical row stride) where the header height is
 * the per-plane height and the accent plane begins at row offset height.
 *
 * Pure logic with no filesystem or hardware dependency: the caller loads the
 * whole file into RAM first, which is also what the encode-then-transmit
 * ordering rule requires. */
#pragma once

/* Via esl_proto.h, never the vendored header directly: that keeps the vendor
 * symbols' linkage independent of include order in C++ callers. */
#include "esl_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t data_offset;
    uint32_t row_stride;
    uint16_t width;
    uint16_t height; /* per-plane height */
    uint16_t bpp;    /* 1, 2, 24 or 32 (1/2 are plane-count markers) */
    bool top_down;
} EslBmpInfo;

/* Parses the 54-byte header. Accepts bpp 1, 2, 24 and 32 — the same set as
 * upstream's tx_bmp_open — and computes upstream's per-bpp row stride.
 * Refusing non-1/2 belongs to the Color 2.6 send path, not here. Returns false
 * only for a bad magic, an unsupported bpp, or a too-short buffer. */
bool esl_bmp_parse(const uint8_t *file, size_t len, EslBmpInfo *info);

/* Nearest-neighbour coordinate mapping, clamped to the source extent. */
uint16_t esl_bmp_map_x(uint16_t out_x, uint16_t tx_w, uint16_t src_w);
uint16_t esl_bmp_map_y(uint16_t out_y, uint16_t tx_h, uint16_t src_h);

typedef struct {
    const uint8_t *file;
    size_t file_len;
    const EslBmpInfo *info;
} EslColor26BmpCtx;

/* TagTinkerPixelAtFn over the Color 2.6 wire space (152x296, two planes).
 * Handles the wire->glass transpose, source orientation and rescaling.
 * ctx must be an EslColor26BmpCtx*. */
uint8_t esl_color26_bmp_pixel(size_t idx, void *ctx);

typedef struct {
    const uint8_t *file;
    size_t file_len;
    const EslBmpInfo *info;
    uint16_t out_w; /* target profile width */
    uint16_t out_h; /* target profile height */
    bool second_plane;
} EslGenericBmpCtx;

/* TagTinkerPixelAtFn for every non-Color-2.6 dot-matrix profile: plain
 * nearest-neighbour rescale to out_w x out_h, with the accent plane taken from
 * a stacked 2-plane source or left clear. ctx must be an EslGenericBmpCtx*.
 * Total pixel count is out_w * out_h * (second_plane ? 2 : 1). */
uint8_t esl_generic_bmp_pixel(size_t idx, void *ctx);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 4: Write the implementation**

Create `src/modules/ir/esl/esl_bmp.c`:

```c
#include "esl_bmp.h"

#define ESL_BMP_HEADER_SIZE 54u

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

bool esl_bmp_parse(const uint8_t *file, size_t len, EslBmpInfo *info) {
    if (file == NULL || info == NULL || len < ESL_BMP_HEADER_SIZE) return false;
    if (file[0] != 'B' || file[1] != 'M') return false;

    /* Same accepted set as upstream tx_bmp_open. */
    const uint16_t bpp = rd16(&file[28]);
    if (bpp != 1u && bpp != 2u && bpp != 24u && bpp != 32u) return false;

    int32_t h = (int32_t)rd32(&file[22]);
    info->top_down = false;
    if (h < 0) {
        info->top_down = true;
        h = -h;
    }

    info->bpp = bpp;
    info->width = (uint16_t)rd32(&file[18]);
    info->height = (uint16_t)h;
    info->data_offset = rd32(&file[10]);

    /* Upstream's per-bpp strides. Markers 1 and 2 both store one bit per
     * pixel, so they share a stride. */
    if (bpp == 1u || bpp == 2u) {
        info->row_stride = (((uint32_t)info->width + 31u) / 32u) * 4u;
    } else if (bpp == 24u) {
        info->row_stride = (((uint32_t)info->width * 3u) + 3u) & ~3u;
    } else {
        info->row_stride = (uint32_t)info->width * 4u;
    }

    if (info->width == 0u || info->height == 0u) return false;
    if (info->data_offset >= len) return false;
    return true;
}

uint16_t esl_bmp_map_x(uint16_t out_x, uint16_t tx_w, uint16_t src_w) {
    if (tx_w == 0u || src_w == 0u) return 0u;
    uint32_t x = (uint32_t)out_x * (uint32_t)src_w / (uint32_t)tx_w;
    if (x >= src_w) x = (uint32_t)src_w - 1u;
    return (uint16_t)x;
}

uint16_t esl_bmp_map_y(uint16_t out_y, uint16_t tx_h, uint16_t src_h) {
    if (tx_h == 0u || src_h == 0u) return 0u;
    uint32_t y = (uint32_t)out_y * (uint32_t)src_h / (uint32_t)tx_h;
    if (y >= src_h) y = (uint32_t)src_h - 1u;
    return (uint16_t)y;
}

/* Maps a wire pixel to a source pixel, honouring the source's orientation:
 * already-wire BMPs pass straight through, glass BMPs are transposed, and any
 * other size is transposed then nearest-neighbour rescaled. */
static void color26_src_xy(const EslBmpInfo *info, uint16_t px, uint16_t py,
                           uint16_t *sx, uint16_t *sy) {
    if (info->width == TAGTINKER_COLOR26_WIRE_W &&
        info->height == TAGTINKER_COLOR26_WIRE_H) {
        *sx = px;
        *sy = py;
        return;
    }

    uint16_t gx = 0u, gy = 0u;
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, px, py, &gx, &gy);

    if (info->width == TAGTINKER_COLOR26_GLASS_W &&
        info->height == TAGTINKER_COLOR26_GLASS_H) {
        *sx = gx;
        *sy = gy;
        return;
    }

    *sx = esl_bmp_map_x(gx, TAGTINKER_COLOR26_GLASS_W, info->width);
    *sy = esl_bmp_map_y(gy, TAGTINKER_COLOR26_GLASS_H, info->height);
}

/* Returns the ESL bit at source (bx, by) in the given plane. Shared by both
 * pixel callbacks. Out-of-range reads return 1 (clear) rather than touching
 * memory. One bit per pixel, matching upstream's bmp_read_pixel. */
static uint8_t esl_bmp_bit(const uint8_t *file, size_t file_len,
                           const EslBmpInfo *info, uint16_t bx, uint16_t by,
                           uint8_t plane) {
    if (info->width == 0u || info->height == 0u) return 1u;
    if (bx >= info->width) bx = (uint16_t)(info->width - 1u);
    if (by >= info->height) by = (uint16_t)(info->height - 1u);

    const uint16_t row =
        info->top_down ? by : (uint16_t)(info->height - 1u - by);
    uint32_t off = info->data_offset +
                   ((uint32_t)row + (uint32_t)plane * (uint32_t)info->height) *
                       info->row_stride;
    off += (uint32_t)bx / 8u;
    if (off >= file_len) return 1u;

    /* BMP bit 1 is white, which is ESL 0. */
    const uint8_t bit = (uint8_t)((file[off] >> (7u - (bx % 8u))) & 1u);
    return bit ? 0u : 1u;
}

uint8_t esl_color26_bmp_pixel(size_t idx, void *ctx) {
    const EslColor26BmpCtx *c = (const EslColor26BmpCtx *)ctx;
    if (c == NULL || c->file == NULL || c->info == NULL) return 1u;

    const size_t plane_count = (size_t)TAGTINKER_COLOR26_WIRE_W *
                               (size_t)TAGTINKER_COLOR26_WIRE_H;
    uint8_t plane = 0u;
    if (idx >= plane_count) {
        plane = 1u;
        idx -= plane_count;
    }
    if (idx >= plane_count) return 1u;

    /* A 1-plane source has no accent data. */
    if (plane == 1u && c->info->bpp != 2u) return 1u;

    const uint16_t px = (uint16_t)(idx % TAGTINKER_COLOR26_WIRE_W);
    const uint16_t py = (uint16_t)(idx / TAGTINKER_COLOR26_WIRE_W);
    uint16_t bx = 0u, by = 0u;
    color26_src_xy(c->info, px, py, &bx, &by);
    return esl_bmp_bit(c->file, c->file_len, c->info, bx, by, plane);
}

uint8_t esl_generic_bmp_pixel(size_t idx, void *ctx) {
    const EslGenericBmpCtx *c = (const EslGenericBmpCtx *)ctx;
    if (c == NULL || c->file == NULL || c->info == NULL) return 1u;
    if (c->out_w == 0u || c->out_h == 0u) return 1u;

    const size_t plane_count = (size_t)c->out_w * (size_t)c->out_h;
    uint8_t plane = 0u;
    if (idx >= plane_count) {
        plane = 1u;
        idx -= plane_count;
    }
    if (idx >= plane_count) return 1u;

    /* A 1-plane source has no accent data, so the accent plane reads clear. */
    if (plane == 1u && c->info->bpp != 2u) return 1u;

    const uint16_t ox = (uint16_t)(idx % c->out_w);
    const uint16_t oy = (uint16_t)(idx / c->out_w);
    const uint16_t sx = esl_bmp_map_x(ox, c->out_w, c->info->width);
    const uint16_t sy = esl_bmp_map_y(oy, c->out_h, c->info->height);
    return esl_bmp_bit(c->file, c->file_len, c->info, sx, sy, plane);
}
```

- [ ] **Step 5: Run tests to verify they pass**

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: all four suites report `0 failures`, then `ALL TESTS PASSED`.

- [ ] **Step 6: Commit**

```bash
cd firmware
git add src/modules/ir/esl/esl_bmp.h src/modules/ir/esl/esl_bmp.c tools/esl_host_tests/test_bmp.c
git commit -m "feat(esl): add host-tested BMP parsing and Color 2.6 pixel mapping"
```

---

## Task 7: M1 image transmit UX

**Files:**
- Modify: `src/modules/ir/esl/esl_app.cpp` (replace the M0 bring-up body)

**Interfaces:**
- Consumes: everything produced by Tasks 1, 3, 4, 5 and 6 — in particular `esl_prompt_target` (written in Task 5, reused unchanged), `esl_bmp_parse`, `EslColor26BmpCtx` + `esl_color26_bmp_pixel`, `EslGenericBmpCtx` + `esl_generic_bmp_pixel`, `esl_tx_send_color26`, `esl_tx_send_generic(..., data_repeats)`, `ESL_GENERIC_DATA_REPEATS`, `tagtinker_profile_needs_wh_swap`, `tagtinker_encode_fn_payload`, `esl_ir_init/deinit/transmit/stop`. From Bruce: `keyboard(const String&, int, const String&, bool) -> String`, `loopSD(FS&, bool, const String&, String) -> String`, `setupSdCard(uint8_t) -> bool`, `loopOptions(options, MENU_TYPE_SUBMENU, title)` with the global `std::vector<Option> options` (`Option(const char*, std::function<void()>)`), `displayError/displayWarning/displaySuccess/displayTextLine`, `drawMainBorderWithTitle`, `progressHandler`, `check(EscPress)`, `returnToMenu`, `bruceConfigPins.irTx`, `checkIrTxPin()`.
- Produces: the final `void startEslTx(void)` behaviour, plus the file-scope helpers `esl_prompt_page`, `esl_pick_bmp`, `esl_read_file`. No new public symbols.

Fidelity requirements specific to this task:
- **Every dot-matrix profile is supported.** Do not gate the feature to Color 2.6.
- **The page is user-selectable 0–7**, mirroring `scene_image_options`. Position, compression and frame-repeat stay at upstream's fixed defaults.
- **Color 2.6 file cap is 24576** (`TX_COLOR26_BMP_MAX`); non-1/2 bpp is refused by the *send path*, matching upstream, not by the parser.

- [ ] **Step 1: Extend `esl_app.cpp` to the full M1 flow**

Rewrite `src/modules/ir/esl/esl_app.cpp`. `esl_prompt_target` and the ops callbacks carry over from Task 5 unchanged; the barcode prompt stays the only source of the tag address:

```cpp
#include "esl_app.h"

#include "esl_bmp.h"
#include "esl_ir_driver.h"
#include "esl_proto.h"
#include "esl_tx.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "modules/ir/TV-B-Gone.h"
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <globals.h>

#define ESL_BARCODE_LEN 17
#define ESL_PRE_TX_SETTLE_MS 500

/* Upstream's Color 2.6 file cap (TX_COLOR26_BMP_MAX). Do not raise it. */
#define ESL_COLOR26_BMP_MAX 24576u

/* Generic profiles have no upstream file cap because upstream streams rows
 * from SD. The encode-then-transmit rule forces us to hold the file in RAM
 * instead, so a bound is required: 256 KB clears the largest profile with
 * headroom (800x480 two-plane 1bpp is ~96 KB). */
#define ESL_GENERIC_BMP_MAX 262144u

struct EslUiCtx {
    bool aborted;
};

static bool ui_send(void *ctx, const uint8_t *frame, size_t len,
                    uint16_t repeats, uint8_t delay) {
    (void)ctx;
    return esl_ir_transmit(frame, len, repeats, delay);
}

static void ui_settle(void *ctx, uint32_t ms) {
    (void)ctx;
    delay(ms);
}

/* EslTxOps abort: consulted between frames by the sequencing layer. */
static bool ui_aborted(void *ctx) {
    EslUiCtx *c = (EslUiCtx *)ctx;
    if (!c->aborted && check(EscPress)) {
        c->aborted = true;
        esl_ir_stop();
    }
    return c->aborted;
}

/* Driver abort hook: consulted between *repeats* of a single frame. Needed as
 * well as ui_aborted because the 401-repeat wake burst happens inside one
 * esl_ir_transmit call, which the frame-level check cannot interrupt. */
static bool ui_abort_poll(void *ctx) {
    EslUiCtx *c = (EslUiCtx *)ctx;
    if (!c->aborted && check(EscPress)) c->aborted = true;
    return c->aborted;
}

static void ui_progress(void *ctx, size_t done, size_t total) {
    (void)ctx;
    progressHandler((int)done, total, "Sending ESL");
}

bool esl_prompt_target(uint8_t plid[4], TagTinkerTagProfile *profile) {
    String entered = keyboard("", ESL_BARCODE_LEN, "Tag barcode (17 chars):");
    entered.trim();

    /* Bruce's keyboard returns ESC when the user backs out. Treat that as a
     * silent cancel rather than scolding them about a length they never
     * entered. (ESC is not whitespace, so it survives trim().) */
    if (entered.length() == 0 || entered == "\x1B") return false;

    if (entered.length() != ESL_BARCODE_LEN) {
        displayError("Barcode must be 17 chars", true);
        return false;
    }
    if (!tagtinker_barcode_to_plid(entered.c_str(), plid)) {
        displayError("Bad barcode", true);
        return false;
    }
    if (!tagtinker_barcode_to_profile(entered.c_str(), profile)) {
        displayError("Unknown tag type", true);
        return false;
    }
    /* Segment tags have no image page, matching supports_graphics upstream. */
    if (profile->kind != TagTinkerTagKindDotMatrix) {
        displayError("Tag has no image page", true);
        return false;
    }
    return true;
}

/* Mirrors scene_image_options, where the page is the only per-image knob.
 * Position, compression and frame-repeat stay at upstream's defaults. */
static const char *ESL_PAGE_LABELS[8] = {"Page 0", "Page 1", "Page 2",
                                         "Page 3", "Page 4", "Page 5",
                                         "Page 6", "Page 7"};

static bool esl_prompt_page(uint8_t *page) {
    int chosen = -1;
    options.clear();
    for (uint8_t p = 0; p < 8; p++) {
        options.push_back(
            Option(ESL_PAGE_LABELS[p], [&chosen, p]() { chosen = (int)p; }));
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "Image page");
    if (chosen < 0) return false; /* user backed out */
    *page = (uint8_t)chosen;
    return true;
}

/* Picks a BMP from the SD card, falling back to LittleFS. */
static String esl_pick_bmp() {
    if (setupSdCard()) {
        String path = loopSD(SD, true, "BMP", "/");
        if (path != "") return path;
    }
    return loopSD(LittleFS, true, "BMP", "/");
}

/* Reads the whole file into PSRAM. Caller frees. */
static uint8_t *esl_read_file(const String &path, size_t max_bytes,
                              size_t *out_len) {
    fs::FS *fs = &SD;
    if (!SD.exists(path)) fs = &LittleFS;

    File f = fs->open(path, FILE_READ);
    if (!f) return nullptr;

    const size_t len = f.size();
    if (len < 54u || len > max_bytes) {
        f.close();
        return nullptr;
    }

    uint8_t *buf = (uint8_t *)ps_malloc(len);
    if (buf == nullptr) {
        f.close();
        return nullptr;
    }
    const size_t got = f.read(buf, len);
    f.close();

    if (got != len) {
        free(buf);
        return nullptr;
    }
    *out_len = len;
    return buf;
}

void startEslTx() {
    drawMainBorderWithTitle("ESL Image");

    uint8_t plid[4] = {0};
    TagTinkerTagProfile profile;
    if (!esl_prompt_target(plid, &profile)) {
        returnToMenu = true;
        return;
    }

    const String path = esl_pick_bmp();
    if (path == "") { /* user cancelled */
        returnToMenu = true;
        return;
    }

    uint8_t page = 0;
    if (!esl_prompt_page(&page)) {
        returnToMenu = true;
        return;
    }

    const bool is_color26 = tagtinker_profile_needs_wh_swap(&profile);
    const size_t cap = is_color26 ? ESL_COLOR26_BMP_MAX : ESL_GENERIC_BMP_MAX;

    /* --- Everything that touches the SD card happens before the IR pin is
     * claimed, because setup_ir_pin() may tear down the SD SPI bus. --- */
    size_t file_len = 0;
    uint8_t *file = esl_read_file(path, cap, &file_len);
    if (file == nullptr) {
        displayError("Cannot read BMP", true);
        returnToMenu = true;
        return;
    }

    EslBmpInfo info;
    if (!esl_bmp_parse(file, file_len, &info)) {
        free(file);
        displayError("Unsupported BMP", true);
        returnToMenu = true;
        return;
    }

    displayTextLine("Encoding...");

    TagTinkerImagePayload payload;
    bool encoded = false;
    uint16_t out_w = 0;
    uint16_t out_h = 0;

    if (is_color26) {
        /* Upstream's Color 2.6 send path is what refuses non-plane sources. */
        if (info.bpp != 1u && info.bpp != 2u) {
            free(file);
            displayError("Need 1/2bpp BMP", true);
            returnToMenu = true;
            return;
        }
        EslColor26BmpCtx ctx = {file, file_len, &info};
        const size_t total = (size_t)TAGTINKER_COLOR26_WIRE_W *
                             TAGTINKER_COLOR26_WIRE_H * 2U;
        encoded = tagtinker_encode_fn_payload(esl_color26_bmp_pixel, &ctx, total,
                                              TagTinkerCompressionAuto, &payload);
    } else {
        /* Accent plane follows the profile's colour capability, matching
         * upstream's use_second_plane with color_clear at its default. */
        out_w = profile.width;
        out_h = profile.height;
        const bool second_plane = (profile.color != TagTinkerTagColorMono);
        EslGenericBmpCtx ctx = {file, file_len, &info, out_w, out_h,
                                second_plane};
        const size_t total =
            (size_t)out_w * out_h * (second_plane ? 2U : 1U);
        encoded = tagtinker_encode_fn_payload(esl_generic_bmp_pixel, &ctx, total,
                                              TagTinkerCompressionAuto, &payload);
    }

    free(file); /* pixels are now baked into the payload */

    if (!encoded) {
        displayError("Encode failed", true);
        returnToMenu = true;
        return;
    }

    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        tagtinker_free_image_payload(&payload);
        displayError("IR init failed", true);
        returnToMenu = true;
        return;
    }

    drawMainBorderWithTitle("ESL Image");
    displayTextLine("Sending, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    EslUiCtx ui = {false};
    EslTxOps ops = {ui_send, ui_settle, ui_aborted, ui_progress, &ui};

    /* Both abort paths share the same ui.aborted latch: the driver hook covers
     * long single-frame bursts, ops.aborted covers frame boundaries. */
    esl_ir_set_abort_hook(ui_abort_poll, &ui);

    const bool ok =
        is_color26
            ? esl_tx_send_color26(&ops, plid, &payload, page)
            : esl_tx_send_generic(&ops, plid, &payload, page, out_w, out_h, 0u,
                                  0u, ESL_GENERIC_DATA_REPEATS);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();
    tagtinker_free_image_payload(&payload);

    if (ui.aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Sent", true);
    } else {
        displayError("TX failed", true);
    }
    returnToMenu = true;
}
```

- [ ] **Step 2: Confirm host tests still pass**

Nothing in this task changes pure logic, so the suites must be untouched:

```bash
cd firmware/tools/esl_host_tests && make test
```

Expected: `ALL TESTS PASSED`.

- [ ] **Step 3: Build**

```bash
cd firmware && /tmp/pio-venv/bin/pio run -e lilygo-t-embed-cc1101
```

Expected: `SUCCESS`.

- [ ] **Step 4: Prepare a test image**

Open `https://i12bp8.github.io/TagTinker` (or `TagTinker/web-image-prep/index.html` locally), pick **SmartTAG Color 2.6**, drop in any picture, and download the BMP. Copy it to the SD card root. Verify the exported size is either 296×152 (glass) or 152×296 (wire) — both are handled.

- [ ] **Step 5: Verify end-to-end on the tag**

Flash, then run `IR -> ESL Image`:

```bash
cd firmware && /tmp/pio-venv/bin/pio run -e lilygo-t-embed-cc1101 -t upload
```

1. Enter your tag's barcode at the prompt.
2. Pick the BMP.
3. Choose a page (leave it at `Page 0` to exercise the remap to the image slot).
4. Watch the progress bar advance through wake, param, data frames and refresh.

Expected: the image appears **upright and correctly oriented** on the tag, not rotated, mirrored, or overlapping the barcode. If it appears rotated 90°, the source-orientation branch in `color26_src_xy` chose the wrong path — check the BMP's actual header dimensions. If it lands on the barcode, check the resolved page is 2.

Then repeat with `Page 3` and confirm the image lands on a different page, proving the picker is wired through rather than ignored.

- [ ] **Step 6: Verify the error and abort paths**

Confirm each of these leaves the device usable, back at the menu, with the IR LED off:
- Enter a 5-character barcode → `Barcode must be 17 chars`.
- Enter `A4165420155299995` (valid shape, unknown type) → `Unknown tag type`.
- Cancel at the file picker → returns to the menu silently.
- Back out of the page picker → returns to the menu silently.
- Press Esc mid-send → `Aborted`.

The generic (non-1626) branch cannot be verified here — no such tag is available. It is covered by `test_tx` and `test_bmp` at the sequence and pixel-mapping level; state that limitation in the report rather than implying hardware coverage.

- [ ] **Step 7: Commit**

```bash
cd firmware
git add src/modules/ir/esl/esl_app.cpp
git commit -m "feat(esl): add M1 image transmit UX with barcode entry and BMP picker"
```

---

## Self-Review

**1. Spec coverage**

| Spec requirement | Task |
|---|---|
| Faithful vendored core, zero edits | Task 1 (steps 5-9, incl. byte-identity check) |
| `esl/protocol/` layout so `../tagtinker_app.h` resolves | Task 1 (steps 5-6) |
| `extern "C"` linkage rule | Task 1 (step 7), used in Tasks 4-7 |
| Do not call unimplemented protocol functions | Global Constraints |
| PP4 timing derived from 64 MHz cycle counts | Task 2 |
| RMT channel, 80 MHz resolution, copy encoder | Task 4 |
| Carrier 1.25 MHz / 0.49 / polarity / always_on | Task 4 |
| RMT idle level low, LED never idles on | Task 4 (`eot_level`, teardown), Task 5 (step 5) |
| Pinned repeat counts, both families | Task 3 (asserted in tests) |
| Encode-then-transmit invariant | Task 5 (step 2), Task 7 (step 1 ordering + comment) |
| `ps_malloc` for buffers | Task 7 (`esl_read_file`) |
| Color 2.6 transpose + orientation + rescale | Task 6 |
| Stacked 2-plane accent handling | Task 6 (`test_color26_accent_plane`) |
| Page remap (image never on barcode page) | Task 3 (`resolve_page`), asserted in `test_tx` |
| Barcode entry only, nothing hardcoded anywhere | Task 5 (`esl_prompt_target`), reused by Task 7 |
| Page user-selectable 0–7 (`scene_image_options` parity) | Task 7 (`esl_prompt_page`), verified in step 5 |
| All dot-matrix profiles supported | Task 6 (`esl_generic_bmp_pixel`), Task 7 (generic branch) |
| Per-family data pacing (50 ms/frame vs 1 ms/32 frames) | Task 3 (`EslTxProfile`), asserted in `test_tx` |
| bpp 1/2/24/32 accepted with upstream strides | Task 6 (`esl_bmp_parse`, `test_parse`) |
| Color 2.6 cap 24576; non-1/2 refused at send | Task 7 (`ESL_COLOR26_BMP_MAX`, send-path check) |
| `data_frame_repeats` threaded, not baked in | Task 3 (`test_generic_honours_data_repeats`) |
| No chunking in the image path | Not implemented anywhere; Global Constraints forbid it |
| BMP picker via `loopSD` | Task 7 (`esl_pick_bmp`) |
| Progress + Esc abort | Tasks 3, 5, 7 |
| Error handling (bad profile, bpp, size, malloc, abort) | Task 3 (guards), Task 6 (parse rejects), Task 7 (all paths) |
| Menu registration | Task 5 (step 3) |
| M0 scope + exit criteria | Task 5 (steps 5-6) |
| M1 exit criterion | Task 7 (step 5) |
| Build check on `lilygo-t-embed-cc1101` | Tasks 4, 5, 7 |
| Drop vestigial `signal_mode` / watchdog yields | Not carried over; `esl_tx.h` and `esl_ir_driver.cpp` have no such layer |

Gaps found and closed during the 1:1 fidelity review:

- **Hardcoded barcode removed.** M0 previously compiled in a barcode and sent a synthetic blank image — neither exists upstream. Task 5 is now a wake-frame-only timing checkpoint driven by the real barcode prompt.
- **Generic tags restored.** Task 7 previously refused non-1626 profiles. It now supports every dot-matrix profile via `esl_generic_bmp_pixel`, with the hardware-verification limitation stated rather than enforced as a feature gate.
- **Page picker added.** Upstream exposes the page; the plan had hardcoded it.
- **Data pacing split per family.** A single shared 50 ms policy would have given generic tags a gap they never see upstream.
- **bpp set and caps corrected** to upstream's (1/2/24/32, `TX_COLOR26_BMP_MAX` 24576).
- **Chunking correctly excluded.** An earlier draft treated it as a missing image-path feature; it is used only by the text path upstream, so it belongs to M2 and is now explicitly forbidden here.

**2. Placeholder scan**

No `TBD`, `TODO`, "implement later", or "similar to Task N". Every code step carries complete code; every command has an expected result. Generic dot-matrix tags are fully implemented and unit-tested; the only deferral there is *hardware* verification, since no non-1626 tag exists in this project — a stated limitation, not a feature gate.

**3. Type consistency**

- `esl_ir_transmit(const uint8_t*, size_t, uint16_t, uint8_t)` matches `EslTxOps::send` minus `ctx` — `ui_send` adapts them in Tasks 5 and 7.
- `esl_color26_bmp_pixel(size_t, void*) -> uint8_t` matches `TagTinkerPixelAtFn` exactly, so it passes directly to `tagtinker_encode_fn_payload`.
- `EslBmpInfo.height` means per-plane height in both Task 6's implementation and its tests.
- `EslColor26BmpCtx` field order `{file, file_len, info}` is identical in the header, the implementation, and every test initialiser.
- `esl_tx_step_count` is used for the progress total in Task 3 and asserted in `test_tx`.
- `ESL_IR_MAX_FRAME_LEN` (96) is `static_assert`-ed against `TAGTINKER_MAX_FRAME_SIZE` in Task 4 step 3.
- Repeat-count macro names are identical between `esl_tx.h` and `test_tx.c`.
