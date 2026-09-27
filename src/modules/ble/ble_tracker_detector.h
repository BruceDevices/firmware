#ifndef __BLE_TRACKER_DETECTOR_H__
#define __BLE_TRACKER_DETECTOR_H__

#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>

// ---------------------------------------------------------------------------
// What this module tracks, and why it is not keyed on MAC address
// ---------------------------------------------------------------------------
//
// Both things we care about re-randomise their advertising address roughly
// every 15 minutes: Apple rotates the Find My key (and the address IS the
// first 6 bytes of that key), and Meta's glasses use resolvable private
// addresses on a similar cadence. So "I have seen THIS address for 20 minutes"
// is a question the radio cannot answer.
//
// What does survive a rotation is the classification: the Apple offline-
// finding payload shape, the Luxottica company ID, the FD5F service UUID, the
// advertised name. So this module tracks PRESENCE PER CATEGORY -- "a separated
// AirTag has been within range continuously for 20 minutes" -- which is both
// answerable and the thing you actually want to know.

enum TrackerType : uint8_t {
    TRACKER_UNKNOWN = 0,

    // Trackers. TRACKER_AIRTAG means separated AND not maintained: the tag
    // has not been in contact with its owner during this key rotation period.
    TRACKER_AIRTAG,
    TRACKER_FIND_MY_WITH_OWNER,
    TRACKER_OTHER_FIND_MY,
    TRACKER_TILE,
    TRACKER_SMARTTAG,
    TRACKER_AIRPODS,

    // Glasses / cameras. Keep these contiguous -- isGlassesType() depends on it.
    GLASSES_META,
    GLASSES_SNAP,
    GLASSES_OTHER,
    GLASSES_CAMERA_GENERIC,

    TRACKER_TYPE_COUNT
};

inline bool isGlassesType(TrackerType type) { return type >= GLASSES_META && type <= GLASSES_CAMERA_GENERIC; }

// Distance bands only. Movement direction was removed: it needs a stable
// identity across samples to mean anything, and a rotating address does not
// provide one -- the "moving away" you'd compute is usually just the key
// rotating mid-window.
enum OwnerProximity : uint8_t {
    PROXIMITY_UNKNOWN = 0,
    PROXIMITY_NEAR,
    PROXIMITY_MEDIUM,
    PROXIMITY_FAR,
};

// Escalation is driven by dwell time, with the ~15 minute rotation period as
// the meaningful boundary: something still with you after its address changed
// is not something you merely walked past.
enum DwellLevel : uint8_t {
    DWELL_NONE = 0,
    DWELL_PRESENT, // here now, briefly
    DWELL_WATCH,   // several minutes
    DWELL_ALERT,   // outlasted one address rotation
    DWELL_STRONG,  // outlasted two, and present most of the time
};

// Apple battery state, from status bits 6-7. Only documented as meaningful
// when the maintained bit is set, so it reads UNKNOWN for separated tags --
// see the note in the .cpp before relying on it.
enum FindMyBattery : uint8_t {
    FINDMY_BATT_UNKNOWN = 0,
    FINDMY_BATT_FULL,
    FINDMY_BATT_MEDIUM,
    FINDMY_BATT_LOW,
    FINDMY_BATT_CRITICAL,
};

#define MAX_ADDRS_PER_CATEGORY 24

struct CategoryPresence {
    bool everSeen;

    // Dwell is the CURRENT continuous run, not the all-time span: something
    // seen at 09:00 and again at 10:00 has not been with you for an hour.
    uint32_t runStartMs;
    uint32_t lastSeenMs;
    uint32_t maxDwellMs; // longest run this session
    uint8_t missedCycles;

    uint16_t cyclesPresent;
    uint16_t consecutive;
    uint16_t maxConsecutive;

    int bestRSSI; // closest this category has ever been
    int lastRSSI; // best in the most recent cycle that saw it
    OwnerProximity proximity;

    uint8_t confidence;
    const char *label;
    String name;

    FindMyBattery battery;
    uint8_t rawStatus; // Apple only; kept for logging unknown bits

    // Bounded set of addresses seen, to tell one rotating device from a crowd.
    uint32_t addrHashes[MAX_ADDRS_PER_CATEGORY];
    uint8_t addrCount;
    bool addrOverflow;

    // per-cycle scratch
    bool cycleHit;
    int cycleBestRSSI;
};

String getTrackerTypeString(TrackerType type);
String getProximityString(OwnerProximity prox);
String getDwellLevelString(DwellLevel level);
String findMyBatteryString(FindMyBattery battery);

// Live monitoring screen. Esc exits, select opens the detail list.
void ble_tracker_detector();

#endif
