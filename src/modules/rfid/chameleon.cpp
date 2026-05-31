/**
 * @file chameleon.cpp
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Emulate tags using a Chameleon device
 * @version 0.1
 * @date 2024-10-09
 */

#include "chameleon.h"
#include "core/display.h"
#include "core/mykeyboard.h"

Chameleon::Chameleon() { setup(); }

Chameleon::~Chameleon() {
    if (_scanned_set.size() > 0) {
        saveScanResult();
        _scanned_set.clear();
        _scanned_tags.clear();
    }
}

void Chameleon::setup() {
    displayBanner();

    if (!connect()) return;

    displayBanner();

    setMode(BATTERY_INFO_MODE);
    vTaskDelay(pdMS_TO_TICKS(500));
    return loop();
}

bool Chameleon::connect() {
    displayInfo("Turn on Chameleon device", true);

    displayBanner();
    padprintln("");
    padprintln("Searching Chameleon Device...");

    if (!chmUltra.searchChameleonDevice()) {
        displayError("Chameleon not found", true);
        return false;
    }

    if (!chmUltra.connectToChamelon()) {
        displayError("Chameleon connect error", true);
        return false;
    }

    displaySuccess("Chameleon Connected");
    delayWithReturn(1000);

    return true;
}

void Chameleon::loop() {
    while (1) {
        if (check(EscPress) || returnToMenu) { break; }

        if (check(SelPress)) { selectMode(); }

        switch (currentMode) {
            case BATTERY_INFO_MODE: getBatteryInfo(); break;

            case FACTORY_RESET_MODE: factoryReset(); break;

            case FULL_SCAN_MODE: fullScanTags(); break;

            case LF_READ_MODE: readLFTag(); break;
            case LF_SCAN_MODE: scanLFTags(); break;
            case LF_CLONE_MODE: cloneLFTag(); break;
            case LF_CUSTOM_UID_MODE: customLFUid(); break;
            case LF_EMULATION_MODE: emulateLF(); break;
            case LF_SAVE_MODE: saveFileLF(); break;
            case LF_LOAD_MODE: loadFileLF(); break;

            case HF_READ_MODE: readHFTag(); break;
            case HF_SCAN_MODE: scanHFTags(); break;
            case HF_WRITE_MODE: writeHFData(); break;
            case HF_CLONE_MODE: cloneHFTag(); break;
            case HF_CUSTOM_UID_MODE: customHFUid(); break;
            case HF_EMULATION_MODE: emulateHF(); break;
            case HF_SAVE_MODE: saveFileHF(); break;
            case HF_LOAD_MODE: loadFileHF(); break;
        }
    }
}

void Chameleon::addOptionSetMode(const char *name, AppMode mode) {
    options.push_back({name, [this, mode]() { setMode(mode); }});
}

void Chameleon::selectMode() {
    options = {};

    if (_hf_read_uid) {
        addOptionSetMode("HF Clone UID", HF_CLONE_MODE);
        addOptionSetMode("HF Write data", HF_WRITE_MODE);
        addOptionSetMode("HF Emulation", HF_EMULATION_MODE);
        addOptionSetMode("HF Save file", HF_SAVE_MODE);
    }
    addOptionSetMode("HF Read", HF_READ_MODE);
    addOptionSetMode("HF Scan", HF_SCAN_MODE);
    addOptionSetMode("HF Load file", HF_LOAD_MODE);
    addOptionSetMode("HF Custom UID", HF_CUSTOM_UID_MODE);

    if (_lf_read_uid) {
        addOptionSetMode("LF Clone UID", LF_CLONE_MODE);
        addOptionSetMode("LF Emulation", LF_EMULATION_MODE);
        addOptionSetMode("LF Save file", LF_SAVE_MODE);
    }
    addOptionSetMode("LF Read", LF_READ_MODE);
    addOptionSetMode("LF Scan", LF_SCAN_MODE);
    addOptionSetMode("LF Load file", LF_LOAD_MODE);
    addOptionSetMode("LF Custom UID", LF_CUSTOM_UID_MODE);

    addOptionSetMode("Full Scan", FULL_SCAN_MODE);
    addOptionSetMode("Factory Reset", FACTORY_RESET_MODE);

    loopOptions(options);
    options.clear();
}

