/*
Last Updated: 23 Jun. 2026
By Ninja-jr
Removed duplicate IR codes while preserving different formats/protocols

TV-B-Gone for Arduino version 0.001
Ported to Arduino by Ken Shirriff, Dec 3, 2009
http://arcfn.com

The original code is:
TV-B-Gone Firmware version 1.2
 for use with ATtiny85v and v1.2 hardware
 (c) Mitch Altman + Limor Fried 2009
*/

// Makes the codes more readable. the OCRA is actually
// programmed in terms of 'periods' not 'freqs' - that
// is, the inverse!
// #define freq_to_timerval(x) (F_CPU / 8 / x - 1)
#define freq_to_timerval(x) (x / 1000)
// Codes captured from Generation 3 TV-B-Gone by Limor Fried & Mitch Altman
// table of POWER codes

// The structure of compressed code entries
struct IrCode {
    uint8_t timer_val;
    uint8_t numpairs;
    uint8_t bitcompression;
    uint16_t const *times;
    uint8_t const *codes;
};

const uint16_t code_na000Times[] = {
    60,
    60,
    60,
    2700,
    120,
    60,
    240,
    60,
};
const uint8_t code_na000Codes[] = {
    0xE2,
    0x20,
    0x80,
    0x78,
    0x88,
    0x20,
    0x10,
};
const struct IrCode code_na000Code = {
    freq_to_timerval(38400),
    26, // # of pairs
    2,  // # of bits per index
    code_na000Times,
    code_na000Codes
};

const uint16_t code_na001Times[] = {
    50,
    100,
    50,
    200,
    50,
    800,
    400,
    400,
};
const uint8_t code_na001Codes[] = {
    0xD5,
    0x41,
    0x11,
    0x00,
    0x14,
    0x44,
    0x6D,
    0x54,
    0x11,
    0x10,
    0x01,
    0x44,
    0x45,
};
const struct IrCode code_na001Code = {
    freq_to_timerval(57143),
    52, // # of pairs
    2,  // # of bits per index
    code_na001Times,
    code_na001Codes
};

const uint16_t code_na002Times[] = {
    42,
    46,
    42,
    133,
    42,
    7519,
    347,
    176,
    347,
    177,
};
const uint8_t code_na002Codes[] = {
    0x60, 0x80, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x04,
    0x12, 0x48, 0x04, 0x12, 0x48, 0x2A, 0x02, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
    0x00, 0x00, 0x80, 0x00, 0x00, 0x10, 0x49, 0x20, 0x10, 0x49, 0x20, 0x80,
};
const struct IrCode code_na002Code = {
    freq_to_timerval(37037),
    100, // # of pairs
    3,   // # of bits per index
    code_na002Times,
    code_na002Codes
};

const uint16_t code_na003Times[] = {
    26,
    185,
    27,
    80,
    27,
    185,
    27,
    4549,
};
const uint8_t code_na003Codes[] = {
    0x15,
    0x5A,
    0x65,
    0x67,
    0x95,
    0x65,
    0x9A,
    0x9B,
    0x95,
    0x5A,
    0x65,
    0x67,
    0x95,
    0x65,
    0x9A,
    0x99,
};
const struct IrCode code_na003Code = {
    freq_to_timerval(38610),
    64, // # of pairs
    2,  // # of bits per index
    code_na003Times,
    code_na003Codes
};

const uint16_t code_na004Times[] = {
    55,
    57,
    55,
    170,
    55,
    3949,
    55,
    9623,
    56,
    0,
    898,
    453,
    900,
    226,
};
const uint8_t code_na004Codes[] = {
    0xA0,
    0x00,
    0x01,
    0x04,
    0x92,
    0x48,
    0x20,
    0x80,
    0x40,
    0x04,
    0x12,
    0x09,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na004Code = {
    freq_to_timerval(38610),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na004Codes
};

const uint16_t code_na005Times[] = {
    88,
    90,
    88,
    91,
    88,
    181,
    88,
    8976,
    177,
    91,
};
const uint8_t code_na005Codes[] = {
    0x10,
    0x92,
    0x49,
    0x46,
    0x33,
    0x09,
    0x24,
    0x94,
    0x60,
};
const struct IrCode code_na005Code = {
    freq_to_timerval(35714),
    24, // # of pairs
    3,  // # of bits per index
    code_na005Times,
    code_na005Codes
};

const uint16_t code_na006Times[] = {
    50,
    62,
    50,
    172,
    50,
    4541,
    448,
    466,
    450,
    465,
};
const uint8_t code_na006Codes[] = {
    0x64, 0x90, 0x00, 0x04, 0x90, 0x00, 0x00, 0x80, 0x00, 0x04, 0x12, 0x49, 0x2A,
    0x12, 0x40, 0x00, 0x12, 0x40, 0x00, 0x02, 0x00, 0x00, 0x10, 0x49, 0x24, 0x90,
};
const struct IrCode code_na006Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_na006Times,
    code_na006Codes
};

const uint16_t code_na007Times[] = {
    49,
    49,
    49,
    50,
    49,
    410,
    49,
    510,
    49,
    12107,
};
const uint8_t code_na007Codes[] = {
    0x09,
    0x94,
    0x53,
    0x29,
    0x94,
    0xD9,
    0x85,
    0x32,
    0x8A,
    0x65,
    0x32,
    0x9B,
    0x20,
};
const struct IrCode code_na007Code = {
    freq_to_timerval(39216),
    34, // # of pairs
    3,  // # of bits per index
    code_na007Times,
    code_na007Codes
};

const uint16_t code_na008Times[] = {
    56,
    58,
    56,
    170,
    56,
    4011,
    898,
    450,
    900,
    449,
};
const uint8_t code_na008Codes[] = {
    0x64, 0x00, 0x49, 0x00, 0x92, 0x00, 0x20, 0x82, 0x01, 0x04, 0x10, 0x48, 0x2A,
    0x10, 0x01, 0x24, 0x02, 0x48, 0x00, 0x82, 0x08, 0x04, 0x10, 0x41, 0x20, 0x90,
};
const struct IrCode code_na008Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_na008Times,
    code_na008Codes
};

const uint16_t code_na009Times[] = {
    53,
    56,
    53,
    171,
    53,
    3950,
    53,
    9599,
    898,
    451,
    900,
    226,
};
const uint8_t code_na009Codes[] = {
    0x84,
    0x90,
    0x00,
    0x20,
    0x80,
    0x08,
    0x00,
    0x00,
    0x09,
    0x24,
    0x92,
    0x40,
    0x0A,
    0xBA,
    0x40,
};
const struct IrCode code_na009Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_na009Codes
};

const uint16_t code_na010Times[] = {
    51,
    55,
    51,
    158,
    51,
    2286,
    841,
    419,
};
const uint8_t code_na010Codes[] = {
    0xD4,
    0x00,
    0x15,
    0x10,
    0x25,
    0x00,
    0x05,
    0x44,
    0x09,
    0x40,
    0x01,
    0x51,
    0x01,
};
const struct IrCode code_na010Code = {
    freq_to_timerval(38462),
    52, // # of pairs
    2,  // # of bits per index
    code_na010Times,
    code_na010Codes
};

const uint16_t code_na011Times[] = {
    55,
    55,
    55,
    172,
    55,
    4039,
    55,
    9348,
    56,
    0,
    884,
    442,
    885,
    225,
};
const uint8_t code_na011Codes[] = {
    0xA0,
    0x00,
    0x41,
    0x04,
    0x92,
    0x08,
    0x24,
    0x90,
    0x40,
    0x00,
    0x02,
    0x09,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na011Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na011Times,
    code_na011Codes
};

const uint16_t code_na012Times[] = {
    81,
    87,
    81,
    254,
    81,
    3280,
    331,
    336,
    331,
    337,
};
const uint8_t code_na012Codes[] = {
    0x64, 0x12, 0x08, 0x24, 0x00, 0x08, 0x20, 0x10, 0x09, 0x2A,
    0x10, 0x48, 0x20, 0x90, 0x00, 0x20, 0x80, 0x40, 0x24, 0x90,
};
const struct IrCode code_na012Code = {
    freq_to_timerval(38462),
    52, // # of pairs
    3,  // # of bits per index
    code_na012Times,
    code_na012Codes
};

const uint16_t code_na013Times[] = {
    53,
    55,
    53,
    167,
    53,
    2304,
    53,
    9369,
    893,
    448,
    895,
    447,
};
const uint8_t code_na013Codes[] = {
    0x80,
    0x12,
    0x40,
    0x04,
    0x00,
    0x09,
    0x00,
    0x12,
    0x41,
    0x24,
    0x82,
    0x01,
    0x00,
    0x10,
    0x48,
    0x24,
    0xAA,
    0xE8,
};
const struct IrCode code_na013Code = {
    freq_to_timerval(38462),
    48, // # of pairs
    3,  // # of bits per index
    code_na013Times,
    code_na013Codes
};

const uint8_t code_na014Codes[] = {
    0xA0,
    0x00,
    0x09,
    0x04,
    0x92,
    0x40,
    0x24,
    0x80,
    0x00,
    0x00,
    0x12,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na014Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na014Codes
};

const uint8_t code_na015Codes[] = {
    0xA0,
    0x80,
    0x01,
    0x04,
    0x12,
    0x48,
    0x24,
    0x00,
    0x00,
    0x00,
    0x92,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na015Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na015Codes
};

const uint16_t code_na016Times[] = {
    28,
    90,
    28,
    211,
    28,
    2507,
};
const uint8_t code_na016Codes[] = {
    0x54,
    0x04,
    0x10,
    0x00,
    0x95,
    0x01,
    0x04,
    0x00,
    0x10,
};
const struct IrCode code_na016Code = {
    freq_to_timerval(34483),
    34, // # of pairs
    2,  // # of bits per index
    code_na016Times,
    code_na016Codes
};

const uint16_t code_na017Times[] = {
    56,
    57,
    56,
    175,
    56,
    4150,
    56,
    9499,
    898,
    227,
    898,
    449,
};
const uint8_t code_na017Codes[] = {
    0xA0,
    0x02,
    0x48,
    0x04,
    0x90,
    0x01,
    0x20,
    0x80,
    0x40,
    0x04,
    0x12,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na017Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na017Codes
};

const uint16_t code_na018Times[] = {
    51,
    55,
    51,
    161,
    51,
    2566,
    849,
    429,
    849,
    430,
};
const uint8_t code_na018Codes[] = {
    0x60, 0x82, 0x08, 0x24, 0x10, 0x41, 0x00, 0x12, 0x40, 0x04, 0x80, 0x09, 0x2A, 0x02, 0x08, 0x20, 0x90,
    0x41, 0x04, 0x00, 0x49, 0x00, 0x12, 0x00, 0x24, 0xA8, 0x08, 0x20, 0x82, 0x41, 0x04, 0x10, 0x01, 0x24,
    0x00, 0x48, 0x00, 0x92, 0xA0, 0x20, 0x82, 0x09, 0x04, 0x10, 0x40, 0x04, 0x90, 0x01, 0x20, 0x02, 0x48,
};
const struct IrCode code_na018Code = {
    freq_to_timerval(38462),
    136, // # of pairs
    3,   // # of bits per index
    code_na018Times,
    code_na018Codes
};

const uint16_t code_na019Times[] = {
    40,
    42,
    40,
    124,
    40,
    4601,
    325,
    163,
    326,
    163,
};
const uint8_t code_na019Codes[] = {
    0x60, 0x10, 0x40, 0x04, 0x80, 0x09, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20,
    0x10, 0x00, 0x20, 0x80, 0x00, 0x0A, 0x00, 0x41, 0x00, 0x12, 0x00, 0x24, 0x00,
    0x00, 0x00, 0x00, 0x40, 0x00, 0x80, 0x40, 0x00, 0x82, 0x00, 0x00, 0x00,
};
const struct IrCode code_na019Code = {
    freq_to_timerval(38462),
    100, // # of pairs
    3,   // # of bits per index
    code_na019Times,
    code_na019Codes
};

const uint16_t code_na020Times[] = {
    60,
    55,
    60,
    163,
    60,
    4099,
    60,
    9698,
    61,
    0,
    898,
    461,
    900,
    230,
};
const uint8_t code_na020Codes[] = {
    0xA0,
    0x10,
    0x00,
    0x04,
    0x82,
    0x49,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na020Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na020Times,
    code_na020Codes
};

const uint16_t code_na021Times[] = {
    48,
    52,
    48,
    160,
    48,
    400,
    48,
    2335,
    799,
    400,
};
const uint8_t code_na021Codes[] = {
    0x80,
    0x10,
    0x40,
    0x08,
    0x82,
    0x08,
    0x01,
    0xC0,
    0x08,
    0x20,
    0x04,
    0x41,
    0x04,
    0x00,
    0x00,
};
const struct IrCode code_na021Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na021Times,
    code_na021Codes
};

const uint16_t code_na022Times[] = {
    53,
    60,
    53,
    175,
    53,
    4463,
    53,
    9453,
    892,
    450,
    895,
    225,
};
const uint8_t code_na022Codes[] = {
    0x80,
    0x02,
    0x40,
    0x00,
    0x02,
    0x40,
    0x00,
    0x00,
    0x01,
    0x24,
    0x92,
    0x48,
    0x0A,
    0xBA,
    0x00,
};
const struct IrCode code_na022Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na022Times,
    code_na022Codes
};

const uint16_t code_na023Times[] = {
    48,
    52,
    48,
    409,
    48,
    504,
    48,
    10461,
};
const uint8_t code_na023Codes[] = {
    0xA1,
    0x18,
    0x61,
    0xA1,
    0x18,
    0x7A,
    0x11,
    0x86,
    0x1A,
    0x11,
    0x86,
};
const struct IrCode code_na023Code = {
    freq_to_timerval(40000),
    44, // # of pairs
    2,  // # of bits per index
    code_na023Times,
    code_na023Codes
};

const uint16_t code_na024Times[] = {
    58,
    60,
    58,
    2569,
    118,
    60,
    237,
    60,
    238,
    60,
};
const uint8_t code_na024Codes[] = {
    0x69,
    0x24,
    0x10,
    0x40,
    0x03,
    0x12,
    0x48,
    0x20,
    0x80,
    0x00,
};
const struct IrCode code_na024Code = {
    freq_to_timerval(38462),
    26, // # of pairs
    3,  // # of bits per index
    code_na024Times,
    code_na024Codes
};

const uint16_t code_na025Times[] = {
    84,
    90,
    84,
    264,
    84,
    3470,
    346,
    350,
    347,
    350,
};
const uint8_t code_na025Codes[] = {
    0x64, 0x92, 0x49, 0x00, 0x00, 0x00, 0x00, 0x02, 0x49, 0x2A,
    0x12, 0x49, 0x24, 0x00, 0x00, 0x00, 0x00, 0x09, 0x24, 0x90,
};
const struct IrCode code_na025Code = {
    freq_to_timerval(38462),
    52, // # of pairs
    3,  // # of bits per index
    code_na025Times,
    code_na025Codes
};

const uint16_t code_na026Times[] = {
    49,
    49,
    49,
    50,
    49,
    410,
    49,
    510,
    49,
    12582,
};
const uint8_t code_na026Codes[] = {
    0x09,
    0x94,
    0x53,
    0x65,
    0x32,
    0x99,
    0x85,
    0x32,
    0x8A,
    0x6C,
    0xA6,
    0x53,
    0x20,
};
const struct IrCode code_na026Code = {
    freq_to_timerval(39216),
    34, // # of pairs
    3,  // # of bits per index
    code_na026Times,
    code_na026Codes
};

