/*
 * Accretion Phone - DIY ESP32-S3 handheld
 * Board interface for Bruce firmware
 *
 * Hardware:
 *   - ESP32-S3 (16MB flash, OPI PSRAM)
 *   - ILI9341 240x320 TFT on SPI (TFT_eSPI, pins in pins_arduino.h)
 *   - XPT2046 resistive touch on its own pins (read by CYD28_TouchR, bit-banged)
 *   - microSD in SDIO 1-bit mode (SD_MMC): CLK=39 CMD=38 D0=40
 *   - INMP441 mic, WS2812 LED (GPIO48), vibration motor (GPIO18), battery ADC (GPIO8)
 */

#include "CYD28_TouchscreenR.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include <Arduino.h>
#include <globals.h>
#include <math.h>
#include <interface.h>

// SD card SDIO pins (1-bit mode)
#define ACCRETION_SD_CLK 39
#define ACCRETION_SD_CMD 38
#define ACCRETION_SD_D0 40

// Backlight PWM (attached after the TFT is initialised)
#define ACCRETION_BL_FREQ 5000
#define ACCRETION_BL_BITS 8

// Optional touch axis inversion, set from accretion-phone.ini
#ifndef ACCRETION_TOUCH_INVERT_X
#define ACCRETION_TOUCH_INVERT_X 0
#endif
#ifndef ACCRETION_TOUCH_INVERT_Y
#define ACCRETION_TOUCH_INVERT_Y 0
#endif

// XPT2046 is read in software SPI (same approach as the original CYD boards):
// it keeps the touch chip independent from the hardware SPI controllers that
// are shared between the display and the external radio modules.
CYD28_TouchR touch(320, 240);

// ---------------------------------------------------------------------------
// Touch calibration (XPT2046)
// The default CYD28_TouchR_CAL_* values are tuned for the CYD, not for this
// panel, so the first boot runs a 4-corner calibration. The result is stored in
// LittleFS and on the SD card (/accretion_touch.conf). To redo it later, keep a finger
// on the screen while the phone boots.
// ---------------------------------------------------------------------------
#define ACCRETION_CAL_FILE "/accretionTouchCal"      // LittleFS copy
#define ACCRETION_CAL_SD_FILE "/accretion_touch.conf" // SD card copy
#define ACCRETION_CAL_MARGIN 20
#define ACCRETION_CAL_TIMEOUT_MS 120000
#define ACCRETION_CAL_MIN_SPAN 1000 // min raw distance between the two calibrated edges

static bool accretionCalValid(int xmin, int xmax, int ymin, int ymax) {
    if (abs(xmax - xmin) < ACCRETION_CAL_MIN_SPAN || abs(ymax - ymin) < ACCRETION_CAL_MIN_SPAN) return false;
    if (xmin < -1500 || xmin > 5600 || xmax < -1500 || xmax > 5600) return false;
    if (ymin < -1500 || ymin > 5600 || ymax < -1500 || ymax > 5600) return false;
    return true;
}

// Read 5 values (xmin, xmax, ymin, ymax, swap) from a calibration file.
// Accepts "key=value" lines as well as plain numbers, one per line.
static bool accretionReadCalFile(FS &fs, const char *path, int v[5]) {
    File f = fs.open(path, FILE_READ);
    if (!f) return false;
    for (int i = 0; i < 5; i++) {
        String line = f.readStringUntil('\n');
        line.trim();
        int eq = line.indexOf('=');
        if (eq >= 0) line = line.substring(eq + 1);
        line.trim();
        if (line.length() == 0) {
            f.close();
            return false;
        }
        v[i] = line.toInt();
    }
    f.close();
    return accretionCalValid(v[0], v[1], v[2], v[3]);
}

static bool accretionWriteCalFile(FS &fs, const char *path, int xmin, int xmax, int ymin, int ymax, bool swap) {
    File f = fs.open(path, FILE_WRITE);
    if (!f) return false;
    f.printf("xmin=%d\nxmax=%d\nymin=%d\nymax=%d\nswap=%d\n", xmin, xmax, ymin, ymax, swap ? 1 : 0);
    f.close();
    return true;
}

static bool accretionSdReady() { return sdcardMounted || setupSdCard(); }