void Chameleon::setMode(AppMode mode) {
    currentMode = mode;
    _battery_set = false;

    displayBanner();

    if (_scanned_set.size() > 0) {
        saveScanResult();
        _scanned_set.clear();
        _scanned_tags.clear();
    }

    chmUltra.cmdChangeMode(chmUltra.HW_MODE_READER);

    switch (mode) {
        case LF_READ_MODE:
        case HF_READ_MODE:
            _lf_read_uid = false;
            _hf_read_uid = false;
            break;
        case LF_SCAN_MODE:
        case HF_SCAN_MODE:
        case FULL_SCAN_MODE:
            _scanned_set.clear();
            _scanned_tags.clear();
            break;
        case LF_LOAD_MODE:
        case HF_LOAD_MODE:
        case LF_CUSTOM_UID_MODE:
        case HF_CUSTOM_UID_MODE:
            _lf_read_uid = false;
            _hf_read_uid = false;
            break;
        case LF_CLONE_MODE:
            padprintln("New UID: " + printableLFUID);
            padprintln("");
            break;
        case HF_CLONE_MODE:
            padprintln("Device type: " + printableHFUID.piccType);
            padprintln("New UID: " + printableHFUID.uid);
            padprintln("");
            break;
        case LF_EMULATION_MODE:
            padprintln("UID: " + printableLFUID);
            padprintln("");
            break;
        case HF_EMULATION_MODE:
            padprintln("Device type: " + printableHFUID.piccType);
            padprintln("UID: " + printableHFUID.uid);
            padprintln("");
            break;

        case LF_SAVE_MODE:
        case HF_SAVE_MODE:
        case BATTERY_INFO_MODE:
        case FACTORY_RESET_MODE: break;
        default: padprintln("Mode not supported"); break;
    }
    delayWithReturn(300);
}

void Chameleon::displayBanner() {
    drawMainBorderWithTitle("CHAMELEON");

    switch (currentMode) {
        case BATTERY_INFO_MODE: printSubtitle("BATTERY INFO"); break;
        case FACTORY_RESET_MODE: printSubtitle("FACTORY RESET"); break;
        case FULL_SCAN_MODE: printSubtitle("FULL SCAN MODE"); break;

        case LF_READ_MODE: printSubtitle("LF READ MODE"); break;
        case LF_SCAN_MODE: printSubtitle("LF SCAN MODE"); break;
        case LF_CLONE_MODE: printSubtitle("LF CLONE MODE"); break;
        case LF_CUSTOM_UID_MODE: printSubtitle("LF CUSTOM UID MODE"); break;
        case LF_EMULATION_MODE: printSubtitle("LF EMULATION MODE"); break;
        case LF_SAVE_MODE: printSubtitle("LF SAVE MODE"); break;
        case LF_LOAD_MODE: printSubtitle("LF LOAD MODE"); break;

        case HF_READ_MODE: printSubtitle("HF READ MODE"); break;
        case HF_SCAN_MODE: printSubtitle("HF SCAN MODE"); break;
        case HF_CLONE_MODE: printSubtitle("HF CLONE MODE"); break;
        case HF_WRITE_MODE: printSubtitle("HF WRITE MODE"); break;
        case HF_CUSTOM_UID_MODE: printSubtitle("HF CUSTOM UID MODE"); break;
        case HF_EMULATION_MODE: printSubtitle("HF EMULATION MODE"); break;
        case HF_SAVE_MODE: printSubtitle("HF SAVE MODE"); break;
        case HF_LOAD_MODE: printSubtitle("HF LOAD MODE"); break;
    }

    tft.setTextSize(FP);
    padprintln("");
    padprintln("Press [OK] to change mode.");
    padprintln("");
}

void Chameleon::dumpHFCardDetails() {
    padprintln("Device type: " + printableHFUID.piccType);
    padprintln("UID: " + printableHFUID.uid);
    padprintln("ATQA: " + printableHFUID.atqa);
    padprintln("SAK: " + printableHFUID.sak);
    if (!pageReadSuccess) padprintln("[!] Failed to read data blocks");
}