const uint8_t code_na027Codes[] = {
    0xC5,
    0x41,
    0x11,
    0x10,
    0x14,
    0x44,
    0x6C,
    0x54,
    0x11,
    0x11,
    0x01,
    0x44,
    0x44,
};
const struct IrCode code_na027Code = {
    freq_to_timerval(57143),
    52, // # of pairs
    2,  // # of bits per index
    code_na001Times,
    code_na027Codes
};

const uint16_t code_na028Times[] = {
    118,
    121,
    118,
    271,
    118,
    4750,
    258,
    271,
};
const uint8_t code_na028Codes[] = {
    0xC4,
    0x45,
    0x14,
    0x04,
    0x6C,
    0x44,
    0x51,
    0x40,
    0x44,
};
const struct IrCode code_na028Code = {
    freq_to_timerval(38610),
    36, // # of pairs
    2,  // # of bits per index
    code_na028Times,
    code_na028Codes
};

const uint16_t code_na029Times[] = {
    88,
    90,
    88,
    91,
    88,
    181,
    177,
    91,
    177,
    8976,
};
const uint8_t code_na029Codes[] = {
    0x0C,
    0x92,
    0x53,
    0x46,
    0x16,
    0x49,
    0x29,
    0xA2,
    0xC0,
};
const struct IrCode code_na029Code = {
    freq_to_timerval(35842),
    22, // # of pairs
    3,  // # of bits per index
    code_na029Times,
    code_na029Codes
};

const uint8_t code_na030Codes[] = {
    0x80,
    0x00,
    0x41,
    0x04,
    0x12,
    0x08,
    0x20,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na030Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_na030Codes
};

const uint16_t code_na031Times[] = {
    88,
    89,
    88,
    90,
    88,
    179,
    88,
    8977,
    177,
    90,
};
const uint8_t code_na031Codes[] = {
    0x06,
    0x12,
    0x49,
    0x46,
    0x32,
    0x61,
    0x24,
    0x94,
    0x60,
};
const struct IrCode code_na031Code = {
    freq_to_timerval(35842),
    24, // # of pairs
    3,  // # of bits per index
    code_na031Times,
    code_na031Codes
};

const uint8_t code_na032Codes[] = {
    0x80,
    0x00,
    0x41,
    0x04,
    0x12,
    0x08,
    0x20,
    0x80,
    0x00,
    0x04,
    0x12,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na032Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_na032Codes
};

const uint16_t code_na033Times[] = {
    40,
    43,
    40,
    122,
    40,
    5297,
    334,
    156,
    336,
    155,
};
const uint8_t code_na033Codes[] = {
    0x60, 0x10, 0x40, 0x04, 0x80, 0x09, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20,
    0x82, 0x00, 0x20, 0x00, 0x00, 0x0A, 0x00, 0x41, 0x00, 0x12, 0x00, 0x24, 0x00,
    0x00, 0x00, 0x00, 0x40, 0x00, 0x82, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00,
};
const struct IrCode code_na033Code = {
    freq_to_timerval(38462),
    100, // # of pairs
    3,   // # of bits per index
    code_na033Times,
    code_na033Codes
};

const uint8_t code_na034Codes[] = {
    0xA0,
    0x00,
    0x41,
    0x04,
    0x92,
    0x08,
    0x24,
    0x92,
    0x48,
    0x00,
    0x00,
    0x01,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na034Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na034Codes
};

const uint16_t code_na035Times[] = {
    96,
    93,
    97,
    93,
    97,
    287,
    97,
    3431,
};
const uint8_t code_na035Codes[] = {
    0x16,
    0x66,
    0x5D,
    0x59,
    0x99,
    0x50,
};
const struct IrCode code_na035Code = {
    freq_to_timerval(41667),
    22, // # of pairs
    2,  // # of bits per index
    code_na035Times,
    code_na035Codes
};

const uint16_t code_na036Times[] = {
    82,
    581,
    84,
    250,
    84,
    580,
    85,
    0,
};
const uint8_t code_na036Codes[] = {
    0x15,
    0x9A,
    0x9C,
};
const struct IrCode code_na036Code = {
    freq_to_timerval(37037),
    11, // # of pairs
    2,  // # of bits per index
    code_na036Times,
    code_na036Codes
};

const uint16_t code_na037Times[] = {
    39,
    263,
    164,
    163,
    514,
    164,
};
const uint8_t code_na037Codes[] = {
    0x80,
    0x45,
    0x00,
};
const struct IrCode code_na037Code = {
    freq_to_timerval(41667),
    11, // # of pairs
    2,  // # of bits per index
    code_na037Times,
    code_na037Codes
};

const uint8_t code_na038Codes[] = {
    0xA4,
    0x10,
    0x40,
    0x00,
    0x82,
    0x09,
    0x20,
    0x80,
    0x40,
    0x04,
    0x12,
    0x09,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na038Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na038Codes
};

const uint16_t code_na039Times[] = {
    113,
    101,
    688,
    2707,
};
const uint8_t code_na039Codes[] = {
    0x11,
};
const struct IrCode code_na039Code = {
    freq_to_timerval(40000),
    4, // # of pairs
    2, // # of bits per index
    code_na039Times,
    code_na039Codes
};

const uint16_t code_na040Times[] = {
    113,
    101,
    113,
    201,
    113,
    2707,
};
const uint8_t code_na040Codes[] = {
    0x06,
    0x04,
};
const struct IrCode code_na040Code = {
    freq_to_timerval(40000),
    8, // # of pairs
    2, // # of bits per index
    code_na040Times,
    code_na040Codes
};

const uint16_t code_na041Times[] = {
    58,
    62,
    58,
    2746,
    117,
    62,
    242,
    62,
};
const uint8_t code_na041Codes[] = {
    0xE2,
    0x20,
    0x80,
    0x78,
    0x88,
    0x20,
    0x00,
};
const struct IrCode code_na041Code = {
    freq_to_timerval(76923),
    26, // # of pairs
    2,  // # of bits per index
    code_na041Times,
    code_na041Codes
};

const uint16_t code_na042Times[] = {
    54,
    65,
    54,
    170,
    54,
    4099,
    54,
    8668,
    899,
    226,
    899,
    421,
};
const uint8_t code_na042Codes[] = {
    0xA4,
    0x80,
    0x00,
    0x20,
    0x82,
    0x49,
    0x00,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na042Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na042Times,
    code_na042Codes
};

const uint16_t code_na043Times[] = {
    43,
    120,
    43,
    121,
    43,
    3491,
    131,
    45,
};
const uint8_t code_na043Codes[] = {
    0x15,
    0x75,
    0x56,
    0x55,
    0x75,
    0x54,
};
const struct IrCode code_na043Code = {
    freq_to_timerval(40000),
    24, // # of pairs
    2,  // # of bits per index
    code_na043Times,
    code_na043Codes
};

const uint16_t code_na044Times[] = {
    51,
    51,
    51,
    160,
    51,
    4096,
    51,
    9513,
    431,
    436,
    883,
    219,
};
const uint8_t code_na044Codes[] = {
    0x84,
    0x90,
    0x00,
    0x00,
    0x02,
    0x49,
    0x20,
    0x80,
    0x00,
    0x04,
    0x12,
    0x49,
    0x2A,
    0xBA,
    0x40,
};
const struct IrCode code_na044Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na044Times,
    code_na044Codes
};

const uint16_t code_na045Times[] = {
    58,
    53,
    58,
    167,
    58,
    4494,
    58,
    9679,
    455,
    449,
    456,
    449,
};
const uint8_t code_na045Codes[] = {
    0x80,
    0x90,
    0x00,
    0x00,
    0x90,
    0x00,
    0x04,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0x97,
    0x48,
};
const struct IrCode code_na045Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_na045Times,
    code_na045Codes
};

const uint16_t code_na046Times[] = {
    51,
    277,
    52,
    53,
    52,
    105,
    52,
    277,
    52,
    2527,
    52,
    12809,
    103,
    54,
};
const uint8_t code_na046Codes[] = {
    0x0B,
    0x12,
    0x63,
    0x44,
    0x92,
    0x6B,
    0x44,
    0x92,
    0x50,
};
const struct IrCode code_na046Code = {
    freq_to_timerval(29412),
    23, // # of pairs
    3,  // # of bits per index
    code_na046Times,
    code_na046Codes
};

const uint8_t code_na047Codes[] = {
    0xA0,
    0x00,
    0x40,
    0x04,
    0x92,
    0x09,
    0x24,
    0x92,
    0x09,
    0x20,
    0x00,
    0x40,
    0x0A,
    0x38,
    0x00,
};
const struct IrCode code_na047Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na047Codes
};

const uint8_t code_na048Codes[] = {
    0x80,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x24,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na048Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na044Times,
    code_na048Codes
};

const uint16_t code_na049Times[] = {
    274,
    854,
    274,
    1986,
};
const uint8_t code_na049Codes[] = {
    0x14,
    0x11,
    0x40,
};
const struct IrCode code_na049Code = {
    freq_to_timerval(45455),
    11, // # of pairs
    2,  // # of bits per index
    code_na049Times,
    code_na049Codes
};

const uint16_t code_na050Times[] = {
    80,
    88,
    80,
    254,
    80,
    3750,
    359,
    331,
};
const uint8_t code_na050Codes[] = {
    0xC0,
    0x00,
    0x01,
    0x55,
    0x55,
    0x52,
    0xC0,
    0x00,
    0x01,
    0x55,
    0x55,
    0x50,
};
const struct IrCode code_na050Code = {
    freq_to_timerval(55556),
    48, // # of pairs
    2,  // # of bits per index
    code_na050Times,
    code_na050Codes
};

const uint8_t code_na051Codes[] = {
    0xA0,
    0x10,
    0x01,
    0x24,
    0x82,
    0x48,
    0x00,
    0x02,
    0x40,
    0x04,
    0x90,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na051Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na051Codes
};

const uint8_t code_na052Codes[] = {
    0xA4,
    0x90,
    0x48,
    0x00,
    0x02,
    0x01,
    0x20,
    0x80,
    0x40,
    0x04,
    0x12,
    0x09,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na052Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na052Codes
};

const uint16_t code_na053Times[] = {
    51,
    232,
    51,
    512,
    51,
    792,
    51,
    2883,
};
const uint8_t code_na053Codes[] = {
    0x22,
    0x21,
    0x40,
    0x1C,
    0x88,
    0x85,
    0x00,
    0x40,
};
const struct IrCode code_na053Code = {
    freq_to_timerval(55556),
    30, // # of pairs
    2,  // # of bits per index
    code_na053Times,
    code_na053Codes
};

const uint8_t code_na054Codes[] = {
    0x22,
    0x20,
    0x15,
    0x72,
    0x22,
    0x01,
    0x54,
};
const struct IrCode code_na054Code = {
    freq_to_timerval(55556),
    28, // # of pairs
    2,  // # of bits per index
    code_na053Times,
    code_na054Codes
};

const uint16_t code_na055Times[] = {
    3,
    10,
    3,
    20,
    3,
    30,
    3,
    12778,
};
const uint8_t code_na055Codes[] = {
    0x81,
    0x51,
    0x14,
    0xB8,
    0x15,
    0x11,
    0x44,
};
const struct IrCode code_na055Code = {
    0,  // Non-pulsed code
    27, // # of pairs
    2,  // # of bits per index
    code_na055Times,
    code_na055Codes
};

const uint16_t code_na056Times[] = {
    55,
    193,
    57,
    192,
    57,
    384,
    58,
    0,
};
const uint8_t code_na056Codes[] = {
    0x2A,
    0x57,
};
const struct IrCode code_na056Code = {
    freq_to_timerval(37175),
    8, // # of pairs
    2, // # of bits per index
    code_na056Times,
    code_na056Codes
};

const uint16_t code_na057Times[] = {
    45,
    148,
    46,
    148,
    46,
    351,
    46,
    2781,
};
const uint8_t code_na057Codes[] = {
    0x2A,
    0x5D,
    0xA9,
    0x60,
};
const struct IrCode code_na057Code = {
    freq_to_timerval(40000),
    14, // # of pairs
    2,  // # of bits per index
    code_na057Times,
    code_na057Codes
};

const uint16_t code_na058Times[] = {
    22,
    101,
    22,
    219,
    23,
    101,
    23,
    219,
    31,
    218,
};
const uint8_t code_na058Codes[] = {
    0x8D,
    0xA4,
    0x08,
    0x04,
    0x04,
    0x92,
    0x4C,
};
const struct IrCode code_na058Code = {
    freq_to_timerval(33333),
    18, // # of pairs
    3,  // # of bits per index
    code_na058Times,
    code_na058Codes
};

const uint8_t code_na059Codes[] = {
    0xA4,
    0x12,
    0x09,
    0x00,
    0x80,
    0x40,
    0x20,
    0x10,
    0x40,
    0x04,
    0x82,
    0x09,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na059Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na059Codes
};

const uint8_t code_na060Codes[] = {
    0xA0,
    0x00,
    0x08,
    0x04,
    0x92,
    0x41,
    0x24,
    0x00,
    0x40,
    0x00,
    0x92,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na060Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na060Codes
};

const uint8_t code_na061Codes[] = {
    0xA0,
    0x00,
    0x08,
    0x24,
    0x92,
    0x41,
    0x04,
    0x82,
    0x00,
    0x00,
    0x10,
    0x49,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na061Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na061Codes
};

const uint8_t code_na062Codes[] = {
    0xA0,
    0x02,
    0x08,
    0x04,
    0x90,
    0x41,
    0x24,
    0x82,
    0x00,
    0x00,
    0x10,
    0x49,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na062Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na062Codes
};

const uint8_t code_na063Codes[] = {
    0xA4,
    0x92,
    0x49,
    0x20,
    0x00,
    0x00,
    0x04,
    0x92,
    0x48,
    0x00,
    0x00,
    0x01,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na063Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na063Codes
};

const uint8_t code_na064Codes[] = {
    0xC0,
    0x01,
    0x51,
    0x55,
    0x54,
    0x04,
    0x2C,
    0x00,
    0x15,
    0x15,
    0x55,
    0x40,
    0x40,
};
const struct IrCode code_na064Code = {
    freq_to_timerval(57143),
    52, // # of pairs
    2,  // # of bits per index
    code_na001Times,
    code_na064Codes
};

const uint16_t code_na065Times[] = {
    48,
    98,
    48,
    197,
    98,
    846,
    395,
    392,
    1953,
    392,
};
const uint8_t code_na065Codes[] = {
    0x84, 0x92, 0x01, 0x24, 0x12, 0x00, 0x04, 0x80, 0x08, 0x09, 0x92, 0x48, 0x04, 0x90, 0x48,
    0x00, 0x12, 0x00, 0x20, 0x26, 0x49, 0x20, 0x12, 0x41, 0x20, 0x00, 0x48, 0x00, 0x80, 0x80,
};
const struct IrCode code_na065Code = {
    freq_to_timerval(59172),
    78, // # of pairs
    3,  // # of bits per index
    code_na065Times,
    code_na065Codes
};

const uint16_t code_na066Times[] = {
    38,
    276,
    165,
    154,
    415,
    155,
    742,
    154,
};
const uint8_t code_na066Codes[] = {
    0xC0,
    0x45,
    0x02,
    0x01,
    0x14,
    0x08,
    0x04,
    0x50,
    0x00,
};
const struct IrCode code_na066Code = {
    freq_to_timerval(38462),
    33, // # of pairs
    2,  // # of bits per index
    code_na066Times,
    code_na066Codes
};

const uint8_t code_na067Codes[] = {
    0x80,
    0x02,
    0x49,
    0x24,
    0x90,
    0x00,
    0x00,
    0x80,
    0x00,
    0x04,
    0x12,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na067Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na044Times,
    code_na067Codes
};

