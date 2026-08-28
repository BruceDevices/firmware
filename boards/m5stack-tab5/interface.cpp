#include "core/powerSave.h"
#include "core/utils.h"
#include "esp32-hal-hosted.h"
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <WiFi.h>
#include <Wire.h>
#include <interface.h>

/***************************************************************************************
** Tab5 built-in keyboard (M5Unit-KEYBOARD) integration
**
** The keyboard hangs off ExtPort1 (internal 10-pin connector):
**   INT = GPIO50 (J9 pin 10)   SDA = GPIO0 (J9 pin 7)   SCL = GPIO1 (J9 pin 8)
**
** It is started from _setup_gpio(). When the keyboard is absent at boot we arm a
** falling-edge interrupt on the INT pin (GPIO50) so a keyboard connected/used later
** flags a hot-plug; InputHandler() then retries the initialization and, once the
** device answers, the navigation flow is enabled.
***************************************************************************************/
namespace kb = m5::unit::tab5_keyboard;

static constexpr int8_t TAB5_KB_SDA = 0;
static constexpr int8_t TAB5_KB_SCL = 1;
static constexpr gpio_num_t TAB5_KB_INT = GPIO_NUM_50;
static constexpr uint8_t TAB5_HID_ENTER = 0x28;
static constexpr uint8_t TAB5_HID_ESC = 0x29;
static constexpr uint8_t TAB5_HID_BACKSPACE = 0x2A;
static constexpr uint8_t TAB5_HID_TAB = 0x2B;
static constexpr uint8_t TAB5_HID_DELETE = 0x4C;
static constexpr uint8_t TAB5_HID_RIGHT = 0x4F;
static constexpr uint8_t TAB5_HID_LEFT = 0x50;
static constexpr uint8_t TAB5_HID_DOWN = 0x51;
static constexpr uint8_t TAB5_HID_UP = 0x52;
static constexpr uint8_t TAB5_HID_MOD_LCTRL = 0x01;
static constexpr uint8_t TAB5_HID_MOD_LSHIFT = 0x02;
static constexpr uint8_t TAB5_HID_MOD_LALT = 0x04;
static constexpr uint8_t TAB5_KEY_LEFT_CTRL = 0x80;
static constexpr uint8_t TAB5_KEY_LEFT_SHIFT = 0x81;
static constexpr uint8_t TAB5_KEY_LEFT_ALT = 0x82;

static m5::unit::UnitUnified tab5KbUnits;
static m5::unit::UnitTab5Keyboard tab5Kb;
static bool tab5KbAdded = false;            // Units.add() must run exactly once
static bool tab5KbReady = false;            // true once begin() succeeds
static volatile bool tab5KbHotplug = false; // set by the INT ISR when kb absent

// Minimal ISR: just latch the hot-plug request; the real work happens in InputHandler().
static void IRAM_ATTR tab5KbHotplugIsr() { tab5KbHotplug = true; }

// Watch the INT line so a keyboard plugged in / used later can be detected.
static void tab5KbArmHotplug() {
    pinMode(TAB5_KB_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(TAB5_KB_INT), tab5KbHotplugIsr, FALLING);
}

// Try to bring the keyboard up. Returns true when the device answers on I2C.
// The library installs its own GPIO50 ISR (event draining) inside begin() once the
// device is confirmed, so begin() failing leaves the INT pin free for our hot-plug ISR.
static bool tab5KbBegin() {
    if (!tab5KbAdded) {
        auto cfg = tab5Kb.config();
        cfg.mode = kb::Mode::Normal; // Normal mode exposes the bitwise per-key state
        cfg.irq_pin = TAB5_KB_INT;   // library drains events on this INT
        tab5Kb.config(cfg);

        Wire.end(); // Closes Wire instance opened by CardKb
        Wire.begin(TAB5_KB_SDA, TAB5_KB_SCL, tab5Kb.component_config().clock);
        if (!tab5KbUnits.add(tab5Kb, Wire)) return false;
        tab5KbAdded = true;
    }
    return tab5KbUnits.begin();
}

// Called from _setup_gpio(): first init attempt. On failure, arm the hot-plug INT.
static void tab5KbSetup() {
    tab5KbReady = tab5KbBegin();
    if (tab5KbReady) {
        Serial.printf("Tab5 keyboard ready (fw 0x%02X)\n", tab5Kb.firmwareVersion());
    } else {
        Serial.printf("Tab5 keyboard absent, arming hot-plug INT on GPIO%d\n", (int)TAB5_KB_INT);
        tab5KbArmHotplug();
    }
}

// Called from InputHandler() once the hot-plug ISR fired: retry the init.
static void tab5KbRetry() {
    // Release our hot-plug ISR so begin() can install the library's own GPIO50 handler.
    detachInterrupt(digitalPinToInterrupt(TAB5_KB_INT));
    tab5KbReady = tab5KbBegin();
    if (tab5KbReady) {
        Serial.printf("Tab5 keyboard connected (fw 0x%02X)\n", tab5Kb.firmwareVersion());
    } else {
        // Still nothing there: keep listening for the next hot-plug edge.
        tab5KbArmHotplug();
    }
}

