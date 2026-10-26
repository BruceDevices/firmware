#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <globals.h>
#include <interface.h>

// Power handler for battery detection
#include "core/i2c_finder.h"
#include <Wire.h>

#define SEL_BTN 0

#define BTN_ACT LOW
#define DW_BTN 40
#define ESC_BTN 21
#define L_BTN 39
#define R_BTN 38
#define UP_BTN 41

static DeviceButtons buttonsCfg() { return DeviceButtons{L_BTN, R_BTN, UP_BTN, DW_BTN, SEL_BTN, ESC_BTN}; }
// Charger chip

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/

// BATTERY GAUGE
#define BATTERY_DESIGN_CAPACITY 1000
bool gaugeOn = false;

void _setup_gpio() {
    bruceConfigPins.buzzer = 10;
    bruceConfigPins.sys_i2c = {(gpio_num_t)47, (gpio_num_t)48}; // sda, scl
    bruceConfigPins.i2c_bus = {(gpio_num_t)47, (gpio_num_t)48}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 47;
    bruceConfigPins.rfRx = 48;
    bruceConfigPins.irTx = 47;
    bruceConfigPins.irRx = 1;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};     // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)48, (gpio_num_t)47};  // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)11};
    bruceConfigPins.PN532_bus = {(gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)11};
    // CC1101/NRF24/SDCARD/LoRa/ST25R share the main SPI bus (sck=17, miso=8, mosi=18)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)9, (gpio_num_t)46, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)13, (gpio_num_t)14
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)3
    }; // sck,miso,mosi,cs
    bruceConfigPins.ST25R_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)11, (gpio_num_t)12, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,irq,-
#if !defined(LITE_VERSION)
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)4, (gpio_num_t)43, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    setSysI2CBus(&Wire); // PMU/battery gauge live on the default Wire object
    Wire.setPins(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);

    hal_buttons_init(buttonsCfg(), 6);

    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.ST25R_bus.cs, OUTPUT); /// NFC PIN
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.ST25R_bus.cs, HIGH);

    // Starts SPI instance for CC1101 and NRF24 with CS pins blocking communication at start

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);

    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);

    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = ST25R3916_SPI_MODULE;

    DevicePmic pmicCfg;
    pmicCfg.pin_sda = bruceConfigPins.sys_i2c.sda;
    pmicCfg.pin_scl = bruceConfigPins.sys_i2c.scl;
    pmicCfg.address = 0x6B; // BQ25896
    hal_pmic_init(pmicCfg, 2000);
    Wire.beginTransmission(BQ27220_I2C_ADDRESS);
    if (Wire.endTransmission() == 0) {
        DeviceGauge gaugeCfg;
        gaugeCfg.design_capacity_mah = BATTERY_DESIGN_CAPACITY;
        hal_gauge_init(gaugeCfg);
        gaugeOn = true;
    }
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp

** Description:   Delivers the battery value from 1-100+
***************************************************************************************/
int getBattery() {
    int percent = 0;
#if defined(GAUGE_BQ27220)
    if (gaugeOn) percent = hal_gauge_get_percent();
#endif

    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}

#ifdef GAUGE_BQ27220
bool isCharging() {
    return gaugeOn ? hal_gauge_is_charging() : false; // Return the charging status from BQ27220
}
#else
bool isCharging() { return false; }
#endif

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
void InputHandler(void) { hal_buttons_poll_6(buttonsCfg()); }

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { hal_pmic_shutdown(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    int countDown;
    /* Long press power off */
    if (digitalRead(ESC_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(ESC_BTN) == BTN_ACT) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 3)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/2", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(ESC_BTN) == BTN_ACT);
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
}
