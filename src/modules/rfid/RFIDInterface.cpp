/**
 * @file RFIDInterface.cpp
 * @brief Shared, module-agnostic helpers for RFID modules.
 *
 * These live in the base class so the serial CLI and the GUI flow through the
 * exact same code regardless of which driver is active (PN532, RFID2, ...).
 * Drivers may override loadFromFile() for richer, hardware-specific parsing.
 */

#include "RFIDInterface.h"
#include "apdu.h"
#include "core/sd_functions.h"
#include <FS.h>

// Generic Bruce .rfid parser. Drivers with extra fields (MIFARE Classic
// blocks/keys, NTAG version/signature/counters) override this.
int RFIDInterface::loadFromFile(const String &filepath) {
    FS *fs;
    if (!getFsStorage(fs)) return FAILURE;

    File file = fs->open(filepath, FILE_READ);
    if (!file) return FAILURE;

    String line;
    String strData;
    strAllPages = "";
    totalPages = 0;
    dataPages = 0;
    pageReadSuccess = true;
    pageReadStatus = SUCCESS;

    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        strData = line.substring(line.indexOf(":") + 1);
        strData.trim();

        if (line.startsWith("Device type:")) printableUID.picc_type = strData;
        else if (line.startsWith("UID:")) printableUID.uid = strData;
        else if (line.startsWith("SAK:")) printableUID.sak = strData;
        else if (line.startsWith("ATQA:")) printableUID.atqa = strData;
        // FeliCa dumps (PN532::save()) reuse the SAK/ATQA/"pages" fields for
        // PMm/system code/block count under FeliCa-specific line names.
        else if (line.startsWith("Manufacture id:")) printableUID.sak = strData;
        else if (line.startsWith("Pages total:") || line.startsWith("Blocks total:"))
            totalPages = strData.toInt();
        else if (line.startsWith("Pages read:") || line.startsWith("Blocks read:")) pageReadSuccess = false;
        else if (line.startsWith("Page ")) {
            strAllPages += line + "\n";
            dataPages++;
        }
        // MIFARE Classic dumps (.rfid/.nfc): keep Block and Key lines so a
        // driver can rebuild the dump for clone/emulate.
        else if (line.startsWith("Block ")) {
            strAllPages += line + "\n";
            dataPages++;
        } else if (line.startsWith("Key A sector ") || line.startsWith("Key B sector ")) {
            strAllPages += line + "\n";
        }
    }
    file.close();

    String uidStr = printableUID.uid;
    uidStr.trim();
    uidStr.replace(" ", "");
    uid.size = uidStr.length() / 2;
    if (uid.size > sizeof(uid.uidByte)) uid.size = sizeof(uid.uidByte);
    for (int i = 0; i < uid.size; i++) {
        uid.uidByte[i] = strtoul(uidStr.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
    }
    uid.sak = strtoul(printableUID.sak.c_str(), NULL, 16);

    String atqaStr = printableUID.atqa;
    atqaStr.replace(" ", "");
    if (atqaStr.length() >= 4) {
        // ATQA is stored in transmission order (atqaByte[0] first).
        uid.atqaByte[0] = strtoul(atqaStr.substring(0, 2).c_str(), NULL, 16);
        uid.atqaByte[1] = strtoul(atqaStr.substring(2, 4).c_str(), NULL, 16);
    }
    if (totalPages == 0) totalPages = dataPages;

    return SUCCESS;
}

void RFIDInterface::buildNdefMessage(const String &type, const String &value) {
    String t = type;
    t.toLowerCase();

    memset(&ndefMessage, 0, sizeof(ndefMessage));
    ndefMessage.begin = 0x03;
    ndefMessage.header = 0xD1;
    ndefMessage.tnf = 0x01;
    ndefMessage.end = 0xFE;

    if (t == "text") {
        ndefMessage.payloadType = NDEF_TEXT;
        ndefMessage.payload[0] = 0x02;
        ndefMessage.payload[1] = 'e';
        ndefMessage.payload[2] = 'n';
        uint8_t len = min((int)value.length(), 96);
        for (uint8_t i = 0; i < len; i++) ndefMessage.payload[i + 3] = value.charAt(i);
        ndefMessage.payloadSize = len + 3;
    } else {
        ndefMessage.payloadType = NDEF_URI;
        ndefMessage.payload[0] = 0x00; // no prefix abbreviation
        uint8_t len = min((int)value.length(), 99);
        for (uint8_t i = 0; i < len; i++) ndefMessage.payload[i + 1] = value.charAt(i);
        ndefMessage.payloadSize = len + 1;
    }
    ndefMessage.messageSize = ndefMessage.payloadSize + 4;
}

void RFIDInterface::buildWifiNdef(const String &ssid, const String &password) {
    std::vector<uint8_t> wscPayload = Ndef::wifiCredentialPayload(ssid.c_str(), password.c_str());
    rawNdefRecord = Ndef::mimeRecord("application/vnd.wfa.wsc", wscPayload, {'1'});
}
