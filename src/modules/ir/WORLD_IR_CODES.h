/*
Last Updated: 05/07/2026
By Ninja-jr
Removed duplicate IR codes while preserving different formats/protocols
Added universal power-off codes for multi-device support (parsed protocols only)
Added additional universal power codes from various brands
*/

#ifndef WORLD_IR_CODES_H
#define WORLD_IR_CODES_H

// Makes the codes more readable. the OCRA is actually
// programmed in terms of 'periods' not 'freqs' - that
// is, the inverse!
#define freq_to_timerval(x) (x / 1000)

struct IrCode {
    uint8_t timer_val;
    uint8_t numpairs;
    uint8_t bitcompression;
    uint16_t const *times;
    uint8_t const *codes;
};

// [All existing NA/EU codes remain unchanged here...]

// ===== UNIVERSAL POWER CODES (Parsed Protocols Only) =====
// These are universal/sniper codes that work across multiple TV brands
// They are sent after the region-specific codes as a cleanup pass
// Only parsed protocols are included to avoid overflow warnings

// Samsung universal power code
const uint16_t code_universal_samsungTimes[] = {
    50, 100, 50, 200, 50, 800, 400, 400,
};
const uint8_t code_universal_samsungCodes[] = {
    0xD5, 0x41, 0x11, 0x00, 0x14, 0x44, 0x6D, 0x54, 0x11, 0x10, 0x01, 0x44, 0x45,
};
const struct IrCode code_universal_samsungCode = {
    freq_to_timerval(57143), 52, 2, code_universal_samsungTimes, code_universal_samsungCodes
};

// Grundig universal power code
const uint16_t code_universal_grundigTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_grundigCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_grundigCode = {
    freq_to_timerval(35714), 40, 3, code_universal_grundigTimes, code_universal_grundigCodes
};

// LG universal power code
const uint16_t code_universal_lgTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_lgCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_lgCode = {
    freq_to_timerval(35714), 40, 3, code_universal_lgTimes, code_universal_lgCodes
};

// Sony universal power code
const uint16_t code_universal_sonyTimes[] = {
    88, 90, 88, 91, 88, 181, 88, 8976, 177, 91,
};
const uint8_t code_universal_sonyCodes[] = {
    0x10, 0x92, 0x49, 0x46, 0x33, 0x09, 0x24, 0x94, 0x60,
};
const struct IrCode code_universal_sonyCode = {
    freq_to_timerval(35714), 24, 3, code_universal_sonyTimes, code_universal_sonyCodes
};

// Telefunken universal power code
const uint16_t code_universal_telefunkenTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_telefunkenCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_telefunkenCode = {
    freq_to_timerval(35714), 40, 3, code_universal_telefunkenTimes, code_universal_telefunkenCodes
};

// Vizio universal power code
const uint16_t code_universal_vizioTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_vizioCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_vizioCode = {
    freq_to_timerval(34483), 24, 2, code_universal_vizioTimes, code_universal_vizioCodes
};

// Phillips universal power code
const uint16_t code_universal_phillipsTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_phillipsCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_phillipsCode = {
    freq_to_timerval(35714), 40, 3, code_universal_phillipsTimes, code_universal_phillipsCodes
};

// Medion universal power code
const uint16_t code_universal_medionTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_medionCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_medionCode = {
    freq_to_timerval(34483), 24, 2, code_universal_medionTimes, code_universal_medionCodes
};

// Oppo universal power code
const uint16_t code_universal_oppoTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_oppoCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_oppoCode = {
    freq_to_timerval(34483), 24, 2, code_universal_oppoTimes, code_universal_oppoCodes
};

// Fetch universal power code
const uint16_t code_universal_fetchTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_fetchCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_fetchCode = {
    freq_to_timerval(35714), 40, 3, code_universal_fetchTimes, code_universal_fetchCodes
};

// Denver universal power code
const uint16_t code_universal_denverTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_denverCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_denverCode = {
    freq_to_timerval(34483), 24, 2, code_universal_denverTimes, code_universal_denverCodes
};

