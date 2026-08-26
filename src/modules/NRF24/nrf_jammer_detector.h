#ifndef __NRF_JAMMER_DETECTOR_H
#define __NRF_JAMMER_DETECTOR_H

#include <cstdint>

// 2.4GHz jammer detector: sweeps the NRF24 RPD across the band and scores
// sustained/broad energy against a weighted heuristic to flag likely jamming.
void jammer_detector();

// Headless variant for the `bluefish nrf24` serial command: no display output, runs for
// durationMs and reports status to serialDevice (works over USB or the BLE serial bridge).
void jammer_detector_headless(uint32_t durationMs);

#endif
