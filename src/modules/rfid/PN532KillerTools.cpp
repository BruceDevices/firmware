#ifndef LITE_VERSION
#include "PN532KillerTools.h"
#include "PN532Killer.h"
#include "apdu.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/sd_functions.h"
#include "driver/uart.h"
#include "globals.h"
#include "hal/gpio_hal.h"
#include "modules/others/audio.h"
#include "soc/gpio_reg.h"
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#if defined(ARDUINO_M5STICK_C_PLUS) || defined(ARDUINO_M5STICK_C_PLUS2)

#define TXD_PIN (GPIO_NUM_25)
#define RXD_PIN (GPIO_NUM_26)
#else
#define TXD_PIN SERIAL_TX
#define RXD_PIN SERIAL_RX
#endif
#define UART_BAUD_RATE 115200

#define UDP_REMOTE_TIMEOUT_MS 60000UL
#define TCP_REMOTE_TIMEOUT_MS 60000UL

extern BLEServer *pServer;
extern BLEService *pService;
extern BLECharacteristic *pTxCharacteristic;
extern BLECharacteristic *pRxCharacteristic;
extern bool bleDataTransferEnabled;
#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
#endif
PN532KillerTools::PN532KillerTools() { setup(); }

PN532KillerTools::~PN532KillerTools() {
    _pn532Killer.close();
    disableBleDataTransfer();
    if (bleDataTransferEnabled) { disableBleDataTransfer(); }
}

void PN532KillerTools::setup() {
    // Reset Pin states
    if (bruceConfigPins.SDCARD_bus.checkConflict(RXD_PIN) ||
        bruceConfigPins.SDCARD_bus.checkConflict(TXD_PIN)) {
        sdcardSPI.end();
    }
    if (bruceConfigPins.CC1101_bus.checkConflict(RXD_PIN) ||
        bruceConfigPins.CC1101_bus.checkConflict(TXD_PIN) ||
        bruceConfigPins.NRF24_bus.checkConflict(RXD_PIN) ||
        bruceConfigPins.NRF24_bus.checkConflict(TXD_PIN)) {
        CC_NRF_SPI.end();
    }
    pinMode(RXD_PIN, INPUT);
    pinMode(TXD_PIN, OUTPUT);
    Serial1.begin(UART_BAUD_RATE, SERIAL_8N1, RXD_PIN, TXD_PIN);

    // Display initial screen and prompt user to press OK to check device
    displayInitialScreen();

    return loop();
}

void PN532KillerTools::displayBanner() {
    drawMainBorderWithTitle(_titleName.c_str());
    delay(200);
}

void PN532KillerTools::displayInitialScreen() {
    _titleName = "PN532 UART";
    drawMainBorderWithTitle(_titleName.c_str());

    tft.setTextSize(FP);
    int margin = tftWidth / 16;
    if (margin < 6) margin = 6;
    if (margin > 24) margin = 24;
    int baseY = tftHeight / 2;

    String line1 = "Connect to PN532/PN532Killer";
    String line2 = "via UART port.";
    String line3 = "Press OK to check device type.";

    int leftX = (tftWidth - line1.length() * 6 * FP) / 2;
    tft.setCursor(leftX, baseY);
    tft.println(line1);
    tft.setCursor(leftX, baseY + FP * 12);
    tft.println(line2);
    tft.setCursor(leftX, baseY + FP * 24);
    tft.println(line3);

    delay(200);
}

void PN532KillerTools::playDeviceDetectedSound() {
    if (bruceConfig.soundEnabled == 0) return; // if sound is disabled, do not play sound

#if !defined(LITE_VERSION)
#if defined(BUZZ_PIN)
    // Play two beeps to indicate successful device detection
    _tone(5000, 50);
    delay(100);
    _tone(5000, 50);
#elif defined(HAS_NS4168_SPKR)
    // Try to play a detection sound file, fallback to startup sound if not available
    if (SD.exists("/device_detected.wav")) {
        playAudioFile(&SD, "/device_detected.wav");
    } else if (LittleFS.exists("/device_detected.wav")) {
        playAudioFile(&LittleFS, "/device_detected.wav");
    } else {
        // Fallback to startup sound logic
        if (bruceConfig.theme.boot_sound) {
            playAudioFile(
                bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.boot_sound)
            );
        } else if (SD.exists("/boot.wav")) {
            playAudioFile(&SD, "/boot.wav");
        } else if (LittleFS.exists("/boot.wav")) {
            playAudioFile(&LittleFS, "/boot.wav");
        }
    }
