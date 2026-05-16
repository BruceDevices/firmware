/*
    AudioGeneratorOpus
    Audio output generator that plays Opus audio files

    Copyright (C) 2025  Earle F. Philhower, III

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

#ifndef _AUDIOGENERATOROPUS_H
#define _AUDIOGENERATOROPUS_H

#include <AudioGenerator.h>
#include "libopus/include/opus.h"

class AudioGeneratorOpus : public AudioGenerator {
public:
    AudioGeneratorOpus();
    virtual ~AudioGeneratorOpus() override;
    virtual bool begin(AudioFileSource *source, AudioOutput *output) override;
    virtual bool loop() override;
    virtual bool stop() override;
    virtual bool isRunning() override;

private:
    OpusDecoder *od = nullptr;

    uint8_t *packet; // Raw compressed, demuxed packet
    uint32_t packetOff;
    opus_int16 *buff; // Decoded PCM
    uint32_t buffPtr;
    uint32_t buffLen;

    bool demux();
    uint8_t hdr[27]; // Page header
    enum {WaitHeader, WaitSegment, ReadPacket} state;
    uint8_t type;
    uint64_t agp;
    uint32_t ssn;
    uint32_t psn;
    uint32_t pcs;
    uint16_t ps; // packet lacing segments
    uint16_t readPS;
    uint8_t seg[256]; // Packet lacing in the current page
    uint16_t curSeg;
    uint32_t lacingBytesToRead;
    void processPacket();
    // From the OpusHead
    uint16_t preskip;
    uint8_t channels;
    uint32_t samplerate;
    uint16_t gain;
};

#endif

