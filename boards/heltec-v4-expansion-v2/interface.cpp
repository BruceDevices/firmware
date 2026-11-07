#include "board_input.h"
// Expansion hardware sequencing and touch reads adapted from LORA Tunnel;
// see NOTICE.md. Bruce input/display integration is specific to this port.
#include "core/bus_HAL.h"
#include "core/display.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include <atomic>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <interface.h>

namespace {
heltec_v4::Button userButton;
heltec_v4::Button nextButton;
heltec_v4::InputQueue inputQueue;
bool touchReady = false;
bool backlightReady = false;
bool touchDown = false;
bool touchWokeScreen = false;
uint32_t touchRepeatAt = 0;
uint32_t droppedInputs = 0;
std::atomic<bool> sleepPauseRequested{false};
std::atomic<bool> sleepPauseAcknowledged{false};

bool pauseInputForSleep() {
    if (xHandle == nullptr) return true;
    // Never wait for an acknowledgement from the current task. Power actions
    // are dispatched by the UI task, not from the input-polling task.
    if (xTaskGetCurrentTaskHandle() == xHandle) return false;
    sleepPauseRequested.store(true, std::memory_order_release);
    const uint32_t started = millis();
    while (!sleepPauseAcknowledged.load(std::memory_order_acquire)) {
        if (uint32_t(millis() - started) >= 500) {
            sleepPauseRequested.store(false, std::memory_order_release);
            return false;
        }
        vTaskDelay(1);
    }
    return true;
}

void queueInput(const heltec_v4::InputEvent &event) {
    if (!inputQueue.push(event) && ++droppedInputs == 1) {
        Serial.println("[Heltec V4] Input queue full; dropping newest input");
    }
}

struct SleepOutput {
    gpio_num_t pin;
    uint8_t level;
};
constexpr SleepOutput sleepOutputs[] = {
    {gpio_num_t(HELTEC_VEXT_ENABLE), HIGH},
    {gpio_num_t(HELTEC_GNSS_ENABLE), HIGH},
    {gpio_num_t(TFT_BL), LOW},
    {gpio_num_t(LORA_FEM_ENABLE), LOW},
    {gpio_num_t(LORA_FEM_CTX), LOW},
    {gpio_num_t(LORA_FEM_POWER), LOW},
    {gpio_num_t(LORA_RST), LOW},
    {gpio_num_t(BUZZ_PIN), LOW},
};

void outputLevel(int pin, uint8_t level) {
    // Set the latch before enabling the output; Arduino 3 digitalWrite() alone
    // does not set the latch until periman knows the pin is a GPIO output.
    gpio_set_level(gpio_num_t(pin), level);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, level);
}

void releaseSleepHolds() {
    gpio_deep_sleep_hold_dis();
    for (const auto &output : sleepOutputs) {
        gpio_set_level(output.pin, output.level);
        gpio_set_direction(output.pin, GPIO_MODE_OUTPUT);
        gpio_hold_dis(output.pin);
    }
    rtc_gpio_deinit(gpio_num_t(HELTEC_USER_BUTTON));
}

bool readTouch(uint16_t &x, uint16_t &y) {
    if (!touchReady) return false;
    Wire1.beginTransmission(HELTEC_TOUCH_ADDRESS);
    Wire1.write(uint8_t(0));
    if (Wire1.endTransmission(true) != 0) return false;
    uint8_t report[7];
    if (Wire1.requestFrom(uint8_t(HELTEC_TOUCH_ADDRESS), sizeof(report), true) != sizeof(report)) {
        while (Wire1.available()) Wire1.read();
        return false;
    }
    for (auto &byte : report) byte = uint8_t(Wire1.read());
    if (!heltec_v4::decodeTouch(report, sizeof(report), x, y)) return false;
    heltec_v4::rotateTouch(bruceConfigPins.rotation, x, y);
    return true;
}

