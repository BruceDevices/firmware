#include "countersurv.h"

#include "modules/ble/ble_common.h"
#include <WiFi.h>
#include <globals.h>
#include <string.h>

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
static portMUX_TYPE s_bleMux = portMUX_INITIALIZER_UNLOCKED;

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
        portENTER_CRITICAL(&s_bleMux);
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
        portEXIT_CRITICAL(&s_bleMux);
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

    if (!bleInit("vnd-cadence")) return "ble: radio unavailable (no memory for the stack)";
    NimBLEScan *scan = NimBLEDevice::getScan();
    if (scan == nullptr) {
        stopBLEStack();
        return "ble: scan object unavailable";
    }

    scan->setScanCallbacks(&s_bleCadenceCallbacks, false);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);

    const uint32_t windowMs = (uint32_t)(seconds < 1 ? 1 : seconds) * 1000UL;
    // This build's start() returns once the controller has been asked to scan, so
    // the window is held here; a start() that blocks simply makes this loop fall
    // through immediately.
    const bool started = scan->start(windowMs, false, true);
    const uint32_t end = millis() + windowMs + 250;
    while ((int32_t)(millis() - end) < 0) delay(50);
    scan->stop();
    scan->clearResults();
    stopBLEStack();

    if (!started) return "ble: scan could not start (radio busy)";

    String out = bleCadenceReport();
    return out;
}

} // namespace CounterSurv
