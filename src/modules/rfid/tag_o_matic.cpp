/**
 * @file tag_o_matic.cpp
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Read and Write RFID tags
 * @version 0.2
 * @date 2024-08-19
 */

#include "tag_o_matic.h"
#include "core/display.h"
#include "core/mykeyboard.h"

#include "PN532.h"
#include "RFID2.h"

#define NDEF_DATA_SIZE 100
#define SCAN_DUMP_SIZE 5

TagOMatic::TagOMatic() {
    _initial_state = READ_MODE;
    setup();
}

TagOMatic::TagOMatic(RFID_State initial_state) {
    if (initial_state == CLONE_MODE || initial_state == WRITE_MODE || initial_state == SAVE_MODE) {
        initial_state = READ_MODE;
    }
    _initial_state = initial_state;
    setup();
}

TagOMatic::~TagOMatic() {
    if (_scanned_set.size() > 0) {
        save_scan_result();
        _scanned_set.clear();
        _scanned_tags.clear();
    }
    delete _rfid; // Deallocate memory for _rfid object
}

void TagOMatic::set_rfid_module() {
    switch (bruceConfigPins.rfidModule) {
        case PN532_I2C_MODULE: _rfid = new PN532(PN532::CONNECTION_TYPE::I2C); break;
#ifdef M5STICK
        case PN532_I2C_SPI_MODULE: _rfid = new PN532(PN532::CONNECTION_TYPE::I2C_SPI); break;
#endif
        case PN532_SPI_MODULE: _rfid = new PN532(PN532::CONNECTION_TYPE::SPI); break;
        case RC522_SPI_MODULE: _rfid = new RFID2(false); break;
        case M5_RFID2_MODULE:
        default: _rfid = new RFID2(); break;
    }
}

void TagOMatic::setup() {
    set_rfid_module();

    if (!_rfid->begin()) {
        displayError("RFID module not found!", true);
        return;
    }

    set_state(_initial_state);
    return loop();
}

void TagOMatic::loop() {
    while (1) {
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }

        if (check(SelPress)) { select_state(); }

        switch (current_state) {
            case READ_MODE: read_card(); break;
            case SCAN_MODE: scan_cards(); break;
            case CHECK_MODE: check_card(); break;
            case LOAD_MODE: load_file(); break;
            case CLONE_MODE: clone_card(); break;
            case CUSTOM_UID_MODE: write_custom_uid(); break;
            case WRITE_MODE: write_data(); break;
            case WRITE_NDEF_MODE: write_ndef_data(); break;
            case ERASE_MODE: erase_card(); break;
            case SAVE_MODE: save_file(); break;
        }
    }
}

void TagOMatic::select_state() {
    options = {};
    if (_read_uid) {
        options.emplace_back("Clone UID", [this]() { set_state(CLONE_MODE); });
        options.emplace_back("Custom UID", [this]() { set_state(CUSTOM_UID_MODE); });
        options.emplace_back("Check tag", [this]() { set_state(CHECK_MODE); });
        options.emplace_back("Write data", [this]() { set_state(WRITE_MODE); });
        options.emplace_back("Save file", [this]() { set_state(SAVE_MODE); });
    }
    options.emplace_back("Read tag", [this]() { set_state(READ_MODE); });
    options.emplace_back("Scan tags", [this]() { set_state(SCAN_MODE); });
    options.emplace_back("Load file", [this]() { set_state(LOAD_MODE); });
    options.emplace_back("Write NDEF", [this]() { set_state(WRITE_NDEF_MODE); });
    options.emplace_back("Erase tag", [this]() { set_state(ERASE_MODE); });

    loopOptions(options);
}

