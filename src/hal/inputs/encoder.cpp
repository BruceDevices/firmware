#include "encoder.h"

#include "globals.h"

#include "core/powerSave.h"
#if defined(HAS_ENCODER)
#include <rotary_decoder.h>

static RotaryDecoder halDecoder;
static TaskHandle_t halEncoderPollTaskHandle = nullptr;

static int stepsPerDetent(EncoderLatchMode mode) {
    switch (mode) {
        case EncoderLatchMode::FOUR3:
        case EncoderLatchMode::FOUR0: return 4;
        default: return 2;
    }
}

static void halEncoderPollTask(void *parameter) {
    while (true) {
        halDecoder.poll();
        vTaskDelay(pdMS_TO_TICKS(4));
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

    pinMode(cfg.pin_a, INPUT_PULLUP);
    pinMode(cfg.pin_b, INPUT_PULLUP);
    halDecoder.begin(cfg.pin_a, cfg.pin_b, stepsPerDetent(mode));

    if (!halEncoderPollTaskHandle) {
        xTaskCreate(halEncoderPollTask, "EncoderPoll", 2048, NULL, 3, &halEncoderPollTaskHandle);
    }
}

void hal_encoder_poll(const DeviceEncoder &cfg) {
    static unsigned long tm = 0;
    static unsigned long tm2 = 0;
    static unsigned long lastMoveMs = 0;
    static int posDifference = 0;
    static long lastPos = 0;

    long newPos = halDecoder.getPosition();
    if (newPos != lastPos) {
        posDifference += (newPos - lastPos);
        RotaryNetSteps += (newPos - lastPos);
        lastPos = newPos;
        lastMoveMs = millis();
    } else if (posDifference != 0 && millis() - lastMoveMs > 30) {
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
