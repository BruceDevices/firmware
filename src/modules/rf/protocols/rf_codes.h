// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <vector>

struct RfCodes {
    uint32_t frequency = 0;
    uint32_t serial = 0;
    uint64_t key = 0;
    uint16_t cnt = 0;
    uint32_t fix = 0;
    uint32_t hop = 0;
    uint32_t encrypted = 0;
    uint32_t seed = 0; // secure/erreka learning; 0 = derive from serial
    uint8_t btn = 0;
    String mf_name = "Unknown";
    String protocol = "";
    String preset = "";
    String data = "";
    int te = 0;
    std::vector<int> indexed_durations;
    String filepath = "";
    int Bit = 0;
    int BitRAW = 0;
    // Full decoded record for variants wider than the legacy 64-bit key or
    // 16-bit counter. Never reconstruct these files from the legacy fields.
    String decodedFile;
    String decodedText;
    bool encoderAvailable = false;

    bool keeloq_check_decrypt(uint32_t decrypt);
    bool keeloq_check_decrypt_centurion(uint32_t decrypt);

    void keeloq_step(uint16_t step);
};

struct HighLow {
    uint8_t high; // 1
    uint8_t low;  // 31
};