// SD card is checked first, then LittleFS. Whichever copy is missing gets re-created.
static bool accretionLoadTouchCal() {
    int v[5];
    bool sd = accretionSdReady();
    bool fromSd = sd && accretionReadCalFile(SD, ACCRETION_CAL_SD_FILE, v);
    bool fromFlash = !fromSd && accretionReadCalFile(LittleFS, ACCRETION_CAL_FILE, v);
    if (!fromSd && !fromFlash) return false;

    touch.setCalibration(v[0], v[1], v[2], v[3], v[4] != 0);
    // keep both copies in sync
    if (fromFlash && sd) accretionWriteCalFile(SD, ACCRETION_CAL_SD_FILE, v[0], v[1], v[2], v[3], v[4] != 0);
    if (fromSd && !LittleFS.exists(ACCRETION_CAL_FILE)) accretionWriteCalFile(LittleFS, ACCRETION_CAL_FILE, v[0], v[1], v[2], v[3], v[4] != 0);
    return true;
}

static void accretionSaveTouchCal(int xmin, int xmax, int ymin, int ymax, bool swap) {
    accretionWriteCalFile(LittleFS, ACCRETION_CAL_FILE, xmin, xmax, ymin, ymax, swap);
    if (accretionSdReady()) accretionWriteCalFile(SD, ACCRETION_CAL_SD_FILE, xmin, xmax, ymin, ymax, swap);
}

// true while the finger stays down for ~1.2 s right at boot
static bool accretionTouchHeldAtBoot() {
    if (!touch.touched()) return false;
    uint32_t t0 = millis();
    while (millis() - t0 < 1200) {
        if (!touch.touched()) return false;
        delay(20);
    }
    return true;
}

static void accretionDrawTarget(int x, int y, uint16_t color) {
    tft.drawFastHLine(x - 10, y, 21, color);
    tft.drawFastVLine(x, y - 10, 21, color);
    tft.drawCircle(x, y, 6, color);
}

// Wait for a press, average the raw readings, wait for release.
// Returns false only on timeout.
static bool accretionCaptureRaw(uint32_t deadline, int16_t &rx, int16_t &ry) {
    while (true) {
        while (!touch.touched()) {
            if (millis() > deadline) return false;
            delay(10);
        }
        int32_t sx = 0, sy = 0;
        int n = 0, seen = 0;
        while (touch.touched() && n < 20) {
            CYD28_TS_Point p = touch.getPointRaw();
            if (seen < 4) seen++; // first samples are noisy, skip them
            else {
                sx += p.x;
                sy += p.y;
                n++;
            }
            delay(12);
        }
        while (touch.touched()) {
            if (millis() > deadline) return false;
            delay(10);
        }
        delay(150); // debounce
        if (n >= 8) {
            rx = sx / n;
            ry = sy / n;
            return true;
        }
        // too short / bouncy tap: ask again
    }
}

