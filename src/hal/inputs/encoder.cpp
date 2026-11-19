#include "encoder.h"

#include "globals.h"

#include "core/powerSave.h"
// RotaryEncoder isn't in every board's lib_deps -- only boards that set
// HAS_ENCODER pull it in, so PlatformIO's LDF doesn't need to find it for
// everyone else.
#if defined(HAS_ENCODER)
#include <RotaryEncoder.h>

static RotaryEncoder *halEncoder = nullptr;

static IRAM_ATTR void halEncoderTick() { halEncoder->tick(); }

static RotaryEncoder::LatchMode toLibMode(EncoderLatchMode mode) {
    switch (mode) {
        case EncoderLatchMode::FOUR3: return RotaryEncoder::LatchMode::FOUR3;
        case EncoderLatchMode::FOUR0: return RotaryEncoder::LatchMode::FOUR0;
        default: return RotaryEncoder::LatchMode::TWO03;
    }
}

void hal_encoder_init(const DeviceEncoder &cfg, EncoderLatchMode mode) {
    if (cfg.pin_sel >= 0) {
        if (cfg.pullup) pinMode(cfg.pin_sel, INPUT_PULLUP);
        else pinMode(cfg.pin_sel, INPUT);
    }
    if (cfg.pin_esc >= 0) {
        if (cfg.pullup) pinMode(cfg.pin_esc, INPUT_PULLUP);
        else pinMode(cfg.pin_esc, INPUT);
    }

    halEncoder = new RotaryEncoder(cfg.pin_a, cfg.pin_b, toLibMode(mode));
    attachInterrupt(digitalPinToInterrupt(cfg.pin_a), halEncoderTick, CHANGE);
    attachInterrupt(digitalPinToInterrupt(cfg.pin_b), halEncoderTick, CHANGE);
}

void hal_encoder_poll(const DeviceEncoder &cfg) {
    static unsigned long tm = 0;
    static unsigned long tm2 = 0; // delay between an encoder step and Select (avoid missclick)
    static unsigned long lastMoveMs = 0;
    static int posDifference = 0;
    static long lastPos = 0;

    long newPos = halEncoder->getPosition();
    if (newPos != lastPos) {
        posDifference += (newPos - lastPos);
        // Independent running total for consumers that apply the whole
        // backlog in one pass (see drainRotarySteps() in globals.h). Never
        // cleared by the stale-drop below -- it's drained exactly.
        RotaryNetSteps += (newPos - lastPos);
        lastPos = newPos;
        lastMoveMs = millis();
    } else if (posDifference != 0 && millis() - lastMoveMs > 30) {
        // Drop any stale queued steps once the encoder has stopped moving.
        posDifference = 0;
    }

    if (millis() - tm < 200 && !LongPress) return;

    bool sel = cfg.pin_sel >= 0 && digitalRead(cfg.pin_sel) == LOW;
    bool esc = cfg.pin_esc >= 0 && digitalRead(cfg.pin_esc) == LOW;

    if (posDifference != 0 || sel || esc) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (posDifference > 0) {
        PrevPress = true;
        posDifference--;
#ifdef HAS_ENCODER_LED
        EncoderLedChange = -1;
#endif
        tm2 = millis();
    }
    if (posDifference < 0) {
        NextPress = true;
        posDifference++;
#ifdef HAS_ENCODER_LED
        EncoderLedChange = 1;
#endif
        tm2 = millis();
    }

    if (sel && millis() - tm2 > 200) {
        posDifference = 0;
        SelPress = true;
        tm = millis();
    }
    if (esc) {
        EscPress = true;
        tm = millis();
    }
}
#else
void hal_encoder_init(const DeviceEncoder &cfg, EncoderLatchMode mode) {
    (void)cfg;
    (void)mode;
}
void hal_encoder_poll(const DeviceEncoder &cfg) { (void)cfg; }
#endif
