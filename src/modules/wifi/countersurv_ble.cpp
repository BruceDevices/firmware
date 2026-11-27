#include "countersurv.h"

#include "core/radio_mem.h"
#include "modules/ble/ble_common.h"
#include "wifi_raw_portal.h"
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_heap_caps.h>
#include <globals.h>
#include <string.h>

namespace {
// Bring the NimBLE host up once, and release a half-started controller on
// failure so its RAM goes back instead of leaking. Kept local: upstream's
// ble_common has no such helper and this port does not edit it.
bool cadenceBleInit(const char *name) {
    // Refuse before touching the controller: bringing NimBLE up without the
    // contiguous DMA block is what used to wedge the board inside init.
    if (!radioHasMemForBle()) return false;
    if (NimBLEDevice::isInitialized()) return true;
    if (NimBLEDevice::init(name == nullptr ? "" : name)) return true;
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    Serial.printf(
        "[BLE] init failed; controller released, internal free=%u dma free=%u\n",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA)
    );
    return false;
}

// Tear the host down here rather than through stopBLEStack(): that helper only
// deinits when its own is_ble_inited flag is set, and a cadence pass does not go
// through the BLE menu path, so calling it would leave the controller running
// with its RAM still held. The next Wi-Fi page then had no DMA block to start on.
void cadenceBleStop() {
    NimBLEScan *scan = NimBLEDevice::getScan();
    if (scan != nullptr) {
        scan->stop();
        scan->clearResults();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    if (NimBLEDevice::isInitialized()) NimBLEDevice::deinit(true);
}
} // namespace

// Task 3, BLE half of the dashboard.
//
// A tracker that follows someone rotates its address so a plain MAC table keeps
// forgetting it. What does not change is the advertisement payload (a Find My
// frame, a Tile service data block, a manufacturer-specific blob), so that is
// the key: one signature, many addresses. The gaps between the first sighting of
// each new address are the rotation cadence, and a short steady cadence is the
// signature of something that is moving with you.
namespace CounterSurv {

namespace {

#define BLE_CAD_SIGS 16
#define BLE_CAD_ADDRS 8

struct BleSigEntry {
    uint32_t sig;      // FNV-1a of name + manufacturer data
    char name[24];
    uint8_t addr[BLE_CAD_ADDRS][6];
    uint8_t addrs;     // distinct addresses seen (may exceed BLE_CAD_ADDRS)
    uint16_t seen;
    uint32_t firstMs;
    uint32_t lastMs;
    uint32_t lastNewMs;
    uint32_t sumInt;   // ms, for the average cadence
    uint16_t cntInt;
    uint16_t minInt;
    uint16_t maxInt;   // 0 = no interval yet
    bool used;
};

static BleSigEntry s_bleSigs[BLE_CAD_SIGS];
static uint32_t s_bleSeen = 0;

class BleCadenceCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice *dev) override {
        if (dev == nullptr) return;
        const uint8_t *raw = dev->getAddress().getBase()->val;
        if (raw == nullptr) return;

        // The payload is what survives an address rotation. A scan response often
        // carries only the name, so name + manufacturer data is the key; an
        // advertisement with neither cannot be linked to anything and is skipped.
        const std::string name = dev->getName();
        std::string mfr;
        if (dev->haveManufacturerData()) mfr = dev->getManufacturerData();
        if (name.empty() && mfr.empty()) return;

        uint32_t sig = 2166136261u;
        for (size_t i = 0; i < name.size(); i++) {
            sig ^= (uint8_t)name[i];
            sig *= 16777619u;
        }
        for (size_t i = 0; i < mfr.size(); i++) {
            sig ^= (uint8_t)mfr[i];
            sig *= 16777619u;
        }

        const uint32_t now = millis();
        // No critical section here on purpose. onResult() runs on the NimBLE
        // host task, which is the only writer, and portENTER_CRITICAL() in this
        // callback can either nest on a mux the host already holds (deadlock)
        // or disable interrupts long enough to trip the task watchdog. Both
        // showed up as the counter-surveillance page locking the board up until
        // a reset. The report only runs after scan->stop(), so plain access is
        // enough.
        s_bleSeen++;

        BleSigEntry *e = nullptr;
        for (int i = 0; i < BLE_CAD_SIGS; i++) {
            if (s_bleSigs[i].used && s_bleSigs[i].sig == sig) {
                e = &s_bleSigs[i];
                break;
            }
        }
        if (e == nullptr) {
            for (int i = 0; i < BLE_CAD_SIGS; i++) {
                if (!s_bleSigs[i].used) {
                    e = &s_bleSigs[i];
                    break;
                }
            }
            if (e != nullptr) {
                memset(e, 0, sizeof(*e));
                e->used = true;
                e->sig = sig;
                e->firstMs = now;
                if (!name.empty()) {
                    strncpy(e->name, name.c_str(), sizeof(e->name) - 1);
                    e->name[sizeof(e->name) - 1] = 0;
                }
            }
        }

