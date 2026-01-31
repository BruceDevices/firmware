#include "LoRaRF.h"
#include "core/config.h"
#include "core/configPins.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <RadioLib.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <vector>

// Helper to access LoRa params in LoRaRF.cpp
extern int spreadingFactor;
extern float signalBandwidth;
extern int codingRateDenominator;
extern int preambleLength;
extern void selectRadioVariant(JsonDocument &doc);

// Meshtastic Constants
#define MESH_BW 250.0
#define MESH_SF 11
#define MESH_CR 5
#define MESH_PREAMBLE 16

void meshtastic() {
    // 1. Setup Meshtastic Params
    spreadingFactor = MESH_SF;
    signalBandwidth = MESH_BW * 1000;
    codingRateDenominator = MESH_CR;
    preambleLength = MESH_PREAMBLE;

    // 2. Load Config & Select Radio
    if (!LittleFS.exists("/lora_settings.json")) {
        File file = LittleFS.open("/lora_settings.json", "w");
        JsonDocument doc;
        doc["LoRa_Frequency"] = "906875000.00"; // default US
        doc["LoRa_Name"] = "BruceMesh";
        doc["LoRa_Radio"] = "SX1278";
        serializeJson(doc, file);
        file.close();
    }
    File file = LittleFS.open("/lora_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, file);
    file.close();

    // Ensure the radio variant global is set correctly
    selectRadioVariant(doc);

    // 3. Frequency Selection
    double BAND = doc["LoRa_Frequency"].as<String>().toDouble();
    if (BAND == 0) BAND = 906875000;
    float bandMHz = (BAND > 1000) ? BAND / 1000000.0f : BAND;

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Starting Meshtastic...", tftWidth / 2, tftHeight / 2);

    // 4. Init Radio (using shared logic)
    // using startLoraRadio from LoRaRF.cpp handles pins and SPI selection
    if (!startLoraRadio(bandMHz)) {
        displayError("LoRa Init Fail");
        return;
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Meshtastic Listener", 10, 10);
    tft.drawString("Freq: " + String(bandMHz) + " MHz", 10, 30);
    tft.drawString("SF11 / BW250 / CR4/5", 10, 50);
    tft.drawString("Press ESC to exit", 10, 70);

    // 6. Loop
    bool quit = false;
    while (!quit) {
        if (loraPacketReceived) {
            loraInterruptEnabled = false;
            loraPacketReceived = false;

            // Read raw Bytes
            int packetLen = 0;
            byte byteArr[256];
            int state = RADIOLIB_ERR_NONE;
            float rssi = 0;

            if (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262) {
                packetLen = lora1262->getPacketLength();
                state = lora1262->readData(byteArr, packetLen);
                rssi = lora1262->getRSSI();
            } else if (loraRadioVariant == LoRaRadioVariant::SX1278 && lora1278) {
                packetLen = lora1278->getPacketLength();
                state = lora1278->readData(byteArr, packetLen);
                rssi = lora1278->getRSSI();
            } else if (loraRadioVariant == LoRaRadioVariant::SX1276 && lora1276) {
                packetLen = lora1276->getPacketLength();
                state = lora1276->readData(byteArr, packetLen);
                rssi = lora1276->getRSSI();
            }

            if (state == RADIOLIB_ERR_NONE) {
                Serial.println("Mesh Packet Rx: " + String(packetLen) + " bytes");
                tft.drawString(
                    "Rx: " + String(packetLen) + "b " + String(rssi) + "dBm", 10, 90 + (rand() % 100)
                );

                // TODO: Nanopb Decode here
                // 1. pb_decode MeshPacket
                // 2. AES Decrypt
                // 3. Print
            }

            // Restart Rx
            if (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262) lora1262->startReceive();
            else if (loraRadioVariant == LoRaRadioVariant::SX1278 && lora1278) lora1278->startReceive();
            else if (lora1276) lora1276->startReceive();

            loraInterruptEnabled = true;
        }

        if (check(EscPress)) quit = true;
        delay(10);
    }

    // Restore defaults
    spreadingFactor = 9;
    signalBandwidth = 31.25E3;
    codingRateDenominator = 8;
    preambleLength = 8;
}
