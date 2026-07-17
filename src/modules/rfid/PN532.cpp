/**
 * @file PN532.cpp
 * @author Rennan Cockles (https://github.com/rennancockles) // Dawi (https://github.com/yourdawi)
 * @brief Read, Write and Emulate RFID tags using PN532 module
 * @version 0.2
 * @date 2026-02-22
 */

#include "PN532.h"
#include "apdu.h"
#include "core/bus_HAL.h"
#include "core/display.h"
#include "core/i2c_finder.h"
#include "core/sd_functions.h"
#include "core/type_convertion.h"
#include <algorithm>
#include <vector>

#ifndef GPIO_NUM_25
#define GPIO_NUM_25 25
#endif

namespace {
bool waitReadyPreferIrq(Adafruit_PN532 &nfc, uint16_t timeoutMs) {
    if (nfc._irq >= 0) {
        uint32_t start = millis();
        while (digitalRead(nfc._irq) != LOW) {
            if (timeoutMs != 0 && (millis() - start) > timeoutMs) return false;
            delay(1);
            yield();
        }
        return true;
    }
    return nfc.waitready(timeoutMs);
}

bool sendCommandCheckAckPreferIrq(Adafruit_PN532 &nfc, uint8_t *cmd, uint8_t cmdLen, uint16_t timeoutMs) {
    nfc.writecommand(cmd, cmdLen);
    delay(1);
    if (!waitReadyPreferIrq(nfc, timeoutMs)) return false;
    if (!nfc.readack()) return false;
    delay(1);
    if (!waitReadyPreferIrq(nfc, timeoutMs)) return false;
    return true;
}

// Identity broadcast by TgInitAsTarget. The PN532 always advertises
// ISO14443A and FeliCa params in a single call; the reader picks whichever
// it polls for. NFCID1t is only 3 bytes - the chip always prefixes the
// emulated single-size NFCID1 with a fixed 0x08 byte, so a byte-perfect
// 4-byte custom UID isn't achievable, only the low 3 bytes are customizable.
struct TargetIdentity {
    // MODE bitfield: 0x00 broadcasts ISO14443A and FeliCa together. 0x05
    // ("PICC only, Passive only") reliably suppresses the FeliCa auto-answer
    // for ISO14443A emulation; readers otherwise tend to lock onto the
    // FeliCa identity even when it's just a leftover default.
    uint8_t mode = 0x00;
    uint8_t sensRes[2] = {0x08, 0x00};                                     // ATQA
    uint8_t nfcid1t[3] = {0xDC, 0x44, 0x20};                               // low 3 bytes of NFCID1
    uint8_t selRes = 0x60;                                                 // SAK
    uint8_t nfcid2t[8] = {0x01, 0xFE, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7}; // FeliCa IDm
    uint8_t pad[8] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7};     // FeliCa PMm
    uint8_t sysCode[2] = {0xFF, 0xFF};                                     // FeliCa system code
};

bool tgInitAsTargetIrq(Adafruit_PN532 &nfc, const TargetIdentity &id) {
    // Matches the older "AsTarget" payload
    uint8_t target[] = {
        PN532_COMMAND_TGINITASTARGET,
        id.mode, // MODE bitfield
        id.sensRes[0],
        id.sensRes[1],
        id.nfcid1t[0],
        id.nfcid1t[1],
        id.nfcid1t[2],
        id.selRes,
        id.nfcid2t[0],
        id.nfcid2t[1],
        id.nfcid2t[2],
        id.nfcid2t[3],
        id.nfcid2t[4],
        id.nfcid2t[5],
        id.nfcid2t[6],
        id.nfcid2t[7],
        id.pad[0],
        id.pad[1],
        id.pad[2],
        id.pad[3],
        id.pad[4],
        id.pad[5],
        id.pad[6],
        id.pad[7],
        id.sysCode[0],
        id.sysCode[1],
        0xAA,
        0x99,
        0x88,
        0x77,
        0x66,
        0x55,
        0x44,
        0x33,
        0x22,
        0x11,
        0x01,
        0x00, // NFCID3t tail
        0x0D, // historical bytes length
        0x52,
        0x46,
        0x49,
        0x44,
        0x49,
        0x4F,
        0x74,
        0x20,
        0x50,
        0x4E,
        0x35,
        0x33,
        0x32
    };

    if (!sendCommandCheckAckPreferIrq(nfc, target, sizeof(target), 1500)) return false;

    // Adafruit AsTarget() reads only 8 bytes here. Reading more can stall on ESP32 I2C.
    uint8_t frame[8] = {0};
    nfc.readdata(frame, sizeof(frame));
    // Accept the salmg variant (0x15 at index 6) and standard response framing (0x8D at index 6).
    if (frame[6] == 0x15) return true;
    if (frame[6] == (PN532_COMMAND_TGINITASTARGET + 1)) {
        uint8_t status = frame[7];
        return status == 0x00 || status == 0x08 || status == 0x15;
    }
    return false;
}

bool tgGetDataIrq(Adafruit_PN532 &nfc, uint8_t *out, uint8_t maxLen, uint8_t *outLen, uint8_t *status) {
    uint8_t cmd = PN532_COMMAND_TGGETDATA;
    if (!sendCommandCheckAckPreferIrq(nfc, &cmd, 1, 1000)) return false;

    // 64 bytes matches Adafruit's getDataTarget() and avoids over-reading on ESP32 I2C.
    uint8_t frame[64] = {0};
    nfc.readdata(frame, sizeof(frame));
    if (frame[6] != (PN532_COMMAND_TGGETDATA + 1)) return false;

    *status = frame[7];
    uint8_t dataLen = frame[3] > 3 ? static_cast<uint8_t>(frame[3] - 3) : 0;
    uint8_t copyLen = std::min<uint8_t>(dataLen, maxLen);
    if (copyLen > 0) memcpy(out, frame + 8, copyLen);
    *outLen = copyLen;
    return true;
}

