#include "powerSave.h"
#include "core/led_control.h"
#include "display.h"
#include "mykeyboard.h"
#include "settings.h"

namespace {
struct PowerProfileSettings {
    float dimmerScale;
    float screenOffScale;
    float autoSleepScale;
    float autoDeepSleepScale;
    int awakeCpuFreqMhz;
};

constexpr PowerProfileSettings kPowerProfiles[] = {
    {0.5f, 0.5f, 0.5f, 0.5f, 80 },  // Aggressive
    {1.0f, 1.0f, 1.0f, 1.0f, 160},  // Balanced
    {1.75f, 2.0f, 1.5f, 2.0f, 240}, // Performance
};

int normalizePowerMode(int mode) {
    if (mode < POWER_MODE_AGGRESSIVE || mode > POWER_MODE_PERFORMANCE) return POWER_MODE_PERFORMANCE;
    return mode;
}

unsigned long scaledTimeoutMs(int seconds, float scale) {
    if (seconds <= 0 || scale <= 0.0f) return 0;
    float scaledSeconds = seconds * scale;
    if (scaledSeconds < 0.5f) scaledSeconds = 0.5f;
    return static_cast<unsigned long>(scaledSeconds * 1000.0f);
}

int lastAppliedMode = -1;
int lastAwakeFreqMhz = 0;

void resetAwakeCpuCache() {
    lastAppliedMode = -1;
    lastAwakeFreqMhz = 0;
}

void applyAwakeCpuFrequency(int mode, const PowerProfileSettings &profile) {
    if (isSleeping) return;
    if (lastAppliedMode == mode && lastAwakeFreqMhz == profile.awakeCpuFreqMhz) return;
    if (profile.awakeCpuFreqMhz > 0) setCpuFrequencyMhz(profile.awakeCpuFreqMhz);
    lastAppliedMode = mode;
    lastAwakeFreqMhz = profile.awakeCpuFreqMhz;
}
} // namespace

void fadeOutScreen(int startValue) {
    for (int brightValue = startValue; brightValue >= 0; brightValue -= 1) {
        setBrightness(max(brightValue, 0), false);
        delay(5);
    }
    turnOffDisplay();
}

void checkPowerSaveTime() {
    int mode = normalizePowerMode(bruceConfig.powerMode);
    const auto &profile = kPowerProfiles[mode];
    applyAwakeCpuFrequency(mode, profile);

    const bool dimmerEnabled = bruceConfig.dimmerSet > 0;
    const bool screenOffEnabled = bruceConfig.screenOffTimeout > 0;
    const bool autoSleepEnabled = bruceConfig.autoSleepTimeout > 0;
    const bool autoDeepSleepEnabled = bruceConfig.autoDeepSleepTimeout > 0;

    if (!dimmerEnabled && !screenOffEnabled && !autoSleepEnabled && !autoDeepSleepEnabled) return;

    unsigned long elapsed = millis() - previousMillis;
    int startDimmerBright = bruceConfig.bright / 3;
    const unsigned long dimmerSetMs =
        dimmerEnabled ? scaledTimeoutMs(bruceConfig.dimmerSet, profile.dimmerScale) : 0;
    const unsigned long screenOffMs =
        screenOffEnabled ? scaledTimeoutMs(bruceConfig.screenOffTimeout, profile.screenOffScale) : 0;
    const unsigned long autoSleepMs =
        autoSleepEnabled ? scaledTimeoutMs(bruceConfig.autoSleepTimeout, profile.autoSleepScale) : 0;
    const unsigned long autoDeepSleepMs = autoDeepSleepEnabled
        ? scaledTimeoutMs(bruceConfig.autoDeepSleepTimeout, profile.autoDeepSleepScale)
        : 0;

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
    resetAwakeCpuCache();

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
    int mode = normalizePowerMode(bruceConfig.powerMode);
    applyAwakeCpuFrequency(mode, kPowerProfiles[mode]);


    panelSleep(false); // wake the screen back up


    ledSleepMode(false);
    getBrightness();
    enableCore0WDT();
    enableCore1WDT();
    enableLoopWDT();
    feedLoopWDT();
    delay(200);
}
