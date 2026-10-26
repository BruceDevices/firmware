// =====================================================================================
//  boards/_New-Device-Model/interface.cpp  --  TEMPLATE
//
//  Copy this whole folder to boards/<your-board>/, rename the .ini and keep this file
//  named interface.cpp. It is the ONLY board-specific C++ file: everything the firmware
//  needs to know about this device that isn't a compile-time -D flag lives here.
//
//  All the entry points below except InputHandler() have a weak default in src/ (see
//  src/main.cpp, src/core/utils.cpp, src/core/mykeyboard.cpp, src/core/settings.cpp,
//  src/core/display.h), so a board only defines the ones it actually needs. Deleting a
//  function from this file is always safe; InputHandler() is the exception, it has no
//  default and every board must provide one.
//
//  Boot order (src/main.cpp):
//      _setup_gpio()        pins + input HAL + PMIC. Runs FIRST, before the display,
//                           before the filesystem, before bruceConfigPins is loaded
//                           from /brucePins.conf.
//      _pre_storage_gpio()  after the first TFT use, before LittleFS/SD are mounted
//      _post_setup_gpio()   after display + storage: backlight, and anything whose bus
//                           must already be up (e.g. a touch controller on the TFT SPI)
//      _late_setup_gpio()   after the input task exists
//
//  Anything assigned to bruceConfigPins.* in _setup_gpio() is a DEFAULT: brucePins.conf
//  overrides it on the next boot and the user can re-map it from Config > Set Device
//  pins without rebuilding. Pins that are hard wiring (display, buttons, backlight) are
//  -D flags in the .ini instead, because they can't be changed at runtime.
// =====================================================================================

#include "core/powerSave.h"
#include <interface.h>

// Shared hardware-abstraction layer -- include only what this board uses.
// Every hal_* function exists for every board: when the matching -D macro isn't set it
// compiles to a no-op returning false/-1, so nothing needs to be #ifdef'd here.
// Full API notes: src/hal/README.md, struct fields: src/hal/device.h.
#include "hal/bright/bright.h"
#include "hal/device.h"
// #include "hal/inputs/buttons.h"
// #include "hal/inputs/encoder.h"
// #include "hal/inputs/touch.h"
// #include "hal/power/gauge.h"
// #include "hal/power/pmic.h"
// #include "core/bus_HAL.h"   // setSysI2CBus(), only when sys_i2c is not on Wire1

// -------------------------------------------------------------------------------------
//  Board wiring that is NOT in the .ini
//
//  Button/encoder GPIOs are plain #defines here (not -D flags) because nothing outside
//  this file uses them. Keep the names short and local.
// -------------------------------------------------------------------------------------
// #define BTN_PREV 39
// #define BTN_NEXT 40
// #define BTN_UP 38
// #define BTN_DOWN 41
// #define BTN_SEL 5
// #define BTN_ESC 6

// =====================================================================================
//  INPUT CONFIG
//
//  The input HAL is stateless: the board describes its wiring in a small Device* struct
//  and hands the same struct to both the _init() and the _poll()/_read() call. Building
//  it in a small function (instead of a global) keeps it in sync and costs nothing.
//
//  Fill in one block per input source the board actually has -- they combine freely.
//  Buttons + touchscreen + keyboard + encoder on the same device is fine; InputHandler()
//  below then just calls one poll per source. The only exclusivity is among the button
//  layouts themselves: at most one HAS_x_BUTTON(S) flag in the .ini.
// =====================================================================================

