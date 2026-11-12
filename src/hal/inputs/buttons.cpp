#include "buttons.h"

#include "core/display.h"
#include "core/powerSave.h"
#include "globals.h"

#if defined(BUTTONS_IDF_COMPONENT)
#include <Button.h>

static Button *hal2Btn1 = nullptr;
static Button *hal2Btn2 = nullptr;
static volatile bool hal2PendingNext = false;
static volatile bool hal2PendingSel = false;
static volatile bool hal2PendingPrev = false;
static volatile bool hal2PendingEsc = false;

static void hal2OnBtn1Click(void *, void *) { hal2PendingNext = true; }
static void hal2OnBtn1Long(void *, void *) { hal2PendingSel = true; }
static void hal2OnBtn1Double(void *, void *) { hal2PendingSel = true; }
static void hal2OnBtn2Click(void *, void *) { hal2PendingPrev = true; }
static void hal2OnBtn2Long(void *, void *) { hal2PendingEsc = true; }
static void hal2OnBtn2Double(void *, void *) { hal2PendingEsc = true; }
#endif

static void configurePin(int8_t pin, bool pullup) {
    if (pin < 0) return;
    if (pullup) pinMode(pin, INPUT_PULLUP);
    else pinMode(pin, INPUT);
}

static bool pressed(int8_t pin, bool activeHigh = false) {
    if (pin < 0) return false;
    return activeHigh ? digitalRead(pin) == HIGH : digitalRead(pin) == LOW;
}

void hal_buttons_init(const DeviceButtons &cfg, uint8_t count) {
    configurePin(cfg.btn1, cfg.pullup);
    if (count >= 3) {
        configurePin(cfg.btn2, cfg.pullup);
        configurePin(cfg.btn3, cfg.pullup);
    }
    if (count >= 5) {
        configurePin(cfg.btn4, cfg.pullup);
        configurePin(cfg.btn5, cfg.pullup);
    }
    if (count >= 6) configurePin(cfg.btn6, cfg.pullup);
}

void hal_buttons_init_2(const DeviceButtons &cfg, uint16_t long_press_ms) {
#if defined(BUTTONS_IDF_COMPONENT)
    button_config_t btn1Cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = long_press_ms,
        .short_press_time = 120,
        .gpio_button_config = {.gpio_num = cfg.btn1, .active_level = 0},
    };
    button_config_t btn2Cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = long_press_ms,
        .short_press_time = 120,
        .gpio_button_config = {.gpio_num = cfg.btn2, .active_level = 0},
    };
    hal2Btn1 = new Button(btn1Cfg);
    hal2Btn1->attachSingleClickEventCb(&hal2OnBtn1Click, nullptr);
    hal2Btn1->attachDoubleClickEventCb(&hal2OnBtn1Double, nullptr);
    hal2Btn1->attachLongPressStartEventCb(&hal2OnBtn1Long, nullptr);

    hal2Btn2 = new Button(btn2Cfg);
    hal2Btn2->attachSingleClickEventCb(&hal2OnBtn2Click, nullptr);
    hal2Btn2->attachDoubleClickEventCb(&hal2OnBtn2Double, nullptr);
    hal2Btn2->attachLongPressStartEventCb(&hal2OnBtn2Long, nullptr);
#else
    (void)cfg;
    (void)long_press_ms;
#endif
}

void hal_buttons_poll_2() {
#if defined(BUTTONS_IDF_COMPONENT)
    static unsigned long tm = 0;
    static bool btnPressed = false;
    if (hal2PendingNext || hal2PendingPrev || hal2PendingSel || hal2PendingEsc) btnPressed = true;

    if (millis() - tm > 200 || LongPress) {
        if (btnPressed) {
            btnPressed = false;
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;
            NextPress = hal2PendingNext;
            PrevPress = hal2PendingPrev;
            SelPress = hal2PendingSel;
            EscPress = hal2PendingEsc;
            hal2PendingNext = false;
            hal2PendingPrev = false;
            hal2PendingSel = false;
            hal2PendingEsc = false;
        }
    }
#endif
}

