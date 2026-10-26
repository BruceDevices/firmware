#if !defined(LITE_VERSION)
#include "wire_tools.h"

#include "core/bus_HAL.h"
#include "core/display.h"
#include <globals.h>
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/settings.h"
#include "core/utils.h"
#include <Wire.h>

// Standard 7-bit probe range (0x00-0x07 and 0x78-0x7F are reserved addresses)
#define I2C_SCAN_FIRST 0x08
#define I2C_SCAN_LAST 0x77

/*********************************************************************
**  Helpers
**********************************************************************/
static TwoWire *i2c_acquire(int8_t sda, int8_t scl) {
    if (sda < 0) sda = (int8_t)bruceConfigPins.i2c_bus.sda;
    if (scl < 0) scl = (int8_t)bruceConfigPins.i2c_bus.scl;
    if (sda < 0 || scl < 0) return nullptr;
    return acquireI2CBus(sda, scl);
}

static int parseHex(String value) {
    value.trim();
    value.replace("0x", "");
    value.replace("0X", "");
    if (value.isEmpty()) return -1;
    return (int)strtol(value.c_str(), nullptr, 16);
}

/*********************************************************************
**  Serial-command primitives (wire i2c ...), see wire_tools.h
**********************************************************************/
int wire_i2c_scan(std::vector<uint8_t> &found, int8_t sda, int8_t scl) {
    found.clear();
    TwoWire *bus = i2c_acquire(sda, scl);
    if (bus == nullptr) return -1;

    for (uint8_t addr = I2C_SCAN_FIRST; addr <= I2C_SCAN_LAST; addr++) {
        bus->beginTransmission(addr);
        if (bus->endTransmission() == 0) found.push_back(addr);
    }
    releaseI2CBus();
    return (int)found.size();
}

String wire_i2c_read_reg(uint8_t addr, uint8_t reg, int8_t sda, int8_t scl) {
    TwoWire *bus = i2c_acquire(sda, scl);
    if (bus == nullptr) return "ERR: no bus pins";

    bus->beginTransmission(addr);
    bus->write(reg);
    if (bus->endTransmission(false) != 0) {
        releaseI2CBus();
        return "ERR: no ack (addr)";
    }
    if (bus->requestFrom((int)addr, 1) != 1) {
        releaseI2CBus();
        return "ERR: read failed";
    }
    uint8_t value = bus->read();
    releaseI2CBus();
    return "OK 0x" + String(value, HEX);
}

bool wire_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value, int8_t sda, int8_t scl) {
    TwoWire *bus = i2c_acquire(sda, scl);
    if (bus == nullptr) return false;

    bus->beginTransmission(addr);
    bus->write(reg);
    bus->write(value);
    bool ok = (bus->endTransmission() == 0);
    releaseI2CBus();
    return ok;
}

/*********************************************************************
**  Function: i2c_pick_register
**  HEX prompt with cancellation handling ("\x1B")
**********************************************************************/
static int i2c_pick_register(const char *msg) {
    String value = hex_keyboard("", 4, msg);
    if (value == "\x1B") return -1;
    int parsed = parseHex(value);
    if (parsed < 0 || parsed > 0xFF) {
        displayError("Register must be 0x00-FF", true);
        return -1;
    }
    return parsed;
}

/*********************************************************************
**  Function: i2c_device_menu
**  Per-device register read/write/dump on the picked address
**********************************************************************/
static void i2c_device_menu(uint8_t addr) {
    while (!returnToMenu) {
        options = {
            {String("Read Register"), [addr]() {
                 int reg = i2c_pick_register("Register address:");
                 if (reg < 0) return;
                 String result = wire_i2c_read_reg(addr, (uint8_t)reg, -1, -1);
                 displayInfo(String(addr, HEX) + ":reg " + String(reg, HEX) + " -> " + result, true);
             }},
            {String("Write Register"), [addr]() {
                 int reg = i2c_pick_register("Register address:");
                 if (reg < 0) return;
                 int value = i2c_pick_register("Value to write:");
                 if (value < 0) return;
                 bool ok = wire_i2c_write_reg(addr, (uint8_t)reg, (uint8_t)value, -1, -1);
                 if (ok) displaySuccess("Write OK", true);
                 else displayError("Write failed (no ack)", true);
             }},
            {String("Dump 256 Bytes"), [addr]() {
                 displayRedStripe("Dumping..");
                 ScrollableTextArea area(FP, BORDER_PAD_X, BORDER_PAD_Y, tftWidth - 2 * BORDER_PAD_X,
                                         tftHeight - BORDER_PAD_X - BORDER_PAD_Y, false, false);
                 bool any = false;
                 for (int base = 0; base < 256; base += 16) {
                     String row = String(base, HEX) + ":";
                     while (row.length() < 5) row = " " + row;
                     for (int reg = base; reg < base + 16; reg++) {
                         String result = wire_i2c_read_reg(addr, (uint8_t)reg, -1, -1);
                         String byteText = result.startsWith("OK") ? result.substring(3) : "--";
                         row += " " + byteText;
                         if (result.startsWith("OK")) any = true;
                     }
                     area.addLine(row);
                     if (check(EscPress)) break;
                 }
                 releaseI2CBus();
                 if (!any) {
                     displayError("Device stopped answering", true);
                     return;
                 }
                 area.draw(true);
                 while (!check(SelPress) && !check(EscPress)) { wakeUpScreen(); vTaskDelay(pdMS_TO_TICKS(20)); }
                 check(SelPress); check(EscPress);
             }},
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, ("I2C 0x" + String(addr, HEX)).c_str());
    }
}

/*********************************************************************
**  Function: i2c_scan_menu
**  Probe the bus and list responding addresses
**********************************************************************/
static void i2c_scan_menu() {
    std::vector<uint8_t> found;
    displayRedStripe("Scanning..");
    int count = wire_i2c_scan(found, -1, -1);
    if (count < 0) {
        displayError("Set I2C pins first", true);
        return;
    }
    if (count == 0) {
        displayInfo("No I2C devices found", true);
        return;
    }

    options.clear();
    for (uint8_t addr : found) {
        options.push_back({String("0x") + String(addr, HEX), [addr]() { i2c_device_menu(addr); }});
    }
    options.push_back({String("Scan again"), []() { i2c_scan_menu(); }});
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "I2C Devices");
}

/*********************************************************************
**  Function: i2c_bus_setup
**  Configuration menu: pins and scan
**********************************************************************/
void i2c_bus_setup() {
    returnToMenu = false;
    while (!returnToMenu) {
        BruceConfigPins::I2CPins pins = bruceConfigPins.i2c_bus;
        options = {
            {String("Pins: SDA ") + int(pins.sda) + " SCL " + int(pins.scl),
             []() { setI2CPinsMenu(bruceConfigPins.i2c_bus); }                             },
            {String("Scan Devices"),       i2c_scan_menu                                   },
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "I2C Bus");
    }
}
#endif
