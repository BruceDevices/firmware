#ifndef LITE_VERSION
#include "emv_reader.hpp"
#include "core/display.h"
#include "core/scrollableTextArea.h"
#include <cstring>
#include <ctime>
#include <esp_random.h>
#include <globals.h>

namespace {
constexpr size_t FrameSize = 280; // Extended PN532 frame + maximum 258-byte R-APDU
const emv::Bytes Ack = {0, 0, 0xff, 0, 0xff, 0};
String cardText(const emv::Card &card) {
    String text = "Application: " + String(card.label.empty() ? "EMV" : card.label.c_str()) + "\nAID: ";
    for (auto b : card.aid) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", b);
        text += hex;
    }
    text += "\nPAN: ";
    for (size_t i = 0; i < card.pan.size(); ++i) {
        if (i && i % 4 == 0) text += ' ';
        text += card.pan[i];
    }
    if (!card.holder.empty()) text += "\nName: " + String(card.holder.c_str());
    if (!card.validFrom.empty()) text += "\nValid from: " + String(card.validFrom.c_str());
    text += "\nExpires: " + String(card.validTo.empty() ? "Not supplied" : card.validTo.c_str());
    return text;
}
} // namespace

EMVReader::~EMVReader() {
    if (nfc) {
        // ACK aborts an outstanding command after cancellation/timeout.
        writeFrame(Ack);
        delay(2);
        _cancelled = false;
        emv::Bytes ignored;
        if (_target) command({PN532_COMMAND_INRELEASE, _target}, ignored, 300);
        command({PN532_COMMAND_RFCONFIGURATION, 0x01, 0x00}, ignored, 300);
        // This driver has no destructor for its dynamically allocated bus adapters.
        delete nfc->i2c_dev;
        nfc->i2c_dev = nullptr;
        delete nfc->spi_dev;
        nfc->spi_dev = nullptr;
    }
    delete _rfid;
}

bool EMVReader::writeFrame(const emv::Bytes &frame) {
    if (frame.empty()) return false;
    if (nfc->spi_dev) {
        const uint8_t prefix = PN532_SPI_DATAWRITE;
        return nfc->spi_dev->write(frame.data(), frame.size(), &prefix, 1);
    }
    if (!nfc->i2c_dev) return false;
    Wire.beginTransmission(PN532_I2C_ADDRESS);
    const bool complete = Wire.write(frame.data(), frame.size()) == frame.size();
    return Wire.endTransmission() == 0 && complete;
}

bool EMVReader::readFrame(emv::Bytes &frame, size_t length) {
    frame.assign(length, 0);
    if (nfc->spi_dev) {
        const uint8_t prefix = PN532_SPI_DATAREAD;
        return nfc->spi_dev->write_then_read(&prefix, 1, frame.data(), frame.size());
    }
    // ONE transaction: BusIO's chunked reads insert extra PN532 RDY bytes.
    if (Wire.requestFrom(uint8_t(PN532_I2C_ADDRESS), length + 1, true) != length + 1) {
        while (Wire.available()) Wire.read();
        return false;
    }
    if (Wire.read() != 0x01) {
        while (Wire.available()) Wire.read();
        return false;
    }
    for (auto &b : frame) b = Wire.read();
    return true;
}

bool EMVReader::waitReady(uint32_t timeout) {
    uint32_t start = millis();
    do {
        if (check(EscPress)) {
            _cancelled = true;
            return false;
        }
        if (nfc->spi_dev ? nfc->isready() : (nfc->_irq >= 0 ? digitalRead(nfc->_irq) == LOW : nfc->isready()))
            return true;
        delay(2);
    } while (millis() - start < timeout);
    return false;
}

bool EMVReader::command(const emv::Bytes &cmd, emv::Bytes &response, uint32_t timeout) {
    if (_cancelled || cmd.empty()) return false;
    emv::Bytes frame;
    if (!writeFrame(emv::pn532Command(cmd))) return false;
    delay(1);
    if (!waitReady(timeout) || !readFrame(frame, Ack.size()) || frame != Ack) {
        writeFrame(Ack);
        delay(2);
        return false;
    }
    delay(1);
    if (!waitReady(timeout) || !readFrame(frame, FrameSize)) {
        writeFrame(Ack);
        delay(2);
        return false;
    }
    return emv::pn532Response(frame, cmd[0], response);
}

bool EMVReader::exchange(const emv::Bytes &apdu, emv::Bytes &response) {
    emv::Bytes cmd = {PN532_COMMAND_INDATAEXCHANGE, _target}, payload;
    cmd.insert(cmd.end(), apdu.begin(), apdu.end());
    // PN532 performs ISO14443-4 chaining internally. Reject unexpected NAD/MI
    // and errors rather than passing status bytes into the BER-TLV parser.
    if (!command(cmd, payload) || payload.empty() || payload[0] != 0) return false;
    response.assign(payload.begin() + 1, payload.end());
    return true;
}

