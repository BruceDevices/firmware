#ifndef __GLOBALS__
#define __GLOBALS__

#include <precompiler_flags.h>

#include <interface.h>

// Globals.h

#define ALCOLOR TFT_RED

#include "SerialDevice.h"
#include "core/USBSerial/USBSerial.h"
#include "core/config.h"
#include "core/configPins.h"
#include "core/serial_commands/cli.h"
#include "core/startup_app.h"
#include <Arduino.h>
#include <ESP32Time.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <SPI.h>
#include <functional>
#include <io_expander/io_expander.h> // ./lib/HAL
#include <vector>
extern io_expander ioExpander;

#if defined(HAS_RTC)
#if defined(HAS_RTC_PCF85063A)
#include "../lib/RTC/pcf85063_RTC.h"
extern pcf85063_RTC _rtc;
#else
#include "../lib/RTC/cplus_RTC.h"
extern cplus_RTC _rtc;
#endif
extern RTC_TimeTypeDef _time;
extern RTC_DateTypeDef _date;
#endif

// Declaração dos objetos TFT
#include <DisplayDrivers.h>
#include <tftLogger.h>
extern tft_logger tft;
extern tft_sprite sprite;
extern tft_sprite draw;

#include "hal/power/gauge.h"
#include "hal/power/pmic.h"

extern int8_t interpreter_state; // -1 - stopped, 0 - background, 1 - waiting for foreground, 2 - foreground

extern BruceConfig bruceConfig;
extern BruceConfigPins bruceConfigPins;
extern SerialCli serialCli;
extern SerialDevice *serialDevice;
extern USBSerial USBserial;
extern StartupApp startupApp;

extern char timeStr[16];
extern SPIClass sdcardSPI;
extern SPIClass AUX_SPI;
extern bool clock_set;
extern time_t localTime;
extern struct tm *timeInfo;
extern ESP32Time rtc;
extern NTPClient timeClient;

extern int prog_handler; // 0 - Flash, 1 - LittleFS, 2 - Download

extern bool sdcardMounted; // inform if SD Cardis active or not

extern bool wifiConnected; // inform if wifi is active or not
extern bool isWebUIActive; // inform if WebUI is active or not

extern volatile int tftWidth;
extern volatile int tftHeight;

extern String wifiIP;

extern bool BLEConnected; // inform if BLE is active or not

extern bool gpsConnected; // inform if GPS is active or not

struct Option {
    String label;
    std::function<void()> operation;
    bool selected = false;
    bool enabled = true;
    bool (*hover)(void *hoverPointer, bool shouldRender);
    void *hoverPointer;
    bool hovered; // return to the remote (webui or app) if it is hovered on the loopoptions

    bool hasColor = false;
    uint16_t color = 0;

    // On-screen bounding box of this item in its most recent draw, for tap-to-select hit-testing
    // (see loopOptions()). Populated by drawOptions()/drawGridCell(); zero/unused otherwise.
    uint16_t x = 0, y = 0, w = 0, h = 0;
    bool contain(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }

    Option(
        const char *lbl, const std::function<void()> &op, bool sel = false,
        bool (*hov)(void *hoverPointer, bool shouldRender) = nullptr, void *ptr = nullptr, bool hvrd = false,
        bool en = true, bool hasClr = false, uint16_t clr = 0
    )
        : label(lbl), operation(op), selected(sel), enabled(en), hover(hov), hoverPointer(ptr), hovered(hvrd),
          hasColor(hasClr), color(clr) {}

    Option(
        const String &lbl, const std::function<void()> &op, bool sel = false,
        bool (*hov)(void *hoverPointer, bool shouldRender) =
            nullptr, // hover lambda returns true if it already handled rendering
        void *ptr = nullptr, bool hvrd = false, bool en = true, bool hasClr = false, uint16_t clr = 0
    )
        : label(lbl), operation(op), selected(sel), enabled(en), hover(hov), hoverPointer(ptr), hovered(hvrd),
          hasColor(hasClr), color(clr) {}
};