const uint16_t code_na068Times[] = {
    43,
    121,
    43,
    9437,
    130,
    45,
    131,
    45,
};
const uint8_t code_na068Codes[] = {
    0x8C,
    0x30,
    0x0D,
    0xCC,
    0x30,
    0x0C,
};
const struct IrCode code_na068Code = {
    freq_to_timerval(40000),
    24, // # of pairs
    2,  // # of bits per index
    code_na068Times,
    code_na068Codes
};

const uint8_t code_na069Codes[] = {
    0xA0,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x24,
    0x82,
    0x00,
    0x00,
    0x10,
    0x49,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na069Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na069Codes
};

const uint16_t code_na070Times[] = {
    27,
    76,
    27,
    182,
    27,
    183,
    27,
    3199,
};
const uint8_t code_na070Codes[] = {
    0x40,
    0x02,
    0x08,
    0xA2,
    0xE0,
    0x00,
    0x82,
    0x28,
    0x40,
};
const struct IrCode code_na070Code = {
    freq_to_timerval(38462),
    33, // # of pairs
    2,  // # of bits per index
    code_na070Times,
    code_na070Codes
};

const uint16_t code_na071Times[] = {
    37,
    181,
    37,
    272,
};
const uint8_t code_na071Codes[] = {
    0x11,
    0x40,
};
const struct IrCode code_na071Code = {
    freq_to_timerval(55556),
    8, // # of pairs
    2, // # of bits per index
    code_na071Times,
    code_na071Codes
};

const uint8_t code_na072Codes[] = {
    0xA0,
    0x90,
    0x00,
    0x00,
    0x90,
    0x00,
    0x00,
    0x10,
    0x40,
    0x04,
    0x82,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na072Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na042Times,
    code_na072Codes
};

const uint8_t code_na073Codes[] = {
    0xA0,
    0x82,
    0x08,
    0x24,
    0x10,
    0x41,
    0x00,
    0x00,
    0x00,
    0x24,
    0x92,
    0x49,
    0x0A,
    0x38,
    0x00,
};
const struct IrCode code_na073Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na073Codes
};

const uint8_t code_na074Codes[] = {
    0xA4,
    0x00,
    0x41,
    0x00,
    0x92,
    0x08,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na074Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na074Codes
};

const uint16_t code_na075Times[] = {
    51,
    98,
    51,
    194,
    102,
    931,
    390,
    390,
    390,
    391,
};
const uint8_t code_na075Codes[] = {
    0x60, 0x00, 0x01, 0x04, 0x10, 0x49, 0x24, 0x82, 0x08, 0x2A,
    0x00, 0x00, 0x04, 0x10, 0x41, 0x24, 0x92, 0x08, 0x20, 0xA0,
};
const struct IrCode code_na075Code = {
    freq_to_timerval(41667),
    52, // # of pairs
    3,  // # of bits per index
    code_na075Times,
    code_na075Codes
};

const uint8_t code_na076Codes[] = {
    0xA0,
    0x92,
    0x09,
    0x04,
    0x00,
    0x40,
    0x20,
    0x10,
    0x40,
    0x04,
    0x82,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na076Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na076Codes
};

const uint8_t code_na077Codes[] = {
    0x10,
    0xA2,
    0x62,
    0x31,
    0x98,
    0x51,
    0x31,
    0x18,
    0x00,
};
const struct IrCode code_na077Code = {
    freq_to_timerval(35714),
    22, // # of pairs
    3,  // # of bits per index
    code_na031Times,
    code_na077Codes
};

const uint16_t code_na078Times[] = {
    40,
    275,
    160,
    154,
    480,
    155,
};
const uint8_t code_na078Codes[] = {
    0x80,
    0x45,
    0x04,
    0x01,
    0x14,
    0x10,
    0x04,
    0x50,
    0x40,
};
const struct IrCode code_na078Code = {
    freq_to_timerval(38462),
    34, // # of pairs
    2,  // # of bits per index
    code_na078Times,
    code_na078Codes
};

const uint8_t code_na079Codes[] = {
    0xA0,
    0x82,
    0x08,
    0x24,
    0x10,
    0x41,
    0x04,
    0x90,
    0x08,
    0x20,
    0x02,
    0x41,
    0x0A,
    0x38,
    0x00,
};
const struct IrCode code_na079Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na079Codes
};

const uint8_t code_na080Codes[] = {
    0x81,
    0x50,
    0x40,
    0xB8,
    0x15,
    0x04,
    0x08,
};
const struct IrCode code_na080Code = {
    0,  // Non-pulsed code
    27, // # of pairs
    2,  // # of bits per index
    code_na055Times,
    code_na080Codes
};

const uint16_t code_na081Times[] = {
    48,
    52,
    48,
    409,
    48,
    504,
    48,
    9978,
};
const uint8_t code_na081Codes[] = {
    0x18,
    0x46,
    0x18,
    0x68,
    0x47,
    0x18,
    0x46,
    0x18,
    0x68,
    0x44,
};
const struct IrCode code_na081Code = {
    freq_to_timerval(40000),
    40, // # of pairs
    2,  // # of bits per index
    code_na081Times,
    code_na081Codes
};

const uint16_t code_na082Times[] = {
    88,
    89,
    88,
    90,
    88,
    179,
    88,
    8888,
    177,
    90,
    177,
    179,
};
const uint8_t code_na082Codes[] = {
    0x0A,
    0x12,
    0x49,
    0x2A,
    0xB2,
    0xA1,
    0x24,
    0x92,
    0xA8,
};
const struct IrCode code_na082Code = {
    freq_to_timerval(35714),
    24, // # of pairs
    3,  // # of bits per index
    code_na082Times,
    code_na082Codes
};

const uint8_t code_na083Codes[] = {
    0x10,
    0x92,
    0x49,
    0x46,
    0x33,
    0x09,
    0x24,
    0x94,
    0x60,
};
const struct IrCode code_na083Code = {
    freq_to_timerval(35714),
    24, // # of pairs
    3,  // # of bits per index
    code_na031Times,
    code_na083Codes
};

const uint16_t code_na084Times[] = {
    41,
    43,
    41,
    128,
    41,
    7476,
    336,
    171,
    338,
    169,
};
const uint8_t code_na084Codes[] = {
    0x60, 0x80, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x40, 0x20, 0x00, 0x00, 0x04,
    0x12, 0x48, 0x04, 0x12, 0x08, 0x2A, 0x02, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
    0x01, 0x00, 0x80, 0x00, 0x00, 0x10, 0x49, 0x20, 0x10, 0x48, 0x20, 0x80,
};
const struct IrCode code_na084Code = {
    freq_to_timerval(37037),
    100, // # of pairs
    3,   // # of bits per index
    code_na084Times,
    code_na084Codes
};

const uint16_t code_na085Times[] = {
    55,
    60,
    55,
    165,
    55,
    2284,
    445,
    437,
    448,
    436,
};
const uint8_t code_na085Codes[] = {
    0x64,
    0x00,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x80,
    0xA1,
    0x00,
    0x00,
    0x00,
    0x00,
    0x10,
    0x00,
    0x20,
    0x10,
};
const struct IrCode code_na085Code = {
    freq_to_timerval(38462),
    44, // # of pairs
    3,  // # of bits per index
    code_na085Times,
    code_na085Codes
};

const uint16_t code_na086Times[] = {
    42,
    46,
    42,
    126,
    42,
    6989,
    347,
    176,
    347,
    177,
};
const uint8_t code_na086Codes[] = {
    0x60, 0x82, 0x08, 0x20, 0x82, 0x41, 0x04, 0x92, 0x00, 0x20, 0x80, 0x40, 0x00,
    0x90, 0x40, 0x04, 0x00, 0x41, 0x2A, 0x02, 0x08, 0x20, 0x82, 0x09, 0x04, 0x12,
    0x48, 0x00, 0x82, 0x01, 0x00, 0x02, 0x41, 0x00, 0x10, 0x01, 0x04, 0x80,
};
const struct IrCode code_na086Code = {
    freq_to_timerval(37175),
    100, // # of pairs
    3,   // # of bits per index
    code_na086Times,
    code_na086Codes
};

const uint16_t code_na087Times[] = {
    56,
    69,
    56,
    174,
    56,
    4165,
    56,
    9585,
    880,
    222,
    880,
    435,
};
const uint8_t code_na087Codes[] = {
    0xA0,
    0x02,
    0x40,
    0x04,
    0x90,
    0x09,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na087Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na087Times,
    code_na087Codes
};

const uint8_t code_na088Codes[] = {
    0x80,
    0x00,
    0x40,
    0x04,
    0x12,
    0x08,
    0x04,
    0x92,
    0x40,
    0x00,
    0x00,
    0x09,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na088Code = {
    freq_to_timerval(38610),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_na088Codes
};

const uint8_t code_na089Codes[] = {
    0xA0,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x20,
    0x80,
    0x40,
    0x04,
    0x12,
    0x09,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na089Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na089Codes
};

const uint16_t code_na090Times[] = {
    88,
    90,
    88,
    91,
    88,
    181,
    88,
    8976,
    177,
    91,
    177,
    181,
};
const uint8_t code_na090Codes[] = {
    0x10,
    0xAB,
    0x11,
    0x8C,
    0xC2,
    0xAC,
    0x46,
    0x00,
};
const struct IrCode code_na090Code = {
    freq_to_timerval(35714),
    20, // # of pairs
    3,  // # of bits per index
    code_na090Times,
    code_na090Codes
};

const uint16_t code_na091Times[] = {
    48,
    100,
    48,
    200,
    48,
    1050,
    400,
    400,
};
const uint8_t code_na091Codes[] = {
    0xD5,
    0x41,
    0x51,
    0x40,
    0x14,
    0x04,
    0x2D,
    0x54,
    0x15,
    0x14,
    0x01,
    0x40,
    0x41,
};
const struct IrCode code_na091Code = {
    freq_to_timerval(58824),
    52, // # of pairs
    2,  // # of bits per index
    code_na091Times,
    code_na091Codes
};

const uint16_t code_na092Times[] = {
    54,
    56,
    54,
    170,
    54,
    4927,
    451,
    447,
};
const uint8_t code_na092Codes[] = {
    0xD1,
    0x00,
    0x11,
    0x00,
    0x04,
    0x00,
    0x11,
    0x55,
    0x6D,
    0x10,
    0x01,
    0x10,
    0x00,
    0x40,
    0x01,
    0x15,
    0x55,
};
const struct IrCode code_na092Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    2,  // # of bits per index
    code_na092Times,
    code_na092Codes
};

const uint16_t code_na093Times[] = {
    55,
    57,
    55,
    167,
    55,
    4400,
    895,
    448,
    897,
    447,
};
const uint8_t code_na093Codes[] = {
    0x60, 0x90, 0x00, 0x20, 0x80, 0x00, 0x04, 0x02, 0x01, 0x00, 0x90, 0x48, 0x2A,
    0x02, 0x40, 0x00, 0x82, 0x00, 0x00, 0x10, 0x08, 0x04, 0x02, 0x41, 0x20, 0x80,
};
const struct IrCode code_na093Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_na093Times,
    code_na093Codes
};

const uint8_t code_na094Codes[] = {
    0x10,
    0x94,
    0x62,
    0x31,
    0x98,
    0x4A,
    0x31,
    0x18,
    0x00,
};
const struct IrCode code_na094Code = {
    freq_to_timerval(35714),
    22, // # of pairs
    3,  // # of bits per index
    code_na005Times,
    code_na094Codes
};

const uint16_t code_na095Times[] = {
    56,
    58,
    56,
    174,
    56,
    4549,
    56,
    9448,
    440,
    446,
};
const uint8_t code_na095Codes[] = {
    0x80,
    0x02,
    0x00,
    0x00,
    0x02,
    0x00,
    0x04,
    0x82,
    0x00,
    0x00,
    0x10,
    0x49,
    0x2A,
    0x17,
    0x08,
};
const struct IrCode code_na095Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_na095Times,
    code_na095Codes
};

const uint8_t code_na096Codes[] = {
    0x80,
    0x80,
    0x40,
    0x04,
    0x92,
    0x49,
    0x20,
    0x92,
    0x00,
    0x04,
    0x00,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na096Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_na096Codes
};

const uint8_t code_na097Codes[] = {
    0x84,
    0x80,
    0x00,
    0x24,
    0x10,
    0x41,
    0x00,
    0x80,
    0x01,
    0x24,
    0x12,
    0x48,
    0x0A,
    0xBA,
    0x40,
};
const struct IrCode code_na097Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_na097Codes
};

const uint8_t code_na098Codes[] = {
    0xA0,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x24,
    0x00,
    0x41,
    0x00,
    0x92,
    0x08,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na098Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na098Codes
};

const uint8_t code_na099Codes[] = {
    0x80,
    0x00,
    0x00,
    0x04,
    0x12,
    0x48,
    0x24,
    0x00,
    0x00,
    0x00,
    0x92,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na099Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_na099Codes
};

const uint16_t code_na100Times[] = {
    43,
    171,
    45,
    60,
    45,
    170,
    54,
    2301,
};
const uint8_t code_na100Codes[] = {
    0x29,
    0x59,
    0x65,
    0x55,
    0xEA,
    0x56,
    0x59,
    0x55,
    0x70,
};
const struct IrCode code_na100Code = {
    freq_to_timerval(35842),
    34, // # of pairs
    2,  // # of bits per index
    code_na100Times,
    code_na100Codes
};

const uint8_t code_na101Codes[] = {
    0xA0,
    0x00,
    0x09,
    0x04,
    0x92,
    0x40,
    0x20,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na101Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na101Codes
};

const uint16_t code_na102Times[] = {
    86,
    87,
    86,
    258,
    86,
    3338,
    346,
    348,
    348,
    347,
};
const uint8_t code_na102Codes[] = {
    0x64, 0x02, 0x08, 0x00, 0x02, 0x09, 0x04, 0x12, 0x49, 0x0A,
    0x10, 0x08, 0x20, 0x00, 0x08, 0x24, 0x10, 0x49, 0x24, 0x10,
};
const struct IrCode code_na102Code = {
    freq_to_timerval(40000),
    52, // # of pairs
    3,  // # of bits per index
    code_na102Times,
    code_na102Codes
};

const uint8_t code_na103Codes[] = {
    0x80,
    0x02,
    0x00,
    0x00,
    0x02,
    0x00,
    0x04,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0x97,
    0x48,
};
const struct IrCode code_na103Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_na045Times,
    code_na103Codes
};

const uint8_t code_na104Codes[] = {
    0xA4,
    0x00,
    0x49,
    0x00,
    0x92,
    0x00,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na104Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na104Codes
};

const uint8_t code_na105Codes[] = {
    0xA4,
    0x80,
    0x00,
    0x20,
    0x12,
    0x49,
    0x04,
    0x92,
    0x49,
    0x20,
    0x00,
    0x00,
    0x0A,
    0x38,
    0x40,
};
const struct IrCode code_na105Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na105Codes
};

const uint8_t code_na106Codes[] = {
    0x80,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x24,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na106Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na044Times,
    code_na106Codes
};

const uint8_t code_na107Codes[] = {
    0x80,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0x97,
    0x48,
};
const struct IrCode code_na107Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_na045Times,
    code_na107Codes
};

const uint8_t code_na108Codes[] = {
    0x80,
    0x90,
    0x40,
    0x00,
    0x90,
    0x40,
    0x04,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0x97,
    0x48,
};
const struct IrCode code_na108Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_na045Times,
    code_na108Codes
};

const uint16_t code_na109Times[] = {
    58,
    61,
    58,
    211,
    58,
    9582,
    73,
    4164,
    883,
    211,
    1050,
    494,
};
const uint8_t code_na109Codes[] = {
    0xA0,
    0x00,
    0x08,
    0x24,
    0x92,
    0x41,
    0x00,
    0x82,
    0x00,
    0x04,
    0x10,
    0x49,
    0x2E,
    0x28,
    0x00,
};
const struct IrCode code_na109Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na109Times,
    code_na109Codes
};

