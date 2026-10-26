#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"

#define SEL_BTN 0
#define BTN_ACT LOW
#define DW_BTN 40
#define L_BTN 39
#define R_BTN 38
#define UP_BTN 41

static DeviceButtons buttonsCfg() { return DeviceButtons{L_BTN, R_BTN, UP_BTN, DW_BTN, SEL_BTN}; }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/

// Power handler for battery detection

void _setup_gpio() {
    // INMP441, standard (Philips) I2S
    bruceConfigPins.mic_bus = {
        (gpio_num_t)1, (gpio_num_t)10, (gpio_num_t)2, MIC_TYPE_I2S_PHILIPS
    }; // clk,data,ws,type
    bruceConfigPins.buzzer = 44;
    bruceConfigPins.sys_i2c = {(gpio_num_t)47, (gpio_num_t)48}; // sda, scl
    bruceConfigPins.i2c_bus = {(gpio_num_t)47, (gpio_num_t)48}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 47;
    bruceConfigPins.rfRx = 48;
    bruceConfigPins.irTx = 5;
    bruceConfigPins.irRx = 4;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)2, (gpio_num_t)1};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)2, (gpio_num_t)1};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)2, (gpio_num_t)1}; // rx, tx (shares SERIAL bus)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)12, (gpio_num_t)43};
    bruceConfigPins.PN532_bus = {(gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)12, (gpio_num_t)43};
    // CC1101/NRF24 share the same SPI bus (sck=13, miso=11, mosi=12)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)12, (gpio_num_t)46, (gpio_num_t)9, (gpio_num_t)10
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)12, (gpio_num_t)14, (gpio_num_t)21
    }; // sck,miso,mosi,cs(ss),ce
    // SDCARD is on its own SPI bus (sck=18, miso=8, mosi=17)
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)18, (gpio_num_t)8, (gpio_num_t)17, (gpio_num_t)3
    }; // sck,miso,mosi,cs

    hal_buttons_init(buttonsCfg(), 5);

    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);

    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);
    // Starts SPI instance for CC1101 and NRF24 with CS pins blocking communication at start

    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    setSysI2CBus(&Wire); // PMU lives on the default Wire object
    Wire.setPins(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
    DevicePmic pmicCfg;
    pmicCfg.pin_sda = bruceConfigPins.sys_i2c.sda;
    pmicCfg.pin_scl = bruceConfigPins.sys_i2c.scl;
    pmicCfg.address = 0x6B; // BQ25896
    hal_pmic_init(pmicCfg);

    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}
bool isCharging() {
    // PPM.disableBatterPowerPath();
    return hal_pmic_is_charging();
}

int getBattery() {
    int voltage = hal_pmic_get_batt_voltage_mv();
    int percent = (voltage - 3300) * 100 / (float)(4150 - 3350);

    if (percent < 0) return 1;
    if (percent > 100) percent = 100;

    if (hal_pmic_is_charging() && percent >= 97) {
        hal_pmic_disable_bat_load();
        percent = 95; // estimate still charging
    }

    if (hal_pmic_is_charge_done()) { percent = 100; }

    return percent;
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
void InputHandler(void) { hal_buttons_poll_5(buttonsCfg()); }

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    int countDown = 0;
    /* Long press power off */
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                if (countDown == 0) {
                    int textWidth = tft.textWidth("PWR OFF IN 3/3", 1);
                    tft.fillRect(tftWidth / 2 - textWidth / 2, 7, textWidth, 18, bruceConfig.bgColor);
                }
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(L_BTN) == BTN_ACT || digitalRead(R_BTN) == BTN_ACT);
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }

        // Clear text after releasing the button
        delay(30);
        if (millis() - time_count > 500) {
            tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
            drawStatusBar();
        }
    }
}
