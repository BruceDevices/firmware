#include "core/powerSave.h"
#include "core/utils.h"
#include <globals.h>
#include <interface.h>

#ifdef USE_SD_MMC
#define PIN_SD_CMD 13
#define PIN_SD_CLK 11
#define PIN_SD_D0 12
#endif
#ifdef HAS_TOUCH
#define TOUCH_MODULES_CST_SELF
#include <TouchLib.h>
#include <Wire.h>
TouchLib touch(Wire, 18, 17, CTS820_SLAVE_ADDRESS, 21);
#endif

#include <Button.h>
volatile bool nxtPress = false;
volatile bool prvPress = false;
volatile bool ecPress = false;
volatile bool slPress = false;
static void onButtonSingleClickCb1(void *button_handle, void *usr_data) { nxtPress = true; }
static void onButtonDoubleClickCb1(void *button_handle, void *usr_data) { slPress = true; }
static void onButtonHoldCb1(void *button_handle, void *usr_data) { slPress = true; }

static void onButtonSingleClickCb2(void *button_handle, void *usr_data) { prvPress = true; }
static void onButtonDoubleClickCb2(void *button_handle, void *usr_data) { ecPress = true; }
static void onButtonHoldCb2(void *button_handle, void *usr_data) { ecPress = true; }

Button *btn1;
Button *btn2;

#if defined(T_DISPLAY_S3)

#endif

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {

#ifdef USE_SD_MMC
    SD.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
#endif

#ifdef HAS_TOUCH
    gpio_hold_dis((gpio_num_t)21); // PIN_TOUCH_RES
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH); // PIN_POWER_ON
    pinMode(21, OUTPUT);    // PIN_TOUCH_RES
    digitalWrite(21, LOW);  // PIN_TOUCH_RES
    delay(500);
    digitalWrite(21, HIGH); // PIN_TOUCH_RES
    Wire.begin(18, 17);     // SDA, SCL
    if (!touch.init()) { Serial.println("Touch IC not found"); }

    touch.setRotation(1);
#endif
    // setup buttons
    button_config_t bt1 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 600,
        .short_press_time = 120,
        .gpio_button_config = {
                               .gpio_num = DW_BTN,
                               .active_level = 0,
                               },
    };
    button_config_t bt2 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 600,
        .short_press_time = 120,
        .gpio_button_config = {
                               .gpio_num = UP_BTN,
                               .active_level = 0,
                               },
    };
    pinMode(SEL_BTN, INPUT_PULLUP);

    btn1 = new Button(bt1);

    // btn->attachPressDownEventCb(&onButtonPressDownCb, NULL);
    btn1->attachSingleClickEventCb(&onButtonSingleClickCb1, NULL);
    btn1->attachDoubleClickEventCb(&onButtonDoubleClickCb1, NULL);
    btn1->attachLongPressStartEventCb(&onButtonHoldCb1, NULL);

    btn2 = new Button(bt2);

    // btn->attachPressDownEventCb(&onButtonPressDownCb, NULL);
    btn2->attachSingleClickEventCb(&onButtonSingleClickCb2, NULL);
    btn2->attachDoubleClickEventCb(&onButtonDoubleClickCb2, NULL);
    btn2->attachLongPressStartEventCb(&onButtonHoldCb2, NULL);

    // setup POWER pin required by the vendor
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    // Start with default IR, RF and RFID Configs, replace old
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;

    bruceConfigPins.irRx = RXLED;
    bruceConfigPins.irTx = TXLED;

    Serial.begin(115200);
}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
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
    static long tm = 0;
    static bool btn_pressed = false;
    bool selPressed = false;
    if (nxtPress || prvPress || ecPress || slPress || selPressed) btn_pressed = true;

    if (millis() - tm > 200 || LongPress) {
#ifdef HAS_TOUCH
        if (touch.read()) {
            auto t = touch.getPoint(0);
            tm = millis();
            if (bruceConfigPins.rotation == 1) {
                t.y = (tftHeight + 20) - t.y;
                // t.x = tftWidth-t.x;
            }
            if (bruceConfigPins.rotation == 3) {
                // t.y = (tftHeight+20)-t.y;
                t.x = tftWidth - t.x;
            }
            // Need to test the other orientations

            if (bruceConfigPins.rotation == 0) {
                int tmp = t.x;
                t.x = tftWidth - t.y;
                t.y = tmp;
            }
            if (bruceConfigPins.rotation == 2) {
                int tmp = t.x;
                t.x = t.y;
                t.y = (tftHeight + 20) - tmp;
            }

            // Serial.printf("\nPressed x=%d , y=%d, rot: %d",t.x, t.y, bruceConfigPins.rotation);

            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;

            // Touch point global variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        }
#endif
        if (digitalRead(SEL_BTN) == BTN_ACT) {
            selPressed = true;
            btn_pressed = true;
        }
        if (btn_pressed) {
            btn_pressed = false;
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;
            SelPress = slPress + selPressed;
            EscPress = ecPress;
            NextPress = nxtPress;
            PrevPress = prvPress;

            nxtPress = false;
            prvPress = false;
            ecPress = false;
            slPress = false;
        }
    }
}

void powerOff() {
#ifdef T_DISPLAY_S3
    tft.fillScreen(bruceConfig.bgColor);
    digitalWrite(PIN_POWER_ON, LOW);
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(0x10);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
#endif
}

void checkReboot() {
#ifdef T_DISPLAY_S3
    int countDown;
    /* Long press power off */
    if (digitalRead(UP_BTN) == BTN_ACT && digitalRead(DW_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(UP_BTN) == BTN_ACT && digitalRead(DW_BTN) == BTN_ACT) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(UP_BTN) == BTN_ACT || digitalRead(DW_BTN) == BTN_ACT);
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }

        // Clear text after releasing the button
        delay(30);
        tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
    }
#endif
}
