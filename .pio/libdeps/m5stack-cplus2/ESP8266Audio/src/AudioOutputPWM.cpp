/*
    AudioOutputPWM
    Base class for the RP2040 PWM audio

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

#if defined(ARDUINO_ARCH_RP2040)
#include <Arduino.h>
#include "AudioOutputPWM.h"

AudioOutputPWM::AudioOutputPWM(long sampleRate, pin_size_t data) {
    pwmOn = false;
    mono = false;
    channels = 2;
    hertz = sampleRate;
    doutPin = data;
    SetGain(1.0);
    pwm.setStereo(true);
}

AudioOutputPWM::~AudioOutputPWM() {
    stop();
}

bool AudioOutputPWM::SetRate(int hz) {
    this->hertz = hz;
    if (pwmOn) {
        pwm.setFrequency(hz);
    }
    return true;
}

bool AudioOutputPWM::SetChannels(int channels) {
    if ((channels < 1) || (channels > 2)) {
        return false;
    }
    this->channels = channels;
    return true;
}

bool AudioOutputPWM::SetOutputModeMono(bool mono) {
    this->mono = mono;
    return true;
}

bool AudioOutputPWM::begin() {
    if (!pwmOn) {
        pwm.setPin(doutPin);
        pwm.setBuffers(8, 128);
        pwm.begin(hertz);
    }
    pwmOn = true;
    SetRate(hertz); // Default
    return true;
}

bool AudioOutputPWM::ConsumeSample(int16_t sample[2]) {
    //Serial.printf("%d\n", sample[0]);
    if (!pwmOn) {
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

    ms[LEFTCHANNEL] = Amplify(ms[LEFTCHANNEL]);
    ms[RIGHTCHANNEL] = Amplify(ms[RIGHTCHANNEL]);

    if (pwm.available()) {
        pwm.write((int16_t) ms[0]);
        pwm.write((int16_t) ms[1]);
        return true;
    } else {
        return false;
    }
}

void AudioOutputPWM::flush() {
    pwm.flush();
}

bool AudioOutputPWM::stop() {
    if (!pwmOn) {
        return false;
    }

    pwm.end();
    pwmOn = false;
    return true;
}

#endif
