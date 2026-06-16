#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <SPI.h>
#include <globals.h>
#include <interface.h>

namespace {
SPIClass touchSPI(FSPI);
constexpr uint32_t TouchSpiHz = 2500000;
constexpr uint8_t XptCmdX = 0xD0;
constexpr uint8_t XptCmdY = 0x90;
constexpr int RawXMin = 408;
constexpr int RawXMax = 3741;
constexpr int RawYMin = 365;
constexpr int RawYMax = 3860;
constexpr int RawMinValid = 100;
constexpr int RawMaxValid = 4000;
bool touchReady = false;

uint16_t readXpt2046Axis(uint8_t command) {
    touchSPI.transfer(command);
    uint16_t value = touchSPI.transfer16(0x0000) >> 3;
    return value;
}

bool readXpt2046Raw(int16_t &rawX, int16_t &rawY) {
    constexpr uint8_t Samples = 5;
    uint32_t sumX = 0;
    uint32_t sumY = 0;
    uint8_t valid = 0;

    touchSPI.beginTransaction(SPISettings(TouchSpiHz, MSBFIRST, SPI_MODE0));
    digitalWrite(TOUCH_CS, LOW);
    delayMicroseconds(2);

    for (uint8_t i = 0; i < Samples; ++i) {
        uint16_t y = readXpt2046Axis(XptCmdY);
        uint16_t x = readXpt2046Axis(XptCmdX);
        if (x >= RawMinValid && x <= RawMaxValid && y >= RawMinValid && y <= RawMaxValid) {
            sumX += x;
            sumY += y;
            ++valid;
        }
    }

    digitalWrite(TOUCH_CS, HIGH);
    touchSPI.endTransaction();

    if (valid < 2) return false;
    rawX = sumX / valid;
    rawY = sumY / valid;
    return true;
}

int16_t clampCoord(long value, int16_t minValue, int16_t maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

void mapTouchToScreen(int16_t rawX, int16_t rawY, int16_t &screenX, int16_t &screenY) {
    int16_t portraitX = clampCoord(map(rawX, RawXMin, RawXMax, 0, TFT_WIDTH - 1), 0, TFT_WIDTH - 1);
    int16_t portraitY = clampCoord(map(rawY, RawYMin, RawYMax, 0, TFT_HEIGHT - 1), 0, TFT_HEIGHT - 1);

    switch (bruceConfigPins.rotation) {
        case 1:
            screenX = clampCoord(portraitY, 0, tftWidth - 1);
            screenY = clampCoord((TFT_WIDTH - 1) - portraitX, 0, tftHeight - 1);
            break;
        case 2:
            screenX = clampCoord((TFT_WIDTH - 1) - portraitX, 0, tftWidth - 1);
            screenY = clampCoord((TFT_HEIGHT - 1) - portraitY, 0, tftHeight - 1);
            break;
        case 3:
            screenX = clampCoord((TFT_HEIGHT - 1) - portraitY, 0, tftWidth - 1);
            screenY = clampCoord(portraitX, 0, tftHeight - 1);
            break;
        case 0:
        default:
            screenX = clampCoord(portraitX, 0, tftWidth - 1);
            screenY = clampCoord(portraitY, 0, tftHeight - 1);
            break;
    }
}
} // namespace

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    touchReady = true;
    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   second stage gpio setup after TFT is initialized
***************************************************************************************/
void _post_setup_gpio() {
    // Backlight is wired fixed to 3.3V (TFT_BL/BACKLIGHT = -1); no PWM pin to initialize.
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Battery ADC is not defined for this shield profile.
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
** Description:   Charger detection is not defined for this shield profile.
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: setBrightness
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { (void)brightval; }

/*********************************************************************
** Function: InputHandler
** Handles touch input and maps it to Bruce touch globals.
**********************************************************************/
void InputHandler(void) {
    static uint32_t lastEvent = 0;
    static bool lastTouchState = false;

    if (!touchReady) return;

    int16_t rawX = 0;
    int16_t rawY = 0;
    bool currentTouchState = readXpt2046Raw(rawX, rawY);

    if (!currentTouchState) {
        lastTouchState = false;
        touchPoint.pressed = false;
        return;
    }

    if (lastTouchState && !LongPress) return;
    if (!LongPress && (millis() - lastEvent) < 120) return;

    lastTouchState = true;
    lastEvent = millis();

    int16_t x = 0;
    int16_t y = 0;
    mapTouchToScreen(rawX, rawY, x, y);

    if (!wakeUpScreen()) {
        AnyKeyPress = true;
        SelPress = true;
    } else {
        return;
    }

    touchPoint.x = x;
    touchPoint.y = y;
    touchPoint.pressed = true;
    touchHeatMap(touchPoint);
}

/*********************************************************************
** Function: powerOff
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { esp_deep_sleep_start(); }

/*********************************************************************
** Function: checkReboot
** Btn logic to turn off the device (not used: HAS_BTN=0)
**********************************************************************/
void checkReboot() {}
