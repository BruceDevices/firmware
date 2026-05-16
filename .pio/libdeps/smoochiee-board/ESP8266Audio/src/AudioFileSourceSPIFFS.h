/*
    AudioFileSourceFS
    Input Arduion "file" to be used by AudioGenerator

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

#ifndef _AUDIOFILESOURCESPIFFS_H
#define _AUDIOFILESOURCESPIFFS_H

#ifndef ARDUINO_ARCH_RP2040

#include <Arduino.h>
#include <FS.h>

#ifdef ESP32
#include <SPIFFS.h>
#endif

#include "AudioFileSource.h"
#include "AudioFileSourceFS.h"

// Yes, I know SPIFFS is deprecated
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

class AudioFileSourceSPIFFS : public AudioFileSourceFS {
public:
    AudioFileSourceSPIFFS() : AudioFileSourceFS(SPIFFS) { };
    AudioFileSourceSPIFFS(const char *filename) : AudioFileSourceFS(SPIFFS, filename) {};
    // Others are inherited from base
};

#endif

#endif

