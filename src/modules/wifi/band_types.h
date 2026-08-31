#ifndef BAND_TYPES_H
#define BAND_TYPES_H

#include <vector>
#include <Arduino.h>
#include <esp_wifi.h>

// ============================================================
// Band Types - Shared between Karma and Deauther
// ============================================================

enum BandType {
    BAND_2_4GHZ = 0,
    BAND_5GHZ = 1,
    BAND_6GHZ = 2
};

struct SupportedBands {
    bool has2_4GHz = false;
    bool has5GHz = false;
    bool has6GHz = false;
    int bandCount = 0;
    std::vector<int> bandList;
};

// ============================================================
// Band Detection Functions - Inline to avoid multiple definitions
// ============================================================

static SupportedBands g_supportedBands;
static bool g_bandsDetected = false;

// Helper: Test if a channel can be set
static inline bool testChannel(uint8_t channel) {
    return esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) == ESP_OK;
}

// Helper: Test multiple channels for a band, return first success
static inline bool testBandChannels(const int* channels, int count) {
    for (int i = 0; i < count; i++) {
        if (testChannel(channels[i])) {
            return true;
        }
    }
    return false;
}

inline void detectSupportedBands() {
    // Skip if already detected
    if (g_bandsDetected) return;
    
    // Reset
    g_supportedBands = SupportedBands();
    
    // ──────────────────────────────────────────
    // 2.4GHz Detection (channels 1-14)
    // ──────────────────────────────────────────
    // Try common 2.4GHz channels
    int ch24[] = {1, 6, 11, 3, 8, 13};
    if (testBandChannels(ch24, sizeof(ch24) / sizeof(ch24[0]))) {
        g_supportedBands.has2_4GHz = true;
        g_supportedBands.bandList.push_back(BAND_2_4GHZ);
        g_supportedBands.bandCount++;
    } else {
        // Ultimate fallback - channel 1 should always work
        if (testChannel(1)) {
            g_supportedBands.has2_4GHz = true;
            g_supportedBands.bandList.push_back(BAND_2_4GHZ);
            g_supportedBands.bandCount++;
        }
    }
    
    // ──────────────────────────────────────────
    // 5GHz Detection (channels 36-165)
    // ──────────────────────────────────────────
    // Test across UNII-1 and UNII-3 bands
    int ch5[] = {36, 40, 44, 48, 149, 153, 157, 161, 165};
    if (testBandChannels(ch5, sizeof(ch5) / sizeof(ch5[0]))) {
        g_supportedBands.has5GHz = true;
        g_supportedBands.bandList.push_back(BAND_5GHZ);
        g_supportedBands.bandCount++;
    }
    
    // ──────────────────────────────────────────
    // 6GHz Detection (channels 1-233, but test >14)
    // ──────────────────────────────────────────
    // Use channels > 14 to avoid 2.4GHz overlap
    // 6GHz channels are 1-233, but we skip 1-14 to avoid false positives
    int ch6[] = {17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233};
    for (int ch : ch6) {
        // Skip channels <= 14 (2.4GHz range)
        if (ch <= 14) continue;
        if (testChannel(ch)) {
            g_supportedBands.has6GHz = true;
            g_supportedBands.bandList.push_back(BAND_6GHZ);
            g_supportedBands.bandCount++;
            break;
        }
    }
    
    // ──────────────────────────────────────────
    // Restore to safe channel
    // ──────────────────────────────────────────
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    // ──────────────────────────────────────────
    // Final safety net
    // ──────────────────────────────────────────
    if (g_supportedBands.bandCount == 0) {
        // If absolutely nothing works, assume 2.4GHz
        Serial.println("[BAND] WARNING: No bands detected! Defaulting to 2.4GHz.");
        g_supportedBands.has2_4GHz = true;
        g_supportedBands.bandList.push_back(BAND_2_4GHZ);
        g_supportedBands.bandCount = 1;
    }
    
    g_bandsDetected = true;
    
    // ──────────────────────────────────────────
    // Log results
    // ──────────────────────────────────────────
    Serial.printf("[BAND] Supported: 2.4:%d 5:%d 6:%d (Count:%d)\n",
                  g_supportedBands.has2_4GHz,
                  g_supportedBands.has5GHz,
                  g_supportedBands.has6GHz,
                  g_supportedBands.bandCount);
    
    if (g_supportedBands.bandCount > 0) {
        String bandStr = "";
        for (int band : g_supportedBands.bandList) {
            if (!bandStr.isEmpty()) bandStr += ", ";
            switch (band) {
                case BAND_2_4GHZ: bandStr += "2.4GHz"; break;
                case BAND_5GHZ: bandStr += "5GHz"; break;
                case BAND_6GHZ: bandStr += "6GHz"; break;
                default: bandStr += "Unknown"; break;
            }
        }
        Serial.printf("[BAND] Detected bands: %s\n", bandStr.c_str());
    }
}

inline bool isBandSupported(int band) {
    // Ensure detection has run
    if (!g_bandsDetected) detectSupportedBands();
    
    switch (band) {
        case BAND_2_4GHZ: return g_supportedBands.has2_4GHz;
        case BAND_5GHZ: return g_supportedBands.has5GHz;
        case BAND_6GHZ: return g_supportedBands.has6GHz;
        default: return false;
    }
}

inline SupportedBands getSupportedBands() {
    // Ensure detection has run
    if (!g_bandsDetected) detectSupportedBands();
    return g_supportedBands;
}

inline String getBandName(int band) {
    switch (band) {
        case BAND_2_4GHZ: return "2.4GHz";
        case BAND_5GHZ: return "5GHz";
        case BAND_6GHZ: return "6GHz";
        default: return "Unknown";
    }
}

#endif