// Drain the keyboard and translate key presses into the navigation globals.
static void tab5KbPoll() {
    if (!tab5KbReady) return;
    tab5KbUnits.update();
    if (!tab5Kb.wasPressed()) return;

    // A key pressed while the screen sleeps only wakes it up (no navigation).
    if (wakeUpScreen()) {
        AnyKeyPress = true;
        return;
    }

    keyStroke pendingKey;
    bool keyPulse = false;
    const bool sym = tab5Kb.isSym();
    const bool aa = tab5Kb.isAa();
    const bool ctrl = tab5Kb.isCtrl();
    const bool alt = tab5Kb.isAlt();

    for (uint8_t kidx = 0; kidx < kb::KEY_COUNT; ++kidx) {
        if (!tab5Kb.wasPressed(kidx)) continue;
        const uint8_t row = static_cast<uint8_t>(kidx / kb::KEY_COL_COUNT);
        const uint8_t col = static_cast<uint8_t>(kidx % kb::KEY_COL_COUNT);
        const kb::HidMapping map = sym ? kb::keyMatrixToHidSym(row, col) : kb::keyMatrixToHidBase(row, col);

        if (map.keycode != 0) {
            pendingKey.hid_keys.emplace_back(map.keycode);
            pendingKey.modifiers |= map.modifier;
            const char ch = tab5Kb.keyMatrixToChar(kidx);
            if (ch != 0 && ch != '\n' && ch != '\b' && ch != '\t') pendingKey.word.emplace_back(ch);
        }

        switch (map.keycode) {
            case TAB5_HID_LEFT: PrevPress = true; break;
            case TAB5_HID_RIGHT: NextPress = true; break;
            case TAB5_HID_UP: UpPress = true; break;
            case TAB5_HID_DOWN: DownPress = true; break;
            case TAB5_HID_ENTER:
                pendingKey.enter = true;
                SelPress = true;
                break;
            case TAB5_HID_ESC:
                pendingKey.exit_key = true;
                pendingKey.fn = true; // Existing text UI cancels on fn+exit_key.
                EscPress = true;
                break;
            case TAB5_HID_BACKSPACE:
            case TAB5_HID_DELETE: pendingKey.del = true; break;
            case TAB5_HID_TAB: pendingKey.word.emplace_back('\t'); break;
            default: break;
        }
        keyPulse = true;
        AnyKeyPress = true;
    }

    if (keyPulse) {
        if (aa) {
            pendingKey.modifiers |= TAB5_HID_MOD_LSHIFT;
            pendingKey.modifier_keys.emplace_back(TAB5_KEY_LEFT_SHIFT);
        }
        if (ctrl) {
            pendingKey.modifiers |= TAB5_HID_MOD_LCTRL;
            pendingKey.modifier_keys.emplace_back(TAB5_KEY_LEFT_CTRL);
        }
        if (alt) {
            pendingKey.modifiers |= TAB5_HID_MOD_LALT;
            pendingKey.modifier_keys.emplace_back(TAB5_KEY_LEFT_ALT);
        }
        if (sym) pendingKey.fn = true;
        pendingKey.pressed = true;
        KeyStroke = pendingKey;
    } else {
        KeyStroke.Clear();
    }
}

void _setup_gpio() {
    M5.begin();
    M5.Power.setExtOutput(true);
    WiFi.setPins(SDIO2_CLK, SDIO2_CMD, SDIO2_D0, SDIO2_D1, SDIO2_D2, SDIO2_D3, SDIO2_RST);
    // Start hosted Wifi and do an async scan to trigger firmware loading and initialization of the WiFi
    // subsystem
    hostedInitWiFi(); // ESP-IDF function to initialize the WiFi driver in hosted mode
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false);
    WiFi.scanNetworks(true);
    bruceConfig.colorInverted = 0;
}

void _late_setup_gpio() {
    // Try to brig up tab5 keyboard after CardKB
    // Need time to bring up the keyboard
    tab5KbSetup();
}

int getBattery() {
    int percent;
    percent = M5.Power.getBatteryLevel();
    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}

void _setBrightness(uint8_t brightval) { M5.Display.setBrightness(brightval); }

void InputHandler(void) {
    // Late keyboard hot-plug: the INT ISR flagged activity while the keyboard was
    // absent, so retry the initialization and enable the navigation flow.
    if (!tab5KbReady && tab5KbHotplug) {
        tab5KbHotplug = false;
        tab5KbRetry();
    }
    tab5KbPoll();

    static long tm = millis();
    if (millis() - tm > 200 || LongPress) {
        M5.update();
        auto t = M5.Touch.getDetail();
        if (t.isPressed() || t.isHolding()) {
            // Serial.printf("x1=%d, y1=%d, ", t.x, t.y);
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;
            // Serial.printf("x2=%d, y2=%d, rot=%d\n", t.x, t.y, rotation);

            // Touch point global variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        } else touchPoint.pressed = false;
    }
}

void powerOff() { M5.Power.powerOff(); }