const uint8_t code_na110Codes[] = {
    0xA4,
    0x80,
    0x00,
    0x20,
    0x12,
    0x49,
    0x00,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na110Code = {
    freq_to_timerval(40161),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na110Codes
};

const uint8_t code_na111Codes[] = {
    0x84,
    0x92,
    0x49,
    0x20,
    0x00,
    0x00,
    0x04,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0xBA,
    0x40,
};
const struct IrCode code_na111Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na044Times,
    code_na111Codes
};

const uint8_t code_na112Codes[] = {
    0xA4,
    0x00,
    0x00,
    0x00,
    0x92,
    0x49,
    0x24,
    0x00,
    0x00,
    0x00,
    0x92,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_na112Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_na112Codes
};

const uint16_t code_na113Times[] = {
    56,
    54,
    56,
    166,
    56,
    3945,
    896,
    442,
    896,
    443,
};
const uint8_t code_na113Codes[] = {
    0x60, 0x00, 0x00, 0x20, 0x02, 0x09, 0x04, 0x02, 0x01, 0x00, 0x90, 0x48, 0x2A,
    0x00, 0x00, 0x00, 0x80, 0x08, 0x24, 0x10, 0x08, 0x04, 0x02, 0x41, 0x20, 0x80,
};
const struct IrCode code_na113Code = {
    freq_to_timerval(40000),
    68, // # of pairs
    3,  // # of bits per index
    code_na113Times,
    code_na113Codes
};

const uint16_t code_na114Times[] = {
    44,
    50,
    44,
    147,
    44,
    447,
    44,
    2236,
    791,
    398,
    793,
    397,
};
const uint8_t code_na114Codes[] = {
    0x84,
    0x10,
    0x40,
    0x08,
    0x82,
    0x08,
    0x01,
    0xD2,
    0x08,
    0x20,
    0x04,
    0x41,
    0x04,
    0x00,
    0x40,
};
const struct IrCode code_na114Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na114Times,
    code_na114Codes
};

const uint16_t code_na115Times[] = {
    81,
    86,
    81,
    296,
    81,
    3349,
    328,
    331,
    329,
    331,
};
const uint8_t code_na115Codes[] = {
    0x60, 0x82, 0x00, 0x20, 0x80, 0x41, 0x04, 0x90, 0x41, 0x2A,
    0x02, 0x08, 0x00, 0x82, 0x01, 0x04, 0x12, 0x41, 0x04, 0x80,
};
const struct IrCode code_na115Code = {
    freq_to_timerval(40000),
    52, // # of pairs
    3,  // # of bits per index
    code_na115Times,
    code_na115Codes
};

const uint8_t code_na116Codes[] = {
    0xA0,
    0x00,
    0x40,
    0x04,
    0x92,
    0x09,
    0x24,
    0x00,
    0x40,
    0x00,
    0x92,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na116Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na116Codes
};

const uint16_t code_na117Times[] = {
    49,
    54,
    49,
    158,
    49,
    420,
    49,
    2446,
    819,
    420,
    821,
    419,
};
const uint8_t code_na117Codes[] = {
    0x84,
    0x00,
    0x00,
    0x08,
    0x12,
    0x40,
    0x01,
    0xD2,
    0x00,
    0x00,
    0x04,
    0x09,
    0x20,
    0x00,
    0x40,
};
const struct IrCode code_na117Code = {
    freq_to_timerval(41667),
    38, // # of pairs
    3,  // # of bits per index
    code_na117Times,
    code_na117Codes
};

const uint8_t code_na118Codes[] = {
    0x84,
    0x90,
    0x49,
    0x20,
    0x02,
    0x00,
    0x04,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0xBA,
    0x40,
};
const struct IrCode code_na118Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na044Times,
    code_na118Codes
};

const uint16_t code_na119Times[] = {
    55,
    63,
    55,
    171,
    55,
    4094,
    55,
    9508,
    881,
    219,
    881,
    438,
};
const uint8_t code_na119Codes[] = {
    0xA0,
    0x10,
    0x00,
    0x04,
    0x82,
    0x49,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na119Code = {
    freq_to_timerval(55556),
    38, // # of pairs
    3,  // # of bits per index
    code_na119Times,
    code_na119Codes
};

const uint8_t code_na120Codes[] = {
    0xA0,
    0x12,
    0x00,
    0x04,
    0x80,
    0x49,
    0x24,
    0x92,
    0x40,
    0x00,
    0x00,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na120Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na120Codes
};

const uint8_t code_na121Codes[] = {
    0xA0,
    0x00,
    0x40,
    0x04,
    0x92,
    0x09,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na121Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na121Codes
};

const uint16_t code_na122Times[] = {
    80,
    95,
    80,
    249,
    80,
    3867,
    81,
    0,
    329,
    322,
};
const uint8_t code_na122Codes[] = {
    0x80,
    0x00,
    0x00,
    0x00,
    0x12,
    0x49,
    0x24,
    0x90,
    0x0A,
    0x80,
    0x00,
    0x00,
    0x00,
    0x12,
    0x49,
    0x24,
    0x90,
    0x0B,
};
const struct IrCode code_na122Code = {
    freq_to_timerval(52632),
    48, // # of pairs
    3,  // # of bits per index
    code_na122Times,
    code_na122Codes
};

const uint8_t code_na123Codes[] = {
    0xA0,
    0x02,
    0x48,
    0x04,
    0x90,
    0x01,
    0x20,
    0x12,
    0x40,
    0x04,
    0x80,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na123Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na123Codes
};

const uint16_t code_na124Times[] = {
    54,
    56,
    54,
    151,
    54,
    4092,
    54,
    8677,
    900,
    421,
    901,
    226,
};
const uint8_t code_na124Codes[] = {
    0x80,
    0x00,
    0x48,
    0x04,
    0x92,
    0x01,
    0x20,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_na124Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na124Times,
    code_na124Codes
};

const uint8_t code_na125Codes[] = {
    0xA0,
    0x02,
    0x48,
    0x04,
    0x90,
    0x01,
    0x20,
    0x80,
    0x40,
    0x04,
    0x12,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na125Code = {
    freq_to_timerval(55556),
    38, // # of pairs
    3,  // # of bits per index
    code_na119Times,
    code_na125Codes
};

const uint8_t code_na126Codes[] = {
    0xA4,
    0x10,
    0x00,
    0x20,
    0x82,
    0x49,
    0x00,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na126Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na126Codes
};

const uint16_t code_na127Times[] = {
    114,
    100,
    115,
    100,
    115,
    200,
    115,
    2706,
};
const uint8_t code_na127Codes[] = {
    0x1B,
    0x59,
};
const struct IrCode code_na127Code = {
    freq_to_timerval(25641),
    8, // # of pairs
    2, // # of bits per index
    code_na127Times,
    code_na127Codes
};

const uint8_t code_na128Codes[] = {
    0x60, 0x02, 0x08, 0x00, 0x02, 0x49, 0x04, 0x12, 0x49, 0x0A,
    0x00, 0x08, 0x20, 0x00, 0x09, 0x24, 0x10, 0x49, 0x24, 0x00,
};
const struct IrCode code_na128Code = {
    freq_to_timerval(40000),
    52, // # of pairs
    3,  // # of bits per index
    code_na102Times,
    code_na128Codes
};

const uint8_t code_na129Codes[] = {
    0xA4,
    0x92,
    0x49,
    0x20,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0x38,
    0x40,
};
const struct IrCode code_na129Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na017Times,
    code_na129Codes
};

const uint16_t code_na130Times[] = {
    88,
    90,
    88,
    258,
    88,
    2247,
    358,
    349,
    358,
    350,
};
const uint8_t code_na130Codes[] = {
    0x64, 0x00, 0x08, 0x24, 0x82, 0x09, 0x24, 0x10, 0x01, 0x0A,
    0x10, 0x00, 0x20, 0x92, 0x08, 0x24, 0x90, 0x40, 0x04, 0x10,
};
const struct IrCode code_na130Code = {
    freq_to_timerval(37037),
    52, // # of pairs
    3,  // # of bits per index
    code_na130Times,
    code_na130Codes
};

const uint8_t code_na131Codes[] = {
    0xA0,
    0x10,
    0x40,
    0x04,
    0x82,
    0x09,
    0x24,
    0x82,
    0x40,
    0x00,
    0x10,
    0x09,
    0x2A,
    0x38,
    0x00,
};
const struct IrCode code_na131Code = {
    freq_to_timerval(40000),
    38, // # of pairs
    3,  // # of bits per index
    code_na042Times,
    code_na131Codes
};

const uint16_t code_na132Times[] = {
    28,
    106,
    28,
    238,
    28,
    370,
    28,
    1173,
};
const uint8_t code_na132Codes[] = {
    0x22,
    0x20,
    0x00,
    0x17,
    0x22,
    0x20,
    0x00,
    0x14,
};
const struct IrCode code_na132Code = {
    freq_to_timerval(83333),
    32, // # of pairs
    2,  // # of bits per index
    code_na132Times,
    code_na132Codes
};

const uint16_t code_na133Times[] = {
    13,
    741,
    15,
    489,
    15,
    740,
    17,
    4641,
    18,
    0,
};
const uint8_t code_na133Codes[] = {
    0x09,
    0x24,
    0x49,
    0x48,
    0xB4,
    0x92,
    0x44,
    0x94,
    0x8C,
};
const struct IrCode code_na133Code = {
    freq_to_timerval(41667),
    24, // # of pairs
    3,  // # of bits per index
    code_na133Times,
    code_na133Codes
};

const uint8_t code_na134Codes[] = {
    0x60, 0x90, 0x00, 0x24, 0x10, 0x00, 0x04, 0x92, 0x00, 0x00, 0x00, 0x49, 0x2A,
    0x02, 0x40, 0x00, 0x90, 0x40, 0x00, 0x12, 0x48, 0x00, 0x00, 0x01, 0x24, 0x80,
};
const struct IrCode code_na134Code = {
    freq_to_timerval(40000),
    68, // # of pairs
    3,  // # of bits per index
    code_na113Times,
    code_na134Codes
};

const uint16_t code_na135Times[] = {
    53,
    59,
    53,
    171,
    53,
    2301,
    892,
    450,
    895,
    448,
};
const uint8_t code_na135Codes[] = {
    0x60, 0x12, 0x49, 0x00, 0x00, 0x09, 0x00, 0x00, 0x49, 0x24, 0x80, 0x00, 0x00, 0x12, 0x49, 0x24, 0xA8,
    0x01, 0x24, 0x90, 0x00, 0x00, 0x90, 0x00, 0x04, 0x92, 0x48, 0x00, 0x00, 0x01, 0x24, 0x92, 0x48,
};
const struct IrCode code_na135Code = {
    freq_to_timerval(38462),
    88, // # of pairs
    3,  // # of bits per index
    code_na135Times,
    code_na135Codes
};

const uint16_t code_na136Times[] = {
    53,
    59,
    53,
    171,
    53,
    2301,
    55,
    0,
    892,
    450,
    895,
    448,
};
const uint8_t code_na136Codes[] = {
    0x84, 0x82, 0x49, 0x00, 0x00, 0x00, 0x20, 0x00, 0x49, 0x24, 0x80, 0x00, 0x00, 0x12, 0x49, 0x24, 0xAA,
    0x48, 0x24, 0x90, 0x00, 0x00, 0x02, 0x00, 0x04, 0x92, 0x48, 0x00, 0x00, 0x01, 0x24, 0x92, 0x4B,
};
const struct IrCode code_na136Code = {
    freq_to_timerval(38610),
    88, // # of pairs
    3,  // # of bits per index
    code_na136Times,
    code_na136Codes
};

const uint16_t code_eu000Times[] = {
    43,
    47,
    43,
    91,
    43,
    8324,
    88,
    47,
    133,
    133,
    264,
    90,
    264,
    91,
};
const uint8_t code_eu000Codes[] = {
    0xA4,
    0x08,
    0x00,
    0x00,
    0x00,
    0x00,
    0x64,
    0x2C,
    0x40,
    0x80,
    0x00,
    0x00,
    0x00,
    0x06,
    0x41,
};
const struct IrCode code_eu000Code = {
    freq_to_timerval(35714),
    40, // # of pairs
    3,  // # of bits per index
    code_eu000Times,
    code_eu000Codes
};

const uint16_t code_eu001Times[] = {
    47,
    265,
    51,
    54,
    51,
    108,
    51,
    263,
    51,
    2053,
    51,
    11647,
    100,
    109,
};
const uint8_t code_eu001Codes[] = {
    0x04,
    0x92,
    0x49,
    0x26,
    0x35,
    0x89,
    0x24,
    0x9A,
    0xD6,
    0x24,
    0x92,
    0x48,
};
const struct IrCode code_eu001Code = {
    freq_to_timerval(30303),
    31, // # of pairs
    3,  // # of bits per index
    code_eu001Times,
    code_eu001Codes
};

const uint16_t code_eu002Times[] = {
    43,
    206,
    46,
    204,
    46,
    456,
    46,
    3488,
};
const uint8_t code_eu002Codes[] = {
    0x1A,
    0x56,
    0xA6,
    0xD6,
    0x95,
    0xA9,
    0x90,
};
const struct IrCode code_eu002Code = {
    freq_to_timerval(33333),
    26, // # of pairs
    2,  // # of bits per index
    code_eu002Times,
    code_eu002Codes
};

const uint16_t code_eu004Times[] = {
    44,
    45,
    44,
    131,
    44,
    7462,
    346,
    176,
    346,
    178,
};
const uint8_t code_eu004Codes[] = {
    0x60, 0x80, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x04,
    0x12, 0x48, 0x04, 0x12, 0x48, 0x2A, 0x02, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
    0x00, 0x00, 0x80, 0x00, 0x00, 0x10, 0x49, 0x20, 0x10, 0x49, 0x20, 0x80,
};
const struct IrCode code_eu004Code = {
    freq_to_timerval(37037),
    100, // # of pairs
    3,   // # of bits per index
    code_eu004Times,
    code_eu004Codes
};

const uint16_t code_eu005Times[] = {
    24,
    190,
    25,
    80,
    25,
    190,
    25,
    4199,
    25,
    4799,
};
const uint8_t code_eu005Codes[] = {
    0x04, 0x92, 0x52, 0x28, 0x92, 0x8C, 0x44, 0x92, 0x89, 0x45, 0x24, 0x53,
    0x44, 0x92, 0x52, 0x28, 0x92, 0x8C, 0x44, 0x92, 0x89, 0x45, 0x24, 0x51,
};
const struct IrCode code_eu005Code = {
    freq_to_timerval(38610),
    64, // # of pairs
    3,  // # of bits per index
    code_eu005Times,
    code_eu005Codes
};

const uint16_t code_eu006Times[] = {
    53,
    63,
    53,
    172,
    53,
    4472,
    54,
    0,
    455,
    468,
};
const uint8_t code_eu006Codes[] = {
    0x84, 0x90, 0x00, 0x04, 0x90, 0x00, 0x00, 0x80, 0x00, 0x04, 0x12, 0x49, 0x2A,
    0x12, 0x40, 0x00, 0x12, 0x40, 0x00, 0x02, 0x00, 0x00, 0x10, 0x49, 0x24, 0xB0,
};
const struct IrCode code_eu006Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_eu006Times,
    code_eu006Codes
};

const uint16_t code_eu007Times[] = {
    50,
    54,
    50,
    159,
    50,
    2307,
    838,
    422,
};
const uint8_t code_eu007Codes[] = {
    0xD4,
    0x00,
    0x15,
    0x10,
    0x25,
    0x00,
    0x05,
    0x44,
    0x09,
    0x40,
    0x01,
    0x51,
    0x01,
};
const struct IrCode code_eu007Code = {
    freq_to_timerval(38462),
    52, // # of pairs
    2,  // # of bits per index
    code_eu007Times,
    code_eu007Codes
};