static void accretionCalibrateTouch() {
    uint8_t oldRotation = bruceConfigPins.rotation;
    tft.setRotation(1); // calibrate in landscape, independent of the saved rotation
    const int W = tft.width();
    const int H = tft.height();
    const int m = ACCRETION_CAL_MARGIN;
    // order: 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
    const int tx[4] = {m, W - m, m, W - m};
    const int ty[4] = {m, m, H - m, H - m};
    bool done = false;

    for (int attempt = 0; attempt < 3 && !done; attempt++) {
        int16_t rx[4], ry[4];
        bool timeout = false;
        uint32_t deadline = millis() + ACCRETION_CAL_TIMEOUT_MS;

        for (int i = 0; i < 4 && !timeout; i++) {
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawCentreString("Kalibrasi Touch", W / 2, H / 2 - 20, 2);
            tft.drawCentreString("Sentuh titik + (pakai ujung pena/kuku)", W / 2, H / 2 + 4, 1);
            accretionDrawTarget(tx[i], ty[i], TFT_YELLOW);
            if (!accretionCaptureRaw(deadline, rx[i], ry[i])) timeout = true;
            else accretionDrawTarget(tx[i], ty[i], TFT_GREEN);
        }
        if (timeout) break;

        // Which raw axis moves along the screen's X axis? (top-left -> top-right)
        bool swap = (abs(ry[1] - ry[0]) + abs(ry[3] - ry[2])) > (abs(rx[1] - rx[0]) + abs(rx[3] - rx[2]));
        if (swap) {
            for (int i = 0; i < 4; i++) {
                int16_t t = rx[i];
                rx[i] = ry[i];
                ry[i] = t;
            }
        }

        // Both pairs of edges must agree on direction and be clearly apart
        int dxTop = rx[1] - rx[0], dxBot = rx[3] - rx[2];
        int dyLeft = ry[2] - ry[0], dyRight = ry[3] - ry[1];
        bool sane = (dxTop * dxBot > 0) && (dyLeft * dyRight > 0) && abs(dxTop) > 400 && abs(dxBot) > 400 &&
                    abs(dyLeft) > 400 && abs(dyRight) > 400;

        float xl = (rx[0] + rx[2]) / 2.0f, xr = (rx[1] + rx[3]) / 2.0f;
        float yt = (ry[0] + ry[1]) / 2.0f, yb = (ry[2] + ry[3]) / 2.0f;
        float kx = (xr - xl) / (float)(W - 2 * m); // raw units per pixel
        float ky = (yb - yt) / (float)(H - 2 * m);
        int xmin = lroundf(xl - kx * m), xmax = lroundf(xr + kx * m);
        int ymin = lroundf(yt - ky * m), ymax = lroundf(yb + ky * m);

        tft.fillScreen(TFT_BLACK);
        if (sane && accretionCalValid(xmin, xmax, ymin, ymax)) {
            touch.setCalibration(xmin, xmax, ymin, ymax, swap);
            accretionSaveTouchCal(xmin, xmax, ymin, ymax, swap);
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawCentreString("Kalibrasi selesai", W / 2, H / 2 - 8, 2);
            done = true;
        } else {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawCentreString("Gagal, ulangi...", W / 2, H / 2 - 8, 2);
        }
        delay(900);
    }

    // On failure/timeout the built-in defaults stay active and nothing is saved,
    // so the calibration is offered again on the next boot.
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(oldRotation);
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // SD card via SDIO (SD_MMC wrapper in lib/HAL)
    SD.setPins(ACCRETION_SD_CLK, ACCRETION_SD_CMD, ACCRETION_SD_D0);

    // Vibration motor: keep it off (Bruce has no driver for it)
    pinMode(ACCRETION_VIB_PIN, OUTPUT);
    digitalWrite(ACCRETION_VIB_PIN, LOW);

    // Touch chip
    pinMode(XPT2046_SPI_CONFIG_CS_GPIO_NUM, OUTPUT);
    digitalWrite(XPT2046_SPI_CONFIG_CS_GPIO_NUM, HIGH);
    if (!touch.begin()) { Serial.println("Touchscreen initialization failed!"); }

    // Panel is a standard (non-inverted) ILI9341
    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup (after the TFT is initialised)
***************************************************************************************/
void _post_setup_gpio() {
    // Brightness control must be initialised after the TFT
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, ACCRETION_BL_FREQ, ACCRETION_BL_BITS);
    ledcWrite(TFT_BL, 255);

#if defined(ACCRETION_TFT_DMA) && defined(ESP32_DMA)
    // Enables the SPI-DMA engine used by tft_sprite::pushSprite() (lib/HAL/display/tftespi.cpp)
    // for full-screen, opaque, 16bpp sprite pushes - i.e. the Home Screen, Lock Screen,
    // Control Center, games, etc. Everything that still needs the old blocking path
    // (8bpp/4bpp sprites, transparent-colour pushes) keeps working exactly as before.
    // ctrl_cs=false: CS stays under the library's normal software control, same as
    // every non-DMA transfer on this board.
    bool dmaOk = tft.native()->initDMA(false);
    Serial.printf("[DMA] accretion-phone TFT DMA init: %s\n", dmaOk ? "ok" : "failed (falling back to blocking pushSprite)");
#endif

    // Touch calibration: first boot (no saved data) or finger held on the screen while booting
    if (accretionTouchHeldAtBoot() || !accretionLoadTouchCal()) accretionCalibrateTouch();
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value (0-100)
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);

    ledcWrite(TFT_BL, dutyCycle);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        // nested on purpose: don't poll the touch chip more often than needed
        if (touch.touched()) {
            auto t = touch.getPointScaled();

            // Map raw (landscape) coordinates to the current TFT rotation
            if (bruceConfigPins.rotation == 3) {
                t.y = (tftHeight + 20) - t.y;
                t.x = tftWidth - t.x;
            }
            if (bruceConfigPins.rotation == 0) {
                int tmp = t.x;
                t.x = tftWidth - t.y;
                t.y = tmp;
            }
            if (bruceConfigPins.rotation == 2) {
                int tmp = t.x;
                t.x = t.y;
                t.y = (tftHeight + 20) - tmp;
            }
            // rotation == 1 (landscape, default): no transform

#if ACCRETION_TOUCH_INVERT_X
            t.x = tftWidth - t.x;
#endif
#if ACCRETION_TOUCH_INVERT_Y
            t.y = (tftHeight + 20) - t.y;
#endif

            if (!wakeUpScreen()) AnyKeyPress = true;
            else goto END;

            // Touch point global variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        END:
            d_tmp = millis();
        }
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (deep sleep, wakes on the BOOT button)
**********************************************************************/
void powerOff() {
    ledcWrite(TFT_BL, 0);
    tft.writecommand(0x10); // ILI9341 SLPIN
    digitalWrite(ACCRETION_VIB_PIN, LOW);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
