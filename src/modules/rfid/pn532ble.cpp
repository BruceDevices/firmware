#ifndef LITE_VERSION
#include "pn532ble.h"
#include "apdu.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"

Pn532ble::Pn532ble() { setup(); }

Pn532ble::~Pn532ble() {}

void Pn532ble::setup() {
    displayBanner();

    if (connect()) showDeviceInfo();

    delay(500);
    return loop();
}

bool Pn532ble::connect() {
    displayInfo("Turn on PN532 BLE");
    delay(500);
    displayBanner();
    padprintln("");
    displayInfo("Searching...");

    if (!pn532_ble.searchForDevice()) {
        displayError("Not found");
        delay(1000);
        return false;
    }

    if (!pn532_ble.connectToDevice()) {
        displayError("Connect failed");
        delay(1000);
        return false;
    }
    displaySuccess("Connected");
    delay(800);

    return true;
}

void Pn532ble::loop() {
    while (1) {
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }

        if (check(SelPress)) { selectMode(); }

#ifdef HAS_KEYBOARD
        if (pn532_ble.isConnected()) {
            if (checkLetterShortcutPress() == 'h') { setMode(HF_14A_SCAN_MODE); }

            if (checkLetterShortcutPress() == 'v') { setMode(GET_FW_MODE); }

            if (checkLetterShortcutPress() == 'c') { setMode(HF_MF_READ_MODE); }

            if (checkLetterShortcutPress() == 'u') { setMode(HF_MFU_READ_MODE); }
            if (checkLetterShortcutPress() == 'e') { setMode(HF_TG_INIT_AS_TARGET_MODE); }
        }

        if (pn532_ble.isPN532Killer()) {
            if (checkLetterShortcutPress() == 'H') { setMode(HF_15_SCAN_MODE); }

            if (checkLetterShortcutPress() == 'l') { setMode(LF_EM4100_SCAN_MODE); }
            if (checkLetterShortcutPress() == 'i') { setMode(HF_ISO15693_READ_MODE); }
        }
#endif
    }
}

void Pn532ble::selectMode() {
    options = {};
    if (pn532_ble.isConnected()) {
        options.push_back({"Scan Tag", [this]() { scanTagMenu(); }});
        options.push_back({"Read Tag", [this]() { readTagMenu(); }});
        options.push_back({"Emulate Tag", [this]() { loadNdefEmulateMenu(); }});
        if (mfd.size() > 0 || mfud.size() > 0 || iso15dump.size() > 0) {
            options.push_back({"Write Dump", [this]() { writeDumpMenu(); }});
            options.push_back({"Save Dump", [this]() { saveDumpMenu(); }});
        };
    }
    options.push_back({"Load Dump", [this]() { loadDumpMenu(); }});
    options.push_back({"Back", [this]() { setMode(STANDBY_MODE); }});

    loopOptions(options);
}

void Pn532ble::scanTagMenu() {
    options = {
        {"Scan ISO14443A", [this]() { setMode(HF_14A_SCAN_MODE); }},
    };

    if (pn532_ble.isPN532Killer()) {
        options.push_back({"Scan ISO15693", [this]() { setMode(HF_15_SCAN_MODE); }});
        options.push_back({"Scan EM4100", [this]() { setMode(LF_EM4100_SCAN_MODE); }});
    }

    options.push_back({"Back", [this]() { selectMode(); }});

    loopOptions(options);
}

void Pn532ble::readTagMenu() {
    options = {
        {"Read MFC", [this]() { setMode(HF_MF_READ_MODE); } },
        {"Read MFU", [this]() { setMode(HF_MFU_READ_MODE); }},
    };

    if (pn532_ble.isPN532Killer()) {
        options.push_back({"Read ISO15693", [this]() { setMode(HF_ISO15693_READ_MODE); }});
        options.push_back({"Read EM4100", [this]() { setMode(LF_EM4100_SCAN_MODE); }});
    }

    options.push_back({"Back", [this]() { selectMode(); }});

    loopOptions(options);
}

void Pn532ble::writeDumpMenu() {
    options = {};

    if (mfd.size() > 0) {
        options.push_back({"Write MFC", [this]() { setMode(HF_MF_WRITE_MODE); }});
    }

    if (mfud.size() > 0) {
        options.push_back({"Write MFU", [this]() { setMode(HF_MFU_WRITE_MODE); }});
    }

    if (pn532_ble.isPN532Killer() && iso15dump.size() > 0) {
        options.push_back({"Write ISO15693", [this]() { setMode(HF_ISO15693_WRITE_MODE); }});
    }

    options.push_back({"Back", [this]() { selectMode(); }});

    loopOptions(options);
}

void Pn532ble::saveDumpMenu() {
    options = {};
    if (mfd.size() == 320 || mfd.size() == 1024 || mfd.size() == 4096) {
        options.push_back({"Save MFC dump", [this]() {
                               String fileName =
                                   saveHfDumpBinFile(mfd, pn532_ble.hf14aTagInfo.uid_hex, "mf-");
                               if (fileName != "") {
                                   displaySuccess("Saved to " + fileName);
                               } else {
                                   displayError("Dump save failed");
                               }
                           }});
    }

    if (mfud.size() > 0) {
        options.push_back({"Save MFU dump", [this]() {
                               String fileName =
                                   saveHfDumpBinFile(mfud, pn532_ble.hf14aTagInfo.uid_hex, "mfu-");
                               if (fileName != "") {
                                   displaySuccess("Saved to " + fileName);
                               } else {
                                   displayError("Dump save failed");
                               }
                           }});
    }

    if (iso15dump.size() > 0) {
        options.push_back({"Save ISO15693 dump", [this]() {
                               String fileName =
                                   saveHfDumpBinFile(iso15dump, pn532_ble.hf15TagInfo.uid_hex, "iso15-");
                               if (fileName != "") {
                                   displaySuccess("Saved to " + fileName);
                               } else {
                                   displayError("Dump save failed");
                               }
                           }});
    }

    options.push_back({"Back", [this]() { selectMode(); }});
    loopOptions(options);
}

void Pn532ble::loadDumpMenu() {
    options = {
        {"Load MFC",      [this]() { setMode(HF_MF_LOAD_DUMP_MODE); }      },
        {"Load MFU",      [this]() { setMode(HF_MFU_LOAD_DUMP_MODE); }     },
        {"Load ISO15693", [this]() { setMode(HF_ISO15693_LOAD_DUMP_MODE); }},
        {"Back",          [this]() { selectMode(); }                       },
    };

    loopOptions(options);
}

