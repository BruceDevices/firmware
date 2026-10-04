#include "gpio_commands.h"
#include <globals.h>

bool is_free_gpio_pin(int pin_no) {
    // check if pin_no is usable for general GPIO
    std::vector<int> usable_pins = {bruceConfigPins.i2c_bus.sda, bruceConfigPins.i2c_bus.scl};

#if defined(ARDUINO_M5STICK_C_PLUS2) || defined(ARDUINO_M5STICK_C_PLUS)
    usable_pins.insert(usable_pins.end(), {25, 26, 32, 33, 0});
#elif defined(ESP32S3DEVKITC1)
    usable_pins.insert(
        usable_pins.end(),
        {
            1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
            14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // GPIO1 to GPIO25
            33,                                             // GPIO33
            38, 39, 40, 41, 42,                             // GPIO38 to GPIO42
            47, 48                                          // GPIO47 to GPIO48
        }
    );
#endif

    for (int usable_pin : usable_pins)
        if (pin_no == usable_pin) return true;

    return false;
}

uint32_t gpioModeCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument(0);
    String args = arg.getValue();
    args.trim();

    // const char* args = cmd_str.c_str() + strlen("gpio mode ");
    int pin_number = -1;
    int mode = 0;

    if (sscanf(args.c_str(), "%d %d", &pin_number, &mode) == 2) {
        // check usable pins according to the env
        if (mode >= 0 && mode <= 9 && is_free_gpio_pin(pin_number)) {
            pinMode(pin_number, mode);
            return true;
        }
    }

    serialDevice->print("Invalid args: ");
    serialDevice->println(args);
    return false;
}

uint32_t gpioSetCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument(0);
    String args = arg.getValue();
    args.trim();

    int pin_number = -1;
    int value = 0;

    if (sscanf(args.c_str(), "%d %d", &pin_number, &value) == 2) {
        // check usable pins according to the env
        if (value >= 0 && value <= 1 && is_free_gpio_pin(pin_number)) {
            digitalWrite(pin_number, value);
            return true;
        }
    }

    serialDevice->print("Invalid args: ");
    serialDevice->println(args);
    return false;
}

uint32_t gpioReadCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument(0);
    String args = arg.getValue();
    args.trim();

    int pin_number = -1;

    if (sscanf(args.c_str(), "%d", &pin_number) == 1) {
        // check usable pins according to the env
        if (is_free_gpio_pin(pin_number)) {
            serialDevice->println(digitalRead(pin_number));
            return true;
        }
    }

    serialDevice->print("Invalid args: ");
    serialDevice->println(args);
    return false;
}

void createGpioCommands(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCmd("gpio");

    cmd.addSingleArgumentCommand("mode", gpioModeCallback);
    cmd.addSingleArgumentCommand("set", gpioSetCallback);
    cmd.addSingleArgumentCommand("read", gpioReadCallback);
}