bool EMVReader::detect() {
    while (!_cancelled) {
        emv::Bytes target;
        if (!command({PN532_COMMAND_INLISTPASSIVETARGET, 1, PN532_MIFARE_ISO14443A}, target)) {
            if (!_cancelled) displayError("PN532 polling failed", true);
            return false;
        }
        if (target.size() == 1 && target[0] == 0) continue;
        if (target.size() < 6 || target[0] != 1 || !target[1] || target[5] > 10 || target[5] < 4 ||
            target.size() < size_t(6 + target[5])) {
            displayError("Invalid NFC target", true);
            return false;
        }
        _target = target[1];
        if (!(target[4] & 0x20)) {
            displayError("Not an ISO14443-4 card", true);
            return false;
        }
        return true;
    }
    return false;
}

void EMVReader::setup() {
    switch (bruceConfigPins.rfidModule) {
        case PN532_I2C_MODULE: _rfid = new PN532(PN532::I2C); break;
#ifdef M5STICK
        case PN532_I2C_SPI_MODULE: _rfid = new PN532(PN532::I2C_SPI); break;
#endif
        case PN532_SPI_MODULE: _rfid = new PN532(PN532::SPI); break;
        default: displayError("EMV requires a PN532", true); return;
    }
    nfc = &_rfid->nfc;
    if (!_rfid->begin()) {
        displayError("PN532 initialization failed", true);
        return;
    }
    if (nfc->i2c_dev && Wire.setBufferSize(FrameSize + 1) < FrameSize + 1) {
        displayError("NFC buffer allocation failed", true);
        return;
    }
    emv::Bytes response;
    // Restore automatic RATS after other NFC modes, and finite polling for Back.
    if (!command({PN532_COMMAND_SETPARAMETERS, 0x14}, response) ||
        !command({PN532_COMMAND_RFCONFIGURATION, 0x05, 0x00, 0x00, 0x00}, response) ||
        !command({PN532_COMMAND_RFCONFIGURATION, 0x01, 0x01}, response)) {
        if (!_cancelled) displayError("PN532 configuration failed", true);
        return;
    }
    displayInfo("Hold EMV card near NFC");
    if (!detect()) return;
    displayInfo("Reading - keep card still");
    emv::Terminal terminal;
    const uint32_t random = esp_random();
    for (unsigned i = 0; i < 4; ++i) terminal.unpredictable[i] = random >> (8 * i);
    time_t now = time(nullptr);
    struct tm date{};
    localtime_r(&now, &date);
    if (date.tm_year < 120 || date.tm_year > 199) {
        // A freshly flashed device may not have its clock set. Use a valid
        // build date instead of the old hard-coded date or an invalid 000000.
        char month[4] = {};
        int year = 0;
        sscanf(__DATE__, "%3s %d %d", month, &date.tm_mday, &year);
        const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
        const char *found = strstr(months, month);
        date.tm_mon = found ? (found - months) / 3 : 0;
        date.tm_year = year - 1900;
    }
    auto bcd = [](int n) { return uint8_t((n / 10) * 16 + n % 10); };
    terminal.date = {bcd((date.tm_year + 1900) % 100), bcd(date.tm_mon + 1), bcd(date.tm_mday)};
    emv::Card card;
    std::string error;
    bool ok = emv::readCard(
        [this](const emv::Bytes &a, emv::Bytes &r) { return exchange(a, r); }, terminal, card, error
    );
    if (_cancelled) return;
    if (!ok) {
        displayError(error.c_str(), true);
        return;
    }
    displayCard(card);
}

void EMVReader::displayCard(const emv::Card &card) {
    drawMainBorderWithTitle("Read EMV Card");
    ScrollableTextArea area(
        1, BORDER_PAD_X, BORDER_PAD_Y, tftWidth - 2 * BORDER_PAD_X, tftHeight - BORDER_PAD_Y - 12, false, true
    );
    area.fromString(cardText(card));
    area.show(true);
    options = {
        {"Save", [this, card]() { saveCard(card); }},
        {"Exit", []() {}}
    };
    loopOptions(options);
    options.clear();
}

void EMVReader::saveCard(const emv::Card &card) {
    FS *fs;
    if (!getFsStorage(fs)) return;
    if (!fs->exists("/BruceRFID")) fs->mkdir("/BruceRFID");
    if (!fs->exists("/BruceRFID/Scans")) fs->mkdir("/BruceRFID/Scans");
    String filename = "/BruceRFID/Scans/emv_" + String(card.pan.c_str()) + ".txt";
    File file = fs->open(filename, FILE_WRITE);
    if (!file) {
        displayError("Error opening file");
        return;
    }
    file.println(cardText(card));
    file.close();
    displaySuccess("EMV data saved");
}
#endif
