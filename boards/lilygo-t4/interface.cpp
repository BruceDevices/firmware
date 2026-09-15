#include "core/powerSave.h"
#include "core/utils.h"
#include <Button.h>

#include <globals.h>
#include <interface.h>

volatile bool nxtPress = false;
volatile bool prvPress = false;
volatile bool ecPress = false;
volatile bool slPress = false;

// LEFT (UP_BTN) -> Previous
static void onPrevSingleClickCb(void *button_handle, void *usr_data) { prvPress = true; }
// RIGHT (DW_BTN) -> Next
static void onNextSingleClickCb(void *button_handle, void *usr_data) { nxtPress = true; }
// CENTER (SEL_BTN) -> Select on click, Escape on double-click or hold
static void onSelSingleClickCb(void *button_handle, void *usr_data) { slPress = true; }
static void onSelDoubleClickCb(void *button_handle, void *usr_data) { ecPress = true; }
static void onSelHoldCb(void *button_handle, void *usr_data) { ecPress = true; }

Button *btnPrev;
Button *btnNext;
Button *btnSel;

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // The three front buttons are on ESP32 input-only GPIOs (37/38/39), which
    // cannot use the internal pull-ups - the T4 board provides external ones,
    // so they are configured as plain INPUT and read as active LOW.
    pinMode(UP_BTN, INPUT);
    pinMode(DW_BTN, INPUT);
    pinMode(SEL_BTN, INPUT);

    button_config_t btPrev = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 600,
        .short_press_time = 120,
        .gpio_button_config =
            {
                       .gpio_num = UP_BTN,
                       .active_level = 0,
                       },
    };
    button_config_t btNext = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 600,
        .short_press_time = 120,
        .gpio_button_config =
            {
                       .gpio_num = DW_BTN,
                       .active_level = 0,
                       },
    };
    button_config_t btSel = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 600,
        .short_press_time = 120,
        .gpio_button_config =
            {
                       .gpio_num = SEL_BTN,
                       .active_level = 0,
                       },
    };

    btnPrev = new Button(btPrev);
    btnPrev->attachSingleClickEventCb(&onPrevSingleClickCb, NULL);

    btnNext = new Button(btNext);
    btnNext->attachSingleClickEventCb(&onNextSingleClickCb, NULL);

    btnSel = new Button(btSel);
    btnSel->attachSingleClickEventCb(&onSelSingleClickCb, NULL);
    btnSel->attachDoubleClickEventCb(&onSelDoubleClickCb, NULL);
    btnSel->attachLongPressStartEventCb(&onSelHoldCb, NULL);

    // Start with default IR and RF configs; the T4 has no on-board modules.
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
    static unsigned long tm = 0;
    static bool btn_pressed = false;
    if (nxtPress || prvPress || ecPress || slPress) btn_pressed = true;

    if (millis() - tm > 200 || LongPress) {
        if (btn_pressed) {
            btn_pressed = false;
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;
            SelPress = slPress;
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

/*********************************************************************
** Function: powerOff
** Turns off the device (deep sleep, wake on the center button)
**********************************************************************/
void powerOff() {
    tft.fillScreen(bruceConfig.bgColor);
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(0x10); // ILI9341 enter sleep
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}
