// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_wmbus.h"
#include "wmbus_codec.h"
#include "rf_utils.h"
#include "core/display.h"
#include "core/utils.h"
#include <algorithm>

namespace {
constexpr size_t MAX_DEVICES = 96;
struct Device {
    wmbus::Frame frame;
    uint32_t count = 0, first = 0, last = 0;
};
struct Session {
    std::vector<Device> devices;
    uint32_t valid = 0, rejected = 0, overflows = 0, timeouts = 0, tableFull = 0;
};

// Packet FIFO mode, 100 kchip/s 2-FSK, 325 kHz BW, sync 0x543D.
// CC1101 parameters: TI AN067 / wMbus-lib. C/T framing: TI SWRA522
// (a CC13xx application note, used here only for the on-air discriminator).
// Packet control differs from the TI example: no appended status bytes,
// infinite reception with a software length limit, no auto TX.
const uint8_t preset[][2] = {
    {0x00,0x06}, {0x01,0x2E}, {0x02,0x00}, {0x03,0x07},
    {0x04,0x54}, {0x05,0x3D}, {0x06,0xFF}, {0x07,0x00},
    {0x08,0x02}, {0x09,0x00}, {0x0A,0x00}, {0x0B,0x08},
    {0x0C,0x00}, {0x0D,0x21}, {0x0E,0x6B}, {0x0F,0xD0},
    {0x10,0x5C}, {0x11,0x04}, {0x12,0x06}, {0x13,0x22},
    {0x14,0xF8}, {0x15,0x44}, {0x16,0x07}, {0x17,0x00},
    {0x18,0x18}, {0x19,0x2E}, {0x1A,0xBF}, {0x1B,0x43},
    {0x1C,0x09}, {0x1D,0xB5}, {0x21,0xB6}, {0x22,0x10},
    {0x23,0xEA}, {0x24,0x2A}, {0x25,0x00}, {0x26,0x1F},
    {0x2C,0x81}, {0x2D,0x35}, {0x2E,0x09}
};

class Receiver {
    uint8_t saved[0x2F] = {};
    uint8_t raw[435] = {};
    size_t used = 0, expected = 0;
    bool active = false, savedValid = false;
    uint32_t started = 0;
    bool waitState(uint8_t state) {
        const uint32_t start = millis();
        do {
            if ((ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 31) == state) return true;
            delay(1);
        } while (millis() - start < 30);
        return false;
    }
    uint8_t fifoCount() {
        // Status registers can be incoherent during updates (CC1101 errata).
        uint8_t a = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES);
        for (int i = 0; i < 4; ++i) {
            const uint8_t b = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES);
            if (a == b) return b;
            a = b;
        }
        return 0; // No coherent sample yet: retry on the next poll, never guess a count.
    }