void Pn532ble::loadNdefEmulateMenu() {
    String prefix = "";

    options = {
        {"Visit Bruce", [&]() { prefix = "https://bruce.computer"; }},
        {"Open Url",    [&]() { prefix = "https://"; }              },
        {"Phone Call",  [&]() { prefix = "tel:"; }                  },
        {"Send Email",  [&]() { prefix = "mailto:"; }               },
        {"Custom",      [&]() { prefix = ""; }                      },
        {"Back",        [&]() { selectMode(); }                     },
    };
    delay(200);
    loopOptions(options);

    String ndef_data = keyboard(prefix, 255, "NDEF data:");
    ndef_data.trim();
    emulationNdefData = ndef_data;
    setMode(HF_TG_INIT_AS_TARGET_MODE);
}

void Pn532ble::setMode(AppMode mode) {
    currentMode = mode;

    displayBanner();
    switch (mode) {
        case STANDBY_MODE: padprintln(""); padprintln("[ok] - Select mode");
#ifdef HAS_KEYBOARD
            if (pn532_ble.isConnected()) { padprintln("[h] - Scan ISO14443A"); }
            if (pn532_ble.isPN532Killer()) {
                padprintln("[H] - Scan ISO15693");
                padprintln("[l] - Scan EM4100");
            }
            padprintln("");
            if (pn532_ble.isConnected()) {
                padprintln("[c] - Read Mifare Classic");
                padprintln("[u] - Read Mifare Ultralight");
            }
            if (pn532_ble.isPN532Killer()) { padprintln("[i] - Read ISO15693"); }
#endif
            break;
        case GET_FW_MODE:
            if (pn532_ble.isConnected()) {
                showDeviceInfo();
            } else {
                padprintln("Device not connected");
            }
            break;
        case HF_14A_SCAN_MODE: hf14aScan(); break;
        case HF_14B_SCAN_MODE: padprintln("Scan mode not supported"); break;
        case HF_15_SCAN_MODE: hf15Scan(); break;
        case LF_EM4100_SCAN_MODE: lfScan(); break;
        case HF_MF_READ_MODE: hf14aMfReadDumpMode(); break;
        case HF_MFU_READ_MODE: hf14aMfuReadDumpMode(); break;
        case HF_ISO15693_READ_MODE: hf15ReadDumpMode(); break;
        case HF_MF_WRITE_MODE: hf14aMfWriteDumpMode(); break;
        case HF_MFU_WRITE_MODE: hf14aMfuWriteDumpMode(); break;
        case HF_ISO15693_WRITE_MODE: hf15WriteDumpMode(); break;
        case HF_MF_LOAD_DUMP_MODE: loadMifareClassicDumpFile(); break;
        case HF_MFU_LOAD_DUMP_MODE: loadMifareUltralightDumpFile(); break;
        case HF_ISO15693_LOAD_DUMP_MODE: loadIso15693DumpFile(); break;
        case HF_TG_INIT_AS_TARGET_MODE: ntagEmulationMode(); break;
    }
}

void Pn532ble::displayBanner() {
    drawMainBorderWithTitle("PN532 BLE");
    padprintln("PN532 HSU Mode on BLE");
    delay(100);
}

void Pn532ble::showDeviceInfo() {
    displayBanner();
    padprintln("Devices: " + String(pn532_ble.getName().c_str()));
    pn532_ble.setNormalMode();
    bool res = pn532_ble.getVersion();
    if (!res) {
        displayError("Get version failed");
        delay(1000);
        return;
    }
    uint8_t *version = pn532_ble.cmdResponse.data;
    uint8_t dataSize = pn532_ble.cmdResponse.dataSize;
    String versionStr = "Version: ";
    for (size_t i = 0; i < dataSize; i++) {
        versionStr += version[i] < 0x10 ? " 0" : " ";
        versionStr += String(version[i], HEX);
    }
    padprintln(versionStr);
    padprintln("------------");
    padprintln("");
    padprintln("[ok] - Select mode");
}

void Pn532ble::hf14aScan() {
    displayBanner();
    padprintln("HF 14a Scan");
    delay(200);
    pn532_ble.setNormalMode();
    PN532_BLE::Iso14aTagInfo tagInfo = pn532_ble.hf14aScan();
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
    } else if (tagInfo.uid.size() != 4 && tagInfo.uid.size() != 7 && tagInfo.uid.size() != 10) {
        displayError("Not ISO14443A Tag");
    } else {
        padprintln("------------");
        padprintln("Type: " + tagInfo.type);
        padprintln("UID:  " + tagInfo.uid_hex);
        padprintln("ATQA: " + tagInfo.atqa_hex);
        padprintln("SAK:  " + tagInfo.sak_hex);
        if (tagInfo.sak == 0x00) {
            padprintln("------------");
        } else if (tagInfo.sak == 0x08 || tagInfo.sak == 0x09 || tagInfo.sak == 0x18) {
            bool isGen1A = pn532_ble.isGen1A();
            padprintln("Gen1A: " + String(isGen1A ? "Yes" : "No"));
            bool isGen3 = pn532_ble.isGen3();
            padprintln("Gen3:  " + String(isGen3 ? "Yes" : "No"));
            bool isGen4 = pn532_ble.isGen4(gen4pwd);
            padprintln("Gen4:  " + String(isGen4 ? "Yes" : "No"));
        }
    }
}

void Pn532ble::hf15Scan() {
    displayBanner();
    padprintln("HF 15 Scan");
    delay(200);
    if (!pn532_ble.isPN532Killer()) {
        displayError("Not supported");
        return;
    }
    pn532_ble.setNormalMode();
    PN532_BLE::Iso15TagInfo tagInfo = pn532_ble.hf15Scan();
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
    } else if (tagInfo.uid.size() != 8) {
        displayError("Not ISO15693 Tag");
    } else {
        padprintln("------------");
        padprintln("UID:  " + tagInfo.uid_hex);
        tagInfo = pn532_ble.hf15Info();
        padprintln("DSFID: " + String(tagInfo.dsfid, HEX));
        padprintln("AFI:   " + String(tagInfo.afi, HEX));
        padprintln("ICRef: " + String(tagInfo.icRef, HEX));
        padprintln("BlockSize: " + String(tagInfo.blockSize));
    }
}

void Pn532ble::lfScan() {
    displayBanner();
    padprintln("LF Scan");
    delay(200);
    if (!pn532_ble.isPN532Killer()) {
        displayError("Not supported");
        return;
    }

    pn532_ble.setNormalMode();
    PN532_BLE::LfTagInfo tagInfo = pn532_ble.lfScan();
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
    } else {
        padprintln("------------");
        padprintln("UID: " + tagInfo.uid_hex);
        padprintln("ID:  " + String(abs(tagInfo.id_dec)));
    }
}