void hal_buttons_poll_1(const DeviceButtons &cfg) {
    static unsigned long tm = 0;
    constexpr unsigned long kInputDebounceMs = 75;
    if (millis() - tm < kInputDebounceMs && !LongPress) return;

    checkPowerSaveTime();

    static bool buttonWasDown = false;
    static unsigned long buttonDownAt = 0;
    static uint8_t drawn = 2;
    constexpr unsigned long kSelectPressMs = 550;
    constexpr unsigned long kBackPressMs = 1200;
    constexpr unsigned long kDoublePressIntervalMs = 300;

    static unsigned long lastButtonReleaseTime = 0;
    static int clickCount = 0;
    static bool pendingNextPress = false;
    static unsigned long pendingTime = 0;

    if (pendingNextPress && millis() - pendingTime > kDoublePressIntervalMs) {
        NextPress = true;
        pendingNextPress = false;
    }

    bool buttonDown = pressed(cfg.btn1, cfg.activeHigh);

    if (buttonDown && !buttonWasDown) {
        buttonWasDown = true;
        buttonDownAt = millis();
        tm = millis();
        AnyKeyPress = true;
        LongPress = false;
        if (wakeUpScreen()) return;
    }

    if (buttonDown) {
        AnyKeyPress = true;
        if (millis() - buttonDownAt >= kSelectPressMs) {
            LongPress = true;
            if (drawn > 1) {
                tft.fillRect(tftWidth - 3, 0, 3, tftHeight, TFT_GREENYELLOW);
                tft.fillRect(0, tftHeight - 3, tftWidth, 3, TFT_GREENYELLOW);
                drawn = 1;
            }
        }
        if (millis() - buttonDownAt >= kBackPressMs && drawn > 0) {
            tft.fillRect(tftWidth - 3, 0, 3, tftHeight, TFT_RED);
            tft.fillRect(0, tftHeight - 3, tftWidth, 3, TFT_RED);
            drawn = 0;
        }
        return;
    }

    if (buttonWasDown) {
        buttonWasDown = false;
        unsigned long heldMs = millis() - buttonDownAt;
        tft.fillRect(tftWidth - 3, 0, 3, tftHeight, bruceConfig.bgColor);
        tft.fillRect(0, tftHeight - 3, tftWidth, 3, bruceConfig.bgColor);
        drawn = 2;

        if (millis() - lastButtonReleaseTime > kDoublePressIntervalMs) { clickCount = 0; }

        if (heldMs >= kBackPressMs) {
            EscPress = true;
            pendingNextPress = false;
        } else if (heldMs >= kSelectPressMs) {
            SelPress = true;
            pendingNextPress = false;
        } else {
            clickCount++;
            lastButtonReleaseTime = millis();

            if (clickCount >= 2) {
                PrevPress = true;
                clickCount = 0;
                pendingNextPress = false;
                AnyKeyPress = true;
                LongPress = false;
                return;
            } else {
                pendingNextPress = true;
                pendingTime = millis();
            }
        }
        AnyKeyPress = true;
        LongPress = false;
    }
}

void hal_buttons_poll_3(const DeviceButtons &cfg) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool prev = pressed(cfg.btn1, cfg.activeHigh);
    bool next = pressed(cfg.btn2, cfg.activeHigh);
    bool sel = pressed(cfg.btn3, cfg.activeHigh);

    bool anyPressed = prev || next || sel;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    EscPress = prev && next;
    if (EscPress) return;
    PrevPress = prev;
    NextPress = next;
    SelPress = sel;
}

void hal_buttons_poll_5(const DeviceButtons &cfg) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool prev = pressed(cfg.btn1, cfg.activeHigh);
    bool next = pressed(cfg.btn2, cfg.activeHigh);
    bool up = pressed(cfg.btn3, cfg.activeHigh);
    bool down = pressed(cfg.btn4, cfg.activeHigh);
    bool sel = pressed(cfg.btn5, cfg.activeHigh);

    if (!(prev || next || up || down || sel)) return;
    tm = millis();
    if (!wakeUpScreen()) AnyKeyPress = true;
    else return;

    if (prev && next) {
        EscPress = true;
        return;
    }
    if (prev) PrevPress = true;
    if (next) NextPress = true;
    if (up) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (down) {
        DownPress = true;
        NextPagePress = true;
    }
    if (sel) SelPress = true;
}

void hal_buttons_poll_6(const DeviceButtons &cfg, bool esc_on_combo_too) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool prev = pressed(cfg.btn1, cfg.activeHigh);
    bool next = pressed(cfg.btn2, cfg.activeHigh);
    bool up = pressed(cfg.btn3, cfg.activeHigh);
    bool down = pressed(cfg.btn4, cfg.activeHigh);
    bool sel = pressed(cfg.btn5, cfg.activeHigh);
    bool esc = pressed(cfg.btn6, cfg.activeHigh);

    if (!(prev || next || up || down || sel || esc)) return;
    tm = millis();
    if (!wakeUpScreen()) AnyKeyPress = true;
    else return;

    if (esc || (esc_on_combo_too && prev && next)) {
        EscPress = true;
        return;
    }
    if (prev) PrevPress = true;
    if (next) NextPress = true;
    if (up) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (down) {
        DownPress = true;
        NextPagePress = true;
    }
    if (sel) SelPress = true;
}
