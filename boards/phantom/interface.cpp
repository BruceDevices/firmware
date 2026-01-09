#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <interface.h>

#define XPT2046_CS TOUCH_CS

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
SPIClass touchSPI;
void _setup_gpio() {
    pinMode(XPT2046_CS, OUTPUT);
    digitalWrite(XPT2046_CS, HIGH);
    bruceConfigPins.rotation = 0;  // portrait mode for Phantom
    bruceConfig.colorInverted = 0; // color invert for Phantom
    tft.setRotation(bruceConfigPins.rotation);
    uint16_t calData[5] = {275, 3500, 280, 3590, 3}; // 0011 = 3
    tft.setTouch(calData);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // uint16_t calData[5];
    // bruceConfigPins.rotation = 0;
    // tft.setRotation(0);
    // tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, 10);
    // Serial.printf("%d\n%d\n%d\n%d\n%d\n", calData[0], calData[1], calData[2], calData[3], calData[4]);
    // tft.setTouch(calData);
    // Brightness control must be initialized after tft in this case @Pirata
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);

    // log_i("dutyCycle for bright 0-255: %d", dutyCycle);
    ledcWrite(TFT_BL, dutyCycle);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm > 200 || LongPress) {
        // I know R3CK.. I Should NOT nest if statements..
        // but it is needed to not keep SPI bus used without need, it save resources
        TouchPoint t;
        // TouchPoint t2;
        checkPowerSaveTime();
        digitalWrite(TFT_CS, HIGH);
        digitalWrite(TOUCH_CS, LOW);
        bool _IH_touched = tft.getTouch(&t.x, &t.y);
        // tft.getTouchRaw(&t2.x, &t2.y);
        digitalWrite(TOUCH_CS, HIGH);
        if (_IH_touched) {
            NextPress = false;
            PrevPress = false;
            UpPress = false;
            DownPress = false;
            SelPress = false;
            EscPress = false;
            AnyKeyPress = false;
            NextPagePress = false;
            PrevPagePress = false;
            touchPoint.pressed = false;
            _IH_touched = false;

            // Serial.printf("\nRAWRaw: Touch Pressed on x=%d, y=%d", t2.x, t2.y);
            // Serial.printf("\nRAW:    Touch Pressed on x=%d, y=%d", t.x, t.y);
            if (bruceConfigPins.rotation == 0) {
                t.y = (tftHeight + 20) - t.y;
                t.x = tftWidth - t.x;
            }
            if (bruceConfigPins.rotation == 3) {
                uint16_t tmp = t.x;
                t.x = map((tftHeight + 20) - t.y, 0, 240, 0, 320);
                t.y = map(tmp, 0, 320, 0, 240);
            }
            if (bruceConfigPins.rotation == 1) {
                uint16_t tmp = t.x;
                t.x = map(t.y, 0, 240, 0, 320);
                t.y = map(tftWidth - tmp, 0, 320, 0, 240);
            }
            // Serial.printf("\nROT: Touch Pressed on x=%d, y=%d, rot: %d\n", t.x, t.y,
            // bruceConfigPins.rotation);
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;

            // Touch point global variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        }
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
