/*
 * Waveshare ESP32-S3-Touch-AMOLED-1.75C - Bruce board interface
 *
 *   Display : CO5300 466x466 round AMOLED (QSPI) via Arduino_GFX
 *             Bruce draws into a centred 340x340 window (see pins_arduino.h)
 *   Touch   : CST9217 @0x5A (SensorLib TouchDrvCST92xx), INT=11, RST=2
 *   PMU     : AXP2101 @0x34 (battery %, charging, PWR key, power off)
 *   Audio   : ES8311 @0x18 + NS4150B amp (GPIO46, active HIGH)
 *   Buttons : BOOT (GPIO0) short = Select/confirm, hold ~0.7s = brightness
 *             PWR short = Next (switch item), PWR hold ~1.5s = Back
 */
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/settings.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <globals.h>
#include <interface.h>

#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>
#include <TouchDrvCSTXXX.hpp>

static XPowersAXP2101 pmu;
static TouchDrvCST92xx touch;
static bool pmuOk = false;
static bool touchOk = false;
static volatile bool touchIrq = false;

static void IRAM_ATTR onTouchIrq() { touchIrq = true; }

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(BTN_PIN, INPUT_PULLUP);

    // Speaker amplifier off until audio is used
    pinMode(AMP_EN_PIN, OUTPUT);
    digitalWrite(AMP_EN_PIN, LOW);

    // One shared I2C bus for every on-board peripheral
    setSysI2CBus(&Wire);
    Wire.begin(SYS_I2C_SDA, SYS_I2C_SCL);
    Wire.setClock(400000);
    delay(10);

    // ---------------- PMU ----------------
    pmuOk = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, SYS_I2C_SDA, SYS_I2C_SCL);
    if (pmuOk) {
        pmu.disableTSPinMeasure(); // no NTC on this board
        pmu.enableBattDetection();
        pmu.enableVbusVoltageMeasure();
        pmu.enableBattVoltageMeasure();
        pmu.enableSystemVoltageMeasure();

        pmu.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
        pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_400MA);
        pmu.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
        pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

        pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_6S);
        pmu.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

        pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        pmu.clearIrqStatus();
        pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ);
        Serial.println("AXP2101 init OK");
    } else {
        Serial.println("AXP2101 not found");
    }

    // ---------------- Touch ----------------
    pinMode(TOUCH_INT_PIN, INPUT_PULLUP);
    touch.setPins(TOUCH_RST_PIN, TOUCH_INT_PIN);
    touchOk = touch.begin(Wire, TOUCH_I2C_ADDR, SYS_I2C_SDA, SYS_I2C_SCL);
    if (touchOk) {
        touch.setMaxCoordinates(AMOLED_PHYS_SIZE, AMOLED_PHYS_SIZE);
        touch.setMirrorXY(true, true);
        attachInterrupt(digitalPinToInterrupt(TOUCH_INT_PIN), onTouchIrq, FALLING);
        Serial.printf("Touch init OK: %s\n", touch.getModelName());
    } else {
        Serial.println("CST9217 touch not found");
    }

    // No on-board CC1101/NRF24 - hide those menus by default
    bruceConfig.disabledMenus.push_back("RF");
    bruceConfig.disabledMenus.push_back("NRF24");
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   second stage setup, after the display is running
***************************************************************************************/
void _post_setup_gpio() {
    Arduino_GFX *g = tft.native();
    if (!g) return;
    // Clear the whole round panel (Bruce only draws inside the centred window,
    // the area around it would otherwise show random RAM content after power-up).
    // Negative coords cancel the window offset, so this addresses all 466x466 pixels.
    Arduino_CO5300 *panel = static_cast<Arduino_CO5300 *>(g);
    // Exactly the visible panel area, like Waveshare's own full-screen fill:
    // RAM columns 6..471, rows 0..465 (window offset cancelled by the negative coords).
    panel->startWrite();
    panel->writeFillRectPreclipped(-AMOLED_WIN_OFS, -AMOLED_WIN_OFS, AMOLED_PHYS_SIZE, AMOLED_PHYS_SIZE, 0x0000);
    panel->endWrite();
    panel->setBrightness(255);

#ifdef PANEL_TEST_CARD
    // One-time diagnostic: which primitives does this panel actually show?
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    const int lx = 20, sx = 150;
    tft.drawString("1 HLINE", lx, 30, 1);
    tft.drawFastHLine(sx, 37, 150, TFT_WHITE);
    tft.drawString("2 VLINE", lx, 60, 1);
    tft.drawFastVLine(sx, 55, 25, TFT_WHITE);
    tft.drawFastVLine(sx + 21, 55, 25, TFT_WHITE);
    tft.drawString("3 HLINE2", lx, 90, 1);
    tft.fillRect(sx, 96, 150, 2, TFT_WHITE);
    tft.drawString("4 VLINE2", lx, 120, 1);
    tft.fillRect(sx, 115, 2, 25, TFT_WHITE);
    tft.fillRect(sx + 21, 115, 2, 25, TFT_WHITE);
    tft.drawString("5 PIXELS", lx, 150, 1);
    for (int i = 0; i < 150; i += 4) tft.drawPixel(sx + i, 157, TFT_WHITE);
    tft.drawString("6 RRECT", lx, 180, 1);
    tft.drawRoundRect(sx, 172, 150, 30, 15, TFT_WHITE);
    tft.drawString("7 TRI", lx, 215, 1);
    tft.fillTriangle(sx, 222, sx + 30, 208, sx + 30, 236, TFT_WHITE);
    tft.drawString("8 LINE", lx, 250, 1);
    tft.drawLine(sx, 245, sx + 150, 265, TFT_WHITE);
    tft.drawString("9 BLOCK", lx, 285, 1);
    tft.fillRect(sx, 280, 40, 20, TFT_WHITE);
    for (int s = 25; s > 0; s--) {
        tft.fillRect(260, 280, 60, 20, TFT_BLACK);
        tft.drawString(String(s), 270, 285, 1);
        delay(1000);
    }
#endif
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 0-100
***************************************************************************************/
int getBattery() {
    if (!pmuOk) return 0;
    if (!pmu.isBatteryConnect()) return 0;
    int p = pmu.getBatteryPercent();
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    return p;
}

/***************************************************************************************
** Function name: isCharging()
***************************************************************************************/
bool isCharging() { return pmuOk && pmu.isCharging(); }

/*********************************************************************
** Function: _setBrightness
** AMOLED brightness is set through the CO5300 (0x51) register
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    Arduino_GFX *g = tft.native();
    if (!g) return;
    uint8_t v;
    if (brightval == 0) v = 0;
    else v = MINBRIGHT + (uint8_t)(((255 - MINBRIGHT) * (uint32_t)brightval) / 100);
    static_cast<Arduino_CO5300 *>(g)->setBrightness(v);
}

/*********************************************************************
** Touch helpers
**********************************************************************/
static bool readTouch(int16_t &x, int16_t &y) {
    if (!touchOk) return false;
    bool irq = touchIrq;
    if (!irq && digitalRead(TOUCH_INT_PIN) != LOW) return false;
    touchIrq = false;

    int16_t tx[2] = {0}, ty[2] = {0};
    uint8_t n = touch.getPoint(tx, ty, 1);
    if (n == 0) return false;

    // Physical (0..465) -> Bruce window (0..AMOLED_WINDOW-1)
    int16_t px = tx[0] - AMOLED_WIN_OFS;
    int16_t py = ty[0] - AMOLED_WIN_OFS;
    // Touches far out in the round rim (well outside the UI) are ignored;
    // slightly outside is snapped to the edge so edge buttons stay easy to hit.
    const int16_t SLACK = 40;
    if (px < -SLACK || py < -SLACK || px >= AMOLED_WINDOW + SLACK || py >= AMOLED_WINDOW + SLACK) return false;
    if (px < 0) px = 0;
    if (py < 0) py = 0;
    if (px >= AMOLED_WINDOW) px = AMOLED_WINDOW - 1;
    if (py >= AMOLED_WINDOW) py = AMOLED_WINDOW - 1;
    x = px;
    y = py;
    return true;
}

// PWR key events from the AXP2101: 0 = none, 1 = short press, 2 = long press
static uint8_t pwrKeyEvent() {
    if (!pmuOk) return 0;
    static unsigned long last = 0;
    if (millis() - last < 60) return 0; // don't hammer the I2C bus
    last = millis();
    uint64_t st = pmu.getIrqStatus();
    // A failed I2C read comes back as all ones - never treat that as a key press
    if ((st & 0xFFFFFFULL) == 0xFFFFFFULL) return 0;
    uint8_t ev = 0;
    if (pmu.isPekeyLongPressIrq()) ev = 2;
    else if (pmu.isPekeyShortPressIrq()) ev = 1;
    if (st) pmu.clearIrqStatus();
    return ev;
}

/*********************************************************************
** Function: InputHandler
** Handles PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    // Touch "tap" tracking: the CST9217 keeps reporting while the finger is
    // down. Only the first report of a touch counts as a press; the next press
    // needs the finger lifted (no report for RELEASE_MS) unless Bruce asks for
    // repeats via LongPress.
    static unsigned long lastTouchSeen = 0;
    static bool fingerDown = false;
    const unsigned long RELEASE_MS = 120;

    int16_t x, y;
    bool touched = readTouch(x, y); // cheap when INT is idle (no I2C traffic)
    if (touched) {
        bool newTap = !fingerDown || (millis() - lastTouchSeen > RELEASE_MS);
        lastTouchSeen = millis();
        fingerDown = true;
        if (!newTap && !(LongPress && millis() - tm > 200)) touched = false;
    } else if (fingerDown && millis() - lastTouchSeen > RELEASE_MS) {
        fingerDown = false;
    }

    if (!touched && millis() - tm < 200 && !LongPress) return;

    // ---- Touch ----
    if (touched) {
        const int16_t W = AMOLED_WINDOW - 1;
        int16_t t_x = x, t_y = y;
        switch (bruceConfigPins.rotation) {
            case 1: t_x = y; t_y = W - x; break;
            case 2: t_x = W - x; t_y = W - y; break;
            case 3: t_x = W - y; t_y = x; break;
            default: break;
        }
        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;

        touchPoint.x = t_x;
        touchPoint.y = t_y;
        touchPoint.pressed = true;
        touchHeatMap(touchPoint);
        return;
    }

    // ---- BOOT button: short press = Select, hold ~0.7s = cycle brightness ----
    if (digitalRead(BTN_PIN) == BTN_ACT) {
        unsigned long s = millis();
        while (digitalRead(BTN_PIN) == BTN_ACT && millis() - s < 700) delay(10);
        bool held = (millis() - s) >= 700;
        tm = millis();
        if (wakeUpScreen()) {
            // screen was asleep: this press only wakes it
        } else if (held) {
            static const uint8_t levels[] = {25, 50, 75, 100};
            uint8_t next = levels[0];
            for (uint8_t i = 0; i < sizeof(levels); i++) {
                if (bruceConfig.bright < levels[i]) {
                    next = levels[i];
                    break;
                }
            }
            if (bruceConfig.bright >= 100) next = levels[0];
            setBrightness(next, true); // applies via _setBrightness and saves it
            // wait for release so the hold doesn't also count as a Select
            while (digitalRead(BTN_PIN) == BTN_ACT) delay(10);
        } else {
            AnyKeyPress = true;
            SelPress = true;
        }
        return;
    }

    // ---- PWR key: short press = Next (switch item), long press (~1.5s) = Back ----
    uint8_t pk = pwrKeyEvent();
    if (pk) {
        tm = millis();
        if (!wakeUpScreen()) {
            AnyKeyPress = true;
            if (pk == 2) EscPress = true;
            else NextPress = true;
        }
    }
}

/*********************************************************************
** Function: powerOff
** Cuts power via the AXP2101 (press PWR to turn back on)
**********************************************************************/
void powerOff() {
    digitalWrite(AMP_EN_PIN, LOW);
    Arduino_GFX *g = tft.native();
    if (g) static_cast<Arduino_CO5300 *>(g)->setBrightness(0);
    if (pmuOk) {
        pmu.shutdown();
        delay(500);
    }
    // Fallback: deep sleep, wake with BOOT
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, BTN_ACT);
    esp_deep_sleep_start();
}