// ---- (A) BUTTONS : HAS_1_BUTTON / HAS_2_BUTTONS / HAS_3_BUTTONS / HAS_4_BUTTONS /
//                    HAS_5_BUTTONS / HAS_6_BUTTONS -------------------------------------
//
//  DeviceButtons is positional -- which physical pin goes in btn1..btn6 IS the mapping,
//  and it differs per layout (src/hal/inputs/buttons.cpp):
//
//    1 button  : btn1 only. One pin carries everything by pulse length:
//                short = Next, double short (300ms window) = Prev,
//                long (550ms) = Sel, extra long (1200ms) = Esc.
//    2 buttons : btn1, btn2. btn1 click = Next, btn1 double-click/hold = Sel;
//                btn2 click = Prev, btn2 double-click/hold = Esc.
//                REQUIRES -DBUTTONS_IDF_COMPONENT=1 and uses hal_buttons_init_2().
//    3 buttons : btn1 = Prev, btn2 = Next, btn3 = Sel. Prev+Next together = Esc.
//    5 buttons : btn1 = Prev, btn2 = Next, btn3 = Up, btn4 = Down, btn5 = Sel.
//                Prev+Next together = Esc.
//    6 buttons : same as 5, plus btn6 = Esc.
//
//  pullup     = true  -> pins are configured INPUT_PULLUP (buttons wired to GND).
//               false -> plain INPUT, for boards with external pull-ups/pull-downs.
//  activeHigh = true  -> a pressed button reads HIGH (rare; most boards are active LOW).
//
// static DeviceButtons buttonsCfg() {
//     DeviceButtons cfg;
//     cfg.btn1 = BTN_PREV;
//     cfg.btn2 = BTN_NEXT;
//     cfg.btn3 = BTN_UP;
//     cfg.btn4 = BTN_DOWN;
//     cfg.btn5 = BTN_SEL;
//     cfg.btn6 = BTN_ESC;
//     cfg.pullup = true; // no external pull-ups on this board
//     cfg.activeHigh = false;
//     return cfg;
//     // Shorthand used by several boards -- same field order as above:
//     // return DeviceButtons{BTN_PREV, BTN_NEXT, BTN_UP, BTN_DOWN, BTN_SEL, BTN_ESC};
// }

// ---- (B) ENCODER : HAS_ENCODER -------------------------------------------------------
//
//  Rotary encoder (pin_a/pin_b) driving Next/Prev, plus a Sel button and an optional
//  Esc button. Leave pin_esc at -1 when the board only has the encoder + one button.
//  If Next/Prev come out inverted on real hardware, swap pin_a and pin_b here.
//
// static DeviceEncoder encoderCfg() {
//     DeviceEncoder cfg;
//     cfg.pin_a = 4;
//     cfg.pin_b = 5;
//     cfg.pin_sel = 0;   // encoder push button
//     cfg.pin_esc = 6;   // -1 when there is no dedicated back button
//     cfg.pullup = true; // internal pull-up on pin_sel/pin_esc
//     return cfg;
// }

