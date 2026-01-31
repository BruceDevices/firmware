#if !defined(LITE_VERSION)
#include "LoRaRF.h"
#include "WString.h"
#include "core/config.h"
#include "core/configPins.h"
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <RadioLib.h>
#include <core/display.h>
#include <core/mykeyboard.h>
#include <core/sd_functions.h>
#include <core/utils.h>
#include <globals.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <vector>

extern BruceConfigPins bruceConfigPins;
bool loadonlyradio = false;
bool jam = false;
bool update = false;
String msg;
String rcvmsg;
String displayName;
bool intlora = true;
// scrolling thing
std::vector<String> messages;
int scrollOffset = 0;
const int maxMessages = 19;

int spreadingFactor = 9;
float signalBandwidth = 31.25E3;
int codingRateDenominator = 8;
int preambleLength = 8;

int contentWidth = tftWidth - 20;
int yStart = 35;
int yPos = yStart;
int ySpacing = 10;
int rightColumnX = tftWidth / 2 + 10;
SPIClass *loraSpi = nullptr;
Module *loraModule = nullptr;
SX1276 *lora1276 = nullptr;
SX1278 *lora1278 = nullptr;
SX1262 *lora1262 = nullptr;
volatile bool loraPacketReceived = false;
volatile bool loraInterruptEnabled = true;

LoRaRadioVariant loraRadioVariant = LoRaRadioVariant::SX1278;

int getLoraIrqPin() {
#ifdef LORA_IRQ
    return LORA_IRQ;
#else
    return bruceConfigPins.LoRa_bus.io2;
#endif
}

int getLoraBusyPin() {
#ifdef LORA_BUSY
    return LORA_BUSY;
#else
    return GPIO_NUM_NC;
#endif
}

int getLoraResetPin() { return bruceConfigPins.LoRa_bus.io0; }
int getLoraCsPin() { return bruceConfigPins.LoRa_bus.cs; }

void clearLoraRadio() {
    if (lora1276) {
        delete lora1276;
        lora1276 = nullptr;
    }
    if (lora1278) {
        delete lora1278;
        lora1278 = nullptr;
    }
    if (lora1262) {
        delete lora1262;
        lora1262 = nullptr;
    }
    if (loraModule) {
        delete loraModule;
        loraModule = nullptr;
    }
}

void onLoraPacket() {
    if (!loraInterruptEnabled) return;
    loraPacketReceived = true;
}

SPIClass *selectLoraSPIBus() {
    SPIClass *selectedSPI = &SPI;
    if (bruceConfigPins.LoRa_bus.mosi == TFT_MOSI) {
#if TFT_MOSI > 0
        selectedSPI = &tft.getSPIinstance();
#endif
        Serial.println("Using TFT SPI for LoRa");
    } else if (bruceConfigPins.SDCARD_bus.mosi == bruceConfigPins.LoRa_bus.mosi) {
        selectedSPI = &sdcardSPI;
        Serial.println("Using SDCard SPI for LoRa");
    } else if (bruceConfigPins.NRF24_bus.mosi == bruceConfigPins.LoRa_bus.mosi ||
               bruceConfigPins.CC1101_bus.mosi == bruceConfigPins.LoRa_bus.mosi) {
        selectedSPI = &CC_NRF_SPI;
        CC_NRF_SPI.begin(
            (int8_t)bruceConfigPins.LoRa_bus.sck,
            (int8_t)bruceConfigPins.LoRa_bus.miso,
            (int8_t)bruceConfigPins.LoRa_bus.mosi
        );
        Serial.println("Using CC/NRF SPI for LoRa");
    } else {
        SPI.begin(
            bruceConfigPins.LoRa_bus.sck,
            bruceConfigPins.LoRa_bus.miso,
            bruceConfigPins.LoRa_bus.mosi,
            bruceConfigPins.LoRa_bus.cs
        );
        Serial.println("Using dedicated SPI for LoRa");
    }
    return selectedSPI;
}

