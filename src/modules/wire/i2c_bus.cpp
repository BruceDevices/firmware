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
// Explicit pins win; otherwise the requested bus as configured.
static I2CLease i2c_open(I2CBusId bus, int8_t sda, int8_t scl) {
    if (sda >= 0 && scl >= 0) return i2cLeasePins(sda, scl);
    return i2cLeaseBus(bus);
}

static String i2c_read_reg_on(const I2CLease &lease, uint8_t addr, uint8_t reg) {
    TwoWire *bus = lease.wire();
    bus->beginTransmission(addr);
    bus->write(reg);
    if (bus->endTransmission(false) != 0) return "ERR: no ack (addr)";
    if (bus->requestFrom((int)addr, 1) != 1) return "ERR: read failed";
    return "OK 0x" + String(bus->read(), HEX);
}

static String i2c_write_reg_on(const I2CLease &lease, uint8_t addr, uint8_t reg, uint8_t value) {
    TwoWire *bus = lease.wire();
    bus->beginTransmission(addr);
    bus->write(reg);
    bus->write(value);
    return bus->endTransmission() == 0 ? "OK written" : "ERR: no ack";
}

static const char *i2c_bus_tag(I2CBusId bus) { return bus == I2CBusId::Sys ? "SYS" : "EXT"; }

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
int wire_i2c_scan(std::vector<WireI2cDevice> &found, int8_t sda, int8_t scl) {
    found.clear();
    if (sda >= 0 && scl >= 0) {
        I2CLease lease = i2cLeasePins(sda, scl);
        if (!lease) return -1;
        for (uint8_t addr = I2C_SCAN_FIRST; addr <= I2C_SCAN_LAST; addr++) {
            if (i2cProbe(lease, addr)) found.push_back({lease.bus(), addr});
        }
        return (int)found.size();
    }
    return i2cScanAll(
        [](I2CBusId bus, uint8_t addr, void *ctx) {
            static_cast<std::vector<WireI2cDevice> *>(ctx)->push_back({bus, addr});
        },
        &found
    );
}

String wire_i2c_read_reg(uint8_t addr, uint8_t reg, I2CBusId bus, int8_t sda, int8_t scl) {
    I2CLease lease = i2c_open(bus, sda, scl);
    if (!lease) return "ERR: bus unavailable";
    return i2c_read_reg_on(lease, addr, reg);
}

String wire_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value, I2CBusId bus, int8_t sda, int8_t scl) {
    I2CLease lease = i2c_open(bus, sda, scl);
    if (!lease) return "ERR: bus unavailable";
    return i2c_write_reg_on(lease, addr, reg, value);
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

// System peripherals (PMIC, IMU, RTC, touch...) live on sys_i2c: a stray write there can cut
// power or brick a setting, so it takes an explicit confirmation once per device menu.
static bool i2c_confirm_sys_write() {
    drawMainBorder(true);
    int8_t choice = displayMessage(
        "Write to the SYSTEM\nI2C bus?\nPMIC/IMU/RTC live here!", "No", nullptr, "Yes", TFT_RED
    );
    return choice == 1;
}

/*********************************************************************
**  Function: i2c_device_menu
**  Per-device register read/write/dump on the picked address
**********************************************************************/
static void i2c_device_menu(WireI2cDevice dev) {
    bool sysWriteAllowed = false;
    while (!returnToMenu) {
        options = {
            {String("Read Register"),
             [dev]() {
                 int reg = i2c_pick_register("Register address:");
                 if (reg < 0) return;
                 String result = wire_i2c_read_reg(dev.addr, (uint8_t)reg, dev.bus, -1, -1);
                 displayInfo(String(dev.addr, HEX) + ":reg " + String(reg, HEX) + " -> " + result, true);
             }},
            {String("Write Register"),
             [dev, &sysWriteAllowed]() {
                 if (dev.bus == I2CBusId::Sys && !sysWriteAllowed) {
                     if (!i2c_confirm_sys_write()) return;
                     sysWriteAllowed = true;
                 }
                 int reg = i2c_pick_register("Register address:");
                 if (reg < 0) return;
                 int value = i2c_pick_register("Value to write:");
                 if (value < 0) return;
                 String result =
                     wire_i2c_write_reg(dev.addr, (uint8_t)reg, (uint8_t)value, dev.bus, -1, -1);
                 if (result.startsWith("OK")) displaySuccess("Write OK", true);
                 else displayError("Write failed: " + result.substring(5), true);
             }},
            {String("Dump 256 Bytes"),
             [dev]() {
                 I2CLease lease = i2cLeaseBus(dev.bus); // one lease for the whole dump
                 if (!lease) {
                     displayError("I2C bus unavailable", true);
                     return;
                 }
                 displayRedStripe("Dumping..");
                 ScrollableTextArea area(
                     FP,
                     BORDER_PAD_X,
                     BORDER_PAD_Y,
                     tftWidth - 2 * BORDER_PAD_X,
                     tftHeight - BORDER_PAD_X - BORDER_PAD_Y,
                     false,
                     false
                 );
                 bool any = false;
                 for (int base = 0; base < 256; base += 16) {
                     String row = String(base, HEX) + ":";
                     while (row.length() < 5) row = " " + row;
                     for (int reg = base; reg < base + 16; reg++) {
                         String result = i2c_read_reg_on(lease, dev.addr, (uint8_t)reg);
                         String byteText = result.startsWith("OK") ? result.substring(3) : "--";
                         row += " " + byteText;
                         if (result.startsWith("OK")) any = true;
                     }
                     area.addLine(row);
                     if (check(EscPress)) break;
                 }
                 lease.release();
                 if (!any) {
                     displayError("Device stopped answering", true);
                     return;
                 }
                 area.draw(true);
                 while (!check(SelPress) && !check(EscPress)) {
                     wakeUpScreen();
                     vTaskDelay(pdMS_TO_TICKS(20));
                 }
                 check(SelPress);
                 check(EscPress);
             }},
        };
        addOptionToMainMenu();
        String title = "I2C 0x" + String(dev.addr, HEX) + " " + i2c_bus_tag(dev.bus);
        loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
    }
}

/*********************************************************************
**  Function: i2c_scan_menu
**  Probe sys_i2c and i2c_bus and list responding addresses per bus
**********************************************************************/
static void i2c_scan_menu() {
    std::vector<WireI2cDevice> found;
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
    for (const WireI2cDevice &dev : found) {
        options.push_back(
            {String("0x") + String(dev.addr, HEX) + " " + i2c_bus_tag(dev.bus),
             [dev]() { i2c_device_menu(dev); }}
        );
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
             []() { setI2CPinsMenu(bruceConfigPins.i2c_bus); }},
            {String("Scan Devices"), i2c_scan_menu},
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "I2C Bus");
    }
}
#endif