void updateArea(ScrollableTextArea &area) {
    if (check(PrevPagePress)) {
        area.scrollUp();
    } else if (check(NextPagePress)) {
        area.scrollDown();
    }
    area.draw();
}

void Pn532ble::hf14aMfReadDumpMode() {
    displayBanner();
    padprintln("HF MFC Dump");
    padprintln("------------");
    pn532_ble.setNormalMode();
    PN532_BLE::Iso14aTagInfo tagInfo = pn532_ble.hf14aScan();
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
        return;
    }
    mfd.clear();
    padprintln("UID:  " + tagInfo.uid_hex);
    delay(200);
    drawMainBorder(true);
    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);

    if (tagInfo.sak == 0x08 || tagInfo.sak == 0x09 || tagInfo.sak == 0x18) {
        if (pn532_ble.isGen1A()) {
            area.addLine("TYPE: " + tagInfo.type);
            area.scrollDown();
            area.draw();
            area.addLine("UID:  " + tagInfo.uid_hex);
            area.scrollDown();
            area.draw();
            area.addLine("MAGI: Gen1A");
            area.scrollDown();
            area.draw();
            area.addLine("------------");
            area.scrollDown();
            area.draw();
            for (uint8_t i = 0; i < 64; i++) {
                uint8_t blockData[16];
                std::vector<uint8_t> res = pn532_ble.sendData({0x30, i}, true);
                if (res.size() < 18) {
                    displayError("Read failed");
                    return;
                }
                String blockStr = String(i) + " ";
                for (uint8_t j = 0; j < 16; j++) {
                    blockData[j] = res[j + 1];
                    mfd.push_back(blockData[j]);
                }

                for (uint8_t j = 0; j < 16; j++) {
                    blockStr += blockData[j] < 0x10 ? "0" : "";
                    blockStr += String(blockData[j], HEX);
                }
                area.addLine(blockStr);
                area.scrollDown();
                area.draw();
            }
            area.addLine("------------");
            area.scrollDown();
            area.draw();
        } else if (pn532_ble.isGen4(gen4pwd)) {
            delay(200);
            area.addLine("TYPE: " + tagInfo.type);
            area.scrollDown();
            area.draw();
            area.addLine("UID:  " + tagInfo.uid_hex);
            area.scrollDown();
            area.draw();
            area.addLine("MAGI: Gen4");
            area.scrollDown();
            area.draw();
            area.addLine("------------");
            area.scrollDown();
            area.draw();
            uint8_t sectorCount = getMifareClassicSectorCount(tagInfo.sak);
            for (uint8_t s = 0; s < sectorCount; s++) {
                uint8_t sectorBlockIdex = (s < 32) ? s * 4 : 32 * 4 + (s - 32) * 16;
                uint8_t sectorBlockSize = (s < 32) ? 4 : 16;
                for (uint8_t i = 0; i < sectorBlockSize; i++) {
                    uint8_t blockIndex = sectorBlockIdex + i;
                    uint8_t blockData[16];
                    std::vector<uint8_t> res =
                        pn532_ble.sendData({0xCF, 0x00, 0x00, 0x00, 0x00, 0xCE, blockIndex}, true);
                    if (res.size() < 18) {
                        displayError("Read failed");
                        return;
                    }
                    String blockStr = String(blockIndex) + " ";
                    for (uint8_t j = 0; j < 16; j++) {
                        blockData[j] = res[j + 1];
                        mfd.push_back(blockData[j]);
                        blockStr += blockData[j] < 0x10 ? "0" : "";
                        blockStr += String(blockData[j], HEX);
                    }
                    area.addLine(blockStr);
                    area.scrollDown();
                    area.draw();
                }
            }
            area.addLine("------------");
            area.scrollDown();
            area.draw();
        } else {
            tagInfo = pn532_ble.hf14aScan();
            area.addLine("TYPE: " + tagInfo.type);
            area.scrollDown();
            area.draw();
            area.addLine("UID:  " + tagInfo.uid_hex);
            area.scrollDown();
            area.draw();
            area.addLine("------------");
            area.scrollDown();
            area.draw();

            uint8_t sectorCount = getMifareClassicSectorCount(tagInfo.sak);
            for (uint8_t s = 0; s < sectorCount; s++) {
                pn532_ble.hf14aScan();
                uint8_t sectorBlockIdex = (s < 32) ? s * 4 : 32 * 4 + (s - 32) * 16;
                bool useKeyA = true;
                bool authResult =
                    pn532_ble.mfAuth(tagInfo.uid, sectorBlockIdex, pn532_ble.mifareDefaultKey, useKeyA);
                if (!authResult) {
                    useKeyA = false;
                    pn532_ble.hf14aScan();
                    authResult =
                        pn532_ble.mfAuth(tagInfo.uid, sectorBlockIdex, pn532_ble.mifareDefaultKey, useKeyA);
                }
                if (!authResult) {
                    displayError("Sector " + String(s) + " auth failed");
                    continue;
                }
                uint8_t sectorBlockSize = (s < 32) ? 4 : 16;
                for (uint8_t i = 0; i < sectorBlockSize; i++) {
                    uint8_t blockIndex = sectorBlockIdex + i;
                    uint8_t blockData[16];
                    std::vector<uint8_t> res = pn532_ble.mfRdbl(blockIndex);
                    if (res.size() < 16) {
                        area.addLine("Sector " + String(s) + " Block " + String(blockIndex) + " read failed");
                        area.scrollDown();
                        area.draw();
                        continue;
                    }
                    String blockStr = String(blockIndex) + " ";
                    for (uint8_t j = 0; j < 16; j++) { blockData[j] = res[j + 1]; }

                    if (i == sectorBlockSize - 1) {
                        if (useKeyA) {
                            for (uint8_t j = 0; j < 6; j++) { blockData[j] = pn532_ble.mifareDefaultKey[j]; }
                        } else {
                            for (uint8_t j = 0; j < 6; j++) {
                                blockData[j + 10] = pn532_ble.mifareDefaultKey[j];
                            }
                        }
                    }

                    for (uint8_t j = 0; j < 16; j++) {
                        mfd.push_back(blockData[j]);
                        blockStr += blockData[j] < 0x10 ? "0" : "";
                        blockStr += String(blockData[j], HEX);
                    }
                    area.addLine(blockStr);
                    area.scrollDown();
                    area.draw();
                }
            }
            area.addLine("------------");
            area.scrollDown();
            area.draw();
        }
        pn532_ble.wakeup();

        while (check(SelPress)) {
            updateArea(area);
            yield();
        }
        while (!check(SelPress)) {
            updateArea(area);
            yield();
        }
    } else {
        area.addLine("Not Mifare Ultralight");
        area.scrollDown();
        area.draw();
    }
}

