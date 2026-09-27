#include "ble_tracker_detector.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "modules/ble/ble_common.h"
#if !defined(LITE_VERSION)
#include "modules/ble/BLE_Suite.h" // BLEStateManager
#endif
#include <algorithm>
#ifndef NIMBLE_SCAN_DEFAULT_MAX_RESULTS
#define NIMBLE_SCAN_DEFAULT_MAX_RESULTS 0xFF
#endif

// --- Bluetooth SIG company identifiers -------------------------------------
#define APPLE_MFG_ID 0x004C
#define TILE_MFG_ID 0x067C // Tile, Inc.  (0x0181 = Gecko Health Innovations)
#define SAMSUNG_MFG_ID 0x0075
#define META_MFG_ID 0x01AB      // Meta Platforms, Inc.
#define META_TECH_MFG_ID 0x058E // Meta Platforms Technologies, LLC (ex-Oculus)
#define LUXOTTICA_MFG_ID 0x0D53 // Luxottica Group S.p.A (Ray-Ban, Oakley)
#define SNAP_MFG_ID 0x03C2      // Snapchat Inc
#define VUZIX_MFG_ID 0x060C
#define EVEN_MFG_ID 0x10F9 // Even Realities Ltd.
#define SONY_MFG_ID 0x012D
#define EPSON_MFG_ID 0x0040
// NOTE: 0x00A3 is "Meta Watch Ltd." -- an unrelated legacy smartwatch company.
// Do not treat it as Meta/Facebook.

// --- Apple offline finding (Find My) ---------------------------------------
#define APPLE_TYPE_FINDMY 0x12
#define APPLE_TYPE_PAIRING 0x07 // proximity pairing (AirPods, unregistered tag)
#define FINDMY_LEN_SEPARATED 0x19
#define FINDMY_LEN_NEARBY 0x02
#define FINDMY_STATUS_MAINTAINED_MASK 0x04
#define FINDMY_STATUS_BATTERY_MASK 0xC0
#define FINDMY_STATUS_BATTERY_SHIFT 6

// --- 16-bit service UUIDs ---------------------------------------------------
#define UUID_TILE 0xFEED
#define UUID_SMARTTAG_REG 0xFD5A   // Samsung SmartTag, registered / offline finding
#define UUID_SMARTTAG_UNREG 0xFD59 // Samsung SmartTag, unregistered
#define UUID_META_FD5F 0xFD5F      // Ray-Ban Meta advertisements
#define UUID_META_FEB7 0xFEB7
#define UUID_META_FEB8 0xFEB8

// --- Tunables ---------------------------------------------------------------
#define RSSI_NONE -127
#define RSSI_STRONG -50 // above this: NEAR
#define RSSI_MEDIUM -70 // above this: MEDIUM
#define RSSI_WEAK -85   // below this: ignored entirely, too far to matter

#define SCAN_CYCLE_MS 8000                 // how long to scan before updating the display
#define ROTATION_MS (15UL * 60UL * 1000UL) // Apple key / BLE RPA rotation period

#define DWELL_WATCH_MS (5UL * 60UL * 1000UL)
#define DWELL_ALERT_MS ROTATION_MS
#define DWELL_STRONG_MS (2UL * ROTATION_MS)

#define MISSED_CYCLES_TO_END_RUN 2 // one empty cycle is jitter, not departure

// --- UI sizing --------------------------------------------------------------
// setTextSize() takes a uint8_t.
// Bruce's FP/FM/FG are 1/2/3 (i.e. small, medium, large font size)
#define TITLE_TEXT_SIZE FP
#define ROW_TEXT_SIZE FP
#define DETAIL_TEXT_SIZE FP

// drawStatusBar() owns y=0..25: clock at (12,12), SD/GPS/BLE icons centred at
// y=7, divider line at y=25. Anything drawn above UI_TOP lands on top of them.
#define UI_TOP 28
#define UI_LEFT 8
#define UI_CLEAR_W (tftWidth - 2 * UI_LEFT)

static CategoryPresence g_presence[TRACKER_TYPE_COUNT];
static uint16_t g_totalCycles = 0;

// Categories currently in range, in display order. Only changes when a scan
// cycle closes, so it is built once per cycle rather than per repaint -- the
// scroll handler needs the count without rebuilding the list.
static std::vector<TrackerType> g_monitorRows;
static int g_monitorScroll = 0;

// ---------------------------------------------------------------------------
// Advertisement parsing helpers
// ---------------------------------------------------------------------------

static bool hasServiceUUID16(const NimBLEAdvertisedDevice *d, uint16_t uuid16) {
    NimBLEUUID want(uuid16);

    for (uint8_t i = 0; i < d->getServiceUUIDCount(); i++) {
        if (d->getServiceUUID(i) == want) return true;
    }
    for (uint8_t i = 0; i < d->getServiceDataCount(); i++) {
        if (d->getServiceDataUUID(i) == want) return true;
    }
    return false;
}