// ---- (C) TOUCH : HAS_TOUCH + one TOUCH_CTRL_* ----------------------------------------
//
//  MirrorX / MirrorY / SwapXY are indexed by the DISPLAY ROTATION (0-3), so the same
//  board behaves correctly in every orientation. Don't try to derive them: copy them
//  from an already-working board with the same panel, or find them by touching the four
//  corners on real hardware.
//
//  Bus fields: pin_sda/pin_scl/pin_rst/pin_irq for the I2C controllers (GT911, CST8xx,
//  FT6x36, GT9895, HI8561); i2c_bus = &Wire1 when the panel is not on the global Wire.
//  TOUCH_CTRL_XPT2046 (resistive) is the odd one out -- its pins come from the
//  CYD28_TouchR_* -D flags in the .ini, not from these fields.
//
// static DeviceTouch touchCfg() {
//     DeviceTouch cfg;
//     cfg.pin_sda = 8;
//     cfg.pin_scl = 18;
//     cfg.pin_rst = -1;
//     cfg.pin_irq = -1;
//     //                        rotation: 0      1      2      3
//     const bool swapXY[4] =  {  true, false,  true, false };
//     const bool mirrorX[4] = {  true, false, false,  true };
//     const bool mirrorY[4] = { false, false,  true,  true };
//     for (int i = 0; i < 4; i++) {
//         cfg.SwapXY[i] = swapXY[i];
//         cfg.MirrorX[i] = mirrorX[i];
//         cfg.MirrorY[i] = mirrorY[i];
//     }
//     // cfg.i2c_bus = &Wire1;  // when the panel is not on the global Wire
//     // cfg.raw_width = 1060;  // TOUCH_CTRL_GT9895 only: the chip does not report
//     // cfg.raw_height = 2400; // its own digitizer resolution
//     // cfg.cst8xx_model = 1;  // TOUCH_CTRL_CST8XX only: skip family auto-probing
//     return cfg;
// }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device -- pins, input HAL, power ICs.
**                Runs before the display and before the filesystem.
***************************************************************************************/
void _setup_gpio() {
    // ---------------------------------------------------------------------------------
    //  1. bruceConfigPins -- runtime-configurable pin defaults
    //
    //  These are only DEFAULTS: on every boot after the first, the values saved in
    //  /brucePins.conf win, and the user can change them from Config > Set Device pins.
    //  Leave a peripheral out entirely (or use GPIO_NUM_NC) when the board doesn't have
    //  it -- the corresponding menu entry then simply finds no hardware.
    // ---------------------------------------------------------------------------------

    // -- Buses --
    // I2C. `i2c_bus` is the user-facing/Grove bus (external modules); `sys_i2c` is the
    // internal bus for soldered-down chips (PMIC, gauge, touch, RTC, IMU) and must never
    // be shut down. They may be the same physical pins -- src/core/bus_HAL.h handles it.
    // bruceConfigPins.i2c_bus = {(gpio_num_t)8, (gpio_num_t)18}; // sda, scl
    // bruceConfigPins.sys_i2c = {(gpio_num_t)8, (gpio_num_t)18}; // sda, scl
    //
    // UARTs. {rx, tx} from the ESP32's point of view.
    // bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};  // generic serial
    // bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};   // GPS module
    // bruceConfigPins.badusb_bus = {(gpio_num_t)18, (gpio_num_t)8}; // CH9329 HID bridge
    //
    // SPI. SPIPins is {sck, miso, mosi, cs, io0, io2} -- io0/io2 carry the extra signals
    // each peripheral needs (see the trailing comments). `outer_bus` is the board's
    // generic SPI bus, used by drivers that have no dedicated bus of their own.
    // bruceConfigPins.outer_bus = {(gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, GPIO_NUM_NC};
    // bruceConfigPins.SDCARD_bus = {
    //     (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)13
    // }; // sck, miso, mosi, cs
    // bruceConfigPins.CC1101_bus = {
    //     (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)12, (gpio_num_t)3, (gpio_num_t)38
    // }; // sck, miso, mosi, cs, gdo0, gdo2
    // bruceConfigPins.NRF24_bus = {
    //     (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)44, (gpio_num_t)43
    // }; // sck, miso, mosi, cs, ce
    // bruceConfigPins.PN532_bus = {
    //     (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, GPIO_NUM_NC
    // }; // sck, miso, mosi, cs
    // bruceConfigPins.ST25R_bus = {
    //     (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, GPIO_NUM_NC
    // }; // sck, miso, mosi, cs
    // #if !defined(LITE_VERSION)
    // bruceConfigPins.W5500_bus = {
    //     (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)44, (gpio_num_t)43, GPIO_NUM_NC
    // }; // sck, miso, mosi, cs, int, rst
    // bruceConfigPins.LoRa_bus = {
    //     (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)12, (gpio_num_t)3, (gpio_num_t)38
    // }; // sck, miso, mosi, cs, rst, dio0
    // #endif

    // -- Single-pin peripherals --
    // bruceConfigPins.irTx = 2;
    // bruceConfigPins.irRx = 1;
    // bruceConfigPins.rfTx = 21;   // one-pin 433MHz transmitter
    // bruceConfigPins.rfRx = 22;   // one-pin 433MHz receiver
    // bruceConfigPins.iButton = 0; // 1-Wire / iButton

    // -- Default module selection --
    // Which radio/RFID hardware the board ships with, so the user doesn't have to pick
    // it on first boot. Defaults are M5_RF_MODULE / M5_RFID2_MODULE (external modules).
    // bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    // bruceConfigPins.rfidModule = PN532_I2C_MODULE;

    // -- Display orientation --
    // Overrides the -DROTATION flag. Set it only when the value must differ from the
    // .ini (e.g. a board that is landscape for TFT_eSPI but used in portrait).
    // bruceConfigPins.rotation = 3;
    // bruceConfig.colorInverted = 1; // when the panel needs inverted colours

    // -- Audio --
    // I2S speaker, needs -DHAS_SPEAKER=1 in the .ini. {bclk, ws(LRCLK), dout, mclk};
    // leave mclk at GPIO_NUM_NC when the codec derives MCLK from BCLK.
    // bruceConfigPins.speaker_bus = {(gpio_num_t)41, (gpio_num_t)43, (gpio_num_t)42, GPIO_NUM_NC};
    //
    // Microphone, needs -DHAS_MICROPHONE=1 in the .ini. {clk, data, ws, type}:
    //   MIC_TYPE_PDM         -- PDM mic (e.g. an SPM1423 on its default wiring): clk + data, no ws
    //   MIC_TYPE_I2S_MSB     -- MSB/left-justified I2S (e.g. an SPM1423 behind an ES8311)
    //   MIC_TYPE_I2S_PHILIPS -- standard (Philips) I2S, e.g. an INMP441
    // Both I2S types take clk as BCLK.
    // bruceConfigPins.mic_bus = {(gpio_num_t)43, (gpio_num_t)46, GPIO_NUM_NC, MIC_TYPE_PDM};
    //
    // Piezo buzzer. Always compiled in, no feature gate -- it just stays silent while the
    // pin is -1. A board with HAS_SPEAKER uses the I2S speaker instead and ignores this.
    // bruceConfigPins.buzzer = 11;

    // ---------------------------------------------------------------------------------
    //  2. Raw GPIO the board needs up before anything else
    //
    //  Typical cases: a power-enable rail that feeds the display/radios, and parking
    //  every SPI chip-select HIGH so a device that boots noisy doesn't corrupt the bus
    //  while the SD card is being probed.
    // ---------------------------------------------------------------------------------
    // pinMode(PIN_POWER_ON, OUTPUT);
    // digitalWrite(PIN_POWER_ON, HIGH);
    // pinMode(TFT_CS, OUTPUT);
    // digitalWrite(TFT_CS, HIGH);
    // pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
    // digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);
    // pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    // digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);

    // ---------------------------------------------------------------------------------
    //  3. Input initialisation -- one call per input source the board has
    //
    //  Each _init() call configures the pins (pinMode etc.); the matching _poll() in
    //  InputHandler() below does the reading. A board with buttons and a touchscreen
    //  (and/or a keyboard, and/or an encoder) inits each of them here and polls each
    //  of them there -- only the button layouts are mutually exclusive.
    // ---------------------------------------------------------------------------------
    // (A) Buttons -- `count` MUST match the hal_buttons_poll_N() called in InputHandler:
    // hal_buttons_init(buttonsCfg(), 6);
    //
    //     HAS_2_BUTTONS is the exception: it uses the ESP-IDF button component, so it has
    //     its own init, taking the long-press threshold in ms (default 600):
    // hal_buttons_init_2(buttonsCfg(), 600);
    //
    // (B) Encoder -- `mode` is how the encoder's quadrature output latches:
    //     TWO03 (the default) fits most cheap EC11-style encoders; FOUR3/FOUR0 exist for
    //     encoders that produce one detent per 4 transitions. If every other detent is
    //     ignored, or one detent moves two menu items, this is the knob to turn.
    // hal_encoder_init(encoderCfg(), EncoderLatchMode::TWO03);
    //
    // (C) Touch -- hal_touch_init(cfg, i2c_addr = 0x5D, xpt_shared_spi = true).
    //     Only do it here when the controller is on its own I2C bus; a panel sharing the
    //     display's SPI/I2C must wait for _post_setup_gpio() (see below).

    // ---------------------------------------------------------------------------------
    //  4. Power ICs (PMIC / fuel gauge)
    //
    //  Only the BQ25896 (-DPMIC_BQ25896) and the BQ27220 (-DGAUGE_BQ27220) are covered
    //  by the shared HAL. A board with any other charger/gauge (AXP2101, AXP192, SY6970,
    //  MAX17048, ...) drives it directly here and overrides getBattery()/isCharging()
    //  below instead -- don't try to force it through DevicePmic/DeviceGauge.
    //
    //  Wire must already be running on the sys_i2c pins before hal_pmic_init().
    //  setSysI2CBus() is only needed when sys_i2c is NOT on the default &Wire1.
    // ---------------------------------------------------------------------------------
    // setSysI2CBus(&Wire);
    // Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
    //
    // DevicePmic pmicCfg;
    // pmicCfg.pin_sda = bruceConfigPins.sys_i2c.sda;
    // pmicCfg.pin_scl = bruceConfigPins.sys_i2c.scl; // both -1 -> driver's own default init
    // pmicCfg.address = 0x6B;                        // BQ25896
    // pmicCfg.charge_target_mv = 4208;               // optional, this is the default
    // pmicCfg.charge_current_ma = 832;               // optional, this is the default
    // hal_pmic_init(pmicCfg);
    //
    // DeviceGauge gaugeCfg;
    // gaugeCfg.design_capacity_mah = 1300; // battery capacity; 0 = keep whatever is programmed
    // hal_gauge_init(gaugeCfg);
    //
    //  No gauge IC? Then nothing is needed here at all: with -DANALOG_BAT_PIN in the .ini
    //  the shared getBattery() reads the voltage divider through hal_gauge_get_percent(),
    //  tuned by ANALOG_BAT_MULTIPLIER / ANALOG_BAT_MIN_MV / ANALOG_BAT_MAX_MV.

    // ---------------------------------------------------------------------------------
    //  5. Backlight
    //
    //  hal_bright_attach() binds the pin(s) to ledc (5kHz, 8-bit); hal_bright_set()
    //  writes a gamma-corrected duty so every board dims with the same feel.
    //  Do it here only if the display is already usable at this point -- most boards
    //  move both calls to _post_setup_gpio(), which runs after the TFT is initialised.
    // ---------------------------------------------------------------------------------
    // hal_bright_attach(TFT_BL);
    // hal_bright_set(TFT_BL, 100);
}