#endif
#endif
}

void PN532KillerTools::playUidFoundSound() {
    if (bruceConfig.soundEnabled == 0) return; // if sound is disabled, do not play sound

#if !defined(LITE_VERSION)
#if defined(BUZZ_PIN)
    // Play a single higher tone to indicate UID found
    _tone(6000, 200);
    _tone(9000, 200);
    _tone(12000, 300);
#elif defined(HAS_NS4168_SPKR)
    // Try to play a UID found sound file, fallback to tone simulation
    if (SD.exists("/uid_found.wav")) {
        playAudioFile(&SD, "/uid_found.wav");
    } else if (LittleFS.exists("/uid_found.wav")) {
        playAudioFile(&LittleFS, "/uid_found.wav");
    } else {
        // No specific sound file, play a simple tone pattern
        playTone(800, 100);
        delay(50);
        playTone(1000, 100);
    }
#endif
#endif
}

void PN532KillerTools::resetDevice(bool showInitialScreen) {
    _deviceInitialized = false;
    _pn532Killer.close();

    if (_udpEnabled) disableUdpDataTransfer();
    if (_tcpEnabled) disableTcpDataTransfer();
    if (bleDataTransferEnabled) disableBleDataTransfer();

    Serial1.end();
    delay(100);
    Serial1.flush();
    delay(100);
    Serial1.begin(UART_BAUD_RATE, SERIAL_8N1, RXD_PIN, TXD_PIN);
    delay(100);
    if (showInitialScreen) { displayInitialScreen(); }
}

void PN532KillerTools::hardwareProbe() {
    Serial.println("Attempting PN532 communication...");
    bool ok = _pn532Killer.setNormalMode();
    if (!ok) {
        _titleName = "PN532 UART";
        _isPn532killer = false;
        _initializationFailed = true;
        displayBanner();
        printCenterFootnote("Check PN532/PN532Killer Connection");
        displayError("Wake Failed");
        return;
    }

    _isPn532killer = _pn532Killer.isPn532killer();
    _initializationFailed = false;
    if (_isPn532killer) {
        _titleName = "PN532Killer";
        Serial.println("Detected PN532Killer device");
    } else {
        _titleName = "PN532 UART";
        Serial.println("Detected standard PN532 device");
    }
    setReaderMode();
}