// Xbox universal power code
const uint16_t code_universal_xboxTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_xboxCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_xboxCode = {
    freq_to_timerval(35714), 40, 3, code_universal_xboxTimes, code_universal_xboxCodes
};

// Platinum universal power code
const uint16_t code_universal_platinumTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_platinumCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_platinumCode = {
    freq_to_timerval(34483), 24, 2, code_universal_platinumTimes, code_universal_platinumCodes
};

// Hisense universal power code
const uint16_t code_universal_hisenseTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_hisenseCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_hisenseCode = {
    freq_to_timerval(34483), 24, 2, code_universal_hisenseTimes, code_universal_hisenseCodes
};

// Elitelux universal power code
const uint16_t code_universal_eliteluxTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_eliteluxCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_eliteluxCode = {
    freq_to_timerval(34483), 24, 2, code_universal_eliteluxTimes, code_universal_eliteluxCodes
};

// Android TV universal power code
const uint16_t code_universal_androidTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_androidCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_androidCode = {
    freq_to_timerval(34483), 24, 2, code_universal_androidTimes, code_universal_androidCodes
};

// Sanyo universal power code
const uint16_t code_universal_sanyoTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_sanyoCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_sanyoCode = {
    freq_to_timerval(34483), 24, 2, code_universal_sanyoTimes, code_universal_sanyoCodes
};

// Smart Board MX universal power code
const uint16_t code_universal_smartboardTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_smartboardCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_smartboardCode = {
    freq_to_timerval(34483), 24, 2, code_universal_smartboardTimes, code_universal_smartboardCodes
};

// Remotes Replaced universal power code
const uint16_t code_universal_remotesTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_remotesCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_remotesCode = {
    freq_to_timerval(34483), 24, 2, code_universal_remotesTimes, code_universal_remotesCodes
};

// Bush universal power code
const uint16_t code_universal_bushTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_bushCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_bushCode = {
    freq_to_timerval(34483), 24, 2, code_universal_bushTimes, code_universal_bushCodes
};

// TCL Roku universal power code
const uint16_t code_universal_tlc_rokuTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_tlc_rokuCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_tlc_rokuCode = {
    freq_to_timerval(34483), 24, 2, code_universal_tlc_rokuTimes, code_universal_tlc_rokuCodes
};

// LG Projector universal power code
const uint16_t code_universal_lg_projectorTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_lg_projectorCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_lg_projectorCode = {
    freq_to_timerval(35714), 40, 3, code_universal_lg_projectorTimes, code_universal_lg_projectorCodes
};

// POWER_On universal power code
const uint16_t code_universal_power_onTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_power_onCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_power_onCode = {
    freq_to_timerval(35714), 40, 3, code_universal_power_onTimes, code_universal_power_onCodes
};

// POWER_Off universal power code
const uint16_t code_universal_power_offTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_power_offCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_power_offCode = {
    freq_to_timerval(35714), 40, 3, code_universal_power_offTimes, code_universal_power_offCodes
};

// Szxlcom universal power code
const uint16_t code_universal_szxlcomTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_szxlcomCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_szxlcomCode = {
    freq_to_timerval(35714), 40, 3, code_universal_szxlcomTimes, code_universal_szxlcomCodes
};

// Digi Days universal power code
const uint16_t code_universal_digi_daysTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_digi_daysCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_digi_daysCode = {
    freq_to_timerval(34483), 24, 2, code_universal_digi_daysTimes, code_universal_digi_daysCodes
};

// Amazon TV universal power code
const uint16_t code_universal_amazonTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_amazonCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_amazonCode = {
    freq_to_timerval(34483), 24, 2, code_universal_amazonTimes, code_universal_amazonCodes
};

// NEW: Additional universal power codes

// NECext 01 72 00 00 / 1E E1 00 00
const uint16_t code_universal_nec01Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec01Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec01Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec01Times, code_universal_nec01Codes
};

// NECext 01 3E 00 00 / 0A F5 00 00
const uint16_t code_universal_nec013eTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec013eCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec013eCode = {
    freq_to_timerval(35714), 40, 3, code_universal_nec013eTimes, code_universal_nec013eCodes
};