void Pn532ble::hf14aMfuReadDumpMode() {
    displayBanner();
    padprintln("HF MFU Dump");
    padprintln("------------");
    pn532_ble.setNormalMode();
    PN532_BLE::Iso14aTagInfo tagInfo = pn532_ble.hf14aScan();
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
        return;
    }
    mfd.clear();
    padprintln("UID:  " + tagInfo.uid_hex);
    delay(200);
    drawMainBorder(true);

    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);

    if (tagInfo.sak == 0x00) {
        mfud.clear();
        area.addLine("TYPE: " + tagInfo.type);
        area.scrollDown();
        area.draw();
        area.addLine("UID:  " + tagInfo.uid_hex);
        area.scrollDown();
        area.draw();

        int max_block = 4;
        int block = 0;
        while (block < max_block) {
            std::vector<uint8_t> res = pn532_ble.mfRdbl(block);
            if (res.size() == 17) { res.erase(res.begin()); }
            if (block == 0 && res.size() == 16) {
                max_block = res[14] * 2 + 9;
                area.addLine("PAGE: " + String(max_block));
                area.addLine("------------");
                area.scrollDown();
                area.draw();
            }
            if (res.size() == 16) {
                for (int i = 0; i < 4; i++) {
                    int block_index = block + i;
                    String blockStr = String(block_index < 10 ? "0" : "") + String(block_index) + " ";
                    for (int k = 0; k < 4; k++) {
                        uint8_t byteData = res[i * 4 + k];
                        mfud.push_back(byteData);
                        blockStr += byteData < 0x10 ? "0" : "";
                        blockStr += String(byteData, HEX);
                    }
                    blockStr += "  |  ";
                    if (block_index == 0) {
                        blockStr += "ID 0-2, BCC1";
                    } else if (block_index == 1) {
                        blockStr += "ID 3-6";
                    } else if (block_index == 2) {
                        blockStr += "BCC2,";
                    } else {
                        for (int k = 0; k < 4; k++) {
                            uint8_t byteData = res[i * 4 + k];
                            if (byteData >= 32 && byteData <= 126) {
                                blockStr += char(byteData);
                            } else {
                                blockStr += " ";
                            }
                        }
                    }

                    area.addLine(blockStr);
                    area.scrollDown();
                    area.draw();
                }
            } else {
                padprintln("Block " + String(block) + " Failed to read");
            }
            block += 4;
        }
        area.addLine("------------");
        area.scrollDown();
        area.draw();
        pn532_ble.wakeup();

        while (check(SelPress)) {
            updateArea(area);
            yield();
        }
        while (!check(SelPress)) {
            updateArea(area);
            yield();
        }
    } else {
        area.addLine("Not Mifare Ultralight");
        area.scrollDown();
        area.draw();
    }
}

