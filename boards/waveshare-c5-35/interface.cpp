/*
 * Waveshare ESP32-C5-Touch-LCD-3.5
 * Board interface implementation for Bruce firmware
 *
 * Hardware:
 *   - ESP32-C5-WROOM-1U (8MB PSRAM)
 *   - ST7796 320x480 TFT (SPI, shared with the microSD card)
 *   - FT6336 capacitive touch (I2C @ 0x38, INT GPIO3)
 *   - I/O expander @ 0x24 (a CH32V006 MCU): EXIO0 touch reset, EXIO1 LCD reset,
 *     EXIO5 panel power, its PWM register drives the backlight
 *   - ES8311 codec (I2C @ 0x18) -> NS4150B amp -> onboard speaker, onboard mic.
 *     No MCLK net: the codec clocks off BCLK
 *   - AXP2101 PMU (I2C @ 0x34), used read-only as a battery gauge
 *   - BOOT button (GPIO28), used as Back
 */

#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Arduino.h>
#include <ESP_I2S.h>
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>
#include <globals.h>
#include <interface.h>

// Board I2C bus: touch, expander, PMU, codec, RTC and IMU all share it
#define WS_I2C_SDA 27
#define WS_I2C_SCL 26
#define WS_I2C_FREQ 100000U

#define WS_TOUCH_INT 3
#define WS_TOUCH_ADDR 0x38
#define WS_BTN_PIN 28

// I2S to the ES8311
#define WS_I2S_BCLK 23
#define WS_I2S_WS 10
#define WS_I2S_DOUT 25
#define WS_I2S_DIN 24

// =============================================
// I/O expander @0x24
// =============================================
#define EXIO_ADDR 0x24
#define EXIO_MODE_REG 0x02
#define EXIO_OUTPUT_REG 0x03
#define EXIO_PWM_REG 0x05
#define EXIO_TP_RST 0
#define EXIO_LCD_RST 1
#define EXIO_POWER 5

static uint8_t exioOutput = 0;
static bool exioReady = false;