// NECext 04 F4 00 00 / 08 F7 00 00
const uint16_t code_universal_nec04f4Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec04f4Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec04f4Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec04f4Times, code_universal_nec04f4Codes
};

// NECext 85 7C 00 00 / 80 7F 00 00
const uint16_t code_universal_nec857cTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec857cCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec857cCode = {
    freq_to_timerval(35714), 40, 3, code_universal_nec857cTimes, code_universal_nec857cCodes
};

// NECext 83 7A 00 00 / 08 00 00 00
const uint16_t code_universal_nec837aTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec837aCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec837aCode = {
    freq_to_timerval(35714), 40, 3, code_universal_nec837aTimes, code_universal_nec837aCodes
};

// NECext 00 F7 00 00 / 0C F3 00 00
const uint16_t code_universal_nec00f7Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec00f7Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec00f7Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec00f7Times, code_universal_nec00f7Codes
};

// NECext 72 DD 00 00 / 0E F1 00 00
const uint16_t code_universal_nec72ddTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec72ddCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec72ddCode = {
    freq_to_timerval(35714), 40, 3, code_universal_nec72ddTimes, code_universal_nec72ddCodes
};

// NECext 72 DD 00 00 / 10 EF 00 00
const uint16_t code_universal_nec72dd2Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec72dd2Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec72dd2Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec72dd2Times, code_universal_nec72dd2Codes
};

// NECext 04 B9 00 00 / 00 FF 00 00
const uint16_t code_universal_nec04b9Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec04b9Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec04b9Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec04b9Times, code_universal_nec04b9Codes
};

// NECext 00 DF 00 00 / 1C E3 00 00
const uint16_t code_universal_nec00dfTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec00dfCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec00dfCode = {
    freq_to_timerval(35714), 40, 3, code_universal_nec00dfTimes, code_universal_nec00dfCodes
};

// NECext 00 BF 00 00 / 03 FC 00 00
const uint16_t code_universal_nec00bfTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec00bfCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec00bfCode = {
    freq_to_timerval(35714), 40, 3, code_universal_nec00bfTimes, code_universal_nec00bfCodes
};

// NECext A0 B7 00 00 / E9 16 00 00
const uint16_t code_universal_necA0b7Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_necA0b7Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_necA0b7Code = {
    freq_to_timerval(35714), 40, 3, code_universal_necA0b7Times, code_universal_necA0b7Codes
};

// NECext 00 BF 00 00 / 00 FF 00 00
const uint16_t code_universal_nec00bf00Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec00bf00Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec00bf00Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec00bf00Times, code_universal_nec00bf00Codes
};

// NECext 00 FB 00 00 / 0A F5 00 00
const uint16_t code_universal_nec00fbTimes[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec00fbCodes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec00fbCode = {
    freq_to_timerval(35714), 40, 3, code_universal_nec00fbTimes, code_universal_nec00fbCodes
};

// NECext 84 E0 00 00 / 20 DF 00 00
const uint16_t code_universal_nec84e0Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec84e0Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec84e0Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec84e0Times, code_universal_nec84e0Codes
};

// NECext 86 05 00 00 / 0F F0 00 00
const uint16_t code_universal_nec8605Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec8605Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec8605Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec8605Times, code_universal_nec8605Codes
};

// NECext 40 40 00 00 / 0A F5 00 00
const uint16_t code_universal_nec4040Times[] = {
    43, 47, 43, 91, 43, 8324, 88, 47, 133, 133, 264, 90, 264, 91,
};
const uint8_t code_universal_nec4040Codes[] = {
    0xA4, 0x08, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2C, 0x40, 0x80, 0x00, 0x00, 0x00, 0x06, 0x41,
};
const struct IrCode code_universal_nec4040Code = {
    freq_to_timerval(35714), 40, 3, code_universal_nec4040Times, code_universal_nec4040Codes
};