public:
    bool begin() {
        if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
            displayError("Select CC1101 in RF Config", true);
            return false;
        }
        // Uses Bruce's board pin configuration, SPI bus and antenna switch.
        if (!initRfModule("rx", 868.95f)) { deinitRfModule(); return false; }
        active = true;
        if (!ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE) || !waitState(1)) { end(); return false; }
        for (uint8_t i = 0; i < sizeof(saved); ++i) saved[i] = ELECHOUSE_cc1101.SpiReadReg(i);
        savedValid = true;
        for (const auto &r : preset) {
            if (!ELECHOUSE_cc1101.SpiWriteReg(r[0], r[1])) { end(); return false; }
        }
        if (!ELECHOUSE_cc1101.SpiStrobe(CC1101_SCAL) || !waitState(1)) { end(); return false; }
        return restart();
    }
    bool restart() {
        used = expected = 0;
        started = 0;
        if (!ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE) || !waitState(1)) return false;
        if (!ELECHOUSE_cc1101.SpiStrobe(CC1101_SFRX) ||
            !ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, 0x02) ||
            !ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTLEN, 0xFF) ||
            !ELECHOUSE_cc1101.SpiStrobe(CC1101_SRX)) return false;
        return waitState(13);
    }
    void end() {
        if (!active) return;
        ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
        waitState(1);
        ELECHOUSE_cc1101.SpiStrobe(CC1101_SFRX);
        if (savedValid)
            for (uint8_t i = 0; i < sizeof(saved); ++i) ELECHOUSE_cc1101.SpiWriteReg(i, saved[i]);
        deinitRfModule();
        active = false;
        savedValid = false;
    }
    ~Receiver() { end(); }
    bool busy() { return used != 0 || (fifoCount() & 0x7F) != 0; }
    // -1 = unrecoverable receiver state, 0 = pending, 1 = validated frame.
    int poll(wmbus::Frame &frame, Session &stats) {
        const uint8_t status = fifoCount();
        if (status & 0x80) {
            ++stats.overflows;
            return restart() ? 0 : -1;
        }
        const size_t available = status & 0x7F;
        const uint8_t state = ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 31;
        if (!started && available) started = millis();
        if (started && millis() - started > 80) {
            ++stats.timeouts;
            return restart() ? 0 : -1;
        }
        if (available) {
            // Leave one byte while RX is running (CC1101 RX FIFO erratum).
            // Stop explicitly once the final bytes are buffered. Infinite mode
            // avoids the 8-bit hardware packet counter limit for long T1 frames.
            const bool final = expected && available >= expected - used;
            if (final) {
                if (!ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE) || !waitState(1)) return -1;
            }
            size_t n = final ? expected - used : state == 1 ? available : available - 1;
            // First establish length before consuming a potentially corrupt frame.
            if (!expected) n = std::min(n, size_t(3) - used);
            if (used + n > sizeof(raw) || (expected && used + n > expected)) {
                ++stats.rejected;
                return restart() ? 0 : -1;
            }
            if (n) {
                if (!ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, raw + used, uint8_t(n))) return -1;
                used += n;
            }
            if (!expected && used >= 3) {
                wmbus::Mode mode;
                if (!wmbus::packetSize(raw, used, expected, mode)) {
                    ++stats.rejected;
                    return restart() ? 0 : -1;
                }
            }
        }
        if (expected && used == expected) {
            const bool ok = wmbus::decode(raw, used, frame);
            const int r = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RSSI);
            frame.rssi = (r >= 128 ? r - 256 : r) / 2 - 74;
            frame.lqi = ELECHOUSE_cc1101.SpiReadStatus(CC1101_LQI) & 0x7F;
            if (!ok) ++stats.rejected;
            if (!restart()) return -1;
            return ok ? 1 : 0;
        }
        return 0;
    }
};

void remember(Session &s, const wmbus::Frame &frame) {
    ++s.valid;
    const auto key = wmbus::deviceKey(frame);
    for (auto &d : s.devices) {
        if (wmbus::deviceKey(d.frame) == key) {
            d.frame = frame; ++d.count; d.last = millis(); return;
        }
    }
    if (s.devices.size() == MAX_DEVICES) { ++s.tableFull; return; }
    Device d;
    d.frame = frame; d.count = 1; d.first = d.last = millis();
    s.devices.push_back(d);
}