void Pn532ble::hf14aMfuWriteDumpMode() {
    displayBanner();
    padprintln("HF MFU Write Dump");
    pn532_ble.setNormalMode();
    PN532_BLE::Iso14aTagInfo tagInfo = pn532_ble.hf14aScan();
    padprintln("------------");
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
        return;
    }
    padprintln("UID:  " + tagInfo.uid_hex);
    padprintln("Type: " + tagInfo.type);
    delay(200);
    drawMainBorder(true);

    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);
    if (tagInfo.sak == 0x00) {
        area.addLine("Write Mifare Ultralight");
        area.addLine("------------");
        area.scrollDown();
        area.draw();
        delay(200);
        int max_block = 4;
        int block = 0;
        std::vector<uint8_t> first4Page = pn532_ble.mfRdbl(block);
        if (first4Page.size() == 17) { first4Page.erase(first4Page.begin()); }
        if (first4Page.size() == 16) { max_block = first4Page[14] * 2 + 9; }

        if (max_block > mfud.size() / 4) {
            displayError("Dump size not enough");
            return;
        }

        for (int i = 0; i < mfud.size(); i += 4) {
            std::vector<uint8_t> data(mfud.begin() + i, mfud.begin() + i + 4);
            bool res = pn532_ble.mfuWrbl(block, data);
            if (res) {
                area.addLine("Block " + String(block) + " write success");
            } else {
                area.addLine("Block " + String(block) + " write failed");
            }
            area.scrollDown();
            area.draw();
            block += 4;
        }
        area.addLine("------------");
        area.scrollDown();
        area.draw();

        pn532_ble.wakeup();
        while (check(SelPress)) {
            updateArea(area);
            yield();
        }
        while (!check(SelPress)) {
            updateArea(area);
            yield();
        }
    } else {
        area.addLine("Not Mifare Ultralight");
        area.scrollDown();
        area.draw();
    }
}
void Pn532ble::hf14aMfWriteDumpMode() {
    displayBanner();
    padprintln("HF 14A Write Dump");
    pn532_ble.setNormalMode();
    PN532_BLE::Iso14aTagInfo tagInfo = pn532_ble.hf14aScan();
    padprintln("------------");
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
        return;
    }
    padprintln("UID:  " + tagInfo.uid_hex);
    padprintln("Type: " + tagInfo.type);
    delay(200);
    drawMainBorder(true);
    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);
    if (mfd.size() == 1024 && pn532_ble.isGen1A()) {
        area.addLine("Write Mifare Classic");
        area.addLine("------------");
        area.scrollDown();
        area.draw();
        delay(200);
        for (uint8_t i = 0; i < 64; i++) {
            uint8_t blockData[16];
            for (uint8_t j = 0; j < 16; j++) { blockData[j] = mfd[i * 16 + j]; }
            std::vector<uint8_t> res1 = pn532_ble.sendData({0xA0, i}, true);
            std::vector<uint8_t> res2 =
                pn532_ble.sendData(std::vector<uint8_t>(blockData, blockData + 16), true);
            if (res2.size() > 0 && res2[0] == 0x00) {
                area.addLine("Block " + String(i) + " write success");
            } else {
                area.addLine("Block " + String(i) + " write failed");
            }
            area.scrollDown();
            area.draw();
        }
        area.addLine("------------");
        area.scrollDown();
        area.draw();
    } else if (pn532_ble.isGen4(gen4pwd)) {
        area.addLine("Write Dump to Gen4");
        area.addLine("------------");
        area.scrollDown();
        area.draw();
        std::vector<uint8_t> pwd;
        for (size_t i = 0; i < gen4pwd.size(); i += 2) {
            pwd.push_back(strtol(gen4pwd.substr(i, 2).c_str(), NULL, 16));
        }
        std::vector<uint8_t> configCmd = {0xCF, pwd[0], pwd[1], pwd[2], pwd[3], 0xF0};
        String gen4Type = "Unknown";
        if (pn532_ble.hf14aTagInfo.uidSize == 4 && mfd.size() == 320) {
            configCmd.insert(configCmd.end(), default4bS20Config.begin(), default4bS20Config.end());
            gen4Type = "4B S20";
        } else if (pn532_ble.hf14aTagInfo.uidSize == 7 && mfd.size() == 320) {
            configCmd.insert(configCmd.end(), default7bS20Config.begin(), default7bS20Config.end());
            gen4Type = "7B S20";
        } else if (pn532_ble.hf14aTagInfo.uidSize == 4 && mfd.size() == 4096) {
            configCmd.insert(configCmd.end(), default4bS70Config.begin(), default4bS70Config.end());
            gen4Type = "4B S70";
        } else if (pn532_ble.hf14aTagInfo.uidSize == 7 && mfd.size() == 4096) {
            configCmd.insert(configCmd.end(), default7bS70Config.begin(), default7bS70Config.end());
            gen4Type = "7B S70";
        } else if (pn532_ble.hf14aTagInfo.uidSize == 4 && mfd.size() == 1024) {
            configCmd.insert(configCmd.end(), default4bS50Config.begin(), default4bS50Config.end());
            gen4Type = "4B S50";
        } else if (pn532_ble.hf14aTagInfo.uidSize == 7 && mfd.size() == 1024) {
            configCmd.insert(configCmd.end(), default7bS50Config.begin(), default7bS50Config.end());
            gen4Type = "7B S50";
        } else {
            area.addLine("Config Gen4 failed");
            area.scrollDown();
            area.draw();
            return;
        }

        std::vector<uint8_t> res = pn532_ble.sendData(configCmd, true);
        if (res.size() > 0 && res[0] == 0x00) {
            area.addLine("Config Gen4 to " + gen4Type + " success");
            area.scrollDown();
            area.draw();
            delay(500);
            uint8_t blockSize = mfd.size() / 16;
            for (uint8_t i = 0; i < blockSize; i++) {
                std::vector<uint8_t> data(mfd.begin() + i * 16, mfd.begin() + i * 16 + 16);
                std::vector<uint8_t> blockWriteCommand = {0xCF, pwd[0], pwd[1], pwd[2], pwd[3], 0xCD, i};
                blockWriteCommand.insert(blockWriteCommand.end(), data.begin(), data.end());
                std::vector<uint8_t> res = pn532_ble.sendData(blockWriteCommand, true);
                if (res.size() > 0 && res[0] == 0x00) {
                    area.addLine("Block " + String(i) + " write success");
                } else {
                    area.addLine("Block " + String(i) + " write failed");
                }
                area.scrollDown();
                area.draw();
            }
            area.addLine("------------");
            area.scrollDown();
            area.draw();
        } else {
            area.addLine("Config Gen4 to S50 failed");
            area.scrollDown();
            area.draw();
        }
    } else if (pn532_ble.isGen3()) {
        area.addLine("Write to Gen3");
        area.addLine("------------");
        area.scrollDown();
        area.draw();
        uint8_t uidSize = pn532_ble.hf14aTagInfo.uidSize;
        area.addLine("Set UID");
        area.scrollDown();
        area.draw();
        std::vector<uint8_t> setUidCmd = {0x90, 0xFB, 0xCC, 0xCC, 0x07};
        for (byte i = 0; i < uidSize; i++) { setUidCmd.push_back(mfd[i]); }
        pn532_ble.sendData(setUidCmd, true);
        area.addLine("Set UID Block");
        area.scrollDown();
        area.draw();
        std::vector<uint8_t> setBlock0Config = {0x90, 0xF0, 0xCC, 0xCC, 0x10};
        for (byte i = 0; i < 16; i++) { setBlock0Config.push_back(mfd[i]); }
        std::vector<uint8_t> res = pn532_ble.sendData(setBlock0Config, true);
        if (res.size() > 0 && res[0] == 0x00) {
            area.addLine("UID and Block0 write success");
            area.scrollDown();
            area.draw();
            hf14aMfWriteDump(area);
        } else {
            area.addLine("UID and Block0 write failed");
            area.scrollDown();
            area.draw();
        }
    } else {
        hf14aMfWriteDump(area);
    }
    pn532_ble.wakeup();

    while (check(SelPress)) {
        updateArea(area);
        yield();
    }
    while (!check(SelPress)) {
        updateArea(area);
        yield();
    }
}

void Pn532ble::hf14aMfWriteDump(ScrollableTextArea &area) {
    uint8_t blockSize = mfd.size() / 16;
    uint8_t sectorCount = mfd.size() == 4096 ? 40 : (mfd.size() / 64);
    for (uint8_t s = 0; s < sectorCount; s++) {
        pn532_ble.hf14aScan();
        uint8_t sectorBlockIdex = (s < 32) ? s * 4 : 32 * 4 + (s - 32) * 16;
        uint8_t sectorBlockSize = (s < 32) ? 4 : 16;
        bool authResult =
            pn532_ble.mfAuth(pn532_ble.hf14aTagInfo.uid, sectorBlockIdex, pn532_ble.mifareDefaultKey, false);
        if (!authResult) {
            pn532_ble.hf14aScan();
            authResult = pn532_ble.mfAuth(
                pn532_ble.hf14aTagInfo.uid, sectorBlockIdex, pn532_ble.mifareDefaultKey, true
            );
        }
        if (!authResult) {
            area.addLine("Sector " + String(s) + " auth failed");
            area.scrollDown();
            area.draw();
            continue;
        }
        for (uint8_t i = 0; i < sectorBlockSize; i++) {
            uint8_t blockIndex = sectorBlockIdex + i;
            std::vector<uint8_t> data(mfd.begin() + blockIndex * 16, mfd.begin() + blockIndex * 16 + 16);
            bool writeResult = pn532_ble.mfWrbl(blockIndex, data);
            if (writeResult) {
                area.addLine("Block " + String(blockIndex) + " write success");
            } else {
                area.addLine("Block " + String(blockIndex) + " write failed");
            }
            area.scrollDown();
            area.draw();
        }
    }
    area.addLine("------------");
    area.scrollDown();
    area.draw();
}
uint8_t Pn532ble::getMifareClassicSectorCount(uint8_t sak) {
    switch (sak) {
        case 0x08: return 16;
        case 0x09: return 5;
        case 0x18: return 40;
        default: return 0;
    }
}

