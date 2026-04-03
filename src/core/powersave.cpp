#include "powerSave.h"
#include "display.h"
#include "settings.h"
#include <esp_sleep.h>
#include <driver/gpio.h>

/* Check if it's time to put the device to sleep */
#define SCREEN_OFF_DELAY 5000

void fadeOutScreen(int startValue) {
    // Use _setBrightness direct to avoid the 10ms delay in setBrightness()
    for (int brightValue = startValue; brightValue >= 0; brightValue--) {
        _setBrightness(max(brightValue, 0));
        delay(12);
    }
}

void fadeInScreen(int targetValue) {
    // Use _setBrightness direct to avoid the 10ms delay in setBrightness()
    for (int b = 0; b <= targetValue; b++) {
        _setBrightness(b);
        delay(12);
    }
}

/*********************************************************************
**  Weak hooks — override in board interface.cpp to detach/reattach
**  encoder interrupts, reset LEDs, etc. before/after light sleep.
**
**  Default implementations do nothing.
**********************************************************************/
void __attribute__((weak)) _screensaver_board_enter() {}
void __attribute__((weak)) _screensaver_board_exit()  {}

/*********************************************************************
**  Function: _screensaver_setup_wakeup
**  Configure GPIO wakeup sources for light sleep.
**
**  NOTE: encoder pins are intentionally NOT registered here.
**  They are handled via _screensaver_board_enter/exit hooks so that
**  their ISRs are detached cleanly and their LED state is preserved.
**********************************************************************/
static bool _screensaver_setup_wakeup() {
#if defined(CONFIG_IDF_TARGET_ESP32)
    // ESP32 classic: single ext0 wakeup on one RTC-capable GPIO
#if defined(DEEPSLEEP_WAKEUP_PIN) && DEEPSLEEP_WAKEUP_PIN >= 0
    esp_sleep_enable_ext0_wakeup(
        (gpio_num_t)DEEPSLEEP_WAKEUP_PIN,
        (DEEPSLEEP_PIN_ACT == LOW) ? 0 : 1
    );
    return true;
#else
    return false;
#endif

#else
    // ESP32-S3/S2/C3/C5/C6/H2: gpio_wakeup on any GPIO
    bool any = false;
    auto _add = [&](int pin, int act_level) {
        if (pin < 0) return;
        gpio_int_type_t lvl = (act_level == LOW) ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL;
        if (gpio_wakeup_enable((gpio_num_t)pin, lvl) == ESP_OK) any = true;
    };

#if defined(DEEPSLEEP_WAKEUP_PIN) && DEEPSLEEP_WAKEUP_PIN >= 0
    _add(DEEPSLEEP_WAKEUP_PIN, DEEPSLEEP_PIN_ACT);
#endif
#if defined(SEL_BTN) && defined(BTN_ACT)
    _add(SEL_BTN, BTN_ACT);
#endif
#if defined(BK_BTN) && defined(BTN_ACT)
    _add(BK_BTN, BTN_ACT);
#endif

    if (any) esp_sleep_enable_gpio_wakeup();
    return any;
#endif
}

