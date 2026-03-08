#if !defined(LITE_VERSION)
#include "rf_chat.h"
#include "core/config.h"
#include "core/configPins.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "globals.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <RadioLib.h>
#include <vector>

// ─────────────────────────────────────────────
//  Module state
// ─────────────────────────────────────────────
extern BruceConfigPins bruceConfigPins;

static bool          rfchat_ready        = false;
static bool          rfchat_update       = false;
static volatile bool rfchat_pktReceived  = false;
static volatile bool rfchat_irqEnabled   = true;

static String rfchat_displayName;
static String rfchat_outMsg;

static std::vector<String> rfchat_messages;
static int rfchat_scrollOffset = 0;
static const int rfchat_maxMessages = 19;

// Layout constants (mirrors LoRaRF)
static const int rfchat_yStart   = 35;
static const int rfchat_ySpacing = 10;

// RadioLib objects
static Module  *rfchat_module = nullptr;
static CC1101  *rfchat_radio  = nullptr;

// ─────────────────────────────────────────────
//  ISR
// ─────────────────────────────────────────────
IRAM_ATTR void onRFChatPacket() {
    if (!rfchat_irqEnabled) return;
    rfchat_pktReceived = true;
}

// ─────────────────────────────────────────────
//  Init / teardown
// ─────────────────────────────────────────────
static void rfchat_clearRadio() {
    if (rfchat_radio)  { delete rfchat_radio;  rfchat_radio  = nullptr; }
    if (rfchat_module) { delete rfchat_module; rfchat_module = nullptr; }
}

/**
 * Start the CC1101 on the configured SPI bus.
 * Returns true on success.
 */
static bool rfchat_startRadio(float freqMHz) {
    rfchat_ready       = false;
    rfchat_pktReceived = false;
    rfchat_irqEnabled  = true;

    // ── Pin validation ──────────────────────────────────────────────────
    int csPin   = bruceConfigPins.CC1101_bus.cs;
    int mosiPin = bruceConfigPins.CC1101_bus.mosi;
    int misoPin = bruceConfigPins.CC1101_bus.miso;
    int sckPin  = bruceConfigPins.CC1101_bus.sck;
    int gdo0Pin = bruceConfigPins.CC1101_bus.io0;  // GDO0 → packet-ready IRQ

    if (csPin == GPIO_NUM_NC || mosiPin == GPIO_NUM_NC ||
        misoPin == GPIO_NUM_NC || sckPin == GPIO_NUM_NC) {
        displayError("CC1101 pins not set!", true);
        return false;
    }
    if (gdo0Pin == GPIO_NUM_NC) {
        displayError("CC1101 GDO0 not set!", true);
        return false;
    }

    // ── SPI bus selection (mirrors LoRaRF pattern) ──────────────────────
    SPIClass *selectedSPI = &CC_NRF_SPI;
    CC_NRF_SPI.begin((int8_t)sckPin, (int8_t)misoPin, (int8_t)mosiPin);

    rfchat_clearRadio();
    rfchat_module = new Module(csPin, gdo0Pin, RADIOLIB_NC, RADIOLIB_NC, *selectedSPI);
    rfchat_radio  = new CC1101(rfchat_module);

    // ── Radio parameters ─────────────────────────────────────────────────
    // 4.8 kbps GFSK – good balance of range / throughput for short messages
    int state = rfchat_radio->begin(freqMHz,   // carrier frequency (MHz)
                                    4.8,        // bit rate        (kbps)
                                    5.0,        // freq deviation  (kHz)
                                    58.5,       // RX bandwidth    (kHz)
                                    10,         // TX power        (dBm)
                                    16);        // preamble length (bytes)

    if (state == RADIOLIB_ERR_NONE) {
        rfchat_radio->setGdo0Action(onRFChatPacket, RISING);
        state = rfchat_radio->startReceive();
    }

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[RF Chat] CC1101 init failed: %d\n", state);
        displayError("CC1101 Init Failed", true);
        rfchat_clearRadio();
        return false;
    }

    rfchat_ready = true;
    Serial.printf("[RF Chat] CC1101 ready @ %.3f MHz\n", freqMHz);
    return true;
}