void prepareDeepSleep() {
    if (xHandle != nullptr && xTaskGetCurrentTaskHandle() == xHandle) {
        Serial.println("[Heltec V4] Request sleep from the UI, not the input task");
        return;
    }
    // USER is also the wake source. A held key would immediately wake us.
    if (digitalRead(HELTEC_USER_BUTTON) == LOW) {
        displayWarning("Release USER, then retry", true);
        return;
    }
    const gpio_num_t wake = gpio_num_t(HELTEC_USER_BUTTON);
    if (esp_sleep_enable_ext0_wakeup(wake, LOW) != ESP_OK) {
        displayError("Wake source unavailable", true);
        return;
    }
    rtc_gpio_pullup_en(wake);
    rtc_gpio_pulldown_dis(wake);

    // Park polling at a boundary outside Wire transactions. Arbitrarily
    // suspending that task can strand its Wire mutex and deadlock Wire1.end().
    if (!pauseInputForSleep()) {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
        rtc_gpio_deinit(wake);
        pinMode(HELTEC_USER_BUTTON, INPUT_PULLUP);
        displayError("Input busy; retry sleep", true);
        return;
    }
    if (sdcardMounted) SD.end();
    sdcardMounted = false;
    _setBrightness(0);
    panelSleep(true);
    Wire1.end();
    touchReady = false;
    if (backlightReady) {
        ledcDetach(TFT_BL);
        backlightReady = false;
    }
    noTone(BUZZ_PIN);

    // Remove the front-end enable before its supply, and hold the SX1262 reset.
    outputLevel(LORA_FEM_ENABLE, LOW);
    outputLevel(LORA_FEM_CTX, LOW);
    outputLevel(LORA_FEM_POWER, LOW);
    constexpr int floatingPins[] = {
        TFT_SCLK, TFT_MOSI, TFT_MISO, TFT_CS, TFT_DC, TFT_RST, SDCARD_CS,
        SYS_I2C_SDA, SYS_I2C_SCL, GPS_SERIAL_RX, GPS_SERIAL_TX, HELTEC_GNSS_PPS,
        LORA_SCK, LORA_MOSI, LORA_MISO, LORA_CS, LORA_IRQ, LORA_BUSY,
    };
    for (int pin : floatingPins) {
        pinMode(pin, INPUT);
        gpio_set_pull_mode(gpio_num_t(pin), GPIO_FLOATING);
    }
    for (const auto &output : sleepOutputs) {
        outputLevel(output.pin, output.level);
        gpio_hold_en(output.pin);
    }
    gpio_deep_sleep_hold_en();
    esp_deep_sleep_start();
}
} // namespace

void _setup_gpio() {
    releaseSleepHolds();
    outputLevel(LORA_FEM_ENABLE, LOW);
    outputLevel(LORA_FEM_CTX, LOW);
    outputLevel(LORA_FEM_POWER, LOW);
    outputLevel(LORA_CS, HIGH);
    outputLevel(SDCARD_CS, HIGH);
    outputLevel(TFT_CS, HIGH);
    outputLevel(TFT_BL, LOW);
    outputLevel(BUZZ_PIN, LOW);
    outputLevel(HELTEC_VEXT_ENABLE, LOW);
    outputLevel(HELTEC_GNSS_ENABLE, LOW);
    pinMode(HELTEC_GNSS_PPS, INPUT);
    pinMode(HELTEC_USER_BUTTON, INPUT_PULLUP);
    pinMode(HELTEC_NEXT_BUTTON, INPUT);
    const uint32_t now = millis();
    userButton.begin(digitalRead(HELTEC_USER_BUTTON) == LOW, now);
    nextButton.begin(digitalRead(HELTEC_NEXT_BUTTON) == HIGH, now);
    analogSetPinAttenuation(HELTEC_BATTERY_ADC, ADC_2_5db);
    setSysI2CBus(&Wire1);
    delay(1000); // VEXT and expansion peripherals settle before the first TFT reset.
}

void _pre_storage_gpio() {
    // TFT_eSPI has completed its hardware reset. GPIO21 also resets touch, so
    // do not reset the touchscreen separately after the display is running.
    Wire1.begin(SYS_I2C_SDA, SYS_I2C_SCL, 100000);
    Wire1.setTimeOut(10);
    delay(60);
    Wire1.beginTransmission(HELTEC_TOUCH_ADDRESS);
    touchReady = Wire1.endTransmission() == 0;
    Serial.printf("[Heltec V4] CHSC6xxx touch: %s\n", touchReady ? "ready" : "not found; use buttons");
    backlightReady = ledcAttach(TFT_BL, 20000, 8);
    _setBrightness(100);
}

