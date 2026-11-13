#include "hc11_at.h"

/*
 * AT command interpreter of the emulated HC-11 module.
 * All response strings match the HC-11 V1.9 user manual, including the
 * "RESETT_OK" typo found in the datasheet.
 */

void Hc11Module::commandReset() {
    baud = 9600;
    channel = 1;
    address = 0;
    txPower = 8;
}

void Hc11Module::factoryReset() {
    commandReset();
    mode = 1; // FU1
    dataBits = 8;
    parity = 'N';
    stopBits = 1;
    sleeping = false;
}

void Hc11Session::begin() {
    module.factoryReset();
    commMode = HC11_MODE_COMMAND;
    hostToRadioBytes = 0;
    radioToHostBytes = 0;
    byteLogStart = 0;
    byteLogLen = 0;
}

void Hc11Session::logBytes(const uint8_t *data, size_t len, bool fromHost) {
    for (size_t i = 0; i < len; i++) {
        size_t idx;
        if (byteLogLen < HC11_BYTE_LOG_SIZE) {
            idx = (byteLogStart + byteLogLen) % HC11_BYTE_LOG_SIZE;
            byteLogLen++;
        } else {
            idx = byteLogStart;
            byteLogStart = (byteLogStart + 1) % HC11_BYTE_LOG_SIZE;
        }
        byteLog[idx] = data[i];
        byteLogFromHost[idx] = fromHost;
    }
}

bool hc11IsValidChannel(uint16_t ch) { return ch >= 1 && ch <= 127; }
bool hc11IsValidAddress(uint16_t addr) { return addr <= 255; }
bool hc11IsValidPower(uint8_t p) { return p >= 1 && p <= 8; }
bool hc11IsValidMode(uint8_t m) { return m >= 1 && m <= 4; }

bool hc11IsValidBaud(uint32_t b) {
    switch (b) {
        case 1200:
        case 2400:
        case 4800:
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200: return true;
        default: return false;
    }
}

String hc11ChannelString(uint16_t ch) {
    char buf[8];
    snprintf(buf, sizeof(buf), "C%03u", (unsigned)ch);
    return String(buf);
}

String hc11AddressString(uint16_t addr) {
    char buf[8];
    snprintf(buf, sizeof(buf), "A%03u", (unsigned)addr);
    return String(buf);
}

String hc11PowerString(uint8_t p) {
    char buf[8];
    snprintf(buf, sizeof(buf), "P%u", (unsigned)p);
    return String(buf);
}

String hc11ModeString(uint8_t m) {
    char buf[8];
    snprintf(buf, sizeof(buf), "FU%u", (unsigned)m);
    return String(buf);
}

// Parse exactly n decimal digits at the start of s; returns false on any
// non-digit character or wrong length.
static bool hc11ParseDigits(const String &s, uint32_t &out, size_t n, uint32_t max) {
    if (s.length() < n) return false;
    uint32_t v = 0;
    for (size_t i = 0; i < n; i++) {
        char c = s.charAt(i);
        if (c < '0' || c > '9') return false;
        v = v * 10 + (uint32_t)(c - '0');
        if (v > max) return false;
    }
    out = v;
    return true;
}

