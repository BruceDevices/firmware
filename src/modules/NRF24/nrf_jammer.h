/**
 * @file nrf_jammer.h
 * @brief Enhanced 2.4 GHz jammer with 12 modes, data flooding, and per-mode config.
 *
 * Supports two jamming strategies:
 *  - Constant Carrier (CW): Unmodulated RF saturates receiver AGC.
 *    Best for FHSS targets (Bluetooth, Drone, RC) and analog video.
 *  - Data Flooding: Sends garbage packets via writeFast() creating
 *    real packet collisions and CRC corruption. Best for channel-specific
 *    protocols (WiFi, BLE, Zigbee).
 *
 * Hardware: Fully supports PA+LNA modules (E01-ML01SP2, up to +20dBm).
 *
 * Credits: Channel mappings and mode presets adapted from EvilCrow-RF-V2
 *          NrfJammer by Joel Sernamoreno. Adapted for Bruce architecture.
 */
#ifndef __NRF_JAMMER_H
#define __NRF_JAMMER_H

#include "modules/NRF24/nrf_common.h"

// ── Jamming Mode Presets ────────────────────────────────────────
// Cycleable preset modes only. CH Jammer and CH Hopper are
// standalone functions accessible from the NRF Jammer submenu.
enum NrfJamMode : uint8_t {
    NRF_JAM_FULL = 0,      // All channels 0-124
    NRF_JAM_WIFI = 1,      // WiFi ch 1, 6, 11 bandwidth
    NRF_JAM_BLE = 2,       // BLE data channels
    NRF_JAM_BLE_ADV = 3,   // BLE advertising channels (37,38,39)
    NRF_JAM_BLUETOOTH = 4, // Classic Bluetooth FHSS
    NRF_JAM_USB = 5,       // USB wireless dongles
    NRF_JAM_VIDEO = 6,     // Video streaming (FPV, baby monitors)
    NRF_JAM_RC = 7,        // RC controllers
    NRF_JAM_ZIGBEE = 8,    // Zigbee channels 11-26
    NRF_JAM_DRONE = 9,     // Drone FHSS protocols
    NRF_JAM_MODE_COUNT = 10
};

// ── Per-mode configuration ──────────────────────────────────────
struct NrfJamConfig {
    uint8_t paLevel;      // 0-3 (MIN..MAX, PA+LNA: 0dBm→+20dBm)
    uint8_t dataRate;     // 0=1Mbps, 1=2Mbps, 2=250Kbps
    uint16_t dwellTimeMs; // Time on each channel (0=turbo, max 200ms)
    uint8_t useFlooding;  // 0=Constant Carrier, 1=Data Flooding
};

// ── Hopper config ───────────────────────────────────────────────
struct NrfHopperConfig {
    uint8_t startChannel; // 0-124
    uint8_t stopChannel;  // 0-124
    uint8_t stepSize;     // 1-10
};

// ── Mode information (name + description) ───────────────────────
struct NrfJamModeInfo {
    const char *name;
    const char *shortName; // For status display (max 12 chars)
};

// ── Public Functions ────────────────────────────────────────────

/// Main jammer with mode selection menu
void nrf_jammer();

/// Direct entry to single channel jammer
void nrf_channel_jammer();

/// Direct entry to custom channel hopper
void nrf_channel_hopper();

#endif // __NRF_JAMMER_H
