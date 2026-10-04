#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <Adafruit_INA219.h>
#include <Wire.h>
#include <globals.h>
#include <interface.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/

// Battery (1S Li-ion) voltage window used to derive the percentage
#define BATTERY_MIN_MV 3300
#define BATTERY_MAX_MV 4150

// Current (mA) above which the pack is considered charging
#define CHARGING_CURRENT_MA 40

// Tune this if the measured bus voltage differs from the real pack voltage
// (shunt placement / boost stage). 1.0 = direct measurement.
#define BATTERY_VOLTAGE_MULTIPLIER 1.0f

// Battery readings are cached, the status bar asks for them on every redraw
#define BATTERY_CACHE_MS 5000

static Adafruit_INA219 ina219;
static bool ina219Ready = false;
static unsigned long lastBatteryRead = 0;
static int lastBatteryPercent = 100;
static float lastBatteryMa = 0;

static bool readIna219(float &busMv, float &currentMa) {
    if (!ina219Ready) return false;
    busMv = ina219.getBusVoltage_V() * 1000.0f;
    currentMa = ina219.getCurrent_mA();
    return true;
}

void _setup_gpio() {
    setSysI2CBus(&Wire);
    Wire.setPins(SYS_I2C_SDA, SYS_I2C_SCL);
    Wire.begin(SYS_I2C_SDA, SYS_I2C_SCL);

    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(R_BTN, INPUT_PULLUP);
    pinMode(L_BTN, INPUT_PULLUP);
    pinMode(ESC_BTN, INPUT_PULLUP);

    // Every device on the shared SPI bus must be deselected at boot
    pinMode(CC1101_SS_PIN, OUTPUT);
    pinMode(NRF24_SS_PIN, OUTPUT);
    pinMode(ST25R_CS, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    pinMode(SDCARD_CS, OUTPUT);
    pinMode(LORA_CS, OUTPUT);
    // W5500 shares the CS and control lines with the LoRa socket (see
    // pins_arduino.h). Repeating them is a no-op while both are 5/4/3, but
    // keeps the boot sequence correct if one of them gets re-assigned.
    pinMode(W5500_SS_PIN, OUTPUT);
    pinMode(W5500_RST_PIN, OUTPUT);
    digitalWrite(CC1101_SS_PIN, HIGH);
    digitalWrite(NRF24_SS_PIN, HIGH);
    digitalWrite(ST25R_CS, HIGH);
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(SDCARD_CS, HIGH);
    digitalWrite(LORA_CS, HIGH);
    digitalWrite(W5500_SS_PIN, HIGH);
    digitalWrite(W5500_RST_PIN, HIGH);

    pinMode(TFT_BL, OUTPUT);

    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = ST25R3916_SPI_MODULE;
    bruceConfigPins.irRx = RXLED;
    bruceConfigPins.irTx = TXLED;
    bruceConfigPins.iButton = IBUTTON_PIN;

    // The library hardcodes 0x40 (A0/A1 strapped to GND)
    ina219Ready = ina219.begin(&Wire);
    if (!ina219Ready) Serial.println("[fatalcore] INA219 not found, battery voltage unavailable");
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100+ (from the INA219 bus voltage)
***************************************************************************************/
int getBattery() {
    float busMv, currentMa;
    if (!readIna219(busMv, currentMa)) return 100;

    if (millis() - lastBatteryRead > BATTERY_CACHE_MS) {
        float mv = busMv * BATTERY_VOLTAGE_MULTIPLIER;
        float percent = ((mv - BATTERY_MIN_MV) / (float)(BATTERY_MAX_MV - BATTERY_MIN_MV)) * 100.0f;

        if (percent <= 0.0f) percent = 1;
        if (percent > 100.0f) percent = 100;

        lastBatteryPercent = (int)(percent + 0.5f);
        lastBatteryMa = currentMa;
        lastBatteryRead = millis();
    }

    return lastBatteryPercent;
}

/***************************************************************************************
** Function name: isCharging()
** location: interface.cpp
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() {
    if (!ina219Ready) return false;
    return lastBatteryRead != 0 && lastBatteryMa > CHARGING_CURRENT_MA;
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
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
    if (millis() - tm < 200 && !LongPress) return;
    bool _u = digitalRead(UP_BTN);
    bool _d = digitalRead(DW_BTN);
    bool _l = digitalRead(L_BTN);
    bool _r = digitalRead(R_BTN);
    bool _s = digitalRead(SEL_BTN);
    bool _e = digitalRead(ESC_BTN);

    if (!_s || !_u || !_d || !_r || !_l || !_e) {
        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (!_l) { PrevPress = true; }
    if (!_r) { NextPress = true; }
    if (!_u) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (!_d) {
        DownPress = true;
        NextPagePress = true;
    }
    if (!_s) { SelPress = true; }

    if (!_e) {
        EscPress = true;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    Serial.println("No PMIC on this board, going to deep sleep instead");
    goToDeepSleep();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    /* Long press ESC to power off */
    if (digitalRead(ESC_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(ESC_BTN) == BTN_ACT) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                int countDown = (millis() - time_count) / 1000 + 1;
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