/*********************************************************************
**  Function: enterScreensaver
**
**  Safe low-power screensaver for all Bruce boards.
**
**  Key design decisions vs previous version:
**
**  1. xHandle (taskInputHandler) is SUSPENDED before sleep.
**     This prevents the task running on the other core from reading
**     encoder state and setting EncoderLedChange while we sleep.
**
**  2. WDT is NOT touched (no disable/enable).
**     Disabling WDT from a FreeRTOS task causes crashes on resume.
**     esp_light_sleep_start() is safe to call without touching WDT.
**
**  3. Board hooks (_screensaver_board_enter/exit) let interface.cpp
**     cleanly detach encoder IRQs and reset per-board peripherals.
**
**  4. Encoder pins are NOT used as wakeup sources.
**     Wakeup is via dedicated button GPIOs only; the encoder hook
**     re-enables encoder input after wakeup.
**********************************************************************/
void enterScreensaver() {
    int savedBright = (currentScreenBrightness > 0) ? currentScreenBrightness : bruceConfig.bright;

    // Block wakeUpScreen() from instantly restoring brightness (would flash)
    isScreensaverActive = true;
    isScreenOff         = true;
    dimmer              = false;

    // ── 1. Suspend InputHandler task (stops encoder/button reading on other core)
    if (xHandle != NULL) vTaskSuspend(xHandle);

    // ── 2. Board-specific pre-sleep hook (detach encoder ISRs, etc.)
    _screensaver_board_enter();

    // ── 3. Smooth fade-out - use _setBrightness to avoid embedded delay
    for (int b = savedBright; b >= 0; b--) {
        _setBrightness(b);
        delay(8);
    }

    // ── 4. Panel off (stops SPI/backlight draw) - no delay to maintain smooth fade
    panelSleep(true);

    // ── 5. CPU → 10 MHz (~20× power reduction)
    setCpuFrequencyMhz(10);

    // ── 6. Configure GPIO wakeup sources
    bool canLightSleep = _screensaver_setup_wakeup();

    // Drain any stale input
    AnyKeyPress = false;
    NextPress   = false;
    PrevPress   = false;
    UpPress     = false;
    DownPress   = false;
    SelPress    = false;
    EscPress    = false;
#ifdef HAS_ENCODER_LED
    EncoderLedChange = 0;
#endif

    // ── 7. Sleep loop (no WDT manipulation needed)
    if (canLightSleep) {
        // CPU fully halted between wakeup events — near-zero active current
        esp_err_t ret;
        do {
            ret = esp_light_sleep_start();
        } while (ret != ESP_OK || esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED);

#if !defined(CONFIG_IDF_TARGET_ESP32)
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
#endif
    } else {
        // Fallback: 10 MHz polling — still ~20× better than normal idle
        while (!AnyKeyPress) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    // ── 8. Restore CPU frequency
    setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);

    // ── 9. Panel on - minimal delay for wakeup
    panelSleep(false);

    // ── 10. Board-specific post-sleep hook (re-attach encoder ISRs, reset LED state)
    _screensaver_board_exit();

    // ── 11. Resume InputHandler task
    if (xHandle != NULL) vTaskResume(xHandle);

    // Consume the wakeup event so it doesn't accidentally navigate the menu
    AnyKeyPress = false;
    NextPress   = false;
    PrevPress   = false;
    UpPress     = false;
    DownPress   = false;
    SelPress    = false;
    EscPress    = false;
#ifdef HAS_ENCODER_LED
    EncoderLedChange = 0;
#endif

    // ── 12. Smooth fade-in - use _setBrightness to avoid embedded delay
    for (int b = 0; b <= savedBright; b++) {
        _setBrightness(b);
        delay(8);
    }
    currentScreenBrightness = savedBright;

    // Restore flags AFTER fade-in (prevents wakeUpScreen from re-triggering)
    isScreensaverActive = false;
    isScreenOff         = false;
    dimmer              = false;
    previousMillis      = millis();
}

void checkPowerSaveTime() {
    if (bruceConfig.dimmerSet == 0) return;

    unsigned long elapsed = millis() - previousMillis;
    int startDimmerBright = bruceConfig.bright / 3;
    int dimmerSetMs       = bruceConfig.dimmerSet * 1000;

    if (elapsed >= dimmerSetMs && !dimmer && !isSleeping) {
        dimmer = true;
        setBrightness(startDimmerBright, false);
    } else if (elapsed >= (dimmerSetMs + SCREEN_OFF_DELAY) && !isScreenOff && !isSleeping) {
        isScreenOff = true;
        fadeOutScreen(startDimmerBright);
    }
}

void sleepModeOn() {
    isSleeping = true;
    setCpuFrequencyMhz(80);
    int startDimmerBright = bruceConfig.bright / 3;
    fadeOutScreen(startDimmerBright);
    panelSleep(true);
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
    panelSleep(false);
    getBrightness();
    enableCore0WDT();
#if SOC_CPU_CORES_NUM > 1
    enableCore1WDT();
#endif
    enableLoopWDT();
    feedLoopWDT();
    delay(200);
}
