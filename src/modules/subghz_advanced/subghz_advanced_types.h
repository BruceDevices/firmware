#ifndef __SUBGHZ_ADVANCED_TYPES_H__
#define __SUBGHZ_ADVANCED_TYPES_H__

#include <Arduino.h>

struct SubGhzAdvancedFrame {
    String source = "";
    String filetype = "";
    String protocol_name = "Unknown";
    uint32_t frequency_hz = 0;
    int bit_count = 0;
    String key_hex = "";
    String counter = "";
    String button = "";
    String serial = "";
    String raw_summary = "";
    String notes = "";
    bool valid = false;

    String toJson() const;
};

#endif
