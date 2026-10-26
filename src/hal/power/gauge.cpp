#include "gauge.h"

#if defined(GAUGE_BQ27220)
#include <bq27220.h>

static BQ27220 gauge;
#elif defined(ANALOG_BAT_PIN)
#include <Arduino.h>

#ifndef ANALOG_BAT_MULTIPLIER
#define ANALOG_BAT_MULTIPLIER 2.0f
#endif
#ifndef ANALOG_BAT_MIN_MV
#define ANALOG_BAT_MIN_MV 3300
#endif
#ifndef ANALOG_BAT_MAX_MV
#define ANALOG_BAT_MAX_MV 4100
#endif

static int8_t analogPin = ANALOG_BAT_PIN;
static float analogMultiplier = ANALOG_BAT_MULTIPLIER;
static uint16_t analogMinMv = ANALOG_BAT_MIN_MV;
static uint16_t analogMaxMv = ANALOG_BAT_MAX_MV;
static bool analogReady = false;

static void analogSetup() {
    if (analogReady || analogPin < 0) return;
    pinMode(analogPin, INPUT);
    analogReady = true;
}
#endif

bool hal_gauge_init(const DeviceGauge &cfg) {
#if defined(GAUGE_BQ27220)
    if (cfg.design_capacity_mah == 0) return true; // don't do anything
    if (gauge.getDesignCap() != cfg.design_capacity_mah) gauge.setDesignCap(cfg.design_capacity_mah);
    return true;
#elif defined(ANALOG_BAT_PIN)
    if (cfg.analog_pin >= 0) analogPin = cfg.analog_pin;
    if (cfg.analog_multiplier > 0) analogMultiplier = cfg.analog_multiplier;
    if (cfg.analog_min_mv > 0) analogMinMv = cfg.analog_min_mv;
    if (cfg.analog_max_mv > 0) analogMaxMv = cfg.analog_max_mv;
    analogReady = false;
    analogSetup();
    return true;
#else
    (void)cfg;
    return false;
#endif
}

int hal_gauge_get_percent() {
#if defined(GAUGE_BQ27220)
    int percent = gauge.getChargePcnt();
    if (percent == 65535) return -1;
    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
#elif defined(ANALOG_BAT_PIN)
    analogSetup();
    if (analogPin < 0 || analogMaxMv <= analogMinMv) return -1;
    float voltage = (float)analogReadMilliVolts(analogPin) * analogMultiplier;
    int percent = (int)(((voltage - analogMinMv) / (float)(analogMaxMv - analogMinMv)) * 100.0f);
    return (percent < 0) ? 0 : (percent > 100) ? 100 : percent;
#else
    return -1;
#endif
}

bool hal_gauge_is_charging() {
#if defined(GAUGE_BQ27220)
    return gauge.getIsCharging();
#else
    return false;
#endif
}

bool hal_gauge_has_info() {
#if defined(GAUGE_BQ27220)
    return true;
#else
    return false;
#endif
}

bool hal_gauge_get_info(DeviceGaugeInfo &out) {
#if defined(GAUGE_BQ27220)
    out.remain_cap_mah = gauge.getRemainCap();
    out.full_cap_mah = gauge.getFullChargeCap();
    out.design_cap_mah = gauge.getDesignCap();
    out.charging = gauge.getIsCharging();
    out.charging_mv = gauge.getVolt(VOLT_MODE::VOLT_CHARGING);
    out.charging_ma = gauge.getCurr(CURR_MODE::CURR_CHARGING);
    out.time_to_empty_min = gauge.getTimeToEmpty();
    out.avg_power_mw = gauge.getAvgPower();
    out.volt_mv = gauge.getVolt(VOLT_MODE::VOLT);
    out.volt_raw_mv = gauge.getVolt(VOLT_MODE::VOLT_RWA);
    out.curr_instant_ma = gauge.getCurr(CURR_MODE::CURR_INSTANT);
    out.curr_average_ma = gauge.getCurr(CURR_MODE::CURR_AVERAGE);
    out.curr_raw_ma = gauge.getCurr(CURR_MODE::CURR_RAW);
    return true;
#else
    (void)out;
    return false;
#endif
}