bool tgSetDataIrq(Adafruit_PN532 &nfc, const uint8_t *data, uint8_t dataLen) {
    if (dataLen == 0 || dataLen > 254) return false;

    uint8_t cmd[255] = {0};
    cmd[0] = PN532_COMMAND_TGSETDATA;
    memcpy(cmd + 1, data, dataLen);
    if (!sendCommandCheckAckPreferIrq(nfc, cmd, static_cast<uint8_t>(dataLen + 1), 1000)) return false;

    // setDataTarget() in the Adafruit implementation also reads 8 bytes.
    uint8_t frame[8] = {0};
    nfc.readdata(frame, sizeof(frame));
    if (frame[6] != (PN532_COMMAND_TGSETDATA + 1)) return false;
    return frame[7] == 0x00;
}

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

bool parseHexBytesAfterColon(const String &line, std::vector<uint8_t> &bytes) {
    bytes.clear();
    int colon = line.indexOf(':');
    if (colon < 0) return false;

    int hi = -1;
    for (int i = colon + 1; i < line.length(); i++) {
        int v = hexNibble(line.charAt(i));
        if (v < 0) continue;
        if (hi < 0) {
            hi = v;
        } else {
            bytes.push_back(static_cast<uint8_t>((hi << 4) | v));
            hi = -1;
        }
    }
    return !bytes.empty() && hi < 0;
}

bool parseHexByteString(const String &s, uint8_t *out, uint8_t len) {
    int hi = -1;
    uint8_t count = 0;
    for (int i = 0; i < s.length() && count < len; i++) {
        int v = hexNibble(s.charAt(i));
        if (v < 0) continue;
        if (hi < 0) {
            hi = v;
        } else {
            out[count++] = static_cast<uint8_t>((hi << 4) | v);
            hi = -1;
        }
    }
    return count == len;
}

// Derives the identity to broadcast via TgInitAsTarget from whatever is
// currently loaded (uid/printableUID). Falls back to the legacy hardcoded
// demo identity when nothing usable is loaded, to avoid regressing the
// no-file-loaded "quick test" emulation flow.
// serveNdefOverT4T: when true, the caller intends to serve the loaded content
// as an NFC Forum Type 4 Tag (NDEF over ISO-DEP APDUs). The PN532 in target
// mode only delivers ISO14443-4 (RATS/ISO-DEP) and NFC-DEP commands to the
// host, so a loaded card's real Type 2 SAK (e.g. NTAG21x) is overridden with
// the ISO14443-4 bit to make the reader do RATS and read NDEF over Type 4.
TargetIdentity buildTargetIdentity(
    const RFIDInterface::Uid &uid, const RFIDInterface::PrintableUID &printableUID, bool serveNdefOverT4T
) {
    TargetIdentity id;

    if (printableUID.picc_type == "FeliCa") {
        // format_data_felica()/parse_data() store IDm in uid.uidByte (8 bytes),
        // PMm as a hex string in printableUID.sak, and the system code as a
        // plain hex string in printableUID.atqa.
        id.mode = 0x00; // Broadcast FeliCa (no confirmed "FeliCa only" bit; keep both).
        memcpy(id.nfcid2t, uid.uidByte, 8);
        uint8_t pmm[8];
        if (parseHexByteString(printableUID.sak, pmm, 8)) memcpy(id.pad, pmm, 8);
        if (printableUID.atqa.length() > 0) {
            uint16_t sysCode = strtoul(printableUID.atqa.c_str(), NULL, 16);
            id.sysCode[0] = static_cast<uint8_t>((sysCode >> 8) & 0xFF);
            id.sysCode[1] = static_cast<uint8_t>(sysCode & 0xFF);
        }
    } else {
        // Not emulating a FeliCa dump: suppress the FeliCa auto-answer so
        // readers that poll both technologies don't lock onto a leftover
        // default FeliCa identity instead of the ISO14443A content below.
        id.mode = 0x05; // "PICC only, Passive only", per TgInitAsTarget()'s own reference call.
        if (uid.size >= 3) {
            id.sensRes[0] = uid.atqaByte[0];
            id.sensRes[1] = uid.atqaByte[1];
            // ATQA's UID-size bits (b7-b6 of the low byte) must say "single
            // size" (4 bytes): TgInitAsTarget's NFCID1t field only holds 3
            // bytes (+1 fixed by the chip), so a real card's "double size"
            // ATQA (e.g. NTAG21x's 0x0044, 7-byte UID) would be a lie we
            // can't back up - readers abort activation when the anticollision
            // cascade doesn't match what ATQA promised.
            id.sensRes[1] &= ~0x40;
            // Only the low 3 bytes of a 4-byte NFCID1 are settable (see
            // TargetIdentity comment above), so best-effort match the tail.
            id.nfcid1t[0] = uid.uidByte[uid.size - 3];
            id.nfcid1t[1] = uid.uidByte[uid.size - 2];
            id.nfcid1t[2] = uid.uidByte[uid.size - 1];
            id.selRes = uid.sak;
            if (serveNdefOverT4T) {
                // The loaded card's real SAK may be Type 2 (0x00, e.g. NTAG21x),
                // which would make the reader try raw Type 2 commands the PN532
                // can't answer in target mode. Force the ISO14443-4 bit so the
                // reader does RATS and reads our NDEF over Type 4 instead.
                id.selRes |= 0x20;
            }
        }
    }

    return id;
}