// ─────────────────────────────────────────────
//  TX
// ─────────────────────────────────────────────
static bool rfchat_sendMessage(String payload) {
    if (!rfchat_ready || !rfchat_radio) return false;
    rfchat_irqEnabled = false;

    int state = rfchat_radio->transmit(payload);
    rfchat_radio->startReceive();
    rfchat_irqEnabled = true;

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[RF Chat] TX failed: %d\n", state);
        displayError("RF send failed");
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────
//  RX
// ─────────────────────────────────────────────
static void rfchat_receiveMessage() {
    if (!rfchat_pktReceived || !rfchat_ready || !rfchat_radio) return;
    rfchat_irqEnabled  = false;
    rfchat_pktReceived = false;

    String incoming;
    int state = rfchat_radio->readData(incoming);

    if (state == RADIOLIB_ERR_NONE && incoming.length() > 0) {
        Serial.println("[RF Chat] RX: " + incoming);
        File f = LittleFS.open("/rf_chats.txt", "a");
        if (f) { f.println(incoming); f.close(); }

        rfchat_messages.push_back(incoming);
        if ((int)rfchat_messages.size() > rfchat_maxMessages)
            rfchat_scrollOffset = rfchat_messages.size() - rfchat_maxMessages;
        rfchat_update = true;
    } else {
        Serial.printf("[RF Chat] RX error: %d\n", state);
    }

    rfchat_radio->startReceive();
    rfchat_irqEnabled = true;
}

// ─────────────────────────────────────────────
//  Render
// ─────────────────────────────────────────────
static void rfchat_render() {
    if (!rfchat_update) return;
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);

    // Header
    tft.setTextColor(0x6DFC);   // greenish – same as LoRa
    if (!rfchat_ready)
        tft.drawString("CC1101 not ready", 10, 13);
    tft.drawString("Nick: " + rfchat_displayName, 10, 25);

    // Messages
    int yPos   = rfchat_yStart;
    int endIdx = rfchat_scrollOffset + rfchat_maxMessages;
    if (endIdx > (int)rfchat_messages.size()) endIdx = rfchat_messages.size();

    for (int i = rfchat_scrollOffset; i < endIdx; i++) {
        tft.setTextColor(bruceConfig.priColor);
        tft.drawString(rfchat_messages[i], 10, yPos);
        yPos += rfchat_ySpacing;
    }

    // Footer hint
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("[SEL] Send  [UP/DN] Scroll  [ESC] Quit", 10, tftHeight - 12);

    rfchat_update = false;
}

// ─────────────────────────────────────────────
//  Load history from flash
// ─────────────────────────────────────────────
static void rfchat_loadMessages() {
    rfchat_messages.clear();
    File f = LittleFS.open("/rf_chats.txt", "r");
    if (!f) return;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length()) rfchat_messages.push_back(line);
    }
    f.close();
    if ((int)rfchat_messages.size() > rfchat_maxMessages)
        rfchat_scrollOffset = rfchat_messages.size() - rfchat_maxMessages;
    else
        rfchat_scrollOffset = 0;
}

// ─────────────────────────────────────────────
//  Action helpers (button callbacks)
// ─────────────────────────────────────────────
static void rfchat_doSend() {
    tft.fillScreen(TFT_BLACK);

    if (!rfchat_ready) {
        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
        tft.drawCentreString("CC1101 not ready!", tftWidth / 2, tftHeight / 2, 2);
        delay(1500);
        rfchat_update = true;
        return;
    }

    String input = keyboard("", 256, "Message:");
    if (input.length() == 0) {
        rfchat_update = true;
        return;
    }

    String full = rfchat_displayName + ": " + input;
    Serial.println("[RF Chat] TX: " + full);

    if (rfchat_sendMessage(full)) {
        File f = LittleFS.open("/rf_chats.txt", "a");
        if (f) { f.println(full); f.close(); }
        rfchat_messages.push_back(full);
        if ((int)rfchat_messages.size() > rfchat_maxMessages)
            rfchat_scrollOffset = rfchat_messages.size() - rfchat_maxMessages;
    }

    tft.fillScreen(TFT_BLACK);
    rfchat_update = true;
}

static void rfchat_scrollUp() {
    if (rfchat_scrollOffset > 0) { rfchat_scrollOffset--; rfchat_update = true; }
}

static void rfchat_scrollDown() {
    if (rfchat_scrollOffset < (int)rfchat_messages.size() - rfchat_maxMessages) {
        rfchat_scrollOffset++;
        rfchat_update = true;
    }
}