const uint8_t code_eu008Codes[] = {
    0xA0,
    0x00,
    0x41,
    0x04,
    0x92,
    0x08,
    0x24,
    0x90,
    0x40,
    0x00,
    0x02,
    0x09,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu008Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu008Codes
};

const uint8_t code_eu011Codes[] = {
    0x84,
    0x00,
    0x48,
    0x04,
    0x02,
    0x01,
    0x04,
    0x80,
    0x09,
    0x00,
    0x12,
    0x40,
    0x2A,
    0xBA,
    0x40,
};
const struct IrCode code_eu011Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu011Codes
};

const uint16_t code_eu012Times[] = {
    46,
    206,
    46,
    459,
    46,
    3447,
};
const uint8_t code_eu012Codes[] = {
    0x05,
    0x01,
    0x51,
    0x81,
    0x40,
    0x54,
    0x40,
};
const struct IrCode code_eu012Code = {
    freq_to_timerval(33445),
    26, // # of pairs
    2,  // # of bits per index
    code_eu012Times,
    code_eu012Codes
};

const uint16_t code_eu013Times[] = {
    53,
    59,
    53,
    171,
    53,
    2302,
    895,
    449,
};
const uint8_t code_eu013Codes[] = {
    0xD4, 0x55, 0x00, 0x00, 0x40, 0x15, 0x54, 0x00, 0x01, 0x55, 0x56,
    0xD4, 0x55, 0x00, 0x00, 0x40, 0x15, 0x54, 0x00, 0x01, 0x55, 0x55,
};
const struct IrCode code_eu013Code = {
    freq_to_timerval(38462),
    88, // # of pairs
    2,  // # of bits per index
    code_eu013Times,
    code_eu013Codes
};

const uint16_t code_eu015Times[] = {
    53,
    54,
    53,
    156,
    53,
    2542,
    851,
    425,
    853,
    424,
};
const uint8_t code_eu015Codes[] = {
    0x60, 0x82, 0x08, 0x24, 0x10, 0x41, 0x00, 0x12, 0x40, 0x04, 0x80, 0x09, 0x2A, 0x02, 0x08, 0x20, 0x90,
    0x41, 0x04, 0x00, 0x49, 0x00, 0x12, 0x00, 0x24, 0xA8, 0x08, 0x20, 0x82, 0x41, 0x04, 0x10, 0x01, 0x24,
    0x00, 0x48, 0x00, 0x92, 0xA0, 0x20, 0x82, 0x09, 0x04, 0x10, 0x40, 0x04, 0x90, 0x01, 0x20, 0x02, 0x48,
};
const struct IrCode code_eu015Code = {
    freq_to_timerval(38462),
    136, // # of pairs
    3,   // # of bits per index
    code_eu015Times,
    code_eu015Codes
};

const uint16_t code_eu016Times[] = {
    28,
    92,
    28,
    213,
    28,
    214,
    28,
    2771,
};
const uint8_t code_eu016Codes[] = {
    0x68,
    0x08,
    0x20,
    0x00,
    0xEA,
    0x02,
    0x08,
    0x00,
    0x10,
};
const struct IrCode code_eu016Code = {
    freq_to_timerval(33333),
    34, // # of pairs
    2,  // # of bits per index
    code_eu016Times,
    code_eu016Codes
};

const uint16_t code_eu017Times[] = {
    15,
    844,
    16,
    557,
    16,
    844,
    16,
    5224,
};
const uint8_t code_eu017Codes[] = {
    0x1A,
    0x9A,
    0x9B,
    0x9A,
    0x9A,
    0x99,
};
const struct IrCode code_eu017Code = {
    freq_to_timerval(33333),
    24, // # of pairs
    2,  // # of bits per index
    code_eu017Times,
    code_eu017Codes
};

const uint8_t code_eu018Codes[] = {
    0xA0,
    0x02,
    0x48,
    0x04,
    0x90,
    0x01,
    0x20,
    0x12,
    0x40,
    0x04,
    0x80,
    0x09,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu018Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu018Codes
};

const uint16_t code_eu019Times[] = {
    50,
    54,
    50,
    158,
    50,
    418,
    50,
    2443,
    843,
    418,
};
const uint8_t code_eu019Codes[] = {
    0x80,
    0x80,
    0x00,
    0x08,
    0x12,
    0x40,
    0x01,
    0xC0,
    0x40,
    0x00,
    0x04,
    0x09,
    0x20,
    0x00,
    0x00,
};
const struct IrCode code_eu019Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_eu019Times,
    code_eu019Codes
};

const uint16_t code_eu020Times[] = {
    48,
    301,
    48,
    651,
    48,
    1001,
    48,
    3001,
};
const uint8_t code_eu020Codes[] = {
    0x22,
    0x20,
    0x00,
    0x01,
    0xC8,
    0x88,
    0x00,
    0x00,
    0x40,
};
const struct IrCode code_eu020Code = {
    freq_to_timerval(35714),
    34, // # of pairs
    2,  // # of bits per index
    code_eu020Times,
    code_eu020Codes
};

const uint8_t code_eu021Codes[] = {
    0x84,
    0x80,
    0x00,
    0x20,
    0x82,
    0x49,
    0x00,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2A,
    0xBA,
    0x40,
};
const struct IrCode code_eu021Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu021Codes
};

const uint8_t code_eu022Codes[] = {
    0xA4,
    0x80,
    0x41,
    0x00,
    0x12,
    0x08,
    0x24,
    0x90,
    0x40,
    0x00,
    0x02,
    0x09,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu022Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu022Codes
};

const uint8_t code_eu024Codes[] = {
    0xA0,
    0x02,
    0x48,
    0x04,
    0x90,
    0x01,
    0x20,
    0x00,
    0x40,
    0x04,
    0x92,
    0x09,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu024Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu024Codes
};

const uint16_t code_eu025Times[] = {
    49,
    52,
    49,
    102,
    49,
    250,
    49,
    252,
    49,
    2377,
    49,
    12009,
    100,
    52,
    100,
    102,
};
const uint8_t code_eu025Codes[] = {
    0x47,
    0x00,
    0x23,
    0x3C,
    0x01,
    0x59,
    0xE0,
    0x04,
};
const struct IrCode code_eu025Code = {
    freq_to_timerval(31250),
    21, // # of pairs
    3,  // # of bits per index
    code_eu025Times,
    code_eu025Codes
};

const uint16_t code_eu026Times[] = {
    14,
    491,
    14,
    743,
    14,
    4926,
};
const uint8_t code_eu026Codes[] = {
    0x55,
    0x40,
    0x42,
    0x55,
    0x40,
    0x41,
};
const struct IrCode code_eu026Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu026Times,
    code_eu026Codes
};

const uint8_t code_eu027Codes[] = {
    0xA0,
    0x82,
    0x08,
    0x24,
    0x10,
    0x41,
    0x04,
    0x10,
    0x01,
    0x20,
    0x82,
    0x48,
    0x0B,
    0x3D,
    0x00,
};
const struct IrCode code_eu027Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu027Codes
};

const uint16_t code_eu028Times[] = {
    47,
    267,
    50,
    55,
    50,
    110,
    50,
    265,
    50,
    2055,
    50,
    12117,
    100,
    57,
};
const uint8_t code_eu028Codes[] = {
    0x04,
    0x92,
    0x49,
    0x26,
    0x34,
    0x72,
    0x24,
    0x9A,
    0xD1,
    0xC8,
    0x92,
    0x48,
};
const struct IrCode code_eu028Code = {
    freq_to_timerval(30303),
    31, // # of pairs
    3,  // # of bits per index
    code_eu028Times,
    code_eu028Codes
};

const uint16_t code_eu029Times[] = {
    50,
    50,
    50,
    99,
    50,
    251,
    50,
    252,
    50,
    1445,
    50,
    11014,
    102,
    49,
    102,
    98,
};
const uint8_t code_eu029Codes[] = {
    0x47,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x64,
    0x62,
    0x00,
    0xE0,
    0x00,
    0x2B,
    0x23,
    0x10,
    0x07,
    0x00,
    0x00,
    0x80,
};
const struct IrCode code_eu029Code = {
    freq_to_timerval(34483),
    46, // # of pairs
    3,  // # of bits per index
    code_eu029Times,
    code_eu029Codes
};

const uint8_t code_eu030Codes[] = {
    0xA0,
    0x10,
    0x00,
    0x04,
    0x82,
    0x49,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu030Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu030Codes
};

const uint16_t code_eu031Times[] = {
    53,
    53,
    53,
    160,
    53,
    1697,
    838,
    422,
};
const uint8_t code_eu031Codes[] = {
    0xD5,
    0x50,
    0x15,
    0x11,
    0x65,
    0x54,
    0x05,
    0x44,
    0x59,
    0x55,
    0x01,
    0x51,
    0x15,
};
const struct IrCode code_eu031Code = {
    freq_to_timerval(38462),
    52, // # of pairs
    2,  // # of bits per index
    code_eu031Times,
    code_eu031Codes
};

const uint16_t code_eu032Times[] = {
    49,
    205,
    49,
    206,
    49,
    456,
    49,
    3690,
};
const uint8_t code_eu032Codes[] = {
    0x1A,
    0x56,
    0xA5,
    0xD6,
    0x95,
    0xA9,
    0x40,
};
const struct IrCode code_eu032Code = {
    freq_to_timerval(33333),
    26, // # of pairs
    2,  // # of bits per index
    code_eu032Times,
    code_eu032Codes
};

const uint16_t code_eu033Times[] = {
    48,
    150,
    50,
    149,
    50,
    347,
    50,
    2936,
};
const uint8_t code_eu033Codes[] = {
    0x2A,
    0x5D,
    0xA9,
    0x60,
};
const struct IrCode code_eu033Code = {
    freq_to_timerval(38462),
    14, // # of pairs
    2,  // # of bits per index
    code_eu033Times,
    code_eu033Codes
};

const uint8_t code_eu034Codes[] = {
    0xA0,
    0x02,
    0x40,
    0x04,
    0x90,
    0x09,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu034Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu034Codes
};

const uint8_t code_eu036Codes[] = {
    0xA4,
    0x00,
    0x49,
    0x00,
    0x92,
    0x00,
    0x20,
    0x02,
    0x00,
    0x04,
    0x90,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu036Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu036Codes
};

const uint16_t code_eu037Times[] = {
    14,
    491,
    14,
    743,
    14,
    5178,
};
const uint8_t code_eu037Codes[] = {
    0x45,
    0x50,
    0x02,
    0x45,
    0x50,
    0x01,
};
const struct IrCode code_eu037Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu037Times,
    code_eu037Codes
};

const uint16_t code_eu038Times[] = {
    3,
    1002,
    3,
    1495,
    3,
    3059,
};
const uint8_t code_eu038Codes[] = {
    0x05,
    0x60,
    0x54,
};
const struct IrCode code_eu038Code = {
    0,  // Non-pulsed code
    11, // # of pairs
    2,  // # of bits per index
    code_eu038Times,
    code_eu038Codes
};

const uint16_t code_eu039Times[] = {
    13,
    445,
    13,
    674,
    13,
    675,
    13,
    4583,
};
const uint8_t code_eu039Codes[] = {
    0x6A,
    0x82,
    0x83,
    0xAA,
    0x82,
    0x81,
};
const struct IrCode code_eu039Code = {
    freq_to_timerval(40161),
    24, // # of pairs
    2,  // # of bits per index
    code_eu039Times,
    code_eu039Codes
};

const uint16_t code_eu040Times[] = {
    85,
    89,
    85,
    264,
    85,
    3402,
    347,
    350,
    348,
    350,
};
const uint8_t code_eu040Codes[] = {
    0x60, 0x90, 0x40, 0x20, 0x80, 0x40, 0x20, 0x90, 0x41, 0x2A,
    0x02, 0x41, 0x00, 0x82, 0x01, 0x00, 0x82, 0x41, 0x04, 0x80,
};
const struct IrCode code_eu040Code = {
    freq_to_timerval(35714),
    52, // # of pairs
    3,  // # of bits per index
    code_eu040Times,
    code_eu040Codes
};

const uint16_t code_eu041Times[] = {
    46,
    300,
    49,
    298,
    49,
    648,
    49,
    997,
    49,
    3056,
};
const uint8_t code_eu041Codes[] = {
    0x0C,
    0xB2,
    0xCA,
    0x49,
    0x13,
    0x0B,
    0x2C,
    0xB2,
    0x92,
    0x44,
    0xB0,
};
const struct IrCode code_eu041Code = {
    freq_to_timerval(33333),
    28, // # of pairs
    3,  // # of bits per index
    code_eu041Times,
    code_eu041Codes
};

const uint8_t code_eu042Codes[] = {
    0x80,
    0x00,
    0x00,
    0x24,
    0x92,
    0x09,
    0x00,
    0x82,
    0x00,
    0x04,
    0x10,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_eu042Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu042Codes
};

const uint16_t code_eu043Times[] = {
    1037,
    4216,
    1040,
    0,
};
const uint8_t code_eu043Codes[] = {
    0x10,
};
const struct IrCode code_eu043Code = {
    freq_to_timerval(41667),
    2, // # of pairs
    2, // # of bits per index
    code_eu043Times,
    code_eu043Codes
};

const uint8_t code_eu044Codes[] = {
    0xA0,
    0x02,
    0x01,
    0x04,
    0x90,
    0x48,
    0x20,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu044Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu044Codes
};

const uint16_t code_eu045Times[] = {
    152,
    471,
    154,
    156,
    154,
    469,
    154,
    2947,
};
const uint8_t code_eu045Codes[] = {
    0x16,
    0xE5,
    0x90,
};
const struct IrCode code_eu045Code = {
    freq_to_timerval(41667),
    10, // # of pairs
    2,  // # of bits per index
    code_eu045Times,
    code_eu045Codes
};

const uint16_t code_eu046Times[] = {
    15,
    493,
    16,
    493,
    16,
    698,
    16,
    1414,
};
const uint8_t code_eu046Codes[] = {
    0x16,
    0xAB,
    0x56,
    0xA9,
};
const struct IrCode code_eu046Code = {
    freq_to_timerval(34602),
    16, // # of pairs
    2,  // # of bits per index
    code_eu046Times,
    code_eu046Codes
};

const uint16_t code_eu047Times[] = {
    3,
    496,
    3,
    745,
    3,
    1488,
};
const uint8_t code_eu047Codes[] = {
    0x41,
    0x24,
    0x12,
    0x41,
    0x00,
};
const struct IrCode code_eu047Code = {
    0,  // Non-pulsed code
    17, // # of pairs
    2,  // # of bits per index
    code_eu047Times,
    code_eu047Codes
};

const uint8_t code_eu048Codes[] = {
    0x80,
    0x00,
    0x00,
    0x24,
    0x82,
    0x49,
    0x04,
    0x80,
    0x40,
    0x00,
    0x12,
    0x09,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_eu048Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu048Codes
};

const uint16_t code_eu049Times[] = {
    55,
    55,
    55,
    167,
    55,
    4577,
    55,
    9506,
    448,
    445,
    450,
    444,
};
const uint8_t code_eu049Codes[] = {
    0x80,
    0x92,
    0x00,
    0x00,
    0x92,
    0x00,
    0x00,
    0x10,
    0x40,
    0x04,
    0x82,
    0x09,
    0x2A,
    0x97,
    0x48,
};
const struct IrCode code_eu049Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_eu049Times,
    code_eu049Codes
};