bool extractNdefMessageFromPageDump(const String &dump, std::vector<uint8_t> &ndefOut) {
    ndefOut.clear();
    if (dump.length() == 0) return false;

    std::vector<uint8_t> userData;
    std::vector<uint8_t> lineBytes;
    int pos = 0;

    while (pos < dump.length()) {
        int nl = dump.indexOf('\n', pos);
        if (nl < 0) nl = dump.length();

        String line = dump.substring(pos, nl);
        line.trim();
        pos = nl + 1;

        if (!line.startsWith("Page ")) continue;

        int colon = line.indexOf(':');
        if (colon < 0) continue;

        int page = line.substring(5, colon).toInt();
        if (page < 4) continue; // Skip UID/lock/CC area before user pages.

        if (!parseHexBytesAfterColon(line, lineBytes)) continue;
        if (lineBytes.size() < 4) continue;

        // Ultralight/NTAG dumps are 4 bytes per page. Use first 4 bytes from each page line.
        userData.insert(userData.end(), lineBytes.begin(), lineBytes.begin() + 4);
    }

    if (userData.empty()) return false;

    // Parse TLV stream and return the first NDEF TLV (0x03).
    size_t i = 0;
    while (i < userData.size()) {
        uint8_t tlv = userData[i++];

        if (tlv == 0x00) continue; // NULL TLV
        if (tlv == 0xFE) break;    // Terminator TLV

        if (i >= userData.size()) return false;

        uint32_t len = userData[i++];
        if (len == 0xFF) {
            if (i + 1 >= userData.size()) return false;
            len = (static_cast<uint32_t>(userData[i]) << 8) | userData[i + 1];
            i += 2;
        }

        if (i + len > userData.size()) return false;

        if (tlv == 0x03) {
            ndefOut.assign(userData.begin() + i, userData.begin() + i + len);
            return !ndefOut.empty();
        }

        i += len; // Skip unknown TLV payload
    }

    return false;
}

bool buildNdefMessageFromStruct(const RFIDInterface::NdefMessage &src, std::vector<uint8_t> &ndefOut) {
    ndefOut.clear();
    if (src.messageSize == 0) return false;
    if (src.payloadSize == 0) return false;

    ndefOut.reserve(4 + src.payloadSize);
    ndefOut.push_back(src.header);
    ndefOut.push_back(src.tnf);
    ndefOut.push_back(src.payloadSize);
    ndefOut.push_back(src.payloadType);
    ndefOut.insert(ndefOut.end(), src.payload, src.payload + src.payloadSize);

    return ndefOut.size() == src.messageSize;
}
} // namespace

PN532::PN532(CONNECTION_TYPE connection_type) {
    _connection_type = connection_type;
    _use_i2c = (connection_type == I2C || connection_type == I2C_SPI);
    if (connection_type == CONNECTION_TYPE::I2C)
        nfc.setInterface(bruceConfigPins.i2c_bus.sda, bruceConfigPins.i2c_bus.scl);
#ifdef M5STICK
    else if (connection_type == CONNECTION_TYPE::I2C_SPI) nfc.setInterface(GPIO_NUM_26, GPIO_NUM_25);
#endif
    else
        nfc.setInterface(
            bruceConfigPins.PN532_bus.sck,
            bruceConfigPins.PN532_bus.miso,
            bruceConfigPins.PN532_bus.mosi,
            bruceConfigPins.PN532_bus.cs
        );
}

bool PN532::begin() {
    TwoWire *Wire = nullptr;
#ifdef M5STICK
    if (_connection_type == CONNECTION_TYPE::I2C_SPI) {
        Wire = acquireI2CBus(GPIO_NUM_26, GPIO_NUM_25);
    } else if (_connection_type == CONNECTION_TYPE::I2C) {
        Wire = acquireI2CBus();
    }
#else
    Wire = acquireI2CBus();
#endif

    if (_use_i2c && Wire != &::Wire) {
        // The vendored Adafruit_PN532/Adafruit_I2CDevice libs always talk to the default global
        // `Wire` object and have no way to be pointed at another TwoWire instance. If bus_HAL had
        // to remap i2c_bus to Wire1 to avoid colliding with sys_i2c, PN532 can't work here.
        displayError("I2C bus conflicts with system I2C bus", true);
        return false;
    }

    bool i2c_check = true;
    if (_use_i2c && Wire != nullptr) {
        Wire->beginTransmission(PN532_I2C_ADDRESS);
        int error = Wire->endTransmission();
        i2c_check = (error == 0);
    }

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();

    return i2c_check || versiondata;
}

int PN532::read(int cardBaudRate) {
    pageReadStatus = FAILURE;

    bool felica = false;
    if (cardBaudRate == PN532_MIFARE_ISO14443A) {
        if (!nfc.startPassiveTargetIDDetection(cardBaudRate)) return TAG_NOT_PRESENT;
        if (!nfc.readDetectedPassiveTargetID()) return FAILURE;
        format_data();
        set_uid();
    } else {
        uint16_t sys_code = 0xFFFF; // Default sys code for FeliCa
        uint8_t req_code = 0x01;    // Default request code for FeliCa
        uint8_t idm[8];
        uint8_t pmm[8];
        uint16_t sys_code_res;
        if (!nfc.felica_Polling(sys_code, req_code, idm, pmm, &sys_code_res)) { return TAG_NOT_PRESENT; }
        format_data_felica(idm, pmm, sys_code_res);
    }

    displayInfo("Reading data blocks...");
    pageReadStatus = read_data_blocks();
    pageReadSuccess = pageReadStatus == SUCCESS;
    return SUCCESS;
}

int PN532::clone() {
    if (!nfc.startPassiveTargetIDDetection()) return TAG_NOT_PRESENT;
    if (!nfc.readDetectedPassiveTargetID()) return FAILURE;

    if (nfc.targetUid.sak != uid.sak) return TAG_NOT_MATCH;

    uint8_t data[16];
    byte bcc = 0;
    int i;
    for (i = 0; i < uid.size; i++) {
        data[i] = uid.uidByte[i];
        bcc = bcc ^ uid.uidByte[i];
    }
    data[i++] = bcc;
    data[i++] = uid.sak;
    data[i++] = uid.atqaByte[1];
    data[i++] = uid.atqaByte[0];
    byte tmp = 0;
    while (i < 16) data[i++] = 0x62 + tmp++;
    if (nfc.mifareclassic_WriteBlock0(data)) {
        return SUCCESS;
    } else {
        // Backdoor failed, try direct write
        uint8_t num = 0;
        while ((!nfc.startPassiveTargetIDDetection() || !nfc.readDetectedPassiveTargetID()) && num++ < 5) {
            displayTextLine("hold on...");
            delay(10);
        }
        uid.size = nfc.targetUid.size;
        for (uint8_t i = 0; i < uid.size; i++) uid.uidByte[i] = nfc.targetUid.uidByte[i];

        if (authenticate_mifare_classic(0) == SUCCESS && nfc.mifareclassic_WriteDataBlock(0, data)) {
            return SUCCESS;
        }
    }
    return FAILURE;
}

