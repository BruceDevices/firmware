#ifndef __MIC
#define __MIC
/**
Notes for next mic implementations:
Some devices use GPIO Zero as Input (T-Embed, Smoochiee), ans the mic driver will set it as Output if no pin
is set So we need to check if the pin was set as Input, lock its state and reset it after finish the
function.


bool isGPIOOutput(gpio_num_t gpio) {
    if (gpio < 0 || gpio > 39) return false;

    if (gpio <= 31) {
        uint32_t reg_val = REG_READ(GPIO_ENABLE_REG);
        return reg_val & (1UL << gpio);
    } else {
        uint32_t reg_val = REG_READ(GPIO_ENABLE1_REG);
        return reg_val & (1UL << (gpio - 32));
    }
}
void mic_function() {
    // Devices that use GPIO 0 to navigation (or any other purposes) will break after start mic
    bool gpioInput = false;
    if (!isGPIOOutput(GPIO_NUM_0)) {
        gpioInput = true;
        gpio_hold_en(GPIO_NUM_0);
    }
        ...
        ...
        ...
    if (gpioInput) {
        gpio_hold_dis(GPIO_NUM_0);
        pinMode(GPIO_NUM_0, INPUT);
    }
}
 */

#include "core/display.h"
#include <fft.h>
#include <globals.h>

/* Mic */
void mic_test();
void mic_test_one_task();
void mic_record();
bool mic_record_wav_to_path(FS *fs, const String &path, uint32_t max_ms, bool stop_on_sel, uint32_t *out_bytes);

#endif
