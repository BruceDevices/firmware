#include "core/powerSave.h"
#include <driver/gpio.h>
#include <interface.h>

// Deselect NRF24/CC1101 before any Arduino/TFT init runs (runs before setup())
static void __attribute__((constructor)) _early_spi_deselect() {
    gpio_reset_pin(GPIO_NUM_15); // CC1101_SS / NRF24_SS
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_15, 1); // CS HIGH = deselected

    gpio_reset_pin(GPIO_NUM_4); // NRF24_CE
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0); // CE LOW = standby
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // 5-way tactile switch — GPIO 34/35 are input-only (no pull-up on chip)
    // Ensure external pull-up resistors are present on these pins
    pinMode(UP_BTN, INPUT);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(L_BTN, INPUT_PULLUP);
    pinMode(R_BTN, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT);

    // Deselect CC1101 and NRF24 on shared SPI bus so they don't interfere with TFT
    pinMode(CC1101_SS_PIN, OUTPUT);
    digitalWrite(CC1101_SS_PIN, HIGH);
    pinMode(NRF24_SS_PIN, OUTPUT);
    digitalWrite(NRF24_SS_PIN, HIGH);
    pinMode(NRF24_CE_PIN, OUTPUT);
    digitalWrite(NRF24_CE_PIN, LOW);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup
***************************************************************************************/
void _post_setup_gpio() {
    // Backlight PWM
    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 255);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
** On 5-way joystick: hold UP for ~400ms = EscPress (back/exit)
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    static unsigned long upHoldStart = 0;
    static bool upEscFired = false;

    // --- Hold-UP-for-Escape: runs every call, bypasses debounce ---
    // Read UP_BTN on every task cycle (~10ms) so the hold timer
    // accumulates correctly regardless of the 200ms debounce gate.
    // Keep setting EscPress while held so the task can't clear it
    // before the application loop reads it via check().
    bool _up_raw = (digitalRead(UP_BTN) == BTN_ACT);
    if (_up_raw) {
        if (upHoldStart == 0) upHoldStart = millis();
        if (millis() - upHoldStart >= 400) {
            EscPress = true;
            AnyKeyPress = true;
            upEscFired = true;
        }
    } else {
        upHoldStart = 0;
        upEscFired = false;
    }

    if (millis() - tm < 200 && !LongPress) return;

    bool _u = _up_raw;
    bool _d = (digitalRead(DW_BTN) == BTN_ACT);
    bool _l = (digitalRead(L_BTN) == BTN_ACT);
    bool _r = (digitalRead(R_BTN) == BTN_ACT);
    bool _s = (digitalRead(SEL_BTN) == BTN_ACT);

    if (_u || _d || _l || _r || _s) {
        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }

    if (_l) { PrevPress = true; }
    if (_r) { NextPress = true; }
    if (_u && !upEscFired) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (_d) {
        DownPress = true;
        NextPagePress = true;
    }
    if (_s) { SelPress = true; }
    if (_l && _r) {
        EscPress = true;
        NextPress = false;
        PrevPress = false;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device
**********************************************************************/
void checkReboot() {
    int countDown = 0;
    /* Long press Left+Right to power off */
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
            if (millis() - time_count > 500) {
                if (countDown == 0) {
                    int textWidth = tft.textWidth("PWR OFF IN 3/3", 1);
                    tft.fillRect(tftWidth / 2 - textWidth / 2, 7, textWidth, 18, bruceConfig.bgColor);
                }
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(L_BTN) == BTN_ACT || digitalRead(R_BTN) == BTN_ACT);
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }
        delay(30);
        if (millis() - time_count > 500) {
            tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
            drawStatusBar();
        }
    }
}

int getBattery() { return 0; }

bool isCharging() { return false; }
