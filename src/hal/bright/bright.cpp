#include "bright.h"
#include <Arduino.h>
#include <math.h>

uint16_t hal_bright_curve(uint8_t percent, const HalBrightCurve &curve) {
    if (percent > 100) percent = 100;
    float linear = percent / 100.0f;
    float scaled = curve.gamma == 1.0f ? linear : powf(linear, curve.gamma);
    return curve.pwm_min + (uint16_t)roundf(scaled * (curve.pwm_max - curve.pwm_min));
}

void hal_bright_attach(const uint8_t *pins, uint8_t pin_count) {
    for (uint8_t i = 0; i < pin_count; i++) ledcAttach(pins[i], HAL_BRIGHT_PWM_FREQ, HAL_BRIGHT_PWM_RES_BITS);
}

void hal_bright_set(const uint8_t *pins, uint8_t pin_count, uint8_t percent, const HalBrightCurve &curve) {
    uint16_t duty = hal_bright_curve(percent, curve);
    for (uint8_t i = 0; i < pin_count; i++) {
        if (!ledcWrite(pins[i], duty)) {
            ledcDetach(pins[i]);
            ledcAttach(pins[i], HAL_BRIGHT_PWM_FREQ, HAL_BRIGHT_PWM_RES_BITS);
            ledcWrite(pins[i], duty);
        }
    }
}
