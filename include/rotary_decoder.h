#pragma once
#include <Arduino.h>

/*
 * Quadrature rotary encoder decoder.
 *
 * Faithful port of the Flipper-Zero-ESP32-Port target_input.c encoder
 * driver (encoder_poll()), including its sampling model, not just its
 * decode math:
 *
 *   - This is PURE POLLING, not interrupt-driven, exactly like Flipper's
 *     driver. poll() must be called from a fixed-cadence loop -- Flipper
 *     runs encoder_poll() on its own dedicated thread every 4ms,
 *     decoupled from GUI/app work; here poll() is called the same way,
 *     from a small dedicated FreeRTOS task (see taskEncoderPoll() in
 *     main.cpp) at a higher priority than the render loop, every 4ms,
 *     so a busy redraw can never delay sampling A/B. No attachInterrupt/
 *     ISR is used anywhere here.
 *   - Each poll() call takes exactly one gpio level sample of A and B
 *     and compares it against the level remembered from the previous
 *     poll() call. There is no edge queue and no loop inside poll() --
 *     it is a single compare-and-return, matching Flipper's function
 *     body exactly.
 *   - A direct, deliberate consequence of that (same as Flipper): if the
 *     encoder is spun fast enough to pass through more than one
 *     transition between two poll() calls, the intermediate transitions
 *     are simply never observed -- not queued, not caught up on later,
 *     just gone. This is not a speed check; it falls straight out of
 *     sample-based (not event-based) reading at a fixed cadence, same
 *     as the original.
 *   - Mechanical encoders bounce; naively reacting to "A changed, check
 *     B" produces false reverse steps. This tracks all 4 valid AB
 *     transitions via a lookup table and only counts a step on a
 *     *valid* transition, skipping (ignoring) anything else as bounce.
 *   - Steps are accumulated and a position change is only committed
 *     once a full detent's worth of valid transitions has been seen,
 *     blocking partial/half-detent noise from ever reaching callers.
 *   - No time-based idle/staleness reset exists here, same as Flipper --
 *     it does not exist in target_input.c's encoder logic either.
 *
 * Drop-in replacement for the RotaryEncoder library's getPosition():
 * call poll() once per fixed-rate InputHandler() pass, and read
 * getPosition() exactly as before.
 */

// Indexed by (old_AB << 2 | new_AB): 0 = invalid/bounce,
// +1 = CW quarter-step, -1 = CCW quarter-step.
//
// Sign is mirrored relative to Flipper's own table: Flipper's convention
// was tuned to their board's A/B wiring, which doesn't match how
// ENCODER_INA/ENCODER_INB are wired here, nor the sign the old
// RotaryEncoder library produced (which the existing InputHandler()
// direction logic -- posDifference > 0 => PrevPress, etc. -- was tuned
// against). Flipping it here, once, keeps direction correct without
// touching any per-board InputHandler() code.
static const int8_t ROTARY_DECODER_ENC_TABLE[16] = {
    /*          new: 00  01  10  11  */
    /* old 00 */  0, +1, -1,  0,
    /* old 01 */ -1,  0,  0, +1,
    /* old 10 */ +1,  0,  0, -1,
    /* old 11 */  0, -1, +1,  0,
};

class RotaryDecoder {
public:
    void begin(uint8_t pinA, uint8_t pinB, uint8_t stepsPerDetent = 2) {
        _pinA = pinA;
        _pinB = pinB;
        _stepsPerDetent = stepsPerDetent;
        bool a = digitalRead(_pinA);
        bool b = digitalRead(_pinB);
        _abState = (a << 1) | b;
        _accum = 0;
        _position = 0;
    }

    // Call once per fixed-cadence poll pass (e.g. once per InputHandler()
    // call). Takes a single level sample of A/B -- exactly like Flipper's
    // encoder_poll(), this is NOT called from an interrupt.
    void poll() {
        bool a = digitalRead(_pinA);
        bool b = digitalRead(_pinB);
        uint8_t newAb = (a << 1) | b;

        if (newAb == _abState) return; // no change since last poll

        int8_t delta = ROTARY_DECODER_ENC_TABLE[(_abState << 2) | newAb];
        _abState = newAb;

        if (delta == 0) return; // invalid transition (bounce) -- skip it

        _accum += delta;

        if (_accum >= _stepsPerDetent) {
            _accum = 0;
            _position++;
        } else if (_accum <= -_stepsPerDetent) {
            _accum = 0;
            _position--;
        }
    }

    int32_t getPosition() { return _position; }

private:
    uint8_t _pinA = 0;
    uint8_t _pinB = 0;
    uint8_t _stepsPerDetent = 2;
    uint8_t _abState = 0;
    int8_t _accum = 0;
    // Written by the dedicated encoder-poll task, read by InputHandler()
    // on a different task -- must be volatile so the reader can't cache
    // a stale value across the task boundary. Single 32-bit aligned word,
    // so plain reads/writes stay atomic on ESP32 without extra locking.
    volatile int32_t _position = 0;
};
