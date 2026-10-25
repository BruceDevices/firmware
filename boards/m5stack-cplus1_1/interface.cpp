#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <AXP192.h>
#include <interface.h>
AXP192 axp192;

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(SEL_BTN, INPUT);
    pinMode(DW_BTN, INPUT);
    setSysI2CBus(&Wire1); // AXP192 (BM8563 RTC included) lives on Wire1
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
    axp192.begin(); // Start the energy management of AXP192
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/

int getBattery() {
    int percent = 0;
#ifndef LITE_VERSION
    float b = axp192.GetBatVoltage();
    percent = ((b - 3.0) / 1.2) * 100;
#endif
    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval > 100) brightval = 100;
    axp192.ScreenBreath(brightval);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    static unsigned long dwPressStart = 0;
    static bool dwHolding = false;
    static bool dwEscFired = false;

    unsigned long now = millis();

    bool upPressed = (axp192.GetBtnPress());
    bool selPressed = (digitalRead(SEL_BTN) == LOW);
    bool dwRaw = (digitalRead(DW_BTN) == LOW);

    // Track DW_BTN (Next button, GPIO 39) for 2s long-press -> ESC
    bool escFromHold = false;
    if (dwRaw) {
        if (!dwHolding) {
            dwHolding = true;
            dwEscFired = false;
            dwPressStart = now;
        } else if (!dwEscFired && (now - dwPressStart >= 2000)) {
            dwEscFired = true;
            escFromHold = true;
        }
    } else {
        dwHolding = false;
        dwEscFired = false;
    }

    if (escFromHold) {
        tm = now;
        if (wakeUpScreen()) return;
        AnyKeyPress = true;
        EscPress = true;
        NextPress = false;
        return;
    }

    // While DW_BTN is still held AFTER 2s ESC already fired, suppress further NextPress
    if (dwRaw && dwEscFired) return;

    if (now - tm < 200 && !LongPress && !upPressed) return;

    bool anyPressed = upPressed || selPressed || dwRaw;
    if (anyPressed) tm = now;
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    if (upPressed && dwRaw) {
        EscPress = true;
        return;
    }
    PrevPress = upPressed;
    NextPress = dwRaw;
    SelPress = selPressed;
}

void powerOff() { axp192.PowerOff(); }
#ifndef LITE_VERSION
/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    int countDown = 0;
    /* Long press power off */
    if (axp192.GetBtnPress()) {
        uint32_t time_count = millis();
        while (axp192.GetBtnPress()) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                if (countDown == 0) {
                    int textWidth = tft.textWidth("PWR OFF IN 3/3", 1);
                    tft.fillRect(60, 7, textWidth, 18, bruceConfig.bgColor);
                }
                tft.setCursor(60, 12);
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                tft.printf(" PWR OFF IN %d/3\n", countDown);
                vTaskDelay(10 / portTICK_RATE_MS);
            }
        }

        // Clear text after releasing the button
        if (millis() - time_count > 500) {
            tft.fillRect(60, 12, 16 * LW, tft.fontHeight(1), bruceConfig.bgColor);
            drawStatusBar();
        }
        PrevPress = true;
    }
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() {
    return axp192.GetBatCurrent() > 20; // need testing
}
#endif
