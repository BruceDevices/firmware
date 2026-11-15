// SPDX-License-Identifier: AGPL-3.0-or-later
// Test-only transceiver backend for the actual Receiver implementation.
#pragma once
#include "wmbus_codec.h"
#include <algorithm>
#include <deque>
#include <vector>
#include <cassert>
#include <cstring>
#include <iostream>

constexpr int CC1101_SPI_MODULE = 1;
constexpr uint8_t CC1101_SIDLE = 0x36, CC1101_SFRX = 0x3A, CC1101_SRX = 0x34, CC1101_SCAL = 0x33;
constexpr uint8_t CC1101_MARCSTATE = 0x35, CC1101_RXBYTES = 0x3B, CC1101_RXFIFO = 0x3F;
constexpr uint8_t CC1101_PKTCTRL0 = 8, CC1101_PKTLEN = 6, CC1101_RSSI = 0x34, CC1101_LQI = 0x33;
static struct { int rfModule = CC1101_SPI_MODULE; } bruceConfigPins;
static uint32_t testTime = 100;
inline uint32_t millis() { return testTime; }
inline void delay(unsigned n) { testTime += n; }
inline void displayError(const char *, bool = false) {}
inline bool initRfModule(const char *, float) { return true; }
inline void deinitRfModule() {}

static struct FakeCC1101 {
    uint8_t regs[0x2F] = {};
    uint8_t state = 1;
    bool overflow = false, stuck = false, failRead = false;
    bool emptiedDuringRx = false;
    std::deque<uint8_t> fifo;
    void reset() { *this = FakeCC1101(); }
    uint8_t SpiReadReg(uint8_t r) { assert(r < sizeof(regs)); return regs[r]; }
    bool SpiWriteReg(uint8_t r, uint8_t v) { assert(r < sizeof(regs)); regs[r] = v; return true; }
    uint8_t SpiReadStatus(uint8_t r) {
        if (r == CC1101_MARCSTATE) return state;
        if (r == CC1101_RXBYTES) return uint8_t(fifo.size()) | (overflow ? 0x80 : 0);
        if (r == CC1101_RSSI) return 160;
        if (r == CC1101_LQI) return 51;
        assert(false); return 0;
    }
    bool SpiStrobe(uint8_t command) {
        if (stuck) return true;
        if (command == CC1101_SIDLE || command == CC1101_SCAL) state = 1;
        else if (command == CC1101_SRX) state = 13;
        else if (command == CC1101_SFRX) { assert(state == 1); fifo.clear(); overflow = false; }
        else assert(false);
        return true;
    }
    bool SpiReadBurstReg(uint8_t r, uint8_t *out, uint8_t n) {
        if (failRead) return false;
        assert(r == CC1101_RXFIFO && n <= fifo.size());
        if (n == fifo.size() && state == 13) emptiedDuringRx = true;
        for (unsigned i = 0; i < n; ++i) { out[i] = fifo.front(); fifo.pop_front(); }
        return true;
    }
    void feed(const uint8_t *p, size_t n) {
        assert(state == 13);
        for (size_t i = 0; i < n; ++i) fifo.push_back(p[i]);
        if (fifo.size() > 64) { fifo.resize(64); overflow = true; }
    }
} ELECHOUSE_cc1101;
