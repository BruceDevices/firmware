#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <CYD28_TouchscreenR.h>
#include <interface.h>

#define ENCODER_INA 2
#define ENCODER_INB 14
#define ENCODER_KEY 0

#define BTN_ACT LOW
extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp

static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // The XPT2046 reports landscape (rotation 1) coordinates
    // rotation:        0      1      2      3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {true, false, false, true};
    const bool mirrorY[4] = {false, false, true, true};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

#ifdef WAVESENTRY
#include "hal/device.h"
#include "hal/inputs/encoder.h"
static DeviceEncoder encoderCfg() {
    DeviceEncoder cfg;
    cfg.pin_a = ENCODER_INA;
    cfg.pin_b = ENCODER_INB;
    cfg.pin_sel = ENCODER_KEY;
    return cfg;
}
#endif

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)33, (gpio_num_t)22}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 33;
    bruceConfigPins.rfRx = 22;
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = 22;
    bruceConfigPins.uart_bus = {(gpio_num_t)13, (gpio_num_t)4};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)13, (gpio_num_t)4};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)13, (gpio_num_t)4}; // rx, tx
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)1};
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)1};
    // CC1101/NRF24/SDCARD share the main SPI bus (sck=18, miso=19, mosi=23)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs(ss),ce
    // SDCARD CS differs by hardware revision: Marauder-V4-V6=12, Marauder-v61=14
    // (SDCARD_CS_V61 only defined by the Marauder-v61 env, see marauder-touch.ini)
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23,
#ifdef SDCARD_CS_V61
        (gpio_num_t)SDCARD_CS_V61
#else
        (gpio_num_t)12
#endif
    }; // sck,miso,mosi,cs

    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 0; // intentional: overrides -DROTATION regardless of value (portrait)
    pinMode(TFT_BL, OUTPUT);
#ifdef WAVESENTRY
    hal_encoder_init(encoderCfg());
#endif
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    if (!hal_touch_init(touchCfg(), 0, true)) { // shares the display SPI bus
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
    if (brightval > 5) digitalWrite(TFT_BL, HIGH);
    else digitalWrite(TFT_BL, LOW);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = millis();
    if (millis() - tm > 300 || LongPress) { // don´t allow multiple readings in less than 200ms
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            tm = millis();
            hal_touch_apply(t);
        } else touchPoint.pressed = false;
    }

#ifdef WAVESENTRY
    hal_encoder_poll(encoderCfg());
#endif
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
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
