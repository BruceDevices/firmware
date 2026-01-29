#include "core/powerSave.h"
#include "core/utils.h"
#include <M5Unified.h>
#include <interface.h>

#define TFT_BRIGHT_CHANNEL 0
#define TFT_BRIGHT_Bits 8
#define TFT_BRIGHT_FREQ 5000

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    M5.begin();
    Wire1.begin(47, 48);

    pinMode(SEL_BTN, INPUT);
    pinMode(DW_BTN, INPUT);

    M5.Power.setExtOutput(false); // It buzzes it ext power is turned on
    //  https://github.com/pr3y/Bruce/blob/main/media/connections/cc1101_stick_SDCard.jpg
    //  Keeps this pin high to allow working with the following pinout
    //  Keeps this pin high to allow working with the following pinout
    // SPI bus
    gpio_reset_pin(GPIO_NUM_0);
    gpio_reset_pin(GPIO_NUM_1);
    gpio_reset_pin(GPIO_NUM_8);
    // Header for future modules
    gpio_reset_pin(GPIO_NUM_2);
    gpio_reset_pin(GPIO_NUM_5);
    gpio_reset_pin(GPIO_NUM_6);
    gpio_reset_pin(GPIO_NUM_9);
    gpio_reset_pin(GPIO_NUM_10);

    pinMode(3, OUTPUT); // SD Card CS
    digitalWrite(3, HIGH);
    pinMode(5, OUTPUT); // CC1101 CS
    digitalWrite(5, HIGH);
    pinMode(6, OUTPUT); // nRF24L01 CS
    digitalWrite(6, HIGH);
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW); // RF jamming prevention
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH); // CS for modules

    // SDCard Pins
    pinMode(0, OUTPUT);
    pinMode(1, INPUT);
    pinMode(8, OUTPUT);

    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(TFT_BL, OUTPUT);
    //=========================================================================
    // Issue: During startup, the SD card might keep the MISO line at a high level continuously, causing RF
    // initialization to fail. Solution：Forcing switch to SD card and sending dummy clocks
    //=========================================================================
    int pin_shared_ctrl = 10; // Controls CS: HIGH=SD_Select, LOW=RF_Select
    int pin_sck = 0;          // SCK Pin for M5StickC Plus 2
    pinMode(pin_shared_ctrl, OUTPUT);
    pinMode(pin_sck, OUTPUT);
    digitalWrite(pin_shared_ctrl, HIGH); // Force Select SD Card
    delay(10);
    for (int i = 0; i < 80; i++) {
        digitalWrite(pin_sck, HIGH);
        delayMicroseconds(10);
        digitalWrite(pin_sck, LOW);
        delayMicroseconds(10);
    } // send dummy clocks
    digitalWrite(pin_shared_ctrl, HIGH); // Keep the SD card selected.

    bruceConfig.colorInverted = 0;
}
/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // PWM backlight setup
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 250);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 250;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 5;
    else dutyCycle = ((brightval * 250) / 100);

    // Serial.printf("dutyCycle for bright 0-255: %d\n", dutyCycle);

    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (!ledcWrite(TFT_BL, dutyCycle)) {
        // Serial.println("Failed to set brightness");
        ledcDetach(TFT_BL);
        ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
        ledcWrite(TFT_BL, dutyCycle);
    }
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int level = M5.Power.getBatteryLevel();
    return (level < 0) ? 0 : (level >= 100) ? 100 : level;
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    static bool selDown = false;
    static unsigned long dwFirstPress = 0;
    static bool dwWaiting = false;
    static unsigned long dwPressStart = 0;
    static bool dwDown = false;
    static bool dwLongFired = false;
    constexpr unsigned long doublePressWindowMs = 300;
    constexpr unsigned long longPressMs = 600;
    unsigned long now = millis();
    if (now - tm < 200 && !LongPress) return;
    if (!wakeUpScreen()) AnyKeyPress = true;
    else return;

    bool selPressed = (digitalRead(SEL_BTN) == BTN_ACT);
    bool dwPressed = (digitalRead(DW_BTN) == BTN_ACT);

    AnyKeyPress = selPressed || dwPressed || dwWaiting;

    if (selPressed && !selDown) {
        SelPress = true;
        tm = now;
    }
    selDown = selPressed;

    if (dwPressed && !dwDown) {
        dwPressStart = now;
        dwLongFired = false;
    }
    if (dwPressed) {
        if (!dwLongFired && (now - dwPressStart) > longPressMs) {
            PrevPress = true;
            dwLongFired = true;
            dwWaiting = false;
            tm = now;
        }
    } else if (dwDown && !dwLongFired) {
        if (dwWaiting && (now - dwFirstPress) <= doublePressWindowMs) {
            PrevPress = true;
            dwWaiting = false;
            tm = now;
        } else {
            dwWaiting = true;
            dwFirstPress = now;
        }
    }
    dwDown = dwPressed;
    if (dwWaiting && !dwPressed && (now - dwFirstPress) > doublePressWindowMs) {
        NextPress = true;
        dwWaiting = false;
        tm = now;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}

bool isCharging() { return M5.Power.isCharging(); }

/*********************************************************************
** Function: _setup_codec_speaker
** location: modules/others/audio.cpp
** Handles audio CODEC to enable/disable speaker
**********************************************************************/
void _setup_codec_speaker(bool enable) {

    static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80, // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xB5, // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18, // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01, // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x12, 0x00, // 0x12 SYSTEM/ power-up DAC - NOT default
        2, 0x13, 0x10, // 0x13 SYSTEM/ Enable output to HP drive - NOT default
        2, 0x32, 0xBF, // 0x32 DAC/ DAC volume (0xBF == ±0 dB )
        2, 0x37, 0x08, // 0x37 DAC/ Bypass DAC equalizer - NOT default
        0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {0};

    i2c_bulk_write(&Wire1, ES8311_ADDR, enable ? enabled_bulk_data : disabled_bulk_data);
}

/*********************************************************************
** Function: _setup_codec_mic
** location: modules/others/mic.cpp
** Handles audio CODEC to enable/disable microphone
**********************************************************************/
void _setup_codec_mic(bool enable) {
    // Set microfone pin for ADV
    mic_bclk_pin = (gpio_num_t)17;

    static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80, // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xBA, // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18, // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01, // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x0E, 0x02, // 0x0E SYSTEM/ : Enable analog PGA, enable ADC modulator
        2, 0x14, 0x10, // ES8311_ADC_REG14 : select Mic1p-Mic1n / PGA GAIN (minimum)
        2, 0x17, 0xBF, // ES8311_ADC_REG17 : ADC_VOLUME 0xBF == ± 0 dB
        2, 0x1C, 0x6A, // ES8311_ADC_REG1C : ADC Equalizer bypass, cancel DC offset in digital domain
        0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
        2,
        0x0D,
        0xFC, // 0x0D SYSTEM/ Power down analog circuitry
        2,
        0x0E,
        0x6A, // 0x0E SYSTEM
        2,
        0x00,
        0x00, // 0x00 RESET/  CSM POWER DOWN
        0
    };

    i2c_bulk_write(&Wire1, ES8311_ADDR, enable ? enabled_bulk_data : disabled_bulk_data);
}
