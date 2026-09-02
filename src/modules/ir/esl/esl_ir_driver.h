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