/***************************************************************************************
** Function name: _pre_storage_gpio()
** Location: main.cpp
** Description:   runs after the first TFT use and before LittleFS/SD are mounted.
**                Only needed by boards whose storage needs a bus/expander fix-up that
**                must not happen before the display is alive (see m5stack-cores3).
***************************************************************************************/
void _pre_storage_gpio() {}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage: display and storage are up. The backlight belongs here on
**                most boards, and so does a touch controller sharing the display bus.
***************************************************************************************/
void _post_setup_gpio() {
    // Touchscreen on the display's SPI bus (TOUCH_CTRL_XPT2046) -- the third argument
    // says "reuse the display's SPI" instead of bit-banging its own:
    // if (!hal_touch_init(touchCfg(), 0, true)) Serial.println("Touch IC not started");
    //
    // I2C touch controller -- the second argument is the chip's I2C address:
    // hal_touch_init(touchCfg(), 0x5D); // GT911; CST8xx is usually 0x15, FT6x36 0x38

    // Backlight. main.cpp calls setBrightness(bruceConfig.bright) right after this
    // function returns, so the 100% written here is only the value used while booting.
    // hal_bright_attach(TFT_BL);
    // hal_bright_set(TFT_BL, 100);
}

/***************************************************************************************
** Function name: _late_setup_gpio()
** Location: main.cpp
** Description:   runs after the input task has been created. Rarely needed.
***************************************************************************************/
void _late_setup_gpio() {}

