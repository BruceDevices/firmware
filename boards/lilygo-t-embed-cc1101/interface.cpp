#include "core/powerSave.h"
#include <bq27220.h>
#include <globals.h>
#include <interface.h>

// Rotary encoder
#include <RotaryEncoder.h>
extern RotaryEncoder *encoder;
RotaryEncoder *encoder = nullptr;
IRAM_ATTR void checkPosition() { encoder->tick(); }

/*********************************************************************
**  Screensaver board hooks — T-Embed CC1101
**
**  Enter: detach encoder IRQs so they don't accumulate ticks or
**         trigger EncoderLedChange during light sleep.
**  Exit:  reset encoder position, clear LED state, re-attach IRQs.
**********************************************************************/
void _screensaver_board_enter() {
    detachInterrupt(digitalPinToInterrupt(ENCODER_INA));
    detachInterrupt(digitalPinToInterrupt(ENCODER_INB));
}

void _screensaver_board_exit() {
    // Reset encoder position so accumulated ticks don't fire on resume
    if (encoder) encoder->setPosition(0);
#ifdef HAS_ENCODER_LED
    EncoderLedChange = 0;
#endif
    // Re-attach encoder interrupts
    attachInterrupt(digitalPinToInterrupt(ENCODER_INA), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INB), checkPosition, CHANGE);
}

// Battery libs
#if defined(T_EMBED_1101)
// Power handler for battery detection
#include <Wire.h>
// Charger chip
#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include <esp32-hal-dac.h>
XPowersPPM PPM;
#elif defined(T_EMBED)

#endif

#ifdef USE_BQ27220_VIA_I2C
#define BATTERY_DESIGN_CAPACITY 1300
#include <bq27220.h>
BQ27220 bq;
#endif

#include "core/i2c_finder.h"
#include "modules/rf/rf_utils.h"
#include <Adafruit_PN532.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(SEL_BTN, INPUT);
#ifdef T_EMBED_1101
    // T-Embed CC1101 has a antenna circuit optimized to each frequency band, controlled by SW0 and SW1
    // Set antenna frequency settings
    pinMode(CC1101_SW1_PIN, OUTPUT);
    pinMode(CC1101_SW0_PIN, OUTPUT);

    // Chip Select CC1101, SD and TFT to HIGH State to fix SD initialization
    pinMode(CC1101_SS_PIN, OUTPUT);
    digitalWrite(CC1101_SS_PIN, HIGH);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);
    pinMode(NRF24_SS_PIN, OUTPUT); // NRF24 on Plus
    digitalWrite(NRF24_SS_PIN, HIGH);

    pinMode(NRF24_CE_PIN, OUTPUT); // put nRF24 in standby
    digitalWrite(NRF24_CE_PIN, LOW);

    // Power chip pin
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH); // Power on CC1101 and LED
    bool pmu_ret = false;
    Wire.begin(GROVE_SDA, GROVE_SCL);
    pmu_ret = PPM.init(Wire, GROVE_SDA, GROVE_SCL, BQ25896_SLAVE_ADDRESS);
    if (pmu_ret) {
        // https://github.com/Xinyuan-LilyGO/T-Embed-CC1101/blob/3e6df69af51befdbd5c96761aca28b9a784413eb/examples/factory_test/factory_test.ino#L399-L425

        PPM.resetDefault();
        PPM.setChargeTargetVoltage(4208);
        PPM.enableMeasure(PowersBQ25896::CONTINUOUS);
    }
    if (bq.getDesignCap() != BATTERY_DESIGN_CAPACITY) { bq.setDesignCap(BATTERY_DESIGN_CAPACITY); }
    // Start with default IR, RF and RFID Configs, replace old
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
    bruceConfigPins.irRx = 1;
    bruceConfigPins.irTx = 2;
#else
    Wire.begin(GROVE_SDA, GROVE_SCL);
    Wire.beginTransmission(0x40);
    if (Wire.endTransmission() == 0) {
        Serial.println("ES7210 Online, No CC1101 version");
        Wire.end();
    } else {
        Serial.println("Probably CC1101 exists");
        bruceConfigPins.CC1101_bus.cs = GPIO_NUM_17;
        bruceConfigPins.CC1101_bus.io0 = GPIO_NUM_18;
        bruceConfigPins.rfModule = CC1101_SPI_MODULE;

        //* If it does not exist, then the CC1101 shield may exist, so there is no need for Wire to exist.
        Wire.endTransmission();
        Wire.end();
    }
    bruceConfigPins.rfidModule = PN532_SPI_MODULE;

