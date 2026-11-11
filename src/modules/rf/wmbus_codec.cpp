// SPDX-License-Identifier: AGPL-3.0-or-later
#include "wmbus_codec.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdio.h>

namespace wmbus {
uint16_t crc(const uint8_t *data, size_t size) {
    uint16_t c = 0;
    for (size_t i = 0; i < size; ++i) {
        c ^= uint16_t(data[i]) << 8;
        for (int b = 0; b < 8; ++b) c = (c & 0x8000) ? uint16_t((c << 1) ^ 0x3D65) : uint16_t(c << 1);
    }
    return uint16_t(~c);
}

size_t formatASize(uint8_t length) {
    if (length < 9) return 0;
    const size_t remaining = size_t(length) - 9;
    return size_t(length) + 3 + 2 * ((remaining + 15) / 16);
}

bool decode3of6(const uint8_t *data, size_t size, size_t count, std::vector<uint8_t> &out) {
    static const uint8_t symbols[16] = {
        0x16, 0x0D, 0x0E, 0x0B, 0x1C, 0x19, 0x1A, 0x13,
        0x2C, 0x25, 0x26, 0x23, 0x34, 0x31, 0x32, 0x29
    };
    out.clear();
    if (count > 290 || size < (count * 12 + 7) / 8) return false;
    for (size_t i = 0; i < count * 2; ++i) {
        uint8_t symbol = 0;
        for (size_t b = i * 6; b < i * 6 + 6; ++b)
            symbol = uint8_t((symbol << 1) | ((data[b / 8] >> (7 - b % 8)) & 1));
        size_t nibble = 0;
        while (nibble < 16 && symbols[nibble] != symbol) ++nibble;
        if (nibble == 16) { out.clear(); return false; }
        if (!(i & 1)) out.push_back(uint8_t(nibble << 4));
        else out.back() |= uint8_t(nibble);
    }
    return true;
}

bool packetSize(const uint8_t *p, size_t size, size_t &total, Mode &mode) {
    total = 0;
    if (size < 3) return false;
    if (p[0] == 0x54) {
        if (p[1] == 0xCD) {
            mode = Mode::C1A;
            const size_t n = formatASize(p[2]);
            if (!n) return false;
            total = n + 2;
        } else if (p[1] == 0x3D) {
            mode = Mode::C1B;
            // L includes CRC bytes for format B; 128 would leave no data in block 2.
            if (p[2] < 12 || p[2] == 128) return false;
            total = size_t(p[2]) + 3;
        } else return false;
    } else {
        mode = Mode::T1;
        std::vector<uint8_t> prefix;
        if (!decode3of6(p, size, 2, prefix)) return false;
        const size_t n = formatASize(prefix[0]);
        if (!n) return false;
        total = (n * 12 + 7) / 8;
    }
    return total <= 435;
}

static bool block(const uint8_t *p, size_t count, std::vector<uint8_t> &out) {
    if (crc(p, count) != uint16_t((uint16_t(p[count]) << 8) | p[count + 1])) return false;
    out.insert(out.end(), p, p + count);
    return true;
}

bool decode(const uint8_t *p, size_t size, Frame &frame) {
    frame.bytes.clear();
    size_t expected;
    Mode mode;
    if (!packetSize(p, size, expected, mode) || size != expected) return false;
    std::vector<uint8_t> decoded;
    if (mode == Mode::T1) {
        std::vector<uint8_t> prefix;
        if (!decode3of6(p, size, 2, prefix) || !decode3of6(p, size, formatASize(prefix[0]), decoded)) return false;
        p = decoded.data();
        size = decoded.size();
    } else { p += 2; size -= 2; }
    std::vector<uint8_t> out;
    if (mode == Mode::C1B) {
        const size_t first = size <= 128 ? size - 2 : 126;
        if (!block(p, first, out)) return false;
        if (size > 128 && !block(p + 128, size - 130, out)) return false;
    } else {
        if (!block(p, 10, out)) return false;
        size_t pos = 12;
        while (pos < size) {
            if (size - pos < 3) return false;
            const size_t n = std::min(size_t(16), size - pos - 2);
            if (!block(p + pos, n, out)) return false;
            pos += n + 2;
        }
    }
    if (out.size() < 10 || out.size() > 256) return false;
    frame.mode = mode;
    frame.wireLength = p[0];
    out[0] = uint8_t(out.size() - 1);
    frame.bytes.swap(out);
    return true;
}

std::string hex(const uint8_t *p, size_t n) {
    static const char digits[] = "0123456789ABCDEF";
    std::string s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s += digits[p[i] >> 4]; s += digits[p[i] & 15]; }
    return s;
}
static std::string hx(uint8_t b) { return hex(&b, 1); }
std::string identity(const Frame &f) {
    if (f.bytes.size() < 10) return "?";
    std::string s;
    for (int i = 7; i >= 4; --i) s += hx(f.bytes[i]);
    return s;
}
std::string manufacturer(const Frame &f) {
    if (f.bytes.size() < 10) return "???";
    const unsigned m = f.bytes[2] | (unsigned(f.bytes[3]) << 8);
    std::string s;
    for (int shift = 10; shift >= 0; shift -= 5) {
        const unsigned c = (m >> shift) & 31;
        s += c >= 1 && c <= 26 ? char('A' + c - 1) : '?';
    }
    return s;
}
std::string deviceKey(const Frame &f) {
    if (f.bytes.size() < 10) return "?";
    return hex(f.bytes.data() + 2, 8); // manufacturer + full address, not ID alone
}
static const char *medium(uint8_t b) {
    switch (b) {
        case 0x00: return "Other";
        case 0x02: return "Electricity";
        case 0x03: return "Gas";
        case 0x04: return "Heat";
        case 0x06: return "Warm water";
        case 0x07: return "Water";
        case 0x08: return "Heat cost allocator";
        case 0x0C: return "Heat (inlet)";
        case 0x16: return "Cold water";
        default: return "Other/reserved";
    }
}

