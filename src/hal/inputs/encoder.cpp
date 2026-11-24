#include "encoder.h"

#include "globals.h"

#include "core/powerSave.h"
// RotaryEncoder isn't in every board's lib_deps -- only boards that set
// HAS_ENCODER pull it in, so PlatformIO's LDF doesn't need to find it for
// everyone else.
#if defined(HAS_ENCODER)
#include <RotaryEncoder.h>

static RotaryEncoder *halEncoder = nullptr;
static DeviceEncoder encoderConfig;
static EncoderButtonDebounce selectButton;
static EncoderButtonDebounce escapeButton;
static bool physicalSelectEvent = false;

static IRAM_ATTR void halEncoderTick() { halEncoder->tick(); }

static RotaryEncoder::LatchMode toLibMode(EncoderLatchMode mode) {
    switch (mode) {
        case EncoderLatchMode::FOUR3: return RotaryEncoder::LatchMode::FOUR3;
        case EncoderLatchMode::FOUR0: return RotaryEncoder::LatchMode::FOUR0;
        default: return RotaryEncoder::LatchMode::TWO03;
    }
}

void hal_encoder_init(const DeviceEncoder &cfg, EncoderLatchMode mode) {
    encoderConfig = cfg;
    if (cfg.pin_sel >= 0) {
        if (cfg.pullup) pinMode(cfg.pin_sel, INPUT_PULLUP);
        else pinMode(cfg.pin_sel, INPUT);
    }
    if (cfg.pin_esc >= 0) {
        if (cfg.pullup) pinMode(cfg.pin_esc, INPUT_PULLUP);
        else pinMode(cfg.pin_esc, INPUT);
    }

    selectButton.init(cfg.pin_sel >= 0 && digitalRead(cfg.pin_sel) == LOW, millis());
    escapeButton.init(cfg.pin_esc >= 0 && digitalRead(cfg.pin_esc) == LOW, millis());
    halEncoder = new RotaryEncoder(cfg.pin_a, cfg.pin_b, toLibMode(mode));
    attachInterrupt(digitalPinToInterrupt(cfg.pin_a), halEncoderTick, CHANGE);
    attachInterrupt(digitalPinToInterrupt(cfg.pin_b), halEncoderTick, CHANGE);
}

void hal_encoder_sample_buttons() {
    const uint32_t now = millis();
    if (selectButton.sample(encoderConfig.pin_sel >= 0 && digitalRead(encoderConfig.pin_sel) == LOW, now) &&
        selectButton.state.down) {
        selectButton.state.suppressed = wakeUpScreen();
    }
    if (escapeButton.sample(encoderConfig.pin_esc >= 0 && digitalRead(encoderConfig.pin_esc) == LOW, now) &&
        escapeButton.state.down) {
        escapeButton.state.suppressed = wakeUpScreen();
    }
}

EncoderButtonState hal_encoder_select_state() {
    inputLock();
    const EncoderButtonState state = selectButton.state;
    inputUnlock();
    return state;
}

void hal_encoder_reset_select_event() { physicalSelectEvent = false; }
bool hal_encoder_select_event_is_physical() { return physicalSelectEvent; }

static void pollEncoder() {
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
        const int32_t delta = newPos - lastPos;
        lastPos = newPos;
        lastMoveMs = millis();
        if (wakeUpScreen()) {
            posDifference = 0;
            RotaryNetSteps.exchange(0, std::memory_order_relaxed);
            return;
        }
        RotaryNetSteps.fetch_add(delta, std::memory_order_relaxed);
    } else if (posDifference != 0 && millis() - lastMoveMs > 30) {
        // Drop any stale queued steps once the encoder has stopped moving.
        posDifference = 0;
    }

    if (millis() - tm < 200 && !LongPress) return;

    // The 10 ms sampler owns GPIO reads and release detection, including while
    // the legacy 75/200 ms event gates are closed.
    bool sel = selectButton.state.down && !selectButton.state.suppressed;
    bool esc = escapeButton.state.down && !escapeButton.state.suppressed;

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
        physicalSelectEvent = true;
        tm = millis();
    }
    if (esc) {
        EscPress = true;
        tm = millis();
    }
}

void hal_encoder_poll(const DeviceEncoder &cfg) {
    (void)cfg; // init() owns the lifetime-stable configuration copy.
    // Some applications poll directly from loopTask as well as the input task.
    // Keep position, cached buttons and legacy event provenance in one update.
    inputLock();
    pollEncoder();
    inputUnlock();
}
#else
void hal_encoder_init(const DeviceEncoder &cfg, EncoderLatchMode mode) {
    (void)cfg;
    (void)mode;
}
void hal_encoder_poll(const DeviceEncoder &cfg) { (void)cfg; }
void hal_encoder_sample_buttons() {}
EncoderButtonState hal_encoder_select_state() { return {}; }
void hal_encoder_reset_select_event() {}
bool hal_encoder_select_event_is_physical() { return false; }
#endif
