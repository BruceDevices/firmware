// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace wmbus {
enum class Mode { T1, C1A, C1B };
struct Frame {
    Mode mode = Mode::T1;
    std::vector<uint8_t> bytes; // DLL CRCs removed; L normalised to bytes.size()-1
    uint8_t wireLength = 0;
    int rssi = 0;
    uint8_t lqi = 0;
};
uint16_t crc(const uint8_t *data, size_t size);
size_t formatASize(uint8_t length);
bool decode3of6(const uint8_t *data, size_t size, size_t byteCount, std::vector<uint8_t> &out);
// Determine complete on-air byte count after the 0x543D sync word.
bool packetSize(const uint8_t *prefix, size_t size, size_t &total, Mode &mode);
bool decode(const uint8_t *radioBytes, size_t size, Frame &frame);
std::string hex(const uint8_t *data, size_t size);
std::string identity(const Frame &frame);
std::string manufacturer(const Frame &frame);
std::string deviceKey(const Frame &frame);
std::vector<std::string> describe(const Frame &frame);
} // namespace wmbus
