#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "hal/inputs/touch.h"
#include <SD_MMC.h>
#include <Wire.h>
#include <XPowersLib.h>
#include <interface.h>

#define SEL_BTN 0

#define DW_BTN 16
#define UP_BTN 12
static PowersSY6970 PMU;
#define LCD_MODULE_CMD_1

#define BOARD_SENSOR_IRQ 21
#define BOARD_TOUCH_RST 13

// No external/internal pull-ups on these pins -- the old code left them plain INPUT.
static DeviceButtons buttonsCfg() {
    DeviceButtons cfg{UP_BTN, DW_BTN, SEL_BTN};
    cfg.pullup = false;
    return cfg;
}

// CST226SE over the system I2C bus (Wire). Raw touch reads landscape-native (like the XPT2046
// boards): derived algebraically from composing the old fixed driver-level pre-transform
// (setMaxCoordinates(TFT_HEIGHT, TFT_WIDTH)/setSwapXY(true)/setMirrorXY(false,false), applied at
// setup regardless of rotation) with InputHandler's old per-rotation remap block -- see
// src/hal/README.md for the method.
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    cfg.pin_sda = bruceConfigPins.sys_i2c.sda;
    cfg.pin_scl = bruceConfigPins.sys_i2c.scl;
    cfg.pin_rst = BOARD_TOUCH_RST;
    cfg.pin_irq = BOARD_SENSOR_IRQ;
    cfg.cst8xx_model = 0; // CST226
    // rotation:        0      1      2      3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {false, false, true, true};
    const bool mirrorY[4] = {false, true, true, false};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

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
    // bclk,ws,dout,mclk
    bruceConfigPins.speaker_bus = {(gpio_num_t)4, (gpio_num_t)15, (gpio_num_t)11, (gpio_num_t)41};
    bruceConfigPins.mic_bus = {(gpio_num_t)1, (gpio_num_t)2, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data,ws,type
    bruceConfigPins.sys_i2c = {(gpio_num_t)5, (gpio_num_t)6}; // sda, scl
    bruceConfigPins.i2c_bus = {(gpio_num_t)5, (gpio_num_t)6}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 5;
    bruceConfigPins.rfRx = 6;
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = 6;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)43, (gpio_num_t)44};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)43, (gpio_num_t)44};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)6, (gpio_num_t)5}; // rx, tx (Grove)
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)18, (gpio_num_t)8, (gpio_num_t)17, (gpio_num_t)14
    }; // sck,miso,mosi,cs
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)8, (gpio_num_t)17, (gpio_num_t)43};
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)8, (gpio_num_t)17, (gpio_num_t)43};
    // CC1101/NRF24 share the main SPI bus (sck=18, miso=8, mosi=17)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)8, (gpio_num_t)17, (gpio_num_t)43, (gpio_num_t)44, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)8, (gpio_num_t)17, (gpio_num_t)43, (gpio_num_t)44
    }; // sck,miso,mosi,cs(ss),ce

    gpio_hold_dis((gpio_num_t)BOARD_TOUCH_RST); // PIN_TOUCH_RES
    hal_buttons_init(buttonsCfg(), 3);

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
    digitalWrite(BOARD_TOUCH_RST, HIGH);  // PIN_TOUCH_RES
    setSysI2CBus(&Wire); // Touch + PMU both live on the default Wire object
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl); // SDA, SCL

    // Initialize capacitive touch
    hal_touch_init(touchCfg(), 0x5A /* CST226SE_SLAVE_ADDRESS */);
    // Set the screen to turn on or off after pressing the screen Home touch button
    hal_touch_set_home_button(-1, -1, touchHomeKeyCallback);

    bool hasPMU =
        PMU.init(Wire, bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl, SY6970_SLAVE_ADDRESS);
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
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
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
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    hal_buttons_poll_3(buttonsCfg());
    BruceTouchPoint t;
    if (hal_touch_read(touchCfg(), t)) hal_touch_apply(t);
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