// Decode standard numeric records only when the surrounding transport explicitly
// declares plaintext. Extended/custom VIFs remain raw to avoid misleading units.
static void records(const std::vector<uint8_t> &b, size_t pos, std::vector<std::string> &lines) {
    unsigned record = 0;
    static const int sizes[15] = {0,1,2,3,4,4,6,8,0,1,2,3,4,-1,6};
    while (pos < b.size()) {
        const size_t start = pos;
        const uint8_t dif = b[pos++];
        if (dif == 0x2F) continue;
        const unsigned type = dif & 15;
        if (type == 15) { lines.push_back("Manufacturer data / continuation: raw"); return; }
        uint8_t ext = dif;
        while ((ext & 0x80) && pos < b.size()) ext = b[pos++];
        if ((ext & 0x80) || pos >= b.size()) { lines.push_back("Truncated record header"); return; }
        const uint8_t vif = b[pos++];
        if ((vif & 0x7F) == 0x7C) { lines.push_back("Custom text VIF: see raw data"); return; }
        ext = vif;
        while ((ext & 0x80) && pos < b.size()) ext = b[pos++];
        if (ext & 0x80) { lines.push_back("Truncated VIF extension"); return; }
        const int count = sizes[type];
        if (count < 0) { lines.push_back("Variable-length record: see raw data"); return; }
        if (size_t(count) > b.size() - pos) { lines.push_back("Truncated record value"); return; }
        lines.push_back("Record " + std::to_string(++record) + " header: " + hex(b.data() + start, pos - start));
        lines.push_back("Value hex LE: " + hex(b.data() + pos, size_t(count)));
        double value = 0;
        bool numeric = count > 0;
        if (type >= 9) {
            double power = 1;
            for (int i = 0; i < count; ++i) {
                const unsigned lo = b[pos + i] & 15, hi = b[pos + i] >> 4;
                if (lo > 9 || hi > 9) { numeric = false; break; }
                value += (lo + 10 * hi) * power; power *= 100;
            }
        } else if (type == 5) {
            uint32_t bits = 0;
            for (int i = 0; i < 4; ++i) bits |= uint32_t(b[pos + i]) << (8 * i);
            float v;
            std::memcpy(&v, &bits, sizeof(v));
            value = v;
            numeric = std::isfinite(value);
        } else {
            double power = 1;
            for (int i = 0; i < count; ++i) {
                const int digit = i == count - 1 && (b[pos + i] & 0x80) ? int(b[pos + i]) - 256 : b[pos + i];
                value += digit * power; power *= 256;
            }
        }
        // Only primary VIF values without extensions are assigned a scale/unit.
        int exponent = 0;
        const char *unit = nullptr;
        if (vif < 0x08) { exponent = (vif & 7) - 3; unit = "Wh"; }
        else if (vif < 0x10) { exponent = vif & 7; unit = "J"; }
        else if (vif < 0x18) { exponent = (vif & 7) - 6; unit = "m3"; }
        else if (vif < 0x20) { exponent = (vif & 7) - 3; unit = "kg"; }
        else if (vif >= 0x28 && vif < 0x30) { exponent = (vif & 7) - 3; unit = "W"; }
        else if (vif >= 0x38 && vif < 0x40) { exponent = (vif & 7) - 6; unit = "m3/h"; }
        else if (vif >= 0x58 && vif < 0x60) { exponent = (vif & 3) - 3; unit = "degC"; }
        else if (vif >= 0x68 && vif < 0x6C) { exponent = (vif & 3) - 3; unit = "bar"; }
        if (numeric && unit) {
            char text[96];
            snprintf(text, sizeof(text), "Value: %.12g %s (function %u)", value * std::pow(10.0, exponent), unit, (dif >> 4) & 3);
            lines.push_back(text);
        }
        pos += size_t(count);
    }
}

