#include "powerSave.h"
#include "display.h"
#include "settings.h"
#include "power_save_timing.h"

/* Check if it's time to put the device to sleep */
#define SCREEN_OFF_DELAY 5000

void fadeOutScreen(int startValue) {
    for (int brightValue = startValue; brightValue >= 0; brightValue -= 1) {
        setBrightness(max(brightValue, 0), false);
        delay(5);
    }
    turnOffDisplay();
}

static void updatePowerSaveTime() {
    static int lastFadeBrightness = -1;
    static unsigned long fadeActivity = 0;
    const unsigned long activity = previousMillis.load(std::memory_order_relaxed);
    if (fadeActivity != activity || !dimmer) {
        lastFadeBrightness = -1;
        fadeActivity = activity;
    }
    if (bruceConfig.dimmerSet == 0 || isSleeping) {
        lastFadeBrightness = -1;
        return;
    }

    const uint32_t elapsed = uint32_t(millis()) - uint32_t(activity);
    int startDimmerBright = min(max(bruceConfig.bright, 0), 100) / 3;
    const uint32_t dimmerSetMs = uint32_t(bruceConfig.dimmerSet) * 1000;

    if (previousMillis.load(std::memory_order_relaxed) != activity) return;
    if (elapsed >= dimmerSetMs && !dimmer) {
        dimmer = true;
        setBrightness(startDimmerBright, false);
        lastFadeBrightness = startDimmerBright;
    } else if (elapsed >= (dimmerSetMs + SCREEN_OFF_DELAY)) {
        isScreenOff = true;
        const int brightValue = idleFadeBrightness(startDimmerBright, elapsed - dimmerSetMs - SCREEN_OFF_DELAY);
        if (brightValue != lastFadeBrightness) {
            setBrightness(brightValue, false);
            lastFadeBrightness = brightValue;
        }
    }
}

void checkPowerSaveTime() {
    // wakeUpScreen() uses the same recursive mutex. A completed wake cannot be
    // overwritten by a stale fade step from the input task or another caller.
    inputLock();
    updatePowerSaveTime();
    inputUnlock();
}

void sleepModeOn() {
    isSleeping = true;
    setCpuFrequencyMhz(80);

    int startDimmerBright = bruceConfig.bright / 3;

    fadeOutScreen(startDimmerBright);

    panelSleep(true); //  power down screen

    disableCore0WDT();
#if SOC_CPU_CORES_NUM > 1
    disableCore1WDT();
#endif
    disableLoopWDT();
    delay(200);
}

void sleepModeOff() {
    isSleeping = false;
    setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);

    panelSleep(false); // wake the screen back up

    getBrightness();
    enableCore0WDT();
#if SOC_CPU_CORES_NUM > 1
    enableCore1WDT();
#endif
    enableLoopWDT();
    feedLoopWDT();
    delay(200);
}