bool startLoraRadio(float bandMHz) {
    intlora = false;
    loraPacketReceived = false;
    loraInterruptEnabled = true;
    const int irqPin = getLoraIrqPin();
    if (getLoraCsPin() == GPIO_NUM_NC || bruceConfigPins.LoRa_bus.mosi == GPIO_NUM_NC ||
        bruceConfigPins.LoRa_bus.miso == GPIO_NUM_NC || bruceConfigPins.LoRa_bus.sck == GPIO_NUM_NC) {
        Serial.println("LoRa pins not configured!");
        displayError("LoRa pins not configured!", true);
        return false;
    }
    if (irqPin == GPIO_NUM_NC) {
        Serial.println("LoRa IRQ pin not configured!");
        displayError("LoRa IRQ pin not configured!", true);
        return false;
    }

    loraSpi = selectLoraSPIBus();
    clearLoraRadio();
    const int busyPin = (loraRadioVariant == LoRaRadioVariant::SX1262) ? getLoraBusyPin() : GPIO_NUM_NC;
    if (loraRadioVariant == LoRaRadioVariant::SX1262 && busyPin == GPIO_NUM_NC) {
        Serial.println("Warning: SX1262 selected but BUSY pin is not configured");
    }
    loraModule = new Module(getLoraCsPin(), irqPin, getLoraResetPin(), busyPin, *loraSpi);

    int state = RADIOLIB_ERR_NONE;
    if (loraRadioVariant == LoRaRadioVariant::SX1278) {
        lora1278 = new SX1278(loraModule);
        state = lora1278->begin(bandMHz);
        if (state == RADIOLIB_ERR_NONE) { lora1278->setDio0Action(onLoraPacket, CHANGE); }
        if (state == RADIOLIB_ERR_NONE) state = lora1278->setSpreadingFactor(spreadingFactor);
        if (state == RADIOLIB_ERR_NONE) state = lora1278->setBandwidth(signalBandwidth / 1000.0);
        if (state == RADIOLIB_ERR_NONE) state = lora1278->setCodingRate(codingRateDenominator);
        if (state == RADIOLIB_ERR_NONE) state = lora1278->setPreambleLength(preambleLength);
        if (state == RADIOLIB_ERR_NONE) state = lora1278->startReceive();
    } else if (loraRadioVariant == LoRaRadioVariant::SX1276) {
        lora1276 = new SX1276(loraModule);
        state = lora1276->begin(bandMHz);
        if (state == RADIOLIB_ERR_NONE) { lora1276->setDio0Action(onLoraPacket, CHANGE); }
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setSpreadingFactor(spreadingFactor);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setBandwidth(signalBandwidth / 1000.0);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setCodingRate(codingRateDenominator);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setPreambleLength(preambleLength);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->startReceive();
    } else {
        lora1262 = new SX1262(loraModule);
        state = lora1262->begin(bandMHz);
        if (state == RADIOLIB_ERR_NONE) { lora1262->setDio1Action(onLoraPacket); }
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setSpreadingFactor(spreadingFactor);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setBandwidth(signalBandwidth / 1000.0);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setCodingRate(codingRateDenominator);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setPreambleLength(preambleLength);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->startReceive();
    }

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Starting LoRa failed! Err %d\n", state);
        displayError("LoRa Init Failed", true);
        clearLoraRadio();
        return false;
    }
    intlora = true;
    Serial.println("LoRa Started");
    return true;
}

bool sendLoraMessage(String &payload) {
    if (!intlora) return false;
    loraInterruptEnabled = false;
    int state = RADIOLIB_ERR_NONE;
    if (loraRadioVariant == LoRaRadioVariant::SX1278 && lora1278) {
        state = lora1278->transmit(payload);
        lora1278->startReceive();
    } else if (loraRadioVariant == LoRaRadioVariant::SX1276 && lora1276) {
        state = lora1276->transmit(payload);
        lora1276->startReceive();
    } else if (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262) {
        state = lora1262->transmit(payload);
        lora1262->startReceive();
    } else {
        loraInterruptEnabled = true;
        return false;
    }
    loraInterruptEnabled = true;
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("LoRa transmit failed: %d\n", state);
        displayError("LoRa send failed");
        return false;
    }
    return true;
}

void reciveMessage() {
    if (!loraPacketReceived || !intlora) return;
    loraInterruptEnabled = false;
    loraPacketReceived = false;
    String incoming;
    int state = (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262) ? lora1262->readData(incoming)
                : (loraRadioVariant == LoRaRadioVariant::SX1278 && lora1278)
                    ? lora1278->readData(incoming)
                    : (lora1276 ? lora1276->readData(incoming) : -1);
    if (state == RADIOLIB_ERR_NONE) {
        rcvmsg = incoming;
        Serial.println("Recived:" + rcvmsg);
        File file = LittleFS.open("/chats.txt", "a");
        file.println(rcvmsg);
        file.close();
        messages.push_back(rcvmsg);
        if (messages.size() > maxMessages) { scrollOffset = messages.size() - maxMessages; }
        update = true;
    } else {
        Serial.printf("LoRa read failed: %d\n", state);
    }
    if (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262) {
        lora1262->startReceive();
    } else if (loraRadioVariant == LoRaRadioVariant::SX1278 && lora1278) {
        lora1278->startReceive();
    } else if (lora1276) {
        lora1276->startReceive();
    }
    loraInterruptEnabled = true;
}

