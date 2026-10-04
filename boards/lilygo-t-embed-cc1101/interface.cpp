#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <bq27220.h>
#include <globals.h>
#include <interface.h>

// Rotary encoder
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/encoder.h"

// Encoder pins differ between the two envs built from this directory; GAUGE_BQ27220 is
// the real discriminator (T_EMBED_1101/T_EMBED are never defined by any -D, see _setup_gpio).
#ifdef GAUGE_BQ27220 // lilygo-t-embed-cc1101 env
#define ENCODER_INA 4
#define ENCODER_INB 5
#else // lilygo-t-embed env
#define ENCODER_INA 2
#define ENCODER_INB 1
#endif
#define ENCODER_KEY 0

#define SEL_BTN ENCODER_KEY

#define BK_BTN 6
#define BTN_ACT LOW
static DeviceEncoder encoderCfg() {
    DeviceEncoder cfg;
    cfg.pin_a = ENCODER_INA;
    cfg.pin_b = ENCODER_INB;
    cfg.pin_sel = SEL_BTN;
#ifdef T_EMBED_1101
    cfg.pin_esc = BK_BTN;
#endif
    return cfg;
}

// Battery libs
#if defined(T_EMBED_1101)
// Power handler for battery detection
#include <Wire.h>
#include <esp32-hal-dac.h>
#elif defined(T_EMBED)

#endif

#ifdef GAUGE_BQ27220
#define BATTERY_DESIGN_CAPACITY 1300
#endif

#include "core/i2c_finder.h"
#include "modules/rf/rf_utils.h"
#include <Adafruit_PN532.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
#ifdef GAUGE_BQ27220
    // lilygo-t-embed-cc1101 env (real discriminator: T_EMBED_1101/T_EMBED are never actually
    // defined by any -D flag in this codebase -- pre-existing dead macros, not touched here --
    // so GAUGE_BQ27220, which IS exclusive to this env's .ini, is used instead)
    bruceConfigPins.i2c_bus = {(gpio_num_t)8, (gpio_num_t)18};    // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)8, (gpio_num_t)18};    // sda, scl
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};  // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};   // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)18, (gpio_num_t)8}; // rx, tx (CH9329; BAD_RX/BAD_TX
                                                                  // fell back to GROVE_SCL/GROVE_SDA)
    bruceConfigPins.irTx = 2;
    bruceConfigPins.irRx = 1;
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, GPIO_NUM_NC};
    // No dedicated PN532 SPI bus on this env (NFC is PN532_I2C_MODULE) -- RC522-SPI shares the
    // generic SPI bus like on boards without a dedicated NFC slot.
    bruceConfigPins.PN532_bus = {(gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, GPIO_NUM_NC};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)12, (gpio_num_t)3, (gpio_num_t)38
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)44, (gpio_num_t)43
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)13
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)44, (gpio_num_t)43, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst (no W5500_RST_PIN on this env -> falls back to -1)
#endif
    bruceConfigPins.speaker_bus = {
        (gpio_num_t)46, (gpio_num_t)40, (gpio_num_t)7, (gpio_num_t)39
    }; // bclk,ws,dout,mclk
    bruceConfigPins.mic_bus = {(gpio_num_t)39, (gpio_num_t)42, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data
#else
    // lilygo-t-embed env (non-CC1101)
    bruceConfigPins.i2c_bus = {(gpio_num_t)44, (gpio_num_t)43};    // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {GPIO_NUM_NC, GPIO_NUM_NC};          // not defined on this variant
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)43, (gpio_num_t)44}; // rx, tx (BAD_RX/BAD_TX fell
                                                                   // back to GROVE_SCL/GROVE_SDA)
    bruceConfigPins.irTx = 44;
    bruceConfigPins.irRx = 43;
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)16};
    // No dedicated PN532 SPI bus defined on this env -- RC522-SPI shares the generic SPI bus.
    bruceConfigPins.PN532_bus = {(gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)16};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)43, (gpio_num_t)44, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2 (no dedicated gdo2 pin on this env)
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)43, (gpio_num_t)44
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)39
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)43, (gpio_num_t)44, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst (no W5500_RST_PIN on this env -> falls back to -1)
#endif
    bruceConfigPins.speaker_bus = {
        (gpio_num_t)7, (gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)39
    }; // bclk,ws,dout,mclk
    bruceConfigPins.mic_bus = {(gpio_num_t)21, (gpio_num_t)14, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data
#endif
    bruceConfigPins.rotation = 3;

    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(SEL_BTN, INPUT);
#ifdef T_EMBED_1101
    // T-Embed CC1101 has a antenna circuit optimized to each frequency band, controlled by SW0 and SW1
    // Set antenna frequency settings
    pinMode(CC1101_SW1_PIN, OUTPUT);
    pinMode(CC1101_SW0_PIN, OUTPUT);

    // Chip Select CC1101, SD and TFT to HIGH State to fix SD initialization
    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT); // NRF24 on Plus
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);

    pinMode(bruceConfigPins.NRF24_bus.io0, OUTPUT); // put nRF24 in standby (io0 slot holds CE)
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);

    // Power chip pin
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH); // Power on CC1101 and LED
    setSysI2CBus(&Wire); // PMIC and gauge live on the default Wire object (GROVE == sys_i2c on this variant)
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
    DevicePmic pmicCfg;
    pmicCfg.pin_sda = bruceConfigPins.sys_i2c.sda;
    pmicCfg.pin_scl = bruceConfigPins.sys_i2c.scl;
    pmicCfg.address = 0x6B; // BQ25896
    hal_pmic_init(pmicCfg);
    DeviceGauge gaugeCfg;
    gaugeCfg.design_capacity_mah = BATTERY_DESIGN_CAPACITY;
    hal_gauge_init(gaugeCfg);
    // Start with default IR, RF and RFID Configs, replace old
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
    bruceConfigPins.irRx = 1;
    bruceConfigPins.irTx = 2;