static bool exioWrite(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(EXIO_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static bool exioSet(uint8_t pin, bool level) {
    if (level) exioOutput |= (uint8_t)(1U << pin);
    else exioOutput &= (uint8_t)~(1U << pin);
    return exioWrite(EXIO_OUTPUT_REG, exioOutput);
}

// Must run exactly once, before tft.init(): pulsing the LCD reset again after the
// panel is initialized blanks the screen.
static bool exioBegin() {
    if (exioReady) return true;

    const uint8_t outputs = (1U << EXIO_TP_RST) | (1U << EXIO_LCD_RST) | (1U << EXIO_POWER);
    bool ok = exioWrite(EXIO_MODE_REG, 0xFF) && exioWrite(EXIO_OUTPUT_REG, 0x00) &&
              exioWrite(EXIO_PWM_REG, 0x00) && exioWrite(EXIO_MODE_REG, outputs);

    // Bring the rail up together with both resets released: resetting the touch
    // chip while its rail is off leaves the FT6336 unresponsive.
    exioOutput = (1U << EXIO_TP_RST) | (1U << EXIO_LCD_RST);
    ok = ok && exioWrite(EXIO_OUTPUT_REG, exioOutput);
    delay(50);
    exioOutput = 0x00;
    ok = ok && exioWrite(EXIO_OUTPUT_REG, exioOutput);
    delay(50);
    exioOutput = outputs;
    ok = ok && exioWrite(EXIO_OUTPUT_REG, exioOutput);
    delay(50);

    // Power is on: pulse each reset (active low) with the rail up
    ok = ok && exioSet(EXIO_LCD_RST, false);
    delay(20);
    ok = ok && exioSet(EXIO_LCD_RST, true);
    delay(120);
    ok = ok && exioSet(EXIO_TP_RST, false);
    delay(20);
    ok = ok && exioSet(EXIO_TP_RST, true);
    delay(300); // FT6336 boot
    ok = ok && exioWrite(EXIO_PWM_REG, 0xFF);

    exioReady = ok;
    Serial.printf("[WS-C5] I/O expander 0x24 %s\n", ok ? "ready" : "NOT FOUND");
    return ok;
}

// =============================================
// Touch Screen (FT6336, hal_touch_init/hal_touch_read -- src/hal/inputs/touch.cpp)
// =============================================
// Raw touch reads in the panel's native 320x480 portrait frame. The touch reset was
// already pulsed by exioBegin(), so the HAL gets neither pin_rst nor reset_cb.
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    cfg.pin_sda = WS_I2C_SDA;
    cfg.pin_scl = WS_I2C_SCL;
    cfg.pin_irq = WS_TOUCH_INT;
    // rotation:        0      1      2      3
    const bool swapXY[4] = {false, true, false, true};
    const bool mirrorX[4] = {false, false, true, true};
    const bool mirrorY[4] = {false, true, true, false};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

// =============================================
// AXP2101 PMU (battery gauge only)
// =============================================
// The PMU powers the whole board, so its rails are never reconfigured here
static XPowersAXP2101 pmu;
static bool pmuReady = false;

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // bclk,ws,dout,mclk (no MCLK net)
    bruceConfigPins.speaker_bus = {
        (gpio_num_t)WS_I2S_BCLK, (gpio_num_t)WS_I2S_WS, (gpio_num_t)WS_I2S_DOUT, GPIO_NUM_NC
    };
    // ES8311 ADC: clocked off the speaker BCLK, and its LRCLK doubles as the mic WS
    // clk,data,ws,type
    bruceConfigPins.mic_bus = {
        (gpio_num_t)WS_I2S_BCLK, (gpio_num_t)WS_I2S_DIN, (gpio_num_t)WS_I2S_WS, MIC_TYPE_I2S_MSB
    };
    bruceConfigPins.sys_i2c = {(gpio_num_t)WS_I2C_SDA, (gpio_num_t)WS_I2C_SCL}; // sda, scl
    bruceConfigPins.i2c_bus = {(gpio_num_t)WS_I2C_SDA, (gpio_num_t)WS_I2C_SCL}; // sda, scl (header J3-27/25)
    // IR / one-pin RF on the free header pins: GPIO1 (J3-17) TX, GPIO0 (J3-15) RX
    bruceConfigPins.irTx = 1;
    bruceConfigPins.irRx = 0;
    bruceConfigPins.rfTx = 1;
    bruceConfigPins.rfRx = 0;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)12, (gpio_num_t)11};   // rx, tx (J3-28/26)
    bruceConfigPins.gps_bus = {(gpio_num_t)12, (gpio_num_t)11};    // rx, tx
    bruceConfigPins.gpsBaudrate = 115200; // Waveshare's LC76G GPS add-on ships at 115200, not 9600
    bruceConfigPins.badusb_bus = {(gpio_num_t)12, (gpio_num_t)11}; // rx, tx (CH9329)
    // microSD shares the display SPI bus
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)7, (gpio_num_t)9}; // sck,miso,mosi,cs

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(9, OUTPUT); // SD CS
    digitalWrite(9, HIGH);
    pinMode(WS_BTN_PIN, INPUT_PULLUP);

    // Panel power, LCD reset and touch reset live on the expander, so this must run before tft.init()
    setSysI2CBus(&Wire);
    Wire.begin(WS_I2C_SDA, WS_I2C_SCL, WS_I2C_FREQ);
    exioBegin();

    if (!hal_touch_init(touchCfg(), WS_TOUCH_ADDR)) Serial.println("Touch IC not Started");
    // IDTHRESHOLD: lower is more sensitive. 16 suits a bare panel; raise toward 30-40
    // if a case or film is added and phantom touches appear.
    hal_touch_set_threshold(16);

    // begin() only probes the chip ID and turns off the NTC (TS pin) measurement
    pmuReady = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, WS_I2C_SDA, WS_I2C_SCL);
    Serial.printf("[WS-C5] AXP2101 %s\n", pmuReady ? "ready" : "NOT FOUND");
}

