#include "powerSave.h"
#include "core/led_control.h"
#include "display.h"
#include "mykeyboard.h"
#include "settings.h"

void fadeOutScreen(int startValue) {
    for (int brightValue = startValue; brightValue >= 0; brightValue -= 1) {
        setBrightness(max(brightValue, 0), false);
        delay(5);
    }
    turnOffDisplay();
}

void checkPowerSaveTime() {
    const bool dimmerEnabled = bruceConfig.dimmerSet > 0;
    const bool screenOffEnabled = bruceConfig.screenOffTimeout > 0;
    const bool autoSleepEnabled = bruceConfig.autoSleepTimeout > 0;
    const bool autoDeepSleepEnabled = bruceConfig.autoDeepSleepTimeout > 0;

    if (!dimmerEnabled && !screenOffEnabled && !autoSleepEnabled && !autoDeepSleepEnabled) return;

    unsigned long elapsed = millis() - previousMillis;
    int startDimmerBright = bruceConfig.bright / 3;
    const unsigned long dimmerSetMs = dimmerEnabled ? bruceConfig.dimmerSet * 1000UL : 0;
    const unsigned long screenOffMs = screenOffEnabled ? bruceConfig.screenOffTimeout * 1000UL : 0;
    const unsigned long autoSleepMs = autoSleepEnabled ? bruceConfig.autoSleepTimeout * 1000UL : 0;
    const unsigned long autoDeepSleepMs =
        autoDeepSleepEnabled ? bruceConfig.autoDeepSleepTimeout * 1000UL : 0;

    if (autoDeepSleepEnabled && elapsed >= autoDeepSleepMs) {
        goToDeepSleep();
        return;
    }

    if (isSleeping) {
        if (AnyKeyPress) {
            sleepModeOff();
            previousMillis = millis();
        }
        return;
    }

    if ((isScreenOff || dimmer) && AnyKeyPress) {
        if (wakeUpScreen()) {
            previousMillis = millis();
            return;
        }
    }

    if (dimmerEnabled && !dimmer && elapsed >= dimmerSetMs) {
        dimmer = true;
        setBrightness(startDimmerBright, false);
    }

    if (screenOffEnabled && !isScreenOff && elapsed >= screenOffMs) {
        isScreenOff = true;
        dimmer = true;
        fadeOutScreen(startDimmerBright);
        return;
    }

    if (autoSleepEnabled && elapsed >= autoSleepMs) {
        sleepModeOn();
    }
}

void sleepModeOn() {
    isSleeping = true;
    setCpuFrequencyMhz(80);

    int startDimmerBright = bruceConfig.bright / 3;

    fadeOutScreen(startDimmerBright);


    panelSleep(true); //  power down screen


    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();
    delay(200);
}

void sleepModeOff() {
    isSleeping = false;
    setCpuFrequencyMhz(240);


    panelSleep(false); // wake the screen back up


    ledSleepMode(false);
    getBrightness();
    enableCore0WDT();
    enableCore1WDT();
    enableLoopWDT();
    feedLoopWDT();
    delay(200);
}