int PN532::erase() {
    if (!nfc.startPassiveTargetIDDetection()) return TAG_NOT_PRESENT;
    if (!nfc.readDetectedPassiveTargetID()) return FAILURE;

    return erase_data_blocks();
}

int PN532::write(int cardBaudRate) {
    if (cardBaudRate == PN532_MIFARE_ISO14443A) {
        if (!nfc.startPassiveTargetIDDetection()) return TAG_NOT_PRESENT;
        if (!nfc.readDetectedPassiveTargetID()) return FAILURE;

        if (nfc.targetUid.sak != uid.sak) return TAG_NOT_MATCH;
    } else {
        uint16_t sys_code = 0xFFFF; // Default sys code for FeliCa
        uint8_t req_code = 0x01;    // Default request code for FeliCa
        uint8_t idm[8];
        uint8_t pmm[8];
        uint16_t sys_code_res;
        if (!nfc.felica_Polling(sys_code, req_code, idm, pmm, &sys_code_res)) { return TAG_NOT_PRESENT; }
    }

    return write_data_blocks();
}

int PN532::write_ndef() {
    if (!nfc.startPassiveTargetIDDetection()) return TAG_NOT_PRESENT;
    if (!nfc.readDetectedPassiveTargetID()) return FAILURE;

    return write_ndef_blocks();
}

