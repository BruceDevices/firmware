/*
    AudioOutputI2S
    Base class for I2S interface port

    Copyright (C) 2017  Earle F. Philhower, III

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#ifdef ESP32
#include <driver/i2s_std.h>
#elif defined(ARDUINO_ARCH_RP2040) || ARDUINO_ESP8266_MAJOR >= 3
#include <I2S.h>
#elif ARDUINO_ESP8266_MAJOR < 3
#include <i2s.h>
#endif
#include "AudioOutputI2S.h"

AudioOutputI2S::AudioOutputI2S() {
    i2sOn = false;
    mono = false;
    lsb_justified = false;
    channels = 2;
    hertz = 44100; // Needs to be application-set
#ifdef ESP32
    bclkPin = 26;
    wclkPin = 25;
    doutPin = 22;
#else
    bclkPin = 26;
    wclkPin = 27;
    doutPin = 28;
#endif
    mclkPin = -1;
    SetGain(1.0);
    SetBuffers(); // Default DMA sizes
}

bool AudioOutputI2S::SetBuffers(int dmaBufferCount, int dmaBufferBytes) {
    if (i2sOn || (dmaBufferCount < 3) || (dmaBufferBytes & 3)) {
        return false;
    }
    _buffers = dmaBufferCount;
    _bufferWords = dmaBufferBytes / 4;
    return true;
}

#if defined(ESP32) || defined(ESP8266)
AudioOutputI2S::AudioOutputI2S(int port, int output_mode, int dma_buf_count, int use_apll) : AudioOutputI2S() {
    (void) port;
    (void) output_mode;
    (void) use_apll;
    SetBuffers(dma_buf_count, 128 * 4);
#ifdef ESP32
    _useAPLL = use_apll;
#endif
}

#if SOC_CLK_APLL_SUPPORTED
bool AudioOutputI2S::SetUseAPLL() {
    if (i2sOn) {
        return false;
    }
    _useAPLL = true;
    return true;
}
#endif

#elif defined(ARDUINO_ARCH_RP2040)
AudioOutputI2S::AudioOutputI2S(long sampleRate, pin_size_t sck, pin_size_t data) : AudioOutputI2S() {
    (void) sampleRate;
    bclkPin = sck;
    wclkPin = sck + 1;
    doutPin = data;
}
#endif

AudioOutputI2S::~AudioOutputI2S() {
    stop();
}

bool AudioOutputI2S::SwapClocks(bool swap) {
    if (i2sOn) {
        return false;
    }
    if (swap) {
        auto t = bclkPin;
        bclkPin = wclkPin;
        wclkPin = t;
    }
    return true;
}

bool AudioOutputI2S::SetPinout(int bclk, int wclk, int dout, int mclk) {
    if (i2sOn) {
        return false;
    }
    bclkPin = bclk;
    wclkPin = wclk;
    doutPin = dout;
    mclkPin = mclk;
    return true;
}

bool AudioOutputI2S::SetRate(int hz) {
    if (hertz == hz) {
        return true;
    }
    hertz = hz;
    if (i2sOn) {
        auto adj = AdjustI2SRate(hz);  // Possibly scale for NoDAC
#ifdef ESP32
        i2s_std_clk_config_t clk_cfg;
        clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)adj);
#if SOC_CLK_APLL_SUPPORTED
        clk_cfg.clk_src = _useAPLL ? i2s_clock_src_t::I2S_CLK_SRC_APLL : i2s_clock_src_t::I2S_CLK_SRC_DEFAULT;
#endif
        i2s_channel_disable(_tx_handle);
        i2s_channel_reconfig_std_clock(_tx_handle, &clk_cfg);
        i2s_channel_enable(_tx_handle);
#elif defined(ESP8266)
        i2s_set_rate(adj);
#elif defined(ARDUINO_ARCH_RP2040)
        i2s.setFrequency(adj);
#endif
    }
    return true;
}

bool AudioOutputI2S::SetChannels(int channels) {
    if ((channels < 1) || (channels > 2)) {
        return false;
    }
    this->channels = channels;
    return true;
}

bool AudioOutputI2S::SetOutputModeMono(bool mono) {
    this->mono = mono;
    return true;
}

bool AudioOutputI2S::SetLsbJustified(bool lsbJustified) {
    if (i2sOn) {
        return false;
    }
    this->lsb_justified = lsbJustified;
    return true;
}

bool AudioOutputI2S::begin() {
#ifdef ESP32
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = _buffers;
    chan_cfg.dma_frame_num = _bufferWords;
    assert(ESP_OK == i2s_new_channel(&chan_cfg, &_tx_handle, nullptr));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(hertz),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = mclkPin < 0 ? I2S_GPIO_UNUSED : (gpio_num_t)mclkPin,
            .bclk = bclkPin < 0 ? I2S_GPIO_UNUSED : (gpio_num_t)bclkPin,
            .ws = wclkPin < 0 ? I2S_GPIO_UNUSED : (gpio_num_t)wclkPin,
            .dout = (gpio_num_t)doutPin,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
#if SOC_CLK_APLL_SUPPORTED
    std_cfg.clk_cfg.clk_src = _useAPLL ? i2s_clock_src_t::I2S_CLK_SRC_APLL : i2s_clock_src_t::I2S_CLK_SRC_DEFAULT;
#endif
    std_cfg.slot_cfg.bit_shift = !lsb_justified; // I2S = shift, LSBJ = no shift
    assert(ESP_OK == i2s_channel_init_std_mode(_tx_handle, &std_cfg));

    // Fill w/0s to start off
    int16_t a[2] = {0, 0};
    size_t written = 0;
    do {
        i2s_channel_preload_data(_tx_handle, (void*)a, sizeof(a), &written);
    } while (written);

    i2sOn = (ESP_OK == i2s_channel_enable(_tx_handle));
#elif defined(ESP8266)
    if (!i2sOn) {
        orig_bck = READ_PERI_REG(PERIPHS_IO_MUX_MTDO_U);
        orig_ws = READ_PERI_REG(PERIPHS_IO_MUX_GPIO2_U);
#ifdef I2S_HAS_BEGIN_RXTX_DRIVE_CLOCKS
        if (!i2s_rxtxdrive_begin(false, true, false, true)) {
            return false;
        }
#else
        if (!i2s_rxtx_begin(false, true)) {
            return false;
        }
#endif
        i2sOn = true;
    }
#elif defined(ARDUINO_ARCH_RP2040)
    if (!i2sOn) {
        i2s.setSysClk(hertz);
        if (wclkPin == bclkPin + 1) {
            i2s.setBCLK(bclkPin);
        } else if (wclkPin == bclkPin - 1) {
            // Swapped!
            i2s.setBCLK(bclkPin - 1);
            i2s.swapClocks();
        } else {
            audioLogger->printf_P(PSTR("I2S: BCLK and WCLK must be adjacent\n"));
            return false;
        }
        i2s.setDATA(doutPin);
        if (mclkPin >= 0) {
            i2s.setMCLK(mclkPin);
            i2s.setMCLKmult(256);
        }
        i2s.setBitsPerSample(16);
        i2sOn = i2s.begin(hertz);
    }
#endif
    SetRate(hertz ? hertz : 44100); // Default
    return true;
}

bool AudioOutputI2S::ConsumeSample(int16_t sample[2]) {
    if (!i2sOn) {
        return false;
    }

    int16_t ms[2];

    ms[0] = sample[0];
    ms[1] = sample[1];
    MakeSampleStereo16(ms);

    if (this->mono) {
        // Average the two samples and overwrite
        int32_t ttl = ms[LEFTCHANNEL] + ms[RIGHTCHANNEL];
        ms[LEFTCHANNEL] = ms[RIGHTCHANNEL] = (ttl >> 1) & 0xffff;
    }
#ifdef ESP32
    uint32_t s32;
    s32 = ((Amplify(ms[RIGHTCHANNEL])) << 16) | (Amplify(ms[LEFTCHANNEL]) & 0xffff);

    size_t i2s_bytes_written = sizeof(uint32_t);
    i2s_channel_write(_tx_handle, (const char*)&s32, sizeof(uint32_t), &i2s_bytes_written, 0);
    return i2s_bytes_written;
#elif defined(ESP8266)
    uint32_t s32 = ((Amplify(ms[RIGHTCHANNEL])) << 16) | (Amplify(ms[LEFTCHANNEL]) & 0xffff);
    return i2s_write_sample_nb(s32); // If we can't store it, return false.  OTW true
#elif defined(ARDUINO_ARCH_RP2040)
    uint32_t s32 = ((Amplify(ms[RIGHTCHANNEL])) << 16) | (Amplify(ms[LEFTCHANNEL]) & 0xffff);
    return !!i2s.write((int32_t)s32, false);
#endif
}

#ifdef ARDUINO_ARCH_RP2040
uint16_t AudioOutputI2S::ConsumeSamples(int16_t *samples, uint16_t count) {
    // We special case the normal stereo, no gain case.  OTW just use the regular full-fat path
    if (this->mono || (gainF2P6 != 1 << 6)) {
        return AudioOutput::ConsumeSamples(samples, count);
    }
    auto ret = i2s.write((const uint8_t *)samples, count * 4);
    ret /= 4;
    return ret;
}
#endif


void AudioOutputI2S::flush() {
#ifdef ESP32
    // makes sure that all stored DMA samples are consumed / played
    int buffersize = _buffers * _bufferWords;
    int16_t samples[2] = {0x0, 0x0};
    for (int i = 0; i < buffersize; i++) {
        while (!ConsumeSample(samples)) {
            delay(10);
        }
    }
#elif defined(ARDUINO_ARCH_RP2040)
    i2s.flush();
#endif
}

bool AudioOutputI2S::stop() {
    if (!i2sOn) {
        return false;
    }

#ifdef ESP32
    i2s_channel_disable(_tx_handle);
    i2s_del_channel(_tx_handle);
#elif defined(ESP8266)
    i2s_end();
#elif defined(ARDUINO_ARCH_RP2040)
    i2s.end();
#endif
    i2sOn = false;
    return true;
}