const uint16_t code_eu050Times[] = {
    91,
    88,
    91,
    267,
    91,
    3621,
    361,
    358,
    361,
    359,
};
const uint8_t code_eu050Codes[] = {
    0x60,
    0x00,
    0x00,
    0x00,
    0x12,
    0x49,
    0x24,
    0x92,
    0x42,
    0x80,
    0x00,
    0x00,
    0x00,
    0x12,
    0x49,
    0x24,
    0x92,
    0x40,
};
const struct IrCode code_eu050Code = {
    freq_to_timerval(33333),
    48, // # of pairs
    3,  // # of bits per index
    code_eu050Times,
    code_eu050Codes
};

const uint16_t code_eu051Times[] = {
    84,
    88,
    84,
    261,
    84,
    3360,
    347,
    347,
    347,
    348,
};
const uint8_t code_eu051Codes[] = {
    0x60, 0x82, 0x00, 0x20, 0x80, 0x41, 0x04, 0x90, 0x41, 0x2A,
    0x02, 0x08, 0x00, 0x82, 0x01, 0x04, 0x12, 0x41, 0x04, 0x80,
};
const struct IrCode code_eu051Code = {
    freq_to_timerval(38462),
    52, // # of pairs
    3,  // # of bits per index
    code_eu051Times,
    code_eu051Codes
};

const uint16_t code_eu052Times[] = {
    16,
    838,
    17,
    558,
    17,
    839,
    17,
    6328,
};
const uint8_t code_eu052Codes[] = {
    0x1A,
    0x9A,
    0x9B,
    0x9A,
    0x9A,
    0x99,
};
const struct IrCode code_eu052Code = {
    freq_to_timerval(31250),
    24, // # of pairs
    2,  // # of bits per index
    code_eu052Times,
    code_eu052Codes
};

const uint8_t code_eu053Codes[] = {
    0x26,
    0xAB,
    0x66,
    0xAA,
};
const struct IrCode code_eu053Code = {
    freq_to_timerval(34483),
    16, // # of pairs
    2,  // # of bits per index
    code_eu046Times,
    code_eu053Codes
};

const uint16_t code_eu054Times[] = {
    49,
    53,
    49,
    104,
    49,
    262,
    49,
    264,
    49,
    8030,
    100,
    103,
};
const uint8_t code_eu054Codes[] = {
    0x40,
    0x1A,
    0x23,
    0x00,
    0xD0,
    0x80,
};
const struct IrCode code_eu054Code = {
    freq_to_timerval(31250),
    14, // # of pairs
    3,  // # of bits per index
    code_eu054Times,
    code_eu054Codes
};

const uint8_t code_eu055Codes[] = {
    0x80,
    0x00,
    0x00,
    0x20,
    0x92,
    0x49,
    0x00,
    0x02,
    0x40,
    0x04,
    0x90,
    0x09,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_eu055Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu055Codes
};

const uint16_t code_eu056Times[] = {
    112,
    107,
    113,
    107,
    677,
    2766,
};
const uint8_t code_eu056Codes[] = {
    0x26,
};
const struct IrCode code_eu056Code = {
    freq_to_timerval(38462),
    4, // # of pairs
    2, // # of bits per index
    code_eu056Times,
    code_eu056Codes
};

const uint8_t code_eu058Codes[] = {
    0x80,
    0x00,
    0x00,
    0x24,
    0x10,
    0x49,
    0x00,
    0x82,
    0x00,
    0x04,
    0x10,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_eu058Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu058Codes
};

const uint16_t code_eu059Times[] = {
    310,
    613,
    310,
    614,
    622,
    8312,
};
const uint8_t code_eu059Codes[] = {
    0x26,
};
const struct IrCode code_eu059Code = {
    freq_to_timerval(41667),
    4, // # of pairs
    2, // # of bits per index
    code_eu059Times,
    code_eu059Codes
};

const uint16_t code_eu060Times[] = {
    50,
    158,
    53,
    51,
    53,
    156,
    53,
    2180,
};
const uint8_t code_eu060Codes[] = {
    0x25,
    0x59,
    0x9A,
    0x5A,
    0xE9,
    0x56,
    0x66,
    0x96,
    0xA0,
};
const struct IrCode code_eu060Code = {
    freq_to_timerval(38462),
    34, // # of pairs
    2,  // # of bits per index
    code_eu060Times,
    code_eu060Codes
};

const uint8_t code_eu061Codes[] = {
    0x10,
    0x92,
    0x54,
    0x24,
    0xB3,
    0x09,
    0x25,
    0x42,
    0x48,
};
const struct IrCode code_eu061Code = {
    freq_to_timerval(35714),
    24, // # of pairs
    3,  // # of bits per index
    code_na005Times,
    code_eu061Codes
};

const uint8_t code_eu062Codes[] = {
    0x25,
    0x99,
    0x9A,
    0x5A,
    0xE9,
    0x66,
    0x66,
    0x96,
    0xA0,
};
const struct IrCode code_eu062Code = {
    freq_to_timerval(38462),
    34, // # of pairs
    2,  // # of bits per index
    code_eu060Times,
    code_eu062Codes
};

const uint8_t code_eu063Codes[] = {
    0x80,
    0x00,
    0x00,
    0x24,
    0x90,
    0x41,
    0x00,
    0x82,
    0x00,
    0x04,
    0x10,
    0x49,
    0x2A,
    0xBA,
    0x00,
};
const struct IrCode code_eu063Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu063Codes
};

const uint16_t code_eu064Times[] = {
    47,
    267,
    50,
    55,
    50,
    110,
    50,
    265,
    50,
    2055,
    50,
    12117,
    100,
    57,
    100,
    112,
};
const uint8_t code_eu064Codes[] = {
    0x04,
    0x92,
    0x49,
    0x26,
    0x32,
    0x51,
    0xCB,
    0xD6,
    0x4A,
    0x39,
    0x72,
};
const struct IrCode code_eu064Code = {
    freq_to_timerval(30395),
    29, // # of pairs
    3,  // # of bits per index
    code_eu064Times,
    code_eu064Codes
};

const uint16_t code_eu065Times[] = {
    47,
    267,
    50,
    55,
    50,
    110,
    50,
    265,
    50,
    2055,
    50,
    12117,
    100,
    112,
};
const uint8_t code_eu065Codes[] = {
    0x04,
    0x92,
    0x49,
    0x26,
    0x32,
    0x4A,
    0x38,
    0x9A,
    0xC9,
    0x28,
    0xE2,
    0x48,
};
const struct IrCode code_eu065Code = {
    freq_to_timerval(30303),
    31, // # of pairs
    3,  // # of bits per index
    code_eu065Times,
    code_eu065Codes
};

const uint8_t code_eu066Codes[] = {
    0x84,
    0x82,
    0x00,
    0x04,
    0x82,
    0x00,
    0x00,
    0x82,
    0x00,
    0x04,
    0x10,
    0x49,
    0x2A,
    0x87,
    0x41,
};
const struct IrCode code_eu066Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_eu049Times,
    code_eu066Codes
};

const uint16_t code_eu067Times[] = {
    94,
    473,
    94,
    728,
    102,
    1637,
};
const uint8_t code_eu067Codes[] = {
    0x41,
    0x24,
    0x12,
};
const struct IrCode code_eu067Code = {
    freq_to_timerval(38462),
    12, // # of pairs
    2,  // # of bits per index
    code_eu067Times,
    code_eu067Codes
};

const uint16_t code_eu068Times[] = {
    49,
    263,
    50,
    54,
    50,
    108,
    50,
    263,
    50,
    2029,
    50,
    10199,
    100,
    110,
};
const uint8_t code_eu068Codes[] = {
    0x04,
    0x92,
    0x49,
    0x26,
    0x34,
    0x49,
    0x38,
    0x9A,
    0xD1,
    0x24,
    0xE2,
    0x48,
};
const struct IrCode code_eu068Code = {
    freq_to_timerval(38610),
    31, // # of pairs
    3,  // # of bits per index
    code_eu068Times,
    code_eu068Codes
};

const uint16_t code_eu069Times[] = {
    4,
    499,
    4,
    750,
    4,
    4999,
};
const uint8_t code_eu069Codes[] = {
    0x05,
    0x54,
    0x06,
    0x05,
    0x54,
    0x04,
};
const struct IrCode code_eu069Code = {
    0,  // Non-pulsed code
    23, // # of pairs
    2,  // # of bits per index
    code_eu069Times,
    code_eu069Codes
};

const uint8_t code_eu070Codes[] = {
    0x14,
    0x54,
    0x06,
    0x14,
    0x54,
    0x04,
};
const struct IrCode code_eu070Code = {
    0,  // Non-pulsed code
    23, // # of pairs
    2,  // # of bits per index
    code_eu069Times,
    code_eu070Codes
};

const uint16_t code_eu071Times[] = {
    14,
    491,
    14,
    743,
    14,
    4422,
};
const uint8_t code_eu071Codes[] = {
    0x45,
    0x44,
    0x56,
    0x45,
    0x44,
    0x55,
};
const struct IrCode code_eu071Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu071Times,
    code_eu071Codes
};

const uint16_t code_eu072Times[] = {
    5,
    568,
    5,
    854,
    5,
    4999,
};
const uint8_t code_eu072Codes[] = {
    0x55,
    0x45,
    0x46,
    0x55,
    0x45,
    0x44,
};
const struct IrCode code_eu072Code = {
    0,  // Non-pulsed code
    23, // # of pairs
    2,  // # of bits per index
    code_eu072Times,
    code_eu072Codes
};

const uint8_t code_eu073Codes[] = {
    0x19,
    0x57,
    0x59,
    0x55,
};
const struct IrCode code_eu073Code = {
    freq_to_timerval(34483),
    16, // # of pairs
    2,  // # of bits per index
    code_eu046Times,
    code_eu073Codes
};

const uint8_t code_eu074Codes[] = {
    0x04,
    0x92,
    0x49,
    0x28,
    0xC6,
    0x49,
    0x24,
    0x92,
    0x51,
    0x80,
};
const struct IrCode code_eu074Code = {
    freq_to_timerval(35714),
    26, // # of pairs
    3,  // # of bits per index
    code_na031Times,
    code_eu074Codes
};

const uint16_t code_eu075Times[] = {
    6,
    566,
    6,
    851,
    6,
    5474,
};
const uint8_t code_eu075Codes[] = {
    0x05,
    0x45,
    0x46,
    0x05,
    0x45,
    0x44,
};
const struct IrCode code_eu075Code = {
    0,  // Non-pulsed code
    23, // # of pairs
    2,  // # of bits per index
    code_eu075Times,
    code_eu075Codes
};

const uint16_t code_eu076Times[] = {
    14,
    843,
    16,
    555,
    16,
    841,
    16,
    4911,
};
const uint8_t code_eu076Codes[] = {
    0x2A,
    0x9A,
    0x9B,
    0xAA,
    0x9A,
    0x9A,
};
const struct IrCode code_eu076Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu076Times,
    code_eu076Codes
};

const uint8_t code_eu077Codes[] = {
    0x04,
    0x92,
    0x49,
    0x26,
    0x32,
    0x51,
    0xC8,
    0x9A,
    0xC9,
    0x47,
    0x22,
    0x48,
};
const struct IrCode code_eu077Code = {
    freq_to_timerval(30303),
    31, // # of pairs
    3,  // # of bits per index
    code_eu028Times,
    code_eu077Codes
};

const uint16_t code_eu078Times[] = {
    6,
    925,
    6,
    1339,
    6,
    2098,
    6,
    2787,
};
const uint8_t code_eu078Codes[] = {
    0x90,
    0x0D,
    0x00,
};
const struct IrCode code_eu078Code = {
    0,  // Non-pulsed code
    12, // # of pairs
    2,  // # of bits per index
    code_eu078Times,
    code_eu078Codes
};

const uint16_t code_eu079Times[] = {
    53,
    59,
    53,
    170,
    53,
    4359,
    892,
    448,
    893,
    448,
};
const uint8_t code_eu079Codes[] = {
    0x60, 0x00, 0x00, 0x24, 0x80, 0x09, 0x04, 0x92, 0x00, 0x00, 0x00, 0x49, 0x2A,
    0x00, 0x00, 0x00, 0x92, 0x00, 0x24, 0x12, 0x48, 0x00, 0x00, 0x01, 0x24, 0x80,
};
const struct IrCode code_eu079Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_eu079Times,
    code_eu079Codes
};

const uint16_t code_eu080Times[] = {
    55,
    57,
    55,
    167,
    55,
    4416,
    895,
    448,
    897,
    447,
};
const uint8_t code_eu080Codes[] = {
    0x60, 0x00, 0x00, 0x20, 0x10, 0x09, 0x04, 0x02, 0x01, 0x00, 0x90, 0x48, 0x2A,
    0x00, 0x00, 0x00, 0x80, 0x40, 0x24, 0x10, 0x08, 0x04, 0x02, 0x41, 0x20, 0x80,
};
const struct IrCode code_eu080Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_eu080Times,
    code_eu080Codes
};

const uint16_t code_eu081Times[] = {
    26,
    185,
    27,
    80,
    27,
    185,
    27,
    4249,
};
const uint8_t code_eu081Codes[] = {
    0x1A, 0x5A, 0x65, 0x67, 0x9A, 0x65, 0x9A, 0x9B, 0x9A, 0x5A,
    0x65, 0x67, 0x9A, 0x65, 0x9A, 0x9B, 0x9A, 0x5A, 0x65, 0x65,
};
const struct IrCode code_eu081Code = {
    freq_to_timerval(38462),
    80, // # of pairs
    2,  // # of bits per index
    code_eu081Times,
    code_eu081Codes
};

const uint16_t code_eu082Times[] = {
    51,
    56,
    51,
    162,
    51,
    2842,
    848,
    430,
    850,
    429,
};
const uint8_t code_eu082Codes[] = {
    0x60, 0x82, 0x08, 0x24, 0x10, 0x41, 0x04, 0x82, 0x40, 0x00, 0x10, 0x09, 0x2A,
    0x02, 0x08, 0x20, 0x90, 0x41, 0x04, 0x12, 0x09, 0x00, 0x00, 0x40, 0x24, 0x80,
};
const struct IrCode code_eu082Code = {
    freq_to_timerval(40000),
    68, // # of pairs
    3,  // # of bits per index
    code_eu082Times,
    code_eu082Codes
};

const uint16_t code_eu083Times[] = {
    16,
    559,
    16,
    847,
    16,
    5900,
    17,
    559,
    17,
    847,
};
const uint8_t code_eu083Codes[] = {
    0x0E,
    0x38,
    0x21,
    0x82,
    0x26,
    0x20,
    0x82,
    0x48,
    0x23,
};
const struct IrCode code_eu083Code = {
    freq_to_timerval(33333),
    24, // # of pairs
    3,  // # of bits per index
    code_eu083Times,
    code_eu083Codes
};

const uint16_t code_eu084Times[] = {
    16,
    484,
    16,
    738,
    16,
    739,
    16,
    4795,
};
const uint8_t code_eu084Codes[] = {
    0x6A,
    0xA0,
    0x03,
    0xAA,
    0xA0,
    0x01,
};
const struct IrCode code_eu084Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu084Times,
    code_eu084Codes
};

const uint16_t code_eu085Times[] = {
    48,
    52,
    48,
    160,
    48,
    400,
    48,
    2120,
    799,
    400,
};
const uint8_t code_eu085Codes[] = {
    0x84,
    0x82,
    0x40,
    0x08,
    0x92,
    0x48,
    0x01,
    0xC2,
    0x41,
    0x20,
    0x04,
    0x49,
    0x24,
    0x00,
    0x40,
};
const struct IrCode code_eu085Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_eu085Times,
    code_eu085Codes
};

const uint16_t code_eu086Times[] = {
    16,
    851,
    17,
    554,
    17,
    850,
    17,
    851,
    17,
    4847,
};
const uint8_t code_eu086Codes[] = {
    0x45,
    0x86,
    0x5B,
    0x05,
    0xC6,
    0x5B,
    0x05,
    0xB0,
    0x42,
};
const struct IrCode code_eu086Code = {
    freq_to_timerval(33333),
    24, // # of pairs
    3,  // # of bits per index
    code_eu086Times,
    code_eu086Codes
};

