#include "core/powerSave.h"
#include "core/utils.h"
#include <SD_MMC.h>
#include <Wire.h>
#include <XPowersLib.h>
#include <interface.h>
static PowersSY6970 PMU;
#define TOUCH_MODULES_CST_SELF
#include <TouchDrvCSTXXX.hpp>
#include <Wire.h>
#define LCD_MODULE_CMD_1

#define BOARD_I2C_SDA 5
#define BOARD_I2C_SCL 6
#define BOARD_SENSOR_IRQ 21
#define BOARD_TOUCH_RST 13
TouchDrvCSTXXX touch;

void touchHomeKeyCallback(void *user_data) {
    Serial.println("Home key pressed!");
    static uint32_t checkMs = 0;
    if (millis() > checkMs) {
        EscPress = true;
        AnyKeyPress = true;
    }
    checkMs = millis() + 200;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    gpio_hold_dis((gpio_num_t)BOARD_TOUCH_RST); // PIN_TOUCH_RES
    pinMode(SEL_BTN, INPUT);
    pinMode(UP_BTN, INPUT);
    pinMode(DW_BTN, INPUT);

    // CS pins of SPI devices to HIGH
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);
    pinMode(6, OUTPUT);
    digitalWrite(6, HIGH);

    pinMode(BOARD_TOUCH_RST, OUTPUT);   // PIN_TOUCH_RES
    digitalWrite(BOARD_TOUCH_RST, LOW); // PIN_TOUCH_RES
    delay(500);
    digitalWrite(BOARD_TOUCH_RST, HIGH);      // PIN_TOUCH_RES
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL); // SDA, SCL

    // Initialize capacitive touch
    touch.setPins(BOARD_TOUCH_RST, BOARD_SENSOR_IRQ);
    touch.begin(Wire, CST226SE_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL);
    touch.setMaxCoordinates(TFT_HEIGHT, TFT_WIDTH);
    touch.setSwapXY(true);
    touch.setMirrorXY(false, false);
    // Set the screen to turn on or off after pressing the screen Home touch button
    touch.setHomeButtonCallback(touchHomeKeyCallback);

    bool hasPMU = PMU.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, SY6970_SLAVE_ADDRESS);
    if (!hasPMU) {
        Serial.println("PMU is not online...");
    } else {
        PMU.disableOTG();
        PMU.enableADCMeasure();
        PMU.enableCharge();
    }
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // PWM backlight setup
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = 0;
    percent = (PMU.getSystemVoltage() - 3300) * 100 / (float)(4150 - 3350);

    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
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
    else if (brightval == 0) dutyCycle = 5;
    else dutyCycle = ((brightval * 255) / 100);

    Serial.printf("dutyCycle for bright 0-255: %d", dutyCycle);
    ledcWrite(TFT_BL, dutyCycle);
}

struct TouchPointPro {
    int16_t x[5];
    int16_t y[5];
};
/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    TouchPointPro t;
    bool touched = touch.getPoint(t.x, t.y, touch.getSupportTouchPoint());
    if (millis() - tm > 200 || LongPress) {
        int sel = digitalRead(SEL_BTN);
        int prev = digitalRead(UP_BTN);
        int next = digitalRead(DW_BTN);
        if (sel == 0 || next == 0 || prev == 0) {
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;
            SelPress = !sel;
            NextPress = !next;
            PrevPress = !prev;
            // Serial.printf("Sel: %d, Next: %d, Prev: %d\n", SelPress, NextPress, PrevPress);
            return;
        }
        if (touched && touch.isPressed()) {
            tm = millis();
            if (bruceConfigPins.rotation == 1) { t.y[0] = TFT_WIDTH - t.y[0]; }
            if (bruceConfigPins.rotation == 3) { t.x[0] = TFT_HEIGHT - t.x[0]; }
            // Need to test these 2
            if (bruceConfigPins.rotation == 0) {
                int tmp = t.x[0];
                t.x[0] = t.y[0];
                t.y[0] = tmp;
            }
            if (bruceConfigPins.rotation == 2) {
                int tmp = t.x[0];
                t.x[0] = TFT_WIDTH - t.y[0];
                t.y[0] = TFT_HEIGHT - tmp;
            }

            // Serial.printf("\nPressed x=%d , y=%d, rot: %d", t.x[0], t.y[0], bruceConfigPins.rotation);

            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;

            // Touch point global variable
            touchPoint.x = t.x[0];
            touchPoint.y = t.y[0];
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
void powerOff() {}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
