#pragma once
#include <vector>
#include <cstdint>
#include <cstring>

static inline std::vector<bool> tpms_durations_to_bitmap(
    const std::vector<int>& durations,
    unsigned int te,
    unsigned int te_delta)
{
    std::vector<bool> bits;
    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? (unsigned)raw : (unsigned)(-raw);
        unsigned int count = (dur + te/2) / te;
        if (count == 0) count = 1;
        for (unsigned int i = 0; i < count; i++)
            bits.push_back(level);
    }
    return bits;
}

#define TPMS_SEEK_NOT_FOUND UINT32_MAX

static inline uint32_t tpms_bitmap_seek_bits(
    const std::vector<bool>& bits,
    uint32_t startpos,
    uint32_t maxbits,
    const char *pattern)
{
    uint32_t patlen = (uint32_t)strlen(pattern);
    if (startpos + patlen > maxbits) return TPMS_SEEK_NOT_FOUND;
    if (patlen == 0) return TPMS_SEEK_NOT_FOUND;

    for (uint32_t i = startpos; i + patlen <= maxbits; i++) {
        bool match = true;
        for (uint32_t j = 0; j < patlen; j++) {
            bool expected = (pattern[j] == '1');
            if (bits[i + j] != expected) { match = false; break; }
        }
        if (match) return i;
    }
    return TPMS_SEEK_NOT_FOUND;
}

static inline uint32_t tpms_convert_from_line_code(
    uint8_t *buf, uint32_t buflen,
    const std::vector<bool>& bits,
    uint32_t offset, uint32_t maxbits,
    const char *zero_pattern,
    const char *one_pattern)
{
    uint32_t zlen = (uint32_t)strlen(zero_pattern);
    uint32_t olen = (uint32_t)strlen(one_pattern);
    if (zlen != olen) return 0;
    uint32_t symlen = zlen;
    uint32_t bitpos = 0;
    uint32_t byte_pos = 0;

    while (offset + symlen <= maxbits && byte_pos < buflen) {
        // Check for zero pattern
        bool is_zero = true;
        for (uint32_t j = 0; j < symlen; j++) {
            bool expected = (zero_pattern[j] == '1');
            if (bits[offset + j] != expected) { is_zero = false; break; }
        }
        if (is_zero) {
            if (bitpos == 0) buf[byte_pos] = 0;
            buf[byte_pos] = (uint8_t)((buf[byte_pos] << 1) | 0);
            bitpos++;
            offset += symlen;
            if (bitpos == 8) { bitpos = 0; byte_pos++; }
            continue;
        }

        // Check for one pattern
        bool is_one = true;
        for (uint32_t j = 0; j < symlen; j++) {
            bool expected = (one_pattern[j] == '1');
            if (bits[offset + j] != expected) { is_one = false; break; }
        }
        if (is_one) {
            if (bitpos == 0) buf[byte_pos] = 0;
            buf[byte_pos] = (uint8_t)((buf[byte_pos] << 1) | 1);
            bitpos++;
            offset += symlen;
            if (bitpos == 8) { bitpos = 0; byte_pos++; }
            continue;
        }

        // Neither pattern matched - stop
        break;
    }

    return byte_pos * 8 + bitpos;
}

static inline uint32_t tpms_convert_from_diff_manchester(
    uint8_t *buf, uint32_t buflen,
    const std::vector<bool>& bits,
    uint32_t offset, uint32_t maxbits,
    bool previous)
{
    uint32_t byte_pos = 0;
    int bitpos = 0;

    while (offset + 2 <= maxbits && byte_pos < buflen) {
        // Differential Manchester: bit is encoded by the TRANSITION
        // at the start of the symbol period.
        // Level at offset is the start level.
        // If level toggles at offset+1, bit = 0 (transition at start)
        // If level stays same at offset+1, bit = 1 (no transition at start)
        // Reference: previous == level at start of symbol
        
        bool start_level = bits[offset];
        bool mid_level = bits[offset + 1];
        
        // In differential Manchester, if level changes from start to mid,
        // that's our transition detection
        bool bit_val;
        if (start_level != previous) {
            // Transition at start of symbol = data 0
            bit_val = false;
        } else {
            // No transition at start = data 1
            bit_val = true;
        }
        previous = mid_level;

        if (bitpos == 0) buf[byte_pos] = 0;
        buf[byte_pos] = (uint8_t)((buf[byte_pos] << 1) | (bit_val ? 1 : 0));
        bitpos++;
        offset += 2;

        if (bitpos == 8) { bitpos = 0; byte_pos++; }
    }

    return byte_pos * 8 + (uint32_t)bitpos;
}

static inline bool tpms_bitmap_match_bits(
    const std::vector<bool>& bits,
    uint32_t bitpos, uint32_t maxbits,
    const char *pattern)
{
    uint32_t patlen = (uint32_t)strlen(pattern);
    if (bitpos + patlen > maxbits) return false;
    for (uint32_t j = 0; j < patlen; j++) {
        if (bits[bitpos + j] != (pattern[j] == '1'))
            return false;
    }
    return true;
}