const uint16_t code_eu087Times[] = {
    14,
    491,
    14,
    743,
    14,
    5126,
};
const uint8_t code_eu087Codes[] = {
    0x55,
    0x50,
    0x02,
    0x55,
    0x50,
    0x01,
};
const struct IrCode code_eu087Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu087Times,
    code_eu087Codes
};

const uint16_t code_eu088Times[] = {
    14,
    491,
    14,
    743,
    14,
    4874,
};
const uint8_t code_eu088Codes[] = {
    0x45,
    0x54,
    0x42,
    0x45,
    0x54,
    0x41,
};
const struct IrCode code_eu088Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu088Times,
    code_eu088Codes
};

const uint8_t code_eu089Codes[] = {
    0x84,
    0x10,
    0x40,
    0x08,
    0x82,
    0x08,
    0x01,
    0xC2,
    0x08,
    0x20,
    0x04,
    0x41,
    0x04,
    0x00,
    0x40,
};
const struct IrCode code_eu089Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na021Times,
    code_eu089Codes
};

const uint16_t code_eu090Times[] = {
    3,
    9,
    3,
    19,
    3,
    29,
    3,
    39,
    3,
    9968,
};
const uint8_t code_eu090Codes[] = {
    0x60,
    0x00,
    0x88,
    0x00,
    0x02,
    0xE3,
    0x00,
    0x04,
    0x40,
    0x00,
    0x16,
};
const struct IrCode code_eu090Code = {
    0,  // Non-pulsed code
    29, // # of pairs
    3,  // # of bits per index
    code_eu090Times,
    code_eu090Codes
};

const uint16_t code_eu091Times[] = {
    15,
    138,
    15,
    446,
    15,
    605,
    15,
    6565,
};
const uint8_t code_eu091Codes[] = {
    0x80,
    0x01,
    0x00,
    0x2E,
    0x00,
    0x04,
    0x00,
    0xA0,
};
const struct IrCode code_eu091Code = {
    freq_to_timerval(38462),
    30, // # of pairs
    2,  // # of bits per index
    code_eu091Times,
    code_eu091Codes
};

const uint16_t code_eu092Times[] = {
    48,
    50,
    48,
    148,
    48,
    149,
    48,
    1424,
};
const uint8_t code_eu092Codes[] = {
    0x48,
    0x80,
    0x0E,
    0x22,
    0x00,
    0x10,
};
const struct IrCode code_eu092Code = {
    freq_to_timerval(40000),
    22, // # of pairs
    2,  // # of bits per index
    code_eu092Times,
    code_eu092Codes
};

const uint16_t code_eu093Times[] = {
    87,
    639,
    88,
    275,
    88,
    639,
};
const uint8_t code_eu093Codes[] = {
    0x15,
    0x9A,
    0x94,
};
const struct IrCode code_eu093Code = {
    freq_to_timerval(35714),
    11, // # of pairs
    2,  // # of bits per index
    code_eu093Times,
    code_eu093Codes
};

const uint16_t code_eu094Times[] = {
    3,
    8,
    3,
    18,
    3,
    24,
    3,
    38,
    3,
    9969,
};
const uint8_t code_eu094Codes[] = {
    0x60,
    0x80,
    0x88,
    0x00,
    0x00,
    0xE3,
    0x04,
    0x04,
    0x40,
    0x00,
    0x06,
};
const struct IrCode code_eu094Code = {
    0,  // Non-pulsed code
    29, // # of pairs
    3,  // # of bits per index
    code_eu094Times,
    code_eu094Codes
};

const uint8_t code_eu095Codes[] = {
    0x2A,
    0xAB,
    0x6A,
    0xAA,
};
const struct IrCode code_eu095Code = {
    freq_to_timerval(34483),
    16, // # of pairs
    2,  // # of bits per index
    code_eu046Times,
    code_eu095Codes
};

const uint16_t code_eu096Times[] = {
    13,
    608,
    14,
    141,
    14,
    296,
    14,
    451,
    14,
    606,
    14,
    608,
    14,
    6207,
};
const uint8_t code_eu096Codes[] = {
    0x04,
    0x94,
    0x4B,
    0x24,
    0x95,
    0x35,
    0x24,
    0xA2,
    0x59,
    0x24,
    0xA8,
    0x40,
};
const struct IrCode code_eu096Code = {
    freq_to_timerval(38462),
    30, // # of pairs
    3,  // # of bits per index
    code_eu096Times,
    code_eu096Codes
};

const uint8_t code_eu097Codes[] = {
    0x19,
    0xAB,
    0x59,
    0xA9,
};
const struct IrCode code_eu097Code = {
    freq_to_timerval(34483),
    16, // # of pairs
    2,  // # of bits per index
    code_eu046Times,
    code_eu097Codes
};

const uint16_t code_eu098Times[] = {
    3,
    8,
    3,
    18,
    3,
    28,
    3,
    12731,
};
const uint8_t code_eu098Codes[] = {
    0x80,
    0x01,
    0x00,
    0xB8,
    0x55,
    0x10,
    0x08,
};
const struct IrCode code_eu098Code = {
    0,  // Non-pulsed code
    27, // # of pairs
    2,  // # of bits per index
    code_eu098Times,
    code_eu098Codes
};

const uint16_t code_eu099Times[] = {
    46,
    53,
    46,
    106,
    46,
    260,
    46,
    1502,
    46,
    10962,
    93,
    53,
    93,
    106,
};
const uint8_t code_eu099Codes[] = {
    0x46,
    0x80,
    0x00,
    0x00,
    0x00,
    0x03,
    0x44,
    0x52,
    0x00,
    0x00,
    0x0C,
    0x22,
    0x22,
    0x90,
    0x00,
    0x00,
    0x60,
    0x80,
};
const struct IrCode code_eu099Code = {
    freq_to_timerval(35714),
    46, // # of pairs
    3,  // # of bits per index
    code_eu099Times,
    code_eu099Codes
};

const uint8_t code_eu100Codes[] = {
    0x80,
    0x04,
    0x00,
    0xB8,
    0x55,
    0x40,
    0x08,
};
const struct IrCode code_eu100Code = {
    0,  // Non-pulsed code
    27, // # of pairs
    2,  // # of bits per index
    code_eu098Times,
    code_eu100Codes
};

const uint16_t code_eu101Times[] = {
    14,
    491,
    14,
    743,
    14,
    4674,
};
const uint8_t code_eu101Codes[] = {
    0x55,
    0x50,
    0x06,
    0x55,
    0x50,
    0x05,
};
const struct IrCode code_eu101Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu101Times,
    code_eu101Codes
};

const uint8_t code_eu102Codes[] = {
    0x45,
    0x54,
    0x02,
    0x45,
    0x54,
    0x01,
};
const struct IrCode code_eu102Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu087Times,
    code_eu102Codes
};

const uint16_t code_eu103Times[] = {
    44,
    815,
    45,
    528,
    45,
    815,
    45,
    5000,
};
const uint8_t code_eu103Codes[] = {
    0x29,
    0x9A,
    0x9B,
    0xA9,
    0x9A,
    0x9A,
};
const struct IrCode code_eu103Code = {
    freq_to_timerval(34483),
    24, // # of pairs
    2,  // # of bits per index
    code_eu103Times,
    code_eu103Codes
};

const uint16_t code_eu104Times[] = {
    14,
    491,
    14,
    743,
    14,
    5881,
};
const uint8_t code_eu104Codes[] = {
    0x44,
    0x40,
    0x02,
    0x44,
    0x40,
    0x01,
};
const struct IrCode code_eu104Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu104Times,
    code_eu104Codes
};

const uint8_t code_eu105Codes[] = {
    0x84,
    0x10,
    0x00,
    0x20,
    0x90,
    0x01,
    0x00,
    0x80,
    0x40,
    0x04,
    0x12,
    0x09,
    0x2A,
    0xBA,
    0x40,
};
const struct IrCode code_eu105Code = {
    freq_to_timerval(38610),
    38, // # of pairs
    3,  // # of bits per index
    code_na009Times,
    code_eu105Codes
};

const uint16_t code_eu106Times[] = {
    48,
    246,
    50,
    47,
    50,
    94,
    50,
    245,
    50,
    1488,
    50,
    10970,
    100,
    47,
    100,
    94,
};
const uint8_t code_eu106Codes[] = {
    0x0B, 0x12, 0x49, 0x24, 0x92, 0x49, 0x8D, 0x1C, 0x89, 0x27, 0xFC, 0xAB,
    0x47, 0x22, 0x49, 0xFF, 0x2A, 0xD1, 0xC8, 0x92, 0x7F, 0xC9, 0x00,
};
const struct IrCode code_eu106Code = {
    freq_to_timerval(38462),
    59, // # of pairs
    3,  // # of bits per index
    code_eu106Times,
    code_eu106Codes
};

const uint16_t code_eu107Times[] = {
    16,
    847,
    16,
    5900,
    17,
    559,
    17,
    846,
    17,
    847,
};
const uint8_t code_eu107Codes[] = {
    0x62,
    0x08,
    0xA0,
    0x8A,
    0x19,
    0x04,
    0x08,
    0x40,
    0x83,
};
const struct IrCode code_eu107Code = {
    freq_to_timerval(33333),
    24, // # of pairs
    3,  // # of bits per index
    code_eu107Times,
    code_eu107Codes
};

const uint16_t code_eu108Times[] = {
    14,
    491,
    14,
    743,
    14,
    4622,
};
const uint8_t code_eu108Codes[] = {
    0x45,
    0x54,
    0x16,
    0x45,
    0x54,
    0x15,
};
const struct IrCode code_eu108Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu108Times,
    code_eu108Codes
};

const uint16_t code_eu109Times[] = {
    24,
    185,
    27,
    78,
    27,
    183,
    27,
    1542,
};
const uint8_t code_eu109Codes[] = {
    0x19,
    0x95,
    0x5E,
    0x66,
    0x55,
    0x50,
};
const struct IrCode code_eu109Code = {
    freq_to_timerval(38462),
    22, // # of pairs
    2,  // # of bits per index
    code_eu109Times,
    code_eu109Codes
};

const uint16_t code_eu110Times[] = {
    56,
    55,
    56,
    168,
    56,
    4850,
    447,
    453,
    448,
    453,
};
const uint8_t code_eu110Codes[] = {
    0x64, 0x10, 0x00, 0x04, 0x10, 0x00, 0x00, 0x80, 0x00, 0x04, 0x12, 0x49, 0x2A,
    0x10, 0x40, 0x00, 0x10, 0x40, 0x00, 0x02, 0x00, 0x00, 0x10, 0x49, 0x24, 0x90,
};
const struct IrCode code_eu110Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_eu110Times,
    code_eu110Codes
};

const uint16_t code_eu111Times[] = {
    49,
    52,
    49,
    250,
    49,
    252,
    49,
    2377,
    49,
    12009,
    100,
    52,
    100,
    102,
};
const uint8_t code_eu111Codes[] = {
    0x22,
    0x80,
    0x1A,
    0x18,
    0x01,
    0x10,
    0xC0,
    0x02,
};
const struct IrCode code_eu111Code = {
    freq_to_timerval(31250),
    21, // # of pairs
    3,  // # of bits per index
    code_eu111Times,
    code_eu111Codes
};

const uint16_t code_eu112Times[] = {
    55,
    55,
    55,
    167,
    55,
    5023,
    55,
    9506,
    448,
    445,
    450,
    444,
};
const uint8_t code_eu112Codes[] = {
    0x80,
    0x02,
    0x00,
    0x00,
    0x02,
    0x00,
    0x04,
    0x92,
    0x00,
    0x00,
    0x00,
    0x49,
    0x2A,
    0x97,
    0x48,
};
const struct IrCode code_eu112Code = {
    freq_to_timerval(38462),
    40, // # of pairs
    3,  // # of bits per index
    code_eu112Times,
    code_eu112Codes
};

const uint8_t code_eu113Codes[] = {
    0x46,
    0x80,
    0x23,
    0x34,
    0x00,
    0x80,
};
const struct IrCode code_eu113Code = {
    freq_to_timerval(31250),
    14, // # of pairs
    3,  // # of bits per index
    code_eu054Times,
    code_eu113Codes
};

const uint8_t code_eu114Codes[] = {
    0x04,
    0x92,
    0x49,
    0x26,
    0x34,
    0x71,
    0x44,
    0x9A,
    0xD1,
    0xC5,
    0x12,
    0x48,
};
const struct IrCode code_eu114Code = {
    freq_to_timerval(30303),
    31, // # of pairs
    3,  // # of bits per index
    code_eu028Times,
    code_eu114Codes
};

const uint16_t code_eu115Times[] = {
    48,
    98,
    48,
    196,
    97,
    836,
    395,
    388,
    1931,
    389,
};
const uint8_t code_eu115Codes[] = {
    0x84, 0x92, 0x01, 0x24, 0x12, 0x00, 0x04, 0x80, 0x08, 0x09, 0x92, 0x48, 0x04, 0x90, 0x48,
    0x00, 0x12, 0x00, 0x20, 0x26, 0x49, 0x20, 0x12, 0x41, 0x20, 0x00, 0x48, 0x00, 0x82,
};
const struct IrCode code_eu115Code = {
    freq_to_timerval(58824),
    77, // # of pairs
    3,  // # of bits per index
    code_eu115Times,
    code_eu115Codes
};

const uint16_t code_eu116Times[] = {
    3,
    9,
    3,
    31,
    3,
    42,
    3,
    10957,
};
const uint8_t code_eu116Codes[] = {
    0x80,
    0x01,
    0x00,
    0x2E,
    0x00,
    0x04,
    0x00,
    0x80,
};
const struct IrCode code_eu116Code = {
    0,  // Non-pulsed code
    29, // # of pairs
    2,  // # of bits per index
    code_eu116Times,
    code_eu116Codes
};

const uint16_t code_eu117Times[] = {
    49,
    53,
    49,
    262,
    49,
    264,
    49,
    8030,
    100,
    103,
};
const uint8_t code_eu117Codes[] = {
    0x22,
    0x00,
    0x1A,
    0x10,
    0x00,
    0x40,
};
const struct IrCode code_eu117Code = {
    freq_to_timerval(31250),
    14, // # of pairs
    3,  // # of bits per index
    code_eu117Times,
    code_eu117Codes
};

const uint16_t code_eu118Times[] = {
    44,
    815,
    45,
    528,
    45,
    815,
    45,
    4713,
};
const uint8_t code_eu118Codes[] = {
    0x2A,
    0x9A,
    0x9B,
    0xAA,
    0x9A,
    0x9A,
};
const struct IrCode code_eu118Code = {
    freq_to_timerval(34483),
    24, // # of pairs
    2,  // # of bits per index
    code_eu118Times,
    code_eu118Codes
};

const uint16_t code_eu119Times[] = {
    14,
    491,
    14,
    743,
    14,
    5430,
};
const uint8_t code_eu119Codes[] = {
    0x44,
    0x44,
    0x02,
    0x44,
    0x44,
    0x01,
};
const struct IrCode code_eu119Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu119Times,
    code_eu119Codes
};

const uint16_t code_eu120Times[] = {
    19,
    78,
    21,
    27,
    21,
    77,
    21,
    3785,
    22,
    0,
};
const uint8_t code_eu120Codes[] = {
    0x09, 0x24, 0x92, 0x49, 0x12, 0x4A, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x94, 0x89, 0x69,
    0x24, 0x92, 0x49, 0x22, 0x49, 0x44, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x91, 0x30,
};
const struct IrCode code_eu120Code = {
    freq_to_timerval(38462),
    82, // # of pairs
    3,  // # of bits per index
    code_eu120Times,
    code_eu120Codes
};