std::vector<std::string> describe(const Frame &f) {
    std::vector<std::string> lines;
    const auto &b = f.bytes;
    if (b.size() < 10) return {"Invalid DLL header"};
    lines.push_back("ID: " + identity(f));
    lines.push_back("Manufacturer: " + manufacturer(f) + " / " + hex(b.data() + 2, 2));
    lines.push_back("Version: 0x" + hx(b[8]) + " Type: 0x" + hx(b[9]));
    lines.push_back(std::string("Medium: ") + medium(b[9]));
    lines.push_back("C: 0x" + hx(b[1]) + " Wire L: " + std::to_string(f.wireLength));
    lines.push_back(std::string("Mode: ") + (f.mode == Mode::T1 ? "T1 A" : f.mode == Mode::C1A ? "C1 A" : "C1 B"));
    lines.push_back("RSSI: " + std::to_string(f.rssi) + " dBm LQI: " + std::to_string(f.lqi));
    size_t pos = 10;
    bool plain = false;
    bool blocked = false;
    if (pos < b.size() && b[pos] >= 0x8C && b[pos] <= 0x8F) {
        const uint8_t ci = b[pos++];
        const bool target = ci == 0x8E || ci == 0x8F;
        const bool session = ci == 0x8D || ci == 0x8F;
        const size_t need = 2 + (target ? 8 : 0) + (session ? 6 : 0);
        lines.push_back("ELL CI: 0x" + hx(ci));
        if (b.size() - pos < need) { lines.push_back("Truncated ELL header"); blocked = true; }
        else {
            lines.push_back("ELL CC: " + hx(b[pos]) + " Access: " + hx(b[pos + 1]));
            pos += 2;
            if (target) { lines.push_back("ELL target: " + hex(b.data() + pos, 8)); pos += 8; }
            if (session) {
                const uint8_t security = b[pos + 3] >> 5;
                lines.push_back("ELL session: " + hex(b.data() + pos, 4));
                lines.push_back("ELL payload CRC: " + hex(b.data() + pos + 4, 2));
                lines.push_back("ELL security: " + std::to_string(security));
                pos += 6;
                if (security) { blocked = true; lines.push_back("Encrypted payload (no key)"); }
            }
        }
    }
    if (!blocked && pos < b.size()) {
        const uint8_t ci = b[pos++];
        lines.push_back("CI: 0x" + hx(ci));
        if (ci == 0x7A || ci == 0x72) {
            const size_t header = ci == 0x72 ? 12 : 4;
            if (b.size() - pos < header) lines.push_back("Truncated transport header");
            else {
                if (ci == 0x72) { lines.push_back("TPL address: " + hex(b.data() + pos, 8)); pos += 8; }
                lines.push_back("Access: 0x" + hx(b[pos]) + " Status: 0x" + hx(b[pos + 1]));
                lines.push_back("Config LE: " + hex(b.data() + pos + 2, 2));
                const unsigned security = b[pos + 3] & 31;
                lines.push_back("TPL security mode: " + std::to_string(security));
                pos += 4;
                plain = security == 0;
                if (!plain) lines.push_back("Protected payload (no key)");
            }
        } else if (ci == 0x78) plain = true;
        else lines.push_back("Unknown/other CI: raw only");
    }
    // Every byte remains inspectable. Only confirmed plaintext is called plaintext.
    if (plain) {
        records(b, pos, lines);
        lines.push_back("Plain application data (hex):");
        for (size_t p = pos; p < b.size(); p += 12)
            lines.push_back(hex(b.data() + p, std::min(size_t(12), b.size() - p)));
    }
    lines.push_back("Full frame hex; DLL CRC removed");
    lines.push_back("Includes ciphertext if protected");
    for (size_t p = 0; p < b.size(); p += 12) {
        char offset[12];
        snprintf(offset, sizeof(offset), "%03u: ", unsigned(p));
        lines.push_back(std::string(offset) + hex(b.data() + p, std::min(size_t(12), b.size() - p)));
    }
    return lines;
}
} // namespace wmbus