void Chameleon::dumpScanResults() {
    for (int i = _scanned_tags.size(); i > 0; i--) {
        if (_scanned_tags.size() > 5 && i <= _scanned_tags.size() - 5) return;
        padprintln(String(i) + ": " + _scanned_tags[i - 1].tagType + " | " + _scanned_tags[i - 1].uid);
    }
}

uint8_t Chameleon::selectSlot() {
    uint8_t slot = 8;

    options = {
        {"1", [&]() { slot = 1; }},
        {"2", [&]() { slot = 2; }},
        {"3", [&]() { slot = 3; }},
        {"4", [&]() { slot = 4; }},
        {"5", [&]() { slot = 5; }},
        {"6", [&]() { slot = 6; }},
        {"7", [&]() { slot = 7; }},
        {"8", [&]() { slot = 8; }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Set Emulation Slot");

    return slot;
}

bool Chameleon::isMifareClassic(byte sak) {
    return (sak == 0x08 || sak == 0x09 || sak == 0x10 || sak == 0x11 || sak == 0x18 || sak == 0x19);
}

// HW Methods

void Chameleon::getBatteryInfo() {
    if (_battery_set) return;

    chmUltra.cmdBatteryInfo();

    displayBanner();
    padprintln("");
    padprintln("Battery " + String(chmUltra.cmdResponse.data[2]) + "%");

    _battery_set = true;

    vTaskDelay(pdMS_TO_TICKS(200));
}

void Chameleon::factoryReset() {
    bool proceed = false;

    options = {
        {"No",  [&]() { proceed = false; }},
        {"Yes", [&]() { proceed = true; } },
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Proceed with Factory Reset?");

    displayBanner();

    if (!proceed) {
        displayInfo("Aborting factory reset.");
        delayWithReturn(500);
    } else if (chmUltra.cmdFactoryReset()) {
        displaySuccess("Factory reset success");
        delayWithReturn(500);
    } else {
        displayError("Factory reset error", true);
    }

    delayWithReturn(500);
    returnToMenu = true;
}

// LF Methods

void Chameleon::readLFTag() {
    if (millis() - _lastReadTime < 2000) return;

    if (!chmUltra.cmdLFRead()) return;

    formatLFUID();
    lfTagData = chmUltra.lfTagData;

    displayBanner();
    padprintln("UID: " + printableLFUID);

    _lf_read_uid = true;
    _lastReadTime = millis();
    delayWithReturn(500);
}

void Chameleon::scanLFTags() {
    if (!chmUltra.cmdLFRead()) return;

    formatLFUID();

    if (_scanned_set.find(printableLFUID) == _scanned_set.end()) {
        Serial.println("New LF tag found: " + printableLFUID);
        _scanned_set.insert(printableLFUID);
        _scanned_tags.push_back({"LF", printableLFUID});
    }

    displayBanner();
    dumpScanResults();

    delayWithReturn(200);
}

void Chameleon::cloneLFTag() {
    if (!chmUltra.cmdLFRead()) return;

    if (chmUltra.cmdLFWrite(lfTagData.uidByte, lfTagData.size)) {
        displaySuccess("UID written successfully.");
        delayWithReturn(500);
    } else {
        displayError("Error writing UID to tag.", true);
    }
    delayWithReturn(500);
    setMode(BATTERY_INFO_MODE);
}

void Chameleon::customLFUid() {
    String custom_uid = keyboard("", 10, "UID (hex):");

    custom_uid.trim();
    custom_uid.replace(" ", "");
    custom_uid.toUpperCase();

    displayBanner();

    if (custom_uid.length() != 10) {
        displayError("Invalid UID", true);
        return setMode(BATTERY_INFO_MODE);
    }

    printableLFUID = "";
    for (size_t i = 0; i < custom_uid.length(); i += 2) {
        printableLFUID += custom_uid.substring(i, i + 2) + " ";
    }
    printableLFUID.trim();
    parseLFUID();

    options = {
        {"Clone UID", [this]() { setMode(LF_CLONE_MODE); }    },
        {"Emulate",   [this]() { setMode(LF_EMULATION_MODE); }},
    };
    loopOptions(options);
}

void Chameleon::emulateLF() {
    uint8_t slot = selectSlot();

    displayBanner();

    if (chmUltra.cmdEnableSlot(slot, chmUltra.RFID_LF) && chmUltra.cmdChangeActiveSlot(slot) &&
        chmUltra.cmdLFEconfig(lfTagData.uidByte, lfTagData.size) &&
        chmUltra.cmdChangeMode(chmUltra.HW_MODE_EMULATOR)) {
        displaySuccess("Emulation successful.");
        delayWithReturn(1000);
    } else {
        displayError("Error emulating LF tag.", true);
    }

    setMode(BATTERY_INFO_MODE);
}

void Chameleon::loadFileLF() {
    displayBanner();

    if (readFileLF()) {
        displaySuccess("File loaded");
        delayWithReturn(1000);
        _lf_read_uid = true;

        options = {
            {"Clone UID", [this]() { setMode(LF_CLONE_MODE); }    },
            {"Emulate",   [this]() { setMode(LF_EMULATION_MODE); }},
        };
        loopOptions(options);
    } else {
        displayError("Error loading file", true);
        setMode(BATTERY_INFO_MODE);
    }
}

void Chameleon::saveFileLF() {
    String data = printableLFUID;
    data.replace(" ", "");
    String filename = keyboard(data, 30, "File name:");

    displayBanner();

    if (writeFileLF(filename)) {
        displaySuccess("File saved.");
        delayWithReturn(1000);
    } else {
        displayError("Error writing file.", true);
    }
    setMode(BATTERY_INFO_MODE);
}

bool Chameleon::readFileLF() {
    String filepath;
    File file;
    FS *fs;

    if (!getFsStorage(fs)) return false;
    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    filepath = loopSD(*fs, true, "RFIDLF", "/BruceRFID");
    file = fs->open(filepath, FILE_READ);

    if (!file) { return false; }

    String line;
    String strData;

    while (file.available()) {
        line = file.readStringUntil('\n');
        strData = line.substring(line.indexOf(":") + 1);
        strData.trim();
        if (line.startsWith("UID:")) printableLFUID = strData;
    }

    file.close();
    vTaskDelay(pdMS_TO_TICKS(100));
    parseLFUID();

    return true;
}

bool Chameleon::writeFileLF(String filename) {
    FS *fs;
    if (!getFsStorage(fs)) return false;

    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    if ((*fs).exists("/BruceRFID/" + filename + ".rfidlf")) {
        int i = 1;
        filename += "_";
        while ((*fs).exists("/BruceRFID/" + filename + String(i) + ".rfidlf")) i++;
        filename += String(i);
    }
    File file = (*fs).open("/BruceRFID/" + filename + ".rfidlf", FILE_WRITE);

    if (!file) { return false; }

    file.println("Filetype: Bruce RFID 125kHz File");
    file.println("Version 1");
    file.println("UID: " + printableLFUID);

    file.close();
    vTaskDelay(pdMS_TO_TICKS(100));
    return true;
}

void Chameleon::formatLFUID() {
    printableLFUID = "";
    for (byte i = 0; i < chmUltra.lfTagData.size; i++) {
        printableLFUID += chmUltra.lfTagData.uidByte[i] < 0x10 ? " 0" : " ";
        printableLFUID += String(chmUltra.lfTagData.uidByte[i], HEX);
    }
    printableLFUID.trim();
    printableLFUID.toUpperCase();
}

void Chameleon::parseLFUID() {
    String strUID = printableLFUID;
    strUID.trim();
    strUID.replace(" ", "");

    lfTagData.size = strUID.length() / 2;
    for (size_t i = 0; i < strUID.length(); i += 2) {
        lfTagData.uidByte[i / 2] = strtoul(strUID.substring(i, i + 2).c_str(), NULL, 16);
    }
}

// HF Methods

void Chameleon::readHFTag() {
    if (millis() - _lastReadTime < 2000) return;

    if (!chmUltra.cmd14aScan()) return;

    displayInfo("Reading data blocks...");
    if (chmUltra.hfTagData.sak == 0x00) chmUltra.cmdMfuVersion();

    pageReadSuccess = readHFDataBlocks();

    formatHFData();
    hfTagData = chmUltra.hfTagData;

    displayBanner();
    dumpHFCardDetails();

    _hf_read_uid = true;
    _lastReadTime = millis();
    delayWithReturn(500);
}

void Chameleon::scanHFTags() {
    if (!chmUltra.cmd14aScan()) return;

    formatHFData();

    if (_scanned_set.find(printableHFUID.uid) == _scanned_set.end()) {
        Serial.println("New HF tag found: " + printableHFUID.uid);
        _scanned_set.insert(printableHFUID.uid);
        _scanned_tags.push_back({"HF", printableHFUID.uid});
    }

    displayBanner();
    dumpScanResults();

    delayWithReturn(200);
}

void Chameleon::cloneHFTag() {
    if (!chmUltra.cmd14aScan()) return;

    if (chmUltra.hfTagData.sak != hfTagData.sak) {
        displayError("Tag types do not match.", true);
        return;
    }

    if (chmUltra.cmdMfSetUid(hfTagData.uidByte, hfTagData.size)) {
        displaySuccess("UID written successfully.");
        delayWithReturn(1000);
    } else {
        displayError("Error writing UID to tag.", true);
    }

    setMode(BATTERY_INFO_MODE);
}

void Chameleon::writeHFData() {
    if (!chmUltra.cmd14aScan()) return;

    if (chmUltra.hfTagData.sak != hfTagData.sak) {
        displayError("Tag types do not match.", true);
        return;
    }

    if (writeHFDataBlocks()) {
        displaySuccess("Tag written successfully.");
        delayWithReturn(1000);
    } else {
        displayError("Error writing data to tag.", true);
    }

    setMode(BATTERY_INFO_MODE);
}

void Chameleon::customHFUid() {
    String custom_uid = keyboard("", 14, "UID (hex):");

    custom_uid.trim();
    custom_uid.replace(" ", "");
    custom_uid.toUpperCase();

    displayBanner();

    if (custom_uid.length() != 8 && custom_uid.length() != 14) {
        displayError("Invalid UID", true);
        return setMode(BATTERY_INFO_MODE);
    }

    printableHFUID.uid = "";
    for (size_t i = 0; i < custom_uid.length(); i += 2) {
        printableHFUID.uid += custom_uid.substring(i, i + 2) + " ";
    }
    printableHFUID.uid.trim();

    printableHFUID.sak = custom_uid.length() == 8 ? "08" : "00";
    printableHFUID.atqa = custom_uid.length() == 8 ? "0004" : "0044";
    pageReadSuccess = true;
    parseHFData();
    printableHFUID.piccType = chmUltra.getTagTypeStr(hfTagData.sak);

    options = {
        {"Clone UID", [this]() { setMode(HF_CLONE_MODE); }    },
        {"Emulate",   [this]() { setMode(HF_EMULATION_MODE); }},
    };
    loopOptions(options);
}

void Chameleon::emulateHF() {
    if (!isMifareClassic(hfTagData.sak)) {
        displayError("Not implemented for this tag type", true);
        return setMode(BATTERY_INFO_MODE);
    }

    String strDump = "";
    String strData = "";
    String line = "";
    int startIndex = 0;
    int finalIndex;

    while (true) {
        finalIndex = strAllPages.indexOf("\n", startIndex);
        if (finalIndex == -1) finalIndex = strAllPages.length();

        line = strAllPages.substring(startIndex, finalIndex);
        if (line.length() < 5) break;

        strData = line.substring(line.indexOf(":") + 1);
        strData.trim();
        strDump += strData;

        startIndex = finalIndex + 1;
    }
    strDump.trim();
    strDump.replace(" ", "");

    uint8_t slot = selectSlot();

    ChameleonUltra::TagType tagType = chmUltra.getTagType(hfTagData.sak);

    displayBanner();

    if (chmUltra.cmdEnableSlot(slot, chmUltra.RFID_HF) && chmUltra.cmdChangeActiveSlot(slot) &&
        chmUltra.cmdChangeSlotType(slot, tagType) && chmUltra.cmdMfEload(strDump) &&
        chmUltra.cmdMfEconfig(hfTagData.uidByte, hfTagData.size, hfTagData.atqaByte, hfTagData.sak) &&
        chmUltra.cmdChangeMode(chmUltra.HW_MODE_EMULATOR)) {
        displaySuccess("Emulation successful.");
        delayWithReturn(1000);
    } else {
        displayError("Error emulating HF tag.", true);
    }

    setMode(BATTERY_INFO_MODE);
}

void Chameleon::loadFileHF() {
    displayBanner();

    if (readFileHF()) {
        displaySuccess("File loaded");
        delayWithReturn(1000);
        _hf_read_uid = true;

        options = {
            {"Clone UID",  [this]() { setMode(HF_CLONE_MODE); }    },
            {"Write Data", [this]() { setMode(HF_WRITE_MODE); }    },
            // {"Write Data",  [this]() { setMode(HF_WRITE_MODE); }},
            {"Emulate",    [this]() { setMode(HF_EMULATION_MODE); }},
        };
        loopOptions(options);
    } else {
        displayError("Error loading file", true);
        setMode(BATTERY_INFO_MODE);
    }
}

void Chameleon::saveFileHF() {
    String uid_str = printableHFUID.uid;
    uid_str.replace(" ", "");
    String filename = keyboard(uid_str, 30, "File name:");

    displayBanner();

    if (writeFileHF(filename)) {
        displaySuccess("File saved.");
        delayWithReturn(1000);
    } else {
        displayError("Error writing file.", true);
    }

    setMode(BATTERY_INFO_MODE);
}

bool Chameleon::readFileHF() {
    String filepath;
    File file;
    FS *fs;

    if (!getFsStorage(fs)) return false;
    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    filepath = loopSD(*fs, true, "RFID|NFC", "/BruceRFID");
    file = fs->open(filepath, FILE_READ);

    if (!file) { return false; }

    String line;
    String strData;
    strAllPages = "";
    pageReadSuccess = true;

    while (file.available()) {
        line = file.readStringUntil('\n');
        strData = line.substring(line.indexOf(":") + 1);
        strData.trim();
        if (line.startsWith("Device type:")) printableHFUID.piccType = strData;
        if (line.startsWith("UID:")) printableHFUID.uid = strData;
        if (line.startsWith("SAK:")) printableHFUID.sak = strData;
        if (line.startsWith("ATQA:")) printableHFUID.atqa = strData;
        if (line.startsWith("Pages total:")) dataPages = strData.toInt();
        if (line.startsWith("Pages read:")) pageReadSuccess = false;
        if (line.startsWith("Page ")) strAllPages += line + "\n";
    }

    file.close();
    vTaskDelay(pdMS_TO_TICKS(100));
    parseHFData();

    return true;
}

bool Chameleon::writeFileHF(String filename) {
    FS *fs;
    if (!getFsStorage(fs)) return false;

    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    if ((*fs).exists("/BruceRFID/" + filename + ".rfid")) {
        int i = 1;
        filename += "_";
        while ((*fs).exists("/BruceRFID/" + filename + String(i) + ".rfid")) i++;
        filename += String(i);
    }
    File file = (*fs).open("/BruceRFID/" + filename + ".rfid", FILE_WRITE);

    if (!file) { return false; }

    file.println("Filetype: Bruce RFID File");
    file.println("Version 1");
    file.println("Device type: " + printableHFUID.piccType);
    file.println("# UID, ATQA and SAK are common for all formats");
    file.println("UID: " + printableHFUID.uid);
    file.println("SAK: " + printableHFUID.sak);
    file.println("ATQA: " + printableHFUID.atqa);
    file.println("# Memory dump");
    file.println("Pages total: " + String(dataPages));
    if (!pageReadSuccess) file.println("Pages read: " + String(dataPages));
    file.print(strAllPages);

    file.close();
    vTaskDelay(pdMS_TO_TICKS(100));
    return true;
}

bool Chameleon::readHFDataBlocks() {
    dataPages = 0;
    totalPages = 0;
    bool readSuccess = false;
    strAllPages = "";

    switch (chmUltra.hfTagData.sak) {
        case 0x08:
        case 0x09:
        case 0x10:
        case 0x11:
        case 0x18:
        case 0x19: readSuccess = readMifareClassicDataBlocks({}); break;

        case 0x00: readSuccess = readMifareUltralightDataBlocks(); break;

        default: break;
    }

    return readSuccess;
}

bool Chameleon::readMifareClassicDataBlocks(uint8_t *key) {
    bool sectorReadSuccess;

    switch (chmUltra.hfTagData.sak) {
        case 0x09:
            totalPages = 20; // 320 bytes / 16 bytes per page
            break;

        case 0x08:
            totalPages = 64; // 1024 bytes / 16 bytes per page
            break;

        case 0x18:
            totalPages = 256; // 4096 bytes / 16 bytes per page
            break;

        case 0x19:
            totalPages = 128; // 2048 bytes / 16 bytes per page
            break;

        default: // Should not happen. Ignore.
            break;
    }

    String strPage;

    for (byte i = 0; i < totalPages; i++) {
        if (!chmUltra.cmdMfReadBlock(i, key)) return false;

        strPage = "";
        for (byte index = 0; index < chmUltra.cmdResponse.dataSize; index++) {
            strPage += chmUltra.cmdResponse.data[index] < 0x10 ? F(" 0") : F(" ");
            strPage += String(chmUltra.cmdResponse.data[index], HEX);
        }
        strPage.trim();
        strPage.toUpperCase();

        strAllPages += "Page " + String(dataPages) + ": " + strPage + "\n";
        dataPages++;
    }

    return true;
}

bool Chameleon::readMifareUltralightDataBlocks() {
    String strPage;

    ChameleonUltra::TagType tagType = chmUltra.getTagType(chmUltra.hfTagData.sak);

    switch (tagType) {
        case ChameleonUltra::NTAG_210:
        case ChameleonUltra::MF0UL11: totalPages = 20; break;
        case ChameleonUltra::NTAG_212:
        case ChameleonUltra::MF0UL21: totalPages = 41; break;
        case ChameleonUltra::NTAG_213: totalPages = 45; break;
        case ChameleonUltra::NTAG_215: totalPages = 135; break;
        case ChameleonUltra::NTAG_216: totalPages = 231; break;
        default: totalPages = 256; break;
    }

    for (byte i = 0; i < totalPages; i++) {
        if (!chmUltra.cmdMfuReadPage(i)) return false;
        if (chmUltra.cmdResponse.dataSize == 0) break;

        strPage = "";
        for (byte index = 0; index < chmUltra.cmdResponse.dataSize; index++) {
            strPage += chmUltra.cmdResponse.data[index] < 0x10 ? F(" 0") : F(" ");
            strPage += String(chmUltra.cmdResponse.data[index], HEX);
        }
        strPage.trim();
        strPage.toUpperCase();

        strAllPages += "Page " + String(dataPages) + ": " + strPage + "\n";
        dataPages++;
    }

    return true;
}

bool Chameleon::writeHFDataBlocks() {
    String pageLine = "";
    String strBytes = "";
    int lineBreakIndex;
    int pageIndex;
    bool blockWriteSuccess;
    int totalSize = strAllPages.length();

    while (strAllPages.length() > 0) {
        lineBreakIndex = strAllPages.indexOf("\n");
        pageLine = strAllPages.substring(0, lineBreakIndex);
        strAllPages = strAllPages.substring(lineBreakIndex + 1);

        pageIndex = pageLine.substring(5, pageLine.indexOf(":")).toInt();
        strBytes = pageLine.substring(pageLine.indexOf(":") + 1);
        strBytes.trim();
        strBytes.replace(" ", "");

        if (pageIndex == 0) continue;

        byte size = strBytes.length() / 2;
        byte buffer[size];
        for (size_t i = 0; i < strBytes.length(); i += 2) {
            buffer[i / 2] = strtoul(strBytes.substring(i, i + 2).c_str(), NULL, 16);
        }

        blockWriteSuccess = false;
        if (isMifareClassic(chmUltra.hfTagData.sak)) {
            if (pageIndex == 0 || (pageIndex + 1) % 4 == 0) continue; // Data blocks for MIFARE Classic
            blockWriteSuccess = chmUltra.cmdMfWriteBlock(pageIndex, {}, buffer, size);
        } else if (chmUltra.hfTagData.sak == 0x00) {
            if (pageIndex < 4 || pageIndex >= dataPages - 5) continue; // Data blocks for NTAG21X
            blockWriteSuccess = chmUltra.cmdMfuWritePage(pageIndex, buffer, size);
        }

        if (!blockWriteSuccess) return false;

        progressHandler(totalSize - strAllPages.length(), totalSize, "Writing data blocks...");
    }

    return true;
}

void Chameleon::formatHFData() {
    byte bcc = 0;

    printableHFUID.piccType = chmUltra.getTagTypeStr(chmUltra.hfTagData.sak);

    printableHFUID.sak = chmUltra.hfTagData.sak < 0x10 ? "0" : "";
    printableHFUID.sak += String(chmUltra.hfTagData.sak, HEX);
    printableHFUID.sak.toUpperCase();

    // UID
    printableHFUID.uid = "";
    for (byte i = 0; i < chmUltra.hfTagData.size; i++) {
        printableHFUID.uid += chmUltra.hfTagData.uidByte[i] < 0x10 ? " 0" : " ";
        printableHFUID.uid += String(chmUltra.hfTagData.uidByte[i], HEX);
        bcc = bcc ^ chmUltra.hfTagData.uidByte[i];
    }
    printableHFUID.uid.trim();
    printableHFUID.uid.toUpperCase();

    // BCC
    printableHFUID.bcc = bcc < 0x10 ? "0" : "";
    printableHFUID.bcc += String(bcc, HEX);
    printableHFUID.bcc.toUpperCase();

    // ATQA
    printableHFUID.atqa = "";
    for (byte i = 0; i < 2; i++) {
        printableHFUID.atqa += chmUltra.hfTagData.atqaByte[i] < 0x10 ? " 0" : " ";
        printableHFUID.atqa += String(chmUltra.hfTagData.atqaByte[i], HEX);
    }
    printableHFUID.atqa.trim();
    printableHFUID.atqa.toUpperCase();
}

void Chameleon::parseHFData() {
    String strUID = printableHFUID.uid;
    strUID.trim();
    strUID.replace(" ", "");
    hfTagData.size = strUID.length() / 2;
    for (size_t i = 0; i < strUID.length(); i += 2) {
        hfTagData.uidByte[i / 2] = strtoul(strUID.substring(i, i + 2).c_str(), NULL, 16);
    }

    printableHFUID.sak.trim();
    hfTagData.sak = strtoul(printableHFUID.sak.c_str(), NULL, 16);

    String strATQA = printableHFUID.atqa;
    strATQA.trim();
    strATQA.replace(" ", "");
    for (size_t i = 0; i < strATQA.length(); i += 2) {
        hfTagData.atqaByte[i / 2] = strtoul(strATQA.substring(i, i + 2).c_str(), NULL, 16);
    }
}

void Chameleon::saveScanResult() {
    FS *fs;
    if (!getFsStorage(fs)) return;

    String filename = "scan_result";

    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    if (!(*fs).exists("/BruceRFID/Scans")) (*fs).mkdir("/BruceRFID/Scans");
    if ((*fs).exists("/BruceRFID/Scans/" + filename + ".rfidscan")) {
        int i = 1;
        filename += "_";
        while ((*fs).exists("/BruceRFID/Scans/" + filename + String(i) + ".rfidscan")) i++;
        filename += String(i);
    }
    File file = (*fs).open("/BruceRFID/Scans/" + filename + ".rfidscan", FILE_WRITE);

    if (!file) { return; }

    file.println("Filetype: Bruce RFID Scan Result");
    for (ScanResult scanResult : _scanned_tags) { file.println(scanResult.tagType + " | " + scanResult.uid); }

    file.close();
    vTaskDelay(pdMS_TO_TICKS(100));
    return;
}

void Chameleon::fullScanTags() {
    scanLFTags();
    scanHFTags();
}

void Chameleon::delayWithReturn(uint32_t ms) {
    auto tm = millis();
    while (millis() - tm < ms && !returnToMenu) { vTaskDelay(pdMS_TO_TICKS(50)); }
}
