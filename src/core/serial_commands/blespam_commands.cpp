#include "blespam_commands.h"
#include "modules/ble/ble_spam.h"
#include <globals.h>

static uint32_t blespamAppleCallback(cmd *c) {
    bleSpamStart(0);
    serialDevice->println("BLE Spam: Apple Continuity");
    return true;
}

static uint32_t blespamSamsungCallback(cmd *c) {
    bleSpamStart(2);
    serialDevice->println("BLE Spam: Samsung");
    return true;
}

static uint32_t blespamAndroidCallback(cmd *c) {
    bleSpamStart(1);
    serialDevice->println("BLE Spam: Google FastPair");
    return true;
}

static uint32_t blespamWindowsCallback(cmd *c) {
    bleSpamStart(3);
    serialDevice->println("BLE Spam: Microsoft SwiftPair");
    return true;
}

static uint32_t blespamStopCallback(cmd *c) {
    bleSpamStop();
    serialDevice->println("BLE Spam stopped");
    return true;
}

void createBleSpamCommands(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCmd("blespam");

    cmd.addCommand("apple", blespamAppleCallback);
    cmd.addCommand("samsung", blespamSamsungCallback);
    cmd.addCommand("android", blespamAndroidCallback);
    cmd.addCommand("windows", blespamWindowsCallback);
    cmd.addCommand("stop", blespamStopCallback);
}