int PN532::emulate() {
    static constexpr uint8_t kInsSelectFile = 0xA4;
    static constexpr uint8_t kInsReadBinary = 0xB0;
    static constexpr uint8_t kInsUpdateBinary = 0xD6;
    static constexpr uint16_t kNdefMaxLen = 128;
    static constexpr uint8_t kMaxResponseData = 252; // +2 SW bytes and 1 command byte for setDataTarget.
    static const std::vector<uint8_t> kCcFile = {
        0x00, 0x0F, 0x20, 0x00, 0x3B, 0x00, 0x34, 0x04, 0x06, 0xE1, 0x04, 0x00, 0x80, 0x00, 0xFF
    };
    static const std::vector<uint8_t> kNdefAidSelectByName = {
        0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01
    };
    static const std::vector<uint8_t> kSwOk = {0x90, 0x00};
    static const std::vector<uint8_t> kSwNotFound = {0x6A, 0x82};
    static const std::vector<uint8_t> kSwNotSupported = {0x6A, 0x81};
    static const std::vector<uint8_t> kSwEof = {0x62, 0x82};

    // NDEF over ISO-DEP (NFC Forum Type 4 Tag) is the only content the PN532
    // can actually deliver in target mode - it never passes raw Type 2
    // (MIFARE Ultralight/NTAG) or FeliCa block-read commands up to the host,
    // regardless of what's loaded. For a Type 2 dump, extract its embedded
    // NDEF; otherwise use an explicitly built NDEF, falling back to a test
    // URL so a bare "quick test" emulation still has something to serve.
    std::vector<uint8_t> emulatedNdefMessage;
    bool canParseUltralightDump = (uid.sak == PICC_TYPE_MIFARE_UL);
    if ((!canParseUltralightDump || !extractNdefMessageFromPageDump(strAllPages, emulatedNdefMessage))) {
        if (!buildNdefMessageFromStruct(this->ndefMessage, emulatedNdefMessage)) {
            std::vector<uint8_t> uriPayload = Ndef::urlNdefAbbrv("https://bruce.computer");
            emulatedNdefMessage = Ndef::newMessage(uriPayload);
        }
    }
    bool hasNdefFile = !emulatedNdefMessage.empty() && emulatedNdefMessage.size() <= (kNdefMaxLen - 2);
    if (!hasNdefFile) return FAILURE;

    std::vector<uint8_t> ndefFile;
    if (hasNdefFile) {
        ndefFile.assign(kNdefMaxLen, 0x00);
        ndefFile[0] = static_cast<uint8_t>((emulatedNdefMessage.size() >> 8) & 0xFF);
        ndefFile[1] = static_cast<uint8_t>(emulatedNdefMessage.size() & 0xFF);
        memcpy(ndefFile.data() + 2, emulatedNdefMessage.data(), emulatedNdefMessage.size());
    }

    TargetIdentity targetIdentity = buildTargetIdentity(uid, printableUID, hasNdefFile);
    TagFile currentFile = TagFile::NONE;
    bool hadInteraction = false;
    bool targetArmed = false;
    uint32_t start = millis();
    uint32_t nextArmTry = 0;
    bool targetReady = false;

    if (_use_i2c) {
        // PN532 target mode is sensitive to ESP32 I2C timing/clock stretching.
        TwoWire *Wire = acquireI2CBus();
        Wire->setClock(100000);
        Wire->setTimeOut(50);
    }
    // `begin()` already wakes and SAMConfig's the PN532. Avoid reusing Adafruit's I2C RDY polling path here.

    while (millis() - start < 60000) {
        if (check(EscPress) || returnToMenu) {
            returnToMenu = true;
            break;
        }
        yield();
        if (!targetReady && millis() >= nextArmTry) {
            targetReady = tgInitAsTargetIrq(nfc, targetIdentity);
            nextArmTry = millis() + 300;
            currentFile = TagFile::NONE;
            if (targetReady) targetArmed = true;
            if (!targetReady) {
                delay(20);
                continue;
            }
        }

        uint8_t request[255] = {0};
        uint8_t request_len = 0;
        uint8_t tgStatus = 0xFF;
        if (!tgGetDataIrq(nfc, request, sizeof(request), &request_len, &tgStatus)) {
            nfc.inRelease();
            targetReady = false;
            delay(20);
            continue;
        }
        if (tgStatus == 0x29) {
            // Target genuinely released/deselected by the initiator.
            nfc.inRelease();
            targetReady = false;
            delay(20);
            continue;
        }
        if (tgStatus != 0x00 || request_len < 1) {
            // 0x25 (and other non-zero statuses) show up continuously while
            // idle/not-yet-selected - NOT a release condition. Releasing here
            // was tearing the link down mid-anticollision/mid-activation.
            delay(10);
            continue;
        }

        // --- ISO-DEP / NFC Forum Type 4 Tag (NDEF over APDU) ---
        if (request_len < 5) {
            delay(10);
            continue;
        }

        std::vector<uint8_t> apdu(request, request + request_len);
        if (apdu.size() < 5) {
            delay(10);
            continue;
        }
        uint8_t ins = apdu[ApduCommand::C_APDU_INS];
        uint8_t p1 = apdu[ApduCommand::C_APDU_P1];
        uint8_t p2 = apdu[ApduCommand::C_APDU_P2];
        uint8_t lc = apdu[ApduCommand::C_APDU_LC];
        uint16_t p1p2 = (static_cast<uint16_t>(p1) << 8) | p2;
        std::vector<uint8_t> response;

        if (ins == kInsSelectFile) {
            if (p1 == ApduCommand::C_APDU_P1_SELECT_BY_ID) {
                if (p2 != 0x0C) {
                    response = kSwOk;
                } else if (
                    lc == 2 && apdu.size() >= 7 && apdu[ApduCommand::C_APDU_DATA] == 0xE1 &&
                    (apdu[ApduCommand::C_APDU_DATA + 1] == 0x03 || apdu[ApduCommand::C_APDU_DATA + 1] == 0x04)
                ) {
                    currentFile = (apdu[ApduCommand::C_APDU_DATA + 1] == 0x03) ? TagFile::CC : TagFile::NDEF;
                    response = kSwOk;
                } else {
                    response = kSwNotFound;
                }
            } else if (p1 == ApduCommand::C_APDU_P1_SELECT_BY_NAME) {
                if (apdu.size() >= 12 &&
                    memcmp(kNdefAidSelectByName.data(), apdu.data() + ApduCommand::C_APDU_P2, 9) == 0) {
                    response = kSwOk;
                } else {
                    response = kSwNotSupported;
                }
            } else {
                response = kSwNotSupported;
            }
        } else if (ins == kInsReadBinary) {
            if (currentFile == TagFile::NONE) {
                response = kSwNotFound;
            } else if (p1p2 > kNdefMaxLen) {
                response = kSwEof;
            } else {
                size_t dataLen = (lc == 0) ? kMaxResponseData : std::min<size_t>(lc, kMaxResponseData);
                if (currentFile == TagFile::CC) {
                    for (size_t i = 0; i < dataLen; i++) {
                        size_t idx = static_cast<size_t>(p1p2) + i;
                        response.push_back(idx < kCcFile.size() ? kCcFile[idx] : 0x00);
                    }
                } else {
                    for (size_t i = 0; i < dataLen; i++) {
                        size_t idx = static_cast<size_t>(p1p2) + i;
                        response.push_back(idx < ndefFile.size() ? ndefFile[idx] : 0x00);
                    }
                }
                response.insert(response.end(), kSwOk.begin(), kSwOk.end());
            }
        } else if (ins == kInsUpdateBinary) {
            response = kSwNotSupported;
        } else {
            response = kSwNotSupported;
        }

        hadInteraction = true;
        if (response.empty() || response.size() > 254) {
            nfc.inRelease();
            targetReady = false;
            delay(20);
            continue;
        }
        if (!tgSetDataIrq(nfc, response.data(), static_cast<uint8_t>(response.size()))) {
            nfc.inRelease();
            targetReady = false;
            delay(20);
        }
    }

    nfc.inRelease();
    if (hadInteraction) return SUCCESS;
    return targetArmed ? TAG_NOT_PRESENT : FAILURE;
}

String PN532::emulationCaveat() const {
    // The PN532 in target mode only delivers ISO14443-4/NFC-DEP commands to
    // the host, so raw Type 2 and FeliCa block reads never reach here. Warn
    // the user before they assume a byte-perfect clone.
    if (printableUID.picc_type == "FeliCa") {
        return "PN532 limitation: only IDm/PMm identity is emulated, "
               "block reads (Read Without Encryption) are not supported.";
    }
    if (uid.sak == PICC_TYPE_MIFARE_UL && totalPages > 0 && strAllPages.length() > 0) {
        return "PN532 limitation: only the NDEF content is emulated (as a "
               "Type 4 Tag), not a byte-perfect raw memory clone.";
    }
    return "";
}

int PN532::load() {
    String filepath;
    File file;
    FS *fs;

    if (!getFsStorage(fs)) return FAILURE;
    filepath = loopSD(*fs, true, "RFID|NFC", "/BruceRFID");
    file = fs->open(filepath, FILE_READ);

    if (!file) { return FAILURE; }

    String line;
    String strData;
    strAllPages = "";
    pageReadSuccess = true;

    while (file.available()) {
        line = file.readStringUntil('\n');
        strData = line.substring(line.indexOf(":") + 1);
        strData.trim();
        if (line.startsWith("Device type:")) printableUID.picc_type = strData;
        if (line.startsWith("UID:")) printableUID.uid = strData;
        if (line.startsWith("SAK:")) printableUID.sak = strData;
        if (line.startsWith("ATQA:")) printableUID.atqa = strData;
        // FeliCa dumps (PN532::save()) reuse the SAK/ATQA/"pages" fields for
        // PMm/system code/block count under FeliCa-specific line names - see
        // the equivalent fix in RFIDInterface::loadFromFile().
        if (line.startsWith("Manufacture id:")) printableUID.sak = strData;
        if (line.startsWith("Blocks total:") || line.startsWith("Pages total:")) {
            // totalPages (not just dataPages) must be set here: emulate()'s
            // Type 2/FeliCa raw dispatch (isType2Dump/isFelicaDump) requires
            // totalPages > 0, and this loop previously never set it - a
            // dump loaded via the GUI "Load file" flow would silently fall
            // through to the generic NDEF/T4T-only path instead.
            dataPages = strData.toInt();
            totalPages = dataPages;
        }
        if (line.startsWith("Pages read:") || line.startsWith("Blocks read:")) pageReadSuccess = false;
        if (line.startsWith("Page ") || line.startsWith("Block ")) strAllPages += line + "\n";
    }

    file.close();
    delay(100);
    parse_data();

    return SUCCESS;
}

