#include "LoRaRF.h"
#include "core/config.h"
#include "core/configPins.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "libs/nanopb/pb_decode.h"
#include "libs/nanopb/pb_encode.h"
#include <Arduino.h>
#include <RadioLib.h>
#include <vector>

// Forward declarations of protobuf structs if headers are not yet in include path
// You will need to add the nanopb library and the generated mesh.pb.h/cpp to your project
// For now, we assume standard Meshtastic Protobuf definitions are available or mocked
// #include "meshtastic/mesh.pb.h" 
// #include "meshtastic/portnums.pb.h"

// Meshtastic Frequency Plans (US) - Channel 20 (LongFast default slot? No, typically 906.875 or similar for US)
// Default LongFast for US is usually 906.875 MHz, EU 869.525, etc.
// We will use a default, but allow user to change it.
#define MESHTASTIC_FREQ_US 906.875
#define MESHTASTIC_FREQ_EU 869.525

// Meshtastic LoRa Settings (LongFast)
#define MESH_BW 250.0  // 250 kHz
#define MESH_SF 11     // Spreading Factor 11
#define MESH_CR 5      // Coding Rate 4/5
#define MESH_SYNC 0x2B // Meshtastic Sync Word
#define MESH_PREAMBLE 16

extern BruceConfigPins bruceConfigPins;
extern SPIClass *loraSpi;
extern Module *loraModule;
extern SX1276 *lora1276;
extern SX1262 *lora1262;
extern bool intlora;
extern volatile bool loraPacketReceived;
extern volatile bool loraInterruptEnabled;

// Reuse LoRaRF.cpp helpers if possible, or reimplement
extern SPIClass *selectLoraSPIBus();
extern int getLoraIrqPin();
extern int getLoraResetPin();
extern int getLoraBusyPin();
extern int getLoraCsPin();
extern void clearLoraRadio();
extern void onLoraPacket();

// Local vars
static String meshName = "BruceNode";
static uint32_t meshID = 0x12345678; // Should be unique (MAC derived)
static std::vector<String> meshMessages;
static int meshScrollOffset = 0;
static const int meshMaxMessages = 15;
static bool meshUpdate = true;

// Mocking Protobuf structures if headers not present to allow compilation of structure
// In a real implementation, link against generated .pb.h files
struct MeshPacketMock {
    uint32_t from;
    uint32_t to;
    uint32_t id;
    uint8_t payload[240];
    size_t payload_len;
    uint8_t hop_limit;
};

void initMeshtasticRadio(float freq) {
    intlora = false;
    loraPacketReceived = false;
    loraInterruptEnabled = true;

    // Pin checks (same as LoRaRF)
    if (getLoraCsPin() == GPIO_NUM_NC) {
        displayError("LoRa Pins Not Configured");
        return;
    }

    loraSpi = selectLoraSPIBus();
    clearLoraRadio();

    // Init Radio
    int state = RADIOLIB_ERR_NONE;
    int busy = getLoraBusyPin(); // Make sure this is defined/externed correctly
    // Re-check loraRadioVariant from config if needed, but we'll assume global or auto-detect
    
    // Default to SX1276 for this example if null
    loraModule = new Module(getLoraCsPin(), getLoraIrqPin(), getLoraResetPin(), busy, *loraSpi);
    
    // We try to init as SX1276 first (common) or check config
    // For simplicity, using the globals from LoRaRF.cpp
    // NOTE: This logic mimics startLoraRadio but applies MESH settings
    
    // Try to detect or use config? We'll assume the pointers are managed by clearLoraRadio
    // Let's re-instantiate based on a simplistic check or config
    // ideally we read `loraRadioVariant` from LoRaRF.cpp
    
    // Force SX1276 for demo validity unless 1262 is required
    lora1276 = new SX1276(loraModule);
    state = lora1276->begin(freq);
    if (state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
         delete lora1276; 
         lora1276 = nullptr;
         lora1262 = new SX1262(loraModule);
         state = lora1262->begin(freq);
    }

    if (state == RADIOLIB_ERR_NONE) {
        // Apply Meshtastic Settings
        if (lora1276) {
             lora1276->setSpreadingFactor(MESH_SF);
             lora1276->setBandwidth(MESH_BW);
             lora1276->setCodingRate(MESH_CR);
             lora1276->setSyncWord(MESH_SYNC);
             lora1276->setPreambleLength(MESH_PREAMBLE);
             lora1276->setDio0Action(onLoraPacket, CHANGE);
             lora1276->startReceive();
        } else if (lora1262) {
             lora1262->setSpreadingFactor(MESH_SF);
             lora1262->setBandwidth(MESH_BW);
             lora1262->setCodingRate(MESH_CR);
             lora1262->setSyncWord(MESH_SYNC);
             lora1262->setPreambleLength(MESH_PREAMBLE);
             lora1262->setDio1Action(onLoraPacket);
             lora1262->startReceive();
        }
        intlora = true;
        Serial.println("Meshtastic Radio Started on " + String(freq) + " MHz");
    } else {
        displayError("Radio Init Failed");
        Serial.println("Radio Error: " + String(state));
    }
}