#endif

#ifdef T_EMBED_1101
    pinMode(BK_BTN, INPUT);
#endif
    pinMode(ENCODER_KEY, INPUT);
    encoder = new RotaryEncoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INA), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INB), checkPosition, CHANGE);
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
#if defined(USE_BQ27220_VIA_I2C)
int getBattery() {
    int percent = 0;
    percent = bq.getChargePcnt();
    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
}
#endif
/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else if (brightval > 99) {
        analogWrite(TFT_BL, 254);
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
    static unsigned long tm = millis();  // debounce for buttons
    static unsigned long tm2 = millis(); // delay between Select and encoder (avoid missclick)
    static int posDifference = 0;
    static int lastPos = 0;
    bool sel = !BTN_ACT;
    bool esc = !BTN_ACT;

    int newPos = encoder->getPosition();
    if (newPos != lastPos) {
        posDifference += (newPos - lastPos);
        lastPos = newPos;
    }

    if (millis() - tm > 200 || LongPress) {
        sel = digitalRead(SEL_BTN);
#ifdef T_EMBED_1101
        esc = digitalRead(BK_BTN);
#endif
    }
    if (posDifference != 0 || sel == BTN_ACT || esc == BTN_ACT) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (posDifference > 0) {
        PrevPress = true;
        posDifference--;
#ifdef HAS_ENCODER_LED
        EncoderLedChange = -1;
#endif
        tm2 = millis();
    }
    if (posDifference < 0) {
        NextPress = true;
        posDifference++;
#ifdef HAS_ENCODER_LED
        EncoderLedChange = 1;
#endif
        tm2 = millis();
    }

    if (sel == BTN_ACT && millis() - tm2 > 200) {
        posDifference = 0;
        SelPress = true;
        tm = millis();
    }
    if (esc == BTN_ACT) {
        AnyKeyPress = true;
        EscPress = true;
        // Serial.println("EscPressed");
        tm = millis();
    }
}

void powerOff() {
#ifdef T_EMBED_1101
    PPM.shutdown();
#endif
}

void powerDownNFC() {
    Adafruit_PN532 nfc = Adafruit_PN532(17, 45);
    bool i2c_check = check_i2c_address(PN532_I2C_ADDRESS);
    nfc.setInterface(GROVE_SDA, GROVE_SCL);
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (i2c_check || versiondata) {
        nfc.powerDown();
    } else {
        Serial.println("Can't powerDown PN532");
    }
}