void Pn532ble::hf15ReadDumpMode() {
    displayBanner();
    padprintln("HF 15 Dump");
    pn532_ble.setNormalMode();
    PN532_BLE::Iso15TagInfo tagInfo = pn532_ble.hf15Scan();
    padprintln("------------");
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
        return;
    }
    padprintln("UID:  " + tagInfo.uid_hex);
    padprintln("Checking Tag...");
    tagInfo = pn532_ble.hf15Info();
    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);

    iso15dump.clear();
    if (tagInfo.blockSize > 0) {
        area.addLine("UID:  " + tagInfo.uid_hex);
        area.addLine("DSFID: " + String(tagInfo.dsfid, HEX));
        area.addLine("AFI:   " + String(tagInfo.afi, HEX));
        area.addLine("ICRef: " + String(tagInfo.icRef, HEX));
        area.addLine("BlockSize: " + String(tagInfo.blockSize));
        area.addLine("------------");
        area.scrollDown();
        area.draw();
        for (uint8_t i = 0; i < tagInfo.blockSize; i++) {
            std::vector<uint8_t> res = pn532_ble.hf15Rdbl(i);
            if (res.size() < 4) {
                displayError("Read failed");
                return;
            }
            String blockStr = String(i) + " ";
            for (uint8_t j = 0; j < 4; j++) {
                iso15dump.push_back(res[j + 1]);
                blockStr += res[j + 1] < 0x10 ? "0" : "";
                blockStr += String(res[j + 1], HEX);
            }
            area.addLine(blockStr);
            area.scrollDown();
            area.draw();
        }
        area.addLine("------------");
        area.scrollDown();
        area.draw();
    }

    while (check(SelPress)) {
        updateArea(area);
        yield();
    }
    while (!check(SelPress)) {
        updateArea(area);
        yield();
    }
}

void Pn532ble::hf15WriteDumpMode() {
    displayBanner();
    padprintln("HF 15 Write Dump");
    pn532_ble.setNormalMode();
    PN532_BLE::Iso15TagInfo tagInfo = pn532_ble.hf15Scan();
    padprintln("------------");
    if (tagInfo.uid.empty()) {
        displayError("No tag found");
        return;
    }
    padprintln("UID:  " + tagInfo.uid_hex);
    padprintln("Checking Tag...");
    tagInfo = pn532_ble.hf15Info();
    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);

    iso15dump.clear();
    if (tagInfo.blockSize > 0) {
        area.addLine("UID:  " + tagInfo.uid_hex);
        area.addLine("DSFID: " + String(tagInfo.dsfid, HEX));
        area.addLine("AFI:   " + String(tagInfo.afi, HEX));
        area.addLine("ICRef: " + String(tagInfo.icRef, HEX));
        area.addLine("BlockSize: " + String(tagInfo.blockSize));
        area.addLine("------------");
        area.scrollDown();
        area.draw();

        // check iso15dump size with tag block size
        if (iso15dump.size() > tagInfo.blockSize * 4) {
            displayError("Dump size is too large");
            return;
        }

        for (uint8_t i = 0; i < tagInfo.blockSize; i++) {
            std::vector<uint8_t> data(iso15dump.begin() + i * 4, iso15dump.begin() + i * 4 + 4);
            bool writeResult = pn532_ble.hf15Wrbl(i, data);
            if (writeResult) {
                area.addLine("Block " + String(i) + " write success");
            } else {
                area.addLine("Block " + String(i) + " write failed");
            }
            area.scrollDown();
            area.draw();
        }

        area.addLine("------------");
        area.scrollDown();
        area.draw();
    }

    while (check(SelPress)) {
        updateArea(area);
        yield();
    }
    while (!check(SelPress)) {
        updateArea(area);
        yield();
    }
}

void Pn532ble::loadMifareClassicDumpFile() {
    FS *fs;
    if (!getFsStorage(fs)) {
        padprintln("No storage found");
        return;
    }
    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    String filePath = loopSD(*fs, true, "bin", "/BruceRFID");
    if (filePath == "") {
        padprintln("No file selected");
        return;
    }

    File file = (*fs).open(filePath, FILE_READ);
    if (!file) {
        padprintln("File open failed");
        return;
    }
    mfd.clear();
    while (file.available()) { mfd.push_back(file.read()); }
    file.close();
    // check dump size if is 320, 1024 or 4096
    if (mfd.size() != 320 && mfd.size() != 1024 && mfd.size() != 4096) {
        padprintln("Invalid dump size: " + String(mfd.size()));
        return;
    }

    displayBanner();

    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);
    area.addLine("Dump: " + filePath);
    area.addLine("Size: " + String(mfd.size()));
    area.addLine("------------");
    area.scrollDown();
    area.draw();
    delay(200);
    uint8_t blockIndex = 0;
    for (size_t i = 0; i < mfd.size(); i += 16) {
        String line = String(blockIndex) + " ";
        for (size_t j = 0; j < 16; j++) {
            line += mfd[i + j] < 0x10 ? "0" : "";
            line += String(mfd[i + j], HEX);
        }
        area.addLine(line);
        area.scrollDown();
        area.draw();
        blockIndex++;
    }
    area.addLine("------------");
    area.scrollDown();
    area.draw();

    while (check(SelPress)) {
        updateArea(area);
        yield();
    }
    while (!check(SelPress)) {
        updateArea(area);
        yield();
    }
}

void Pn532ble::loadMifareUltralightDumpFile() {
    FS *fs;
    if (!getFsStorage(fs)) {
        padprintln("No storage found");
        return;
    }
    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    String filePath = loopSD(*fs, true, "bin", "/BruceRFID");
    if (filePath == "") {
        padprintln("No file selected");
        return;
    }

    File file = (*fs).open(filePath, FILE_READ);
    if (!file) {
        padprintln("File open failed");
        return;
    }
    mfd.clear();
    while (file.available()) { mfd.push_back(file.read()); }
    file.close();

    displayBanner();

    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);
    area.addLine("Dump: " + filePath);
    area.addLine("Size: " + String(mfd.size()));
    area.addLine("------------");
    area.scrollDown();
    area.draw();
    delay(200);
    uint8_t blockIndex = 0;
    for (size_t i = 0; i < mfd.size(); i += 4) {
        String line = String(blockIndex) + " ";
        for (size_t j = 0; j < 4; j++) {
            line += mfd[i + j] < 0x10 ? "0" : "";
            line += String(mfd[i + j], HEX);
        }
        area.addLine(line);
        area.scrollDown();
        area.draw();
        blockIndex++;
    }
    area.addLine("------------");
    area.scrollDown();
    area.draw();

    while (check(SelPress)) {
        updateArea(area);
        yield();
    }
    while (!check(SelPress)) {
        updateArea(area);
        yield();
    }
}

