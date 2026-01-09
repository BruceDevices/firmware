#include "core/powerSave.h"
#include "core/utils.h"
#include <Wire.h>
#include <bq27220.h>
#include <globals.h>
#include <interface.h>

// Rotary encoder
#include <RotaryEncoder.h>
extern RotaryEncoder *encoder;
RotaryEncoder *encoder = nullptr;
IRAM_ATTR void checkPosition() { encoder->tick(); }

// GPIO expander
#include <ExtensionIOXL9555.hpp>
ExtensionIOXL9555 io;

// Charger chip
#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include <esp32-hal-dac.h>
XPowersPPM PPM;

// Battery libs
#ifdef USE_BQ27220_VIA_I2C
#define BATTERY_DESIGN_CAPACITY 1500
#include <bq27220.h>
BQ27220 bq;
#endif

#include "core/i2c_finder.h"
#include "modules/rf/rf_utils.h"

// Keyboard
#include <Adafruit_TCA8418.h>
Adafruit_TCA8418 *keyboard;

// Haptic
#include "SensorDRV2605.hpp"
SensorDRV2605 drv;
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

    {{' ', ' ', ' '}}
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

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {

    pinMode(SEL_BTN, INPUT);
    pinMode(BK_BTN, INPUT);

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    pinMode(NFC_CS, OUTPUT);
    digitalWrite(NFC_CS, HIGH);

    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);

    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, HIGH);

    // Power management
    bool pmu_ret = false;
    pmu_ret = PPM.init(Wire, GROVE_SDA, GROVE_SCL, BQ25896_SLAVE_ADDRESS);
    if (pmu_ret) {
        PPM.setSysPowerDownVoltage(3300);
        PPM.setInputCurrentLimit(3250);
        Serial.printf("getInputCurrentLimit: %ld mA\n", PPM.getInputCurrentLimit());
        PPM.disableCurrentLimitPin();
        PPM.setChargeTargetVoltage(4208);
        PPM.setPrechargeCurr(64);
        PPM.setChargerConstantCurr(832);
        PPM.getChargerConstantCurr();
        Serial.printf("getChargerConstantCurr: %d mA\n", PPM.getChargerConstantCurr());
        PPM.enableMeasure(PowersBQ25896::CONTINUOUS);
        PPM.disableOTG();
        PPM.enableCharge();
    }

    // Battery gauge
    if (bq.getDesignCap() != BATTERY_DESIGN_CAPACITY) { bq.setDesignCap(BATTERY_DESIGN_CAPACITY); }

    // IO Expander
    // TODO: Needs updating to use the same interface as the other IO Expanders (io_expander ioExpander)
    // if (ioExpander.init(IO_EXPANDER_ADDRESS, &Wire)) {
    //     const uint8_t expands[] = {
    //         EXPANDS_KB_RST,
    //         EXPANDS_KB_EN,
    //         EXPANDS_SD_EN,
    //         EXPANDS_DRV_EN,
    //         EXPANDS_AMP_EN, // Audio
    //     };
    //     for (auto pin : expands) {
    //         ioExpander.pinMode(pin, OUTPUT);
    //         ioExpander.digitalWrite(pin, HIGH);
    //         delay(1);
    //     }
    //     ioExpander.pinMode(EXPANDS_SD_PULLEN, INPUT);
    //     ioExpander.digitalWrite(EXPANDS_DRV_EN, LOW);
    // } else {
    //     Serial.println("Initializing expander failed");
    // }
    if (io.begin(Wire, IO_EXPANDER_ADDRESS)) {
        const uint8_t expands[] = {
            EXPANDS_KB_RST,
            EXPANDS_KB_EN,
            EXPANDS_SD_EN,
            EXPANDS_DRV_EN,
            EXPANDS_AMP_EN, // Audio
            EXPANDS_LORA_EN,
            EXPANDS_GPS_EN,
            EXPANDS_GPS_RST,
            EXPANDS_NFC_EN,
        };
        for (auto pin : expands) {
            io.pinMode(pin, OUTPUT);
            io.digitalWrite(pin, HIGH);
            delay(1);
        }
        io.pinMode(EXPANDS_SD_PULLEN, INPUT);
    } else {
        Serial.println("Initializing expander failed");
    }

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
    pinMode(ENCODER_KEY, INPUT);
    encoder = new RotaryEncoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::FOUR3);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INA), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INB), checkPosition, CHANGE);

    // Haptic driver
    if (!drv.begin(Wire, SDA, SCL)) {
        Serial.println("Failed to find DRV2605.");
    } else {
        Serial.println("Init DRV2605 Sensor success!");
        drv.selectLibrary(1);
        drv.setMode(SensorDRV2605::MODE_INTTRIG);
        drv.useERM();

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

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    static float smoothed = -1;
    constexpr float alpha = 0.2f;

    int pct = bq.getChargePcnt();

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
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
        analogWrite(KEYBOARD_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
        analogWrite(KEYBOARD_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = millis();
    static int posDifference = 0;
    static int lastPos = 0;
    bool sel = !BTN_ACT;
    bool esc = !BTN_ACT;

    uint8_t keyValue = 0;
    uint8_t keyVal = '\0';

    if (millis() - tm < 500) return;

    int newPos = encoder->getPosition();
    if (newPos != lastPos) {
        posDifference += (newPos - lastPos);
        lastPos = newPos;
    }

    sel = digitalRead(SEL_BTN);
    esc = digitalRead(BK_BTN);

    if (keyboard->available() > 0) {
        int keyValue = keyboard->getEvent();
        bool pressed = keyValue & 0x80;
        keyValue &= 0x7F;
        keyValue--;
        if (keyValue / 10 < 4) {
            if (handleSpecialKeys(keyValue, pressed) > 0) goto END;
            keyVal = getKeyChar(keyValue);
        }
        if (pressed && !wakeUpScreen() && keyVal != '\0') {
            KeyStroke.Clear();
            KeyStroke.hid_keys.push_back(keyVal);
            if (keyVal == KEY_BACKSPACE) {
                KeyStroke.del = true;
                EscPress = true;
            }
            if (keyVal == KEY_ENTER) {
                KeyStroke.enter = true;
                SelPress = true;
            }
            if (keyVal == KEY_FN) KeyStroke.fn = true;
            KeyStroke.word.push_back(keyVal);
            KeyStroke.pressed = true;

            // Haptic feedback
            drv.setWaveform(0, 81);
            drv.setWaveform(1, 0);
            drv.run();
        }
    } else KeyStroke.Clear();

    if (posDifference != 0 || sel == BTN_ACT || esc == BTN_ACT || KeyStroke.enter) {
        if (!wakeUpScreen()) {
            AnyKeyPress = true;

            // Haptic feedback
            drv.setWaveform(0, 1);
            drv.setWaveform(1, 0);
            drv.run();

            if (posDifference < 0) {
                PrevPress = true;
                posDifference++;
            }
            if (posDifference > 0) {
                NextPress = true;
                posDifference--;
            }
            if (sel == BTN_ACT) SelPress = true;
            if (esc == BTN_ACT) EscPress = true;
        } else goto END;
    }

END:
    if (sel == BTN_ACT || esc == BTN_ACT) tm = millis();
}

void powerOff() { PPM.shutdown(); }

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
#ifdef USE_BQ27220_VIA_I2C
bool isCharging() { return bq.getIsCharging(); }
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