int PN532::save(const String &filename) {
    FS *fs;
    if (!getFsStorage(fs)) return FAILURE;

    File file = createNewFile(fs, "/BruceRFID", filename + ".rfid");

    if (!file) { return FAILURE; }

    file.println("Filetype: Bruce RFID File");
    file.println("Version 1");
    file.println("Device type: " + printableUID.picc_type);
    file.println("# UID, ATQA and SAK are common for all formats");
    file.println("UID: " + printableUID.uid);
    if (printableUID.picc_type != "FeliCa") {
        file.println("SAK: " + printableUID.sak);
        file.println("ATQA: " + printableUID.atqa);
        file.println("# Memory dump");
        file.println("Pages total: " + String(dataPages));
        if (!pageReadSuccess) file.println("Pages read: " + String(dataPages));
    } else {
        file.println("Manufacture id: " + printableUID.sak);
        file.println("Blocks total: " + String(totalPages));
        file.println("Blocks read: " + String(dataPages));
    }
    file.print(strAllPages);

    file.close();
    delay(100);
    return SUCCESS;
}

String PN532::get_tag_type() {
    String tag_type = nfc.PICC_GetTypeName(nfc.targetUid.sak);

    if (nfc.targetUid.sak == PICC_TYPE_MIFARE_UL) {
        switch (totalPages) {
            case 45: tag_type = "NTAG213"; break;
            case 135: tag_type = "NTAG215"; break;
            case 231: tag_type = "NTAG216"; break;
            default: break;
        }
    }

    return tag_type;
}

void PN532::set_uid() {
    uid.sak = nfc.targetUid.sak;
    uid.size = nfc.targetUid.size;

    for (byte i = 0; i < 2; i++) uid.atqaByte[i] = nfc.targetUid.atqaByte[i];

    for (byte i = 0; i < nfc.targetUid.size; i++) { uid.uidByte[i] = nfc.targetUid.uidByte[i]; }
}

void PN532::format_data() {
    byte bcc = 0;

    printableUID.picc_type = get_tag_type();

    printableUID.sak = nfc.targetUid.sak < 0x10 ? "0" : "";
    printableUID.sak += String(nfc.targetUid.sak, HEX);
    printableUID.sak.toUpperCase();

    // UID
    for (byte i = 0; i < nfc.targetUid.size; i++) { bcc = bcc ^ nfc.targetUid.uidByte[i]; }
    printableUID.uid = hexToStr(nfc.targetUid.uidByte, nfc.targetUid.size);

    // BCC
    printableUID.bcc = bcc < 0x10 ? "0" : "";
    printableUID.bcc += String(bcc, HEX);
    printableUID.bcc.toUpperCase();

    // ATQA
    printableUID.atqa = hexToStr(nfc.targetUid.atqaByte, 2);
}

void PN532::format_data_felica(uint8_t idm[8], uint8_t pmm[8], uint16_t sys_code) {
    // Reuse uid-sak-atqa to save memory
    printableUID.picc_type = "FeliCa";
    printableUID.uid = hexToStr(idm, 8);
    printableUID.sak = hexToStr(pmm, 8);
    printableUID.atqa = String(sys_code, HEX);

    memcpy(uid.uidByte, idm, 8);
}

void PN532::parse_data() {
    String strUID = printableUID.uid;
    strUID.trim();
    strUID.replace(" ", "");
    uid.size = strUID.length() / 2;
    for (size_t i = 0; i < strUID.length(); i += 2) {
        uid.uidByte[i / 2] = strtoul(strUID.substring(i, i + 2).c_str(), NULL, 16);
    }

    printableUID.sak.trim();
    uid.sak = strtoul(printableUID.sak.c_str(), NULL, 16);

    String strAtqa = printableUID.atqa;
    strAtqa.trim();
    strAtqa.replace(" ", "");
    for (size_t i = 0; i < strAtqa.length(); i += 2) {
        uid.atqaByte[i / 2] = strtoul(strAtqa.substring(i, i + 2).c_str(), NULL, 16);
    }
}

int PN532::read_data_blocks() {
    dataPages = 0;
    totalPages = 0;
    int readStatus = FAILURE;

    strAllPages = "";

    if (printableUID.picc_type != "FeliCa") {
        switch (uid.sak) {
            case PICC_TYPE_MIFARE_MINI:
            case PICC_TYPE_MIFARE_1K:
            case PICC_TYPE_MIFARE_4K: readStatus = read_mifare_classic_data_blocks(); break;

            case PICC_TYPE_MIFARE_UL:
                readStatus = read_mifare_ultralight_data_blocks();
                if (totalPages == 0) totalPages = dataPages;
                break;

            default: break;
        }
    } else {
        readStatus = read_felica_data();
    }

    return readStatus;
}

