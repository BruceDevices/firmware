#ifndef LITE_VERSION
#include "emv_reader.hpp"
#include "BerTlv.h"
#include "core/display.h"
#include <globals.h>

void EMVReader::setup() {
    _cancelled = false;
    switch (bruceConfigPins.rfidModule) {
        case PN532_I2C_MODULE: _rfid = new PN532(PN532::CONNECTION_TYPE::I2C); break;
#ifdef M5STICK
        case PN532_I2C_SPI_MODULE: _rfid = new PN532(PN532::CONNECTION_TYPE::I2C_SPI); break;
#endif
        case PN532_SPI_MODULE: _rfid = new PN532(PN532::CONNECTION_TYPE::SPI); break;
        default: {
            Serial.println("EMVReader: Unsupported RFID module for EMV reading.");
            return;
        }
    }

    _rfid->begin();
    nfc = &(_rfid->nfc);

    displayInfo("Waiting for EMV card...");
    EMVCard card = read_emv_card();
    if (_cancelled) return;
    display_emv(card);

    free(card.pan);
    free(card.validfrom);
    free(card.validto);
    free(card.aid);
}

void EMVReader::parse_pan(std::vector<uint8_t> *afl_content, EMVCard *card) {
    auto pos = find(afl_content->begin(), afl_content->end(), 0x5A);
    uint8_t len = *(pos + 1);
    uint8_t pan_begin = distance(afl_content->begin(), pos) + 2;
    card->pan = (uint8_t *)malloc(len);
    memcpy(card->pan, &afl_content->data()[pan_begin], len);
    card->pan_len = len;
}

void EMVReader::parse_validfrom(std::vector<uint8_t> *afl_content, EMVCard *card) {
    bool found = false;
    for (size_t i = 0; i < afl_content->size() && !found; i++) {
        if (afl_content->at(i) == 0x5F && afl_content->at(i + 1) == 0x25) {
            size_t begin = i + 3; // The format is 0x5F 0x25 SIZE DATA so skip 3 bytes
            card->validfrom = (uint8_t *)malloc(2);
            // The format in card is YEAR/MONTH but I want MONTH/YEAR since is the standard format
            card->validfrom[0] = afl_content->at(begin + 1);
            card->validfrom[1] = afl_content->at(begin);
            found = true;
        }
    }
}

void EMVReader::parse_validto(std::vector<uint8_t> *afl_content, EMVCard *card) {
    bool found = false;
    for (size_t i = 0; i < afl_content->size() && !found; i++) {
        if (afl_content->at(i) == 0x5F && afl_content->at(i + 1) == 0x24) {
            size_t begin = i + 3; // The format is 0x5F 0x24 SIZE DATA so skip 3 bytes
            card->validto = (uint8_t *)malloc(2);
            // The format in card is YEAR/MONTH but I want MONTH/YEAR since is the standard format
            card->validto[0] = afl_content->at(begin + 1);
            card->validto[1] = afl_content->at(begin);
            found = true;
        }
    }
}

std::vector<uint8_t> EMVReader::emv_ask_for_aid() {
    uint8_t uid[7];
    uint8_t len;
    uint8_t response[240];
    uint8_t response_len = 0;
    std::vector<uint8_t> aid;
    while (!_cancelled) {
        if (check(EscPress)) {
            _cancelled = true;
            break;
        }

        // Short timeout allows us to poll for EscPress while waiting for a card
        if (nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 100)) {
            /* Select Application */
            uint8_t ask_for_aid_apdu[] = {0x00, 0xA4, 0x04, 0x00, 0x0e, 0x32, 0x50, 0x41, 0x59, 0x2e,
                                          0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
            if (nfc->EMVinDataExchange(ask_for_aid_apdu, sizeof(ask_for_aid_apdu), response, &response_len)) {
                std::vector<uint8_t> response_vector(&response[0], &response[response_len]);
                BerTlv Tlv;
                Tlv.SetTlv(response_vector);
                if (Tlv.GetValue("4F", &aid) != OK) { // Application ID
                    Serial.println("Can't get aidFeliCa");
                    aid.clear();
                }
                Serial.println("Success AID");
            }
            break;
        }

        delay(50);
    }
    return aid;
}

