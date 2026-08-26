#ifndef __BLE_SPAM_DETECTOR_H
#define __BLE_SPAM_DETECTOR_H

#include <cstdint>

// BLE Spam detector: watches nearby advertising for the vendor payload
// families BLE Spam abuses (Apple Continuity, Samsung, Microsoft Swift Pair,
// Google Fast Pair) and scores abnormal packet rate + MAC-address churn per
// vendor to flag which one is being spammed.
void ble_spam_detector();

// Headless variant for the `bluefish blespam` serial command: no display output, runs for
// durationMs and reports status to serialDevice (works over USB or the BLE serial bridge).
void ble_spam_detector_headless(uint32_t durationMs);

#endif
