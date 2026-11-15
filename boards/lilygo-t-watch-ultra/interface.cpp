#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "hal/inputs/touch.h"
#include <Wire.h>
#include <XPowersLib.h>
#include <globals.h>
#include <interface.h>

// GPIO expander (XL9555, register-compatible with the PCA9555)
#include <IoExpanderXL9555.hpp>

// Haptic motor driver (DRV2605 on the system I2C bus)
#include "HapticDrivers.hpp"

XPowersAXP2101 PPM;
IoExpanderXL9555 xio;
HapticDriver_DRV2605 drv;

// GPIOs (https://wiki.lilygo.cc/products/t-watch-series/t-watch-ultra/)
#define BOOT_BTN 0
#define NFC_CS 4
#define NFC_INT 5
#define TP_INT 12
#define GNSS_RX 43 // MIA-M10Q TX output -> ESP32 UART RX
#define GNSS_TX 44 // MIA-M10Q RX input  <- ESP32 UART TX

// XL9555 IO expander ports
#define EXPANDS_DRV_EN 6     // DRV2605 haptic enable
#define EXPANDS_DISP_EN 7    // display power enable
#define EXPANDS_TOUCH_RST 8  // CST9217 touch reset
#define EXPANDS_SD_DET 10    // microSD card detect
#define EXPANDS_LORA_RF_SW 11// antenna switch: HIGH = built-in LoRa antenna, LOW = USB

// AXP2101 rails (LilyGo factory assignment):
// ALDO1 SD card / ALDO2 display / ALDO3 radio (LoRa) / ALDO4 sensor (1.8V)
// BLDO1 GNSS / BLDO2 speaker amp / DLDO1 NFC

static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // Reset runs through the XL9555 (EXPANDS_TOUCH_RST), pulsed in
    // _post_setup_gpio() right before hal_touch_init(), so pin_rst stays -1.
    cfg.pin_rst = -1;
    cfg.pin_irq = TP_INT;
    cfg.i2c_bus = &Wire;
    // Known CST92xx-family chip (CST9217) at 0x1A -- skip the shared CST8XX
    // driver's auto-probe (CST226/CST8XX first), which would pulse RST and
    // poke foreign register maps before reaching the right driver.
    cfg.cst8xx_model = 2; // TouchDrv_CST92XX
    // Derived from the manual per-rotation math against displayConfig
    // (rot0: swap only; rot1: mirrorY; rot2: swap+mirrorX+mirrorY; rot3:
    // identity) -- same table the upstream port ships for this panel.
    // rotation:        0     1      2     3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {false, false, true, false};
    const bool mirrorY[4] = {false, true, true, false};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

static DeviceButtons buttonsCfg() { return DeviceButtons{BOOT_BTN}; }

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // bclk, ws, dout, mclk (MAX98357A)
    bruceConfigPins.speaker_bus = {(gpio_num_t)9, (gpio_num_t)10, (gpio_num_t)11, GPIO_NUM_NC};
    // clk, data, ws, type (T3902 PDM microphone)
    bruceConfigPins.mic_bus = {(gpio_num_t)17, (gpio_num_t)18, GPIO_NUM_NC, MIC_TYPE_PDM};
    bruceConfigPins.i2c_bus = {(gpio_num_t)3, (gpio_num_t)2};   // sda, scl
    bruceConfigPins.sys_i2c = {(gpio_num_t)3, (gpio_num_t)2};   // sda, scl
    // No IR LED, no one-pin RF header, no Grove on this board
    bruceConfigPins.rfTx = -1;
    bruceConfigPins.rfRx = -1;
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = -1;
    // The only external header wires GPIO43/44, shared with the GNSS UART
    bruceConfigPins.uart_bus = {(gpio_num_t)GNSS_RX, (gpio_num_t)GNSS_TX}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)GNSS_RX, (gpio_num_t)GNSS_TX};  // rx, tx
    // BadUSB goes over the native USB-OTG port
    // Board's default/generic SPI bus (sck, miso, mosi, cs): SD card, NFC and
    // LoRa all hang off it
    bruceConfigPins.outer_bus = {(gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)21};
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)21};
    bruceConfigPins.ST25R_bus = {
        (gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)NFC_CS, (gpio_num_t)NFC_INT,
        GPIO_NUM_NC
    }; // sck, miso, mosi, cs, irq, -
    // No CC1101, NRF24, PN532/RC522 or W5500 on this board
    bruceConfigPins.CC1101_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfigPins.NRF24_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfigPins.PN532_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)36, (gpio_num_t)47, (gpio_num_t)14
    }; // sck, miso, mosi, cs, rst(io0), dio1(io2)
