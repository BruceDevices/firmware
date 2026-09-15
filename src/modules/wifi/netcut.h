#ifndef __NETCUT_H__
#define __NETCUT_H__

/**
 * @file netcut.h
 * @brief NetCut ARP Module for Bruce Firmware
 * @description ARP Poisoning / Restore / Troll module integrated into Bruce WiFi menu.
 *              Ported from standalone NetCut ESP32 firmware.
 *
 * Key design decisions:
 *   - Uses LwIP netif->linkoutput() for ARP packet injection (NOT esp_wifi_internal_tx)
 *   - Reuses Bruce's ARPScanner infrastructure for device discovery
 *   - All UI via Bruce's loopOptions() — no custom display drawing
 *   - Troll Mode is BLOCKING — stops when user presses EscPress/Back
 *   - No WebUI — control exclusively via T-Embed screen + encoder
 *   - VIP persistence via LittleFS (Bruce's standard config system)
 *
 * @note Requires active WiFi STA connection before use.
 */

#include <Arduino.h>
#include <IPAddress.h>
#include <WiFi.h>

// LwIP headers for ARP packet construction
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"

// Bruce core headers
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/net_utils.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include <globals.h>

// ============================================
// Configuration Constants
// ============================================
#define NETCUT_MAX_DEVICES 64      // Max devices in scan list
#define NETCUT_ARP_INTERVAL_MS 150 // ARP poison packet repeat interval (ms)
#define NETCUT_POISON_BURST 5      // Packets per poison burst
#define NETCUT_RESTORE_BURST 5     // Packets per restore burst
#define NETCUT_RESTORE_ROUNDS 3    // Restore repetition rounds

#define NETCUT_TROLL_DEFAULT_OFFLINE_MS 10000 // Default troll offline duration
#define NETCUT_TROLL_DEFAULT_ONLINE_MS 5000   // Default troll online duration
#define NETCUT_TROLL_MIN_MS 1000              // Minimum troll interval
#define NETCUT_TROLL_MAX_MS 60000             // Maximum troll interval

#define NETCUT_VIP_FILE "/netcut_vip.json" // LittleFS VIP persistence file

// ============================================
// Data Structures
// ============================================

/**
 * @brief Represents a network device discovered by ARP scan.
 *
 * Adapted from standalone NetCut's NetworkDevice struct.
 * Stores MAC as raw bytes (uint8_t[6]) for direct use in ARP packets,
 * avoiding repeated string-to-bytes conversion in hot path.
 */
struct NetCutDevice {
    IPAddress ip;        // Device IP address
    uint8_t macBytes[6]; // MAC address as raw bytes (for ARP packets)
    String macStr;       // MAC address as string (for display/VIP lookup)

    bool isCut;    // Currently being ARP poisoned
    bool isTroll;  // Currently in troll mode
    bool isVip;    // VIP-protected

    // Troll mode individual timers
    unsigned long lastTrollToggle; // Last time troll state toggled (millis)
    bool isTrollOffline;           // Current troll phase: true=poisoning, false=restoring

    // Restore continuation timer
    unsigned long restoreUntil; // Keep sending restore packets until this time (millis)

    NetCutDevice()
        : isCut(false), isTroll(false), isVip(false), lastTrollToggle(0),
          isTrollOffline(false), restoreUntil(0) {
        memset(macBytes, 0, 6);
    }
};

// ============================================
// Module State (internal, managed by netcut.cpp)
// ============================================
// All state is file-scoped in netcut.cpp to prevent global collision.
// Only function declarations are exposed here.

// ============================================
// Public API — Menu Entry Point
// ============================================

/**
 * @brief Main entry point for NetCut module.
 *        Called from WifiMenu as a menu option.
 *        Requires active WiFi STA connection.
 *
 * Flow: Check WiFi → Scan Devices → Show Device List → Per-device/Global actions
 */
void netcutMenu();

// ============================================
// Public API — Core ARP Operations
// ============================================

/**
 * @brief Scan the local network for devices via ARP requests.
 *        Uses LwIP etharp_request() + etharp_get_entry() — same method as Bruce's ARPScanner.
 *        Results are stored in internal device list.
 * @return Number of devices found.
 */
int netcutScanDevices();

/**
 * @brief Send ARP poison packets to a single device.
 *        Poisons both the target AND the gateway (bidirectional).
 *        Uses netif->linkoutput() via LwIP pbuf — NOT esp_wifi_internal_tx().
 * @param deviceIndex  Index in internal device list
 * @param burstCount   Number of packets to send (default 1 for periodic, 20 for instant)
 */
void netcutPoisonDevice(int deviceIndex, int burstCount = 1);

/**
 * @brief Send ARP restore packets to a single device.
 *        Sends correct gateway MAC to target, and correct target MAC to gateway.
 *        Uses broadcast + unicast ARP reply/request combo for reliability (RFC 826).
 * @param deviceIndex  Index in internal device list
 */
void netcutRestoreDevice(int deviceIndex);

/**
 * @brief Cut all non-VIP devices (bulk ARP poison).
 */
void netcutCutAll();

/**
 * @brief Resume all devices (bulk ARP restore).
 *        Also clears isCut and isTroll flags.
 */
void netcutResumeAll();

// ============================================
// Public API — Troll Mode
// ============================================

/**
 * @brief Start Troll Mode for a single device.
 *        BLOCKING — loops until user presses EscPress.
 *        Alternates between poison (offline) and restore (online) phases
 *        using individual per-device timers.
 * @param deviceIndex  Index in internal device list
 */
void netcutTrollDevice(int deviceIndex);

/**
 * @brief Start Troll Mode for ALL non-VIP devices.
 *        BLOCKING — loops until user presses EscPress.
 */
void netcutTrollAll();

// ============================================
// Public API — VIP Management
// ============================================

/**
 * @brief Toggle VIP status for a device.
 *        VIP devices are immune to Cut/Troll operations.
 *        If device is currently Cut/Trolled, it will be restored immediately.
 * @param deviceIndex  Index in internal device list
 */
void netcutToggleVip(int deviceIndex);

/**
 * @brief Save VIP list to LittleFS.
 *        Persists MAC addresses of all VIP-flagged devices.
 */
void netcutSaveVipList();

/**
 * @brief Load VIP list from LittleFS.
 *        Called during scan to auto-flag previously-saved VIP devices.
 */
void netcutLoadVipList();

// ============================================
// Public API — Troll Timing Configuration
// ============================================

/**
 * @brief Interactive menu to adjust troll offline/online durations.
 *        Uses Bruce's loopOptions() for value input.
 */
void netcutTrollTimingMenu();

#endif // __NETCUT_H__