void TagOMatic::set_state(RFID_State state) {
    current_state = state;
    display_banner();
    if (_scanned_set.size() > 0) {
        save_scan_result();
        _scanned_set.clear();
        _scanned_tags.clear();
    }
    _sourceUID = "";
    _sourcePages = "";

    switch (state) {
        case READ_MODE:
        case LOAD_MODE: _read_uid = false; break;
        case SCAN_MODE:
            _scanned_set.clear();
            _scanned_tags.clear();
            break;
        case CHECK_MODE:
            _sourceUID = _rfid->printableUID.uid;
            _sourcePages = _rfid->strAllPages;
            padprintln("Source UID: " + _sourceUID);
            padprintln("");
            break;
        case CLONE_MODE:
            padprintln("New UID: " + _rfid->printableUID.uid);
            padprintln("SAK: " + _rfid->printableUID.sak);
            padprintln("");
            break;
        case WRITE_MODE:
            if (!_rfid->pageReadSuccess) padprintln("[!] Data blocks are incomplete");
            padprintln(String(_rfid->dataPages) + " pages of data to write");
            padprintln("");
            break;
        case WRITE_NDEF_MODE: _ndef_created = false; break;
        case SAVE_MODE:
        case ERASE_MODE:
        case CUSTOM_UID_MODE: break;
    }
    delay(300);
}

void TagOMatic::display_banner() {
    drawMainBorderWithTitle("TAG-O-MATIC");

    switch (current_state) {
        case READ_MODE: printSubtitle("READ MODE"); break;
        case SCAN_MODE: printSubtitle("SCAN MODE"); break;
        case CHECK_MODE: printSubtitle("CHECK MODE"); break;
        case LOAD_MODE: printSubtitle("LOAD MODE"); break;
        case CLONE_MODE: printSubtitle("CLONE MODE"); break;
        case CUSTOM_UID_MODE: printSubtitle("CUSTOM UID MODE"); break;
        case ERASE_MODE: printSubtitle("ERASE MODE"); break;
        case WRITE_MODE: printSubtitle("WRITE DATA MODE"); break;
        case WRITE_NDEF_MODE: printSubtitle("WRITE NDEF MODE"); break;
        case SAVE_MODE: printSubtitle("SAVE MODE"); break;
    }

    tft.setTextSize(FP);
    padprintln("");
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    padprintln("Press [OK] to change mode.");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("");
}

void TagOMatic::dump_card_details() {
    padprintln("Device type: " + _rfid->printableUID.picc_type);
    if (_rfid->printableUID.picc_type != "FeliCa") {
        padprintln("UID: " + _rfid->printableUID.uid);
        padprintln("ATQA: " + _rfid->printableUID.atqa);
        padprintln("SAK: " + _rfid->printableUID.sak);
    } else {
        padprintln("IDm: " + _rfid->printableUID.uid);
        padprintln("PMm: " + _rfid->printableUID.sak);
        padprintln("Sys code: " + _rfid->printableUID.atqa);
    }
    if (_rfid->pageReadStatus != RFIDInterface::SUCCESS)
        padprintln("[!] " + _rfid->statusMessage(_rfid->pageReadStatus));
}

void TagOMatic::dump_check_details() {
    padprintln("Source UID: " + _sourceUID);
    padprintln("");

    padprintln("UID: " + String(_sourceUID == _rfid->printableUID.uid ? "OK" : "NOT OK"));
    padprintln("Data: " + String(_sourcePages == _rfid->strAllPages ? "OK" : "NOT OK"));
    padprintln("");

    if (_rfid->pageReadStatus != RFIDInterface::SUCCESS)
        padprintln("[!] " + _rfid->statusMessage(_rfid->pageReadStatus));
}

void TagOMatic::dump_ndef_details() {
    if (!_ndef_created) return;

    String payload_type = "";
    switch (_rfid->ndefMessage.payloadType) {
        case RFIDInterface::NDEF_URI: payload_type = "URI"; break;
        case RFIDInterface::NDEF_TEXT: payload_type = "Text"; break;
    }

    padprintln("Payload type: " + payload_type);
    padprintln("Payload size: " + String(_rfid->ndefMessage.payloadSize) + " bytes");
}

void TagOMatic::dump_scan_results() {
    for (int i = _scanned_tags.size(); i > 0; i--) {
        if (_scanned_tags.size() > SCAN_DUMP_SIZE && i <= _scanned_tags.size() - SCAN_DUMP_SIZE) return;
        padprintln(String(i) + ": " + _scanned_tags[i - 1]);
    }
}