std::vector<uint8_t> EMVReader::emv_ask_for_app_name() {
    uint8_t uid[7];
    uint8_t len;
    uint8_t response[240];
    uint8_t response_len = 0;
    std::vector<uint8_t> app_name;
    uint8_t ask_for_aid_apdu[] = {0x00, 0xA4, 0x04, 0x00, 0x0e, 0x32, 0x50, 0x41, 0x59, 0x2e,
                                  0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
    if (nfc->EMVinDataExchange(ask_for_aid_apdu, sizeof(ask_for_aid_apdu), response, &response_len)) {
        std::vector<uint8_t> response_vector(&response[0], &response[response_len]);
        BerTlv Tlv;
        Tlv.SetTlv(response_vector);
        if (Tlv.GetValue("50", &app_name) != OK) { // Card name
            Serial.println("Can't get app name");
            app_name.clear();
        }
        Serial.println("Success app_name");
    }
    return app_name;
}

std::vector<uint8_t> EMVReader::emv_ask_for_pdol(std::vector<uint8_t> *aid) {
    uint8_t uid[7];
    uint8_t len;
    uint8_t response[240];
    uint8_t response_len = 0;
    std::vector<uint8_t> pdol;
    /* ------------------- AID -----------------*/
    uint8_t ask_for_pdol[] = {0x00, 0xa4, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00};
    memcpy(ask_for_pdol + 5, aid->data(), 7);

    if (nfc->EMVinDataExchange(ask_for_pdol, sizeof(ask_for_pdol), response, &response_len)) {
        std::vector<uint8_t> response_vector(&response[0], &response[response_len]);

        BerTlv Tlv;
        Tlv.SetTlv(response_vector);
        if (Tlv.GetValue("9F38", &pdol) != OK) { // PDOL(Some card doesn't have it)
            Serial.println("Can't get PDOL");
            pdol.clear();
        }
    }
    return pdol;
}

void EMVReader::emv_read_visa(std::vector<uint8_t> *pdol_data, EMVCard *card) {
    uint8_t response[240];
    uint8_t response_len = 0;

    uint8_t payload[] = {
        // --- HEADER ---
        0x80,
        0xA8,
        0x00,
        0x00, // CLA, INS, P1, P2

        0x23, // Len: 35 bytes (0x23 Hex)

        // --- DATA FIELD ---
        0x83,
        0x21, // Payload len

        // Payload
        0x20,
        0x00,
        0x00,
        0x00, // 9F66 (TTQ - Visa Standard)
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, // 9F02 (Amount 0)
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, // 9F03 (Amount Other 0)
        0x03,
        0x80, // 9F1A (Country: Italy)
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, // 95   (TVR: No errors)
        0x09,
        0x78, // 5F2A (Currency: Euro)
        0x25,
        0x11,
        0x25, // 9A   (Date: 25 Nov 25)
        0x00, // 9C   (Tx Type: Purchase)
        0x12,
        0x34,
        0x56,
        0x78, // 9F37 (Unpredictable Num)

        0x00 // Len of response expected by the card(0 means all)
    };

    if (nfc->EMVinDataExchange(payload, sizeof(payload), response, &response_len)) {
        std::vector<uint8_t> response_vector(&response[0], &response[response_len]);

        BerTlv Tlv;
        Tlv.SetTlv(response_vector);
        std::vector<uint8_t> container;
        if (Tlv.GetValue("57", &container) != OK) {
            Serial.println("Can't get Track 2 Equivalent Data(PAN for VISA)");
        } else {
            Serial.println("PAN found in Track 2 Equivalent Data");
            card->pan = (uint8_t *)malloc(8);
            memcpy(card->pan, container.data(), 8 * sizeof(uint8_t)); // Copy data from TLV to struct
            card->pan_len = 8;

            // Index 8 is separator 'D' and first digit of ValidTo month
            // Index 9 is second digit of ValidTo month and first digit of ValidTo year
            // Index 10 is second digit of ValidTo year and first digit of Service Code
            card->validto = (uint8_t *)malloc(2);
            card->validto[0] = ((container[9] & 0x0F) << 4) + ((container[10] & 0xF0) >> 4);
            card->validto[1] = ((container[8] & 0x0F) << 4) + ((container[9] & 0xF0) >> 4);

            container.clear();
        }
    }
}

std::vector<uint8_t> EMVReader::emv_get_processing_options_no_pdol() {
    uint8_t response[240];
    uint8_t response_len = 0;
    std::vector<uint8_t> afl;
    uint8_t ask_for_afl[] = {0x80, 0xa8, 0x00, 0x00, 0x02, 0x83, 0x00, 0x00}; // Get AFL

    if (nfc->EMVinDataExchange(ask_for_afl, sizeof(ask_for_afl), response, &response_len)) {
        std::vector<uint8_t> response_vector(&response[0], &response[response_len]);
        BerTlv Tlv;
        Tlv.SetTlv(response_vector);
        if (Tlv.GetValue("94", &afl) != OK) { // AFL
            Serial.println("Can't get AFL");
            afl.clear();
        }
    }
    return afl;
}

std::vector<uint8_t> EMVReader::emv_read_record(uint8_t p1, uint8_t p2) {
    uint8_t response[240];
    uint8_t response_len = 0;
    std::vector<uint8_t> result;

    uint8_t read_afl[] = {0x00, 0xB2, p1, p2, 0x00};

    if (nfc->EMVinDataExchange(read_afl, sizeof(read_afl), response, &response_len)) {
        result = std::vector<uint8_t>(&response[0], &response[response_len]);
    }
    return result;
}

void EMVReader::read_afl(EMVCard *card, std::vector<uint8_t> *afl) {
    for (size_t i = 0; i < afl->size(); i += 4) {
        uint8_t sfi = (afl->at(i) >> 3); // Get SFI from AFL entry
        uint8_t record_start = afl->at(i + 1);
        uint8_t record_end = afl->at(i + 2);

        std::vector<uint8_t> afl_content = emv_read_record(record_start, (sfi << 3) | 0b00000100);
        if (!afl_content.empty()) {
            BerTlv Tlv;
            Tlv.SetTlv(afl_content);
            std::vector<uint8_t> container;
            if (Tlv.GetValue("5A", &container) !=
                OK) { // Get PAN(Credit Card Number). If TLV is corrupted(happens often with PN532) fallback
                      // to a workaround to find information
                parse_pan(&afl_content, card);
                parse_validfrom(&afl_content, card);
                parse_validto(&afl_content, card);
                Serial.println("PAN parsed with workaround");
                return;
            } else {
                memcpy(card->pan, container.data(), container.size()); // Copy data from TLV to struct
                container.clear();
                if (Tlv.GetValue("5F25", &container) != OK) { // Get ValidFrom date
                    parse_validfrom(&afl_content, card);
                    parse_validto(&afl_content, card);
                } else {
                    memcpy(card->validfrom, container.data(), container.size());
                    if (Tlv.GetValue("5F24", &container) != OK) { // Get ValidTo date
                        parse_validto(&afl_content, card);
                    } else {
                        memcpy(card->validto, container.data(), container.size());
                    }
                }
                Serial.println("PAN parsed without workaround");
                return;
            }

            if (Tlv.GetValue("5A", &container) == OK) { // Get PAN(Credit Card Number)
                card->pan = (uint8_t *)malloc(container.size());
                memcpy(card->pan, container.data(), container.size()); // Copy data from TLV to struct
                card->pan_len = container.size();
                Serial.println("PAN parsed without workaround");
                return;
            }
        } else {
            Serial.println("Can't parse AFL data for P2");
        }
    }
}

bool is_visa(EMVCard *card) {
    for (size_t i = 0; i < AID_DICT_SIZE; i++) {
        if (memcmp(card->aid, known_aid[i].aid, 7) == 0) {
            if (known_aid[i].vendor == EMV_VISA) return true;
            else return false;
        }
    }

    return false;
}

EMVCard EMVReader::read_emv_card() {
    EMVCard res;
    if (_cancelled) return res;

    std::vector<uint8_t> aid = emv_ask_for_aid(); // Perform Application Selection
    if (_cancelled) return res;

    if (aid.empty()) { // If we can't get AID, we can't read the card
        res.parsed = false;
        Serial.println("Can't read card");
    } else {
        // Copy AID to result card
        res.aid = (uint8_t *)malloc(aid.size());
        memcpy(res.aid, aid.data(), aid.size());

        // Initialize Application Process
        std::vector<uint8_t> pdol = emv_ask_for_pdol(&aid); // Check if card require PDOL(for example, VISA)

        if (pdol.empty()) { // No PDOL(for example Mastercard)
            std::vector<uint8_t> afl = emv_get_processing_options_no_pdol(); // Try to get AFL without PDOL

            if (!afl.empty()) {
                // Read Application data
                read_afl(&res, &afl); // Read AFL
            } else {
                Serial.println("Can't get AFL ID");
            }
        } else {
            if (is_visa(&res)) {
                Serial.println("VISA card detected");
                emv_read_visa(&pdol, &res);

            } else {
                Serial.println("Non-VISA card with PDOL detected, not supported yet");
                // std::vector<uint8_t> afl = emv_get_processing_options(&pdol);

                // std::vector<uint8_t> afl = emv_get_processing_options_no_pdol();
                // if (!afl.empty()) {
                //     read_afl(&res, &afl);
                //     Serial.println("Got AFL with PDOL");
                // } else {
                //     Serial.println("Can't get AFL with PDOL");
                // }
            }
        }
    }

    Serial.println("EMV Read complete");
    return res;
}

// From https://github.com/huckor/BER-TLV
std::string BinToAscii(uint8_t *BinData, size_t size)

{
    char AsciiHexNo[5];
    std::string Return;
    for (int i = 0; i < size; i++) {
        sprintf(AsciiHexNo, "%02X", BinData[i]);
        Return += AsciiHexNo;
    }

    return Return;
}

void EMVReader::display_emv(EMVCard card) {
    drawMainBorderWithTitle("Read EMV Card");
    std::string aid;
    std::string pan;
    std::string issuedate;
    std::string validto;

    if (card.parsed) {
        bool found = false;
        for (size_t i = 0; i < AID_DICT_SIZE && !found; i++) {
            if (memcmp(card.aid, known_aid[i].aid, 7) == 0) {
                found = true;
                aid = known_aid[i].name;
                padprintln(known_aid[i].name);
                break;
            }
        }

        if (!found) { padprintln("Unknown card vendor"); }
        if (card.pan != nullptr) {
            pan = BinToAscii(card.pan, card.pan_len);
            // Add some spacing
            size_t pad = 0;
            for (size_t i = 0; i < pan.size(); i++) {
                if (i % 4 == 0 && i != 0) { pan.insert(pan.begin() + i + (pad++), ' '); }
            }

            padprintln(pan.c_str());
        } else {
            padprintln("Unknown PAN");
        }
        if (card.validfrom != nullptr) {
            issuedate = BinToAscii(card.validfrom, 2);
            issuedate.insert(issuedate.begin() + 2, '/');
            padprintln(issuedate.c_str());
        } else {
            padprintln("Unknown issue date");
        }

        if (card.validto != nullptr) {
            validto = BinToAscii(card.validto, 2);
            validto.insert(validto.begin() + 2, '/');
            padprintln(validto.c_str());
        } else {
            padprintln("Unknown valid to date");
        }
    } else {
        padprintln("Failed to read EMV Card.");
    }

    padprintln("Press any key to continue...");

    while (!AnyKeyPress) { delay(100); }

    options = {};
    options.emplace_back("Save", [this, aid, pan, issuedate, validto]() {
        this->save_emv(aid.c_str(), pan.c_str(), issuedate.c_str(), validto.c_str());
    });
    options.emplace_back("Exit", [this]() { return; });

    loopOptions(options);
}

void EMVReader::save_emv(const char *aid, const char *pan, const char *validfrom, const char *validto) {
    FS *fs;
    if (!getFsStorage(fs)) return;

    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    if (!(*fs).exists("/BruceRFID/Scans")) (*fs).mkdir("/BruceRFID/Scans");

    String filename = "emv_";
    String pan_dashed = String(pan);
    pan_dashed.replace(" ", "_");
    filename += pan_dashed;
    filename += ".txt";

    File file = (*fs).open("/BruceRFID/Scans/" + filename, FILE_WRITE);

    if (!file) {
        displayError("Error opening file.");
        return;
    }

    file.println("Vendor: " + String(aid));
    file.println("Credit Card Number: " + String(pan));
    file.println("Valid From: " + String(validfrom));
    file.println("Valid To: " + String(validto));

    file.close();
    displaySuccess("EMV data saved.");
}
#endif
