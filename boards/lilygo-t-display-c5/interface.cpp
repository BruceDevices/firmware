#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <interface.h>

#define BTN_A 0
#define BTN_ACT LOW
#define BTN_B 28

/***************************************************************************************
** LILYGO T-Display-C5 — Bruce board interface
** ST7789 170x320 (no touch), two physical buttons (IO0 + BOOT).
***************************************************************************************/

/***************************************************************************************
** Function name: _setup_gpio()
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)2, (gpio_num_t)3}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 2;
    bruceConfigPins.rfRx = 3;
    bruceConfigPins.irTx = 1;
    bruceConfigPins.irRx = 1;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)12, (gpio_num_t)11}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)4, (gpio_num_t)5};    // rx, tx (shares radio pins 4/5)
    bruceConfigPins.badusb_bus = {(gpio_num_t)2, (gpio_num_t)3}; // rx, tx
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)-1, (gpio_num_t)-1, (gpio_num_t)-1, (gpio_num_t)-1
    }; // sck,miso,mosi,cs (no microSD on this board)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)7, (gpio_num_t)6, (gpio_num_t)9, (gpio_num_t)4};
    bruceConfigPins.PN532_bus = {(gpio_num_t)7, (gpio_num_t)6, (gpio_num_t)9, (gpio_num_t)4};
    // CC1101/NRF24/W5500 share the radio SPI bus (sck=7, miso=6, mosi=9, cs=4, control=5)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)7, (gpio_num_t)6, (gpio_num_t)9, (gpio_num_t)4, (gpio_num_t)5, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)7, (gpio_num_t)6, (gpio_num_t)9, (gpio_num_t)4, (gpio_num_t)5
    }; // sck,miso,mosi,cs(ss),ce
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)7, (gpio_num_t)6, (gpio_num_t)9, (gpio_num_t)4, (gpio_num_t)5, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(TFT_MOSI, OUTPUT);
    digitalWrite(TFT_MOSI, HIGH);
    pinMode(TFT_SCLK, OUTPUT);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);

#ifdef HAS_2_BUTTONS
    hal_buttons_init_2(DeviceButtons{BTN_B, BTN_A}, 600);
#endif

    // All external SPI radios share CS=4 / control=5 on this board.
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
#if !defined(LITE_VERSION)
    pinMode(bruceConfigPins.W5500_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.W5500_bus.cs, HIGH);
#endif
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);

    if (bruceConfigPins.SDCARD_bus.cs >= 0) { // no microSD on the T-Display-C5
        pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
        digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);
    }

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
***************************************************************************************/
void _post_setup_gpio() {
    // No touch controller on this board — nothing to calibrate.
}

/***************************************************************************************
** Function name: getBattery()
** NOTE: battery gauge lives in the AXP2602 PMU over I2C (SDA=2, SCL=3, INT=10).
**       Returning 0 for now — implement AXP2602 readout here if you want % / charge.
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: setBrightness
** Backlight is a plain GPIO (BL=25) on PWM. The display does NOT
** depend on the AXP2602 — its begin() only wakes the battery fuel
** gauge, it gates no display rail — so no PMU init is needed to get
** a picture. AXP2602 is only required for the battery readouts below.
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // NOTE: analogWrite()/LEDC auto-attach on GPIO25 can fail silently on the
    // ESP32-C5 with this core and leave the backlight pin undriven (black
    // screen). Drive it as a plain digital on/off instead so the panel is
    // always lit. PWM dimming can be restored later with an explicit
    // ledcAttach(TFT_BL, 5000, 8) once confirmed working.
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, brightval > 0 ? HIGH : LOW);
}

/*********************************************************************
** Function: InputHandler
** BTN_B (28) -> Next (click) / Sel (double click or hold)
** BTN_A (0)  -> Prev (click) / Esc (double click or hold)
**********************************************************************/
void InputHandler(void) { hal_buttons_poll_2(); }

/*********************************************************************
** Function: powerOff
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {}