void TagOMatic::read_card() {
    if (millis() - _lastReadTime < 2000) return;

    if (_rfid->read() != RFIDInterface::SUCCESS) {
        if (bruceConfigPins.rfidModule != M5_RFID2_MODULE) { // Read felica if module is PN532
            if (_rfid->read(1) != RFIDInterface::SUCCESS) return;
        } else {
            return;
        }
    }

    Serial.print("Tag read status: ");
    Serial.println(_rfid->statusMessage(_rfid->pageReadStatus));

    display_banner();
    dump_card_details();

    _read_uid = true;
    _lastReadTime = millis();
    delayWithReturn(500);
}

void TagOMatic::scan_cards() {
    if (_rfid->read() != RFIDInterface::SUCCESS) return;

    if (_scanned_set.find(_rfid->printableUID.uid) == _scanned_set.end()) {
        Serial.println("New tag found: " + _rfid->printableUID.uid);
        _scanned_set.insert(_rfid->printableUID.uid);
        _scanned_tags.push_back(_rfid->printableUID.uid);
    }

    display_banner();
    dump_scan_results();

    delayWithReturn(200);
}

void TagOMatic::check_card() {
    if (millis() - _lastReadTime < 2000) return;

    if (_rfid->read() != RFIDInterface::SUCCESS) return;

    display_banner();
    dump_check_details();

    _lastReadTime = millis();
    delayWithReturn(500);
}

void TagOMatic::clone_card() {
    int result = _rfid->clone();

    switch (result) {
        case RFIDInterface::TAG_NOT_PRESENT: return; break;
        case RFIDInterface::NOT_IMPLEMENTED: displayError("Not implemented for this module."); break;
        case RFIDInterface::TAG_NOT_MATCH: displayError("Tag types do not match."); break;
        case RFIDInterface::SUCCESS: displaySuccess("UID written successfully."); break;
        default: displayError("Error writing UID to tag."); break;
    }

    delayWithReturn(1000);
    set_state(READ_MODE);
}

void TagOMatic::write_custom_uid() {
    String custom_uid = keyboard("", _rfid->uid.size * 2, "UID (hex):");

    custom_uid.trim();
    custom_uid.replace(" ", "");
    custom_uid.toUpperCase();

    display_banner();

    if (custom_uid.length() != _rfid->uid.size * 2) {
        displayError("Invalid UID.", true);
        set_state(READ_MODE);
        return;
    }

    _rfid->printableUID.uid = "";
    for (size_t i = 0; i < custom_uid.length(); i += 2) {
        _rfid->uid.uidByte[i / 2] = strtoul(custom_uid.substring(i, i + 2).c_str(), NULL, 16);
        _rfid->printableUID.uid += custom_uid.substring(i, i + 2) + " ";
    }
    _rfid->printableUID.uid.trim();

    delayWithReturn(200);
    set_state(CLONE_MODE);
}

void TagOMatic::erase_card() {
    int result = _rfid->erase();

    switch (result) {
        case RFIDInterface::TAG_NOT_PRESENT: return; break;
        case RFIDInterface::SUCCESS: displaySuccess("Tag erased successfully."); break;
        default: displayError("Error erasing data from tag."); break;
    }

    delayWithReturn(1000);
    set_state(READ_MODE);
}

void TagOMatic::write_data() {
    int result = -1;
    if (_rfid->printableUID.picc_type != "FeliCa") {
        result = _rfid->write();
    } else {
        result = _rfid->write(1);
    }

    switch (result) {
        case RFIDInterface::TAG_NOT_PRESENT: return; break;
        case RFIDInterface::TAG_NOT_MATCH: displayError("Tag types do not match."); break;
        case RFIDInterface::SUCCESS: displaySuccess("Tag written successfully."); break;
        default: displayError("Error writing data to tag."); break;
    }

    delayWithReturn(1000);
    set_state(READ_MODE);
}

void TagOMatic::write_ndef_data() {
    if (!_ndef_created) {
        create_ndef_message();
        display_banner();
        dump_ndef_details();
    }

    int result = _rfid->write_ndef();

    switch (result) {
        case RFIDInterface::TAG_NOT_PRESENT: return; break;
        case RFIDInterface::TAG_NOT_MATCH: displayError("Tag is not MIFARE Ultralight."); break;
        case RFIDInterface::SUCCESS: displaySuccess("Tag written successfully."); break;
        default: displayError("Error writing data to tag."); break;
    }

    delayWithReturn(1000);
    set_state(READ_MODE);
}

