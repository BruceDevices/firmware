#ifndef BRUCE_HAL_BRIGHT_H
#define BRUCE_HAL_BRIGHT_H

#include <stdint.h>

// Backlight/PWM brightness HAL.

#define HAL_BRIGHT_PWM_FREQ 5000
#define HAL_BRIGHT_PWM_RES_BITS 8

// duty = pwm_min + round(pow(percent/100, gamma) * (pwm_max - pwm_min))
// gamma = 1.0 is a plain linear ramp. Every board previously had its own
// hand-tuned lookup table or formula (surveyed in docs/etapa_8.md); they are
// unified onto this single curve so brightness behaves the same everywhere.
struct HalBrightCurve {
    uint16_t pwm_min = 0;
    uint16_t pwm_max = 255;
    float gamma = 2.2f;
};

uint16_t hal_bright_curve(uint8_t percent, const HalBrightCurve &curve = HalBrightCurve());

// Attaches one or more backlight pins to ledc at the shared HAL frequency/resolution.
void hal_bright_attach(const uint8_t *pins, uint8_t pin_count);
inline void hal_bright_attach(uint8_t pin) { hal_bright_attach(&pin, 1); }

// Writes the curve-mapped duty cycle to one or more backlight pins (e.g. screen +
// keyboard backlight on lilygo-t-lora-pager, warm+cool channels on xteink-x4pro).
// Retries once via ledcDetach()/ledcAttach() if the first ledcWrite() fails.
void hal_bright_set(
    const uint8_t *pins, uint8_t pin_count, uint8_t percent, const HalBrightCurve &curve = HalBrightCurve()
);
inline void hal_bright_set(uint8_t pin, uint8_t percent, const HalBrightCurve &curve = HalBrightCurve()) {
    hal_bright_set(&pin, 1, percent, curve);
}

#endif
