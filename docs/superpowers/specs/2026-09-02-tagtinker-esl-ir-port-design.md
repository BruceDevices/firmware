# Porting TagTinker's IR ESL flow into Bruce — Design (M0 + M1)

Date: 2026-09-02
Status: Approved for planning (M0 + M1 specced in detail; M2–M5 are an approved roadmap)

## Goal

Port the IR Electronic-Shelf-Label (ESL) transmit flow from the TagTinker Flipper Zero
app into the Bruce ESP32 firmware, ultimately reaching feature parity with TagTinker.

The end-to-end verification target is a **SmartTAG Color 2.6** tag (type code `1626`,
red accent, 296×152 landscape glass), which the project owner already drives
successfully from a Flipper via TagTinker PR #53
(`https://github.com/i12bp8/TagTinker/pull/53`, branch `add-smarttag-color-1626`).
That PR — not upstream `main` — is the source of truth for this tag, because it adds
both the `1626` profile and a distinct transmit path.

Full parity is the destination, delivered as sequenced milestones. This document
specs **M0** (IR engine first-light) and **M1** (image transmit UX) in detail and
records M2–M5 as an approved roadmap.

## Source and target, in brief

### TagTinker (source)
- The wire protocol is portable C: CRC-16 (`0x8408`), frame builders, RLE plane
  encoding, a ~40-entry tag-profile table, and PLID-from-barcode. ~570 LOC we lift
  almost verbatim (441 LOC upstream + ~124 added by PR #53).
- The IR driver is **not** portable: a Flipper STM32 `TIM1 CH3N` PWM carrier at
  ~1.2549 MHz with `DWT->CYCCNT` cycle-counted bursts/gaps ("PP4" — 2 bits/symbol).
  It does **not** use Flipper's `furi_hal_infrared` / InfraredSignal API.
- On-device image work is only nearest-neighbour rescale + RLE. Dithering/quantisation
  happens off-device in the browser `web-image-prep` tool, which exports a
  Flipper-ready 1bpp / stacked-2bpp BMP.

### Bruce (target)
- ESP32 (C++/Arduino/PlatformIO), many boards. Verification board: **LilyGo T-Embed
  CC1101** (ESP32-S3, 16 MB flash, PSRAM, full/non-LITE build, default IR TX GPIO 2,
  selectable via `bruceConfigPins.irTx`).
- Toolchain: pioarduino `platform-espressif32` 55.03.39 → Arduino-ESP32 3.3.x →
  **ESP-IDF v5**, so the modern `driver/rmt_tx.h` API with native
  `rmt_carrier_config_t` is available.
- IR features live under `src/modules/ir/` and register with one line in
  `src/core/menu_items/IRMenu.cpp`. `loopSD(fs, true, "BMP", "/")` provides a file
  picker. `drawBmp` is display-only 24-bit and is **not** usable as the TX pipeline —
  we bring our own BMP decoder.

## PP4 timing (pinned from `TagTinker/ir/tagtinker_ir.c`, 64 MHz cycle counts)

| Phase | Cycles @ 64 MHz | ≈ µs |
|-------|----------------:|-----:|
| Carrier (ARR=51, CCR=25) | — | ≈1.2549 MHz, ≈49% duty |
| Burst (every symbol) | 2581 | 40.33 |
| Gap, dibit value 0 | 3871 | 60.5 |
| Gap, dibit value 1 | 15483 | 241.9 |
| Gap, dibit value 2 | 7741 | 121.0 |
| Gap, dibit value 3 | 11612 | 181.4 |

Encoding: for each byte, LSB-first dibits (`sym = b & 3; b >>= 2`), emit
`carrier_on → burst → carrier_off → gap[sym]`; a single closing burst ends the frame.
The gap table is indexed by the **raw 2-bit value** (0..3). Frame length capped at 255.
`repeats = N` sends N+1 frames; `delay` between repeats is in units of 500 µs.

## Frame sequences and repeat counts (pinned — load-bearing)

Whether a tag latches depends on these counts, so they are specified exactly.
In every case `delay = 1` (500 µs between repeats of the same frame), and
`repeats = N` means **N+1 transmissions**.

### Color 2.6 / type 1626 (`tx_send_color26_payload`, PR #53)

| Stage | Frame | `repeats` | Sends | Notes |
|-------|-------|----------:|------:|-------|
| Wake | `make_wake_frame` (cmd `0x17` + 22×`0x01`) | 400 | 401 | `TX_COLOR26_WAKE_REPEATS` |
| — | | | | 50 ms settle |
| Param | `make_image_param_frame`, dims **152×296** (wire) | 1 | 2 | comp_type + resolved page |
| — | | | | 50 ms settle |
| Data | `make_image_data_frame` × `byte_count / 20` | 1 | 2 each | 50 ms between consecutive data frames |
| — | | | | 50 ms settle |
| Refresh | `make_refresh_frame` (MCU `0x01`) | 1 | 2 | |

### Generic dot-matrix tags (`tx_send_full_payload` → `tx_send_payload_frames`)

| Stage | Frame | `repeats` | Sends | Notes |
|-------|-------|----------:|------:|-------|
| Ping | `make_ping_frame` (cmd `0x97`) | 80 | 81 | |
| — | | | | 50 ms settle |
| Param | `make_image_param_frame` (MCU `0x05`) | 15 | 16 | via `tx_send_image_start` |
| — | | | | 50 ms settle |
| Data | `make_image_data_frame` × N | `data_frame_repeats` (default 2) | 3 each | **1 ms delay every 32 frames**, not between each |
| — | | | | 50 ms settle |
| Refresh | `make_refresh_frame` (MCU `0x01`) | 20 | 21 | |

**The two families pace data frames differently and must not share one policy:**

| | Color 2.6 | Generic |
|---|---|---|
| Data `repeats` | `1` (fixed) | `data_frame_repeats` (default 2) |
| Between data frames | **50 ms after every frame** | **1 ms after every 32nd frame** |

Conflating these is a fidelity bug: a generic tag would receive a 50 ms gap it
never sees upstream, and a Color 2.6 tag would lose the pacing it needs.

### Port simplifications (safe to drop)

- `tx_apply_signal_mode()` is a vestigial no-op (`return repeats & 0x7FFF`;
  `TagTinkerSignalPP4` is the only enum value). Do not carry the signal-mode layer over.
- TagTinker's "yield to the OS every 5 repeats" watchdog workaround is unnecessary:
  `rmt_tx_wait_all_done` blocks on a semaphore and yields to FreeRTOS naturally.
  Do not re-introduce it.

## Port fidelity: this is a 1:1 port

The governing rule: **Bruce must behave as TagTinker behaves.** Where upstream
has a constant, a threshold, a user-facing option, or a timing policy, the port
reproduces it rather than substituting a cleaner-looking choice. Anything that
cannot be reproduced identically is listed below as an explicit adaptation —
nothing is adapted silently.

### Reproduce exactly (no substitutions)

| Upstream | Value / behaviour |
|---|---|
| `TX_COLOR26_BMP_MAX` | 24576 bytes (Color 2.6 BMP file cap) |
| `tx_bmp_open` accepted bpp | **1, 2, 24, 32** — the Color 2.6 *send* is what rejects non-1/2 |
| Image page | User-selectable 0–7 via `scene_image_options`. **The `resolve_page` remap is a prepare-time *default*, not a wire invariant:** upstream applies it in `tagtinker_prepare_bmp_tx`, then the picker overwrites `job->page` with the raw index, and the send path transmits it with no second resolve. An explicit pick of page 0 must therefore reach the wire as 0. |
| Position / compression / frame-repeat in Image Options | Deliberately fixed at defaults (0,0 / auto RLE / ×2). Upstream comment: *"pages are the only knob that meaningfully changes per-image."* |
| `data_frame_repeats` | Default 2, threaded through as a parameter. The Settings screen exposing the 1–10 range is **M2** scope, so M1 correctly uses the default at the call site. |
| Data-frame pacing | Per-family, per the table above |
| Tag coverage | Every dot-matrix profile, not just Color 2.6 |
| Pre-TX settle | 500 ms before IR blasting for image/text jobs |

### Deliberate adaptations (platform-forced, behaviour-preserving)

| Adaptation | Why it does not change behaviour |
|---|---|
| Drop `tx_apply_signal_mode()` | Literally `return repeats & 0x7FFF` upstream; `TagTinkerSignalPP4` is the only mode |
| Drop the "yield every 5 repeats" watchdog workaround | RMT is hardware-timed and `rmt_tx_wait_all_done` yields on a semaphore |
| `EslTxOps` callbacks instead of Flipper scenes/threads | UI plumbing only; frame bytes and ordering unchanged |
| Generic path loads the BMP into RAM instead of streaming it from SD in two passes | Forced by the encode-then-transmit rule (SD may not be read after the IR pin is claimed). Produces a byte-identical payload; PSRAM makes the buffer cheap. Color 2.6 already loads the whole file upstream, so that path is unchanged. |

### Not image-path concerns (belongs to M2, text)

Chunked streaming — `tx_should_send_full_job` (`TX_FULL_JOB_PIXEL_LIMIT`
49152), `tx_pick_chunk_height` (8 KB/plane budget, 8-row alignment) and
`tx_chunk_settle_delay_ms` (clamp(500 + px/20, 800, 2000)) — is used **only** by
`tx_send_full_text_image` / `tx_stream_text_image`. `tx_stream_bmp_image` never
chunks. Do not add chunking to the image path; port it with text in M2.

## Architectural decisions (approved)

1. **Faithful vendored core.** Copy `tagtinker_proto.c/.h` (with PR #53 changes)
   verbatim into Bruce; write a thin Bruce C++ UI/orchestration layer and a new
   ESP32 IR driver on top. Keeps the wire protocol byte-identical and diffable
   against upstream; quarantines Flipper-isms behind one small driver + compat shim.
2. **RMT peripheral with hardware carrier** for the ESP32 IR engine. Hardware-accurate,
   jitter-immune (no interrupt-disabling needed). Rejected: IRremoteESP8266
   `mark`/`space` (software carrier can't hold a ~0.4 µs half-period reliably).

## Module layout

```
firmware/src/modules/ir/esl/
  tagtinker_app.h            # compat shim: COUNT_OF (satisfies proto.c's ../tagtinker_app.h)
  protocol/tagtinker_proto.c # VENDORED verbatim from PR #53 (CRC, RLE, profiles, color26, encode_fn)
  protocol/tagtinker_proto.h # VENDORED verbatim from PR #53
  esl_proto.h                # extern "C" wrapper for C++ consumers
  esl_pp4.h/.c               # pure PP4 symbol builder (host-tested)
  esl_tx.h/.c                # frame sequences w/ injectable ops (host-tested)
  esl_bmp.h/.c               # BMP header parse + pixel-callbacks (host-tested)
  esl_ir_driver.h/.cpp       # NEW ESP32 RMT PP4 driver (hardware shim)
  esl_font.h                 # VENDORED tagtinker_font.h (used from M2 for text)
  esl_app.h/.cpp             # Bruce UI + orchestration (replaces the Flipper scenes)
firmware/src/core/menu_items/IRMenu.cpp   # +1 entry: {"ESL Image", startEslTx}
firmware/tools/esl_host_tests/            # host test suite (outside src/, never compiled in)
```

The vendored files **must** live in an `esl/protocol/` subdirectory: `tagtinker_proto.c`
contains `#include "../tagtinker_app.h"`, so that path has to resolve to our shim at
`esl/tagtinker_app.h`. The shim supplies only `COUNT_OF`, which is all the protocol code
needs. Keeping the vendored files edit-free is a hard requirement: it is the proof that
Bruce speaks the exact wire format already validated on hardware. (Confirmed by
compiling them unmodified on the host with `-Wall -Wextra`.)

Pure logic (`esl_pp4`, `esl_tx`, `esl_bmp`) is plain C with no hardware dependencies so
it can be unit-tested with `cc` on the host; only `esl_ir_driver` and `esl_app` are C++
and hardware-bound.

### C / C++ linkage (must not be overlooked)

`tagtinker_proto.h` has **no `extern "C"` guard**, and `tagtinker_proto.c` is compiled
as C by PlatformIO. Its non-inline functions (`tagtinker_crc16`, `tagtinker_make_*`,
`tagtinker_encode_*`, `tagtinker_barcode_*`) would therefore name-mangle when included
from the C++ translation units and fail to link. `esl_compat.h` cannot fix this, because
the guard has to wrap the declarations themselves.

Rule: **C++ callers include the vendored header inside an `extern "C"` block**, which
preserves the zero-edit requirement:

```cpp
extern "C" {
#include "tagtinker_proto.h"
}
```

The `static inline` helpers in that header (`tagtinker_color26_*`,
`tagtinker_profile_*`) have internal linkage and are unaffected either way.

## M0 — RMT PP4 driver (first-light)

### Driver API (mirrors `tagtinker_ir.h`)
```c
void esl_ir_init(int gpio);
void esl_ir_deinit(void);
bool esl_ir_transmit(const uint8_t* data, size_t len, uint16_t repeats, uint8_t delay);
bool esl_ir_is_busy(void);
void esl_ir_stop(void);
```

### RMT configuration
- `rmt_new_tx_channel`: `gpio_num = bruceConfigPins.irTx`, `clk_src = RMT_CLK_SRC_DEFAULT`
  (APB/PLL 80 MHz), `resolution_hz = 80_000_000` (0.0125 µs/tick),
  `mem_block_symbols` sized for streaming (DMA optional on S3), `trans_queue_depth ≥ 1`.
- `rmt_apply_carrier`: `frequency_hz ≈ 1_250_000` (80 MHz ÷ 64), `duty_cycle = 0.49`,
  `flags.polarity_active_low = false` so the carrier lands on the **high/mark** level
  (matches the active-high IR LED; T-Embed `LED_ON = HIGH`), `flags.always_on = false`.
  1.25 MHz is within ~0.4% of the Flipper's 1.2549 MHz; IR demodulators tolerate far
  more. Note duty granularity is 1/64 ≈ 1.6% at this resolution, so `0.49` actually
  realises ~48.4% — close enough to the Flipper's ~49%. If empirical range/response is
  poor, tune `resolution_hz`/`frequency_hz` (documented constraint: carrier granularity
  vs the RMT 15-bit per-duration limit).
- Encoder: `rmt_new_copy_encoder`. It ping-pongs through `mem_block_symbols`, so a
  maximum-length 255-byte frame (255×4 + 1 = 1021 symbols) streams without a size cap.
- Symbol build: one `rmt_symbol_word_t` per PP4 dibit —
  `{level0=1, duration0=burst_ticks, level1=0, duration1=gap_ticks[dibit]}` (LSB-first),
  then a final closing-burst symbol. **Derive ticks at compile time from the 64 MHz
  cycle counts** (`cycles * resolution_hz / 64000000`) rather than hardcoding converted
  numbers. At 80 MHz that yields burst **3226** tk and gaps
  **{4838, 19353, 9676, 14515}** tk — all < 32767 (fits the 15-bit field).
- **Idle level must be LOW.** Set `rmt_transmit_config_t.flags.eot_level = 0` and
  explicitly `digitalWrite(irTx, LED_OFF)` on teardown. With `LED_ON = HIGH`, a
  default-high idle would leave the IR LED energised between frames and after TX.
- Transmit a whole frame with `rmt_transmit` + `rmt_tx_wait_all_done`. Loop for
  `repeats`, inserting `delay × 500 µs` between frames (`esp_rom_delay_us` / `vTaskDelay`).
  No critical section required — RMT timing is independent of CPU scheduling.
- `esl_ir_stop` sets a stop flag checked between frames and disables the carrier;
  `esl_ir_deinit` deletes the channel so it does not contend with FastLED's RMT usage.

### Encode-then-transmit ordering (hard invariant)

The invariant is **not** merely "BMP in RAM" — it is that the **complete encoded payload
is in RAM before the IR pin is claimed**. Both pipelines already satisfy this: they
produce a full `TagTinkerImagePayload` before any transmission, and the generic path
makes *two* SD passes while doing so. Sequence:

1. Read + encode with SD active → complete `TagTinkerImagePayload` in RAM.
2. `setup_ir_pin(bruceConfigPins.irTx, OUTPUT)` (may tear down SD SPI on some boards).
3. Transmit entirely from RAM.
4. Restore pin to `LED_OFF`, re-enable SD.

Framing it as "BMP in RAM" would wrongly permit the generic path's streaming reads to
straddle IR setup. Use `ps_malloc` for payload buffers: an 800×480 red tag is ~768k
pixels across two planes, which is well past the comfortable internal-RAM budget.

On the verification board this conflict happens to be moot — SD is `SDCARD_CS 13` plus
the shared SPI pins while IR is GPIO 2 — but the encode-first rule keeps the design
board-agnostic, which the "any IR-capable board" goal requires.

### M0 scope and exit criteria

M0 is a **timing checkpoint, not a feature**. It introduces no behaviour that
TagTinker does not have, and **nothing is hardcoded** — the barcode comes from
the same user prompt M1 uses, so the real addressing path is exercised from the
first run.

- Prompt for the tag barcode (Bruce `keyboard()`), derive PLID + profile with
  `tagtinker_barcode_to_plid` / `tagtinker_barcode_to_profile`.
- Emit **only the wake frame** (`tagtinker_make_wake_frame`, cmd `0x17` + 22×`0x01`,
  repeats 400). No image payload: upstream has no "send a blank image" feature and the
  port must not invent one.
- Exit criterion: logic analyzer / scope on the IR pin shows carrier ≈1.25 MHz,
  burst 40.3 µs, and the four distinct gaps {60.5, 121.0, 181.4, 241.9} µs (within a
  few %), with the line resting low between frames.

Making the tag visibly refresh is **M1's** gate, because that requires a real image
payload — which is M1's job, not a synthetic stand-in.

## M1 — Image TX (real UX)

### User flow

Mirrors upstream's `Set Image` flow: pick tag → pick image → choose page → send.

1. `IRMenu → "ESL Image"` → `startEslTx()`.
2. **Barcode entry** (no pre-filled default, nothing hardcoded): Bruce `keyboard()`
   collects the 17-char barcode; `tagtinker_barcode_to_plid` +
   `tagtinker_barcode_to_profile` derive PLID + profile. Unknown/invalid → error line,
   return. Segment-kind profiles are rejected (no image page), matching
   `tagtinker_target_supports_graphics`. The active target lives in RAM for the session
   (persistent multi-tag storage is M3).
3. **BMP pick:** `loopSD(fs, true, "BMP", "/")`. Empty return = user cancelled.
4. **Page pick:** a 0–7 page selector then "Send", mirroring `scene_image_options`.
   Position, compression and frame-repeat stay at upstream's fixed defaults (0,0 /
   auto RLE / ×2).
5. **Load + encode:** read the BMP into RAM (Color 2.6 capped at **24576** bytes, as
   upstream). Parse the header, accepting bpp **1, 2, 24, 32**. Encode with the vendored
   `tagtinker_encode_fn_payload` and the profile-appropriate pixel callback:
   - **Color 2.6 (type 1626):** transpose callback — iterate wire space (152×296), map
     each wire pixel to glass via `tagtinker_color26_proto_to_glass` (`bx=py`,
     `by=proto_w-1-px`), then map glass → source (identity for 152×296 wire or 296×152
     glass BMPs, else nearest-neighbour rescale), 1bpp or stacked-2bpp planes. Rejects
     bpp other than 1/2, as upstream does at send time.
   - **All other dot-matrix profiles:** nearest-neighbour rescale to the profile's
     dimensions, with the accent plane driven by the profile's colour capability —
     the same pixel selection `tx_stream_bmp_image` performs.
6. **Transmit:** profile-appropriate sequence via the M0 driver, using the exact repeat
   counts **and the per-family data pacing** pinned above. Color 2.6 uses
   `wake→param(152×296)→data→refresh`; generic uses `ping→param→data→refresh`. **Both
   transmit the caller's page verbatim.** `tagtinker_color26_resolve_page` seeds the
   picker's *default* for Color 2.6 so the out-of-the-box choice avoids the barcode page —
   exactly as upstream applies it at prepare time. It never rewrites an explicit pick.
7. **UI:** `drawMainBorderWithTitle("ESL Image")`, `progressHandler(frame_i, frame_count)`,
   `check(EscPress)` → `esl_ir_stop()`. On success `displaySuccess`, then restore pin/SD.

### M1 exit criterion
A web-image-prep BMP renders upright and correct on the physical Color 2.6 tag,
end-to-end from the Bruce menu, on a user-chosen page. The generic path compiles and
is unit-tested at the sequence and pixel-mapping level, but stays unverified on
hardware until a non-1626 tag is available — a limitation to state, not to hide.

## Data flow

```
barcode → PLID + profile
BMP (SD) ─┐
text/test ┤→ pixel-callback (transpose + NN rescale) → RLE/raw plane encode
          │→ frame builders (wake/ping, param, data, refresh)
          └→ RMT symbols (carrier + PP4 gaps) → IR LED → tag
```

## Error handling

- Invalid/unknown barcode or profile → error line, no TX.
- BMP bpp not 1/2, or size over the per-profile RAM cap → error line, no TX.
- `malloc`/RMT channel allocation failure → error line, safe teardown.
- IR pin unset/invalid → reuse Bruce's `checkIrTxPin()` to force selection.
- User abort (Esc) → `esl_ir_stop()`, carrier off, restore pin/SD.
- Always restore the IR pin to `LED_OFF` on exit.
- **SD remount is a known gap, not a delivered behaviour.** `setup_ir_pin()` calls `sdcardSPI.end()` only when the chosen IR pin conflicts with the SD bus. On the verification board it never fires (IR is GPIO 2; SD is CS 13 plus the shared SPI pins), and every existing Bruce IR feature has the same gap, so M1 does not remount. On a board where the IR pin *does* share the SD bus, one send would leave SD unmounted for the session. Fixing it properly means `closeSdCard()` + `setupSdCard()` after deinit — `setupSdCard()` alone returns early while `sdcardMounted` is still true.

## Testing / verification

- **M0:** logic-analyzer/scope capture of the IR pin (carrier + burst/gap vs the µs
  table); then a real Color 2.6 refresh.
- **M1:** correct, upright image on the physical tag from the Bruce menu.
- Any throwaway capture/analysis scripts live in `/tmp`, never committed.
- Build check: `pio run -e lilygo-t-embed-cc1101` (full, non-LITE).

## Constraints and risks

- **Carrier duty/accuracy at 1.25 MHz:** verify with a scope; tune `resolution_hz` /
  `frequency_hz` if the tag responds weakly.
- **Built-in IR LED range** at this carrier: the tag may need to be close; an external
  IR LED on a selectable pin is a fallback.
- **RMT contention with FastLED — confirmed, not hypothetical.** The verification board
  declares `HAS_RGB_LED`, 8× `WS2812B` on GPIO 14, and the build sets
  `FASTLED_RMT_BUILTIN_DRIVER=1`, so FastLED holds RMT channels on this exact hardware.
  Allocate the ESL channel only during TX and delete it after; never overlap with LED
  effects.
- **Total send time:** `wake ×400` ≈ 10 s+ per image; the progress UI and Esc-abort make
  this acceptable, and it is faithful to the working Flipper flow.
- **Flash/RAM:** T-Embed CC1101 is 16 MB / PSRAM, so budget is comfortable; on tight
  boards the feature can later be guarded by a build flag.

## Roadmap beyond M1 (approved, specced when reached)

- **M2 — Text + test patterns** on-device (vendored `esl_font.h`, `render_text_region_ex`,
  Color 2.6 text transpose path from PR #53). **Includes the chunked streaming path**
  (`tx_should_send_full_job` / `tx_pick_chunk_height` / `tx_chunk_settle_delay_ms` /
  `tx_send_image_chunk`), which upstream uses only for text — plus the Settings screen
  exposing `data_frame_repeats` (1–10).
- **M3 — Tag identify:** persistent saved targets; reuse Bruce NFC (PN532) to scan a
  tag's PLID/profile (TagTinker decodes Mifare Ultralight NDEF → 17-char barcode).
- **M4 — Broadcast payloads** (page change / diagnostic; PLID = 0 frames).
- **M5 — WiFi plugins:** Bruce is itself a WiFi ESP32, so plugins become "fetch/render
  a design → BMP → existing TX path" (same cloud worker, or render locally).

## Out of scope (for now)

- On-device dithering/quantisation (keep the browser web-image-prep flow; parity-faithful).
- Pushing any changes to the BruceDevices upstream (all work stays on the local
  `esl-ir-port` branch).
