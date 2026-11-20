#include "power_commands.h"
#include "core/settings.h"
#include <globals.h>

uint32_t poweroffCallback(cmd *c) {
    powerOff();
#ifndef HELTEC_V4_EXPANSION_V2
    esp_deep_sleep_start(); // only wake up via hardware reset
#endif
    // The Heltec hook configures USER wake and may return after refusing sleep
    // with a held wake key. Do not bypass that refusal with an unarmed sleep.
    return true;
}

uint32_t rebootCallback(cmd *c) {
    ESP.restart();
    return true;
}

uint32_t sleepCallback(cmd *c) {
    setSleepMode();
    return true;
}

void createPoweroffCommand(SimpleCLI *cli) { Command cmd = cli->addCommand("poweroff", poweroffCallback); }

void createRebootCommand(SimpleCLI *cli) { Command cmd = cli->addCommand("reboot", rebootCallback); }

void createSleepCommand(SimpleCLI *cli) { Command cmd = cli->addCommand("sleep", sleepCallback); }

void createPowerCommand(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCommand("power");

    Command cmdOff = cmd.addCommand("off", poweroffCallback);
    Command cmdReboot = cmd.addCommand("reboot", rebootCallback);
    Command cmdSleep = cmd.addCommand("sleep", sleepCallback);
}

void createPowerCommands(SimpleCLI *cli) {
    createPoweroffCommand(cli);
    createRebootCommand(cli);
    createSleepCommand(cli);

    createPowerCommand(cli);
}
