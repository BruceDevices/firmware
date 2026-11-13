#ifndef __HC11_AT_H__
#define __HC11_AT_H__

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Emulated HC-11 wireless RF UART module (virtual, no radio).
 * Parameter ranges and factory defaults per the official HC-11 V1.9
 * user manual (hc01.com); AT response formats per the same datasheet
 * according to the HC-11 V1.9 user manual.
 */

struct Hc11Module {
    uint16_t channel = 1;   // AT+Cxxx, valid 1..127   (factory: C001)
    uint16_t address = 0;   // AT+Axxx, valid 0..255   (factory: A000)
    uint8_t  txPower = 8;   // AT+Px,   valid 1..8     (factory: P8, +10dBm)
    uint8_t  mode = 1;      // AT+FUx,  valid 1..4     (factory: FU1)
    uint32_t baud = 9600;   // AT+Bxxxx (factory: 9600)
    uint8_t  dataBits = 8;  // AT+Uxxx  (factory: 8N1)
    char     parity = 'N';  // 'N' none, 'O' odd, 'E' even
    uint8_t  stopBits = 1;  // 1, 2, 3 (=1.5 per datasheet)
    bool     sleeping = false; // AT+SLEEP

    // AT+RESET per datasheet: "The UART baud rate, UART communication channel,
    // module address and transmit power restore factory default."
    // Mode (FUx) and UART format are not listed and are kept unchanged.
    void commandReset();

    // Full factory state (used on app launch / new session)
    void factoryReset();
};

enum Hc11CommMode : uint8_t { HC11_MODE_COMMAND = 0, HC11_MODE_TRANSPARENT = 1 };

#define HC11_BYTE_LOG_SIZE 384

struct Hc11Session {
    Hc11Module module;
    Hc11CommMode commMode = HC11_MODE_COMMAND;

    // Traffic counters for the on-device monitor
    uint32_t hostToRadioBytes = 0; // data bytes received from host (transparent mode)
    uint32_t radioToHostBytes = 0; // bytes sent to host (AT responses)

    // Ring buffer with the last bytes exchanged, for the byte-log screen
    uint8_t byteLog[HC11_BYTE_LOG_SIZE];
    bool byteLogFromHost[HC11_BYTE_LOG_SIZE]; // direction of each entry
    size_t  byteLogStart = 0;
    size_t  byteLogLen = 0;

    void begin(); // new session: factory defaults, zero counters, command mode
    void logBytes(const uint8_t *data, size_t len, bool fromHost);
};

// Handle one AT command line (line terminators already stripped).
// Returns the response to send back to the host, or an empty String to
// stay silent (unknown/invalid commands are silently ignored).
String hc11HandleAtLine(Hc11Session &session, const String &line);

// Validation helpers (shared with the device-side config screen)
bool hc11IsValidChannel(uint16_t ch);
bool hc11IsValidAddress(uint16_t addr);
bool hc11IsValidPower(uint8_t p);
bool hc11IsValidMode(uint8_t m);
bool hc11IsValidBaud(uint32_t b);

// Formatting helpers ("C020", "A012", "P8", "FU3", "B9600")
String hc11ChannelString(uint16_t ch);
String hc11AddressString(uint16_t addr);
String hc11PowerString(uint8_t p);
String hc11ModeString(uint8_t m);

#endif // __HC11_AT_H__