int PN532::read_mifare_classic_data_blocks() {
    byte no_of_sectors = 0;
    int sectorReadStatus = FAILURE;

    switch (uid.sak) {
        case PICC_TYPE_MIFARE_MINI:
            no_of_sectors = 5;
            totalPages = 20; // 320 bytes / 16 bytes per page
            break;

        case PICC_TYPE_MIFARE_1K:
            no_of_sectors = 16;
            totalPages = 64; // 1024 bytes / 16 bytes per page
            break;

        case PICC_TYPE_MIFARE_4K:
            no_of_sectors = 40;
            totalPages = 256; // 4096 bytes / 16 bytes per page
            break;

        default: // Should not happen. Ignore.
            break;
    }

    if (no_of_sectors) {
        for (int8_t i = 0; i < no_of_sectors; i++) {
            sectorReadStatus = read_mifare_classic_data_sector(i);
            if (sectorReadStatus != SUCCESS) break;
        }
    }
    return sectorReadStatus;
}

int PN532::read_mifare_classic_data_sector(byte sector) {
    byte firstBlock;
    byte no_of_blocks;

    if (sector < 32) {
        no_of_blocks = 4;
        firstBlock = sector * no_of_blocks;
    } else if (sector < 40) {
        no_of_blocks = 16;
        firstBlock = 128 + (sector - 32) * no_of_blocks;
    } else {
        return FAILURE;
    }

    byte buffer[18];
    byte blockAddr;
    String strPage;

    int authStatus = authenticate_mifare_classic(firstBlock);
    if (authStatus != SUCCESS) return authStatus;

    for (int8_t blockOffset = 0; blockOffset < no_of_blocks; blockOffset++) {
        strPage = "";
        blockAddr = firstBlock + blockOffset;

        if (!nfc.mifareclassic_ReadDataBlock(blockAddr, buffer)) return FAILURE;

        strPage = hexToStr(buffer, 16);

        strAllPages += "Page " + String(dataPages) + ": " + strPage + "\n";
        dataPages++;
    }

    return SUCCESS;
}

int PN532::authenticate_mifare_classic(byte block) {
    bruceConfig.ensureMifareKeysLoaded();
    uint8_t successA = 0;
    uint8_t successB = 0;

    for (auto key : keys) {
        successA = nfc.mifareclassic_AuthenticateBlock(uid.uidByte, uid.size, block, 0, key);
        if (successA) break;

        if (!nfc.startPassiveTargetIDDetection() || !nfc.readDetectedPassiveTargetID()) {
            return TAG_NOT_PRESENT;
        }
    }

    if (!successA) {
        uint8_t keyA[6];

        for (const auto &mifKey : bruceConfig.mifareKeys) {
            for (size_t i = 0; i < mifKey.length(); i += 2) {
                keyA[i / 2] = strtoul(mifKey.substring(i, i + 2).c_str(), NULL, 16);
            }

            successA = nfc.mifareclassic_AuthenticateBlock(uid.uidByte, uid.size, block, 0, keyA);
            if (successA) break;

            if (!nfc.startPassiveTargetIDDetection() || !nfc.readDetectedPassiveTargetID()) {
                return TAG_NOT_PRESENT;
            }
        }
    }

    for (auto key : keys) {
        successB = nfc.mifareclassic_AuthenticateBlock(uid.uidByte, uid.size, block, 1, key);
        if (successB) break;

        if (!nfc.startPassiveTargetIDDetection() || !nfc.readDetectedPassiveTargetID()) {
            return TAG_NOT_PRESENT;
        }
    }

    if (!successB) {
        uint8_t keyB[6];

        for (const auto &mifKey : bruceConfig.mifareKeys) {
            for (size_t i = 0; i < mifKey.length(); i += 2) {
                keyB[i / 2] = strtoul(mifKey.substring(i, i + 2).c_str(), NULL, 16);
            }

            successB = nfc.mifareclassic_AuthenticateBlock(uid.uidByte, uid.size, block, 1, keyB);
            if (successB) break;

            if (!nfc.startPassiveTargetIDDetection() || !nfc.readDetectedPassiveTargetID()) {
                return TAG_NOT_PRESENT;
            }
        }
    }

    return (successA && successB) ? SUCCESS : TAG_AUTH_ERROR;
}

int PN532::read_mifare_ultralight_data_blocks() {
    uint8_t success;
    byte buffer[18];
    byte i;
    String strPage = "";

    uint8_t buf[4];
    nfc.mifareultralight_ReadPage(3, buf);
    switch (buf[2]) {
        // NTAG213
        case 0x12: totalPages = 45; break;
        // NTAG215
        case 0x3E: totalPages = 135; break;
        // NTAG216
        case 0x6D: totalPages = 231; break;
        // MIFARE UL
        default: totalPages = 64; break;
    }

    for (byte page = 0; page < totalPages; page += 4) {
        success = nfc.ntag2xx_ReadPage(page, buffer);
        if (!success) return FAILURE;

        for (byte offset = 0; offset < 4; offset++) {
            strPage = "";
            for (byte index = 0; index < 4; index++) {
                i = 4 * offset + index;
                strPage += buffer[i] < 0x10 ? F(" 0") : F(" ");
                strPage += String(buffer[i], HEX);
            }
            strPage.trim();
            strPage.toUpperCase();

            strAllPages += "Page " + String(dataPages) + ": " + strPage + "\n";
            dataPages++;
            if (dataPages >= totalPages) break;
        }
    }

    return SUCCESS;
}

int PN532::read_felica_data() {
    String strPage = "";
    totalPages = 14;

    for (uint16_t i = 0x8000; i < 0x8000 + totalPages; i++) {
        uint16_t block_list[1] = {i}; // Read the block i
        uint8_t block_data[1][16] = {0};
        uint16_t default_service_code[1] = {
            0x000B
        }; // Default service code for reading. Should works for every card
        int res = nfc.felica_ReadWithoutEncryption(1, default_service_code, 1, block_list, block_data);

        for (size_t i = 0; i < 16; i++) {
            if (res) { // If card block read successfully, copy data to string
                strPage = hexToStr(block_data[0], 16);
            }
        }
        if (res) { // If PN532 can't read the FeliCa tag, don't write the block to file
            strAllPages += "Block " + String(dataPages++) + ": " + strPage + "\n";
        }
        strPage = ""; // Reset for the next block
    }

    return SUCCESS;
}

