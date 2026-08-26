#ifndef __RF_JAMMER_DETECTOR_H
#define __RF_JAMMER_DETECTOR_H

#include <cstdint>

// CC1101 jammer detector: sweeps the configured sub-GHz range and scores
// sustained RSSI energy per frequency to flag and classify FULL POWER vs
// INTERMITTENT jamming (the two fixed-frequency modes RFJammer already runs).
void rf_jammer_detector();

// Headless variant for the `bluefish subghz` serial command: no display output, runs for
// durationMs and reports status to serialDevice (works over USB or the BLE serial bridge).
void rf_jammer_detector_headless(uint32_t durationMs);

#endif