void TagOMatic::create_ndef_message() {
    options = {
        {"Text", [this]() { create_ndef_text(); }},
        {"URL",  [this]() { create_ndef_url(); } },
    };

    loopOptions(options);
}

void TagOMatic::create_ndef_text() {
    _rfid->ndefMessage.payloadType = RFIDInterface::NDEF_TEXT;
    byte uic = 0;
    byte i;

    _rfid->ndefMessage.payload[0] = 0x02; // language size
    _rfid->ndefMessage.payload[1] = 0x65; // "en" language
    _rfid->ndefMessage.payload[2] = 0x6E;

    String ndef_data = keyboard("", NDEF_DATA_SIZE, "NDEF data:");

    for (i = 0; i < ndef_data.length(); i++) { _rfid->ndefMessage.payload[i + 3] = ndef_data.charAt(i); }
    _rfid->ndefMessage.payloadSize = i + 3;
    _rfid->ndefMessage.messageSize = _rfid->ndefMessage.payloadSize + 4;

    _ndef_created = true;
}

void TagOMatic::create_ndef_url() {
    _rfid->ndefMessage.payloadType = RFIDInterface::NDEF_URI;
    byte uic = 0;
    String prefix = "";
    byte i;

    options = {
        {"http://www.",
         [&]() {
             uic = 1;
             prefix = "http://www.";
         }                },
        {"https://www.",
         [&]() {
             uic = 2;
             prefix = "https://www.";
         }                },
        {"http://",
         [&]() {
             uic = 3;
             prefix = "http://";
         }                },
        {"https://",
         [&]() {
             uic = 4;
             prefix = "https://";
         }                },
        {"tel:",
         [&]() {
             uic = 5;
             prefix = "tel:";
         }                },
        {"mailto:",
         [&]() {
             uic = 6;
             prefix = "mailto:";
         }                },
        {"None",         [&]() {
             uic = 0;
             prefix = "None";
         }},
    };

    loopOptions(options);

    _rfid->ndefMessage.payload[0] = uic;

    String ndef_data = keyboard(prefix, NDEF_DATA_SIZE, "NDEF data:");
    ndef_data = ndef_data.substring(prefix.length());

    for (i = 0; i < ndef_data.length(); i++) { _rfid->ndefMessage.payload[i + 1] = ndef_data.charAt(i); }
    _rfid->ndefMessage.payloadSize = i + 1;
    _rfid->ndefMessage.messageSize = _rfid->ndefMessage.payloadSize + 4;

    _ndef_created = true;
}

void TagOMatic::load_file() {
    display_banner();

    int result = _rfid->load();

    if (result == RFIDInterface::SUCCESS) {
        displaySuccess("File loaded.");
        delay(500);
        _read_uid = true;

        options = {
            {"Clone UID",  [this]() { set_state(CLONE_MODE); }},
            {"Write data", [this]() { set_state(WRITE_MODE); }},
            {"Check tag",  [this]() { set_state(CHECK_MODE); }},
        };

        loopOptions(options);
    } else {
        displayError("Error loading file.", true);
        set_state(READ_MODE);
    }
}

void TagOMatic::save_file() {
    String uid_str = _rfid->printableUID.uid;
    uid_str.replace(" ", "");
    String filename = keyboard(uid_str, 30, "File name:");

    display_banner();

    int result = _rfid->save(filename);

    if (result == RFIDInterface::SUCCESS) {
        displaySuccess("File saved.");
    } else {
        displayError("Error writing file.");
    }
    delayWithReturn(1000);
    set_state(READ_MODE);
}

void TagOMatic::save_scan_result() {
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
    for (String uid : _scanned_tags) { file.println(uid); }

    file.close();
    return;
}

void TagOMatic::delayWithReturn(uint32_t ms) {
    auto tm = millis();
    while (millis() - tm < ms && !returnToMenu) { vTaskDelay(pdMS_TO_TICKS(50)); }
}