struct keyStroke { // DO NOT CHANGE IT!!!!!
    bool pressed = false;
    bool exit_key = false;
    bool fn = false;
    bool del = false;
    bool enter = false;
    bool alt = false;
    bool ctrl = false;
    bool gui = false;
    uint8_t modifiers = 0;
    std::vector<char> word;
    std::vector<uint8_t> hid_keys;
    std::vector<uint8_t> modifier_keys;

    // Clear function
    void Clear() {
        pressed = false;
        exit_key = false;
        fn = false;
        del = false;
        enter = false;
        alt = false;
        ctrl = false;
        gui = false;
        modifiers = 0;
        word.clear();
        hid_keys.clear();
        modifier_keys.clear();
    }
};

struct BruceTouchPoint {
    bool pressed = false;
    uint16_t x;
    uint16_t y;

    // clear touch to better handle tasks
    void Clear(void) {
        pressed = false;
        x = 0;
        y = 0;
    }
};

extern BruceTouchPoint touchPoint;
// true (default): touchHeatMap() maps taps anywhere on screen into zone-based Prev/Sel/Next/Esc/Up/Down,
// same as physical buttons. A screen that wants to hit-test raw taps itself (e.g. tap-to-select in
// loopOptions) sets this false while it runs; the TouchFooter band keeps working either way.
extern volatile bool touchZoneOutsideFooterEnabled;
extern keyStroke KeyStroke;
extern std::vector<Option> options;

template <typename R, typename... Args>
std::function<void()> lambdaHelper(R (*callback)(Args...), std::decay_t<Args>... args) {
    return [=]() { (void)callback(args...); };
}

extern String fileToCopy;

extern const int bufSize;

extern bool returnToMenu; // variable to check and break loops to return to main menu

extern String cachedPassword;

extern int currentScreenBrightness;

// Screen sleep control variables
extern unsigned long previousMillis;
extern bool isSleeping;
extern bool isScreenOff;
extern bool dimmer;

extern volatile bool NextPress;

extern volatile bool PrevPress;

extern volatile bool UpPress;

extern volatile bool DownPress;

extern volatile bool SelPress;

extern volatile bool EscPress;

extern volatile bool AnyKeyPress;

extern volatile bool NextPagePress;

extern volatile bool PrevPagePress;

extern volatile bool LongPress;

extern volatile bool SerialCmdPress;

extern volatile int forceMenuOption;

extern volatile uint8_t menuOptionType; // updates when drawing loopoptions, to send to remote controller

extern String menuOptionLabel;

#ifdef HAS_ENCODER_LED
extern volatile int EncoderLedChange;
#endif

// Net pending rotary encoder steps, independent from NextPress/PrevPress,
// so a consumer can apply a whole backlog at once instead of one per redraw.
extern volatile int32_t RotaryNetSteps;

#ifdef HAS_ENCODER
static inline int32_t drainRotarySteps() {
    int32_t steps = RotaryNetSteps;
    RotaryNetSteps -= steps;
    return steps;
}
#endif

extern TaskHandle_t xHandle;

// Recursive mutex shared by the InputHandler task (writer) and its consumers (check(),
// _getKeyPress()). The consumers wait on it instead of suspending the input task, so neither
// side is ever frozen mid-update (or mid SPI transfer when the touchscreen shares the display bus).
void inputLockInit();
void inputLock();
void inputUnlock();

extern inline bool check(volatile bool &btn, bool resetButtonStatus = true) {
    if (!btn) return false;
#ifdef HAS_ENCODER
    // NextPress/PrevPress here are rotary-encoder-derived, not raw mechanical
    // button reads -- the encoder's own quadrature decode already rejects
    // bounce, so they need no extra handling.
    if (&btn == &NextPress || &btn == &PrevPress) {
        if (resetButtonStatus) {
            btn = false;
            AnyKeyPress = false;
            SerialCmdPress = false;
        }
        return true;
    }
#endif
    inputLock();
    if (resetButtonStatus) {
        btn = false;
        AnyKeyPress = false;
        SerialCmdPress = false;
    }
    inputUnlock();
    return true;
}

extern gpio_num_t mic_bclk_pin; // used to configure Cardputer ADV Microphone

#endif