/*********************************************************************
** Function: _setBrightness
** location: settings.cpp
** Applies a 0-100 brightness value.
**
** hal_bright_set() maps percent -> duty with a shared gamma curve, and retries once if
** the ledc write fails. Pass an array + count for a board with two backlight rails
** driven together (screen + keyboard, warm + cool):
**     static const uint8_t bl[] = {TFT_BL, KB_BL};
**     hal_bright_set(bl, 2, brightval);
**
** Boards that don't dim through a raw PWM pin do NOT use this HAL: they call whatever
** their own stack exposes instead (M5.Display.setBrightness(), an AXP192 ScreenBreath()
** register write, a panel-controller command), and e-paper boards leave this empty.
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // hal_bright_set(TFT_BL, brightval);
}

/*********************************************************************
** Function: InputHandler
** location: main.cpp (taskInputHandler)
** Sets the input globals: PrevPress, NextPress, UpPress, DownPress, SelPress, EscPress,
** AnyKeyPress and touchPoint.
**
** This is the one function with no weak default -- every board must define it.
**
** It runs in its own FreeRTOS task, roughly every 10ms. The task already does the
** bookkeeping around it, so this function must NOT:
**   - clear the input globals (the task clears all of them before each call),
**   - take the input lock (the task holds it across the call),
**   - call checkPowerSaveTime() (the task calls it every cycle),
**   - block or delay for long -- the whole UI waits on this task.
**
** The hal_*_poll() helpers set the globals directly, so a board with a single input
** source is a one-liner. A board with several just calls several polls.
**********************************************************************/
void InputHandler(void) {
    // (A) Buttons -- the N must match the count passed to hal_buttons_init():
    // hal_buttons_poll_1(buttonsCfg());
    // hal_buttons_poll_2();             // HAS_2_BUTTONS: takes no cfg
    // hal_buttons_poll_3(buttonsCfg());
    // hal_buttons_poll_5(buttonsCfg());
    // hal_buttons_poll_6(buttonsCfg()); // pass true to also map Prev+Next to Esc

    // (B) Encoder -- rotation feeds Next/Prev, pin_sel/pin_esc feed Sel/Esc:
    // hal_encoder_poll(encoderCfg());

    // (C) Touch. Reading a touch panel over I2C/SPI is slow compared to a digitalRead,
    // so rate-limit it to ~5Hz; LongPress bypasses the limit so drag/hold stays smooth.
    // hal_touch_apply() publishes the point to touchPoint/touchHeatMap() and wakes the
    // screen, swallowing the press that did the waking (the same convention every other
    // input source follows); it returns false when there is nothing more to do.
    //
    // static unsigned long tm = 0;
    // if (millis() - tm > 200 || LongPress) {
    //     BruceTouchPoint t;
    //     if (hal_touch_read(touchCfg(), t)) {
    //         tm = millis();
    //         hal_touch_apply(t);
    //     }
    // }

    // (D) Anything read by hand (a keyboard matrix, an IO expander, a trackball) sets
    // the globals itself. wakeUpScreen() returns true when the press was consumed to
    // wake the display, in which case it must not also count as a navigation press:
    //
    // if (digitalRead(BTN_SEL) == LOW) {
    //     if (!wakeUpScreen()) {
    //         AnyKeyPress = true;
    //         SelPress = true;
    //     }
    // }
}