const uint8_t code_eu121Codes[] = {
    0x64, 0x00, 0x09, 0x24, 0x00, 0x09, 0x24, 0x00, 0x09, 0x2A,
    0x10, 0x00, 0x24, 0x90, 0x00, 0x24, 0x90, 0x00, 0x24, 0x90,
};
const struct IrCode code_eu121Code = {
    freq_to_timerval(38462),
    52, // # of pairs
    3,  // # of bits per index
    code_eu051Times,
    code_eu121Codes
};

const uint8_t code_eu122Codes[] = {
    0x04, 0xA4, 0x92, 0x49, 0x22, 0x49, 0x48, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x94, 0x89, 0x68,
    0x94, 0x92, 0x49, 0x24, 0x49, 0x29, 0x12, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x91, 0x30,
};
const struct IrCode code_eu122Code = {
    freq_to_timerval(38462),
    82, // # of pairs
    3,  // # of bits per index
    code_eu120Times,
    code_eu122Codes
};

const uint16_t code_eu123Times[] = {
    13,
    490,
    13,
    741,
    13,
    742,
    13,
    5443,
};
const uint8_t code_eu123Codes[] = {
    0x6A,
    0xA0,
    0x0B,
    0xAA,
    0xA0,
    0x09,
};
const struct IrCode code_eu123Code = {
    freq_to_timerval(40000),
    24, // # of pairs
    2,  // # of bits per index
    code_eu123Times,
    code_eu123Codes
};

const uint16_t code_eu124Times[] = {
    50,
    54,
    50,
    158,
    50,
    407,
    50,
    2153,
    843,
    407,
};
const uint8_t code_eu124Codes[] = {
    0x80,
    0x10,
    0x40,
    0x08,
    0x92,
    0x48,
    0x01,
    0xC0,
    0x08,
    0x20,
    0x04,
    0x49,
    0x24,
    0x00,
    0x00,
};
const struct IrCode code_eu124Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_eu124Times,
    code_eu124Codes
};

const uint16_t code_eu125Times[] = {
    55,
    56,
    55,
    168,
    55,
    3929,
    56,
    0,
    882,
    454,
    884,
    452,
};
const uint8_t code_eu125Codes[] = {
    0x84, 0x80, 0x00, 0x20, 0x82, 0x49, 0x00, 0x02, 0x00, 0x04, 0x90, 0x49, 0x2A,
    0x92, 0x00, 0x00, 0x82, 0x09, 0x24, 0x00, 0x08, 0x00, 0x12, 0x41, 0x24, 0xB0,
};
const struct IrCode code_eu125Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    3,  // # of bits per index
    code_eu125Times,
    code_eu125Codes
};

const uint8_t code_eu126Codes[] = {
    0xA0,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x20,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu126Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu126Codes
};

const uint8_t code_eu127Codes[] = {
    0x44,
    0x40,
    0x56,
    0x44,
    0x40,
    0x55,
};
const struct IrCode code_eu127Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu087Times,
    code_eu127Codes
};

const uint16_t code_eu128Times[] = {
    152,
    471,
    154,
    156,
    154,
    469,
    154,
    782,
    154,
    2947,
};
const uint8_t code_eu128Codes[] = {
    0x05,
    0xC4,
    0x59,
};
const struct IrCode code_eu128Code = {
    freq_to_timerval(41667),
    8, // # of pairs
    3, // # of bits per index
    code_eu128Times,
    code_eu128Codes
};

const uint16_t code_eu129Times[] = {
    50,
    50,
    50,
    99,
    50,
    251,
    50,
    252,
    50,
    1449,
    50,
    11014,
    102,
    49,
    102,
    98,
};
const uint8_t code_eu129Codes[] = {
    0x47,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x8C,
    0x8C,
    0x40,
    0x03,
    0xF1,
    0xEB,
    0x23,
    0x10,
    0x00,
    0xFC,
    0x74,
};
const struct IrCode code_eu129Code = {
    freq_to_timerval(38462),
    45, // # of pairs
    3,  // # of bits per index
    code_eu129Times,
    code_eu129Codes
};

const uint8_t code_eu130Codes[] = {
    0x47,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x8C,
    0x8C,
    0x40,
    0x03,
    0xE3,
    0xEB,
    0x23,
    0x10,
    0x00,
    0xF8,
    0xF4,
};
const struct IrCode code_eu130Code = {
    freq_to_timerval(38462),
    45, // # of pairs
    3,  // # of bits per index
    code_eu129Times,
    code_eu130Codes
};

const uint16_t code_eu131Times[] = {
    14,
    491,
    14,
    743,
    14,
    4170,
};
const uint8_t code_eu131Codes[] = {
    0x55,
    0x55,
    0x42,
    0x55,
    0x55,
    0x41,
};
const struct IrCode code_eu131Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu131Times,
    code_eu131Codes
};

const uint8_t code_eu132Codes[] = {
    0x05,
    0x50,
    0x06,
    0x05,
    0x50,
    0x04,
};
const struct IrCode code_eu132Code = {
    0,  // Non-pulsed code
    23, // # of pairs
    2,  // # of bits per index
    code_eu069Times,
    code_eu132Codes
};

const uint8_t code_eu133Codes[] = {
    0x55,
    0x54,
    0x12,
    0x55,
    0x54,
    0x11,
};
const struct IrCode code_eu133Code = {
    freq_to_timerval(38462),
    24, // # of pairs
    2,  // # of bits per index
    code_eu071Times,
    code_eu133Codes
};

const uint16_t code_eu134Times[] = {
    13,
    490,
    13,
    741,
    13,
    742,
    13,
    5939,
};
const uint8_t code_eu134Codes[] = {
    0x40,
    0x0A,
    0x83,
    0x80,
    0x0A,
    0x81,
};
const struct IrCode code_eu134Code = {
    freq_to_timerval(40000),
    24, // # of pairs
    2,  // # of bits per index
    code_eu134Times,
    code_eu134Codes
};

const uint16_t code_eu135Times[] = {
    6,
    566,
    6,
    851,
    6,
    5188,
};
const uint8_t code_eu135Codes[] = {
    0x54,
    0x45,
    0x46,
    0x54,
    0x45,
    0x44,
};
const struct IrCode code_eu135Code = {
    0,  // Non-pulsed code
    23, // # of pairs
    2,  // # of bits per index
    code_eu135Times,
    code_eu135Codes
};

const uint8_t code_eu136Codes[] = {
    0xA0,
    0x00,
    0x00,
    0x04,
    0x92,
    0x49,
    0x24,
    0x00,
    0x00,
    0x00,
    0x92,
    0x49,
    0x2B,
    0x3D,
    0x00,
};
const struct IrCode code_eu136Code = {
    freq_to_timerval(38462),
    38, // # of pairs
    3,  // # of bits per index
    code_na004Times,
    code_eu136Codes
};

const uint16_t code_eu137Times[] = {
    86,
    91,
    87,
    90,
    87,
    180,
    87,
    8868,
    88,
    0,
    174,
    90,
};
const uint8_t code_eu137Codes[] = {
    0x14,
    0x95,
    0x4A,
    0x35,
    0x9A,
    0x4A,
    0xA5,
    0x1B,
    0x00,
};
const struct IrCode code_eu137Code = {
    freq_to_timerval(35714),
    22, // # of pairs
    3,  // # of bits per index
    code_eu137Times,
    code_eu137Codes
};

const uint16_t code_eu138Times[] = {
    4,
    1036,
    4,
    1507,
    4,
    3005,
};
const uint8_t code_eu138Codes[] = {
    0x05,
    0x60,
    0x54,
};
const struct IrCode code_eu138Code = {
    0,  // Non-pulsed code
    11, // # of pairs
    2,  // # of bits per index
    code_eu138Times,
    code_eu138Codes
};

const uint16_t code_eu139Times[] = {
    0,
    0,
    14,
    141,
    14,
    452,
    14,
    607,
    14,
    6310,
};
const uint8_t code_eu139Codes[] = {
    0x64,
    0x92,
    0x4A,
    0x24,
    0x92,
    0xE3,
    0x24,
    0x92,
    0x51,
    0x24,
    0x96,
    0x00,
};
const struct IrCode code_eu139Code = {
    0,  // Non-pulsed code
    30, // # of pairs
    3,  // # of bits per index
    code_eu139Times,
    code_eu139Codes
};

const uint16_t code_eu140Times[] = {
    448,
    448,
    56,
    168,
    56,
    56,
    56,
    4526,
};
const uint8_t code_eu140Codes[] = {
    0x15,
    0xAA,
    0x95,
    0xAA,
    0xAA,
    0x5A,
    0x55,
    0xA5,
    0xB1,
    0x5A,
    0xA9,
    0x5A,
    0xAA,
    0xA5,
    0xA5,
    0x5A,
    0x5B,
};
const struct IrCode code_eu140Code = {
    freq_to_timerval(38462),
    68, // # of pairs
    2,  // # of bits per index
    code_eu140Times,
    code_eu140Codes
};

////////////////////////////////////////////////////////////////

// const array (called "NApowerCodes") of const pointers to IrCode structs
const IrCode *const NApowerCodes[] = {
    &code_na000Code, &code_na001Code, &code_na002Code, &code_na003Code, &code_na004Code, &code_na005Code,
    &code_na006Code, &code_na007Code, &code_na008Code, &code_na009Code, &code_na010Code, &code_na011Code,
    &code_na012Code, &code_na013Code, &code_na014Code, &code_na015Code, &code_na016Code, &code_na017Code,
    &code_na018Code, &code_na019Code, &code_na020Code, &code_na021Code, &code_na022Code, &code_na023Code,
    &code_na024Code, &code_na025Code, &code_na026Code, &code_na027Code, &code_na028Code, &code_na029Code,
    &code_na030Code, &code_na031Code, &code_na032Code, &code_na033Code, &code_na034Code, &code_na035Code,
    &code_na036Code, &code_na037Code, &code_na038Code, &code_na039Code, &code_na040Code, &code_na041Code,
    &code_na042Code, &code_na043Code, &code_na044Code, &code_na045Code, &code_na046Code, &code_na047Code,
    &code_na048Code, &code_na049Code, &code_na050Code, &code_na051Code, &code_na052Code, &code_na053Code,
    &code_na054Code, &code_na055Code, &code_na056Code, &code_na057Code, &code_na058Code, &code_na059Code,
    &code_na060Code, &code_na061Code, &code_na062Code, &code_na063Code, &code_na064Code, &code_na065Code,
    &code_na066Code, &code_na067Code, &code_na068Code, &code_na069Code, &code_na070Code, &code_na071Code,
    &code_na072Code, &code_na073Code, &code_na074Code, &code_na075Code, &code_na076Code, &code_na077Code,
    &code_na078Code, &code_na079Code, &code_na080Code, &code_na081Code, &code_na082Code, &code_na083Code,
    &code_na084Code, &code_na085Code, &code_na086Code, &code_na087Code, &code_na088Code, &code_na089Code,
    &code_na090Code, &code_na091Code, &code_na092Code, &code_na093Code, &code_na094Code, &code_na095Code,
    &code_na096Code, &code_na097Code, &code_na098Code, &code_na099Code, &code_na100Code, &code_na101Code,
    &code_na102Code, &code_na103Code, &code_na104Code, &code_na105Code, &code_na106Code, &code_na107Code,
    &code_na108Code, &code_na109Code, &code_na110Code, &code_na111Code, &code_na112Code, &code_na113Code,
    &code_na114Code, &code_na115Code, &code_na116Code, &code_na117Code, &code_na118Code, &code_na119Code,
    &code_na120Code, &code_na121Code, &code_na122Code, &code_na123Code, &code_na124Code, &code_na125Code,
    &code_na126Code, &code_na127Code, &code_na128Code, &code_na129Code, &code_na130Code, &code_na131Code,
    &code_na132Code, &code_na133Code, &code_na134Code, &code_na135Code, &code_na136Code,
};

const IrCode *const EUpowerCodes[] = {
    &code_eu000Code,
    &code_eu001Code,
    &code_eu002Code,
    &code_na000Code,
    &code_eu004Code,
    &code_eu005Code,
    &code_eu006Code,
    &code_eu007Code,
    &code_eu008Code,
    &code_na005Code,
    &code_na004Code,
    &code_eu011Code,
    &code_eu012Code,
    &code_eu013Code,
    &code_na021Code,
    &code_eu015Code,
    &code_eu016Code,
    &code_eu017Code,
    &code_eu018Code,
    &code_eu019Code,
    &code_eu020Code,
    &code_eu021Code,
    &code_eu022Code,
    &code_na022Code,
    &code_eu024Code,
    &code_eu025Code,
    &code_eu026Code,
    &code_eu140Code,
    &code_eu027Code,
    &code_eu028Code,
    &code_eu029Code,
    &code_eu030Code,
    &code_eu031Code,
    &code_eu032Code,
    &code_eu033Code,
    &code_eu034Code,
    &code_eu036Code,
    &code_eu037Code,
    &code_eu038Code,
    &code_eu039Code,
    &code_eu040Code,
    &code_eu041Code,
    &code_eu042Code,
    &code_eu043Code,
    &code_eu044Code,
    &code_eu045Code,
    &code_eu046Code,
    &code_eu047Code,
    &code_eu048Code,
    &code_eu049Code,
    &code_eu050Code,
    &code_eu051Code,
    &code_eu052Code,
    &code_eu053Code,
    &code_eu054Code,
    &code_eu055Code,
    &code_eu056Code,
    &code_eu058Code,
    &code_eu059Code,
    &code_eu060Code,
    &code_eu061Code,
    &code_eu062Code,
    &code_eu063Code,
    &code_eu064Code,
    &code_eu065Code,
    &code_eu066Code,
    &code_eu067Code,
    &code_eu068Code,
    &code_eu069Code,
    &code_eu070Code,
    &code_eu071Code,
    &code_eu072Code,
    &code_eu073Code,
    &code_eu074Code,
    &code_eu075Code,
    &code_eu076Code,
    &code_eu077Code,
    &code_eu078Code,
    &code_eu079Code,
    &code_eu080Code,
    &code_eu081Code,
    &code_eu082Code,
    &code_eu083Code,
    &code_eu084Code,
    &code_eu085Code,
    &code_eu086Code,
    &code_eu087Code,
    &code_eu088Code,
    &code_eu089Code,
    &code_eu090Code,
    &code_eu091Code,
    &code_eu092Code,
    &code_eu093Code,
    &code_eu094Code,
    &code_eu095Code,
    &code_eu096Code,
    &code_eu097Code,
    &code_eu098Code,
    &code_eu099Code,
    &code_eu100Code,
    &code_eu101Code,
    &code_eu102Code,
    &code_eu103Code,
    &code_eu104Code,
    &code_eu105Code,
    &code_eu106Code,
    &code_eu107Code,
    &code_eu108Code,
    &code_eu109Code,
    &code_eu110Code,
    &code_eu111Code,
    &code_eu112Code,
    &code_eu113Code,
    &code_eu114Code,
    &code_eu115Code,
    &code_eu116Code,
    &code_eu117Code,
    &code_eu118Code,
    &code_eu119Code,
    &code_eu120Code,
    &code_eu121Code,
    &code_eu122Code,
    &code_eu123Code,
    &code_eu124Code,
    &code_eu125Code,
    &code_eu126Code,
    &code_eu127Code,
    &code_eu128Code,
    &code_eu129Code,
    &code_eu130Code,
    &code_eu131Code,
    &code_eu132Code,
    &code_eu133Code,
    &code_eu134Code,
    &code_eu135Code,
    &code_eu136Code,
    &code_eu137Code,
    &code_eu138Code,
    &code_eu139Code,
};