#else
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
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

    hal_encoder_init(encoderCfg());

    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) { hal_encoder_poll(encoderCfg()); }

void powerOff() {
#ifdef T_EMBED_1101
    hal_pmic_shutdown();
#endif
}

void powerDownNFC() {
    Adafruit_PN532 nfc = Adafruit_PN532(17, 45);
    bool i2c_check = check_i2c_address(PN532_I2C_ADDRESS);
    nfc.setInterface(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
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

    // Constants for better readability
    const int SLEEP_TIMEOUT = 3;
    const int RESTART_TIMEOUT = 5;
    const int DISPLAY_DELAY = 500;
    const char *SLEEP_TEXT = "DEEP SLEEP IN 3/3";
    const char *RESTART_TEXT = "RESTART IN 5/5";

    // Calculate banner dimensions once
    const int maxTextWidth = tft.textWidth(SLEEP_TEXT, 1);
    const int bannerX = tftWidth / 2 - maxTextWidth / 2 - 5;
    const int bannerY = 12;
    const int bannerWidth = maxTextWidth + 10;
    const int bannerHeight = tft.fontHeight(1);

    // Helper function to clear banner area
    auto clearBanner = [&]() { tft.fillRect(bannerX, 7, bannerWidth, 18, bruceConfig.bgColor); };

    // Helper function to clear text line only
    auto clearTextLine = [&]() {
        tft.fillRect(bannerX, bannerY, bannerWidth, bannerHeight, bruceConfig.bgColor);
    };

    uint32_t time_count = millis();
    bool isRestartMode = false;
    bool previousMode = false;
    int countDown = 0;
    bool bannerInitialized = false;

    while (digitalRead(BK_BTN) == BTN_ACT) {
        // Check if SEL_BTN is pressed for restart mode
        isRestartMode = (digitalRead(SEL_BTN) == BTN_ACT);

        // Handle mode change: reset timer and clear display
        if (isRestartMode != previousMode && bannerInitialized) {
            clearBanner();
            time_count = millis();
            countDown = 0;
            bannerInitialized = false;
            previousMode = isRestartMode;
            delay(50);
            continue;
        }

        previousMode = isRestartMode;

        // Only show countdown after initial delay
        if (millis() - time_count <= DISPLAY_DELAY) {
            delay(10);
            continue;
        }

        // Initialize banner on first display
        if (!bannerInitialized) {
            clearBanner();
            bannerInitialized = true;
        }

        // Calculate current countdown value
        int newCountDown = (millis() - time_count - DISPLAY_DELAY) / 1000 + 1;

        // Only update display if countdown changed OR mode just initialized
        if (newCountDown != countDown || !bannerInitialized) {
            countDown = newCountDown;

            // Initialize banner on first display
            if (!bannerInitialized) {
                clearBanner();
                bannerInitialized = true;
            }

            tft.setTextSize(1);

            if (isRestartMode) {
                // RESTART MODE: 5 seconds
                if (countDown >= RESTART_TIMEOUT + 1) {
                    // Execute restart
                    tft.fillScreen(bruceConfig.bgColor);
                    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
                    tft.drawCentreString("RESTARTING...", tftWidth / 2, tftHeight / 2, 2);
                    delay(1000);
                    ESP.restart();
                }

                // Display countdown
                tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
                clearTextLine();
                tft.drawCentreString(
                    "RESTART IN " + String(countDown) + "/" + String(RESTART_TIMEOUT),
                    tftWidth / 2,
                    bannerY,
                    1
                );

            } else {
                // DEEP SLEEP MODE: Normal text, 3 seconds
                if (countDown >= SLEEP_TIMEOUT + 1) {
                    // Execute deep sleep
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(BK_BTN) == BTN_ACT);
                    delay(200);
                    powerDownNFC();
                    powerDownCC1101();
                    tft.sleep(true);
                    delay(1000); // Delay for debouncing ;)
                    digitalWrite(PIN_POWER_ON, LOW);
                    esp_sleep_enable_ext0_wakeup(GPIO_NUM_6, LOW);
                    esp_deep_sleep_start();
                }

                // Display countdown
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                clearTextLine();
                tft.drawCentreString(
                    "DEEP SLEEP IN " + String(countDown) + "/" + String(SLEEP_TIMEOUT),
                    tftWidth / 2,
                    bannerY,
                    1
                );
            }
        }

        delay(10);
    }

    // Clear banner after button release
    delay(30);
    if (millis() - time_count > DISPLAY_DELAY) {
        clearBanner();
        drawStatusBar();
    }
#endif
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
#ifdef GAUGE_BQ27220
bool isCharging() {
    return hal_gauge_is_charging(); // Return the charging status from BQ27220
}
#else
bool isCharging() { return false; }
#endif
