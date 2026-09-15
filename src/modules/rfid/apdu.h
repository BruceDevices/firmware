#ifndef APUD_H
#define APUD_H

#include <cstdint>
#include <string>
#include <vector>

class ApduCommand {
public:
    static const uint8_t C_APDU_CLA;
    static const uint8_t C_APDU_INS;
    static const uint8_t C_APDU_P1;
    static const uint8_t C_APDU_P2;
    static const uint8_t C_APDU_LC;
    static const uint8_t C_APDU_DATA;
    static const uint8_t C_APDU_P1_SELECT_BY_ID;
    static const uint8_t C_APDU_P1_SELECT_BY_NAME;

    static const uint8_t R_APDU_SW1_COMMAND_COMPLETE;
    static const uint8_t R_APDU_SW2_COMMAND_COMPLETE;
    static const uint8_t R_APDU_SW1_NDEF_TAG_NOT_FOUND;
    static const uint8_t R_APDU_SW2_NDEF_TAG_NOT_FOUND;
    static const uint8_t R_APDU_SW1_FUNCTION_NOT_SUPPORTED;
    static const uint8_t R_APDU_SW2_FUNCTION_NOT_SUPPORTED;
    static const uint8_t R_APDU_SW1_MEMORY_FAILURE;
    static const uint8_t R_APDU_SW2_MEMORY_FAILURE;
    static const uint8_t R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES;
    static const uint8_t R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES;

    static const uint8_t ISO7816_SELECT_FILE;
    static const uint8_t ISO7816_READ_BINARY;
    static const uint8_t ISO7816_UPDATE_BINARY;
};

class NdefCommand {
public:
    static const std::vector<uint8_t> APPLICATION_NAME_V2;
    static const uint8_t NDEF_MAX_LENGTH;
};

class Ndef {
public:
    static std::vector<uint8_t> urlNdefAbbrv(std::string url);
    static std::vector<uint8_t> newMessage(std::vector<uint8_t> ndef);
    static const uint8_t TNF_WELL_KNOWN;
    static const uint8_t TNF_MEDIA;
    static const uint8_t RTD_URI;
    static std::vector<uint8_t> mimeRecord(
        const std::string &mimeType, const std::vector<uint8_t> &payload, const std::vector<uint8_t> &id = {}
    );
    static std::vector<uint8_t> wifiCredentialPayload(const std::string &ssid, const std::string &password);
};

enum class TagFile : int { NONE = 0, CC = 1, NDEF = 2 };

#endif // APUD_H
