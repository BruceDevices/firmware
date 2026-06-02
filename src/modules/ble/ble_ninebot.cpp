/**
 * @file ble_ninebot.cpp
 * @author Sor3nt (https://github.com/Sor3nt)
 * @brief Scooter Tuning
 * @version 0.1
 * @date 2025-04-26
 * @credits thanks "mr unknown" for the payloads
 */
#if !defined(LITE_VERSION)
#include "ble_ninebot.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <functional>
#include <vector>

#define SCAN_TIME 5        // Scan duration in seconds
#define SCAN_INTERVAL 100  // BLE scan interval
#define SCAN_WINDOW 99     // BLE scan window
#define CMD_DELAY 500      // UI delay after commands
#define UI_READ_DELAY 2000 // UI delay for read feedback

#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
#endif

#ifdef NIMBLE_V2_PLUS
#define __Override__
#else
#define __Override__ override
#endif

static NimBLEScan *pBLEScan;
static NimBLEClient *pClient = nullptr;

// Nordic UART Service (NUS)
static NimBLEUUID uartServiceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID txCharUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");

bool scooterDisconnected = true;

class ScooterClientCallbacks : public NimBLEClientCallbacks {
    void onDisconnect(NimBLEClient *client) __Override__ { scooterDisconnected = true; }
};

static const uint8_t max2Gpayload[] = {
    0x55, 0xAB, 0x4D, 0x41, 0x58, 0x32, 0x53, 0x63, 0x6F, 0x6F, 0x74, 0x65, 0x72, 0x5F, 0x31
};
static const uint8_t f2payload[] = {
    0x55, 0xAB, 0x46, 0x32, 0x53, 0x63, 0x6F, 0x6F, 0x74, 0x65, 0x72, 0x5F, 0x31
};
static const uint8_t gen1[] = {0x5A, 0xA5, 0x00, 0x4B, 0x48, 0x94, 0xE3, 0x3A, 0x91, 0xE0, 0x32, 0x7E, 0xC2};
static const uint8_t gen2[] = {0x5A, 0xA5, 0x00, 0x4B, 0x48, 0x94, 0xE3, 0x3A, 0x91, 0xE0, 0x43, 0x3E, 0xC2};
static const uint8_t gen3[] = {0x5A, 0xA5, 0x00, 0x4B, 0x48, 0x94, 0xE3, 0x3A, 0x91, 0xE0, 0x42, 0x7E, 0xC4};

struct PayloadDef {
    const char *label;
    const uint8_t *data;
    size_t len;
};
static const PayloadDef payloads[] = {
    {"Ninebot Max 2G / G30", max2Gpayload, sizeof(max2Gpayload)},
    {"Ninebot F2",           f2payload,    sizeof(f2payload)   },
    {"Ninebot Generic 1",    gen1,         sizeof(gen1)        },
    {"Ninebot Generic 2",    gen2,         sizeof(gen2)        },
    {"Ninebot Generic 3",    gen3,         sizeof(gen3)        }
};

static std::vector<Option>
buildModelOptions(NimBLERemoteCharacteristic *pTXChar, std::vector<Option> &deviceSelection) {
    std::vector<Option> charOptions;
    charOptions.reserve(sizeof(payloads) / sizeof(payloads[0]) + 1);

    for (size_t i = 0; i < sizeof(payloads) / sizeof(payloads[0]); ++i) {
        const PayloadDef &pd = payloads[i];
        charOptions.push_back({pd.label, [pTXChar, &deviceSelection, &pd](void) {
                                   bool success = pTXChar->writeValue(pd.data, pd.len, true);
                                   displayTextLine(success ? "Write success!" : "Write failed!");
                                   delay(UI_READ_DELAY);
                                   std::vector<Option> nextOpts = buildModelOptions(pTXChar, deviceSelection);
                                   loopOptions(nextOpts);
                               }});
    }

    charOptions.push_back({"Back", [&](void) {
                               pClient->disconnect();
                               scooterDisconnected = true;
                               delay(CMD_DELAY);
                               loopOptions(deviceSelection);
                           }});

    return charOptions;
}