int PN532::write_data_blocks() {
    String pageLine = "";
    String strBytes = "";
    int lineBreakIndex;
    int pageIndex;
    bool blockWriteSuccess;
    int totalSize = strAllPages.length();

    while (strAllPages.length() > 0) {
        lineBreakIndex = strAllPages.indexOf("\n");

        if (lineBreakIndex < 0) { break; } // Added for .JS and BugFix

        pageLine = strAllPages.substring(0, lineBreakIndex);
        strAllPages = strAllPages.substring(lineBreakIndex + 1);

        pageIndex = pageLine.substring(5, pageLine.indexOf(":")).toInt();
        strBytes = pageLine.substring(pageLine.indexOf(":") + 1);
        strBytes.trim();

        if (pageIndex == 0) continue;

        if (printableUID.picc_type != "FeliCa") {
            switch (uid.sak) {
                case PICC_TYPE_MIFARE_MINI:
                case PICC_TYPE_MIFARE_1K:
                case PICC_TYPE_MIFARE_4K:
                    if (pageIndex == 0 || (pageIndex + 1) % 4 == 0) continue;
                    blockWriteSuccess = write_mifare_classic_data_block(pageIndex, strBytes);
                    break;

                case PICC_TYPE_MIFARE_UL:
                    if (pageIndex < 4 || pageIndex >= dataPages - 5) continue;
                    blockWriteSuccess = write_mifare_ultralight_data_block(pageIndex, strBytes);
                    break;

                default: blockWriteSuccess = false; break;
            }
        } else {
            blockWriteSuccess = write_felica_data_block(pageIndex, strBytes);
        }

        if (!blockWriteSuccess) return FAILURE;

        progressHandler(totalSize - strAllPages.length(), totalSize, "Writing data blocks...");
    }

    return SUCCESS;
}

bool PN532::write_mifare_classic_data_block(int block, String data) {
    data.replace(" ", "");
    byte size = data.length() / 2;
    byte buffer[size];

    if (size != 16) return false;

    for (size_t i = 0; i < data.length(); i += 2) {
        buffer[i / 2] = strtoul(data.substring(i, i + 2).c_str(), NULL, 16);
    }

    if (authenticate_mifare_classic(block) != SUCCESS) return false;

    return nfc.mifareclassic_WriteDataBlock(block, buffer);
}

bool PN532::write_mifare_ultralight_data_block(int block, String data) {
    data.replace(" ", "");
    byte size = data.length() / 2;
    byte buffer[size];

    if (size != 4) return false;

    for (size_t i = 0; i < data.length(); i += 2) {
        buffer[i / 2] = strtoul(data.substring(i, i + 2).c_str(), NULL, 16);
    }

    return nfc.ntag2xx_WritePage(block, buffer);
}

int PN532::write_felica_data_block(int block, String data) {
    data.replace(" ", "");
    byte size = data.length() / 2;
    uint8_t block_data[1][16] = {0};

    if (size != 16) { return false; }

    for (size_t i = 0; i < data.length(); i += 2) {
        block_data[0][i / 2] = strtoul(data.substring(i, i + 2).c_str(), NULL, 16);
    }

    uint16_t block_list[1] = {(uint16_t)(block +
                                         0x8000)}; // Write the block i. Block in FeliCa start from 0x8000

    uint16_t default_service_code[1] = {
        0x0009
    }; // Default service code for writing. Should works for every card

    return nfc.felica_WriteWithoutEncryption(1, default_service_code, block, block_list, block_data);
}

int PN532::erase_data_blocks() {
    bool blockWriteSuccess;

    switch (uid.sak) {
        case PICC_TYPE_MIFARE_MINI:
        case PICC_TYPE_MIFARE_1K:
        case PICC_TYPE_MIFARE_4K:
            for (byte i = 1; i < 64; i++) {
                if ((i + 1) % 4 == 0) continue;
                blockWriteSuccess =
                    write_mifare_classic_data_block(i, "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
                if (!blockWriteSuccess) return FAILURE;
            }
            break;

        case PICC_TYPE_MIFARE_UL:
            // NDEF stardard
            blockWriteSuccess = write_mifare_ultralight_data_block(4, "03 00 FE 00");
            if (!blockWriteSuccess) return FAILURE;

            for (byte i = 5; i < 130; i++) {
                blockWriteSuccess = write_mifare_ultralight_data_block(i, "00 00 00 00");
                if (!blockWriteSuccess) return FAILURE;
            }
            break;

        default: break;
    }

    return SUCCESS;
}

int PN532::write_ndef_blocks() {
    if (uid.sak != PICC_TYPE_MIFARE_UL) return TAG_NOT_MATCH;

    byte ndef_size = ndefMessage.messageSize + 3;
    byte payload_size = ndef_size % 4 == 0 ? ndef_size : ndef_size + (4 - (ndef_size % 4));
    byte ndef_payload[payload_size];
    byte i;
    bool blockWriteSuccess;
    uint8_t success;

    ndef_payload[0] = ndefMessage.begin;
    ndef_payload[1] = ndefMessage.messageSize;
    ndef_payload[2] = ndefMessage.header;
    ndef_payload[3] = ndefMessage.tnf;
    ndef_payload[4] = ndefMessage.payloadSize;
    ndef_payload[5] = ndefMessage.payloadType;

    for (i = 0; i < ndefMessage.payloadSize; i++) { ndef_payload[i + 6] = ndefMessage.payload[i]; }

    ndef_payload[ndef_size - 1] = ndefMessage.end;

    if (payload_size > ndef_size) {
        for (i = ndef_size; i < payload_size; i++) { ndef_payload[i] = 0; }
    }

    for (int i = 0; i < payload_size; i += 4) {
        int block = 4 + (i / 4);
        success = nfc.ntag2xx_WritePage(block, ndef_payload + i);
        if (!success) return FAILURE;
    }

    return SUCCESS;
}