void _post_setup_gpio() {
    // begin_tft() restores the saved brightness before this hook.
}

int getBattery() {
    static uint32_t sampledAt = 0;
    static int percent = 0;
    const uint32_t now = millis();
    if (sampledAt != 0 && uint32_t(now - sampledAt) < 5000) return percent;
    uint32_t sum = 0;
    for (unsigned i = 0; i < 8; ++i) sum += analogReadMilliVolts(HELTEC_BATTERY_ADC);
    const uint32_t millivolts = (sum * 49) / 80; // 390k/100k divider: 4.9x.
    // Voltage estimate only: no fuel gauge or readable charger-status signal.
    percent = millivolts >= 2500 && millivolts <= 4500
                  ? constrain((int(millivolts) - 3300) * 100 / 900, 1, 100)
                  : 0;
    sampledAt = now;
    return percent;
}

bool isCharging() { return false; }

void _setBrightness(uint8_t brightval) {
    if (!backlightReady) return;
    const uint32_t percent = min(uint32_t(brightval), uint32_t(100));
    const uint32_t duty = percent == 0 ? 0 : MINBRIGHT + ((255 - MINBRIGHT) * percent) / 100;
    ledcWrite(TFT_BL, duty);
}

// Called by the input task on every tick, before its event-consumption gate.
// Both sampling and publication stay in that task; no additional task/heap is used.
void _pollInput() {
    if (sleepPauseRequested.load(std::memory_order_acquire)) {
        sleepPauseAcknowledged.store(true, std::memory_order_release);
        // Stay inside this hook so the generic task cannot run bus recovery or
        // publish more input while the UI shuts down the peripheral supplies.
        while (sleepPauseRequested.load(std::memory_order_acquire)) vTaskDelay(1);
        sleepPauseAcknowledged.store(false, std::memory_order_release);
        return;
    }
    const uint32_t now = millis();
    const auto user = userButton.update(digitalRead(HELTEC_USER_BUTTON) == LOW, now);
    const auto next = nextButton.update(digitalRead(HELTEC_NEXT_BUTTON) == HIGH, now);
    if (user != heltec_v4::ButtonEvent::None || next != heltec_v4::ButtonEvent::None) {
        if (!wakeUpScreen()) {
            heltec_v4::InputEvent event;
            if (user == heltec_v4::ButtonEvent::Click) event.actions |= heltec_v4::Select;
            if (user == heltec_v4::ButtonEvent::Hold) event.actions |= heltec_v4::Back;
            if (next == heltec_v4::ButtonEvent::Click) event.actions |= heltec_v4::Next;
            if (next == heltec_v4::ButtonEvent::Hold) event.actions |= heltec_v4::Previous;
            queueInput(event);
        }
    }

    uint16_t x = 0, y = 0;
    if (!readTouch(x, y)) {
        touchDown = false;
        touchWokeScreen = false;
        return;
    }
    const bool first = !touchDown;
    touchDown = true;
    if (first) {
        touchRepeatAt = now + 450;
        touchWokeScreen = wakeUpScreen();
    }
    if (touchWokeScreen) return;
    if (!first && (!LongPress || int32_t(now - touchRepeatAt) < 0)) return;
    if (!first) {
        touchRepeatAt = now + 200;
        wakeUpScreen(); // Keep the dimmer awake during deliberate repeat input.
    }
    queueInput({heltec_v4::Touch, x, y});
}

void InputHandler() {
    if (sleepPauseRequested.load(std::memory_order_acquire)) return;
    heltec_v4::InputEvent event;
    if (!inputQueue.pop(event)) return;
    AnyKeyPress = true;
    SelPress = (event.actions & heltec_v4::Select) != 0;
    EscPress = (event.actions & heltec_v4::Back) != 0;
    NextPress = (event.actions & heltec_v4::Next) != 0;
    PrevPress = (event.actions & heltec_v4::Previous) != 0;
    if (event.actions & heltec_v4::Touch) {
        touchPoint.x = event.x;
        touchPoint.y = event.y;
        touchPoint.pressed = true;
        touchHeatMap(touchPoint);
    }
}

// Software cannot operate the expansion board's physical battery latch.
void powerOff() { prepareDeepSleep(); }
void goToDeepSleep() { prepareDeepSleep(); }
void checkReboot() {}
