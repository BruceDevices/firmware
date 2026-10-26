#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Wire.h>
#include <globals.h>
#include <interface.h>

// Rotary encoder
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/encoder.h"

#define ENCODER_INA 40
#define ENCODER_INB 41
#define ENCODER_KEY 7

#define SEL_BTN ENCODER_KEY

#define AUDIO_I2S_MCLK 10
#define AUDIO_I2S_SCK 11
#define AUDIO_I2S_SDIN 17
#define AUDIO_I2S_SDOUT 45
#define AUDIO_I2S_WS 18
#define BK_BTN 0
#define BTN_ACT LOW
#define CAPS_LOCK 0x00
#define EXPANDS_AMP_EN 1
#define EXPANDS_DRV_EN 0
#define EXPANDS_GPIO_EN 11
#define EXPANDS_GPS_RST 7
#define EXPANDS_KB_EN 10
#define EXPANDS_KB_PWR 8
#define EXPANDS_KB_RST 2
#define EXPANDS_LORA_EN 3
#define EXPANDS_SD_DET 12
#define EXPANDS_SD_EN 14
#define KB_I2C_ADDRESS 0x34
#define KEYBOARD_BL 46
#define KEY_SHIFT 0x1c

static const uint8_t backlightPins[] = {TFT_BL, KEYBOARD_BL};

static DeviceEncoder encoderCfg() {
    DeviceEncoder cfg;
    // A/B swapped on purpose: this board's wiring reports rotation opposite to
    // the other encoder boards for the Next/Prev mapping hal_encoder_poll()
    // assumes (hal_encoder_poll() has no invert flag).
    cfg.pin_a = ENCODER_INB;
    cfg.pin_b = ENCODER_INA;
    cfg.pin_sel = SEL_BTN;
    cfg.pin_esc = BK_BTN;
    return cfg;
}

#include <esp32-hal-dac.h>

// Battery libs
#ifdef GAUGE_BQ27220
#define BATTERY_DESIGN_CAPACITY 1500
#endif

#include "core/i2c_finder.h"
#include "modules/rf/rf_utils.h"

// Keyboard
#include <Adafruit_TCA8418.h>
Adafruit_TCA8418 *keyboard;

// Haptic
#include "HapticDrivers.hpp"
HapticDriver_DRV2605 drv;
void hapticTest(uint8_t effect);
uint8_t effect = 1;

// Audio
#include "AudioBoard.h"
DriverPins PinsAudioBoardES8311;
AudioBoard board(AudioDriverES8311, PinsAudioBoardES8311);

// Keyboard
bool fn_key_pressed = false;
bool shift_key_pressed = false;
bool caps_lock = false;

#define KB_ROWS 4
#define KB_COLS 10

struct KeyValue_t {
    const char value_first;
    const char value_second;
    const char value_third;
};

const KeyValue_t _key_value_map[KB_ROWS][KB_COLS] = {
    {{'q', 'Q', '1'},
     {'w', 'W', '2'},
     {'e', 'E', '3'},
     {'r', 'R', '4'},
     {'t', 'T', '5'},
     {'y', 'Y', '6'},
     {'u', 'U', '7'},
     {'i', 'I', '8'},
     {'o', 'O', '9'},
     {'p', 'P', '0'}},

    {{'a', 'A', '*'},
     {'s', 'S', '/'},
     {'d', 'D', '+'},
     {'f', 'F', '-'},
     {'g', 'G', '='},
     {'h', 'H', ':'},
     {'j', 'J', '\''},
     {'k', 'K', '"'},
     {'l', 'L', '@'},
     {KEY_ENTER, KEY_ENTER, '&'}},

    {{KEY_FN, KEY_FN, KEY_FN},
     {'z', 'Z', '_'},
     {'x', 'X', '$'},
     {'c', 'C', ';'},
     {'v', 'V', '?'},
     {'b', 'B', '!'},
     {'n', 'N', ','},
     {'m', 'M', '.'},
     {KEY_SHIFT, KEY_SHIFT, CAPS_LOCK},
     {KEY_BACKSPACE, KEY_BACKSPACE, '#'}},

    {{' ', ' ', KEY_TAB}}
};

char getKeyChar(uint8_t k) {
    char keyVal;
    if (fn_key_pressed) {
        keyVal = _key_value_map[k / 10][k % 10].value_third;
    } else if (shift_key_pressed ^ caps_lock) {
        keyVal = _key_value_map[k / 10][k % 10].value_second;
    } else {
        keyVal = _key_value_map[k / 10][k % 10].value_first;
    }
    return keyVal;
}

int handleSpecialKeys(uint8_t k, bool pressed) {
    char keyVal = _key_value_map[k / 10][k % 10].value_first;
    switch (keyVal) {
        case KEY_FN: fn_key_pressed = !fn_key_pressed; return 1;
        case KEY_SHIFT: {
            shift_key_pressed = pressed;
            if (fn_key_pressed && shift_key_pressed) { caps_lock = !caps_lock; }
            return 1;
        }
        default: break;
    }
    return 0;
}

