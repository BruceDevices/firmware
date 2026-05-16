/*
    AudioOutputPWM
    Base class for the RP2040 PWM audio output

    Copyright (C) 2023  Earle F. Philhower, III

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
#include <PWMAudio.h>

class AudioOutputPWM : public AudioOutput {
public:
    AudioOutputPWM(long sampleRate = 44100, pin_size_t data = 0);
    virtual ~AudioOutputPWM() override;
    virtual bool SetRate(int hz) override;
    virtual bool SetChannels(int channels) override;
    virtual bool begin() override;
    virtual bool ConsumeSample(int16_t sample[2]) override;
    virtual void flush() override;
    virtual bool stop() override;

    bool SetOutputModeMono(bool mono);  // Force mono output no matter the input

protected:
    bool SetPinout();
    virtual int AdjustI2SRate(int hz) {
        return hz;
    }
    uint8_t portNo;
    int output_mode;
    bool mono;
    bool pwmOn;

    uint8_t doutPin;

    PWMAudio pwm;
};

#endif
