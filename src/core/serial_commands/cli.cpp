#include "cli.h"
#include "badusb_commands.h"
#include "core/sd_functions.h"
#include "crypto_commands.h"
#include "gpio_commands.h"
#include "interpreter_commands.h"
#include "ir_commands.h"
#include "power_commands.h"
#include "rf_commands.h"
#include "rfid_commands.h"
#include "screen_commands.h"
#include "settings_commands.h"
#include "sound_commands.h"
#include "storage_commands.h"
#include "util_commands.h"
#include "wifi_commands.h"
#include <globals.h>

void cliErrorCallback(cmd_error *e) {
    CommandError cmdError(e); // Create wrapper object

    serialDevice->print("ERROR: ");
    serialDevice->println(cmdError.toString());

    if (cmdError.hasCommand()) {
        serialDevice->print("Did you mean \"");
        serialDevice->print(cmdError.getCommand().toString());
        serialDevice->println("\"?");
    }
}

SerialCli::SerialCli() { setup(); }

void SerialCli::setup() {
    _cli.setOnError(cliErrorCallback);

    createCryptoCommands(&_cli);
    createGpioCommands(&_cli);
    createIrCommands(&_cli);
    createPowerCommands(&_cli);
    createRfCommands(&_cli);
    createRfidCommands(&_cli);
    createSettingsCommands(&_cli);
    createStorageCommands(&_cli);
    createUtilCommands(&_cli);
    createWifiCommands(&_cli);

#ifdef USB_as_HID
    createBadUsbCommands(&_cli);
#endif
#ifndef LITE_VERSION
    createInterpreterCommands(&_cli);
#endif
#ifdef HAS_SCREEN
    createScreenCommands(&_cli);
#endif
#if defined(HAS_NS4168_SPKR) || defined(BUZZ_PIN)
    createSoundCommands(&_cli);
#endif
}
