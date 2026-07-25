#ifndef __CAMERA_BRANDS_H__
#define __CAMERA_BRANDS_H__

#include <Arduino.h>

// Fingerprint database for consumer / surveillance Wi-Fi + BLE cameras.
//
// Built for the brands with the largest presence in the Australian and Hong
// Kong markets (Hikvision, Dahua, EZVIZ, Reolink, TP-Link/Tapo, Xiaomi, Arlo,
// Ring, Eufy, Wyze, Imou, Lorex, Swann, Aqara, Axon, Flock ...). OUI prefixes
// come from the IEEE registry; SSID / BLE-name substrings come from each
// vendor's setup soft-AP and advertised device names.
//
// Two match strategies are used because they have very different precision:
//   * OUI match  - only used for vendors that almost exclusively ship cameras /
//                  NVRs (Hikvision, Dahua, EZVIZ, Reolink, Ring, Arlo, Wyze,
//                  Imou, Lorex, Axon, Flock). High confidence.
//   * name match - used for multi-product vendors (TP-Link, Xiaomi, Aqara,
//                  Eufy, Swann, Nest) where the OUI is shared with phones /
//                  routers and would cause false positives. We match the
//                  camera-specific SSID / advertised-name tokens instead.

struct CameraBrand {
    const char *name;
    const char *const *ouis; // lowercase "aa:bb:cc", or nullptr
    uint16_t ouiCount;
    const char *const *patterns; // lowercase SSID/BLE-name substrings, or nullptr
    uint16_t patternCount;
};

// Identify a device as a known camera from its (lowercased) MAC and (lowercased)
// SSID/advertised name. Returns the brand name or nullptr. When matched,
// *methodOut (if non-null) is set to "OUI" or "name".
const char *identifyCamera(const String &macLower, const String &nameLower, const char **methodOut = nullptr);

// Identify an iLnkP2P / CS2 Network P2P camera by its UID prefix (the leading
// letters of a UID like "VSTF-123456-ABCDE"). These P2P cameras are the cheap
// OEM class that OUI/SSID fingerprinting misses: they ship generic Wi-Fi chips
// and self-identify only via the LAN-search protocol (UDP 32108).
//
// `prefixUpper` should be the upper-case letter prefix. Returns a brand/family
// label for a known prefix, or nullptr (caller falls back to a generic label).
const char *identifyP2PPrefix(const String &prefixUpper);

#endif
