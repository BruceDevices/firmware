/*
    AudioOutputI2S
    Base class for an I2S output port

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

#pragma once

#include "AudioOutput.h"

#if defined(ARDUINO_ARCH_RP2040)
#include <Arduino.h>
#include <I2S.h>
#elif defined(ESP32)
#include <driver/i2s_std.h>
#endif

class AudioOutputI2S : public AudioOutput {
public:
#if defined(ESP32) || defined(ESP8266)
#ifndef ESP8266
    [[deprecated("Use AudioOutputI2S(dmaBuffers, dmaBufferBytes) for normal I2S, AudioOutputPDM() for PDM mode, or AudioOutputInternalDAC() for DAC mode")]]
#endif
    AudioOutputI2S(int port, int output_mode = EXTERNAL_I2S, int dma_buf_count = 8, int use_apll = APLL_DISABLE);
    enum : int { APLL_AUTO = -1, APLL_ENABLE = 1, APLL_DISABLE = 0 };
    enum : int { EXTERNAL_I2S = 0, INTERNAL_DAC = 1, INTERNAL_PDM = 2 };
    AudioOutputI2S();
#elif defined(ARDUINO_ARCH_RP2040)
    [[deprecated]] AudioOutputI2S(long sampleRate, pin_size_t sck = 26, pin_size_t data = 28);
    AudioOutputI2S();
#endif
#if defined(ESP32) || defined(ESP8266)
#define __DMA_BUFF_BYTES (4608 / 2)
#else
#define __DMA_BUFF_BYTES 4608
#endif
    bool SetBuffers(int dmaBufferCount = 5, int dmaBufferBytes = __DMA_BUFF_BYTES);
#undef __DMA_BUFF_BYTES
    bool SetPinout(int bclkPin, int wclkPin, int doutPin, int mclkPin = -1);

#if defined(ESP32) && SOC_CLK_APLL_SUPPORTED
    bool SetUseAPLL();
#endif

    virtual ~AudioOutputI2S() override;
    virtual bool SetRate(int hz) override;
    virtual bool SetChannels(int channels) override;
    virtual bool begin() override;
    virtual bool ConsumeSample(int16_t sample[2]) override;
#ifdef ARDUINO_ARCH_RP2040
    // Have an optimized block pass-through for the Pico
    virtual uint16_t ConsumeSamples(int16_t *samples, uint16_t count) override;
#endif
    virtual void flush() override;
    virtual bool stop() override;

    [[deprecated("Use AudioOutputInternalDAC() for DAC output, begin(void) for normal I2S")]] bool begin(bool txDAC);
    bool SetOutputModeMono(bool mono);  // Force mono output no matter the input
    bool SetLsbJustified(bool lsbJustified);  // Allow supporting non-I2S chips, e.g. PT8211
    [[deprecated("Use SetPinout() with an MCLK pin to enable")]] bool SetMclk(bool enabled) {
        (void) enabled;
        return true;
    }
    [[deprecated("Use SetPinout() and specify the bclk/wclk to automatically set the clock swapping")]] bool SwapClocks(bool swap_clocks);  // Swap BCLK and WCLK

protected:
    virtual int AdjustI2SRate(int hz) {
        return hz;
    }
    bool mono;
    bool lsb_justified;
    bool i2sOn;

    int8_t bclkPin;
    int8_t wclkPin;
    int8_t doutPin;
    int8_t mclkPin;

    size_t _buffers;
    size_t _bufferWords;

#ifdef ESP32
    bool _useAPLL;
    // I2S IDF object and info
    i2s_chan_handle_t _tx_handle;
#elif defined(ARDUINO_ARCH_RP2040)
    // Normal software-defined I2S
    I2S i2s;
#elif defined(ESP8266)
    // We can restore the old values and free up these pins when in NoDAC mode
    uint32_t orig_bck;
    uint32_t orig_ws;
#endif
};
