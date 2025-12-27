#include "core/powerSave.h"
#include "core/utils.h"
#include <CYD28_TouchscreenR.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <interface.h>
#include <soc/adc_channel.h>
#include <soc/soc_caps.h>
CYD28_TouchR touch(320, 240);

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfig.colorInverted = 0;
    pinMode(CC1101_SS_PIN, OUTPUT);
    pinMode(NRF24_SS_PIN, OUTPUT);
    digitalWrite(CC1101_SS_PIN, HIGH);
    digitalWrite(NRF24_SS_PIN, HIGH);
    pinMode(TFT_BL, OUTPUT);

    bruceConfig.rfModule = CC1101_SPI_MODULE;
    bruceConfig.irRx = RXLED;
    // bruceConfig.irTx = LED;
    Wire.setPins(SDA, SCL);
    Wire.begin();
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    if (!touch.begin(&tft.getSPIinstance())) {
        Serial.println("Touch IC not Started");
        log_i("Touch IC not Started");
    } else Serial.println("Touch IC Started");
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/

void InputHandler(void) {
    static long tm = millis();

    // debounce touch events
    if (millis() - tm > 300 || LongPress) {
        if (touch.touched()) {
            auto raw = touch.getPointScaled();
            tm = millis();

            // --- Rotation compensation ---
            int16_t tx = raw.x;
            int16_t ty = raw.y;

            switch (bruceConfig.rotation) {
                case 2: // portrait
                {
                    int tmp = tx;
                    tx = tftWidth - ty;
                    ty = tmp;
                } break;
                case 3: // landscape
                    // no swap needed
                    break;
                case 0: // portrait inverted
                {
                    int tmp = tx;
                    tx = ty;
                    ty = (tftHeight + 0) - tmp; // calibrate in real time
                } break;
                case 1:                        // landscape inverted
                    ty = (tftHeight + 0) - ty; // calibrate in real time
                    tx = tftWidth - tx;
                    break;
            }

            // Serial.printf( "Touch: raw=(%d,%d) mapped=(%d,%d) rot=%d\n", raw.x, raw.y, tx, ty,
            // bruceConfig.rotation" );

            // wake screen if off
            if (!wakeUpScreen()) {
                AnyKeyPress = true;
            } else {
                return;
            }

            // store in global touch point
            touchPoint.x = tx;
            touchPoint.y = ty;
            touchPoint.pressed = true;

            // optional: heatmap logging
            touchHeatMap(touchPoint);

        } else {
            touchPoint.pressed = false;
        }
    }
}

/*********************************************************************
** Function: keyboard
** location: mykeyboard.cpp
** Starts keyboard to type data
**********************************************************************/
// String keyboard(String mytext, int maxSize, String msg) {}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