void initPeripherals() {
    if (ioExpander.init()) {
        const uint8_t expands[] = {
            EXPANDS_DRV_EN,
            EXPANDS_AMP_EN,
            EXPANDS_KB_RST,
            EXPANDS_LORA_EN,
            EXPANDS_GPS_EN,
            EXPANDS_NFC_EN,
            EXPANDS_GPS_RST,
            EXPANDS_KB_PWR,
            EXPANDS_KB_EN,
            EXPANDS_GPIO_EN,
            EXPANDS_SD_EN
        };
        for (auto pin : expands) {
            ioExpander.pinMode(pin, OUTPUT);
            ioExpander.digitalWrite(pin, HIGH);
            delay(1);
        }
        ioExpander.pinMode(EXPANDS_SD_DET, INPUT);
        Serial.println("Initializing expander OK");
    } else {
        Serial.println("Initializing expander failed");
    }
    delay(50);
}
/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // bclk,ws,dout,mclk
    bruceConfigPins.speaker_bus = {(gpio_num_t)11, (gpio_num_t)18, (gpio_num_t)45, (gpio_num_t)10};
    // clk,data,ws,type
    bruceConfigPins.mic_bus = {(gpio_num_t)18, (gpio_num_t)17, (gpio_num_t)11, MIC_TYPE_I2S_MSB};
    bruceConfigPins.i2c_bus = {(gpio_num_t)3, (gpio_num_t)2}; // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)3, (gpio_num_t)2}; // sda, scl
    bruceConfigPins.rfTx = 3;
    bruceConfigPins.rfRx = 2;
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = -1;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)4, (gpio_num_t)12};   // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)2, (gpio_num_t)3}; // rx, tx (inherited from Grove I2C)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)21};
    // No dedicated PN532 on this board (NFC is ST25R3916) - RC522-SPI shares the SD card SPI slot
    bruceConfigPins.PN532_bus = {(gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)21};
    // CC1101/NRF24 live over the GPIO expansion header, sharing sck/miso/mosi with the main bus
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)44, (gpio_num_t)43, (gpio_num_t)9
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)44, (gpio_num_t)43
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)21
    }; // sck,miso,mosi,cs
    bruceConfigPins.ST25R_bus = {
        (gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)39, (gpio_num_t)5, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,irq,-
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)44, (gpio_num_t)43, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)35, (gpio_num_t)33, (gpio_num_t)34, (gpio_num_t)36, (gpio_num_t)47, (gpio_num_t)14
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    pinMode(SEL_BTN, INPUT);
    pinMode(BK_BTN, INPUT);
    pinMode(bruceConfigPins.ST25R_bus.io0, INPUT);

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);

    pinMode(NFC_CS, OUTPUT);
    digitalWrite(NFC_CS, HIGH);

#if !defined(LITE_VERSION)
    pinMode(bruceConfigPins.LoRa_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.LoRa_bus.cs, HIGH);

    pinMode(bruceConfigPins.LoRa_bus.io0, OUTPUT);
    digitalWrite(bruceConfigPins.LoRa_bus.io0, HIGH);
#endif
    setSysI2CBus(&Wire); // PMU/keyboard/RTC/codec all live on the default Wire object
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);

    // Power management
    DevicePmic pmicCfg;
    pmicCfg.pin_sda = bruceConfigPins.sys_i2c.sda;
    pmicCfg.pin_scl = bruceConfigPins.sys_i2c.scl;
    pmicCfg.address = 0x6B; // BQ25896
    pmicCfg.charge_target_mv = 4288;
    pmicCfg.charge_current_ma = 704;
    hal_pmic_init(pmicCfg);

    // Battery gauge
    DeviceGauge gaugeCfg;
    gaugeCfg.design_capacity_mah = BATTERY_DESIGN_CAPACITY;
    hal_gauge_init(gaugeCfg);
    initPeripherals();

    // Initialise keyboard
    keyboard = new Adafruit_TCA8418();
    if (!keyboard->begin(KB_I2C_ADDRESS, &Wire)) {
        Serial.println("Failed to find Keyboard");

    } else {
        Serial.println("Initializing Keyboard succeeded");
    }
    keyboard->matrix(KB_ROWS, KB_COLS);
    keyboard->flush();

    // Start with default IR, RF, GPS and RFID Configs, replace old
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = ST25R3916_SPI_MODULE;
    bruceConfigPins.irRx = 1;
    bruceConfigPins.gpsBaudrate = 38400;

    // Encoder
    hal_encoder_init(encoderCfg(), EncoderLatchMode::FOUR3);

    // Haptic driver
    if (!drv.begin(Wire, bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl)) {
        Serial.println("Failed to find DRV2605.");
    } else {
        Serial.println("Init DRV2605 Sensor success!");
        drv.selectLibrary(1);
        drv.setMode(HapticMode::INTERNAL_TRIGGER);
        drv.setActuatorType(HapticActuatorType::ERM);

        // Startup buzz
        drv.setWaveform(0, 70);
        drv.setWaveform(1, 0);
        drv.run();
    }

    // Audio
    // https://github.com/meshtastic/firmware/blob/ee6449746bf8c5358b8adbde05b96e2b2d04f450/src/platform/extra_variants/t_lora_pager/variant.cpp
    // AudioDriverLogger.begin(Serial, AudioDriverLogLevel::Debug);
    // I2C: function, scl, sda
    PinsAudioBoardES8311.addI2C(PinFunction::CODEC, Wire);
    // I2S: function, mclk, bck, ws, data_out, data_in
    PinsAudioBoardES8311.addI2S(
        PinFunction::CODEC, AUDIO_I2S_MCLK, AUDIO_I2S_SCK, AUDIO_I2S_WS, AUDIO_I2S_SDOUT, AUDIO_I2S_SDIN
    );

    // configure codec
    CodecConfig cfg;
    cfg.input_device = ADC_INPUT_LINE1;
    cfg.output_device = DAC_OUTPUT_ALL;
    cfg.i2s.bits = BIT_LENGTH_16BITS;
    cfg.i2s.rate = RATE_44K;
    board.begin(cfg);
}