String hc11HandleAtLine(Hc11Session &session, const String &rawLine) {
    String line = rawLine;
    // tolerate CR sent by host terminals before LF
    while (line.length() > 0 &&
           (line.charAt(line.length() - 1) == '\r' || line.charAt(line.length() - 1) == '\n')) {
        line.remove(line.length() - 1);
    }
    while (line.length() > 0 && (line.charAt(0) == '\r' || line.charAt(0) == '\n')) { line.remove(0, 1); }
    if (line.length() == 0) return String(); // empty line: stay silent

    Hc11Module &m = session.module;

    // 1) AT - test command
    if (line == "AT") return "OK";

    // 2) AT+Axxx - module address (000..255), response has NO dash: "OK012"
    if (line.startsWith("AT+A")) {
        uint32_t v;
        if (!hc11ParseDigits(line.substring(4), v, 3, 255)) return String();
        m.address = (uint16_t)v;
        char buf[12];
        snprintf(buf, sizeof(buf), "OK%03u", (unsigned)v);
        return String(buf);
    }

    // 3) AT+Bxxxx - UART baud rate, response: "OK-B19200"
    if (line.startsWith("AT+B")) {
        uint32_t v = 0;
        String num = line.substring(4);
        if (num.length() == 0 || num.length() > 6) return String();
        for (size_t i = 0; i < num.length(); i++) {
            char c = num.charAt(i);
            if (c < '0' || c > '9') return String();
            v = v * 10 + (uint32_t)(c - '0');
        }
        if (!hc11IsValidBaud(v)) return String();
        m.baud = v;
        return "OK-B" + num;
    }

    // 4) AT+Cxxx - channel (001..127), response: "OK-C020"
    if (line.startsWith("AT+C")) {
        uint32_t v;
        if (!hc11ParseDigits(line.substring(4), v, 3, 127)) return String();
        if (v < 1) return String();
        m.channel = (uint16_t)v;
        return "OK-" + hc11ChannelString(m.channel);
    }

    // 5) AT+FUx - UART transparent mode (1..4), response: "OK-FU3"
    if (line.startsWith("AT+FU")) {
        uint32_t v;
        if (!hc11ParseDigits(line.substring(5), v, 1, 4)) return String();
        if (v < 1) return String();
        m.mode = (uint8_t)v;
        return "OK-" + hc11ModeString(m.mode);
    }

    // 6) AT+Px - transmit power (1..8), response: "OK-P5"
    if (line.startsWith("AT+P")) {
        uint32_t v;
        if (!hc11ParseDigits(line.substring(4), v, 1, 8)) return String();
        if (v < 1) return String();
        m.txPower = (uint8_t)v;
        return "OK-" + hc11PowerString(m.txPower);
    }

    // 8) AT+RX - query all parameters, CRLF separated (the final CRLF is
    // appended by the response writer, like for every other response)
    // (checked before AT+Ry: same length, 'X' is not a valid query key)
    if (line == "AT+RX") {
        String out = "U" + String(m.mode);
        out += "\r\n";
        out += "B" + String(m.baud);
        out += "\r\n";
        out += hc11ChannelString(m.channel);
        out += "\r\n";
        out += hc11AddressString(m.address);
        out += "\r\n";
        out += hc11PowerString(m.txPower);
        return out;
    }

    // 7) AT+Ry - query a single parameter (no "OK" prefix in the answer)
    if (line.startsWith("AT+R") && line.length() == 5) {
        switch (line.charAt(4)) {
            case 'A': return hc11AddressString(m.address);       // "A000"
            case 'B': return "B" + String(m.baud);               // "B9600"
            case 'C': return "R" + hc11ChannelString(m.channel); // "RC001"
            case 'F': return "U" + String(m.mode);               // "U1"
            case 'P': return hc11PowerString(m.txPower);         // "P8"
            default: return String();
        }
    }

    // 9) AT+Uxxx - UART format (data bits, parity, stop bits), response: "OK-U8O1"
    if (line.startsWith("AT+U") && line.length() == 7) {
        uint8_t d = (uint8_t)(line.charAt(4) - '0');
        char p = line.charAt(5);
        uint8_t s = (uint8_t)(line.charAt(6) - '0');
        if (d < 6 || d > 8) return String();
        if (p != 'N' && p != 'O' && p != 'E') return String();
        if (s < 1 || s > 3) return String();
        m.dataBits = d;
        m.parity = p;
        m.stopBits = s;
        return "OK-U" + line.substring(4);
    }

    // 10) AT+V - firmware version
    if (line == "AT+V") return "HC-11_V1.9";

    // 11) AT+SLEEP - enter sleep mode on leaving the command mode
    if (line == "AT+SLEEP") {
        m.sleeping = true;
        return "OK";
    }

    // 12) AT+RESET - restore factory defaults ("RESETT_OK" typo is in the datasheet)
    if (line == "AT+RESET") {
        m.commandReset();
        return "RESETT_OK";
    }

    // Unknown or invalid command: silently ignored
    return String();
}
