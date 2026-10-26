#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include <Arduino.h>
#include <interface.h>

#define GT911_SLAVE_ADDRESS_L 0x5D

#if defined(HAS_CAPACITIVE_TOUCH)
#include "hal/device.h"
#include "hal/inputs/touch.h"
// GT911 and CST816S/CST820 both go through the HAL now (TOUCH_CTRL_GT911/TOUCH_CTRL_CST8XX). The
// table reproduces the old per-rotation setMaxCoordinates/setSwapXY/setMirrorXY dance (GT911) and
// the old CYD28_TouchscreenC::convertRawXY pre-transform composed with InputHandler's old
// per-rotation remap (CST8xx) -- both driver families land on the exact same table.
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
#if defined(TOUCH_CTRL_GT911)
    cfg.pin_sda = SYS_I2C_SDA;
    cfg.pin_scl = SYS_I2C_SCL;
    cfg.pin_rst = GT911_TOUCH_CONFIG_RST_GPIO_NUM;
    cfg.pin_irq = GT911_TOUCH_CONFIG_INT_GPIO_NUM;
#else
    cfg.pin_sda = CST816S_I2C_CONFIG_SDA_IO_NUM;
    cfg.pin_scl = CST816S_I2C_CONFIG_SCL_IO_NUM;
    cfg.pin_rst = CST816S_TOUCH_CONFIG_RST_GPIO_NUM;
    cfg.pin_irq = CST816S_TOUCH_CONFIG_INT_GPIO_NUM;
#endif
    // rotation:        0      1      2      3
    const bool swapXY[4] = {false, true, false, true};
    const bool mirrorX[4] = {false, false, true, true};
    const bool mirrorY[4] = {false, true, true, false};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}
#elif defined(TOUCH_CTRL_XPT2046)
#include "CYD28_TouchscreenR.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp
#define XPT2046_CS CYD28_TouchR_CS
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
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
#endif

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)27};
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)27};
    bruceConfigPins.i2c_bus = {(gpio_num_t)27, (gpio_num_t)22}; // sda, scl

#if defined(SYS_I2C_SDA) && defined(SYS_I2C_SCL)
    bruceConfigPins.sys_i2c = {(gpio_num_t)SYS_I2C_SDA, (gpio_num_t)SYS_I2C_SCL}; // sda, scl
#endif

    bruceConfigPins.badusb_bus = {(gpio_num_t)22, (gpio_num_t)27}; // rx, tx (CH9329)
    bruceConfigPins.irTx = 22;
    bruceConfigPins.irRx = 27;
    bruceConfigPins.rfTx = 27;
    bruceConfigPins.rfRx = 22;
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)5
    }; // sck,miso,mosi,cs
#if defined(ST7796_DRIVER)
    // CYD35_base variant (CYD-3248S035x)
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)3, (gpio_num_t)1}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)3, (gpio_num_t)1};  // rx, tx
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)35, (gpio_num_t)22, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)35, (gpio_num_t)22
    }; // sck,miso,mosi,cs(ss),ce
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)35, (gpio_num_t)22, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif
#else
    // CYD_base variant (CYD-2432Sxxx)
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)1, (gpio_num_t)3}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)1, (gpio_num_t)3};  // rx, tx
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)27, (gpio_num_t)22, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)27, (gpio_num_t)22
    }; // sck,miso,mosi,cs(ss),ce
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)27, (gpio_num_t)22, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif
#endif

#ifndef HAS_CAPACITIVE_TOUCH // Capacitive Touchscreen uses I2C to communicate
    pinMode(XPT2046_CS, OUTPUT);
    digitalWrite(XPT2046_CS, HIGH);
#endif

#if defined(HAS_CAPACITIVE_TOUCH)
    setSysI2CBus(&Wire1);
#if defined(TOUCH_CTRL_GT911)
    bruceConfigPins.sys_i2c.sda = (gpio_num_t)SYS_I2C_SDA;
    bruceConfigPins.sys_i2c.scl = (gpio_num_t)SYS_I2C_SCL;
#else
    bruceConfigPins.sys_i2c.sda = (gpio_num_t)CST816S_I2C_CONFIG_SDA_IO_NUM;
    bruceConfigPins.sys_i2c.scl = (gpio_num_t)CST816S_I2C_CONFIG_SCL_IO_NUM;
#endif
#endif

    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
#if defined(TOUCH_CTRL_GT911)
    if (!hal_touch_init(touchCfg(), GT911_SLAVE_ADDRESS_L)) {
        Serial.println("Failed to find GT911 - check your wiring!");
    }
#endif
#if defined(TOUCH_CTRL_CST8XX)
    if (!hal_touch_init(touchCfg(), 0x15 /* CST816_SLAVE_ADDRESS -- also CST820's address */)) {
        Serial.println("Touch IC not Started");
        log_i("Touch IC not Started");
    } else log_i("Touch IC Started");
#endif
#if defined(TOUCH_CTRL_XPT2046)
    hal_touch_init(touchCfg(), 0, TFT_MOSI == CYD28_TouchR_MOSI); // shared with the display SPI bus or on its own pins
    // calibration: loadTouchCalibration()/calibrateTouch() in main.cpp (NVS "touch_cal")
#endif

    // Brightness control must be initialized after tft in this case @Pirata
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);

    // Force sync color inversion to prevent bruceConf.json from overriding
    // the value set in _setup_gpio(). For CYD variants with TFT_INVERSION_ON,
    // the init() sequence sends INVON; we send INVOFF here to ensure normal colors.
#ifdef TFT_INVERSION_ON
    bruceConfig.colorInverted = 0;
    tft.invertDisplay(0);
#else
    bruceConfig.colorInverted = 1;
    tft.invertDisplay(1);
#endif

    bruceConfigPins.gpsBaudrate = 9600;

    bool pinsChanged = false;
    if (bruceConfigPins.rfTx != 22) {
        bruceConfigPins.rfTx = 22;
        pinsChanged = true;
    }
    if (bruceConfigPins.rfRx != 27) {
        bruceConfigPins.rfRx = 27;
        pinsChanged = true;
    }
    if (bruceConfigPins.irTx != 22) {
        bruceConfigPins.irTx = 22;
        pinsChanged = true;
    }
    if (bruceConfigPins.irRx != 27) {
        bruceConfigPins.irRx = 27;
        pinsChanged = true;
    }
    if (pinsChanged) bruceConfigPins.saveFile();
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            d_tmp = millis();
            hal_touch_apply(t);
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
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
