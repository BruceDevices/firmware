/**
 * @file RFIDInterface.h
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Interface for RFID modules
 * @version 0.1
 * @date 2024-08-19
 */

#ifndef __RFID_INTERFACE_H__
#define __RFID_INTERFACE_H__

#include <globals.h>

class RFIDInterface {
public:
    typedef struct {
        byte size;
        byte uidByte[10];
        byte sak;
        byte atqaByte[2];
    } Uid;

    typedef struct {
        String uid;
        String bcc;
        String sak;
        String atqa;
        String picc_type;
    } PrintableUID;

    typedef struct {
        byte begin = 0x03;
        byte messageSize;
        byte header = 0xD1;
        byte tnf = 0x01;
        byte payloadSize;
        byte payloadType;
        byte payload[140];
        byte end = 0xFE;
    } NdefMessage;

    enum ReturnCode {
        SUCCESS = 0,
        FAILURE = 1,
        TAG_NOT_PRESENT = 2,
        TAG_NOT_MATCH = 3,
        TAG_AUTH_ERROR = 4,
        NOT_IMPLEMENTED = 5,
    };

    enum NDEF_Payload_Type { NDEF_TEXT = 0x54, NDEF_URI = 0x55 };

    uint8_t keys[15][6] = {
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
        {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
        {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD},
        {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A},
        {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
        {0x71, 0x4C, 0x5C, 0x88, 0x6E, 0x97},
        {0x58, 0x7E, 0xE5, 0xF9, 0x35, 0x0F},
        {0xA0, 0x47, 0x8C, 0xC3, 0x90, 0x91},
        {0x53, 0x3C, 0xB6, 0xC7, 0x23, 0xF6},
        {0x8F, 0xD0, 0xA4, 0xF2, 0x56, 0xE9},
        {0xA6, 0x45, 0x98, 0xA7, 0x74, 0x78},
        {0x26, 0x94, 0x0B, 0x21, 0xFF, 0x5D},
        {0xFC, 0x00, 0x01, 0x87, 0x78, 0xF7},
        {0x00, 0x00, 0x0F, 0xFE, 0x24, 0x88}
    };

    Uid uid;
    PrintableUID printableUID;
    NdefMessage ndefMessage;
    String strAllPages = "";
    int totalPages = 0;
    int dataPages = 0;
    bool pageReadSuccess = false;
    int pageReadStatus = FAILURE;

    virtual ~RFIDInterface() {} // Virtual destructor

    /////////////////////////////////////////////////////////////////////////////////////
    // Life Cycle
    /////////////////////////////////////////////////////////////////////////////////////
    virtual bool begin() = 0;

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    virtual int read(int cardBaudRate = 0) = 0; // cardBaudRate = 0 means MIFARE, 1 means FeliCa
    virtual int clone() = 0;
    virtual int erase() = 0;
    virtual int write(int cardBaudRate = 0) = 0;
    virtual int write_ndef() = 0;
    virtual int emulate() { return NOT_IMPLEMENTED; }
    virtual int load() = 0;
    virtual int save(String filename) = 0;

    String statusMessage(int status) const {
        switch (status) {
            case SUCCESS: return String(F("Success"));
            case FAILURE: return String(F("Failed reading data blocks"));
            case TAG_NOT_PRESENT: return String(F("Failed reading. Tag not found"));
            case TAG_NOT_MATCH: return String(F("Error! Tags don't match"));
            case TAG_AUTH_ERROR: return String(F("Failed authenticating"));
            case NOT_IMPLEMENTED: return String(F("Not implemented"));
            default: return String();
        }
    }
};

#endif
