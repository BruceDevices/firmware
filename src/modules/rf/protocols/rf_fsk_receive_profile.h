// SPDX-License-Identifier: AGPL-3.0-or-later
// Receive register values derived from Flipper Zero firmware (GPL-3.0-or-later),
// Copyright Flipper Devices Inc. and contributors. See THIRD_PARTY.md.
// Reference: 3fb1d51727733d2007abc5ec636a89328857803e,
// lib/subghz/devices/cc1101_configs.c (2FSK asynchronous presets).
#pragma once
#include <cstdint>
#include <cstring>

enum class RfFskReceiveProfileResult { NotFsk, Applied, Failed };

// Caller must put the radio in IDLE first and enter RX only after success.
// This is deliberately RX-only: the legacy shared preset table also drives TX.
// Frequency, board calibration and PA settings remain owned by the radio driver.
template <typename WriteRegister, typename ReadRegister>
RfFskReceiveProfileResult rf_apply_fsk_receive_profile(
    const char *name, WriteRegister writeRegister, ReadRegister readRegister
) {
    const bool wide = name && std::strcmp(name, "2FSKDev476Async") == 0;
    const bool narrow = name && std::strcmp(name, "2FSKDev238Async") == 0;
    if (!wide && !narrow) return RfFskReceiveProfileResult::NotFsk;

    struct Register { uint8_t address, value; };
    const Register registers[] = {
        {0x02, 0x0D}, // IOCFG0: asynchronous data output
        {0x0B, 0x06}, // FSCTRL1: intermediate frequency
        {0x08, 0x32}, // PKTCTRL0: asynchronous, continuous
        {0x07, 0x04}, // PKTCTRL1
        {0x14, 0x00}, // MDMCFG0
        {0x13, 0x02}, // MDMCFG1
        {0x12, 0x04}, // MDMCFG2: 2-FSK
        {0x11, 0x83}, // MDMCFG3: 4.79794 kBaud
        {0x10, 0x67}, // MDMCFG4: 270.833333 kHz receive bandwidth
        {0x15, uint8_t(wide ? 0x47 : 0x04)}, // DEVIATN: 47.60742 / 2.380371 kHz
        {0x18, 0x18}, // MCSM0: calibrate when entering RX
        {0x19, 0x16}, // FOCCFG: FSK frequency compensation
        {0x1D, 0x91}, // AGCCTRL0: restore FSK gain control after OOK setup
        {0x1C, 0x00}, // AGCCTRL1
        {0x1B, 0x07}, // AGCCTRL2
        {0x20, 0xFB}, // WORCTRL
        {0x22, 0x10}, // FREND0
        {0x21, 0x56}, // FREND1: FSK receive front end
    };
    for (const auto &reg : registers)
        if (!writeRegister(reg.address, reg.value)) return RfFskReceiveProfileResult::Failed;
    for (const auto &reg : registers)
        if (readRegister(reg.address) != reg.value) return RfFskReceiveProfileResult::Failed;
    return RfFskReceiveProfileResult::Applied;
}
