#include "core/i2c_finder.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <globals.h>
#include <interface.h>

#ifdef XPOWERS_CHIP_BQ25896
#include <Wire.h>
#include <XPowersLib.h>
XPowersPPM PPM;
#endif

// Interrupt flag from expander
volatile bool expanderInterrupt = false;

void IRAM_ATTR expanderISR() { expanderInterrupt = true; }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {

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

    Wire.begin(GROVE_SDA, GROVE_SCL);

    // Configure buttons on expander as inputs
    ioExpander.button(IO_EXP_UP);
    ioExpander.button(IO_EXP_DOWN);
    ioExpander.button(IO_EXP_SEL);
    ioExpander.button(IO_EXP_ESC);
    ioExpander.button(IO_EXP_LEFT);
    ioExpander.button(IO_EXP_RIGHT);

    // Enable interrupts on the button pins only
    ioExpander.enableGPIOInterrupts();

    // Setup interrupt pin from AW9523 INT (active LOW)
    pinMode(EXPANDER_INT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EXPANDER_INT_PIN), expanderISR, FALLING);

    // === PMU (BQ25896) Setup ===
    bool pmu_ret = PPM.init(); // Modern XPowersLib prefers this
    if (pmu_ret) {
        PPM.setSysPowerDownVoltage(3300);
        PPM.setInputCurrentLimit(3250);
        Serial.printf("getInputCurrentLimit: %d mA\n", PPM.getInputCurrentLimit());
        PPM.disableCurrentLimitPin();
        PPM.setChargeTargetVoltage(4208);
        PPM.setPrechargeCurr(64);
        PPM.setChargerConstantCurr(320);
        PPM.enableMeasure(PowersBQ25896::CONTINUOUS);
        PPM.disableOTG();
        PPM.enableCharge();
    } else {
        Serial.println("BQ25896 init failed!");
    }

    pinMode(NRF24_SS_PIN, OUTPUT);
    pinMode(CC1101_SS_PIN, OUTPUT);
    pinMode(SDCARD_CS, OUTPUT);
    // pinMode(W5500_SS_PIN, OUTPUT);
    pinMode(TFT_CS, OUTPUT);

    digitalWrite(NRF24_SS_PIN, HIGH);
    digitalWrite(CC1101_SS_PIN, HIGH);
    digitalWrite(SDCARD_CS, HIGH);
    // digitalWrite(W5500_SS_PIN, HIGH);
    digitalWrite(TFT_CS, HIGH);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
***************************************************************************************/
void _post_setup_gpio() {
    // Can be used for second-stage init if needed
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    uint16_t voltage = PPM.getBattVoltage();

    if (voltage < 3300) return 0;
    if (voltage >= 4200) return 100;

    if (voltage >= 3900) return 80 + (voltage - 3900) * 20 / 300;
    else if (voltage >= 3700) return 40 + (voltage - 3700) * 40 / 200;
    else if (voltage >= 3500) return 10 + (voltage - 3500) * 30 / 200;
    else return (voltage - 3300) * 10 / 200;
}

/***************************************************************************************
** Function name: isCharging()
***************************************************************************************/
bool isCharging() { return PPM.isCharging(); }

/*********************************************************************
** Function: setBrightness
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100.0));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles PrevPress, NextPress, SelPress, AnyKeyPress, EscPress
** using IO Expander
**********************************************************************/
void InputHandler() {
    static unsigned long tm = 0;

    static bool lastUp = false;
    static bool lastDown = false;
    static bool lastLeft = false;
    static bool lastRight = false;
    static bool lastSel = false;
    static bool lastEsc = false;

    uint16_t pins = ioExpander.inputGPIO();

    bool up = !(pins & (1 << IO_EXP_UP));
    bool down = !(pins & (1 << IO_EXP_DOWN));
    bool left = !(pins & (1 << IO_EXP_LEFT));
    bool right = !(pins & (1 << IO_EXP_RIGHT));
    bool sel = !(pins & (1 << IO_EXP_SEL));
    bool esc = !(pins & (1 << IO_EXP_ESC));
    bool anyPressed = up || down || left || right || sel || esc;

    if (!expanderInterrupt && !LongPress && (millis() - tm < 250)) { return; }

    if (anyPressed) {
        tm = millis();

        if (!wakeUpScreen()) {
            AnyKeyPress = true;
        } else {
            return;
        }
    }

    UpPress = up && !lastUp;
    DownPress = down && !lastDown;
    PrevPress = left && !lastLeft;
    NextPress = right && !lastRight;
    SelPress = sel && !lastSel;
    EscPress = esc && !lastEsc;

    PrevPagePress = UpPress;
    NextPagePress = DownPress;

    lastUp = up;
    lastDown = down;
    lastLeft = left;
    lastRight = right;
    lastSel = sel;
    lastEsc = esc;

    expanderInterrupt = false;
}
/*********************************************************************
** Function: powerOff
**********************************************************************/
void powerOff() { PPM.shutdown(); }

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {
    int countDown;
    /* Long press power off */
    if (ioExpander.readPin(IO_EXP_ESC) == LOW) {
        uint32_t time_count = millis();
        while (ioExpander.readPin(IO_EXP_ESC) == LOW) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 3)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/2", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (ioExpander.readPin(IO_EXP_ESC) == LOW);
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
