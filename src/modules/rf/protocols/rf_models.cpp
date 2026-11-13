// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_models.h"

#include "../rf_utils.h"       // ELECHOUSE_cc1101, setMHZ
#include "rf_fsk_receive_profile.h"
#include "rf_presets.h"

// Raw CC1101 register dump for the Honda "custom" preset, taken verbatim from
// ProtoPirate's models.txt (Flipper CustomPresetData, registers 0x00..0x2D).
static const uint8_t kHondaRegisters[] = {
    0x02, 0x0D, 0x0B, 0x06, 0x08, 0x32, 0x07, 0x04, 0x14, 0x00, 0x13, 0x02,
    0x12, 0x07, 0x11, 0x36, 0x10, 0xE9, 0x15, 0x32, 0x18, 0x18, 0x19, 0x16,
    0x1D, 0x92, 0x1C, 0x40, 0x1B, 0x03, 0x20, 0xFB, 0x22, 0x10, 0x21, 0x56,
    0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const CarModel kModels[] = {
    {"433.92 Mhz AM650", 433920000, "Ook650Async", nullptr, 0},
    {"433.92 Mhz FM476", 433920000, "2FSKDev476Async", nullptr, 0},
    {"315 Mhz AM650", 315000000, "Ook650Async", nullptr, 0},
    {"315 Mhz FM476", 315000000, "2FSKDev476Async", nullptr, 0},
    {"Ford 433.92 Mhz AM", 433920000, "Ook650Async", nullptr, 0},
    {"Ford 433.92 Mhz FM", 433920000, "2FSKDev476Async", nullptr, 0},
    {"Honda 433.92 Mhz FM", 433920000, "Custom", kHondaRegisters, sizeof(kHondaRegisters)},
    {"Honda 315 Mhz FM", 315000000, "Custom", kHondaRegisters, sizeof(kHondaRegisters)},
    {"KIA 433.92 Mhz AM", 433920000, "Ook650Async", nullptr, 0},
    {"KIA 433.92 Mhz FM", 433920000, "2FSKDev476Async", nullptr, 0},
    {"Mitsubishi Outlander", 433920000, "2FSKDev476Async", nullptr, 0},
    {"VAG 434.42 Mhz AM", 434420000, "Ook650Async", nullptr, 0},
};

int car_model_count() { return (int)(sizeof(kModels) / sizeof(kModels[0])); }

const CarModel *car_model_at(int index) {
    if (index < 0 || index >= car_model_count()) return nullptr;
    return &kModels[index];
}

bool car_model_apply(const CarModel *model) {
    if (!model) return false;

    ELECHOUSE_cc1101.setSidle();

    if (model->registers && model->registerCount) {
        // Custom preset: write the raw register dump (0x00..0x2D), then restore
        // the frequency (the dump's FREQ bytes are overridden by the model's).
        for (uint8_t i = 0; i < model->registerCount; i++)
            ELECHOUSE_cc1101.SpiWriteReg(i, model->registers[i]);
        setMHZ(model->freqHz / 1000000.0f);
    } else {
        const RfPreset *preset = rf_find_preset(String(model->preset));
        const auto fsk = rf_apply_fsk_receive_profile(
            model->preset,
            [](uint8_t a, uint8_t v) { return ELECHOUSE_cc1101.SpiWriteReg(a, v); },
            [](uint8_t a) { return ELECHOUSE_cc1101.SpiReadReg(a); });
        if (fsk == RfFskReceiveProfileResult::NotFsk && preset) {
            ELECHOUSE_cc1101.setModulation(preset->modulation);
            ELECHOUSE_cc1101.setDeviation(preset->deviation ? preset->deviation : 1.58f);
            ELECHOUSE_cc1101.setRxBW(preset->rxBW ? preset->rxBW : 270.83f);
            ELECHOUSE_cc1101.setDRate(preset->dataRate ? preset->dataRate : 10.f);
            ELECHOUSE_cc1101.setPktFormat(3);
            setMHZ(model->freqHz / 1000000.0f);
        } else if (fsk == RfFskReceiveProfileResult::Failed) {
            return false;
        } else {
            // FSK profile applied; set frequency explicitly.
            setMHZ(model->freqHz / 1000000.0f);
        }
    }

    ELECHOUSE_cc1101.SetRx();
    return true;
}