// render stuff

void render() {
    if (!update) return;
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(0x6DFC);
    if (!intlora) { tft.drawString("Lora Init Failed", 10, 13); }
    Serial.println(String(displayName));
    tft.drawString("USRN: " + String(displayName), 10, 25);

    int yPos = yStart;
    int endLine = scrollOffset + maxMessages;
    if (endLine > messages.size()) endLine = messages.size();
    for (int i = scrollOffset; i < endLine; i++) {
        tft.setTextColor(bruceConfig.priColor);
        tft.drawString(messages[i], 10, yPos);
        yPos += ySpacing;
    }
    update = false;
}

void loadMessages() {
    messages.clear();
    File file = LittleFS.open("/chats.txt", "r");
    while (file.available()) {
        String line = file.readStringUntil('\n');
        messages.push_back(line);
    }
    file.close();
    if (messages.size() > maxMessages) {
        scrollOffset = messages.size() - maxMessages;
    } else {
        scrollOffset = 0;
    }
}

// optional call funcs
void sendmsg() {
    Serial.println("C bttn");
    tft.fillScreen(TFT_BLACK);
    if (!intlora) {
        tft.setTextColor(bruceConfig.priColor);

        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
        tft.setCursor(10, tftHeight / 2 - 10);
        tft.print("LoRa not init!");

        tft.drawCentreString("LoRa not initialized!", tftWidth / 2, tftHeight / 2, 2);
        delay(1500);
        update = true;
        return;
    }
    msg = keyboard(msg, 256, "Message:");
    msg = String(displayName) + ": " + msg;
    if (msg == "") return;
    Serial.println(msg);
    if (!sendLoraMessage(msg)) {
        update = true;
        return;
    }
    tft.fillScreen(TFT_BLACK);
    update = true;
    File file = LittleFS.open("/chats.txt", "a");
    file.println(msg);
    file.close();

    messages.push_back(msg);
    if (messages.size() > maxMessages) { scrollOffset = messages.size() - maxMessages; }
    msg = "";
}

void upress() {
    Serial.println("Up Pressed");
    if (scrollOffset > 0) {
        scrollOffset--;
        update = true;
    }
}

void downpress() {
    Serial.println("Down Pressed");
    if (scrollOffset < messages.size() - maxMessages) {
        scrollOffset++;
        update = true;
    }
}

void selectRadioVariant(JsonDocument &doc) {
    String stored = doc["LoRa_Radio"] | "SX1278";
    if (stored.equalsIgnoreCase("SX1262")) {
        loraRadioVariant = LoRaRadioVariant::SX1262;
    } else if (stored.equalsIgnoreCase("SX1276")) {
        loraRadioVariant = LoRaRadioVariant::SX1276;
    } else {
        loraRadioVariant = LoRaRadioVariant::SX1278;
    }
    std::vector<Option> radioOptions = {
        {"SX1278", []() {}},
        {"SX1276", []() {}},
        {"SX1262", []() {}}
    };
    int defaultIndex = 0;
    if (loraRadioVariant == LoRaRadioVariant::SX1276) defaultIndex = 1;
    if (loraRadioVariant == LoRaRadioVariant::SX1262) defaultIndex = 2;
    int selected = loopOptions(radioOptions, MENU_TYPE_SUBMENU, "LoRa Radio", defaultIndex);
    if (selected >= 0) {
        if (selected == 2) loraRadioVariant = LoRaRadioVariant::SX1262;
        else if (selected == 1) loraRadioVariant = LoRaRadioVariant::SX1276;
        else loraRadioVariant = LoRaRadioVariant::SX1278;
        doc["LoRa_Radio"] = (loraRadioVariant == LoRaRadioVariant::SX1262)   ? "SX1262"
                            : (loraRadioVariant == LoRaRadioVariant::SX1276) ? "SX1276"
                                                                             : "SX1278";
        File cfg = LittleFS.open("/lora_settings.json", "w");
        serializeJson(doc, cfg);
        cfg.close();
    }
}