void Pn532ble::loadIso15693DumpFile() {
    FS *fs;
    if (!getFsStorage(fs)) {
        padprintln("No storage found");
        return;
    }
    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    String filePath = loopSD(*fs, true, "bin", "/BruceRFID");
    if (filePath == "") {
        padprintln("No file selected");
        return;
    }

    File file = (*fs).open(filePath, FILE_READ);
    if (!file) {
        padprintln("File open failed");
        return;
    }
    iso15dump.clear();
    while (file.available()) { iso15dump.push_back(file.read()); }
    file.close();

    displayBanner();

    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);
    area.addLine("Dump: " + filePath);
    area.addLine("Size: " + String(iso15dump.size()));
    area.addLine("------------");
    area.scrollDown();
    area.draw();
    delay(200);
    uint8_t blockIndex = 0;
    for (size_t i = 0; i < iso15dump.size(); i += 4) {
        String line = String(blockIndex) + " ";
        for (size_t j = 0; j < 4; j++) {
            line += iso15dump[i + j] < 0x10 ? "0" : "";
            line += String(iso15dump[i + j], HEX);
        }
        area.addLine(line);
        area.scrollDown();
        area.draw();
        blockIndex++;
    }
    area.addLine("------------");
    area.scrollDown();
    area.draw();

    while (check(SelPress)) {
        updateArea(area);
        yield();
    }
    while (!check(SelPress)) {
        updateArea(area);
        yield();
    }
}

