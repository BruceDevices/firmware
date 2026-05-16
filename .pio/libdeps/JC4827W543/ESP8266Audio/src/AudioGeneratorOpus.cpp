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

#include <AudioGeneratorOpus.h>

AudioGeneratorOpus::AudioGeneratorOpus() {
    od = nullptr;
    buff = nullptr;
    buffPtr = 0;
    packet = nullptr;
    buffLen = 0;
    running = false;
}

AudioGeneratorOpus::~AudioGeneratorOpus() {
    if (od) {
        free(od);
    }
    od = nullptr;
    free(buff);
    buff = nullptr;
    free(packet);
    packet = nullptr;
}

bool AudioGeneratorOpus::begin(AudioFileSource *source, AudioOutput *output) {
    buff = (opus_int16*)malloc(4096 * sizeof(opus_int16));
    if (!buff) {
        return false;
    }

    packet = (uint8_t *)malloc(1024);
    if (!packet) {
        free(buff);
        buff = nullptr;
        return false;
    }
    packetOff = 0;

    od = (OpusDecoder *) malloc(opus_decoder_get_size(2));
    if (!od) {
        free(buff);
        buff = nullptr;
        free(packet);
        packet = nullptr;
        return false;
    }
    opus_decoder_init(od, 48000, 2);

    if (!source) {
        return false;
    }
    file = source;

    if (!output) {
        return false;
    }
    this->output = output;

    if (!file->isOpen()) {
        return false;    // Error
    }

    lastSample[0] = 0;
    lastSample[1] = 0;

    buffPtr = 0;
    buffLen = 0;

    bzero(hdr, sizeof(hdr));
    packetOff = 0;
    state = WaitHeader;
    preskip = 0;

    output->begin();

    // These are fixed by Opus
    output->SetRate(48000);
    output->SetChannels(2);

    running = true;
    return true;
}

