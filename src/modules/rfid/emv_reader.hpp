#ifndef EMV_READER_H
#define EMV_READER_H

#include "PN532.h"
#include <Arduino.h>

typedef enum emv_vendor {
    EMV_VISA,
    EMV_MASTERCARD,
    EMV_UNKNOWN
} EMV_Vendor;

typedef struct EMVCard {
    bool parsed = true;
    uint8_t *aid = nullptr;
    EMV_Vendor vendor = EMV_UNKNOWN;
    size_t pan_len = 0;
    uint8_t *pan = nullptr;
   // uint8_t *afl_raw = nullptr;
    uint8_t *validfrom = nullptr;
    uint8_t *validto = nullptr;
} EMVCard;

typedef struct EMVAID {
    uint8_t aid[7];
    const char *name;
    EMV_Vendor vendor;
} EMVAID;

#define AID_DICT_SIZE 11

// http://hartleyenterprises.com/listAID.html
const EMVAID known_aid[AID_DICT_SIZE] = {
    // MasterCard family
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x04, 0x10, 0x10}, .name = "MasterCard", .vendor = EMV_MASTERCARD },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x04, 0x22, 0x03}, .name = "U.S Maestro", .vendor = EMV_MASTERCARD},
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x04, 0x30, 0x60}, .name = "Maestro", .vendor = EMV_MASTERCARD    },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x04, 0x60, 0x00}, .name = "Cirrus", .vendor = EMV_MASTERCARD     },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x04, 0x99, 0x99}, .name = "MasterCard", .vendor = EMV_MASTERCARD },

    // Visa family
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10}, .name = "Visa", .vendor = EMV_VISA       },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x20, 0x10}, .name = "Electron", .vendor = EMV_VISA   },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x20, 0x20}, .name = "V-Pay", .vendor = EMV_VISA      },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x30, 0x10}, .name = "Visa", .vendor = EMV_VISA       },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x80, 0x10}, .name = "Visa", .vendor = EMV_VISA       },
    {.aid = {0xA0, 0x00, 0x00, 0x00, 0x98, 0x08, 0x40}, .name = "Visa", .vendor = EMV_VISA       }
};

class EMVReader {
private:

    // EMV methods created with the help of https://werner.rothschopf.net/201703_arduino_esp8266_nfc.htm
    void parse_pan(std::vector<uint8_t> *afl_content, EMVCard *card);
    void parse_validfrom(std::vector<uint8_t> *afl_content, EMVCard *card);
    void parse_validto(std::vector<uint8_t> *afl_content, EMVCard *card);
    void read_afl(EMVCard *card, std::vector<uint8_t> *afl);

    // EMV methods created with the help of https://werner.rothschopf.net/201703_arduino_esp8266_nfc.htm
    std::vector<uint8_t> emv_ask_for_aid();
    std::vector<uint8_t> emv_ask_for_app_name();
    std::vector<uint8_t> emv_ask_for_pdol(std::vector<uint8_t> *aid);
    std::vector<uint8_t> emv_get_processing_options_no_pdol();

    // VISA save the card details in tag 57 (Track 2 Equivalent Data) so we need a different read method
    void emv_read_visa(std::vector<uint8_t> *pdol_data, EMVCard *card);
    std::vector<uint8_t> emv_read_record(uint8_t p1, uint8_t p2);

    PN532 *_rfid;
    Adafruit_PN532 *nfc = nullptr;
    bool _cancelled = false;
    EMVCard read_emv_card();
    void display_emv(EMVCard card);
    void save_emv(const char *aid, const char *pan, const char *validfrom, const char *validto);

public:
    EMVReader() { setup(); };
    ~EMVReader() {};
    void setup();
};

#endif
