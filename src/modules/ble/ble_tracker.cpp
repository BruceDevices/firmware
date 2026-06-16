#if !defined(LITE_VERSION)
#include "ble_tracker.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <globals.h>
#include <vector>

struct TrackedDevice {
    std::string address;
    std::string name;
    int rssi;
    unsigned long lastSeen;
};

static std::vector<TrackedDevice> trackedDevices;
static volatile bool trackerRunning = false;

static bool deviceExists(const std::string &addr) {
    for (auto &d : trackedDevices) {
        if (d.address == addr) return true;
    }
    return false;
}

static void updateDevice(const std::string &addr, const std::string &name, int rssi) {
    for (auto &d : trackedDevices) {
        if (d.address == addr) {
            d.rssi = rssi;
            d.lastSeen = millis();
            if (!name.empty()) d.name = name;
            return;
        }
    }
    TrackedDevice dev;
    dev.address = addr;
    dev.name = name;
    dev.rssi = rssi;
    dev.lastSeen = millis();
    trackedDevices.push_back(dev);
}

static void drawTrackerScreen() {
    drawMainBorderWithTitle("BLE Tracker");
    int y = 45;

    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(12, y);
    tft.printf("Devices found: %d", (int)trackedDevices.size());
    y += 16;

    int maxShow = (tftHeight - y - 25) / 14;
    int start = max(0, (int)trackedDevices.size() - maxShow);

    for (int i = start; i < (int)trackedDevices.size() && i - start < maxShow; i++) {
        auto &d = trackedDevices[i];
        tft.setTextColor(d.rssi > -60 ? TFT_GREEN : d.rssi > -80 ? TFT_YELLOW : TFT_RED, bruceConfig.bgColor);
        tft.setCursor(12, y);

        String label = d.name.empty() ? String(d.address.c_str()) : String(d.name.c_str());
        if (label.length() > 20) label = label.substring(0, 19) + "..";
        tft.printf("%s %ddBm", label.c_str(), d.rssi);
        y += 14;
    }

    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("Esc to stop", tftWidth / 2, tftHeight - 20, 1);
}

class TrackerAdvertCallback : public NimBLEAdvertCallbacks {
    void onResult(const NimBLEAdvertDetails *advertDetails) {
        NimBLEAddress addr = advertDetails->getAddress();
        int rssi = advertDetails->getRSSI();
        std::string name = advertDetails->isName() ? advertDetails->getName().c_str() : "";
        updateDevice(addr.toString(), name, rssi);
    }
};

static TrackerAdvertCallback advertCallback;

void bleTracker() {
    NimBLEDevice::deinit(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    if (!NimBLEDevice::init("")) {
        displayError("BLE init failed", true);
        return;
    }

    trackedDevices.clear();
    trackerRunning = true;

    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertCallbacks(&advertCallback);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);

    drawTrackerScreen();

    unsigned long lastDraw = 0;
    while (trackerRunning) {
        pScan->start(500, false);

        if (millis() - lastDraw > 1000) {
            drawTrackerScreen();
            lastDraw = millis();
        }

        if (check(EscPress)) trackerRunning = false;
        esp_task_wdt_reset();
    }

    pScan->stop();
    NimBLEDevice::deinit(true);
    displayInfo("Tracker stopped: " + String((int)trackedDevices.size()) + " devices", true);
}
#endif