static bool getMfgData(const NimBLEAdvertisedDevice *d, uint16_t &id, std::string &data) {
    if (d->getManufacturerDataCount() == 0) return false;

    data = d->getManufacturerData(0);
    if (data.size() < 2) return false;

    id = ((uint16_t)(uint8_t)data[1] << 8) | (uint8_t)data[0];
    return true;
}

static bool nameContains(const String &lowerName, const char *needle) {
    if (!needle) return true;
    if (lowerName.isEmpty()) return false;
    return lowerName.indexOf(needle) != -1;
}

static FindMyBattery decodeBattery(uint8_t status) {
    if ((status & FINDMY_STATUS_MAINTAINED_MASK) == 0) return FINDMY_BATT_UNKNOWN;

    switch ((status & FINDMY_STATUS_BATTERY_MASK) >> FINDMY_STATUS_BATTERY_SHIFT) {
        case 0: return FINDMY_BATT_FULL;
        case 1: return FINDMY_BATT_MEDIUM;
        case 2: return FINDMY_BATT_LOW;
        default: return FINDMY_BATT_CRITICAL;
    }
}

// ---------------------------------------------------------------------------
// Tracker matching
// ---------------------------------------------------------------------------

static bool matchTracker(
    const NimBLEAdvertisedDevice *device, const String &lowerName, TrackerType &type, uint8_t &confidence,
    const char *&label, uint8_t &rawStatus
) {
    // 1. Service UUIDs -- the most specific signal available.
    if (hasServiceUUID16(device, UUID_TILE)) {
        type = TRACKER_TILE;
        confidence = 95;
        label = "Tile tracker (Life360)";
        return true;
    }
    if (hasServiceUUID16(device, UUID_SMARTTAG_REG) || hasServiceUUID16(device, UUID_SMARTTAG_UNREG)) {
        type = TRACKER_SMARTTAG;
        confidence = 95;
        label = "Samsung SmartTag";
        return true;
    }

    // 2. Manufacturer data.
    uint16_t mfgId = 0;
    std::string mfg;
    if (getMfgData(device, mfgId, mfg)) {
        if (mfgId == APPLE_MFG_ID && mfg.size() >= 5) {
            uint8_t appleType = (uint8_t)mfg[2];
            uint8_t appleLen = (uint8_t)mfg[3];
            uint8_t status = (uint8_t)mfg[4];

            if (appleType == APPLE_TYPE_FINDMY) {
                bool maintained = (status & FINDMY_STATUS_MAINTAINED_MASK) != 0;
                rawStatus = status;

                if (appleLen == FINDMY_LEN_SEPARATED) {
                    // A frame claiming 25 bytes must actually carry them.
                    if (mfg.size() < 4 + FINDMY_LEN_SEPARATED) return false;

                    if (!maintained) {
                        // The one that matters: publishing its rotating key AND
                        // no owner contact this rotation period.
                        type = TRACKER_AIRTAG;
                        confidence = 95;
                        label = "AirTag (not paired with owner)";
                    } else {
                        // Separated from, but the owner was in contact within
                        // the last 15 minutes. Almost certainly someone's own
                        // tag in their own pocket.
                        type = TRACKER_FIND_MY_WITH_OWNER;
                        confidence = 80;
                        label = "AirTag (paired with owner)";
                    }
                    return true;
                }

                if (appleLen == FINDMY_LEN_NEARBY) {
                    // Short form: owner's device is present.
                    type = TRACKER_FIND_MY_WITH_OWNER;
                    confidence = 75;
                    label = "iPhone";
                    return true;
                }

                type = TRACKER_OTHER_FIND_MY;
                confidence = 60;
                label = "Unknown Apple device";
                return true;
            }

            if (appleType == APPLE_TYPE_PAIRING) {
                if (nameContains(lowerName, "airpod")) {
                    type = TRACKER_AIRPODS;
                    confidence = 90;
                    label = "AirPods";
                } else {
                    type = TRACKER_OTHER_FIND_MY;
                    confidence = 60;
                    label = "Apple accessory";
                }
                return true;
            }
            // Any other Apple type is Continuity chatter from a
            // phone, watch or Mac. Not a tracker.
        }

        if (mfgId == TILE_MFG_ID) {
            type = TRACKER_TILE;
            confidence = 90;
            label = "Tile tracker (Life360)";
            return true;
        }

        // Deliberately NOT matching SAMSUNG_MFG_ID on its own: every Samsung
        // phone, TV and earbud in range broadcasts it. SmartTags are caught by
        // their service UUID above.
        (void)SAMSUNG_MFG_ID;
    }

    // 3. Name fallback for devices in pairing / unregistered mode.
    if (nameContains(lowerName, "AirPod")) {
        type = TRACKER_AIRPODS;
        confidence = 80;
        label = "AirPods";
        return true;
    }
    if (nameContains(lowerName, "Tile")) {
        type = TRACKER_TILE;
        confidence = 75;
        label = "Tile tracker";
        return true;
    }
    if (nameContains(lowerName, "SmartTag")) {
        type = TRACKER_SMARTTAG;
        confidence = 80;
        label = "Samsung SmartTag";
        return true;
    }
    if (nameContains(lowerName, "Chipolo")) {
        type = TRACKER_OTHER_FIND_MY;
        confidence = 80;
        label = "Chipolo tracker";
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Camera / smart-glasses matching
// ---------------------------------------------------------------------------

struct GlassesRule {
    int32_t companyId;
    uint16_t serviceUuid16;
    const char *namePattern;
    TrackerType type;
    uint8_t confidence;
    const char *label;
};

static const GlassesRule GLASSES_RULES[] = {
    // --- Meta: company ID together with a Meta service UUID ---------------
    {META_MFG_ID,      UUID_META_FD5F, nullptr,         GLASSES_META,           95, "Meta glasses"           },
    {META_TECH_MFG_ID, UUID_META_FD5F, nullptr,         GLASSES_META,           95, "Meta glasses"           },
    {-1,               UUID_META_FD5F, nullptr,         GLASSES_META,           90, "Meta device (FD5F)"     },
    {-1,               UUID_META_FEB7, nullptr,         GLASSES_META,           82, "Meta device (FEB7)"     },
    {-1,               UUID_META_FEB8, nullptr,         GLASSES_META,           82, "Meta device (FEB8)"     },

    // --- Meta company ID alone --------------------------------
    {META_MFG_ID,      0,              nullptr,         GLASSES_META,           80, "Meta device"            },
    {META_TECH_MFG_ID, 0,              nullptr,         GLASSES_META,           80, "Meta device"            },
    {LUXOTTICA_MFG_ID, 0,              nullptr,         GLASSES_META,           88, "Luxottica smart glasses"},
    {LUXOTTICA_MFG_ID, 0,              "ray",           GLASSES_META,           92, "Ray-Ban Meta"           },
    {LUXOTTICA_MFG_ID, 0,              "oakley",        GLASSES_META,           92, "Oakley Meta"            },

    // --- other vendors: company ID + product name -------------------------
    {SNAP_MFG_ID,      0,              nullptr,         GLASSES_SNAP,           88, "Snap glasses"           },
    {SNAP_MFG_ID,      0,              "spectacles",    GLASSES_SNAP,           92, "Snap glasses"           },
    {VUZIX_MFG_ID,     0,              nullptr,         GLASSES_OTHER,          85, "Vuzix glasses"          },
    {EVEN_MFG_ID,      0,              nullptr,         GLASSES_OTHER,          85, "Even Realities glasses" },
    {SONY_MFG_ID,      0,              "smarteyeglass", GLASSES_OTHER,          90, "Sony SmartEyeglass"     },
    {EPSON_MFG_ID,     0,              "moverio",       GLASSES_OTHER,          90, "Epson Moverio"          },

    // --- advertised-name matches ------------------------------------------
    {-1,               0,              "ray-ban",       GLASSES_META,           85, "Ray-Ban Meta"           },
    {-1,               0,              "rayban",        GLASSES_META,           85, "Ray-Ban Meta"           },
    {-1,               0,              "ray ban",       GLASSES_META,           85, "Ray-Ban Meta"           },
    {-1,               0,              "meta_rb",       GLASSES_META,           88, "Meta Ray-Ban"           },
    {-1,               0,              "oakley meta",   GLASSES_META,           88, "Oakley Meta"            },
    {-1,               0,              "meta glasses",  GLASSES_META,           85, "Meta glasses"           },
    {-1,               0,              "spectacles",    GLASSES_SNAP,           85, "Snap Spectacles"        },
    {-1,               0,              "vuzix",         GLASSES_OTHER,          85, "Vuzix glasses"          },
    {-1,               0,              "rayneo",        GLASSES_OTHER,          85, "TCL RayNeo"             },
    {-1,               0,              "xreal",         GLASSES_OTHER,          80, "XREAL glasses"          },
    {-1,               0,              "nreal",         GLASSES_OTHER,          80, "Nreal glasses"          },
    {-1,               0,              "rokid",         GLASSES_OTHER,          80, "Rokid glasses"          },
    {-1,               0,              "inmo",          GLASSES_OTHER,          78, "INMO glasses"           },
    {-1,               0,              "solos",         GLASSES_OTHER,          78, "Solos AirGo"            },
    {-1,               0,              "even g",        GLASSES_OTHER,          78, "Even Realities"         },
    {-1,               0,              "echo frames",   GLASSES_OTHER,          80, "Amazon Echo Frames"     },
    {-1,               0,              "focals",        GLASSES_OTHER,          78, "North Focals"           },
    {-1,               0,              "moverio",       GLASSES_OTHER,          82, "Epson Moverio"          },
    {-1,               0,              "google glass",  GLASSES_OTHER,          85, "Google Glass"           },
    {-1,               0,              "heycyan",       GLASSES_OTHER,          85, "HeyCyan glasses"        },
    {-1,               0,              "myvu",          GLASSES_OTHER,          80, "Meizu MYVU"             },
    {-1,               0,              "halliday",      GLASSES_OTHER,          78, "Halliday glasses"       },
    // --- might work ------------------------------------------
    {-1,               0,              "anko",          GLASSES_CAMERA_GENERIC, 65, "Anko/Kmart glasses?"    },
    {-1,               0,              "kmart",         GLASSES_CAMERA_GENERIC, 65, "Kmart glasses?"         },
    {-1,               0,              "ai glasses",    GLASSES_CAMERA_GENERIC, 70, "Generic AI glasses"     },
    {-1,               0,              "smart glasses", GLASSES_CAMERA_GENERIC, 70, "Generic smart glasses"  },
    {-1,               0,              "glasses",       GLASSES_CAMERA_GENERIC, 60, "Name contains 'glasses'"},
    {-1,               0,              "camera",        GLASSES_CAMERA_GENERIC, 55, "Name contains 'camera'" },
};

static bool matchGlasses(
    const NimBLEAdvertisedDevice *device, const String &lowerName, TrackerType &type, uint8_t &confidence,
    const char *&label
) {
    uint16_t mfgId = 0;
    std::string mfg;
    bool haveMfg = getMfgData(device, mfgId, mfg);

    if (haveMfg && (mfgId == META_TECH_MFG_ID || mfgId == META_MFG_ID)) {
        if (mfg.find("META_RB_GLASS") != std::string::npos) {
            type = GLASSES_META;
            confidence = 99;
            label = "Meta Ray-Ban (confirmed)";
            return true;
        }
    }

    const GlassesRule *best = nullptr;
    for (const GlassesRule &r : GLASSES_RULES) {
        if (r.companyId >= 0 && (!haveMfg || mfgId != (uint16_t)r.companyId)) continue;
        if (r.serviceUuid16 != 0 && !hasServiceUUID16(device, r.serviceUuid16)) continue;
        if (r.namePattern != nullptr && !nameContains(lowerName, r.namePattern)) continue;

        if (best == nullptr || r.confidence > best->confidence) best = &r;
    }

    if (best == nullptr) return false;

    type = best->type;
    confidence = best->confidence;
    label = best->label;
    return true;
}

// ---------------------------------------------------------------------------

static bool classifyAdvert(
    const NimBLEAdvertisedDevice *device, TrackerType &type, uint8_t &confidence, const char *&label,
    uint8_t &rawStatus
) {
    type = TRACKER_UNKNOWN;
    confidence = 0;
    label = "Unknown";
    rawStatus = 0;
    if (!device) return false;

    String lowerName = device->getName().c_str();
    lowerName.toLowerCase();

    if (matchGlasses(device, lowerName, type, confidence, label)) return true;
    if (matchTracker(device, lowerName, type, confidence, label, rawStatus)) return true;

    return false;
}

// ---------------------------------------------------------------------------
// Presence bookkeeping
// ---------------------------------------------------------------------------

static OwnerProximity proximityFromRSSI(int rssi) {
    if (rssi <= RSSI_NONE) return PROXIMITY_UNKNOWN;
    if (rssi > RSSI_STRONG) return PROXIMITY_NEAR;
    if (rssi > RSSI_MEDIUM) return PROXIMITY_MEDIUM;
    return PROXIMITY_FAR;
}

static uint32_t dwellMs(const CategoryPresence &c) {
    if (c.consecutive == 0) return 0;
    return c.lastSeenMs - c.runStartMs; // unsigned: survives millis() wrap
}

static DwellLevel dwellLevel(const CategoryPresence &c) {
    if (c.consecutive == 0) return DWELL_NONE;

    uint32_t d = dwellMs(c);
    if (d >= DWELL_STRONG_MS && c.cyclesPresent * 2 >= g_totalCycles) return DWELL_STRONG;
    if (d >= DWELL_ALERT_MS) return DWELL_ALERT;
    if (d >= DWELL_WATCH_MS) return DWELL_WATCH;
    return DWELL_PRESENT;
}

// How many distinct addresses ONE device would be expected to have shown over
// this dwell. Observed far above expected means a crowd, not a follower.
static uint16_t expectedAddrs(const CategoryPresence &c) { return 1 + (uint16_t)(dwellMs(c) / ROTATION_MS); }

static void resetPresence() {
    for (uint8_t i = 0; i < TRACKER_TYPE_COUNT; i++) {
        CategoryPresence &c = g_presence[i];
        c.everSeen = false;
        c.runStartMs = 0;
        c.lastSeenMs = 0;
        c.maxDwellMs = 0;
        c.missedCycles = 0;
        c.cyclesPresent = 0;
        c.consecutive = 0;
        c.maxConsecutive = 0;
        c.bestRSSI = RSSI_NONE;
        c.lastRSSI = RSSI_NONE;
        c.proximity = PROXIMITY_UNKNOWN;
        c.confidence = 0;
        c.label = "";
        c.name = "";
        c.battery = FINDMY_BATT_UNKNOWN;
        c.rawStatus = 0;
        c.addrCount = 0;
        c.addrOverflow = false;
        c.cycleHit = false;
        c.cycleBestRSSI = RSSI_NONE;
    }
    g_totalCycles = 0;
    g_monitorRows.clear();
    g_monitorScroll = 0;
}

static void noteAddress(CategoryPresence &c, const String &address) {
    uint32_t hash = 2166136261UL; // FNV-1a
    for (size_t i = 0; i < address.length(); i++) {
        hash ^= (uint8_t)address[i];
        hash *= 16777619UL;
    }

    for (uint8_t i = 0; i < c.addrCount; i++) {
        if (c.addrHashes[i] == hash) return;
    }
    if (c.addrCount >= MAX_ADDRS_PER_CATEGORY) {
        c.addrOverflow = true;
        return;
    }
    c.addrHashes[c.addrCount++] = hash;
}

static void onClassifiedAdvert(
    TrackerType type, uint8_t confidence, const char *label, uint8_t rawStatus, int rssi,
    const String &address, const String &name
) {
    if (type == TRACKER_UNKNOWN || type >= TRACKER_TYPE_COUNT) return;

    // Too far away to be meaningful -- do not let it accumulate dwell.
    if (rssi < RSSI_WEAK) return;

    CategoryPresence &c = g_presence[type];

    c.cycleHit = true;
    if (rssi > c.cycleBestRSSI) c.cycleBestRSSI = rssi;

    if (confidence > c.confidence) {
        c.confidence = confidence;
        c.label = label;
    }
    if (c.name.isEmpty() && !name.isEmpty()) c.name = name;
    if (rawStatus != 0 || c.rawStatus == 0) {
        c.rawStatus = rawStatus;
        c.battery = decodeBattery(rawStatus);
    }

    noteAddress(c, address);
}

static void closePresenceCycle() {
    uint32_t now = millis();
    g_totalCycles++;

    for (uint8_t i = 0; i < TRACKER_TYPE_COUNT; i++) {
        CategoryPresence &c = g_presence[i];

        if (c.cycleHit) {
            c.missedCycles = 0;
            if (c.consecutive == 0) c.runStartMs = now; // new run begins
            c.consecutive++;
            if (c.consecutive > c.maxConsecutive) c.maxConsecutive = c.consecutive;

            c.cyclesPresent++;
            c.everSeen = true;
            c.lastSeenMs = now;
            c.lastRSSI = c.cycleBestRSSI;
            c.proximity = proximityFromRSSI(c.cycleBestRSSI);
            if (c.cycleBestRSSI > c.bestRSSI) c.bestRSSI = c.cycleBestRSSI;

            uint32_t d = dwellMs(c);
            if (d > c.maxDwellMs) c.maxDwellMs = d;
        } else if (c.consecutive > 0) {
            // A single empty cycle is advert timing jitter, not a departure.
            c.missedCycles++;
            if (c.missedCycles >= MISSED_CYCLES_TO_END_RUN) {
                c.consecutive = 0;
                c.missedCycles = 0;
                c.proximity = PROXIMITY_UNKNOWN;
                c.lastRSSI = RSSI_NONE;
            }
        }

        c.cycleHit = false;
        c.cycleBestRSSI = RSSI_NONE;
    }
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------

String getTrackerTypeString(TrackerType type) {
    switch (type) {
        case TRACKER_AIRTAG: return "AirTag";
        case TRACKER_FIND_MY_WITH_OWNER: return "AirTag & Owner";
        case TRACKER_OTHER_FIND_MY: return "Apple Device";
        case TRACKER_TILE: return "Tile";
        case TRACKER_SMARTTAG: return "SmartTag";
        case TRACKER_AIRPODS: return "AirPods";
        case GLASSES_META: return "META GLASSES";
        case GLASSES_SNAP: return "SPECTACLES";
        case GLASSES_OTHER: return "GLASSES";
        case GLASSES_CAMERA_GENERIC: return "CAMERA?";
        default: return "Unknown";
    }
}

static String getTrackerShortString(TrackerType type) {
    switch (type) {
        case GLASSES_META: return "META GLASSES";
        case GLASSES_SNAP: return "SNAP GLASSES";
        case GLASSES_OTHER: return "CAMERA GLASSES";
        case GLASSES_CAMERA_GENERIC: return "CAMERA GLASSES?";
        case TRACKER_AIRTAG: return "AIRTAG, NO OWNER";
        case TRACKER_FIND_MY_WITH_OWNER: return "APPLE + OWNER";
        case TRACKER_OTHER_FIND_MY: return "OTHER FIND MY";
        case TRACKER_TILE: return "TRACKER TILE";
        case TRACKER_SMARTTAG: return "SMART TAG";
        case TRACKER_AIRPODS: return "AIRPODS";
        default: return "?";
    }
}

String getProximityString(OwnerProximity prox) {
    switch (prox) {
        case PROXIMITY_NEAR: return "very close";
        case PROXIMITY_MEDIUM: return "nearby";
        case PROXIMITY_FAR: return "far away";
        default: return "--";
    }
}

String getDwellLevelString(DwellLevel level) {
    switch (level) {
        case DWELL_STRONG: return "FOLLOWING";
        case DWELL_ALERT: return "ALERT";
        case DWELL_WATCH: return "WATCH";
        case DWELL_PRESENT: return "& still nearby";
        default: return "-";
    }
}

String findMyBatteryString(FindMyBattery battery) {
    switch (battery) {
        case FINDMY_BATT_FULL: return "Full";
        case FINDMY_BATT_MEDIUM: return "Medium";
        case FINDMY_BATT_LOW: return "Low";
        case FINDMY_BATT_CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

static String formatDuration(uint32_t ms) {
    uint32_t totalSec = ms / 1000;
    char buf[16];
    if (totalSec < 60) {
        snprintf(buf, sizeof(buf), "%lus", (unsigned long)totalSec);
    } else {
        snprintf(
            buf, sizeof(buf), "%lum%02lus", (unsigned long)(totalSec / 60), (unsigned long)(totalSec % 60)
        );
    }
    return String(buf);
}

static String formatDurationShort(uint32_t ms) {
    uint32_t totalSec = ms / 1000;
    char buf[12];
    if (totalSec < 60) {
        snprintf(buf, sizeof(buf), "%lus", (unsigned long)totalSec);
    } else if (totalSec < 3600) {
        snprintf(buf, sizeof(buf), "%lum", (unsigned long)(totalSec / 60));
    } else {
        snprintf(
            buf,
            sizeof(buf),
            "%luh%02lum",
            (unsigned long)(totalSec / 3600),
            (unsigned long)((totalSec % 3600) / 60)
        );
    }
    return String(buf);
}

// ---------------------------------------------------------------------------
// Scan plumbing
// ---------------------------------------------------------------------------

class TrackerDeviceCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
        TrackerType type;
        uint8_t confidence;
        const char *label;
        uint8_t rawStatus;
        if (!classifyAdvert(advertisedDevice, type, confidence, label, rawStatus)) return;

        onClassifiedAdvert(
            type,
            confidence,
            label,
            rawStatus,
            advertisedDevice->getRSSI(),
            String(advertisedDevice->getAddress().toString().c_str()),
            String(advertisedDevice->getName().c_str())
        );
    }
};

static TrackerDeviceCallbacks trackerCallbacks;

namespace {
struct ScanSettingsGuard {
    BLEScan *scan;
    explicit ScanSettingsGuard(BLEScan *s) : scan(s) {}
    ScanSettingsGuard(const ScanSettingsGuard &) = delete;
    ScanSettingsGuard &operator=(const ScanSettingsGuard &) = delete;
    ~ScanSettingsGuard() {
        if (scan == nullptr) return;
        scan->setScanCallbacks(nullptr, false);
        scan->setMaxResults(NIMBLE_SCAN_DEFAULT_MAX_RESULTS);
    }
};
} // namespace

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------

static uint16_t levelColor(DwellLevel level) {
    switch (level) {
        case DWELL_STRONG: return TFT_RED;
        case DWELL_ALERT: return TFT_ORANGE;
        case DWELL_WATCH: return TFT_YELLOW;
        case DWELL_PRESENT: return TFT_GREEN;
        default: return TFT_DARKGREY;
    }
}

// Anything with a camera on it is red the moment it is seen, regardless of how
// long it has been around -- presence is the whole point for glasses.
static uint16_t rowColor(TrackerType type, DwellLevel level) {
    if (isGlassesType(type)) return TFT_RED;
    return levelColor(level);
}

static void drawTrackerDetail(TrackerType type) {
    const CategoryPresence &c = g_presence[type];

    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();

    tft.setTextSize(TITLE_TEXT_SIZE);
    tft.setTextColor(isGlassesType(type) ? TFT_RED : bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString(
        isGlassesType(type) ? "Camera device" : "Tracker info", tftWidth / 2, UI_TOP + 2, SMOOTH_FONT
    );

    tft.setTextSize(DETAIL_TEXT_SIZE);
    const int step = (DETAIL_TEXT_SIZE == FP ? 12 : 16);
    int y = UI_TOP + (TITLE_TEXT_SIZE == FP ? 14 : 22);
    const int bottom = tftHeight - 14;

    tft.setTextColor(isGlassesType(type) ? TFT_RED : bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString(" " + String(c.label), 8, y);
    y += step;

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    if (y + step <= bottom) {
        tft.drawString(" It was generally " + getProximityString(c.proximity), 8, y);
        y += step;
    }
    if (y + step <= bottom) {
        tft.drawString(" Most recent signal strength: " + String(c.lastRSSI), 8, y);
        y += step;
    }
    if (y + step <= bottom) {
        tft.drawString(" Max signal strength heard: " + String(c.bestRSSI), 8, y);
        y += step;
    }
    if (y + step <= bottom) {
        tft.drawString(
            " Total addresses: " + String(c.addrCount) + (c.addrOverflow ? "+" : "") +
                ". Expected: " + String(expectedAddrs(c)),
            8,
            y
        );
        y += step;
    }
    if (y + step <= bottom &&
        (type == TRACKER_AIRTAG || type == TRACKER_FIND_MY_WITH_OWNER || type == TRACKER_OTHER_FIND_MY)) {
        char sbuf[40];
        snprintf(sbuf, sizeof(sbuf), " Device's battery level is %s", findMyBatteryString(c.battery).c_str());
        tft.drawString(String(sbuf), 8, y);
        y += step;
    }

    tft.setTextSize(ROW_TEXT_SIZE);
    tft.drawCentreString(
        "Press " + String(BTN_ALIAS) + " to go back", tftWidth / 2, tftHeight - 17, SMOOTH_FONT
    );
    delay(300);
    while (!check(SelPress)) { yield(); }
}

static void openDetailList() {
    std::vector<TrackerType> present;
    for (uint8_t i = 0; i < TRACKER_TYPE_COUNT; i++) {
        if (g_presence[i].everSeen) present.push_back((TrackerType)i);
    }

    if (present.empty()) {
        displayError("No trackers or glasses found - weird");
        // delay(1200);
        return;
    }

    std::sort(present.begin(), present.end(), [](TrackerType a, TrackerType b) {
        DwellLevel la = dwellLevel(g_presence[a]);
        DwellLevel lb = dwellLevel(g_presence[b]);
        if (la != lb) return la > lb;
        return g_presence[a].maxDwellMs > g_presence[b].maxDwellMs;
    });

    options.clear();
    for (TrackerType t : present) {
        tft.setTextSize(ROW_TEXT_SIZE == 12);
        String entry =
            getTrackerTypeString(t) + " was nearby for " + formatDuration(g_presence[t].maxDwellMs);
        options.emplace_back(entry.c_str(), [t]() { drawTrackerDetail(t); });
    }
    addOptionToMainMenu();
    tft.setTextSize(ROW_TEXT_SIZE == 12);
    loopOptions(options);
    options.clear();
}

#define MONITOR_TAGLINE_Y (UI_TOP + 3)
#define MONITOR_COUNTDOWN_Y (MONITOR_TAGLINE_Y + 15)
#define MONITOR_ROWS_TOP (MONITOR_COUNTDOWN_Y + 15)
#define MONITOR_ROW_STEP (ROW_TEXT_SIZE + 13)

static void drawMonitorChrome() {
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();

    tft.setTextSize(FP);
    tft.drawCentreString(
        String(BTN_ALIAS) + " = device selection | Esc = quit", tftWidth / 2, tftHeight - 16, SMOOTH_FONT
    );

    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.drawCentreString(
        "Uncover nearby trackers & ai glasses", tftWidth / 2, MONITOR_TAGLINE_Y, SMOOTH_FONT
    );
}

static void drawCountdown(uint32_t msRemaining) {
    const uint32_t total = SCAN_CYCLE_MS / 1000;
    uint32_t secs = (msRemaining + 999) / 1000;
    if (secs > total) secs = total;

    tft.setTextSize(ROW_TEXT_SIZE);
    tft.fillRect(UI_LEFT, MONITOR_COUNTDOWN_Y, UI_CLEAR_W, ROW_TEXT_SIZE + 5, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString(
        "Refreshing in " + String(secs) + "s", tftWidth / 2, MONITOR_COUNTDOWN_Y, SMOOTH_FONT
    );
}

static int monitorMaxRows() {
    const int bottomMargin = 17;
    int maxRows = (tftHeight - bottomMargin - MONITOR_ROWS_TOP) / (MONITOR_ROW_STEP + 1);
    return maxRows < 1 ? 1 : maxRows;
}

// Rebuilds the visible row set. Call this when the presence data changed, not on
// every repaint -- scrolling only moves a window over an unchanged list.
static void rebuildMonitorRows() {
    // The sort below re-runs every cycle, so a bare integer offset would end up
    // pointing at a different category after a refresh and the list would appear
    // to jump while being read. Pin the window to whatever was on the top line.
    bool hadRows = !g_monitorRows.empty();
    TrackerType pinned = TRACKER_UNKNOWN;
    if (hadRows && g_monitorScroll < (int)g_monitorRows.size())
        pinned = g_monitorRows[g_monitorScroll];

    g_monitorRows.clear();
    for (uint8_t i = 0; i < TRACKER_TYPE_COUNT; i++) {
        if (g_presence[i].consecutive > 0) g_monitorRows.push_back((TrackerType)i);
    }

    // Glasses first, then longest dwell.
    // A camera in range outranks a tag that has merely been around a while.
    std::sort(g_monitorRows.begin(), g_monitorRows.end(), [](TrackerType a, TrackerType b) {
        if (isGlassesType(a) != isGlassesType(b)) return isGlassesType(a);
        DwellLevel la = dwellLevel(g_presence[a]);
        DwellLevel lb = dwellLevel(g_presence[b]);
        if (la != lb) return la > lb;
        return dwellMs(g_presence[a]) > dwellMs(g_presence[b]);
    });

    if (hadRows) {
        g_monitorScroll = 0;
        for (size_t i = 0; i < g_monitorRows.size(); i++) {
            if (g_monitorRows[i] == pinned) {
                g_monitorScroll = (int)i;
                break;
            }
        }
    }
}

static void drawMonitorRows() {
    const int top = MONITOR_ROWS_TOP;
    const int step = MONITOR_ROW_STEP + 1;
    const int maxRows = monitorMaxRows();

    tft.fillRect(8, top, tftWidth - 16, maxRows * step, bruceConfig.bgColor);

    int maxScroll = (int)g_monitorRows.size() - maxRows;
    if (maxScroll < 0) maxScroll = 0;
    if (g_monitorScroll > maxScroll) g_monitorScroll = maxScroll;
    if (g_monitorScroll < 0) g_monitorScroll = 0;

    int shown = (int)g_monitorRows.size() - g_monitorScroll;
    if (shown > maxRows) shown = maxRows;

    int y = top;
    for (int i = 0; i < shown; i++) {
        TrackerType type = g_monitorRows[g_monitorScroll + i];
        const CategoryPresence &c = g_presence[type];
        tft.setTextColor(rowColor(type, dwellLevel(c)), bruceConfig.bgColor);
        tft.drawString(
            " " + getTrackerShortString(type) + ";" + " " + getProximityString(c.proximity) + " " +
                formatDurationShort(dwellMs(c)),
            8,
            y
        );
        y += step;
    }

    // Carets rather than a "+N more" line: on a 135px panel there are only three
    // row slots, and spending one on an overflow note costs a third of the view.
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    if (g_monitorScroll > 0) tft.drawString("^", tftWidth - 14, top);
    if (g_monitorScroll < maxScroll) tft.drawString("v", tftWidth - 14, top + (shown - 1) * step);
}

void ble_tracker_detector() {
    tft.fillScreen(bruceConfig.bgColor);
    displayTextLine("LOADING...");

    resetPresence();

    bool bleWasActiveBefore = BLEConnected || (BLEDevice::getServer() != nullptr);
#if !defined(LITE_VERSION)
    bleWasActiveBefore =
        bleWasActiveBefore || BLEStateManager::isBLEActive() || BLEStateManager::getActiveClientCount() > 0;
#endif

    if (!ble_scan_setup() || !pBLEScan) {
        displayError("Failed to init BLE scan");
        return;
    }

    {
        ScanSettingsGuard guard(pBLEScan);

        pBLEScan->setScanCallbacks(&trackerCallbacks, true);
        pBLEScan->setMaxResults(0);
        pBLEScan->clearResults();

        drawMonitorChrome();
        rebuildMonitorRows();
        drawMonitorRows();

        pBLEScan->start(0, false, true);

        uint32_t cycleEnd = millis() + SCAN_CYCLE_MS;
        uint32_t lastTick = 0;

        while (!check(EscPress)) {
            uint32_t now = millis();

            if ((int32_t)(now - cycleEnd) >= 0) {
                closePresenceCycle();
                cycleEnd = now + SCAN_CYCLE_MS;
                rebuildMonitorRows();
                drawMonitorRows();
                lastTick = 0; // force countdown repaint
            }

            if (now - lastTick >= 200) {
                lastTick = now;
                drawCountdown(cycleEnd - now);
            }

            if (check(SelPress)) {
                pBLEScan->stop();
                openDetailList();
                drawMonitorChrome();
                drawMonitorRows();
                pBLEScan->start(0, false, true);
                cycleEnd = millis() + SCAN_CYCLE_MS;
            }

            // Repaint immediately instead of waiting for the countdown tick, so a
            // scroll step lands as soon as the key is read.
            if (check(PrevPress) && g_monitorScroll > 0) {
                g_monitorScroll--;
                drawMonitorRows();
            }
            if (check(NextPress)) {
                g_monitorScroll++; // drawMonitorRows() clamps it back if that overshot
                drawMonitorRows();
            }

            if (!pBLEScan->isScanning()) { pBLEScan->start(0, false, true); }

            vTaskDelay(pdMS_TO_TICKS(20));
        }

        pBLEScan->stop();
    }

    if (!bleWasActiveBefore) {
#if !defined(LITE_VERSION)
        if (!BLEStateManager::isBLEActive()) { stopBLEStack(); }
#else
        stopBLEStack();
#endif
    }

    openDetailList();
}
