#include "i2c_finder.h"
#include "bus_HAL.h"
#include "display.h"
#include "mykeyboard.h"

#define FIRST_I2C_ADDRESS 0x08 // 0x00-0x07 and 0x78-0x7F are reserved
#define LAST_I2C_ADDRESS 0x77

namespace {
bool pinsConfigured(const BruceConfigPins::I2CPins &pins) { return pins.sda >= 0 && pins.scl >= 0; }

// sys_i2c and i2c_bus wired to the same pins are one physical bus: scanned and listed once.
bool busesShared() {
    return pinsConfigured(bruceConfigPins.sys_i2c) && bruceConfigPins.i2c_bus.sda == bruceConfigPins.sys_i2c.sda &&
           bruceConfigPins.i2c_bus.scl == bruceConfigPins.sys_i2c.scl;
}

// One line per bus: "SYS 8/18: 0x34, 0x55" / "EXT 2/1: none" / "EXT 2/1: unavailable"
void printBusScan(I2CBusId id, const String &label) {
    padprint(label + ": ");
    I2CLease lease = i2cLeaseBus(id);
    if (!lease) {
        // Configured but couldn't be opened: a line held low, or the SoC's only controller
        // belongs to sys_i2c.
        tft.print("unavailable");
        padprintln("");
        return;
    }
    bool first = true;
    for (uint8_t addr = FIRST_I2C_ADDRESS; addr <= LAST_I2C_ADDRESS; addr++) {
        if (!i2cProbe(lease, addr)) continue;
        if (!first) tft.print(", ");
        first = false;
        tft.printf("0x%X", addr);
    }
    if (first) tft.print("none");
    padprintln("");
}
} // namespace

void find_i2c_addresses() {
    drawMainBorderWithTitle("I2C Finder");
    padprintln("");
    padprintln("");

    padprintln("Checking I2C addresses ...\n\n");
    delay(300);

    bool sysConfigured = pinsConfigured(bruceConfigPins.sys_i2c);
    bool userConfigured = pinsConfigured(bruceConfigPins.i2c_bus);
    if (!sysConfigured && !userConfigured) {
        padprintln("No I2C bus configured");
    } else if (busesShared()) {
        printBusScan(
            I2CBusId::Sys,
            "SYS+EXT " + String((int)bruceConfigPins.sys_i2c.sda) + "/" + String((int)bruceConfigPins.sys_i2c.scl)
        );
    } else {
        if (sysConfigured) printBusScan(I2CBusId::Sys, i2cBusLabel(I2CBusId::Sys));
        if (userConfigured) printBusScan(I2CBusId::User, i2cBusLabel(I2CBusId::User));
    }

    while (1) {
        if (check(EscPress) || check(SelPress)) {
            returnToMenu = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// First address that answers on any bus (sys_i2c first), 0 if none.
uint8_t find_first_i2c_address() {
    uint8_t found = 0;
    i2cScanAll(
        [](I2CBusId, uint8_t addr, void *ctx) {
            uint8_t *first = static_cast<uint8_t *>(ctx);
            if (*first == 0) *first = addr;
        },
        &found
    );
    return found;
}

// True if the address answers on sys_i2c or i2c_bus.
bool check_i2c_address(uint8_t i2c_address) {
    I2CDeviceQuery query{i2c_address};
    return i2cLeaseDevice(query).found();
}