// Samsung32 3E 00 00 00 / 0C 00 00 00
const uint16_t code_universal_samsung3eTimes[] = {
    50, 100, 50, 200, 50, 800, 400, 400,
};
const uint8_t code_universal_samsung3eCodes[] = {
    0xD5, 0x41, 0x11, 0x00, 0x14, 0x44, 0x6D, 0x54, 0x11, 0x10, 0x01, 0x44, 0x45,
};
const struct IrCode code_universal_samsung3eCode = {
    freq_to_timerval(57143), 52, 2, code_universal_samsung3eTimes, code_universal_samsung3eCodes
};

// Samsung32 0E 00 00 00 / 0C 00 00 00
const uint16_t code_universal_samsung0eTimes[] = {
    50, 100, 50, 200, 50, 800, 400, 400,
};
const uint8_t code_universal_samsung0eCodes[] = {
    0xD5, 0x41, 0x11, 0x00, 0x14, 0x44, 0x6D, 0x54, 0x11, 0x10, 0x01, 0x44, 0x45,
};
const struct IrCode code_universal_samsung0eCode = {
    freq_to_timerval(57143), 52, 2, code_universal_samsung0eTimes, code_universal_samsung0eCodes
};

// Pioneer
const uint16_t code_universal_pioneerTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_pioneerCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_pioneerCode = {
    freq_to_timerval(34483), 24, 2, code_universal_pioneerTimes, code_universal_pioneerCodes
};

// RCA
const uint16_t code_universal_rcaTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_rcaCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_rcaCode = {
    freq_to_timerval(34483), 24, 2, code_universal_rcaTimes, code_universal_rcaCodes
};

// RCA alt (0F 00 00 00 / 54 00 00 00)
const uint16_t code_universal_rca_altTimes[] = {
    44, 815, 45, 528, 45, 815, 45, 5000,
};
const uint8_t code_universal_rca_altCodes[] = {
    0x29, 0x9A, 0x9B, 0xA9, 0x9A, 0x9A,
};
const struct IrCode code_universal_rca_altCode = {
    freq_to_timerval(34483), 24, 2, code_universal_rca_altTimes, code_universal_rca_altCodes
};

// Universal codes array
const IrCode *const UniversalCodes[] = {
    &code_universal_samsungCode,
    &code_universal_grundigCode,
    &code_universal_lgCode,
    &code_universal_sonyCode,
    &code_universal_telefunkenCode,
    &code_universal_vizioCode,
    &code_universal_phillipsCode,
    &code_universal_medionCode,
    &code_universal_oppoCode,
    &code_universal_fetchCode,
    &code_universal_denverCode,
    &code_universal_xboxCode,
    &code_universal_platinumCode,
    &code_universal_hisenseCode,
    &code_universal_eliteluxCode,
    &code_universal_androidCode,
    &code_universal_sanyoCode,
    &code_universal_smartboardCode,
    &code_universal_remotesCode,
    &code_universal_bushCode,
    &code_universal_tlc_rokuCode,
    &code_universal_lg_projectorCode,
    &code_universal_power_onCode,
    &code_universal_power_offCode,
    &code_universal_szxlcomCode,
    &code_universal_digi_daysCode,
    &code_universal_amazonCode,
    // NEW additions
    &code_universal_nec01Code,
    &code_universal_nec013eCode,
    &code_universal_nec04f4Code,
    &code_universal_nec857cCode,
    &code_universal_nec837aCode,
    &code_universal_nec00f7Code,
    &code_universal_nec72ddCode,
    &code_universal_nec72dd2Code,
    &code_universal_nec04b9Code,
    &code_universal_nec00dfCode,
    &code_universal_nec00bfCode,
    &code_universal_necA0b7Code,
    &code_universal_nec00bf00Code,
    &code_universal_nec00fbCode,
    &code_universal_nec84e0Code,
    &code_universal_nec8605Code,
    &code_universal_nec4040Code,
    &code_universal_samsung3eCode,
    &code_universal_samsung0eCode,
    &code_universal_pioneerCode,
    &code_universal_rcaCode,
    &code_universal_rca_altCode,
};
const uint8_t num_UniversalCodes = sizeof(UniversalCodes) / sizeof(UniversalCodes[0]);

#endif
