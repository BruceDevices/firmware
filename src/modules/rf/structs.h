#ifndef RF_STRUCTS_H
#define RF_STRUCTS_H

#include "core/display.h"
#include "protocols/rf_codes.h"
#include <driver/rmt_rx.h>
#include <driver/rmt_tx.h>

struct RawRecording {
    float frequency;
    std::vector<rmt_symbol_word_t *> codes;
    std::vector<uint16_t> codeLengths;
    std::vector<uint16_t> gaps;
};

struct RawRecordingStatus {
    float frequency = 0.f;
    int rssiCount = 0;  // Counter for the number of RSSI readings
    int latestRssi = 0; // Store the latest RSSI value
    bool recordingStarted = false;
    bool recordingFinished = false;
    unsigned long firstSignalTime = 0; // Store the time of the latest signal
    unsigned long lastSignalTime = 0;  // Store the time of the latest signal
    unsigned long lastRssiUpdate = 0;
};

struct FreqFound {
    float freq;
    int rssi;
};


struct Protocol {
    uint16_t pulseLength; // base pulse length in microseconds, e.g. 350
    HighLow syncFactor;
    HighLow zero;
    HighLow one;
    bool invertedSignal;
};

struct KeeloqKey {
    String mf_name{};
    uint64_t key = 0;
    uint32_t type = 0;
};

#endif