void PN532KillerTools::loop() {
    while (1) {
        if (check(EscPress)) {
            if (_udpEnabled) disableUdpDataTransfer();
            if (_tcpEnabled) disableTcpDataTransfer();
            return;
        }

        // If device not initialized, wait for user to press OK for hardware detection
        if (!_deviceInitialized) {
            if (check(SelPress)) {
                if (_initializationFailed) {
                    failedInitMenu();
                } else {
                    displayInfo("Checking device");
                    hardwareProbe();
                    if (!_initializationFailed) {
                        _deviceInitialized = true;
                        playDeviceDetectedSound();
                    }
                }
            }
            delay(50); // Small delay to avoid excessive CPU usage
            continue;
        }

        if (_workMode == PN532KillerCmd::WorkMode::Reader) {
            if (check(NextPress)) { readTagUid(); }
        } else if (_workMode == PN532KillerCmd::WorkMode::Emulator) {
            if (check(NextPress)) { setEmulatorNextSlot(false, false); }
            if (check(PrevPress)) { setEmulatorNextSlot(true, false); }
        } else if (_workMode == PN532KillerCmd::WorkMode::Sniffer) {
            if (check(NextPress) && _snifferType == PN532KillerCmd::SnifferType::MFKey32v2) {
                setSnifferUid();
            }
        }

        if (check(SelPress)) { mainMenu(); }

        if (_udpEnabled) {
            int packetSize = _udp.parsePacket();
            if (packetSize) {
                if (!_udpHasRemote) {
                    _udpRemoteIP = _udp.remoteIP();
                    _udpRemotePort = _udp.remotePort();
                    _udpHasRemote = true;
                    _udpLastPacketMs = millis();
                    printCenterFootnote(String("Remote: ") + _udpRemoteIP.toString());
                } else {
                    _udpLastPacketMs = millis();
                }
                uint8_t inbuf[256];
                int len = _udp.read(inbuf, sizeof(inbuf));
                if (len > 0) { sendUdpToPn532(inbuf, len); }
            } else if (_udpHasRemote) {
                if (millis() - _udpLastPacketMs > UDP_REMOTE_TIMEOUT_MS) {
                    _udpHasRemote = false;
                    printCenterFootnote("Waiting for UDP client...");
                }
            }
        }
        if (_tcpEnabled) {
            if (!_tcpHasClient) {
                WiFiClient newClient = _tcpServer.accept();
                if (newClient) {
                    _tcpClient.stop();
                    _tcpClient = newClient;
                    _tcpHasClient = true;
                    _tcpLastPacketMs = millis();
                    printCenterFootnote(String("TCP:") + _tcpClient.remoteIP().toString());
                    Serial.println("TCP Client connected");
                }
            } else if (!_tcpClient.connected()) {
                _tcpClient.stop();
                _tcpHasClient = false;
                printCenterFootnote("Waiting TCP client...");
                Serial.println("TCP Client disconnected");
            } else {
                while (_tcpClient.connected() && _tcpClient.available()) {
                    uint8_t buf[256];
                    int r = _tcpClient.read(buf, sizeof(buf));
                    if (r > 0) {
                        sendTcpToPn532(buf, r);
                        _tcpLastPacketMs = millis();
                    }
                }
                if (_tcpHasClient && millis() - _tcpLastPacketMs > TCP_REMOTE_TIMEOUT_MS) {
                    _tcpClient.stop();
                    _tcpHasClient = false;
                    printCenterFootnote("Waiting TCP client...");
                }
            }
        }

        if (bleDataTransferEnabled) {
            std::vector<uint8_t> bleDataBuffer;
            while (Serial1.available()) {
                uint8_t data = Serial1.read();
                bleDataBuffer.push_back(data);
            }
            if (!bleDataBuffer.empty()) {
                pTxCharacteristic->setValue(bleDataBuffer.data(), bleDataBuffer.size());
                pTxCharacteristic->notify();
                if (_udpEnabled && _udpHasRemote) {
                    _udp.beginPacket(_udpRemoteIP, _udpRemotePort);
                    _udp.write(bleDataBuffer.data(), bleDataBuffer.size());
                    _udp.endPacket();
                    _udpLastPacketMs = millis();
                }
                Serial.print("BLE < ");
                for (size_t i = 0; i < bleDataBuffer.size(); i++) {
                    if (bleDataBuffer[i] < 0x10) Serial.print("0");
                    Serial.print(bleDataBuffer[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
            }
        }
        if (_udpEnabled || _tcpEnabled) { drainUartToUdp(true); }

        if (returnToMenu) {
            if (_udpEnabled) disableUdpDataTransfer();
            returnToMenu = false;
            break;
        }
    }
}

void PN532KillerTools::sendUdpToPn532(const uint8_t *data, int len) {
    if (len <= 0) return;

    Serial1.write(data, len);
    _udpLastPacketMs = millis();

    Serial.print("UDP > ");
    for (int i = 0; i < len; ++i) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void PN532KillerTools::sendTcpToPn532(const uint8_t *data, int len) {
    if (len <= 0) return;
    Serial1.write(data, len);
    _tcpLastPacketMs = millis();
    Serial.print("TCP > ");
    for (int i = 0; i < len; ++i) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void PN532KillerTools::drainUartToUdp(bool log) {
    bool anyNet = (_udpEnabled && _udpHasRemote) || (_tcpEnabled && _tcpHasClient);
    if (!anyNet) return;
    if (!Serial1.available()) return;
    uint8_t buf[256];
    size_t idx = 0;
    uint32_t gapStart = millis();
    const uint32_t GAP_MS = 8;
    while (millis() - gapStart < GAP_MS && idx < sizeof(buf)) {
        while (Serial1.available() && idx < sizeof(buf)) {
            buf[idx++] = Serial1.read();
            gapStart = millis();
        }
    }
    if (!idx) return;
    if (_udpEnabled && _udpHasRemote) {
        _udp.beginPacket(_udpRemoteIP, _udpRemotePort);
        _udp.write(buf, idx);
        _udp.endPacket();
        _udpLastPacketMs = millis();
    }
    if (_tcpEnabled && _tcpHasClient && _tcpClient.connected()) {
        _tcpClient.write(buf, idx);
        _tcpLastPacketMs = millis();
    }
    if (log) {
        Serial.print("NET < ");
        for (size_t i = 0; i < idx; ++i) {
            if (buf[i] < 0x10) Serial.print("0");
            Serial.print(buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}

void PN532KillerTools::mainMenu() {
    options = {
        {"Reader", [&]() { readerMenu(); }}
    };
    if (_isPn532killer) {
        options.push_back({"Emulator", [&]() { emulatorMenu(); }});
        options.push_back({"Sniffer", [&]() { snifferMenu(); }});
    }

    String netLabel = "Net";
    if (bleDataTransferEnabled || _udpEnabled || _tcpEnabled) {
        netLabel += "(";
        bool first = true;
        if (bleDataTransferEnabled) {
            netLabel += "BLE";
            first = false;
        }
        if (_tcpEnabled) {
            if (!first) netLabel += "+";
            netLabel += "TCP";
            first = false;
        }
        if (_udpEnabled) {
            if (!first) netLabel += "+";
            netLabel += "UDP";
        }
        netLabel += ")";
    } else {
        netLabel = "BLE/TCP/UDP";
    }
    options.push_back({netLabel.c_str(), [&]() { netMenu(); }});
    // check device
    options.push_back({"Reset", [&]() { resetDevice(); }});
    options.push_back({"Return", [&]() { returnToMenu = true; }});
    loopOptions(options);
}

void PN532KillerTools::failedInitMenu() {
    options = {
        {"Reader", [&]() { readerMenu(); }}
    };
    if (_isPn532killer) {
        options.push_back({"Emulator", [&]() { emulatorMenu(); }});
        options.push_back({"Sniffer", [&]() { snifferMenu(); }});
    }

    String netLabel = "Net";
    if (bleDataTransferEnabled || _udpEnabled || _tcpEnabled) {
        netLabel += "(";
        bool first = true;
        if (bleDataTransferEnabled) {
            netLabel += "BLE";
            first = false;
        }
        if (_tcpEnabled) {
            if (!first) netLabel += "+";
            netLabel += "TCP";
            first = false;
        }
        if (_udpEnabled) {
            if (!first) netLabel += "+";
            netLabel += "UDP";
        }
        netLabel += ")";
    } else {
        netLabel = "BLE/TCP/UDP";
    }
    options.push_back({netLabel.c_str(), [&]() { netMenu(); }});
    options.push_back({"Reset", [&]() {
                           resetDevice(false);
                           displayInfo("Checking device");
                           hardwareProbe();
                           if (!_initializationFailed) {
                               _deviceInitialized = true;
                               playDeviceDetectedSound();
                           }
                       }});
    options.push_back({"Return", [&]() { returnToMenu = true; }});

    // 默认选择Reset项（倒数第二个选项）
    loopOptions(options, options.size() - 2);
}

void PN532KillerTools::netMenu() {
    std::vector<Option> netOptions;
    netOptions.push_back({bleDataTransferEnabled ? "BLE:ON" : "BLE:OFF", [&]() {
                              if (bleDataTransferEnabled) disableBleDataTransfer();
                              else enableBleDataTransfer();
                          }});
    netOptions.push_back({_udpEnabled ? "UDP:ON" : "UDP:OFF", [&]() {
                              if (_udpEnabled) disableUdpDataTransfer();
                              else {
                                  if (WiFi.isConnected() || WiFi.getMode() == WIFI_AP ||
                                      WiFi.getMode() == WIFI_AP_STA)
                                      enableUdpDataTransfer();
                                  else udpWifiSelectMenu();
                              }
                          }});
    netOptions.push_back({_tcpEnabled ? "TCP:ON" : "TCP:OFF", [&]() {
                              if (_tcpEnabled) disableTcpDataTransfer();
                              else {
                                  if (WiFi.isConnected() || WiFi.getMode() == WIFI_AP ||
                                      WiFi.getMode() == WIFI_AP_STA)
                                      enableTcpDataTransfer();
                                  else udpWifiSelectMenu();
                              }
                          }});
    netOptions.push_back({"Return", [&]() { mainMenu(); }});
    loopOptions(netOptions);
}
void PN532KillerTools::readerMenu() {
    options = {
        {"Scan UID", [&]() { readTagUid(); }   },
        {"Return",   [&]() { setReaderMode(); }}
    };

    loopOptions(options);
}

void PN532KillerTools::emulatorMenu() {
    _pn532Killer.tagIndex = -1;
    options = {
        {"MFC1K",
         [&]() {
             _tagType = PN532KillerCmd::TagType::MFC1K;
             setEmulatorNextSlot(false, true);
         }                            },
        {"NTAG",
         [&]() {
             _tagType = PN532KillerCmd::TagType::NTAG;
             setEmulatorNextSlot(false, true);
         }                            },
        {"ISO15693",
         [&]() {
             _tagType = PN532KillerCmd::TagType::ISO15693;
             setEmulatorNextSlot(false, true);
         }                            },
        {"Return",   [&]() { return; }}
    };

    loopOptions(options);
}

void PN532KillerTools::snifferMenu() {
    options = {
        {"MFKey32v2",
         [&]() {
             _snifferType = PN532KillerCmd::SnifferType::MFKey32v2;
             setSnifferMode();
         }                             },
        {"MFKey64",
         [&]() {
             _snifferType = PN532KillerCmd::SnifferType::MFKey64;
             setSnifferMode();
         }                             },
        {"Return",    [&]() { return; }}
    };

    loopOptions(options);
}

void PN532KillerTools::setSnifferMode() {
    _workMode = PN532KillerCmd::WorkMode::Sniffer;
    displayBanner();
    printSubtitle("Sniffer Mode");
    _pn532Killer.setSnifferMode(_snifferType);
    String tagType = "MFC 1K";
    String snifferType = "";
    if (_snifferType == PN532KillerCmd::SnifferType::MFKey32v2) {
        drawMfkey32Icon(tftWidth / 4 - 40, (tftHeight) / 2 - 5);
        snifferType = "MFKey32v2";
        printCenterFootnote("Press Next to set UID");
    } else {
        drawMfkey64Icon(tftWidth / 4 - 40, (tftHeight) / 2 - 5);
        snifferType = "MFKey64";
    }
    tft.setTextSize(FM);
    tft.setCursor(tftWidth / 2 - 20, tftHeight / 2 + 5);
    tft.print(tagType);
    tft.setCursor(tftWidth / 2 - 20, tftHeight / 2 + FM * 10 + 5);
    tft.print(snifferType);
}

void PN532KillerTools::setSnifferUid() {
    displayInfo("Scanning UID...");
    _pn532Killer.setNormalMode();
    TagTechnology::Iso14aTagInfo hf14aTagInfo = _pn532Killer.hf14aScan();
    if (!hf14aTagInfo.uid.empty()) {
        displaySuccess(String(hf14aTagInfo.uid_hex.c_str()));
        delay(100);
        _pn532Killer.setSnifferUid(hf14aTagInfo.uid_hex);
        setSnifferMode();
        return;
    }
    displayError("No tag found");
}

void PN532KillerTools::setReaderMode() {
    displayBanner();
    printSubtitle("Reader Mode");
    // Regular PN532 does not display ISO15693 text (if not supported)
    drawCreditCard(tftWidth / 4 - 40, (tftHeight) / 2 - 10);
    tft.setTextSize(FM);
    tft.setCursor(tftWidth / 2 - 20, tftHeight / 2);
    tft.print("ISO14443");
    if (_isPn532killer) { // Only enhanced version shows ISO15693 hint
        tft.setCursor(tftWidth / 2 - 20, tftHeight / 2 + FM * 10);
        tft.print("ISO15693");
    }
    printCenterFootnote("Press OK to select mode");
    _pn532Killer.setNormalMode();
}

void PN532KillerTools::readTagUid() {
    _workMode = PN532KillerCmd::WorkMode::Reader;
    _pn532Killer.setNormalMode();
    displayBanner();
    printSubtitle("UID Reader");
    printCenterFootnote("Scanning ISO14443...");
    auto hf14aTagInfo = _pn532Killer.hf14aScan();
    bool tagFound = false;
    if (!hf14aTagInfo.uid.empty()) {
        printUid("ISO14443", hf14aTagInfo.uid_hex.c_str());
        tagFound = true;
        playUidFoundSound(); // Play sound when UID is found
    }
    if (_isPn532killer && !tagFound) {
        printCenterFootnote("Scanning ISO15693...");
        auto hf15TagInfo = _pn532Killer.hf15Scan();
        if (!hf15TagInfo.uid.empty()) {
            printUid("ISO15693", hf15TagInfo.uid_hex.c_str());
            tagFound = true;
            playUidFoundSound(); // Play sound when UID is found
        }
    }
    if (tagFound) {
        printCenterFootnote("Press Next/Down to scan again");
        return;
    }
    displayError("No tag found");
}

void PN532KillerTools::printUid(const char *protocol, const char *uid) {
    tft.setTextSize(FM);
    tft.setCursor((tftWidth - strlen(protocol) * 6 * FM) / 2, tftHeight / 2);
    tft.println(protocol);
    tft.setCursor((tftWidth - strlen(uid) * 6 * FM) / 2, tft.getCursorY() + 2);
    tft.println(uid);
    tft.setTextSize(FP);
}

void PN532KillerTools::setEmulatorNextSlot(bool reverse, bool redrawTypeName) {
    _workMode = PN532KillerCmd::WorkMode::Emulator;
    if (reverse) {
        if (_pn532Killer.tagIndex <= 0) {
            _pn532Killer.tagIndex = 7;
        } else {
            _pn532Killer.tagIndex--;
        }
    } else {
        if (_pn532Killer.tagIndex >= 7) {
            _pn532Killer.tagIndex = 0;
        } else {
            _pn532Killer.tagIndex++;
        }
    }
    if (_tagType == PN532KillerCmd::TagType::MFC1K) {
        _pn532Killer.switchEmulatorMifareSlot(_pn532Killer.tagIndex);
    } else if (_tagType == PN532KillerCmd::TagType::NTAG) {
        _pn532Killer.switchEmulatorNtagSlot(_pn532Killer.tagIndex);
    } else if (_tagType == PN532KillerCmd::TagType::ISO15693) {
        _pn532Killer.switchEmulatorIso15693Slot(_pn532Killer.tagIndex);
    } else if (_tagType == PN532KillerCmd::TagType::EM4100) {
        _pn532Killer.switchEmulatorEm4100Slot(_pn532Killer.tagIndex);
    }

    if (redrawTypeName) {
        displayBanner();
        printSubtitle("Emulator Mode");
        drawCreditCard(tftWidth / 4 - 40, (tftHeight) / 2 - 5);
        tft.setTextSize(FM);
        String typeName;
        switch (_tagType) {
            case PN532KillerCmd::TagType::MFC1K: typeName = "MFC 1K"; break;
            case PN532KillerCmd::TagType::NTAG: typeName = "NTAG"; break;
            case PN532KillerCmd::TagType::ISO15693: typeName = "ISO15693"; break;
            case PN532KillerCmd::TagType::EM4100: typeName = "EM4100"; break;
            default: typeName = "Unknown"; break;
        }
        tft.setCursor(tftWidth / 2 - 20, tftHeight / 2 + 5);
        tft.print(typeName);
    }

    String slotText = String(_pn532Killer.tagIndex + 1) + "/8";
    int slotLabelX = tftWidth / 2 - 20;
    int slotLabelY = tftHeight / 2 + FM * 10 + 5;
    tft.setTextSize(FM);
    tft.setCursor(slotLabelX, slotLabelY);
    tft.print("Slot: ");
    int slotTextX = tft.getCursorX();
    int slotTextY = slotLabelY;

    tft.fillRect(slotTextX, slotTextY, 40, FM * 10, TFT_BLACK);
    tft.setCursor(slotTextX, slotTextY);
    tft.print(slotText);
}

class RxCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
        std::string value = pCharacteristic->getValue();
        if (!value.empty()) { Serial1.write((uint8_t *)value.data(), value.length()); }
        Serial.print("BLE > ");
        for (size_t i = 0; i < value.length(); i++) {
            if ((uint8_t)value[i] < 0x10) { Serial.print("0"); }
            Serial.print((uint8_t)value[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        if (!BLEConnected) {
            BLEConnected = true;
            drawStatusBar();
        }
    }
};

class PN532ServerCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer *pServer, NimBLEConnInfo &connInfo) override {
        BLEConnected = true;
        drawStatusBar();
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
        BLEConnected = false;
        drawStatusBar();
        // Restart advertising after disconnection
        pServer->getAdvertising()->start();
    }
};

bool PN532KillerTools::enableBleDataTransfer() {
    if (bleDataTransferEnabled) return true;

    BLEDevice::init("BRUCE-PN532-BLE");
    pServer = BLEDevice::createServer();
    if (!pServer) {
        displayError("BLE Server Fail");
        return false;
    }

    // Set server callbacks to handle connection/disconnection
    pServer->setCallbacks(new PN532ServerCallbacks());

    pService = pServer->createService("0000fff0-0000-1000-8000-00805f9b34fb");
    if (!pService) {
        displayError("BLE Service Fail");
        return false;
    }

    pTxCharacteristic = pService->createCharacteristic(
        "0000fff1-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    if (!pTxCharacteristic) {
        displayError("BLE TX Fail");
        return false;
    }

    pRxCharacteristic = pService->createCharacteristic(
        "0000fff2-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    if (!pRxCharacteristic) {
        displayError("BLE RX Fail");
        return false;
    }

    // Log and forward any writes from client to UART
    pRxCharacteristic->setCallbacks(new RxCharacteristicCallbacks());

    // Optional: also set callback on TX to detect client writing wrong characteristic
    pTxCharacteristic->setCallbacks(new RxCharacteristicCallbacks());

    pService->start();

    // Configure advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();

    bleDataTransferEnabled = true;
    displayInfo("BLE Enabled");
    delay(100);
    return true;
}

bool PN532KillerTools::disableBleDataTransfer() {
    if (!bleDataTransferEnabled) return true;

    if (pServer) {
        pServer->getAdvertising()->stop();
#if defined(CONFIG_IDF_TARGET_ESP32C5)
        esp_bt_controller_deinit();
#else
        BLEDevice::deinit();
#endif
    }

    pServer = nullptr;
    pService = nullptr;
    pTxCharacteristic = nullptr;
    pRxCharacteristic = nullptr;

    bleDataTransferEnabled = false;
    BLEConnected = false;
    displayInfo("BLE Disabled");
    delay(100);
    return true;
}

bool PN532KillerTools::enableUdpDataTransfer() {
    if (_udpEnabled) return true;
    // Ensure WiFi active
    if (!(WiFi.isConnected() || WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)) {
        displayError("No WiFi");
        return false;
    }
    if (!_udp.begin(18888)) {
        displayError("UDP Fail");
        return false;
    }
    _udpEnabled = true;
    _udpHasRemote = false; // wait for first packet to learn remote
    _udpLastPacketMs = millis();

    // UI display
    displayBanner();
    printSubtitle("UDP Reader Mode");

    IPAddress ip;
    if (WiFi.isConnected()) {
        ip = WiFi.localIP();
    } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        ip = WiFi.softAPIP();
    } else {
        ip = IPAddress(0, 0, 0, 0);
    }
    String ipLine = String("UDP:") + ip.toString();
    String portLine = String("Port: 18888");

    tft.setTextSize(FM);
    int margin = tftWidth / 16;
    if (margin < 6) margin = 6;
    if (margin > 24) margin = 24;
    int blockW = tftWidth - margin * 2;
    int baseY = tftHeight / 2 - 10;
    tft.fillRect(margin - 4, baseY - 4, blockW + 8, FM * 24 + 8, TFT_BLACK);
    tft.setCursor(margin, baseY);
    tft.print(ipLine);
    tft.setCursor(margin, baseY + FM * 12);
    tft.print(portLine);
    printCenterFootnote("Waiting for UDP client...");

    delay(150);
    return true;
}

bool PN532KillerTools::disableUdpDataTransfer() {
    if (!_udpEnabled) return true;
    _udp.stop();
    _udpEnabled = false;
    _udpHasRemote = false;
    displayInfo("UDP Off");
    delay(100);
    return true;
}

bool PN532KillerTools::enableTcpDataTransfer() {
    if (_tcpEnabled) return true;
    if (!(WiFi.isConnected() || WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)) {
        displayError("No WiFi");
        return false;
    }
    _tcpServer.begin();
    _tcpServer.setNoDelay(true);
    _tcpEnabled = true;
    _tcpHasClient = false;
    _tcpLastPacketMs = millis();

    displayBanner();
    printSubtitle("TCP Reader Mode");
    IPAddress ip = WiFi.isConnected() ? WiFi.localIP() : WiFi.softAPIP();
    tft.setTextSize(FM);
    int margin = tftWidth / 16;
    if (margin < 6) margin = 6;
    if (margin > 24) margin = 24;
    int baseY = tftHeight / 2 - 10;
    tft.fillRect(margin - 4, baseY - 4, tftWidth - margin * 2 + 8, FM * 24 + 8, TFT_BLACK);
    tft.setCursor(margin, baseY);
    tft.print(String("TCP:") + ip.toString());
    tft.setCursor(margin, baseY + FM * 12);
    tft.print("Port: 18889");
    printCenterFootnote("Waiting TCP client...");
    delay(150);
    return true;
}

bool PN532KillerTools::disableTcpDataTransfer() {
    if (!_tcpEnabled) return true;
    if (_tcpClient) _tcpClient.stop();
    _tcpServer.stop();
    _tcpEnabled = false;
    _tcpHasClient = false;
    displayInfo("TCP Off");
    delay(100);
    return true;
}

void PN532KillerTools::udpWifiSelectMenu() {
    if (WiFi.isConnected() || WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        enableUdpDataTransfer();
        return;
    }
    displayBanner();
    printSubtitle("UDP Network");
    printCenterFootnote("Select WiFi mode");
    std::vector<Option> selOptions;
    selOptions.push_back({"My Network", [&]() {
                              displayInfo("Connecting...");
                              uint32_t t = millis();
                              while (!WiFi.isConnected() && millis() - t < 5000) delay(100);
                              if (!WiFi.isConnected()) {
                                  displayError("Fail WiFi");
                                  return;
                              }
                              enableUdpDataTransfer();
                          }});
    selOptions.push_back({"AP Mode", [&]() {
                              displayInfo("Starting AP...");
                              WiFi.mode(WIFI_AP);
                              WiFi.softAP("BRUCE-UDP", "", 6);
                              delay(200);
                              enableUdpDataTransfer();
                          }});
    selOptions.push_back({"Return", [&]() { return; }});
    loopOptions(selOptions);
}
#endif