BLENinebot::BLENinebot() { setup(); }
BLENinebot::~BLENinebot() {
    if (!scooterDisconnected) pClient->disconnect();
    pBLEScan->clearResults();
}

void BLENinebot::clientDisconnect() {
    if (!scooterDisconnected) {
        pClient->disconnect();
        scooterDisconnected = true;
    }
}

void BLENinebot::setup() {
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(SCAN_INTERVAL);
    pBLEScan->setWindow(SCAN_WINDOW);

    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new ScooterClientCallbacks(), false);
    pClient->setConnectTimeout(3);

    delay(CMD_DELAY);
    loop();
}

void BLENinebot::redrawMainBorder() {
    drawMainBorder();
    tft.drawString("-=Ninebot Tuning=-", (tftWidth / 2) - ((18 * 6) / 2), 12);
}

void BLENinebot::loop() {
    std::vector<Option> deviceSelection;

    while (!check(EscPress)) {
        redrawMainBorder();
        displayTextLine("Scanning...");
#ifdef NIMBLE_V2_PLUS
        NimBLEScanResults results = pBLEScan->getResults(SCAN_TIME * 1000, false);
#else
        NimBLEScanResults results = pBLEScan->start(SCAN_TIME, false);
#endif
        if (check(EscPress)) return;

        if (results.getCount() == 0) {
            displayTextLine("No Scooter found. Retry...");
            delay(UI_READ_DELAY);
            pBLEScan->clearResults();
            deviceSelection.clear();
            continue;
        }

        deviceSelection.clear();
        deviceSelection.reserve(results.getCount() + 2);

        for (int i = 0; i < results.getCount(); ++i) {
#ifdef NIMBLE_V2_PLUS
            const NimBLEAdvertisedDevice *adv = results.getDevice(i);
            String name = adv->getName().length() ? String(adv->getName().c_str())
                                                  : String(adv->getAddress().toString().c_str());
#else
            NimBLEAdvertisedDevice adv = results.getDevice(i);
            String name = adv.getName().length() ? String(adv.getName().c_str())
                                                 : String(adv.getAddress().toString().c_str());
#endif

            deviceSelection.push_back(
                {name, [&, adv](void) mutable {
                     redrawMainBorder();
                     displayTextLine("Connecting...");
                     scooterDisconnected = false;
#ifdef NIMBLE_V2_PLUS
                     if (!pClient->connect(adv->getAddress()))
#else
                     if (!pClient->connect(adv.getAddress()))
#endif
                     {
                         displayTextLine("Connection failed.");
                         delay(UI_READ_DELAY);
                         clientDisconnect();
                         loopOptions(deviceSelection);
                         return;
                     }

                     displayTextLine("Connected!");
                     if (!pClient->discoverAttributes()) {
                         displayTextLine("Discover failed.");
                         clientDisconnect();
                         loopOptions(deviceSelection);
                         return;
                     }

                     NimBLERemoteService *svc = pClient->getService(uartServiceUUID);
                     if (svc != nullptr) {
                         NimBLERemoteCharacteristic *ch = svc->getCharacteristic(txCharUUID);
                         if (ch != nullptr && (ch->canWrite() || ch->canWriteNoResponse())) {
                             std::vector<Option> nextOpts = buildModelOptions(ch, deviceSelection);
                             loopOptions(nextOpts);
                             clientDisconnect();
                             loopOptions(deviceSelection);
                             return;
                         } else {
                             displayTextLine("TX not writable");
                         }
                     } else {
                         displayTextLine("Not a scooter");
                     }

                     clientDisconnect();
                     loopOptions(deviceSelection);
                 }}
            );
        }

        bool returnToMenu = false;
        deviceSelection.push_back({"Scan again", [&]() { returnToMenu = false; }});
        deviceSelection.push_back({"Main Menu", [&]() { returnToMenu = true; }});
        loopOptions(deviceSelection);

        if (returnToMenu) return;

        pBLEScan->clearResults();
    }
}
#endif