void mainloop() {
    long pressStartTime = 0;
    bool isPressing = false;
    bool breakloop = false;
    while (true) {
        render();
        reciveMessage();
        if (breakloop) { break; }
#ifdef HAS_3_BUTTONS
        if (EscPress) {
            long _tmp = millis();

            LongPress = true;
            while (EscPress) {
                if (millis() - _tmp > 200) {
                    // start drawing arc after short delay; animate over 500ms
                    int sweep = 0;
                    long elapsed = millis() - (_tmp + 200);
                    if (elapsed > 0) sweep = 360 * elapsed / 500;
                    if (sweep > 360) sweep = 360;
                    tft.drawArc(
                        tftWidth / 2,
                        tftHeight / 2,
                        25,
                        15,
                        0,
                        sweep,
                        getColorVariation(bruceConfig.priColor),
                        bruceConfig.bgColor
                    );
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            // clear arc
            tft.drawArc(
                tftWidth / 2, tftHeight / 2, 25, 15, 0, 360, bruceConfig.bgColor, bruceConfig.bgColor
            );
            LongPress = false;
            // #endif

            // decide short vs long after release
            if (millis() - _tmp > 700) {
                // long press -> exit
                breakloop = true;
            } else {
                // short press -> scroll down; consume flag first
                check(EscPress);
                upress();
            }
        }

        if (check(NextPress)) downpress();
        if (check(SelPress)) sendmsg();
#else

        if (check(NextPress)) downpress();
        if (check(EscPress)) break;
        if (check(PrevPress)) upress();
        if (check(SelPress)) sendmsg();
#endif

        delay(20);
    }
}

void lorachat() {
    // Set Default LoRa params (Legacy)
    spreadingFactor = 9;
    signalBandwidth = 31.25E3;
    codingRateDenominator = 8;
    preambleLength = 8;

    // set filesystem thing
    if (!LittleFS.exists("/chats.txt")) {
        File file = LittleFS.open("/chats.txt", "w");
        file.close();
        Serial.println("chat file created :)");
    }
    if (!LittleFS.exists("/lora_settings.json")) {
        Serial.println("creating lora settings .json file");
        JsonDocument doc;
        File file = LittleFS.open("/lora_settings.json", "w");
        doc["LoRa_Frequency"] = "434500000.00";
        doc["LoRa_Name"] = "BruceTest";
        doc["LoRa_Radio"] = "SX1278";
        serializeJson(doc, file);
        file.close();
    }
    File file = LittleFS.open("/lora_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, file);
    displayName = doc["LoRa_Name"].as<String>();
    double BAND = doc["LoRa_Frequency"].as<String>().toDouble();
    file.close();
    selectRadioVariant(doc);
    float bandMHz = (BAND > 1000) ? BAND / 1000000.0f : BAND;
    if (bandMHz <= 0) {
        displayError("Invalid LoRa frequency", true);
        return;
    }
    tft.fillScreen(TFT_BLACK);
    update = true;
    Serial.println("Initializing LoRa...");
    Serial.println(
        "Pins: SCK:" + String(bruceConfigPins.LoRa_bus.sck) +
        " MISO:" + String(bruceConfigPins.LoRa_bus.miso) + " MOSI:" + String(bruceConfigPins.LoRa_bus.mosi) +
        " CS:" + String(bruceConfigPins.LoRa_bus.cs) + " RST:" + String(getLoraResetPin()) +
        " IRQ:" + String(getLoraIrqPin()) + "BAND: " + String(bandMHz) + "MHz Radio: " +
        ((loraRadioVariant == LoRaRadioVariant::SX1262)   ? "SX1262"
         : (loraRadioVariant == LoRaRadioVariant::SX1276) ? "SX1276"
                                                          : "SX1278") +
        " DisplayName:  " + displayName
    );

    if (!startLoraRadio(bandMHz)) {
        update = true;
        return;
    }
    tft.setTextWrap(true, true);
    tft.setTextDatum(TL_DATUM);

    if (!loadonlyradio) {
        loadMessages();
        mainloop();
    }
}

// settings
// check the saving and loading
void changeusername() {
    tft.fillScreen(TFT_BLACK);
    String username = keyboard(username, 64, "");
    if (username == "") return;
    File file = LittleFS.open("/lora_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, file);
    file.close();
    doc["LoRa_Name"] = username;
    file = LittleFS.open("/lora_settings.json", "w");
    serializeJson(doc, file);
    file.close();
}

void chfreq() {
    tft.fillScreen(TFT_BLACK);
    char buf[15];
    File file = LittleFS.open("/lora_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, file);
    file.close();

    double dfreq = doc["LoRa_Frequency"].as<String>().toDouble();
    dfreq = dfreq / 1000000;
    snprintf(buf, sizeof(buf), "%.3f", dfreq);
    String freq = num_keyboard(buf, 12, "in Mhz");
    dfreq = freq.toDouble();
    if (dfreq == 0) {
        displayError("Invalid value");
        return;
    } else if (dfreq > 1000) {
        displayError("Invalid value, Exceeds 1Ghz");
        return;
    }
    dfreq = dfreq * 1000000;
    snprintf(buf, sizeof(buf), "%.2f", dfreq);
    doc["LoRa_Frequency"] = buf;

    file = LittleFS.open("/lora_settings.json", "w");
    serializeJson(doc, file);
    file.close();
}

// jammer thingy

String generateRandomString(int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    String result = "";
    result.reserve(length);

    for (int i = 0; i < length; i++) {
        uint32_t randomNum = esp_random();
        result += charset[randomNum % (sizeof(charset) - 1)];
    }
    return result;
}

String jammyjelly = generateRandomString(250);

void jammyjammer() {
    loadonlyradio = true;
    lorachat();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(10, tftHeight / 2 - 10);
    tft.print("Jamming...");
    if (!intlora) {
        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
        tft.setCursor(10, tftHeight / 2 - 10);
        tft.print("LoRa not init!");
        delay(1500);
        update = true;
        loadonlyradio = false;
        return;
    }
    while (true) {
        sendLoraMessage(jammyjelly);
        delay(10);
        if (check(EscPress)) {
            long _tmp = millis();
            while (EscPress) {
                if (millis() - _tmp > 200) {
                    // start drawing arc after short delay; animate over 500ms
                    int sweep = 0;
                    long elapsed = millis() - (_tmp + 200);
                    if (elapsed > 0) sweep = 360 * elapsed / 500;
                    if (sweep > 360) sweep = 360;
                    tft.drawArc(
                        tftWidth / 2,
                        tftHeight / 2,
                        25,
                        15,
                        0,
                        sweep,
                        getColorVariation(bruceConfig.priColor),
                        bruceConfig.bgColor
                    );
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            // clear arc
            tft.drawArc(
                tftWidth / 2, tftHeight / 2, 25, 15, 0, 360, bruceConfig.bgColor, bruceConfig.bgColor
            );
            LongPress = false;
            loadonlyradio = false;
            break;
        }
    }
}
#endif

void sndfile() {
    // Select source filesystem
    std::vector<Option> fsOptions = {
        {"SD Card",  []() {}},
        {"LittleFS", []() {}}
    };

    int selectedObj = loopOptions(fsOptions, MENU_TYPE_SUBMENU, "Select Source");
    if (selectedObj == -1) return; // User pressed Back

    fs::FS *filesystem = nullptr;
    String filePath = "";

    if (selectedObj == 0) {
        // SD Card
        if (!setupSdCard()) {
            displayError("SD Init Failed");
            return;
        }
        filesystem = &SD;
        filePath = loopSD(SD, true);
    } else {
        // LittleFS
        filesystem = &LittleFS;
        filePath = loopSD(LittleFS, true);
    }

    if (filePath == "") {
        Serial.println("No file selected");
        displayError("No file selected");
        return;
    } else {
        if (filesystem->exists(filePath)) {
            File file = filesystem->open(filePath, "r");
            size_t totalSize = file.size();
            size_t bytesSent = 0;
            loadonlyradio = true;
            lorachat();
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(TFT_WHITE);
            tft.println("Sending file:");
            tft.println(filePath);

            int barX = 10;
            int barY = tftHeight / 2;
            int barWidth = tftWidth - 20;
            int barHeight = 20;

            tft.drawRect(barX, barY, barWidth, barHeight, TFT_WHITE);

            delay(1000);
            while (file.available()) {
                String chunk = "";
                for (int i = 0; i < 250 && file.available(); i++) { chunk += (char)file.read(); }
                String payload = chunk;
                if (!sendLoraMessage(payload)) {
                    displayError("Failed to send chunk!");
                    Serial.println("Failed to send chunk!");
                    loadonlyradio = false;
                    break;
                }

                // Update Progress
                bytesSent += chunk.length();
                float progress = (float)bytesSent / totalSize;
                int fillWidth = (int)(progress * (barWidth - 2));
                if (fillWidth > barWidth - 2) fillWidth = barWidth - 2;

                tft.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, bruceConfig.priColor);

                delay(10);
            }
            file.close();
            tft.setCursor(10, barY + barHeight + 5);
            tft.setTextColor(TFT_GREEN);
            tft.println("File send complete!");
            delay(2000);
            tft.fillScreen(TFT_BLACK);
            loadonlyradio = false;
        } else {
            Serial.println("File does not exist");
            displayError("File does not exist");
            loadonlyradio = false;
        }
    }
}
