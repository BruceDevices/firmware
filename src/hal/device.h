#ifndef BRUCE_HAL_DEVICE_H
#define BRUCE_HAL_DEVICE_H

#include <cstdint>

// Pure data describing a board's input/power wiring. Filled by each
// board's _setup_gpio() and handed to the hal_* modules; no logic here.

struct DeviceButtons {
    int8_t btn1 = -1;
    int8_t btn2 = -1;
    int8_t btn3 = -1;
    int8_t btn4 = -1;
    int8_t btn5 = -1;
    int8_t btn6 = -1;
    bool pullup = true;      // false for boards without internal/external pull-ups (e.g. m5stack-cplus2)
    bool activeHigh = false; // true for boards wired active-HIGH (e.g. seeedstudio-reterminal-d1001)
};

struct DeviceTouch {
    int8_t pin_sda = -1;
    int8_t pin_scl = -1;
    int8_t pin_rst = -1;
    int8_t pin_irq = -1;
    bool MirrorX[4] = {false, false, false, false};
    bool MirrorY[4] = {false, false, false, false};
    bool SwapXY[4] = {false, false, false, false};
    int16_t HomeBtn = -1;
    void *i2c_bus = nullptr;
    // TOUCH_CTRL_XPT2046: SPIClass* to run the touch on when it is neither on the display bus nor
    // bit-banged (e.g. one returned by acquireSPIBus()). Used when xpt_shared_spi is false.
    void *spi_bus = nullptr;
    uint16_t raw_width = 0;
    uint16_t raw_height = 0;
    bool gt911_int_sync = false;
    // Drives the touch controller's RST line on boards where it isn't a raw
    // ESP32 GPIO (e.g. behind an IO expander like M5IOE1) -- called with
    // HIGH/LOW instead of the pin_rst GPIO writes hal_touch_init would
    // otherwise do. Leave pin_rst at -1 when this is set; hal_touch_init
    // pulses low then high through the callback before the chip driver's
    // begin(), and passes -1 as its own pin_rst so it never touches a raw
    // GPIO itself.
    void (*reset_cb)(bool level) = nullptr;
    // TOUCH_CTRL_CST8XX only: the shared driver (SensorLib's TouchDrvCSTXXX)
    // auto-probes chip families in this fixed order: 0=CST226, 1=CST8XX
    // (CST816/CST820/CST716), 2=CST92xx, 3=CST3530. Fine for boards that
    // don't know their exact chip, but every failed candidate ahead of the
    // real one still pulses RST (with that candidate's own, possibly wrong,
    // timing) and writes/reads its own register map on the real chip before
    // the correct driver ever runs -- which can leave it reporting a
    // stuck/phantom touch. Set this to the real chip's index (see above) on
    // boards that already know it, to skip straight to that driver. -1 (the
    // default) leaves auto-probing enabled.
    int8_t cst8xx_model = -1;
};

struct DeviceEncoder {
    int8_t pin_a = -1;
    int8_t pin_b = -1;
    int8_t pin_sel = -1;
    int8_t pin_esc = -1; // -1 if there's no dedicated esc button (only the encoder + pin_sel)
    bool pullup = false; // internal pull-up on pin_sel/pin_esc
};

struct DevicePmic {
    int8_t pin_sda = -1; // pin_sda and pin_scl both -1 -> driver's own default I2C init (no pins)
    int8_t pin_scl = -1;
    uint8_t address = 0;
    uint16_t charge_target_mv = 4208; // charge voltage limit
    uint16_t charge_current_ma = 832; // constant-current charge limit
};

struct DeviceGauge {
    int8_t pin_sda = -1;
    int8_t pin_scl = -1;
    uint8_t address = 0;
    uint16_t design_capacity_mah = 0;
    // Analog battery reading (voltage divider on an ADC pin) -- used when no
    // GAUGE_* IC macro is set. Left at -1/0 the values come from the
    // ANALOG_BAT_PIN / ANALOG_BAT_MULTIPLIER / ANALOG_BAT_MIN_MV /
    // ANALOG_BAT_MAX_MV build flags, so a board only needs the flags.
    int8_t analog_pin = -1;
    float analog_multiplier = 0; // 0 = ANALOG_BAT_MULTIPLIER (default 2.0)
    uint16_t analog_min_mv = 0;  // 0 = ANALOG_BAT_MIN_MV (default 3300) -> 0%
    uint16_t analog_max_mv = 0;  // 0 = ANALOG_BAT_MAX_MV (default 4100) -> 100%
};

// Detailed fuel-gauge readings (GAUGE_BQ27220 only)
struct DeviceGaugeInfo {
    int remain_cap_mah = 0;
    int full_cap_mah = 0;
    int design_cap_mah = 0;
    bool charging = false;
    int charging_mv = 0;
    int charging_ma = 0;
    int time_to_empty_min = 0;
    int avg_power_mw = 0;
    int volt_mv = 0;
    int volt_raw_mv = 0;
    int curr_instant_ma = 0;
    int curr_average_ma = 0;
    int curr_raw_ma = 0;
};

#endif