void powerDownCC1101() {
    if (!initRfModule("rx", bruceConfigPins.rfFreq)) { Serial.println("Can't init CC1101"); }

    ELECHOUSE_cc1101.goSleep();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to restart the device (ESP.restart) or deep sleep
**********************************************************************/
void checkReboot() {
#ifdef T_EMBED_1101
    // Early exit if button not pressed
    if (digitalRead(BK_BTN) != BTN_ACT) return;

    // Timing thresholds
    const uint32_t SCREENSAVER_THRESHOLD = 600;  // 0.6s → screensaver
    const uint32_t SLEEP_THRESHOLD       = 3000; // 3s   → deep sleep
    const uint32_t RESTART_THRESHOLD     = 5000; // 5s   → restart
    const uint32_t DISPLAY_DELAY         = 200;  // delay before showing hint

    // Banner geometry (sized for the longest string)
    const char *LONGEST_TEXT = "DEEP SLEEP IN 3/3";
    const int maxTextWidth   = tft.textWidth(LONGEST_TEXT, 1);
    const int bannerX        = tftWidth / 2 - maxTextWidth / 2 - 5;
    const int bannerY        = 12;
    const int bannerW        = maxTextWidth + 10;
    const int bannerH        = tft.fontHeight(1);

    auto clearBanner   = [&]() { tft.fillRect(bannerX, 7, bannerW, 18, bruceConfig.bgColor); };
    auto clearTextLine = [&]() { tft.fillRect(bannerX, bannerY, bannerW, bannerH, bruceConfig.bgColor); };

    uint32_t pressStart    = millis();
    bool bannerShown       = false;
    bool screensaverHinted = false;
    bool isRestartMode     = false;
    bool previousMode      = false;
    int  countDown         = 0;

    // ── Phase 1: wait for release or threshold ────────────────────────────
    while (digitalRead(BK_BTN) == BTN_ACT) {
        uint32_t held = millis() - pressStart;
        isRestartMode = (digitalRead(SEL_BTN) == BTN_ACT);

        // Mode switched → reset timer and display
        if (isRestartMode != previousMode && bannerShown) {
            clearBanner();
            pressStart    = millis();
            held          = 0;
            countDown     = 0;
            bannerShown   = false;
            screensaverHinted = false;
            previousMode  = isRestartMode;
            delay(50);
            continue;
        }
        previousMode = isRestartMode;

        // ── 500ms – 1000ms: show screensaver hint (non-restart mode only) ──
        if (!isRestartMode && held >= DISPLAY_DELAY && held < SCREENSAVER_THRESHOLD && !screensaverHinted) {
            screensaverHinted = true;
            bannerShown = true;
            tft.setTextSize(1);
            clearBanner();
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawCentreString("SCREENSAVER", tftWidth / 2, bannerY, 1);
        }

        // ── 1000ms+: switch to sleep/restart countdown ────────────────────
        if (held >= SCREENSAVER_THRESHOLD) {
            if (!bannerShown || screensaverHinted) {
                // Clear whatever was shown before and start countdown
                clearBanner();
                bannerShown   = true;
                screensaverHinted = false;
                countDown     = 0;
            }

            int newCountDown = (held - SCREENSAVER_THRESHOLD) / 1000 + 1;
            if (newCountDown != countDown) {
                countDown = newCountDown;
                tft.setTextSize(1);
                clearTextLine();

                if (isRestartMode) {
                    int remaining = (RESTART_THRESHOLD - SCREENSAVER_THRESHOLD) / 1000;
                    if (countDown > remaining) {
                        tft.fillScreen(bruceConfig.bgColor);
                        tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
                        tft.drawCentreString("RESTARTING...", tftWidth / 2, tftHeight / 2, 2);
                        delay(1000);
                        ESP.restart();
                    }
                    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
                    tft.drawCentreString(
                        "RESTART IN " + String(countDown) + "/" + String(remaining),
                        tftWidth / 2, bannerY, 1
                    );
                } else {
                    int remaining = (SLEEP_THRESHOLD - SCREENSAVER_THRESHOLD) / 1000;
                    if (countDown > remaining) {
                        tft.fillScreen(bruceConfig.bgColor);
                        while (digitalRead(BK_BTN) == BTN_ACT);
                        delay(200);
                        powerDownNFC();
                        powerDownCC1101();
                        tft.sleep(true);
                        delay(1000);
                        digitalWrite(PIN_POWER_ON, LOW);
                        esp_sleep_enable_ext0_wakeup(GPIO_NUM_6, LOW);
                        esp_deep_sleep_start();
                    }
                    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                    tft.drawCentreString(
                        "DEEP SLEEP IN " + String(countDown) + "/" + String(remaining),
                        tftWidth / 2, bannerY, 1
                    );
                }
            }
        }

        delay(10);
    }

    // ── Button released ───────────────────────────────────────────────────
    uint32_t held = millis() - pressStart;
    delay(30);

    if (bannerShown) clearBanner();

    // Released in screensaver window (500ms – 1000ms, non-restart mode) → screensaver
    if (!isRestartMode && held >= DISPLAY_DELAY && held < SCREENSAVER_THRESHOLD) {
        enterScreensaver();
        return; // status bar will be redrawn by loopOptions after screensaver
    }

    // If we released after the display delay without going into deep sleep → redraw
    if (held > DISPLAY_DELAY) drawStatusBar();
#endif
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
#ifdef USE_BQ27220_VIA_I2C
bool isCharging() {
    return bq.getIsCharging(); // Return the charging status from BQ27220
}
#else
bool isCharging() { return false; }
#endif
