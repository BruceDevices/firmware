#include "core/powerSave.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <interface.h>

// =============================================================================
//  Hosyond 3.5" ESP32-S3 interface
//  Display:  ST77922 320x480 QSPI (handled by LovyanGFX)
//  Touch:    ST77922 integrated capacitive touch via I2C @ 0x55
//  Backlight: PWM on GPIO41
//  NeoPixel:  WS2812 GRBW on GPIO40
//  Audio:     ES8311 I2S codec (MCK=17, BCK=18, DOUT=15, WS=21)
//  SD/MMC:    4-bit mode (CLK=5, CMD=4, D0-D3=6,7,2,3)
// =============================================================================

#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_ST77922_I2C)

// ST77922 touch register map (from vendor esp_lcd_st77922.c)
#define ST77922_TOUCH_INFO_REG   0x0010  // adv_info: bit3=with_coord
#define ST77922_MAX_TOUCHES_REG  0x0009  // number of active touch points
#define ST77922_COORD0_REG       0x0014  // first touch-point data (7 bytes each)

static bool touch_ready = false;

// Write a 16-bit register address then read count bytes.
static bool tp_read(uint16_t reg, uint8_t *buf, uint8_t count) {
    Wire1.beginTransmission(ST77922_TOUCH_ADDR);
    Wire1.write((uint8_t)(reg >> 8));
    Wire1.write((uint8_t)(reg & 0xFF));
    if (Wire1.endTransmission(false) != 0) return false;
    uint8_t received = Wire1.requestFrom((uint8_t)ST77922_TOUCH_ADDR, count);
    if (received != count) return false;
    for (uint8_t i = 0; i < count; i++) buf[i] = Wire1.read();
    return true;
}

// Returns 1 if a valid touch is detected and sets x/y; returns 0 otherwise.
// Coordinate format per touch_report_t in vendor driver (7 bytes/point):
//   byte0 bits[7:2] = x_h (6 bits), bit1 = reserved, bit0 = valid
//   byte1            = x_l (8 bits)  → x = (x_h << 8) | x_l  (14-bit, display units)
//   byte2            = y_h (8 bits)
//   byte3            = y_l (8 bits)  → y = (y_h << 8) | y_l  (16-bit, display units)
//   byte4 = area, byte5 = intensity, byte6 = reserved
static uint8_t tp_get_point(int16_t *x, int16_t *y) {
    uint8_t adv_info = 0;
    if (!tp_read(ST77922_TOUCH_INFO_REG, &adv_info, 1)) return 0;
    if (!(adv_info & 0x08)) return 0;  // bit3: with_coord

    uint8_t num_points = 0;
    if (!tp_read(ST77922_MAX_TOUCHES_REG, &num_points, 1)) return 0;
    if (num_points == 0 || num_points > 10) return 0;

    uint8_t data[7];
    if (!tp_read(ST77922_COORD0_REG, data, 7)) return 0;

    bool valid = (data[0] & 0x01);
    if (!valid) return 0;

    // Coordinates are reported in display pixel units by the touch firmware
    *x = (int16_t)(((uint16_t)(data[0] >> 2) << 8) | data[1]);
    *y = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    return 1;
}

#endif // HAS_CAPACITIVE_TOUCH && TOUCH_ST77922_I2C

/***************************************************************************************
** Function name: _setup_gpio()
***************************************************************************************/
void _setup_gpio() {
    bruceConfig.colorInverted = 0;

#if defined(USE_SD_MMC)
    // Configure 4-bit SDMMC pins before first SD.begin() call
    SD.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
#endif

#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_ST77922_I2C)
    // Power-on reset for the ST77922 touch controller
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(50);

    pinMode(TOUCH_INT, INPUT_PULLUP);

    Wire1.begin(TOUCH_SDA, TOUCH_SCL, 400000);
    touch_ready = true;
    Serial.println("ST77922 touch initialised");
#endif
}

/***************************************************************************************
** Function name: _post_setup_gpio()
***************************************************************************************/
void _post_setup_gpio() {
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);

