// SPDX-License-Identifier: AGPL-3.0-or-later
#include "wmbus_codec.h"
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>
using Bytes = std::vector<uint8_t>;

static void addCRC(Bytes &wire, const Bytes &bytes, size_t start, size_t count) {
    wire.insert(wire.end(), bytes.begin() + start, bytes.begin() + start + count);
    uint16_t c = wmbus::crc(bytes.data() + start, count);
    wire.push_back(uint8_t(c >> 8)); wire.push_back(uint8_t(c));
}
static Bytes frameA(const Bytes &plain) {
    Bytes wire;
    addCRC(wire, plain, 0, 10);
    for (size_t p = 10; p < plain.size(); p += 16) addCRC(wire, plain, p, std::min(size_t(16), plain.size() - p));
    return wire;
}
static Bytes frameB(Bytes plain) {
    plain[0] = uint8_t(plain.size() - 1 + (plain.size() <= 126 ? 2 : 4));
    Bytes wire;
    if (plain.size() <= 126) addCRC(wire, plain, 0, plain.size());
    else { addCRC(wire, plain, 0, 126); addCRC(wire, plain, 126, plain.size() - 126); }
    wire.insert(wire.begin(), {0x54, 0x3D});
    return wire;
}
static Bytes encode(const Bytes &bytes) {
    const char *code[16] = {
        "010110","001101","001110","001011","011100","011001","011010","010011",
        "101100","100101","100110","100011","110100","110001","110010","101001"
    };
    std::string bits;
    for (uint8_t b : bytes) { bits += code[b >> 4]; bits += code[b & 15]; }
    while (bits.size() % 8) bits += '0';
    Bytes wire;
    for (size_t p = 0; p < bits.size(); p += 8) wire.push_back(uint8_t(std::stoi(bits.substr(p, 8), nullptr, 2)));
    return wire;
}
static bool has(const std::vector<std::string> &lines, const std::string &part) {
    for (const auto &l : lines) if (l.find(part) != std::string::npos) return true;
    return false;
}
static Bytes unhex(const std::string &text) {
    Bytes bytes;
    for (size_t p = 0; p < text.size(); p += 2) bytes.push_back(uint8_t(std::stoul(text.substr(p, 2), nullptr, 16)));
    return bytes;
}
int main() {
    const uint8_t check[] = {'1','2','3','4','5','6','7','8','9'};
    assert(wmbus::crc(check, 9) == 0xC2B7); // CRC-16/EN-13757 standard check value
    // Unencrypted short transport header: water meter, ID 12345678, manufacturer KAM.
    Bytes plain = {18,0x44,0x2D,0x2C,0x78,0x56,0x34,0x12,0x01,0x07,0x7A,0x10,0x00,0x00,0x00,0x04,0x13,0x12,0x00};
    wmbus::Frame f;
    Bytes wire = frameA(plain); wire.insert(wire.begin(), {0x54,0xCD});
    assert(wmbus::decode(wire.data(), wire.size(), f));
    assert(f.bytes == plain && f.mode == wmbus::Mode::C1A);
    assert(wmbus::identity(f) == "12345678" && wmbus::manufacturer(f) == "KAM");
    assert(has(wmbus::describe(f), "Plain application"));
    auto volume = f;
    volume.bytes.resize(15);
    volume.bytes.insert(volume.bytes.end(), {4,0x13,0xD2,4,0,0}); // 1234 * 10^-3 m3
    volume.bytes[0] = uint8_t(volume.bytes.size() - 1);
    assert(has(wmbus::describe(volume), "1.234 m3"));
    volume.bytes[16] = 0x93; // Extension flag without a supported primary VIF: no invented unit
    assert(!has(wmbus::describe(volume), "Value:"));
    // Published plaintext example from the wmbusmeters README, not a local
    // encoder-generated record: SEN water meter, volume 123.529 m3.
    auto published = f;
    published.bytes = unhex("1844AE4C4455223368077A55000000041389E20100023B0000");
    assert(wmbus::identity(published) == "33225544");
    assert(wmbus::manufacturer(published) == "SEN");
    assert(has(wmbus::describe(published), "123.529 m3"));
    auto changed = f; changed.bytes[3] ^= 1;
    assert(wmbus::deviceKey(f) != wmbus::deviceKey(changed));
    // Every one-bit corruption (including each block CRC and link header) is rejected.
    for (size_t p = 2; p < wire.size(); ++p) for (int bit = 0; bit < 8; ++bit) {
        auto bad = wire; bad[p] ^= uint8_t(1 << bit);
        assert(!wmbus::decode(bad.data(), bad.size(), f));
    }
    // All format A lengths, including >255 radio bytes and maximum 435-byte T frame.
    for (size_t length = 9; length <= 255; ++length) {
        Bytes data(length + 1);
        for (size_t i = 0; i < data.size(); ++i) data[i] = uint8_t(i * 19 + 3);
        data[0] = uint8_t(length);
        const auto a = frameA(data);
        assert(a.size() == wmbus::formatASize(uint8_t(length)));
        for (int mode = 0; mode < 2; ++mode) {
            auto raw = mode == 0 ? encode(a) : a;
            if (mode) raw.insert(raw.begin(), {0x54,0xCD});
            assert(wmbus::decode(raw.data(), raw.size(), f));
            assert(f.bytes == data);
            for (size_t cut = 0; cut < raw.size(); ++cut) assert(!wmbus::decode(raw.data(), cut, f));
            raw.push_back(0);
            assert(!wmbus::decode(raw.data(), raw.size(), f));
        }
    }
    // Both B CRC blocks, with the exact 128-byte boundary and long frames.
    for (size_t count = 11; count <= 252; ++count) {
        Bytes data(count, 0x43); data[0] = uint8_t(count - 1);
        auto raw = frameB(data);
        assert(wmbus::decode(raw.data(), raw.size(), f));
        assert(f.bytes == data && f.mode == wmbus::Mode::C1B);
        raw.back() ^= 1;
        assert(!wmbus::decode(raw.data(), raw.size(), f));
    }
    // Protected short/long transport payloads must never be labelled plaintext.
    f.bytes = plain; f.bytes[14] = 5;
    assert(has(wmbus::describe(f), "Protected payload") && !has(wmbus::describe(f), "Plain application"));
    assert(!has(wmbus::describe(f), "Record "));
    f.bytes = {26,0x44,0x2D,0x2C,0x78,0x56,0x34,0x12,1,7,0x72,0x78,0x56,0x34,0x12,0x2D,0x2C,1,7,1,0,0,5,1,2,3,4};
    assert(has(wmbus::describe(f), "TPL security mode: 5") && !has(wmbus::describe(f), "Plain application"));
    // Encrypted ELL II: session security = 1, so the next byte is ciphertext, not a CI.
    f.bytes = {21,0x44,0x2D,0x2C,0x78,0x56,0x34,0x12,1,7,0x8D,0x20,1,0,0,0,0x20,0,0,0x78,1,2};
    assert(has(wmbus::describe(f), "Encrypted payload") && !has(wmbus::describe(f), "Plain application"));
    // Bounds and unknown headers; the exact production decoder receives random bytes.
    std::mt19937 rng(13757);
    for (unsigned n = 0; n < 20000; ++n) {
        Bytes random(rng() % 460);
        for (auto &b : random) b = uint8_t(rng());
        wmbus::decode(random.data(), random.size(), f);
        f.bytes = random;
        wmbus::describe(f);
        // Force plaintext so random DIF/DIFE/VIF chains are exercised too.
        if (random.size() > 10) { f.bytes[10] = 0x78; wmbus::describe(f); }
    }
    std::cout << "PASS: CRC check value, T1/C1 A/B lengths, corruption, truncation, encryption boundaries, 20000 random inputs\n";
}