// ─────────────────────────────────────────────
//  Main loop
// ─────────────────────────────────────────────
static void rfchat_mainLoop() {
    while (true) {
        rfchat_render();
        rfchat_receiveMessage();

#ifdef HAS_3_BUTTONS
        if (EscPress) {
            long t0 = millis();
            while (EscPress) {
                long elapsed = millis() - (t0 + 200);
                if (elapsed > 0) {
                    int sweep = min((int)(360 * elapsed / 500), 360);
                    tft.drawArc(tftWidth/2, tftHeight/2, 25, 15, 0, sweep,
                                getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            tft.drawArc(tftWidth/2, tftHeight/2, 25, 15, 0, 360,
                        bruceConfig.bgColor, bruceConfig.bgColor);
            if (millis() - t0 > 700) break;         // long press → exit
            check(EscPress);
            rfchat_scrollUp();
        }
        if (check(NextPress)) rfchat_scrollDown();
        if (check(SelPress))  rfchat_doSend();
#else
        if (check(EscPress))  break;
        if (check(PrevPress)) rfchat_scrollUp();
        if (check(NextPress)) rfchat_scrollDown();
        if (check(SelPress))  rfchat_doSend();
#endif

        delay(20);
    }
}

// ─────────────────────────────────────────────
//  Settings helpers (public, used by menu)
// ─────────────────────────────────────────────
void rf_chat_change_username() {
    tft.fillScreen(TFT_BLACK);
    String username = keyboard("", 64, "Username:");
    if (username.length() == 0) return;

    File f = LittleFS.open("/rf_chat_settings.json", "r");
    JsonDocument doc;
    if (f) { deserializeJson(doc, f); f.close(); }
    doc["RF_Name"] = username;
    f = LittleFS.open("/rf_chat_settings.json", "w");
    serializeJson(doc, f);
    f.close();
}

/**
 * Let the user pick a preset frequency or enter a custom one.
 * Valid CC1101 / M5 RF bands: 315, 433, 868, 915 MHz.
 */
void rf_chat_change_freq() {
    std::vector<Option> presets = {
        {"315.000 MHz", [](){}},
        {"433.920 MHz", [](){}},
        {"868.350 MHz", [](){}},
        {"915.000 MHz", [](){}},
        {"Custom",      [](){}},
    };

    int sel = loopOptions(presets, MENU_TYPE_SUBMENU, "RF Chat Frequency");
    if (sel < 0) return;

    float freq = 0.0f;
    if      (sel == 0) freq = 315.000f;
    else if (sel == 1) freq = 433.920f;
    else if (sel == 2) freq = 868.350f;
    else if (sel == 3) freq = 915.000f;
    else {
        // Custom numeric input
        char buf[12] = "433.920";
        String raw = num_keyboard(buf, 10, "Freq (MHz)");
        freq = raw.toFloat();
        if (freq < 300.0f || freq > 928.0f) {
            displayError("Out of range!\n300-928 MHz");
            return;
        }
    }

    File f = LittleFS.open("/rf_chat_settings.json", "r");
    JsonDocument doc;
    if (f) { deserializeJson(doc, f); f.close(); }
    doc["RF_Frequency"] = freq;
    f = LittleFS.open("/rf_chat_settings.json", "w");
    serializeJson(doc, f);
    f.close();
}

// ─────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────
void rf_chat() {
    // ── Ensure settings file exists ─────────────────────────────────────
    if (!LittleFS.exists("/rf_chat_settings.json")) {
        JsonDocument doc;
        doc["RF_Frequency"] = 433.920f;
        doc["RF_Name"]      = "Bruce";
        File f = LittleFS.open("/rf_chat_settings.json", "w");
        serializeJson(doc, f);
        f.close();
        Serial.println("[RF Chat] Created default settings.");
    }

    // ── Ensure chat history file exists ────────────────────────────────
    if (!LittleFS.exists("/rf_chats.txt")) {
        File f = LittleFS.open("/rf_chats.txt", "w");
        f.close();
    }

    // ── Load settings ──────────────────────────────────────────────────
    File f = LittleFS.open("/rf_chat_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, f);
    f.close();

    rfchat_displayName = doc["RF_Name"] | "Bruce";
    float freqMHz      = doc["RF_Frequency"] | 433.920f;

    // Sanity-check stored frequency
    if (freqMHz < 300.0f || freqMHz > 928.0f) {
        displayError("Bad frequency in\nsettings. Reset to\n433.92 MHz");
        freqMHz = 433.920f;
    }

    // ── Show splash ────────────────────────────────────────────────────
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor);
    tft.drawCentreString("RF Chat", tftWidth / 2, 5, 2);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawCentreString(String(freqMHz, 3) + " MHz", tftWidth / 2, 22, 1);
    tft.drawCentreString("Nick: " + rfchat_displayName,  tftWidth / 2, 34, 1);
    tft.drawCentreString("Starting CC1101...",           tftWidth / 2, tftHeight / 2, 1);
    delay(400);

    Serial.printf("[RF Chat] freq=%.3f MHz  nick=%s\n", freqMHz, rfchat_displayName.c_str());
    Serial.printf("[RF Chat] Pins CS:%d MOSI:%d MISO:%d SCK:%d GDO0:%d\n",
        bruceConfigPins.CC1101_bus.cs,   bruceConfigPins.CC1101_bus.mosi,
        bruceConfigPins.CC1101_bus.miso, bruceConfigPins.CC1101_bus.sck,
        bruceConfigPins.CC1101_bus.io0);

    // ── Boot radio ─────────────────────────────────────────────────────
    if (!rfchat_startRadio(freqMHz)) {
        rfchat_clearRadio();
        return;
    }

    tft.setTextWrap(true, true);
    tft.setTextDatum(TL_DATUM);
    rfchat_loadMessages();
    rfchat_update = true;
    rfchat_mainLoop();

    // ── Cleanup ────────────────────────────────────────────────────────
    rfchat_clearRadio();
    rfchat_ready = false;
}
#endif