#ifdef HOSYOND_TEST_PATTERN
    {
        int w = tft.width(), h = tft.height();
        Serial.printf("[TESTPAT] tft %dx%d rot=%d\n", w, h, tft.getRotation());
        tft.fillScreen(TFT_BLACK);
        tft.drawRect(0, 0, w, h, TFT_RED);
        tft.drawRect(1, 1, w - 2, h - 2, TFT_GREEN);
        tft.fillRect(0, 0, 40, 40, TFT_BLUE);
        tft.fillRect(w - 40, 0, 40, 40, TFT_YELLOW);
        tft.fillRect(0, h - 40, 40, 40, TFT_WHITE);
        tft.fillRect(w - 40, h - 40, 40, 40, TFT_MAGENTA);
        tft.fillRect(0, h / 2 - 4, w, 8, TFT_CYAN);
        tft.fillRect(w / 2 - 4, 0, 8, h, TFT_ORANGE);
        tft.drawLine(0, 0, w - 1, h - 1, TFT_WHITE);
        {
            static const uint16_t bars[8] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000, 0xFFE0, 0x07FF, 0xF81F};
            const int iw = 160, ih = 120;
            uint16_t *img = (uint16_t *)ps_malloc(iw * ih * 2);
            if (img) {
                for (int y = 0; y < ih; y++)
                    for (int x = 0; x < iw; x++)
                        img[y * iw + x] = (y % 30 == 0) ? 0x0000 : bars[x / 20];
                tft.pushImage(80, 60, iw, ih, img);
                const int sw = 50, sh = 50;
                for (int y = 0; y < sh; y++)
                    for (int x = 0; x < sw; x++) img[y * sw + x] = ((x / 10 + y / 10) & 1) ? 0xF800 : 0x001F;
                tft.pushImage(10, 330, sw, sh, img);
                free(img);
            }
        }
        for (int i = 0; i < 8; i++) {
            tft.fillRect(10, 200 + i * 14, i + 1, 10, TFT_WHITE);       // width 1..8 at x=10
            tft.fillRect(170 + i, 200 + i * 14, 8, 10, TFT_WHITE);      // width 8 at x offset 0..7
        }
        delay(8000);
    }
#endif
}

/***************************************************************************************
** Function name: getBattery()
***************************************************************************************/
int getBattery() {
#if defined(ANALOG_BAT_PIN) && ANALOG_BAT_PIN >= 0
    // 2× voltage divider; ADC full-scale = 3.3 V → 4.2 V max battery
    int raw = analogRead(ANALOG_BAT_PIN);
    int mv = (raw * 3300 * 2) / 4095;
    // Linear approximation 3400 mV = 0 %, 4200 mV = 100 %
    int pct = (mv - 3400) * 100 / 800;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
#else
    return 100;
#endif
}

/*********************************************************************
** Function: _setBrightness
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75)  dutyCycle = 130;
    else if (brightval == 50)  dutyCycle = 70;
    else if (brightval == 25)  dutyCycle = 20;
    else if (brightval == 0)   dutyCycle = 0;
    else dutyCycle = (int)((brightval * 255) / 100);
    ledcWrite(TFT_BL, dutyCycle);
}

/*********************************************************************
** Function: InputHandler
**********************************************************************/
void InputHandler(void) {
#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_ST77922_I2C)
    if (!touch_ready) {
        checkPowerSaveTime();
        return;
    }

    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        static unsigned long tm = millis();
        int16_t tx = 0, ty = 0;

        static bool lastTouchState = false;
        static unsigned long lastTouchTime = 0;

        uint8_t touched = tp_get_point(&tx, &ty);

        // Adjust coordinates for current rotation
        if (touched) {
            int16_t tmp;
            switch (bruceConfigPins.rotation) {
                case 0:  // portrait
                    break;
                case 1:  // landscape: x = raw_y, y = (WIDTH-1) - raw_x
                    tmp = tx;
                    tx  = ty;
                    ty  = (TFT_WIDTH - 1) - tmp;
                    break;
                case 2:  // portrait inverted
                    tx = (TFT_WIDTH  - 1) - tx;
                    ty = (TFT_HEIGHT - 1) - ty;
                    break;
                case 3:  // landscape inverted
                    tmp = tx;
                    tx  = (TFT_HEIGHT - 1) - ty;
                    ty  = tmp;
                    break;
                default: break;
            }
        }

        bool currentTouchState = (touched > 0);
        if (currentTouchState && !lastTouchState && (millis() - lastTouchTime) > 100) {
            lastTouchTime = millis();
        } else if (!currentTouchState || lastTouchState) {
            touched = 0;
        }
        lastTouchState = currentTouchState;

        if (((millis() - tm) > 190 || LongPress) && touched) {
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else goto END;

            touchPoint.x = tx;
            touchPoint.y = ty;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        END:
            d_tmp = millis();
        }

        if (!touched) checkPowerSaveTime();
    }
#else
    checkPowerSaveTime();
    PrevPress  = false;
    NextPress  = false;
    SelPress   = false;
    AnyKeyPress = false;
    EscPress   = false;
#endif
}

/*********************************************************************
** Function: powerOff
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {}
