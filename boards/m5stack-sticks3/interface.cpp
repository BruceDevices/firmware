#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <M5Unified.h>
#include <interface.h>

#define SEL_BTN 11

#define BTN_ACT LOW
#define DW_BTN 12

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // bclk,ws,dout,mclk
    bruceConfigPins.speaker_bus = {(gpio_num_t)17, (gpio_num_t)15, (gpio_num_t)14, (gpio_num_t)18};
    // clk,data,ws,type
    bruceConfigPins.mic_bus = {(gpio_num_t)17, (gpio_num_t)16, (gpio_num_t)15, MIC_TYPE_I2S_MSB};
    bruceConfigPins.i2c_bus = {(gpio_num_t)9, (gpio_num_t)10};  // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)47, (gpio_num_t)48}; // sda, scl
    bruceConfigPins.rfTx = 9;
    bruceConfigPins.rfRx = 10;
    bruceConfigPins.irTx = 46;
    bruceConfigPins.irRx = 42;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};  // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)10, (gpio_num_t)9};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)10, (gpio_num_t)9}; // rx, tx (Grove SCL/SDA fallback)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)6, (gpio_num_t)43};
    bruceConfigPins.PN532_bus = {(gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)6, (gpio_num_t)43};
    // CC1101/NRF24/SDCARD/W5500/LoRa share the main SPI bus (sck=5, miso=4, mosi=6)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)3, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)6, (gpio_num_t)8, (gpio_num_t)1
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)6, (gpio_num_t)7
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)6, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)6, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    M5.begin();
    Wire1.begin(47, 48);
    setSysI2CBus(&Wire1);

    // SEL_BTN -> Next (click) / Sel (double click or hold)
    // DW_BTN  -> Prev (click) / Esc (double click or hold)
    hal_buttons_init_2(DeviceButtons{SEL_BTN, DW_BTN}, 600);

    M5.Power.setExtOutput(false); // It buzzes it ext power is turned on

    /*
| Device  | SCK   | MISO  | MOSI  | CS    | GDO0/CE   |
| ---     | :---: | :---: | :---: | :---: | :---:     |
| SD Card | 5     | 4     | 6     | 7     | ---       |
| CC1101  | 5     | 4     | 6     | 2     | 3         |
| NRF24   | 5     | 4     | 6     | 8     | 1         |
| PN532   | 5     | 4     | 6     | 43    | --        |
| WS500   | 5     | 4     | 6     | **    | **        |
| LoRa    | 5     | 4     | 6     | **    | **        |
    */
    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH); // SD Card CS
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH); // CC1101 CS
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH); // nRF24L01 CS
    pinMode(43, OUTPUT);
    digitalWrite(43, HIGH); // PN532 CS
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW); // M5RF433 avoid Jamming
    pinMode(46, OUTPUT);
    digitalWrite(46, LOW); // Infrared LED Off

    bruceConfig.colorInverted = 0;
}
/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // PWM backlight setup
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

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
void InputHandler(void) { hal_buttons_poll_2(); }

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

bool isCharging() {
    // Strategy to stop buzzing
    static int lastState = -1;
    bool charging = M5.Power.isCharging();
    if (charging && lastState != 1) {
        lastState = 1;
        M5.Power.setExtOutput(false);
    } else if (!charging && lastState != 0) {
        lastState = 0;
        M5.Power.setExtOutput(true);
    }
    return charging;
}

/*********************************************************************
** Function: _setup_codec_speaker
** location: modules/others/audio.cpp
** Handles audio CODEC to enable/disable speaker
**********************************************************************/
static TimerHandle_t speaker_off_timer = NULL;

static void speaker_off_timer_cb(TimerHandle_t xTimer) {
    if (!speaker_off_timer) return;
    static constexpr const uint8_t disabled_bulk_data[] = {0};
    i2c_bulk_write(&Wire1, ES8311_ADDR, disabled_bulk_data); // Shutdown ES8311
    M5.In_I2C.bitOff(0x6E, 0x11, 1 << 3, 100000);            // Set gpio3 output low (turn off PA)
}

void _setup_codec_speaker(bool enable) {

    static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80, // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xB5, // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18, // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01, // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x12, 0x00, // 0x12 SYSTEM/ power-up DAC - NOT default
        2, 0x13, 0x10, // 0x13 SYSTEM/ Enable output to HP drive - NOT default
        2, 0x32, 0xBF, // 0x32 DAC/ DAC volume (0xBF == +-0 dB )
        2, 0x37, 0x08, // 0x37 DAC/ Bypass DAC equalizer - NOT default
        0
    };

    if (speaker_off_timer == NULL) {
        speaker_off_timer =
            xTimerCreate("SpkOffTimer", pdMS_TO_TICKS(100), pdFALSE, (void *)0, speaker_off_timer_cb);
    }

    if (enable) {
        if (speaker_off_timer != NULL && xTimerIsTimerActive(speaker_off_timer)) {
            xTimerStop(speaker_off_timer, 0); // Cancel pending shutdown
        } else {
            i2c_bulk_write(&Wire1, ES8311_ADDR, enabled_bulk_data);
            M5.In_I2C.bitOn(0x6E, 0x11, 1 << 3, 100000); // Set gpio3 output high (turn on PA)
        }
    } else {
        if (speaker_off_timer != NULL) {
            xTimerReset(speaker_off_timer, 0); // Start/reset shutdown timeout for 100ms
        }
    }
}

/*********************************************************************
** Function: _setup_codec_mic
** location: modules/others/mic.cpp
** Handles audio CODEC to enable/disable microphone
**********************************************************************/
void _setup_codec_mic(bool enable) {
    static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80, // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xBA, // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18, // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01, // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x0E, 0x02, // 0x0E SYSTEM/ : Enable analog PGA, enable ADC modulator
        2, 0x14, 0x10, // ES8311_ADC_REG14 : select Mic1p-Mic1n / PGA GAIN (minimum)
        2, 0x17, 0xBF, // ES8311_ADC_REG17 : ADC_VOLUME 0xBF == +- 0 dB
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
