
#pragma once
#if defined(ESP32) && SOC_DAC_SUPPORTED
#include "AudioOutput.h"
#include <driver/dac_continuous.h>

class AudioOutputInternalDAC : public AudioOutput {
    const int buffer_sz;
    std::vector<uint8_t> buffer;
    dac_continuous_handle_t dac_handle = nullptr;

    void dac_init() {
        ESP_LOGI("dac_init", "bps=%d, ch=%d, freq=%d", 16, channels, hertz);

        if (dac_handle) {
            dac_continuous_disable(dac_handle);
            dac_continuous_del_channels(dac_handle);
        }

        dac_continuous_config_t dac_config = {
            .chan_mask = channels == 2 ? DAC_CHANNEL_MASK_ALL : DAC_CHANNEL_MASK_CH0,
            .desc_num = 2, // we dont need more
            .buf_size = (size_t)buffer_sz,
            .freq_hz = hertz,
            .offset = 0,
            .clk_src = DAC_DIGI_CLK_SRC_APLL,
            .chan_mode = channels == 2 ? DAC_CHANNEL_MODE_ALTER : DAC_CHANNEL_MODE_SIMUL,
        };

        dac_continuous_new_channels(&dac_config, &dac_handle);
        dac_continuous_enable(dac_handle);
    }

    void dac_deinit() {
        if (dac_handle) {
            dac_continuous_disable(dac_handle);
            dac_continuous_del_channels(dac_handle);
            dac_handle = nullptr;
        }

        // prevent frying of your audio amplifier
        // dac pins like to go high after deinit

#if defined(CONFIG_IDF_TARGET_ESP32)
        int dac_1_gpio = 25, dac_2_gpio = 26;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
        int dac_1_gpio = 17, dac_2_gpio = 18;
#else
        static_assert(0, "AudioOutputInternalDAC: Unsupported Target " CONFIG_IDF_TARGET);
#endif

        pinMode(dac_1_gpio, OUTPUT);
        digitalWrite(dac_1_gpio, LOW);

        if (channels == 2) {
            pinMode(dac_2_gpio, OUTPUT);
            digitalWrite(dac_2_gpio, LOW);
        }
    }

    void dac_write(uint8_t* samples, uint16_t count) {
        if (buffer.size() + count >= buffer_sz) {
            size_t needed = buffer_sz - buffer.size();
            buffer.insert(buffer.end(), samples, samples + needed);
            samples += needed; count -= needed;

            dac_continuous_write(dac_handle, buffer.data(), buffer_sz, NULL, -1);
            buffer.clear();

            while (count >= buffer_sz) {
                dac_continuous_write(dac_handle, samples, buffer_sz, NULL, -1);
                samples += buffer_sz; count -= buffer_sz;
            }
        }

        if (count) {
            buffer.insert(buffer.end(), samples, samples + count);
        }
    }

public:
    // buffer_size should be in rannge [32, 4092]. large values can cause out
    // of memory situation which can have side effects like wifi not working
    AudioOutputInternalDAC(uint16_t buffer_size = 256): buffer_sz{buffer_size} {
        dac_deinit(); //sets the pins low until dac_init
    }

    ~AudioOutputInternalDAC() {
        dac_deinit();
    }

    uint16_t ConsumeSamples(int16_t* samples, uint16_t count) override {
        if (!dac_handle) {
            dac_init();
        }
        auto u8_samples = (uint8_t*)samples;

        for (int i = 0 ; i < count ; ++i) {
            u8_samples[i] = (samples[i] + 32768) / 257;
        }

        dac_write(u8_samples, count);
        return count;
    }

    bool ConsumeSample(int16_t samples[2]) {
        if (!dac_handle) {
            dac_init();
        }
        uint8_t u8_samples[2];

        u8_samples[0] = (samples[0] + 32768) / 257;
        u8_samples[1] = (samples[1] + 32768) / 257;

        dac_write(u8_samples, channels);
        return true;
    }

    bool begin() override {
        hertz = 44100;
        return true;
    }

    void flush() override {
        if (dac_handle) {
            buffer.resize(buffer_sz); buffer.back() = 0; //ensure last sample is 0
            dac_continuous_write(dac_handle, buffer.data(), buffer_sz, NULL, -1);
        }
        buffer.clear();
    }

    bool stop() override {
        flush();
        return true;
    }

    bool SetRate(int hz) override {
        if (hertz != hz) {
            hertz = hz;
            flush();
            dac_deinit();
        }
        return true;
    }

    bool SetChannels(int ch) override {
        if (channels != ch) {
            channels = ch;
            flush();
            dac_deinit();
        }
        return true;
    }
};
#endif
