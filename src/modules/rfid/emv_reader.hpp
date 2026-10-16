#ifndef EMV_READER_H
#define EMV_READER_H
#include "PN532.h"
#include "emv_protocol.h"

class EMVReader {
    PN532 *_rfid = nullptr;
    Adafruit_PN532 *nfc = nullptr;
    bool _cancelled = false;
    uint8_t _target = 0;
    bool writeFrame(const emv::Bytes &frame);
    bool readFrame(emv::Bytes &frame, size_t length);
    bool waitReady(uint32_t timeout);
    bool command(const emv::Bytes &cmd, emv::Bytes &response, uint32_t timeout = 2000);
    bool exchange(const emv::Bytes &apdu, emv::Bytes &response);
    bool detect();
    void displayCard(const emv::Card &card);
    void saveCard(const emv::Card &card);

public:
    EMVReader() { setup(); }
    ~EMVReader();
    void setup();
};
#endif
