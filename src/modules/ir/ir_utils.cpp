#include "ir_utils.h"

void setup_ir_pin(int pin, uint8_t mode) {
    if (bruceConfigPins.SDCARD_bus.checkConflict(pin)) sdcardSPI.end();
    gpio_reset_pin((gpio_num_t)pin);
    pinMode(pin, mode);
}