int rows() { return std::max(1, (int(tftHeight) - 40) / 10); }
void screen(const char *title) {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(4, 3); tft.print(title);
}
void drawList(const Session &s, size_t selected) {
    screen("Wireless M-Bus   T1/C1   868.95 MHz");
    tft.setCursor(4, 15);
    tft.printf("Devices:%u RX:%lu Bad:%lu OV:%lu", unsigned(s.devices.size()),
               (unsigned long)s.valid, (unsigned long)s.rejected, (unsigned long)s.overflows);
    if (s.devices.empty()) {
        tft.setCursor(4, 40); tft.print("Waiting for valid telegrams...");
        tft.setCursor(4, 54); tft.print("Meters may transmit infrequently.");
    }
    const size_t start = selected / rows() * rows();
    for (size_t i = start; i < s.devices.size() && i < start + rows(); ++i) {
        const auto &d = s.devices[i];
        tft.setCursor(4, 28 + int(i - start) * 10);
        tft.printf("%c %s %s %4d dBm #%lu", i == selected ? '>' : ' ',
                   wmbus::identity(d.frame).c_str(), wmbus::manufacturer(d.frame).c_str(),
                   d.frame.rssi, (unsigned long)d.count);
    }
    tft.setCursor(4, tftHeight - 10);
    tft.print(s.tableFull ? "Table full! Esc: stop/export" : "Wheel: select  Click: detail  Esc: stop");
}
void details(const Device &d) {
    auto source = wmbus::describe(d.frame);
    source.insert(source.begin(), "Packets: " + std::to_string(d.count) + " Last boot ms: " + std::to_string(d.last));
    std::vector<std::string> lines;
    const size_t columns = std::max(8, (int(tftWidth) - 8) / 6);
    for (const auto &line : source) {
        for (size_t p = 0; p < line.size(); p += columns) lines.push_back(line.substr(p, columns));
    }
    size_t top = 0;
    bool redraw = true;
    while (!returnToMenu) {
        if (redraw) {
            screen("Wireless M-Bus details (RX paused)");
            for (size_t i = top; i < lines.size() && i < top + rows(); ++i) {
                tft.setCursor(4, 24 + int(i - top) * 10); tft.print(lines[i].c_str());
            }
            tft.setCursor(4, tftHeight - 10); tft.print("Wheel: scroll  Click/Esc: resume");
            redraw = false;
        }
        if (check(EscPress) || check(SelPress)) break;
        if (check(NextPress) && top + rows() < lines.size()) { ++top; redraw = true; }
        if (check(PrevPress) && top) { --top; redraw = true; }
        delay(20);
    }
}
void scan(Session &s) {
    Receiver rx;
    if (!rx.begin()) { displayError("wM-Bus receiver init failed", true); return; }
    size_t selected = 0;
    uint32_t lastDraw = 0;
    bool dirty = true;
    drawList(s, selected);
    while (!returnToMenu) {
        wmbus::Frame f;
        const int result = rx.poll(f, s);
        if (result < 0) { displayError("CC1101 state timeout", true); break; }
        if (result == 1) { remember(s, f); dirty = true; }
        // Drain FIFO without screen or SD access while a packet is in flight.
        if (rx.busy()) { delayMicroseconds(80); continue; }
        if (check(EscPress)) break;
        if (check(NextPress) && !s.devices.empty()) { selected = (selected + 1) % s.devices.size(); dirty = true; }
        if (check(PrevPress) && !s.devices.empty()) { selected = (selected + s.devices.size() - 1) % s.devices.size(); dirty = true; }
        if (check(SelPress) && !s.devices.empty()) {
            rx.end();
            details(s.devices[selected]);
            if (returnToMenu || !rx.begin()) break;
            dirty = true;
        }
        if ((dirty || millis() - lastDraw > 2000) && millis() - lastDraw > 300) {
            drawList(s, selected); dirty = false; lastDraw = millis();
        }
        delay(1);
    }
    rx.end();
}
void exportSD(const Session &s) {
    if (s.devices.empty()) { displayInfo("No devices yet", true); return; }
    if (!setupSdCard()) { displayError("SD card not available", true); return; }
    if (!SD.exists("/wmbus") && !SD.mkdir("/wmbus")) { displayError("Cannot create /wmbus", true); return; }
    String path = "/wmbus/devices-" + String(millis());
    unsigned suffix = 0;
    String fileName;
    do { fileName = path + "-" + String(suffix++) + ".jsonl"; } while (SD.exists(fileName));
    File out = SD.open(fileName, FILE_WRITE);
    if (!out) { displayError("Cannot create export", true); return; }
    bool ok = true;
    for (const auto &d : s.devices) {
        const auto &f = d.frame;
        String row = "{\"id\":\"" + String(wmbus::identity(f).c_str()) + "\",\"manufacturer\":\"" +
                     String(wmbus::manufacturer(f).c_str()) + "\",\"mode\":\"" +
                     (f.mode == wmbus::Mode::T1 ? "T1-A" : f.mode == wmbus::Mode::C1A ? "C1-A" : "C1-B") +
                     "\",\"count\":" + String(d.count) + ",\"first_ms\":" + String(d.first) +
                     ",\"last_ms\":" + String(d.last) + ",\"rssi_dbm\":" + String(f.rssi) +
                     ",\"lqi\":" + String(f.lqi) + ",\"wire_l\":" + String(f.wireLength) +
                     ",\"crc_valid\":true,\"dll_crc_removed\":true,\"frame_hex\":\"" +
                     String(wmbus::hex(f.bytes.data(), f.bytes.size()).c_str()) + "\"}\n";
        if (out.print(row) != row.length()) { ok = false; break; }
    }
    out.flush();
    if (out.getWriteError()) ok = false;
    out.close();
    if (ok) displaySuccess("Saved " + fileName, true);
    else displayError("SD write failed: partial export", true);
}
} // namespace

void rf_wmbus() {
    Session session;
    session.devices.reserve(MAX_DEVICES);
    while (!returnToMenu) {
        std::vector<Option> menu = {
            {"Scan T1/C1 868.95", [&]() { scan(session); }},
            {"Export devices to SD", [&]() { exportSD(session); }},
            {"Clear results", [&]() { session = Session(); session.devices.reserve(MAX_DEVICES); }},
            {"Back", []() {}}
        };
        const int choice = loopOptions(menu, MENU_TYPE_SUBMENU, "Wireless M-Bus");
        if (choice < 0 || choice == int(menu.size()) - 1) break;
    }
}
