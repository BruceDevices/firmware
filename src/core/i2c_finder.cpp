#include "i2c_finder.h"
#include "bus_HAL.h"
#include "display.h"
#include "mykeyboard.h"

#define FIRST_I2C_ADDRESS 0x01
#define LAST_I2C_ADDRESS 0x7F

void find_i2c_addresses() {
    drawMainBorderWithTitle("I2C Finder");
    padprintln("");
    padprintln("");

    bool first_found = true;
    TwoWire *Wire = acquireI2CBus();

    padprintln("Checking I2C addresses ...\n\n");
    delay(300);

    padprint("Found: ");

    for (uint8_t i = FIRST_I2C_ADDRESS; i <= LAST_I2C_ADDRESS; i++) {
        Wire->beginTransmission(i);
        if (Wire->endTransmission() == 0) {
            if (!first_found) tft.print(", ");
            else first_found = false;
            tft.printf("0x%X", i);
        }
    }

    while (1) {
        if (check(EscPress) || check(SelPress)) {
            returnToMenu = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    releaseI2CBus();
}

uint8_t find_first_i2c_address() {
    TwoWire *Wire = acquireI2CBus();
    uint8_t found = 0;
    for (uint8_t i = FIRST_I2C_ADDRESS; i <= LAST_I2C_ADDRESS; i++) {
        Wire->beginTransmission(i);
        if (Wire->endTransmission() == 0) {
            found = i;
            break;
        }
    }
    releaseI2CBus();
    return found;
}

bool check_i2c_address(uint8_t i2c_address) {
    TwoWire *Wire = acquireI2CBus();
    Wire->beginTransmission(i2c_address);
    int error = Wire->endTransmission();
    releaseI2CBus();
    return (error == 0);
}