#endif

    // No CC1101 / NRF24 hardware: hide those menus by default
    bruceConfig.disabledMenus.push_back("RF");
    bruceConfig.disabledMenus.push_back("NRF24");

    // Park every SPI chip-select HIGH so the panel/SD/NFC/LoRa never see
    // stray clocks during bus setup
    const uint8_t csPins[] = {TFT_CS, NFC_CS, (uint8_t)bruceConfigPins.SDCARD_bus.cs
#if !defined(LITE_VERSION)
        ,
        (uint8_t)bruceConfigPins.LoRa_bus.cs
#endif
    };
    for (auto pin : csPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }
    pinMode(bruceConfigPins.ST25R_bus.io0, INPUT); // ST25R3916 interrupt

    setSysI2CBus(&Wire); // PMU, RTC, touch, haptic and the IO expander
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);

    // Power management (AXP2101 at 0x34)
    if (PPM.init(Wire, bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl, AXP2101_SLAVE_ADDRESS)) {
        PPM.setSysPowerDownVoltage(3300);
        PPM.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
        // Battery is 1100 mAh; LilyGO recommends charging below half capacity
        PPM.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
        PPM.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
        PPM.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
        // Rails, at the LilyGO factory voltages
        PPM.setALDO1Voltage(3300);  // SD card
        PPM.enableALDO1();
        PPM.setALDO2Voltage(3300);  // Display
        PPM.enableALDO2();
        PPM.setALDO3Voltage(3300);  // Radio (LoRa)
        PPM.enableALDO3();
        PPM.setALDO4Voltage(1800);  // Sensor (BHI260AP)
        PPM.enableALDO4();
        PPM.setBLDO1Voltage(3300);  // GNSS
        PPM.enableBLDO1();
        PPM.setBLDO2Voltage(3300);  // Speaker amplifier
        PPM.enableBLDO2();
        PPM.setDLDO1Voltage(3300);  // NFC (ST25R3916)
        PPM.enableDLDO1();
        PPM.disableDC2();
        PPM.disableDC3();
        PPM.disableDC4();
        PPM.disableDC5();
        PPM.disableCPUSLDO();
        PPM.disableDLDO2();
        // Power button timing: hold 1 s to power on, 6 s to power off
        PPM.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
        PPM.setPowerKeyPressOffTime(XPOWERS_POWEROFF_6S);
        // No battery temperature sensing on this board
        PPM.disableTSPinMeasure();
        PPM.enableBattDetection();
        PPM.enableVbusVoltageMeasure();
        PPM.enableBattVoltageMeasure();
        PPM.enableSystemVoltageMeasure();
        PPM.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        PPM.clearIrqStatus();
    } else {
        Serial.println("AXP2101 not found");
    }

    // IO expander (XL9555 at 0x20)
    bool expanderOK = xio.begin(Wire, 0x20);
    if (!expanderOK) {
        delay(30);
        expanderOK = xio.begin(Wire, 0x20);
    }
    if (expanderOK) {
        const uint8_t expands[] = {EXPANDS_DRV_EN, EXPANDS_DISP_EN, EXPANDS_TOUCH_RST};
        for (auto pin : expands) {
            xio.pinMode(pin, OUTPUT);
            xio.digitalWrite(pin, HIGH);
            delay(1);
        }
        xio.pinMode(EXPANDS_SD_DET, INPUT);
        xio.pinMode(EXPANDS_LORA_RF_SW, OUTPUT);
        xio.digitalWrite(EXPANDS_LORA_RF_SW, HIGH); // use the built-in LoRa antenna
    } else {
        Serial.println("Initializing expander failed");
    }

    // Haptic driver (DRV2605 at 0x5A)
    if (!drv.begin(Wire, bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl)) {
        Serial.println("Failed to find DRV2605.");
    } else {
        drv.selectLibrary(1);
        drv.setMode(HapticMode::INTERNAL_TRIGGER);
    }

    // Onboard NFC is the ST25R3916; onboard radio is an SX1262
    bruceConfigPins.rfidModule = ST25R3916_SPI_MODULE;
    bruceConfigPins.gpsBaudrate = 38400; // MIA-M10Q default

    hal_buttons_init(buttonsCfg(), 1);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // Touch reset lives on the IO expander: pulse it right before the
    // controller probe (hal_touch_init leaves pin_rst alone at -1)
    xio.digitalWrite(EXPANDS_TOUCH_RST, LOW);
    delay(20);
    xio.digitalWrite(EXPANDS_TOUCH_RST, HIGH);
    delay(60);
    if (!hal_touch_init(touchCfg(), 0x1A)) { Serial.println("Touch IC not Started"); }
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = PPM.getBatteryPercent();
    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
** Function: setBrightness
** The AMOLED has no backlight rail; brightness is a CO5300 register.
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    auto *panel = static_cast<Arduino_CO5300 *>(tft.outputDriver());
    if (panel) panel->setBrightness(brightval * 254 / 100);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            tm = millis();
            if (!hal_touch_apply(t)) return;
            drv.setWaveform(0, 75);
            drv.setWaveform(1, 0); // end waveform
            drv.run();
        }
    }
    hal_buttons_poll_1(buttonsCfg());
}

/*********************************************************************
** Function: powerOff
** Turns the device off (PMU shutdown; BOOT wakes it back up)
**********************************************************************/
void powerOff() {
    const uint8_t expands[] = {EXPANDS_DRV_EN, EXPANDS_DISP_EN};
    for (auto pin : expands) {
        xio.digitalWrite(pin, LOW);
        delay(1);
    }
    PPM.shutdown();

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    delay(200);
    esp_deep_sleep_start();
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() { return PPM.isCharging(); }

/*********************************************************************
** Function: _setup_codec_speaker
** location: modules/others/audio.cpp
** No codec IC: the MAX98357A amp follows the I2S bus directly
**********************************************************************/
void _setup_codec_speaker(bool enable) {}

/*********************************************************************
** Function: _setup_codec_mic
** location: modules/others/mic.cpp
** No codec IC: the T3902 PDM mic follows the I2S peripheral directly
**********************************************************************/
void _setup_codec_mic(bool enable) {}
