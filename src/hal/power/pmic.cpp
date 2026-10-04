#include "pmic.h"

#if defined(PMIC_BQ25896)
#define XPOWERS_CHIP_BQ25896
#include <Wire.h>
#include <XPowersLib.h>

static XPowersPPM ppm;

static void applyOperatingPoint(
    uint16_t input_current_limit_ma, uint16_t charge_target_mv, uint16_t charge_current_ma
) {
    ppm.setSysPowerDownVoltage(3300);
    ppm.setInputCurrentLimit(input_current_limit_ma);
    ppm.disableCurrentLimitPin();
    ppm.setChargeTargetVoltage(charge_target_mv);
    ppm.setPrechargeCurr(64);
    ppm.setChargerConstantCurr(charge_current_ma);
    ppm.enableMeasure();
    ppm.disableOTG();
    ppm.enableCharge();
}
#endif

bool hal_pmic_init(const DevicePmic &cfg, uint16_t input_current_limit_ma) {
#if defined(PMIC_BQ25896)
    bool ok = (cfg.pin_sda < 0 && cfg.pin_scl < 0) ? ppm.init()
                                                    : ppm.init(Wire, cfg.pin_sda, cfg.pin_scl, cfg.address);
    if (!ok) return false;
    applyOperatingPoint(input_current_limit_ma, cfg.charge_target_mv, cfg.charge_current_ma);
    return true;
#else
    (void)cfg;
    (void)input_current_limit_ma;
    return false;
#endif
}

bool hal_pmic_init_via_callbacks(
    uint8_t address, PmicI2cFptr readReg, PmicI2cFptr writeReg, uint16_t input_current_limit_ma
) {
#if defined(PMIC_BQ25896)
    if (!ppm.begin(address, readReg, writeReg)) return false;
    applyOperatingPoint(input_current_limit_ma, 4208, 832);
    return true;
#else
    (void)address;
    (void)readReg;
    (void)writeReg;
    (void)input_current_limit_ma;
    return false;
#endif
}

void hal_pmic_shutdown() {
#if defined(PMIC_BQ25896)
    ppm.shutdown();
#endif
}

int hal_pmic_get_input_current_limit_ma() {
#if defined(PMIC_BQ25896)
    return (int)ppm.getInputCurrentLimit();
#else
    return -1;
#endif
}

int hal_pmic_get_charger_constant_curr_ma() {
#if defined(PMIC_BQ25896)
    return (int)ppm.getChargerConstantCurr();
#else
    return -1;
#endif
}

int hal_pmic_get_system_voltage_mv() {
#if defined(PMIC_BQ25896)
    return (int)ppm.getSystemVoltage();
#else
    return -1;
#endif
}

int hal_pmic_get_ntc_percent() {
#if defined(PMIC_BQ25896)
    return (int)ppm.getNTCPercentage();
#else
    return -1;
#endif
}

void hal_pmic_enable_otg() {
#if defined(PMIC_BQ25896)
    ppm.enableOTG();
#endif
}

void hal_pmic_disable_otg() {
#if defined(PMIC_BQ25896)
    ppm.disableOTG();
#endif
}

bool hal_pmic_is_charging() {
#if defined(PMIC_BQ25896)
    return ppm.isCharging();
#else
    return false;
#endif
}

bool hal_pmic_is_charge_done() {
#if defined(PMIC_BQ25896)
    return ppm.isChargeDone();
#else
    return false;
#endif
}

int hal_pmic_get_batt_voltage_mv() {
#if defined(PMIC_BQ25896)
    return (int)ppm.getBattVoltage();
#else
    return -1;
#endif
}

void hal_pmic_disable_bat_load() {
#if defined(PMIC_BQ25896)
    ppm.disableBatLoad();
#endif
}
