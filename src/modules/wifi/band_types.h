// band_types.h
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

inline void detectSupportedBands() {
    // Reset
    g_supportedBands = SupportedBands();
    
    // Check 2.4GHz (channels 1-14)
    if (esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE) == ESP_OK) {
        g_supportedBands.has2_4GHz = true;
        g_supportedBands.bandList.push_back(BAND_2_4GHZ);
        g_supportedBands.bandCount++;
    }
    
    // Check 5GHz (channels 36-165)
    if (esp_wifi_set_channel(36, WIFI_SECOND_CHAN_NONE) == ESP_OK) {
        g_supportedBands.has5GHz = true;
        g_supportedBands.bandList.push_back(BAND_5GHZ);
        g_supportedBands.bandCount++;
    } else if (esp_wifi_set_channel(149, WIFI_SECOND_CHAN_NONE) == ESP_OK) {
        g_supportedBands.has5GHz = true;
        g_supportedBands.bandList.push_back(BAND_5GHZ);
        g_supportedBands.bandCount++;
    }
    
    // Check 6GHz (channels 1-233)
    if (esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE) == ESP_OK) {
        // Simplified 6GHz detection
        g_supportedBands.has6GHz = true;
        g_supportedBands.bandList.push_back(BAND_6GHZ);
        g_supportedBands.bandCount++;
    }
    
    // Restore to a safe channel
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    Serial.printf("[BAND] Supported bands: 2.4:%d 5:%d 6:%d Count:%d\n",
                  g_supportedBands.has2_4GHz, g_supportedBands.has5GHz,
                  g_supportedBands.has6GHz, g_supportedBands.bandCount);
}

inline bool isBandSupported(int band) {
    switch (band) {
        case BAND_2_4GHZ: return g_supportedBands.has2_4GHz;
        case BAND_5GHZ: return g_supportedBands.has5GHz;
        case BAND_6GHZ: return g_supportedBands.has6GHz;
        default: return false;
    }
}

inline SupportedBands getSupportedBands() {
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

#endif // BAND_TYPES_H
