#include "apdu.h"
#include <string>
#include <vector>

const uint8_t ApduCommand::C_APDU_CLA = 0;
const uint8_t ApduCommand::C_APDU_INS = 1;
const uint8_t ApduCommand::C_APDU_P1 = 2;
const uint8_t ApduCommand::C_APDU_P2 = 3;
const uint8_t ApduCommand::C_APDU_LC = 4;
const uint8_t ApduCommand::C_APDU_DATA = 5;
const uint8_t ApduCommand::C_APDU_P1_SELECT_BY_ID = 0x00;
const uint8_t ApduCommand::C_APDU_P1_SELECT_BY_NAME = 0x04;

const uint8_t ApduCommand::R_APDU_SW1_COMMAND_COMPLETE = 0x90;
const uint8_t ApduCommand::R_APDU_SW2_COMMAND_COMPLETE = 0x00;
const uint8_t ApduCommand::R_APDU_SW1_NDEF_TAG_NOT_FOUND = 0x6A;
const uint8_t ApduCommand::R_APDU_SW2_NDEF_TAG_NOT_FOUND = 0x82;
const uint8_t ApduCommand::R_APDU_SW1_FUNCTION_NOT_SUPPORTED = 0x6A;
const uint8_t ApduCommand::R_APDU_SW2_FUNCTION_NOT_SUPPORTED = 0x81;
const uint8_t ApduCommand::R_APDU_SW1_MEMORY_FAILURE = 0x65;
const uint8_t ApduCommand::R_APDU_SW2_MEMORY_FAILURE = 0x81;
const uint8_t ApduCommand::R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES = 0x62;
const uint8_t ApduCommand::R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES = 0x82;

const uint8_t ApduCommand::ISO7816_SELECT_FILE = 0xA4;
const uint8_t ApduCommand::ISO7816_READ_BINARY = 0xB0;
const uint8_t ApduCommand::ISO7816_UPDATE_BINARY = 0xD6;

const std::vector<uint8_t> NdefCommand::APPLICATION_NAME_V2 = {
    0, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01
};
const uint8_t NdefCommand::NDEF_MAX_LENGTH = 0x64;

const uint8_t Ndef::TNF_WELL_KNOWN = 0x01;
const uint8_t Ndef::TNF_MEDIA = 0x02;
const uint8_t Ndef::RTD_URI = 0x55;

// Appends one Wi-Fi Simple Config TLV attribute (2-byte big-endian type,
// 2-byte big-endian length, value) - see the Wi-Fi Alliance WSC spec.
void appendWscAttr(std::vector<uint8_t> &out, uint16_t type, const std::vector<uint8_t> &value) {
    out.push_back(static_cast<uint8_t>((type >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(type & 0xFF));
    out.push_back(static_cast<uint8_t>((value.size() >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value.size() & 0xFF));
    out.insert(out.end(), value.begin(), value.end());
}

std::vector<uint8_t> Ndef::urlNdefAbbrv(std::string url) {
    std::vector<std::string> abbrv_table = {
        "http://www.",
        "https://www.",
        "http://",
        "https://",
        "tel:",
        "mailto:",
        "ftp://anonymous:anonymous@",
        "ftp://ftp.",
        "ftps://",
        "sftp://",
        "smb://",
        "nfs://",
        "ftp://",
        "dav://",
        "news:",
        "telnet://",
        "imap:",
        "rtsp://",
        "urn:",
        "pop:",
        "sip:",
        "sips:",
        "tftp:",
        "btspp://",
        "btl2cap://",
        "btgoep://",
        "tcpobex://",
        "irdaobex://",
        "file://",
        "urn:epc:id:",
        "urn:epc:tag:",
        "urn:epc:pat:",
        "urn:epc:raw:",
        "urn:epc:",
        "urn:nfc:"
    };

    std::vector<uint8_t> ndefMessage;

    for (size_t i = 0; i < abbrv_table.size(); ++i) {
        if (url.find(abbrv_table[i]) == 0) {
            ndefMessage.push_back(static_cast<uint8_t>(i + 1));
            url = url.substr(abbrv_table[i].length());
            break;
        }
    }

    ndefMessage.insert(ndefMessage.end(), url.begin(), url.end());
    return ndefMessage;
}

std::vector<uint8_t> Ndef::newMessage(std::vector<uint8_t> ndef) {
    std::vector<uint8_t> message;
    message.push_back(0xD1); // NDEF record header
    message.push_back(TNF_WELL_KNOWN);
    message.push_back(ndef.size());
    message.push_back(RTD_URI);
    message.insert(message.end(), ndef.begin(), ndef.end());
    return message;
}

std::vector<uint8_t> Ndef::mimeRecord(
    const std::string &mimeType, const std::vector<uint8_t> &payload, const std::vector<uint8_t> &id
) {
    bool hasId = !id.empty();
    // MB=1, ME=1, CF=0, SR=1 (payload always < 256 bytes for our callers) + IL + TNF.
    uint8_t header = 0xD0 | (hasId ? 0x08 : 0x00) | TNF_MEDIA;

    std::vector<uint8_t> record;
    record.push_back(header);
    record.push_back(static_cast<uint8_t>(mimeType.size()));
    record.push_back(static_cast<uint8_t>(payload.size()));
    if (hasId) record.push_back(static_cast<uint8_t>(id.size()));
    record.insert(record.end(), mimeType.begin(), mimeType.end());
    if (hasId) record.insert(record.end(), id.begin(), id.end());
    record.insert(record.end(), payload.begin(), payload.end());
    return record;
}

std::vector<uint8_t> Ndef::wifiCredentialPayload(const std::string &ssid, const std::string &password) {
    std::vector<uint8_t> cred;
    appendWscAttr(cred, 0x1026, {0x01});                                         // Network Index
    appendWscAttr(cred, 0x1045, std::vector<uint8_t>(ssid.begin(), ssid.end())); // SSID
    if (password.empty()) {
        appendWscAttr(cred, 0x1003, {0x00, 0x01}); // Auth Type: Open
        appendWscAttr(cred, 0x100F, {0x00, 0x01}); // Encryption Type: None
    } else {
        appendWscAttr(cred, 0x1003, {0x00, 0x22}); // Auth Type: WPA/WPA2-Personal
        appendWscAttr(cred, 0x100F, {0x00, 0x0C}); // Encryption Type: TKIP+AES
        appendWscAttr(cred, 0x1027, std::vector<uint8_t>(password.begin(), password.end())); // Network Key
    }
    appendWscAttr(cred, 0x1020, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}); // MAC Address (wildcard)

    std::vector<uint8_t> payload;
    appendWscAttr(payload, 0x100E, cred); // Credential wraps everything above
    return payload;
}
