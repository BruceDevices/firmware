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
#include <functional>
#include <globals.h>

/**
 * @brief Microphone recording configuration
 * @note Used by mic_record_app() and mic_record_wav_to_path()
 */
struct MicConfig {
    uint32_t record_time_ms; ///< Recording duration (0 = unlimited)
    float gain;              ///< Audio gain multiplier (0.5-4.0, default 2.0)
    bool stealth_mode;       ///< Enable low-brightness mode
};

/* Mic */
void mic_test();
void mic_test_one_task();
bool mic_record_wav_to_path(
    FS *fs, const String &path, uint32_t max_ms, uint32_t *out_bytes, float gain,
    std::function<bool(void)> onProgress = nullptr
);
void mic_record_app(); // Mic GUI app @Senape3000

/**
 * Capture raw audio samples from microphone
 * Returns array of 16-bit PCM samples
 *
 * @param numSamples - Number of samples to capture (64-4096)
 * @param sampleRate - Sample rate in Hz (8000, 16000, 22050, 32000, 44100, 48000)
 * @param gain - Audio gain multiplier (0.5-4.0)
 * @param outSamples - [OUT] Pointer to sample buffer (caller must free())
 * @param outSampleRate - [OUT] Actual sample rate used
 * @return true if successful
 */
bool mic_capture_samples(
    uint32_t numSamples, uint32_t sampleRate, float gain, int16_t **outSamples, uint32_t *outSampleRate
);
#endif