void goToDeepSleep() { powerOff(); }

/*********************************************************************
** Function: checkReboot
** Hold BOOT for ~3s to power off
**********************************************************************/
void checkReboot() {
    int c = 0;
    while (digitalRead(BTN_PIN) == BTN_ACT) {
        delay(100);
        c++;
        if (c > 30) powerOff();
    }
}

// =============================================
// ES8311 Audio Codec (speaker)
// =============================================
#ifdef ES8311_CODEC
static void es8311_write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

void _setup_codec_speaker(bool enable) {
    if (enable) {
        es8311_write_reg(0x00, 0x1F); // reset
        delay(20);
        es8311_write_reg(0x00, 0x00);
        es8311_write_reg(0x00, 0x80); // power on, slave mode

        es8311_write_reg(0x01, 0x3F); // MCLK from MCLK pin, all clocks on
        es8311_write_reg(0x02, 0x00);
        es8311_write_reg(0x03, 0x10);
        es8311_write_reg(0x04, 0x10);
        es8311_write_reg(0x05, 0x00);
        es8311_write_reg(0x06, 0x03);
        es8311_write_reg(0x07, 0x00);
        es8311_write_reg(0x08, 0xFF);

        es8311_write_reg(0x09, 0x0C); // SDP in 16-bit I2S
        es8311_write_reg(0x0A, 0x0C); // SDP out 16-bit I2S

        es8311_write_reg(0x0D, 0x01); // power up analog
        es8311_write_reg(0x0E, 0x02);
        es8311_write_reg(0x12, 0x00); // power up DAC
        es8311_write_reg(0x13, 0x10); // HP drive
        es8311_write_reg(0x1C, 0x6A);
        es8311_write_reg(0x37, 0x08); // bypass DAC EQ
        es8311_write_reg(0x32, 0xBF); // DAC volume

        digitalWrite(AMP_EN_PIN, HIGH);
    } else {
        digitalWrite(AMP_EN_PIN, LOW);
        es8311_write_reg(0x0D, 0xFC);
        es8311_write_reg(0x00, 0x00);
    }
}
#endif