void Pn532ble::ntagEmulationMode() {
    ScrollableTextArea area(FP, 10, 28, tftWidth - 20, tftHeight - 38);
    area.addLine("Emulate Tag");
    area.addLine(emulationNdefData);
    area.addLine("------------");
    area.scrollDown();
    area.draw();
    delay(200);

    bool stopFlag = false;
    std::vector<uint8_t> tgInitAsTargetCmd = {0x04, 0x08, 0x00, 0x11, 0x22, 0x33, 0x60, 0x01, 0xFE, 0xA2,
                                              0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4,
                                              0xC5, 0xC6, 0xC7, 0xFF, 0xFF, 0xAA, 0x99, 0x88, 0x77, 0x66,
                                              0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0x00};
    std::vector<uint8_t> initAsTargetResult = pn532_ble.tgInitAsTarget(tgInitAsTargetCmd);
    if (initAsTargetResult.size() == 0) {
        displayError("Init as target failed");
        return;
    }
    String respStr = "Resp: ";
    for (size_t i = 0; i < initAsTargetResult.size(); i++) {
        respStr += initAsTargetResult[i] < 0x10 ? "0" : "";
        String hexStr = String(initAsTargetResult[i], HEX);
        hexStr.toUpperCase();
        respStr += hexStr + " ";
    }
    area.addLine(respStr);
    area.scrollDown();
    area.draw();
    std::vector<uint8_t> compatibilityContainer = {
        0x00, 0x0F, 0x20, 0x00, 0x54, 0x00, 0xFF, 0x04, 0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0x00
    };

    TagFile currentFile = TagFile::NONE;

    while (!stopFlag) {
        if (check(PrevPagePress)) {
            area.scrollUp();
        } else if (check(NextPagePress)) {
            area.scrollDown();
        } else if (check(SelPress)) {
            stopFlag = true;
            break;
        }
        area.draw();

        std::vector<uint8_t> resp = pn532_ble.getData();
        if (check(SelPress)) {
            stopFlag = true;
            break;
        }
        if (resp.empty()) {
            pn532_ble.inRelease();
            delay(10);
            continue;
        }
        respStr = "<= ";
        for (size_t i = 0; i < resp.size(); i++) {
            respStr += resp[i] < 0x10 ? "0" : "";
            respStr += String(resp[i], HEX) + " ";
        }
        area.addLine(respStr);
        area.scrollDown();
        area.draw();

        if (resp[0] == 0x29 || resp[0] == 0x25) {
            area.addLine("Reinit as target");
            area.scrollDown();
            area.draw();
            initAsTargetResult = pn532_ble.tgInitAsTarget(tgInitAsTargetCmd);
            continue;
        }

        if (resp.size() < 5) {
            delay(10);
            area.addLine("Invalid response");
            area.scrollDown();
            area.draw();
            continue;
        }
        resp.erase(resp.begin());
        uint8_t ins = resp[ApduCommand::C_APDU_INS];
        uint8_t p1 = resp[ApduCommand::C_APDU_P1];
        uint8_t p2 = resp[ApduCommand::C_APDU_P2];
        uint16_t p1p2Length = (p1 << 8) | p2;
        uint8_t lc = resp[ApduCommand::C_APDU_LC];

        std::vector<uint8_t> wbuf;

        if (ins == ApduCommand::ISO7816_SELECT_FILE) {
            if (p1 == ApduCommand::C_APDU_P1_SELECT_BY_ID) {
                area.addLine("Reader: Select by ID");
                area.scrollDown();
                area.draw();
                if (p2 != 0x0C) {
                    wbuf = {
                        ApduCommand::R_APDU_SW1_COMMAND_COMPLETE, ApduCommand::R_APDU_SW2_COMMAND_COMPLETE
                    };
                } else if (lc == 0x02 && resp[5] == 0xE1 && (resp[6] == 0x03 || resp[6] == 0x04)) {
                    currentFile = (resp[6] == 0x03) ? TagFile::CC : TagFile::NDEF;
                    wbuf = {
                        ApduCommand::R_APDU_SW1_COMMAND_COMPLETE, ApduCommand::R_APDU_SW2_COMMAND_COMPLETE
                    };
                } else {
                    wbuf = {
                        ApduCommand::R_APDU_SW1_NDEF_TAG_NOT_FOUND, ApduCommand::R_APDU_SW2_NDEF_TAG_NOT_FOUND
                    };
                }
            } else if (p1 == ApduCommand::C_APDU_P1_SELECT_BY_NAME) {
                area.addLine("Reader: Select by Name");
                area.scrollDown();
                area.draw();
                std::vector<uint8_t> application(resp.begin() + 3, resp.begin() + 12);
                if (std::equal(
                        application.begin(), application.end(), NdefCommand::APPLICATION_NAME_V2.begin()
                    )) {
                    wbuf = {
                        ApduCommand::R_APDU_SW1_COMMAND_COMPLETE, ApduCommand::R_APDU_SW2_COMMAND_COMPLETE
                    };
                    area.addLine("Application: V1");
                    area.scrollDown();
                    area.draw();
                } else {
                    wbuf = {
                        ApduCommand::R_APDU_SW1_FUNCTION_NOT_SUPPORTED,
                        ApduCommand::R_APDU_SW2_FUNCTION_NOT_SUPPORTED
                    };
                    area.addLine("Application not found");
                    area.scrollDown();
                    area.draw();
                }
            } else {
                area.addLine("Reader: Unknown function");
                area.scrollDown();
                area.draw();
                wbuf = {
                    ApduCommand::R_APDU_SW1_FUNCTION_NOT_SUPPORTED,
                    ApduCommand::R_APDU_SW2_FUNCTION_NOT_SUPPORTED
                };
            }
        } else if (ins == ApduCommand::ISO7816_READ_BINARY) {
            area.addLine("Reader: Read Binary");
            area.scrollDown();
            area.draw();
            if (currentFile == TagFile::NONE) {
                wbuf = {
                    ApduCommand::R_APDU_SW1_NDEF_TAG_NOT_FOUND, ApduCommand::R_APDU_SW2_NDEF_TAG_NOT_FOUND
                };
            } else if (currentFile == TagFile::CC) {
                if (p1p2Length > NdefCommand::NDEF_MAX_LENGTH) {
                    area.addLine("Reached max length");
                    area.scrollDown();
                    area.draw();
                    wbuf = {
                        ApduCommand::R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES,
                        ApduCommand::R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES
                    };
                } else {
                    area.addLine("Set CC Data");
                    area.scrollDown();
                    area.draw();
                    compatibilityContainer[11] = (NdefCommand::NDEF_MAX_LENGTH & 0xFF00) >> 8;
                    compatibilityContainer[12] = NdefCommand::NDEF_MAX_LENGTH & 0xFF;
                    compatibilityContainer[14] = 0xFF;
                    wbuf.insert(
                        wbuf.end(),
                        compatibilityContainer.begin() + p1p2Length,
                        compatibilityContainer.begin() + p1p2Length + lc
                    );
                    wbuf.push_back(ApduCommand::R_APDU_SW1_COMMAND_COMPLETE);
                    wbuf.push_back(ApduCommand::R_APDU_SW2_COMMAND_COMPLETE);
                }
            } else if (currentFile == TagFile::NDEF) {
                if (p1p2Length > NdefCommand::NDEF_MAX_LENGTH) {
                    area.addLine("NDEF Read failed");
                    area.scrollDown();
                    area.draw();
                    wbuf = {
                        ApduCommand::R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES,
                        ApduCommand::R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES
                    };
                } else {
                    std::vector<uint8_t> payload = Ndef::urlNdefAbbrv(std::string(emulationNdefData.c_str()));
                    String payloadStr = "Payload: ";
                    for (size_t i = 0; i < payload.size(); i++) {
                        payloadStr += payload[i] < 0x10 ? "0" : "";
                        payloadStr += String(payload[i], HEX) + " ";
                    }
                    area.addLine(payloadStr);
                    area.scrollDown();
                    area.draw();
                    Serial.println(payloadStr);
                    std::vector<uint8_t> uriMessage = Ndef::newMessage(payload);
                    String uriMessageStr = "URI Message: ";
                    for (size_t i = 0; i < uriMessage.size(); i++) {
                        uriMessageStr += uriMessage[i] < 0x10 ? "0" : "";
                        uriMessageStr += String(uriMessage[i], HEX) + " ";
                    }

                    area.addLine(uriMessageStr);
                    area.scrollDown();
                    area.draw();
                    Serial.println(uriMessageStr);
                    if (lc == 0x02) {
                        wbuf.push_back((uriMessage.size() >> 8) & 0xFF);
                        wbuf.push_back(uriMessage.size() & 0xFF);
                        wbuf.push_back(ApduCommand::R_APDU_SW1_COMMAND_COMPLETE);
                        wbuf.push_back(ApduCommand::R_APDU_SW2_COMMAND_COMPLETE);
                        area.addLine("Set NDEF data length");
                        area.scrollDown();
                        area.draw();
                    } else {
                        wbuf.insert(wbuf.end(), uriMessage.begin(), uriMessage.end());
                        wbuf.push_back(ApduCommand::R_APDU_SW1_COMMAND_COMPLETE);
                        wbuf.push_back(ApduCommand::R_APDU_SW2_COMMAND_COMPLETE);
                        area.addLine("Set NDEF Data");
                        area.scrollDown();
                        area.draw();
                    }
                }
            }
        } else if (ins == ApduCommand::ISO7816_UPDATE_BINARY) {
            area.addLine("Reader: Update Binary");
            area.scrollDown();
            area.draw();
            wbuf = {
                ApduCommand::R_APDU_SW1_FUNCTION_NOT_SUPPORTED, ApduCommand::R_APDU_SW2_FUNCTION_NOT_SUPPORTED
            };
        } else {
            area.addLine("Unknown Command");
            area.scrollDown();
            area.draw();
            wbuf = {};
        }

        String wbufStr = "PN532: ";
        for (size_t i = 0; i < wbuf.size(); i++) {
            wbufStr += wbuf[i] < 0x10 ? "0" : "";
            String hexStr = String(wbuf[i], HEX);
            hexStr.toUpperCase();
            wbufStr += hexStr + " ";
        }
        wbufStr += " <= ";
        std::vector<uint8_t> setDataResult = pn532_ble.setData(wbuf);
        if (check(SelPress)) {
            stopFlag = true;
            break;
        }
        for (size_t i = 0; i < setDataResult.size(); i++) {
            wbufStr += setDataResult[i] < 0x10 ? "0" : "";
            String hexStr = String(setDataResult[i], HEX);
            hexStr.toUpperCase();
            wbufStr += hexStr + " ";
        }
        Serial.println(wbufStr);
        area.addLine(wbufStr);
        area.scrollDown();
        area.draw();
    }

    pn532_ble.setNormalMode();
}

String Pn532ble::saveHfDumpBinFile(std::vector<uint8_t> data, String uid, String prefix) {
    FS *fs;
    if (!getFsStorage(fs)) return "";
    if (!(*fs).exists("/rfid")) (*fs).mkdir("/rfid");
    if (!(*fs).exists("/rfid/hf")) (*fs).mkdir("/rfid/hf");
    String fileName = prefix + uid;
    if ((*fs).exists("/rfid/hf/" + fileName + ".bin")) {
        int i = 1;
        fileName += "_";
        while ((*fs).exists("/rfid/hf/" + fileName + String(i) + ".bin")) i++;
        fileName += String(i);
    }
    fileName = fileName + ".bin";
    String filePath = "/rfid/hf/" + fileName;
    File file = (*fs).open(filePath, FILE_WRITE);
    if (!file) { return ""; }
    for (size_t i = 0; i < data.size(); i++) { file.write(data[i]); }
    file.close();
    return fileName;
}
#endif
