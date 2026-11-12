/*
 * M5Stack StackChan (SKU K151) — CoreS3 head on the StackChan robot body.
 *
 * Head electronics are plain CoreS3 (ILI9342C + FT6336U on G11/G12, AXP2101, BM8563,
 * AW88298/ES7210 audio, GC0308 camera). The body adds (https://docs.m5stack.com/en/StackChan):
 *   - ST25R3916 NFC, I2C addr 0x50, on the sys I2C bus (G12 SDA / G11 SCL), no IRQ pin
 *   - IR transmitter G5, IR receiver (IRM56384) G10
 *   - Servos (G6/G7) and the 12x WS2812 ring (PY32L020 expander) are NOT driven by Bruce
 *   - Grove Port A (G2/G1), Port B (G9/G8), Port C (G17/G18) on the body
 */
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <M5Unified.h>
#include <interface.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>

static void setupCoreS3SharedSpiPins() {
    pinMode(TFT_CS, OUTPUT);
    pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);

    // CoreS3 shares the display D/C pin with SPI MISO. M5.begin() and the first
    // TFT_eSPI draw happen before storage is mounted, so release GPIO35 back to
    // MISO at the pre-storage hook. TFT_eSPI switches it to D/C only while TFT CS is active.
#if defined(USE_HSPI_PORT)
    pinMatrixInAttach(TFT_MISO, SPI3_Q_IN_IDX, false);
    *(volatile uint32_t *)GPIO_FUNC35_OUT_SEL_CFG_REG = SPI3_Q_OUT_IDX;
#else
    pinMatrixInAttach(TFT_MISO, FSPIQ_IN_IDX, false);
    *(volatile uint32_t *)GPIO_FUNC35_OUT_SEL_CFG_REG = FSPIQ_OUT_IDX;
#endif
    *(volatile uint32_t *)GPIO_ENABLE1_W1TC_REG = 1u << (TFT_MISO - 32);
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)2, (gpio_num_t)1};   // sda, scl (Grove Port A)
    bruceConfigPins.sys_i2c = {(gpio_num_t)12, (gpio_num_t)11}; // sda, scl
    bruceConfigPins.rfTx = 2;
    bruceConfigPins.rfRx = 1;
    bruceConfigPins.irTx = 5;  // body IR transmitter
    bruceConfigPins.irRx = 10; // body IR receiver (IRM56384)
    bruceConfigPins.rotation = 1;
    bruceConfigPins.badusb_bus = {(gpio_num_t)1, (gpio_num_t)2}; // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)36, (gpio_num_t)35, (gpio_num_t)37, (gpio_num_t)26};
    bruceConfigPins.PN532_bus = {(gpio_num_t)36, (gpio_num_t)35, (gpio_num_t)37, (gpio_num_t)26};
    // The body's NFC is the onboard reader, and the M5Stack NFC Unit (U216) on Grove
    // answers at the same 0x50 address — _initI2C() probes the Grove bus first, so both
    // work. The board preset pins only matter for the body NFC (sys I2C bus).
    bruceConfigPins.ST25R_bus = {(gpio_num_t)12, (gpio_num_t)11, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfigPins.rfidModule = ST25R3916_I2C_MODULE;
    // CC1101/NRF24/W5500 share the SD Card's SPI bus (sck, miso, mosi). No radio is
    // wired on the body; G6/G7 are the servo UART and G5/G10 the IR pair, so the chip
    // selects land on Grove Port B (G8/G9) — the only pins a module could reach.
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)36, (gpio_num_t)35, (gpio_num_t)37, (gpio_num_t)8, (gpio_num_t)9, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)36, (gpio_num_t)35, (gpio_num_t)37, (gpio_num_t)8, (gpio_num_t)9
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)36, (gpio_num_t)35, (gpio_num_t)37, (gpio_num_t)4
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)36, (gpio_num_t)35, (gpio_num_t)37, (gpio_num_t)13, (gpio_num_t)14, (gpio_num_t)7
    }; // sck,miso,mosi,cs,int,rst
#endif

    // CoreS3 audio: AW88298 speaker amp (BCK=G34, WCK=G33, DATO=G13), ES7210 mic codec
    // (MCLK=G0). M5Unified drives both; the pins are here for the raw-I2S playback path
    // and the Speaker Pins menu.
    bruceConfigPins.speaker_bus = {(gpio_num_t)34, (gpio_num_t)33, (gpio_num_t)13, (gpio_num_t)0};

    M5.begin();
    M5.Power.setUsbOutput(false);
    M5.Power.setExtOutput(true);
    setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire);
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
}

void _pre_storage_gpio() { setupCoreS3SharedSpiPins(); }

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent;
    percent = M5.Power.getBatteryLevel();
    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { M5.Display.setBrightness(brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;
    if (!trylockSysI2CBus()) return; // RFID driver mid-transaction - retry next tick
    M5.update();
    unlockSysI2CBus();
    auto t = M5.Touch.getDetail();
    if (t.isPressed() || t.isHolding()) {
        tm = millis();

        if (bruceConfigPins.rotation == 3) {
            t.y = (tftHeight + TOUCH_FOOTER_HEIGHT) - t.y;
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
            t.y = (tftHeight + TOUCH_FOOTER_HEIGHT) - tmp;
        }

        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;

        // Touch point global variable
        touchPoint.x = t.x;
        touchPoint.y = t.y;
        touchPoint.pressed = true;
        touchHeatMap(touchPoint);
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }
void goToDeepSleep() { M5.Power.deepSleep(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}

/*********************************************************************
** Function: _setup_codec_speaker
** location: modules/others/audio.cpp
** Brings the AW88298 amp up before the raw-I2S playback path writes samples.
** M5Unified owns the amp's I2C setup; never ends it here, _tone() keeps using it.
**********************************************************************/
void _setup_codec_speaker(bool enable) {
    if (enable) M5.Speaker.begin();
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() {
    if (M5.Power.getBatteryCurrent() > 0 || M5.Power.getBatteryCurrent()) return true;
    else return false;
}