void _post_setup_gpio() {
    initPeripherals();
    hal_bright_attach(backlightPins, 2);
    hal_bright_set(backlightPins, 2, 100);
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    static float smoothed = -1;
    constexpr float alpha = 0.2f;

    int pct = hal_gauge_get_percent();

    if (pct >= 0 && pct <= 100) {
        if (smoothed < 0) {
            smoothed = pct;
        } else {
            smoothed = alpha * pct + (1 - alpha) * smoothed;
        }
    }

    return static_cast<int>(std::ceil(smoothed));
}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(backlightPins, 2, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    uint8_t keyValue = 0;
    uint8_t keyVal = '\0';

    if (keyboard->available() > 0) {
        keyStroke pendingKey;
        bool keyPulse = false;
        bool hapticPulse = false;

        // Drain the full TCA8418 FIFO so quick taps are handled immediately.
        while (keyboard->available() > 0) {
            int keyValue = keyboard->getEvent();
            bool pressed = keyValue & 0x80;
            keyValue &= 0x7F;
            keyValue--;

            if (keyValue / 10 >= 4) continue;
            if (handleSpecialKeys(keyValue, pressed) > 0) continue;

            keyVal = getKeyChar(keyValue);
            if (!pressed || keyVal == '\0') continue;
            if (wakeUpScreen()) continue;

            pendingKey.hid_keys.push_back(keyVal);
            if (keyVal == KEY_BACKSPACE) {
                pendingKey.del = true;
                EscPress = true;
            }
            if (keyVal == KEY_ENTER) {
                pendingKey.enter = true;
                SelPress = true;
            }
            if (keyVal == KEY_FN) pendingKey.fn = true;
            pendingKey.word.push_back(keyVal);
            keyPulse = true;
            hapticPulse = true;
        }

        if (keyPulse) {
            pendingKey.pressed = true;
            KeyStroke = pendingKey;

            if (hapticPulse) {
                drv.setWaveform(0, 81);
                drv.setWaveform(1, 0);
                drv.run();
            }
        } else {
            KeyStroke.Clear();
        }
    } else KeyStroke.Clear();

    if (KeyStroke.enter) {
        if (!wakeUpScreen()) {
            AnyKeyPress = true;
            drv.setWaveform(0, 1); // Haptic feedback
            drv.setWaveform(1, 0);
            drv.run();
        }
    }

    // Encoder rotation, Select (encoder key) and Esc (back key)
    bool hadPress = NextPress || PrevPress || SelPress || EscPress;
    hal_encoder_poll(encoderCfg());
    if (!hadPress && (NextPress || PrevPress || SelPress || EscPress)) {
        drv.setWaveform(0, 1); // Haptic feedback
        drv.setWaveform(1, 0);
        drv.run();
    }
}

void powerOff() { hal_pmic_shutdown(); }

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
#ifdef GAUGE_BQ27220
bool isCharging() { return hal_gauge_is_charging(); }
#else
bool isCharging() { return false; }
#endif

/*********************************************************************
** Function: _setup_codec_speaker
** location: modules/others/audio.cpp
** Handles audio CODEC to enable/disable speaker
**********************************************************************/
void _setup_codec_speaker(bool enable) {}

/*********************************************************************
** Function: _setup_codec_mic
** location: modules/others/mic.cpp
** Handles audio CODEC to enable/disable microphone
**********************************************************************/
void _setup_codec_mic(bool enable) {}