        if (e != nullptr) {
            e->seen++;
            e->lastMs = now;
            bool known = false;
            for (int a = 0; a < e->addrs && a < BLE_CAD_ADDRS; a++) {
                if (memcmp(e->addr[a], raw, 6) == 0) {
                    known = true;
                    break;
                }
            }
            if (!known) {
                if (e->addrs > 0 && e->lastNewMs != 0) {
                    uint32_t dt = now - e->lastNewMs;
                    const uint16_t capped = (uint16_t)(dt > 0xFFFF ? 0xFFFF : dt);
                    if (e->cntInt < 0xFFFF) {
                        e->sumInt += capped;
                        e->cntInt++;
                    }
                    if (e->minInt == 0 || capped < e->minInt) e->minInt = capped;
                    if (capped > e->maxInt) e->maxInt = capped;
                }
                if (e->addrs < BLE_CAD_ADDRS) memcpy(e->addr[e->addrs], raw, 6);
                if (e->addrs < 0xFF) e->addrs++;
                e->lastNewMs = now;
            }
        }
    }
};

static BleCadenceCallbacks s_bleCadenceCallbacks;

static String bleCadenceReport() {
    String out = "ble tracker cadence: " + String(s_bleSeen) + " advertisements, ";
    int rotating = 0;
    for (int i = 0; i < BLE_CAD_SIGS; i++)
        if (s_bleSigs[i].used && s_bleSigs[i].addrs >= 2) rotating++;
    out += String(rotating) + " rotating payloads\n";

    for (int i = 0; i < BLE_CAD_SIGS; i++) {
        const BleSigEntry &e = s_bleSigs[i];
        if (!e.used || e.addrs < 2) continue;
        const uint32_t avg = e.cntInt ? e.sumInt / e.cntInt : 0;
        out += "  " + String(e.name[0] ? e.name : "unknown") + " sig " + String(e.sig, HEX) + " macs=" +
               String(e.addrs) + " seen=" + String(e.seen);
        if (avg) {
            out += " rotation avg " + String(avg / 1000.0f, 1) + "s (min " + String(e.minInt / 1000.0f, 1) +
                   "s max " + String(e.maxInt / 1000.0f, 1) + "s)";
        }
        // Same payload, several addresses inside one window is the only honest
        // proof of rotation; how fast it rotates is the hint that it moves.
        if (e.addrs >= 3 && avg && avg <= 120000) out += " - FOLLOWING";
        else if (e.addrs >= 3) out += " - ROTATING";
        out += "\n";
    }
    if (rotating == 0) out += "  no payload appeared from more than one address\n";
    return out;
}

} // namespace

String bleCadence(int seconds) {
    // One pass at a time. The dashboard can fire this twice from a double click,
    // and two concurrent NimBLE inits are a guaranteed wedge.
    static bool s_running = false;
    if (s_running) return "ble: pass already running";
    s_running = true;
    struct Guard {
        bool &flag;
        ~Guard() { flag = false; }
    } guard{s_running};

    memset(s_bleSigs, 0, sizeof(s_bleSigs));
    s_bleSeen = 0;

    // A live Wi-Fi connection holds internal DMA RAM that the BLE controller
    // needs. The old page could be opened straight from WiFi > Wifi Atks with
    // the radio still up, which made init fail and the scan return instantly.
    // Drop the station link first; the caller explicitly asked for a BLE pass.
    if (WiFi.getMode() != WIFI_MODE_NULL) {
        WiFi.disconnect(true, false);
        WiFi.mode(WIFI_OFF);
        wifiConnected = false;
        wifiIP = "";
        delay(100);
    }

    if (!cadenceBleInit("vnd-cadence")) return "ble: radio unavailable (no memory for the stack)";
    NimBLEScan *scan = NimBLEDevice::getScan();
    if (scan == nullptr) {
        cadenceBleStop();
        return "ble: scan object unavailable";
    }

    scan->setScanCallbacks(&s_bleCadenceCallbacks, false);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);

    const uint32_t windowMs = (uint32_t)(seconds < 1 ? 1 : seconds) * 1000UL;
    // Duration 0 = scan until stopped. The window is held by the loop below, so
    // the scan cannot outlive the pass and cannot block this task while it runs.
    const bool started = scan->start(0, false, true);
    const uint32_t end = millis() + windowMs;
    while ((int32_t)(millis() - end) < 0) vTaskDelay(pdMS_TO_TICKS(50));

    // Always tear down, including when start() failed half-way.
    cadenceBleStop();

    if (!started) return "ble: scan could not start (radio busy)";

    String out = bleCadenceReport();
    return out;
}

} // namespace CounterSurv
