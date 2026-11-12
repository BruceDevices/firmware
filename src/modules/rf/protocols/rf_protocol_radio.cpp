// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../rf_utils.h"
#include "rf_encoder.h"
#include "rf_presets.h"
#include "rf_protocol_engine.h"
#include <cerrno>
#include <sstream>

bool rf_protocol_transmit_raw(const char *input) {
    if (!input || strlen(input) > 262144) return false;
    std::istringstream file(input);
    std::string line, preset, protocol;
    uint32_t frequency = 0;
    std::vector<int> pulses;
    uint64_t total = 0;
    bool gotPreset = false, gotFrequency = false, gotProtocol = false;
    while (std::getline(file, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon), value = line.substr(colon + 1);
        const auto first = value.find_first_not_of(" \t\r");
        value = first == std::string::npos ? "" : value.substr(first);
        const auto last = value.find_last_not_of(" \t\r");
        if (last != std::string::npos) value.resize(last + 1);
        if (key == "Frequency") {
            if (gotFrequency || value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
                return false;
            errno = 0;
            char *end;
            unsigned long parsed = strtoul(value.c_str(), &end, 10);
            if (errno || *end || parsed > UINT32_MAX) return false;
            frequency = parsed;
            gotFrequency = true;
        } else if (key == "Preset") {
            if (gotPreset) return false;
            preset = value;
            gotPreset = true;
        } else if (key == "Protocol") {
            if (gotProtocol) return false;
            protocol = value;
            gotProtocol = true;
        } else if (key == "Bruce_Buffer_Full" && value != "0") return false;
        else if (key == "RAW_Data" || key == "Data_RAW") {
            std::istringstream values(value);
            std::string token;
            while (values >> token) {
                errno = 0;
                char *end;
                long v = strtol(token.c_str(), &end, 10);
                if (errno || *end || end == token.c_str() || !v || v < -1000000 || v > 1000000 ||
                    pulses.size() >= 16384)
                    return false;
                total += v > 0 ? v : -v;
                if (total > 10000000) return false;
                pulses.push_back(int(v));
            }
        }
    }
    const RfPreset *rp = rf_find_preset(String(preset.c_str()));
    bool validFrequency = (frequency >= 300000000 && frequency <= 348000000) ||
                          (frequency >= 387000000 && frequency <= 464000000) ||
                          (frequency >= 779000000 && frequency <= 928000000);
    if (!rp || !validFrequency || protocol != "RAW" || pulses.empty() ||
        bruceConfigPins.rfModule != CC1101_SPI_MODULE)
        return false;
    if (!initRfModule("tx", frequency / 1000000.0f)) return false;
    ELECHOUSE_cc1101.setSidle();
    ELECHOUSE_cc1101.setModulation(rp->modulation);
    ELECHOUSE_cc1101.setDeviation(rp->deviation ? rp->deviation : 1.58f);
    ELECHOUSE_cc1101.setRxBW(rp->rxBW ? rp->rxBW : 270.83f);
    ELECHOUSE_cc1101.setDRate(rp->dataRate ? rp->dataRate : 10.f);
    ELECHOUSE_cc1101.setPktFormat(3);
    pinMode(bruceConfigPins.CC1101_bus.io0, OUTPUT);
    ELECHOUSE_cc1101.SetTx();
    bool success = rf_tx_durations(pulses);
    deinitRfModule();
    return success;
}