void renderMeshChat() {
    if (!meshUpdate) return;
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN);
    
    tft.drawString("Meshtastic: " + meshName, 5, 5);
    tft.drawLine(0, 20, tftWidth, 20, TFT_WHITE);

    int y = 25;
    int h = 10;
    
    int start = meshScrollOffset;
    if (start < 0) start = 0;
    
    for (size_t i = start; i < meshMessages.size(); i++) {
        if (y > tftHeight - 20) break;
        tft.drawString(meshMessages[i], 5, y);
        y += h;
    }
    
    // Status Bar
    tft.fillRect(0, tftHeight - 15, tftWidth, 15, 0x2124); // Dark Grey
    if (intlora) tft.drawString("LR: ON", 5, tftHeight - 12);
    else tft.drawString("LR: OFF", 5, tftHeight - 12);
    
    meshUpdate = false;
}

void processMeshPacket() {
    if (!loraPacketReceived) return;
    loraInterruptEnabled = false;
    loraPacketReceived = false;

    // Read Data
    String packetData;
    int state = RADIOLIB_ERR_NONE;
    float snr = 0;
    float rssi = 0;

    if (lora1276) {
        state = lora1276->readData(packetData);
        snr = lora1276->getSNR();
        rssi = lora1276->getRSSI();
        lora1276->startReceive();
    } else if (lora1262) {
        state = lora1262->readData(packetData);
        snr = lora1262->getSNR();
        rssi = lora1262->getRSSI();
        lora1262->startReceive();
    }

    if (state == RADIOLIB_ERR_NONE) {
        // Decode Logic goes here
        // Note: Real implementation needs Nanopb to decode packetData bytes
        // For now we just dump raw hex or ASCII
        
        Serial.println("Mesh Packet Rx: " + String(packetData.length()) + " bytes");
        
        // Pseudo-decoding for visualization
        String displayMsg = "Rx(" + String((int)rssi) + "): ";
        
        // Heuristic: Check if it looks like a text message (portnum is encrypted usually)
        // Without AES, we can only see unencrypted headers.
        // If we assumed user sends raw text (not standard Meshtastic):
        displayMsg += packetData;
        
        meshMessages.push_back(displayMsg);
        if (meshMessages.size() > meshMaxMessages) {
             meshScrollOffset = meshMessages.size() - meshMaxMessages;
        }
        meshUpdate = true;
    }
    
    loraInterruptEnabled = true;
}

void sendMessageMesh() {
    // 1. Get Input
    String text = keyboard("", 100, "Send Mesh Msg");
    if (text == "") return;

    // 2. Encapsulate in Protobuf (Pseudo-code as we lack headers locally in this clip)
    // meshtastic_MeshPacket p = ...
    // p.payload = text...
    // pb_encode(...)
    
    // For this example, sending RAW text so other BRUCE devices on same settings can read it
    // Real Meshtastic nodes will ignore this as "Unknown/Garbage" if not protobuf
    
    String payload = text; 
    
    // 3. Transmit
    int state = RADIOLIB_ERR_NONE;
    if (lora1276) {
        state = lora1276->transmit(payload);
        lora1276->startReceive();
    } else if (lora1262) {
        state = lora1262->transmit(payload);
        lora1262->startReceive();
    }

    if (state == RADIOLIB_ERR_NONE) {
        meshMessages.push_back("Me: " + text);
         if (meshMessages.size() > meshMaxMessages) {
             meshScrollOffset = meshMessages.size() - meshMaxMessages;
        }
        meshUpdate = true;
    } else {
        displayError("Tx Failed");
    }
}

void meshtastic_app() {
    // Setup
    tft.fillScreen(TFT_BLACK);
    displayRedStripe("Starting Meshtastic...");
    
    // Frequency selection
    // Simple verification - usually prompted or config
    initMeshtasticRadio(MESHTASTIC_FREQ_US); // Defaulting to US for demo
    
    meshUpdate = true;
    
    while (true) {
        renderMeshChat();
        processMeshPacket();
        
        // Input Handling
        if (check(SelPress)) {
            sendMessageMesh();
            meshUpdate = true;
        }
        
        if (check(EscPress)) {
            // Exit
            break;
        }
        
        if (check(NextPress)) {
            if (meshScrollOffset < (int)meshMessages.size() - 5) {
                meshScrollOffset++;
                meshUpdate = true;
            }
        }
        if (check(PrevPress)) {
            if (meshScrollOffset > 0) {
                meshScrollOffset--;
                meshUpdate = true;
            }
        }
        
        delay(10);
    }
    
    // Cleanup / Restore default radio state if needed
    intlora = false;
    clearLoraRadio();
}
