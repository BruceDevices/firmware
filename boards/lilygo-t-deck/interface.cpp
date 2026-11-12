#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Wire.h>
#include <interface.h>

#define BTN_ACT LOW

// Setup for Trackball
void IRAM_ATTR ISR_up();
void IRAM_ATTR ISR_down();
void IRAM_ATTR ISR_left();
void IRAM_ATTR ISR_right();

bool trackball_interrupted = false;
int8_t trackball_up_count = 0;
int8_t trackball_down_count = 0;
int8_t trackball_left_count = 0;
int8_t trackball_right_count = 0;
void IRAM_ATTR ISR_up() {
    trackball_interrupted = true;
    trackball_up_count = 1;
}
void IRAM_ATTR ISR_down() {
    trackball_interrupted = true;
    trackball_down_count = 1;
}
void IRAM_ATTR ISR_left() {
    trackball_interrupted = true;
    trackball_left_count = 1;
}
void IRAM_ATTR ISR_right() {
    trackball_interrupted = true;
    trackball_right_count = 1;
}

void ISR_rst() {
    trackball_up_count = 0;
    trackball_down_count = 0;
    trackball_left_count = 0;
    trackball_right_count = 0;
    trackball_interrupted = false;
}

#define LILYGO_KB_SLAVE_ADDRESS 0x55
#define KB_I2C_SDA 18 // SYS_I2C_SDA
#define KB_I2C_SCL 8  // SYS_I2C_SCL
#define SEL_BTN 0
#define UP_BTN 3
#define DW_BTN 15
#define L_BTN 2
#define R_BTN 1
#define PIN_POWER_ON 10
#define BOARD_TOUCH_INT 16

// covers that variant -- same per-rotation values the old InputHandler's isPlus=false branch used.
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    cfg.pin_sda = KB_I2C_SDA;
    cfg.pin_scl = KB_I2C_SCL;
    cfg.pin_irq = BOARD_TOUCH_INT;
    cfg.i2c_bus = &Wire1;
#ifdef T_DECK_PLUS
    constexpr bool isPlus = true;
#else
    constexpr bool isPlus = false;
#endif
    // rotation:          0         1        2         3
    bool swapXY[4] = {false, true, false, true};
    bool mirrorX[4] = {false, !isPlus, true, isPlus};
    bool mirrorY[4] = {!isPlus, true, isPlus, false};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.speaker_bus = {
        (gpio_num_t)7, (gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)48
    }; // bclk,ws,dout,mclk
    // Wiring kept for reference; this board has no HAS_MICROPHONE gate
    bruceConfigPins.mic_bus = {(gpio_num_t)21, (gpio_num_t)14, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data,ws,type
    bruceConfigPins.sys_i2c = {(gpio_num_t)18, (gpio_num_t)8};                             // sda, scl
    bruceConfigPins.i2c_bus = {(gpio_num_t)43, (gpio_num_t)44};                            // sda, scl (Grove)
    bruceConfigPins.rfTx = 43;
    bruceConfigPins.rfRx = 44;
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = 44;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)44, (gpio_num_t)43}; // rx, tx (Grove)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)-1};
    bruceConfigPins.PN532_bus = {(gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)-1};
    // CC1101/NRF24/SDCARD/W5500/LoRa share the main SPI bus (sck=40, miso=38, mosi=41)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)43, (gpio_num_t)44, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)43, (gpio_num_t)44
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)39
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)43, (gpio_num_t)44, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)40, (gpio_num_t)38, (gpio_num_t)41, (gpio_num_t)9, (gpio_num_t)17, (gpio_num_t)45
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    delay(500);           // time to ESP32C3 start and enable the keyboard
    setSysI2CBus(&Wire1); // Keyboard + GT911 touch both live on the default Wire object
    if (!Wire1.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl))
        Serial.println("Fail starting ESP32-C3 keyboard");

    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(SEL_BTN, INPUT);

    pinMode(9, OUTPUT); // LoRa Radio CS Pin to HIGH (Inhibit the SPI Communication for this module)
    digitalWrite(9, HIGH);

    // Setup for Trackball
    pinMode(UP_BTN, INPUT_PULLUP);
    attachInterrupt(UP_BTN, ISR_up, FALLING);
    pinMode(DW_BTN, INPUT_PULLUP);
    attachInterrupt(DW_BTN, ISR_down, FALLING);
    pinMode(L_BTN, INPUT_PULLUP);
    attachInterrupt(L_BTN, ISR_left, FALLING);
    pinMode(R_BTN, INPUT_PULLUP);
    attachInterrupt(R_BTN, ISR_right, FALLING);

#ifdef T_DECK_PLUS
    bruceConfigPins.gpsBaudrate = 38400;
#endif
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    if (!hal_touch_init(touchCfg())) { Serial.println("Failed to find GT911 - check your wiring!"); }
    // Brightness control must be initialized after tft in this case @Pirata
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}
/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    char keyValue = 0;
    static unsigned long tm = millis();
    BruceTouchPoint t;
    bool touched = hal_touch_read(touchCfg(), t);
    delay(1);
    Wire1.requestFrom(LILYGO_KB_SLAVE_ADDRESS, 1);
    while (Wire1.available() > 0) {
        keyValue = Wire1.read();
        delay(1);
    }
    if (millis() - tm < 200 && !LongPress) return;

    // 0 - UP
    // 1 - Down
    // 2 - Left
    // 3 - Right
    if (trackball_interrupted) {
        uint8_t xx = 1;
        uint8_t yy = 1;
        xx += trackball_left_count;
        xx -= trackball_right_count;
        yy -= trackball_up_count;
        yy += trackball_down_count;
        if (xx == 1 && yy == 1) {
            ISR_rst();
        } else {
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;
        }
        delay(50);
        // Print "bot - xx - yy",  1 is normal value for xx and yy 0 and 2 means movement on the axis
        // Serial.print(bot); Serial.print("-"); Serial.print(xx); Serial.print("-"); Serial.println(yy);
        if (xx < 1 || yy < 1) {
            ISR_rst();
            PrevPress = true;
        } // left , Up
        else if (xx > 1 || yy > 1) {
            ISR_rst();
            NextPress = true;
        } // right, Down
    }

    if (keyValue != (char)0x00) {
        if (!wakeUpScreen()) {
            AnyKeyPress = true;
        } else return;
        KeyStroke.Clear();
        KeyStroke.hid_keys.push_back(keyValue);
        if (keyValue == ' ') KeyStroke.exit_key = true; // key pressed to try to exit
        if (keyValue == (char)0x08) KeyStroke.del = true;
        if (keyValue == (char)0x0D) KeyStroke.enter = true;
        if (digitalRead(SEL_BTN) == BTN_ACT) KeyStroke.fn = true;
        KeyStroke.word.push_back(keyValue);
        if (KeyStroke.del) EscPress = true;
        if (KeyStroke.enter) SelPress = true;
        KeyStroke.pressed = true;
        tm = millis();
    } else KeyStroke.pressed = false;

    if (digitalRead(SEL_BTN) == BTN_ACT) {
        tm = millis();
        if (!wakeUpScreen()) {
            AnyKeyPress = true;
        } else return;
        SelPress = true;
    }

    if ((millis() - tm) > 190 || LongPress) { // one reading each 190ms
        if (touched) {
            tm = millis();
            hal_touch_apply(t);
            return;
        }
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