/*********************************************************************
** Function: getBattery
** location: core/utils.cpp (weak)
** Returns the battery charge, 0-100.
**
** Only define this for a battery IC the HAL doesn't cover. With -DANALOG_BAT_PIN or
** -DGAUGE_BQ27220 the shared implementation already does the right thing, and a board
** with no battery at all can leave it out too (it then returns 0).
**********************************************************************/
// int getBattery() { return 0; }

/*********************************************************************
** Function: isCharging
** location: core/display.h (weak)
** True while the battery is charging -- drives the status-bar icon.
** With GAUGE_BQ27220:              return hal_gauge_is_charging();
** With PMIC_BQ25896 and no gauge:  return hal_pmic_is_charging();
**********************************************************************/
// bool isCharging() { return false; }

/*********************************************************************
** Function: powerOff
** location: core/mykeyboard.cpp (weak)
** Turns the device off, or gets as close as the hardware allows.
** The default just shows "Not available".
**********************************************************************/
void powerOff() {
    // With a PMIC that can cut its own rails:
    // hal_pmic_shutdown();
    //
    // Otherwise, deep sleep with a wake-up button:
    // hal_bright_set(TFT_BL, 0);
    // esp_sleep_enable_ext0_wakeup((gpio_num_t)DEEPSLEEP_WAKEUP_PIN, DEEPSLEEP_PIN_ACT);
    // esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: core/mykeyboard.cpp (weak)
** Called from the main loop. Boards use it for a "hold a button to power off/restart"
** gesture -- draw the countdown, then call powerOff()/ESP.restart(). Leave it empty
** when the board has no such gesture.
**********************************************************************/
void checkReboot() {}
