#ifndef __IR_ATTACK_DETECTOR_H
#define __IR_ATTACK_DETECTOR_H

#include <cstdint>

// IR attack detector: watches the IR receiver for abnormal event volume and
// classifies it as TV-B-Gone (bursts of valid, varied protocols on a steady
// ~200ms cadence) or IR Jammer (mostly undecodable noise at high rate).
void ir_attack_detector();

// Headless variant for the `bluefish ir` serial command: no display output, runs for durationMs
// and reports status to serialDevice (works over USB or the BLE serial bridge).
void ir_attack_detector_headless(uint32_t durationMs);

#endif
