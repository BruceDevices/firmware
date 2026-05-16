/*
    AudioOutputPDM
    Base class for a ESP32 PDM output port

    Copyright (C) 20125  Earle F. Philhower, III

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

#if defined(ESP32)

#include <driver/i2s_pdm.h>

#if SOC_I2S_SUPPORTS_PDM_TX

#include "AudioOutput.h"
#include "AudioOutputI2S.h"

class AudioOutputPDM : public AudioOutputI2S {
public:
    AudioOutputPDM(int pin = 0);

    bool SetPinout(int pdmPin);

    virtual ~AudioOutputPDM() override;
    virtual bool SetRate(int hz) override;
    virtual bool begin() override;

protected:
    virtual int AdjustI2SRate(int hz) {
        // TODO - There is some fixed off-by-ratio 1/1.25 in the PDM output clock vs. the PCM input data at IDF 5.5
        hz *= 8;
        hz /= 10;
        return hz;
    }

    int8_t pdmPin;
};

#endif

#endif