bool AudioGeneratorOpus::demux() {
    int r;
    uint8_t b;
    //static int pktno = 0;

    while (true) {
        switch (state) {
        case WaitHeader:
            //audioLogger->printf("STATE: WaitHeader\n");
            if (!file->read(&b, 1)) {
                return false; // No data
            }
            memmove(hdr, hdr + 1, 26);
            hdr[26] = b;
            if ((hdr[0] == 'O') && (hdr[1] == 'g') && (hdr[2] == 'g') && (hdr[3] == 'S') && (hdr[4] == 0)) {
                // We have a header!
                type = hdr[5];
                agp = hdr[13];
                for (int i = 1; i < 7; i++) {
                    agp <<= 8;
                    agp |= hdr[13 - i];
                }
                ssn = hdr[14] | (hdr[15] << 8) | (hdr[16] << 16) | (hdr[17] << 24);
                psn = hdr[18] | (hdr[19] << 8) | (hdr[20] << 16) | (hdr[21] << 24);
                pcs = hdr[22] | (hdr[23] << 8) | (hdr[24] << 16) | (hdr[25] << 24);
                ps = hdr[26];
                readPS = 0;
                //audioLogger->printf("HEADER: typ: %d, agp: %llu, ssn: %08x, psn: %08x, pcs: %08x, ps: %d\n", type, agp, ssn, psn, pcs, ps);
                state = WaitSegment;
            }
            break;
        case WaitSegment:
            //audioLogger->printf("STATE: WaitSegment\n");
            r = file->read(seg + readPS, ps - readPS);
            readPS += r; // Keep track of what we read
            if (readPS < ps) {
                // Not enough data avail to finish, loop
                return false;
            }
            //audioLogger->printf("SEGMENTS (%d): ", ps);
            for (int i = 0; i < ps; i++) {
                //audioLogger->printf(" %02x", seg[i]);
            }
            //audioLogger->printf("\n");
            curSeg = 0;
            lacingBytesToRead = seg[0];
            state = ReadPacket;
            break;
        case ReadPacket:
            //audioLogger->printf("STATE: ReadPacket\n");
            //audioLogger->printf("readpacket seg %d len %d\n", curSeg, seg[curSeg]);
            r = file->read(packet + packetOff, lacingBytesToRead);
            packetOff += r;
            lacingBytesToRead -= r;
            if (lacingBytesToRead) {
                //audioLogger->printf("short read\n");
                return false; // Not enough avail
            }
            // Read lacing bit, see if there's more
            if (seg[curSeg] != 0xff) {
                //audioLogger->printf("PACKET %d (%d): ", pktno++, packetOff);
                for (uint32_t i = 0; i < packetOff; i++) {
                    //audioLogger->printf(" %02X", packet[i]);
                }
                //audioLogger->printf("\n");
                // We have a full packet in the buffer, decode it
                // First, is it a header?
                if ((packetOff >= 17) && !memcmp(packet, "OpusHead", 8) && (packet[8] == 1)) {
                    channels = packet[9];
                    preskip = packet[10] | (packet[11] << 8);
                    samplerate = packet[12] | (packet[13] << 8) | (packet[14] << 16) | (packet[15] << 24);
                    gain = packet[16] | (packet[17]++);
                    //audioLogger->printf("HEADER: chan: %d, sr: %d, skip %d\n", channels, samplerate, preskip);
                    packetOff = 0; // For better or worse, we've processed this pkt
                } else if ((packetOff >= 9) && !memcmp(packet, "OpusTags", 8)) {
                    // This is an unparsed TAG.  TODO find metadata format
                    packetOff = 0; // For better or worse, we've processed this pkt
                } else {
                    // This should be a regular packet
                    int ret = opus_decode(od, packet, packetOff, buff, 2048, 0);
                    packetOff = 0; // For better or worse, we've processed this pkt
                    if (ret > 0) {
                        buffLen = ret * 2;
                        if (preskip) {
                            if (buffLen >= preskip) {
                                buffPtr = preskip;
                                preskip = 0;
                                //audioLogger->printf("donepreskip\n");
                            } else {
                                buffPtr = buffLen;
                                preskip -= buffLen;
                            }
                        } else {
                            buffPtr = 0;
                        }
                        curSeg++;
                        if (curSeg >= ps) {
                            state = WaitHeader;
                            lacingBytesToRead = 0;
                        } else {
                            lacingBytesToRead = seg[curSeg];
                        }
                        return true; // We have filled a buffer
                    } else {
                        //audioLogger->printf("nodecode\n");
                        // Could be header pkt, we ignore them here
                        // That means we ignore the preskip value, for now
                    }
                    packetOff = 0; // For better or worse, we've processed this pkt
                }
            }
            // Only have partial pkt, need next segment
            curSeg++;
            if (curSeg >= ps) {
                state = WaitHeader;
                lacingBytesToRead = 0;
            } else {
                lacingBytesToRead = seg[curSeg];
            }
            break;
        default:
            state = WaitHeader;
            break;
        }
    }
}


bool AudioGeneratorOpus::loop() {
    if (!running) {
        goto done;
    }

    if (!output->ConsumeSample(lastSample)) {
        goto done;    // Try and send last buffered sample
    }

    do {
        if (buffPtr == buffLen) {
            // Will run until we either run out of data, would block, or decode something
            if (!demux()) {
                running = false;
                goto done;
            }
        }
        if (buffPtr == buffLen) {
            // Still nothing
            goto done;
        }

        lastSample[AudioOutput::LEFTCHANNEL] = buff[buffPtr] & 0xffff;
        lastSample[AudioOutput::RIGHTCHANNEL] = buff[buffPtr + 1] & 0xffff;
        buffPtr += 2;
    } while (running && output->ConsumeSample(lastSample));

done:
    file->loop();
    output->loop();

    return running;
}

bool AudioGeneratorOpus::stop() {
    if (od) {
        free(od);
    }
    od = nullptr;
    free(buff);
    buff = nullptr;
    running = false;
    output->stop();
    return true;
}

bool AudioGeneratorOpus::isRunning() {
    return running;
}