/*********************************************************************
** Function: setBrightness
** set brightness value through the expander PWM register
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (!exioReady) return;
    uint8_t bl = brightval == 0 ? 0 : 1 + round((254 * brightval) / 100.0);
    inputLock(); // the input task polls touch on the same I2C bus
    exioWrite(EXIO_PWM_REG, bl);
    inputUnlock();
}

/*********************************************************************
** Function: InputHandler
** Handles touch and button inputs
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    // BOOT button = Back
    static bool btnWasDown = false;
    bool btnDown = digitalRead(WS_BTN_PIN) == LOW;
    if (btnDown && !btnWasDown) {
        btnWasDown = true;
        tm = millis();
        if (!wakeUpScreen()) {
            AnyKeyPress = true;
            EscPress = true;
        }
        return;
    }
    btnWasDown = btnDown;

    BruceTouchPoint t;
    if (hal_touch_read(touchCfg(), t)) {
        tm = millis();
        hal_touch_apply(t);
    }
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 0-100 (0 with no battery)
***************************************************************************************/
int getBattery() {
    // The status bar asks often; the gauge only moves slowly
    static int cached = 0;
    static unsigned long last = 0;
    if (!pmuReady) return 0;
    if (last != 0 && millis() - last < 5000) return cached;
    last = millis();

    inputLock();
    int percent = pmu.isBatteryConnect() ? pmu.getBatteryPercent() : 0;
    inputUnlock();
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    cached = percent;
    return cached;
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Charge state from the AXP2101
***************************************************************************************/
bool isCharging() {
    if (!pmuReady) return false;
    inputLock();
    bool charging = pmu.isCharging();
    inputUnlock();
    return charging;
}

/*********************************************************************
** Function: powerOff
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    _setBrightness(0);
    tft.writecommand(0x10); // SLPIN
    esp_deep_sleep_start(); // BOOT (GPIO28) isn't an LP GPIO, so RST wakes the board
}

// =============================================
// ES8311 Audio Codec Setup (via I2C register writes)
// =============================================
#define ES8311_ADDR 0x18

static bool es8311Write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

// Slave on the bus, 16-bit I2S. With no MCLK the codec runs off SCLK: Bruce sends 16-bit
// stereo, so BCLK = 32 x fs, and the 8x pre-multiplier gives an internal 256 x fs at every
// sample rate. That keeps the coefficient registers fixed whatever rate is played.
static bool es8311InitDac() {
    if (!es8311Write(0x00, 0x1F)) return false; // reset everything
    delay(20);
    es8311Write(0x00, 0x00);
    es8311Write(0x00, 0x80);        // power on, serial port slave
    es8311Write(0x01, 0x3F | 0x80); // clock from SCLK, every clock on
    es8311Write(0x02, 3 << 3);      // pre-divider 1, multiplier 8x
    es8311Write(0x03, 0x10);        // single speed, ADC OSR
    es8311Write(0x04, 0x10);        // DAC OSR
    es8311Write(0x05, 0x00);        // ADC and DAC dividers 1
    es8311Write(0x06, 0x03);        // BCLK not inverted
    es8311Write(0x07, 0x00);        // LRCK divider high
    es8311Write(0x08, 0xFF);        // LRCK divider low
    es8311Write(0x09, 0x0C);        // SDP in: 16-bit I2S
    es8311Write(0x0A, 0x0C);        // SDP out: 16-bit I2S
    es8311Write(0x0D, 0x01);        // analog circuitry up
    es8311Write(0x0E, 0x02);        // PGA and ADC modulator
    es8311Write(0x12, 0x00);        // DAC up
    es8311Write(0x13, 0x10);        // output to the HP drive (the amp's input)
    es8311Write(0x1C, 0x6A);        // ADC equalizer bypass, DC offset cancel
    es8311Write(0x37, 0x08);        // DAC equalizer bypass
    return es8311Write(0x32, 0xD8); // DAC volume, Bruce scales the samples on top
}

// The codec derives its clock from SCLK, so BCLK must be running while it powers up.
// Bruce calls the codec hooks before its own I2S channel exists, so run a short-lived
// master and hand the (only) I2S controller back before playback or recording starts.
static bool es8311InitClocked(bool mic) {
    I2SClass i2s;
    i2s.setPins(WS_I2S_BCLK, WS_I2S_WS, mic ? -1 : WS_I2S_DOUT, mic ? WS_I2S_DIN : -1);
    bool clocks = i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    delay(5);
    inputLock();
    bool ok = es8311InitDac();
    if (mic) {
        es8311Write(0x32, 0x00); // DAC muted while recording
        es8311Write(0x14, 0x1A); // analog mic on MIC1P/N, max PGA gain
        es8311Write(0x17, 0xC8); // ADC volume
    }
    inputUnlock();
    i2s.end();
    if (!clocks) Serial.println("[WS-C5] ES8311: no I2S clock during init");
    return ok;
}

/*********************************************************************
** Function: _setup_codec_speaker
** Handles audio CODEC to enable/disable speaker
**********************************************************************/
void _setup_codec_speaker(bool enable) {
    if (enable) {
        if (!es8311InitClocked(false)) Serial.println("[WS-C5] ES8311 not found");
        return;
    }
    inputLock();
    es8311Write(0x32, 0x00); // mute the DAC between sounds
    inputUnlock();
}

/*********************************************************************
** Function: _setup_codec_mic
** Handles audio CODEC to enable/disable microphone
**********************************************************************/
void _setup_codec_mic(bool enable) {
    if (enable) {
        if (!es8311InitClocked(true)) Serial.println("[WS-C5] ES8311 not found");
        return;
    }
    inputLock();
    es8311Write(0x14, 0x00); // analog mic input off
    es8311Write(0x17, 0x00); // ADC volume to minimum
    inputUnlock();
}
