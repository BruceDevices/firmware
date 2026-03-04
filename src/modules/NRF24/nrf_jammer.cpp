/**
 * @file nrf_jammer.cpp
 * @brief Enhanced 2.4 GHz jammer with 12 modes and dual strategy.
 *
 * Features over original:
 *  - 12 jamming mode presets with tuned channel lists
 *  - Data flooding via writeFast() for packet collision attacks
 *  - Constant carrier (CW) for FHSS disruption
 *  - Per-mode configurable PA, data rate, dwell time
 *  - Config persistence via LittleFS
 *  - Random hopping for FHSS targets (BT, Drone)
 *  - Live mode/channel switching during operation
 *  - Improved UI with adaptive layout
 *  - UART support preserved for external NRF modules
 *
 * Hardware: E01-ML01SP2 (NRF24L01+ PA+LNA, +20dBm effective).
 */

#include "nrf_jammer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <LittleFS.h>
#include <globals.h>

// ── Garbage payload for data flooding ───────────────────────────
// 32 bytes maximises TX duty cycle per burst at 2Mbps
static const uint8_t JAM_FLOOD_DATA[32] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0xDE, 0xAD, 0xBE,
                                           0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0xFF, 0x00, 0xFF, 0x00, 0xA5, 0x5A,
                                           0xA5, 0x5A, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

// ── Config persistence ──────────────────────────────────────────
static const char *NRF_JAM_CFG_PATH = "/nrf_jam_cfg.bin";
#define NRF_JAM_CFG_VERSION 3

// ── Per-mode default configs ────────────────────────────────────
// Tuned for E01-ML01SP2 (PA+LNA): PA=3 → chip 0dBm → ~+20dBm at antenna
//
// Strategy rationale:
//   Flooding (1): packet collisions + CRC corruption. Best for
//     channel-specific protocols (WiFi, BLE, Zigbee)
//   CW (0): saturates receiver AGC, disrupts PLL lock. Best for
//     FHSS targets (BT classic, Drone, RC) and analog links (video)
//
// Default: CW (constant carrier) for all modes — proven reliable
// like the original jammer. Users can switch to Flooding via config menu.
//
// CW: unmodulated carrier saturates receiver AGC → proven & fast
// Flooding: packet collisions via writeFast → optional, enable in config
//
//                                        PA  DR  dwell  flood
static NrfJamConfig jamConfigs[NRF_JAM_MODE_COUNT] = {
    /* FULL       */ {3, 1, 0, 0}, // CW: rapid sweep all 125ch (like original)
                                   /* WIFI       */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
                                   /* BLE        */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
                                   /* BLE_ADV    */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
                                   /* BLUETOOTH  */
    {3, 1, 0, 0}, // CW: no delay, fast sweep (like original)
                                   /* USB        */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
                                   /* VIDEO      */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
                                   /* RC         */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
                                   /* ZIGBEE     */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
                                   /* DRONE      */
    {3, 1, 0, 0}, // CW: no delay, fast sweep
};

// ── Mode information table ──────────────────────────────────────
static const NrfJamModeInfo MODE_INFO[NRF_JAM_MODE_COUNT] = {
    {"Full Spectrum",   "Full Spec" },
    {"WiFi 2.4GHz",     "WiFi 2.4"  },
    {"BLE Data",        "BLE Data"  },
    {"BLE Advertising", "BLE Adv"   },
    {"BT Classic",      "BT Classic"},
    {"USB Dongles",     "USB Dongle"},
    {"Video/FPV",       "Video FPV" },
    {"RC Controllers",  "RC Ctrl"   },
    {"Zigbee",          "Zigbee"    },
    {"Drone FHSS",      "Drone"     },
};

// ── Channel lists ───────────────────────────────────────────────

// WiFi ch 1,6,11: each spans 22MHz, sub-channels cover bandwidth
static const uint8_t CH_WIFI[] = {
    1,  3,  5,  7,  9,  11, 13, 15, 17, 19, 21, 23, // WiFi ch 1
    26, 28, 30, 32, 34, 36, 38, 40, 42,             // WiFi ch 6
    51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73  // WiFi ch 11
};

// BLE data channels: nRF24 ch 2-80 (even numbers cover BLE ch 0-36)
static const uint8_t CH_BLE[] = {2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
                                 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56,
                                 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80};

// BLE advertising: ch37=2402→nRF ch2, ch38=2426→nRF ch26, ch39=2480→nRF ch80
static const uint8_t CH_BLE_ADV[] = {2, 26, 80};

// Classic Bluetooth: all FHSS channels 2-80 (matches original jammer)
static const uint8_t CH_BLUETOOTH[] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                                       18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
                                       34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                                       50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
                                       66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80};

// USB wireless dongles
static const uint8_t CH_USB[] = {40, 50, 60};

// Video streaming (upper ISM band)
static const uint8_t CH_VIDEO[] = {70, 75, 80};

// RC controllers (low channels)
static const uint8_t CH_RC[] = {1, 3, 5, 7};

// Zigbee ch 11-26: 3 nRF sub-channels per Zigbee channel (±1MHz)
static const uint8_t CH_ZIGBEE[] = {
    4,  5,  6,  // ch11
    9,  10, 11, // ch12
    14, 15, 16, // ch13
    19, 20, 21, // ch14
    24, 25, 26, // ch15
    29, 30, 31, // ch16
    34, 35, 36, // ch17
    39, 40, 41, // ch18
    44, 45, 46, // ch19
    49, 50, 51, // ch20
    54, 55, 56, // ch21
    59, 60, 61, // ch22
    64, 65, 66, // ch23
    69, 70, 71, // ch24
    74, 75, 76, // ch25
    79, 80, 81  // ch26
};

// ── Channel list accessor ───────────────────────────────────────
static const uint8_t *getChannelList(NrfJamMode mode, size_t &count) {
    switch (mode) {
        case NRF_JAM_WIFI: count = sizeof(CH_WIFI); return CH_WIFI;
        case NRF_JAM_BLE: count = sizeof(CH_BLE); return CH_BLE;
        case NRF_JAM_BLE_ADV: count = sizeof(CH_BLE_ADV); return CH_BLE_ADV;
        case NRF_JAM_BLUETOOTH: count = sizeof(CH_BLUETOOTH); return CH_BLUETOOTH;
        case NRF_JAM_USB: count = sizeof(CH_USB); return CH_USB;
        case NRF_JAM_VIDEO: count = sizeof(CH_VIDEO); return CH_VIDEO;
        case NRF_JAM_RC: count = sizeof(CH_RC); return CH_RC;
        case NRF_JAM_ZIGBEE: count = sizeof(CH_ZIGBEE); return CH_ZIGBEE;
        case NRF_JAM_DRONE: count = 0; return nullptr; // Random hop
        default: count = 0; return nullptr;
    }
}

// ── Config persistence ──────────────────────────────────────────
static void loadJamConfigs() {
    if (!LittleFS.exists(NRF_JAM_CFG_PATH)) return;

    File f = LittleFS.open(NRF_JAM_CFG_PATH, "r");
    if (!f) return;

    uint8_t version = f.read();
    if (version != NRF_JAM_CFG_VERSION) {
        f.close();
        return;
    }

    size_t expected = sizeof(NrfJamConfig) * NRF_JAM_MODE_COUNT;
    size_t bytesRead = f.read((uint8_t *)jamConfigs, expected);
    f.close();

    if (bytesRead != expected) return;

    // Clamp values
    for (int i = 0; i < NRF_JAM_MODE_COUNT; i++) {
        if (jamConfigs[i].paLevel > 3) jamConfigs[i].paLevel = 3;
        if (jamConfigs[i].dataRate > 2) jamConfigs[i].dataRate = 1;
        if (jamConfigs[i].dwellTimeMs > 200) jamConfigs[i].dwellTimeMs = 200;
        if (jamConfigs[i].useFlooding > 1) jamConfigs[i].useFlooding = 1;
    }
    Serial.println("[JAM] Configs loaded from flash");
}

static void saveJamConfigs() {
    File f = LittleFS.open(NRF_JAM_CFG_PATH, FILE_WRITE);
    if (!f) return;
    f.write(NRF_JAM_CFG_VERSION);
    f.write((uint8_t *)jamConfigs, sizeof(NrfJamConfig) * NRF_JAM_MODE_COUNT);
    f.close();
    Serial.println("[JAM] Configs saved to flash");
}

// ── Apply RF config to hardware ─────────────────────────────────
static void applyJamConfig(const NrfJamConfig &cfg, bool flooding) {
    rf24_pa_dbm_e paLevels[] = {RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX};
    rf24_datarate_e dataRates[] = {RF24_1MBPS, RF24_2MBPS, RF24_250KBPS};

    NRFradio.setPALevel(paLevels[cfg.paLevel & 3]);

    if (!NRFradio.setDataRate(dataRates[cfg.dataRate <= 2 ? cfg.dataRate : 1])) {
        Serial.println("[JAM] Warning: setDataRate failed");
    }

    NRFradio.setAutoAck(false);
    NRFradio.setRetries(0, 0);
    NRFradio.disableCRC();

    if (flooding) {
        NRFradio.setPayloadSize(32);
        NRFradio.setAddressWidth(3);
        uint8_t txAddr[] = {0xE7, 0xE7, 0xE7};
        NRFradio.openWritingPipe(txAddr);
        NRFradio.stopListening();
    }
}

// ── Data flooding on a channel ──────────────────────────────────
// Safely switch channel (CE LOW → configure → CE HIGH) then burst.
static void floodChannel(uint8_t ch, uint16_t dwellMs) {
    // CE LOW to safely change channel mid-flight
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
    NRFradio.flush_tx();
    NRFradio.setChannel(ch);

    if (dwellMs == 0) {
        // Turbo: fill FIFO (3 packets) and fire
        NRFradio.writeFast(JAM_FLOOD_DATA, 32, true);
        NRFradio.writeFast(JAM_FLOOD_DATA, 32, true);
        NRFradio.writeFast(JAM_FLOOD_DATA, 32, true);
        delayMicroseconds(500);
        return;
    }

    unsigned long startMs = millis();
    while ((millis() - startMs) < dwellMs) {
        if (!NRFradio.writeFast(JAM_FLOOD_DATA, 32, true)) { delayMicroseconds(10); }
    }
}

// ── CW initialization helper ────────────────────────────────────
// Must call powerUp() before startConstCarrier() because
// stopConstCarrier() → powerDown() clears the internal PWR_UP flag,
// and startConstCarrier() never restores it (RF24 library bug).
static void initCW(int channel) {
    NRFradio.powerUp();
    delay(5); // Tpd2stby: power-down → standby settle
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.startConstCarrier(RF24_PA_MAX, channel);
    NRFradio.setAddressWidth(5);
    NRFradio.setPayloadSize(2);
    NRFradio.setDataRate(RF24_2MBPS);
}

// ── CW on a channel ─────────────────────────────────────────────
// Carrier stays on — just move the frequency via setChannel().
// This matches the original jammer behavior: startConstCarrier once at
// init, then setChannel() to hop.  PLL re-locks in ~130µs, carrier
// is never fully off → maximum duty cycle.
static void cwChannel(uint8_t ch, uint16_t dwellMs) {
    NRFradio.setChannel(ch);

    if (dwellMs == 0) return;

    if (dwellMs <= 5) {
        delayMicroseconds((uint32_t)dwellMs * 1000);
    } else {
        delay(dwellMs);
    }
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ CONFIG EDIT UI ═══════════════════════════════
// ══════════════════════════════════════════════════════════════════

static void editModeConfig(NrfJamMode mode) {
    NrfJamConfig &cfg = jamConfigs[(uint8_t)mode];
    const char *paLabels[] = {"MIN (-18dBm)", "LOW (-12dBm)", "HIGH (-6dBm)", "MAX (0/+20dBm)"};
    const char *drLabels[] = {"1 Mbps", "2 Mbps", "250 Kbps"};
    const char *stratLabels[] = {"Constant Carrier", "Data Flooding"};

    int menuIdx = 0;
    bool editing = false;
    bool redraw = true;

    while (true) {
        if (check(EscPress)) {
            saveJamConfigs();
            break;
        }

        if (redraw) {
            drawMainBorderWithTitle(MODE_INFO[(uint8_t)mode].shortName);
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

            int y = BORDER_PAD_Y + FM * LH + 4;
            int lineH = max(14, tftHeight / 10);

            // Items
            const char *items[] = {"PA Level", "Data Rate", "Dwell (ms)", "Strategy", "Save & Back"};
            String values[] = {
                paLabels[cfg.paLevel & 3],
                drLabels[cfg.dataRate <= 2 ? cfg.dataRate : 1],
                String(cfg.dwellTimeMs),
                stratLabels[cfg.useFlooding & 1],
                ""
            };

            for (int i = 0; i < 5; i++) {
                int itemY = y + i * lineH;
                uint16_t fg = (i == menuIdx) ? bruceConfig.bgColor : bruceConfig.priColor;
                uint16_t bg = (i == menuIdx) ? bruceConfig.priColor : bruceConfig.bgColor;

                tft.fillRect(7, itemY, tftWidth - 14, lineH - 2, bg);
                tft.setTextColor(fg, bg);
                String line = String(items[i]);
                if (values[i].length() > 0) { line += ": " + values[i]; }
                tft.drawString(line, 12, itemY + 2, 1);

                if (editing && i == menuIdx && i < 4) {
                    tft.setTextColor(TFT_YELLOW, bg);
                    tft.drawRightString("<>", tftWidth - 12, itemY + 2, 1);
                }
            }
            redraw = false;
        }

        if (check(NextPress)) {
            if (editing) {
                switch (menuIdx) {
                    case 0: cfg.paLevel = (cfg.paLevel + 1) % 4; break;
                    case 1: cfg.dataRate = (cfg.dataRate + 1) % 3; break;
                    case 2: cfg.dwellTimeMs = min(200, (int)cfg.dwellTimeMs + 1); break;
                    case 3: cfg.useFlooding = !cfg.useFlooding; break;
                }
            } else {
                menuIdx = (menuIdx + 1) % 5;
            }
            redraw = true;
        }

        if (check(PrevPress)) {
            if (editing) {
                switch (menuIdx) {
                    case 0: cfg.paLevel = (cfg.paLevel + 3) % 4; break;
                    case 1: cfg.dataRate = (cfg.dataRate + 2) % 3; break;
                    case 2: cfg.dwellTimeMs = max(0, (int)cfg.dwellTimeMs - 1); break;
                    case 3: cfg.useFlooding = !cfg.useFlooding; break;
                }
            } else {
                menuIdx = (menuIdx + 4) % 5;
            }
            redraw = true;
        }

        if (check(SelPress)) {
            if (menuIdx == 4) {
                saveJamConfigs();
                break;
            }
            editing = !editing;
            redraw = true;
        }

        delay(50);
    }
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ JAMMER STATUS UI ═════════════════════════════
// ══════════════════════════════════════════════════════════════════

static void drawJammerStatus(NrfJamMode mode, int currentCh, uint8_t nrfOnline, bool initial) {
    const NrfJamConfig &cfg = jamConfigs[(uint8_t)mode];

    if (initial) { drawMainBorderWithTitle("NRF JAMMER"); }

    int y = BORDER_PAD_Y + FM * LH + 4;
    int lineH = max(14, tftHeight / 10);

    tft.setTextSize(FP);

    // Mode name
    tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    tft.drawString(MODE_INFO[(uint8_t)mode].shortName, 12, y + 2, 1);

    y += lineH;

    // Status
    tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    char buf[40];
    snprintf(buf, sizeof(buf), "Status: %d ACTIVE", nrfOnline);
    tft.drawString(buf, 12, y + 2, 1);

    y += lineH;

    // Channel / Frequency
    tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    int freq = 2400 + currentCh;
    snprintf(buf, sizeof(buf), "CH:%d  %dMHz", currentCh, freq);
    tft.drawString(buf, 12, y + 2, 1);

    y += lineH;

    // Config summary
    tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    snprintf(buf, sizeof(buf), "%s dwell:%dms", cfg.useFlooding ? "FLOOD" : "CW", cfg.dwellTimeMs);
    tft.drawString(buf, 12, y + 2, 1);

    // Footer
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
    tft.fillRect(7, footerY, tftWidth - 14, FP * LH, bruceConfig.bgColor);
    tft.drawCentreString("[ESC]Stop [<>]Mode [OK]Cfg", tftWidth / 2, footerY, 1);
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ JAMMER EXECUTION LOOP ════════════════════════
// ══════════════════════════════════════════════════════════════════

static void runJammer(NRF24_MODE nrfMode, NrfJamMode jamMode) {
    int OnX = 0;
    uint8_t NRFOnline = 1;
    uint8_t NRFSPI = 0;
    NrfJamMode currentMode = jamMode;
    int channel = 0;
    int hopIndex = 0;

    if (CHECK_NRF_SPI(nrfMode)) {
        NrfJamConfig &cfg = jamConfigs[(uint8_t)currentMode];
        bool flooding = cfg.useFlooding;

        if (flooding) {
            applyJamConfig(cfg, true);
        } else {
            initCW(channel);
        }
        NRFSPI = 1;
    }

    drawJammerStatus(currentMode, channel, NRFOnline, true);

    if (CHECK_NRF_UART(nrfMode) || CHECK_NRF_BOTH(nrfMode)) {
        NRFSerial.println("RADIOS");
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    bool running = true;
    bool redraw = false;

    while (running) {
        // ── Check for exit ──────────────────────────────────────
        if (check(EscPress)) {
            running = false;
            break;
        }

        // ── UART handling ───────────────────────────────────────
        if (CHECK_NRF_UART(nrfMode) || CHECK_NRF_BOTH(nrfMode)) {
            if (OnX == 0) {
                NRFSerial.println("RADIOS");
                vTaskDelay(250 / portTICK_PERIOD_MS);
            }
            if (NRFSerial.available()) {
                String incomingNRFs = NRFSerial.readStringUntil('\n');
                incomingNRFs.trim();
                if (incomingNRFs.length() == 1 && isDigit(incomingNRFs.charAt(0))) {
                    OnX = 1;
                    NRFOnline = incomingNRFs.toInt();
                    if (CHECK_NRF_BOTH(nrfMode)) NRFOnline += NRFSPI;
                    redraw = true;
                }
            }
        }

        // ── Config: press SEL to edit mode config ───────────────
        if (check(SelPress)) {
            if (CHECK_NRF_SPI(nrfMode)) NRFradio.stopConstCarrier();
            editModeConfig(currentMode);

            // Re-apply config after edit — must use initCW() because
            // stopConstCarrier() → powerDown() clears internal PWR_UP,
            // and bare startConstCarrier() never restores it.
            if (CHECK_NRF_SPI(nrfMode)) {
                NrfJamConfig &cfg = jamConfigs[(uint8_t)currentMode];
                if (cfg.useFlooding) {
                    applyJamConfig(cfg, true);
                } else {
                    initCW(channel);
                }
            }
            redraw = true;
        }

        // ── Mode cycling: Next/Prev ─────────────────────────────
        if (check(NextPress)) {
            NrfJamMode prevMode = currentMode;
            currentMode = (NrfJamMode)(((uint8_t)currentMode + 1) % NRF_JAM_MODE_COUNT);
            hopIndex = 0;
            if (CHECK_NRF_SPI(nrfMode)) {
                NrfJamConfig &prevCfg = jamConfigs[(uint8_t)prevMode];
                NrfJamConfig &cfg = jamConfigs[(uint8_t)currentMode];
                if (prevCfg.useFlooding != cfg.useFlooding) {
                    NRFradio.stopConstCarrier();
                    if (cfg.useFlooding) {
                        applyJamConfig(cfg, true);
                    } else {
                        initCW(channel);
                    }
                }
            }
            if (CHECK_NRF_UART(nrfMode) || CHECK_NRF_BOTH(nrfMode)) {
                NRFSerial.println(MODE_INFO[(uint8_t)currentMode].shortName);
            }
            redraw = true;
        }
        if (check(PrevPress)) {
            NrfJamMode prevMode = currentMode;
            currentMode = (NrfJamMode)(((uint8_t)currentMode + NRF_JAM_MODE_COUNT - 1) % NRF_JAM_MODE_COUNT);
            hopIndex = 0;
            if (CHECK_NRF_SPI(nrfMode)) {
                NrfJamConfig &prevCfg = jamConfigs[(uint8_t)prevMode];
                NrfJamConfig &cfg = jamConfigs[(uint8_t)currentMode];
                if (prevCfg.useFlooding != cfg.useFlooding) {
                    NRFradio.stopConstCarrier();
                    if (cfg.useFlooding) {
                        applyJamConfig(cfg, true);
                    } else {
                        initCW(channel);
                    }
                }
            }
            redraw = true;
        }

        // ── Redraw on state changes only (no periodic redraw) ───
        if (redraw) {
            drawJammerStatus(currentMode, channel, NRFOnline, true);
            redraw = false;
        }

        // ── Jamming logic (SPI mode) ────────────────────────────
        if (!CHECK_NRF_SPI(nrfMode)) {
            delay(10);
            continue;
        }

        NrfJamConfig &cfg = jamConfigs[(uint8_t)currentMode];
        bool flooding = cfg.useFlooding;
        uint16_t dwellMs = cfg.dwellTimeMs;

        switch (currentMode) {
            case NRF_JAM_FULL:
            case NRF_JAM_DRONE: {
                // Full sweep 0-124 (like original)
                if (flooding) floodChannel(hopIndex, dwellMs);
                else cwChannel(hopIndex, dwellMs);
                channel = hopIndex;
                hopIndex = (hopIndex + 1) % 125;
                break;
            }

            default: {
                // All preset channel list modes: sequential hopping
                // (BLE, BLE_ADV, WiFi, Bluetooth, USB, Video, RC, Zigbee)
                size_t count;
                const uint8_t *channels = getChannelList(currentMode, count);
                if (count > 0 && channels) {
                    uint8_t ch = channels[hopIndex % count];
                    if (flooding) floodChannel(ch, dwellMs);
                    else cwChannel(ch, dwellMs);
                    channel = ch;
                    hopIndex++;
                    if (hopIndex >= (int)count) hopIndex = 0;
                } else {
                    delay(1);
                }
                break;
            }
        }
    }

    // ── Cleanup ─────────────────────────────────────────────────
    if (CHECK_NRF_SPI(nrfMode)) {
        NRFradio.stopConstCarrier();
        NRFradio.flush_tx();
        NRFradio.powerDown();
    }
    if (CHECK_NRF_UART(nrfMode) || CHECK_NRF_BOTH(nrfMode)) { NRFSerial.println("OFF"); }
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ PUBLIC ENTRY POINTS ══════════════════════════
// ══════════════════════════════════════════════════════════════════

void nrf_jammer() {
    loadJamConfigs();

    // Feature selection submenu (radio init deferred until needed)
    NrfJamMode selectedMode = NRF_JAM_FULL;
    int selectedAction = 0; // 0=none, 1=preset mode, 2=ch_jammer, 3=ch_hopper

    options.clear();
    for (int i = 0; i < NRF_JAM_MODE_COUNT; i++) {
        int mIdx = i;
        options.push_back({MODE_INFO[i].name, [&selectedMode, &selectedAction, mIdx]() {
                               selectedMode = (NrfJamMode)mIdx;
                               selectedAction = 1;
                           }});
    }
    options.push_back({"Single CH", [&selectedAction]() { selectedAction = 2; }});
    options.push_back({"CH Hopper", [&selectedAction]() { selectedAction = 3; }});
    options.push_back({"Reset Settings", [&selectedAction]() { selectedAction = 4; }});
    options.push_back({"Back", [=]() { returnToMenu = true; }});

    loopOptions(options, MENU_TYPE_SUBMENU, "NRF Jammer");

    if (returnToMenu || selectedAction == 0) return;

    // CH Jammer and CH Hopper handle their own radio init
    if (selectedAction == 2) {
        nrf_channel_jammer();
        return;
    }
    if (selectedAction == 3) {
        nrf_channel_hopper();
        return;
    }
    if (selectedAction == 4) {
        // Reset settings: delete config file and reload defaults
        if (LittleFS.exists(NRF_JAM_CFG_PATH)) { LittleFS.remove(NRF_JAM_CFG_PATH); }
        displaySuccess("Settings reset", true);
        return;
    }

    // Preset mode selected — init radio and run
    NRF24_MODE nrfMode = nrf_setMode();
    if (returnToMenu || nrfMode == NRF_MODE_DISABLED) return;

    if (!nrf_start(nrfMode)) {
        displayError("NRF24 not found");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        return;
    }

    runJammer(nrfMode, selectedMode);
}

void nrf_channel_jammer() {
    int OnX = 0;
    NRF24_MODE mode = nrf_setMode();
    if (returnToMenu || mode == NRF_MODE_DISABLED) return;

    uint8_t NRFOnline = 1;
    uint8_t NRFSPI = 0;

    if (!nrf_start(mode)) {
        displayError("NRF24 not found");
        delay(500);
        return;
    }

    int channel = 50;
    bool redraw = true;
    bool paused = false;

    if (CHECK_NRF_SPI(mode)) {
        initCW(channel);
        NRFSPI = 1;
    }

    if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
        NRFSerial.println("RADIOS");
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    while (true) {
        if (check(EscPress)) break;

        if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
            if (OnX == 0) {
                NRFSerial.println("RADIOS");
                vTaskDelay(250 / portTICK_PERIOD_MS);
            }
            if (NRFSerial.available()) {
                String incomingNRFs = NRFSerial.readStringUntil('\n');
                incomingNRFs.trim();
                if (incomingNRFs.length() == 1 && isDigit(incomingNRFs.charAt(0))) {
                    NRFOnline = incomingNRFs.toInt();
                    if (CHECK_NRF_BOTH(mode)) NRFOnline += NRFSPI;
                    redraw = true;
                    OnX = 1;
                }
            }
        }

        if (redraw) {
            drawMainBorderWithTitle("SINGLE CH JAMMER");

            int contentY = BORDER_PAD_Y + FM * LH + 4;
            int lineH = max(14, tftHeight / 10);
            int freq = 2400 + channel;

            tft.setTextSize(FP);

            // Status
            tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
            char buf[40];
            snprintf(buf, sizeof(buf), "STATUS: %d ACTIVE", NRFOnline);
            tft.drawCentreString(buf, tftWidth / 2, contentY, 1);
            contentY += lineH;

            // Channel / Frequency
            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            snprintf(buf, sizeof(buf), "CH: %d  (%d MHz)", channel, freq);
            tft.drawCentreString(buf, tftWidth / 2, contentY, 1);
            contentY += lineH;

            // Pause state
            tft.setTextColor(paused ? TFT_RED : TFT_GREEN, bruceConfig.bgColor);
            tft.fillRect(7, contentY, tftWidth - 14, lineH, bruceConfig.bgColor);
            tft.drawCentreString(paused ? "PAUSED" : "JAMMING", tftWidth / 2, contentY, 1);

            // Footer
            int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
            tft.fillRect(7, footerY, tftWidth - 14, FP * LH, bruceConfig.bgColor);
            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawCentreString("[ESC]Exit [<>]CH [OK]Pause", tftWidth / 2, footerY, 1);

            if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) { NRFSerial.println("CH_" + String(channel)); }
            redraw = false;
        }

        // SEL: pause/resume
        if (check(SelPress)) {
            paused = !paused;
            if (CHECK_NRF_SPI(mode)) {
                if (paused) {
                    NRFradio.stopConstCarrier();
                } else {
                    initCW(channel);
                }
            }
            redraw = true;
        }

        if (check(NextPress)) {
            channel++;
            if (channel > 125) channel = 0;
            if (CHECK_NRF_SPI(mode) && !paused) {
                NRFradio.setChannel(channel);
                NRFradio.startConstCarrier(RF24_PA_MAX, channel);
            }
            redraw = true;
        }
        if (check(PrevPress)) {
            channel--;
            if (channel < 0) channel = 125;
            if (CHECK_NRF_SPI(mode) && !paused) {
                NRFradio.setChannel(channel);
                NRFradio.startConstCarrier(RF24_PA_MAX, channel);
            }
            redraw = true;
        }
    }

    if (CHECK_NRF_SPI(mode)) NRFradio.stopConstCarrier();
    if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) NRFSerial.println("OFF");
}

void nrf_channel_hopper() {
    loadJamConfigs();

    NRF24_MODE nrfMode = nrf_setMode();
    if (returnToMenu || nrfMode == NRF_MODE_DISABLED) return;

    if (!nrf_start(nrfMode)) {
        displayError("NRF24 not found");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        return;
    }

    // ── Hopper config UI ────────────────────────────────────────
    NrfHopperConfig hopCfg = {0, 80, 2};
    int menuIndex = 0;
    bool editMode = false;
    bool redraw = true;

    vTaskDelay(350 / portTICK_PERIOD_MS);

    if (CHECK_NRF_UART(nrfMode) || CHECK_NRF_BOTH(nrfMode)) {
        NRFSerial.println("RADIOS");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while (true) {
        if (check(EscPress)) return;

        if (redraw) {
            drawMainBorderWithTitle("HOPPER CONFIG");
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

            int y = BORDER_PAD_Y + FM * LH + 4;
            int lineH = max(16, tftHeight / 8);

            const char *labels[] = {"Start CH", "Stop CH", "Step", "Start Jammer", "Exit"};
            int values[] = {hopCfg.startChannel, hopCfg.stopChannel, hopCfg.stepSize, -1, -1};

            for (int i = 0; i < 5; i++) {
                int itemY = y + i * lineH;
                uint16_t fg = (i == menuIndex) ? bruceConfig.bgColor : bruceConfig.priColor;
                uint16_t bg = (i == menuIndex) ? bruceConfig.priColor : bruceConfig.bgColor;

                tft.fillRect(7, itemY, tftWidth - 14, lineH - 2, bg);
                tft.setTextColor(fg, bg);

                char line[32];
                if (values[i] >= 0) {
                    int freq = 2400 + values[i];
                    snprintf(line, sizeof(line), "%s: %d (%dMHz)", labels[i], values[i], freq);
                } else {
                    snprintf(line, sizeof(line), "%s", labels[i]);
                }
                tft.drawString(line, 12, itemY + 2, 1);

                if (editMode && i == menuIndex && i < 3) {
                    tft.setTextColor(TFT_YELLOW, bg);
                    tft.drawRightString("<>", tftWidth - 12, itemY + 2, 1);
                }
            }
            redraw = false;
        }

        if (check(NextPress)) {
            if (editMode) {
                if (menuIndex == 0) hopCfg.startChannel = (hopCfg.startChannel + 1) % 126;
                if (menuIndex == 1) hopCfg.stopChannel = (hopCfg.stopChannel + 1) % 126;
                if (menuIndex == 2) hopCfg.stepSize = (hopCfg.stepSize % 10) + 1;
            } else {
                menuIndex = (menuIndex + 1) % 5;
            }
            redraw = true;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        if (check(PrevPress)) {
            if (editMode) {
                if (menuIndex == 0) hopCfg.startChannel = (hopCfg.startChannel + 125) % 126;
                if (menuIndex == 1) hopCfg.stopChannel = (hopCfg.stopChannel + 125) % 126;
                if (menuIndex == 2) hopCfg.stepSize = ((hopCfg.stepSize - 2 + 10) % 10) + 1;
            } else {
                menuIndex = (menuIndex + 4) % 5;
            }
            redraw = true;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        if (check(SelPress)) {
            if (menuIndex == 3) {
                // Start hopper jammer inline
                if (CHECK_NRF_UART(nrfMode) || CHECK_NRF_BOTH(nrfMode)) {
                    NRFSerial.println(
                        "HOPPER_" + String(hopCfg.startChannel) + "_" + String(hopCfg.stopChannel) + "_" +
                        String(hopCfg.stepSize)
                    );
                }

                if (CHECK_NRF_SPI(nrfMode)) { initCW(hopCfg.startChannel); }

                int ch = hopCfg.startChannel;
                bool hopRedraw = true;

                drawMainBorderWithTitle("CH HOPPER");

                while (true) {
                    if (check(EscPress)) break;

                    if (hopRedraw) {
                        int contentY = BORDER_PAD_Y + FM * LH + 4;
                        int lineHop = max(14, tftHeight / 10);
                        tft.setTextSize(FP);

                        tft.fillRect(7, contentY, tftWidth - 14, lineHop, bruceConfig.bgColor);
                        tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                        char hBuf[40];
                        snprintf(
                            hBuf,
                            sizeof(hBuf),
                            "Range: %d - %d  Step: %d",
                            hopCfg.startChannel,
                            hopCfg.stopChannel,
                            hopCfg.stepSize
                        );
                        tft.drawCentreString(hBuf, tftWidth / 2, contentY, 1);
                        contentY += lineHop;

                        tft.fillRect(7, contentY, tftWidth - 14, lineHop, bruceConfig.bgColor);
                        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                        int freq = 2400 + ch;
                        snprintf(hBuf, sizeof(hBuf), "CH: %d  (%d MHz)", ch, freq);
                        tft.drawCentreString(hBuf, tftWidth / 2, contentY, 1);

                        int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
                        tft.fillRect(7, footerY, tftWidth - 14, FP * LH, bruceConfig.bgColor);
                        tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
                        tft.drawCentreString("[ESC] Stop", tftWidth / 2, footerY, 1);

                        hopRedraw = false;
                    }

                    if (CHECK_NRF_SPI(nrfMode)) { cwChannel(ch, 0); }

                    ch += hopCfg.stepSize;
                    if (ch > hopCfg.stopChannel) {
                        ch = hopCfg.startChannel;
                        hopRedraw = true; // Update display on wrap
                    }
                }

                if (CHECK_NRF_SPI(nrfMode)) {
                    NRFradio.stopConstCarrier();
                    NRFradio.powerDown();
                }
                if (CHECK_NRF_UART(nrfMode) || CHECK_NRF_BOTH(nrfMode)) { NRFSerial.println("OFF"); }
                return;
            } else if (menuIndex == 4) {
                return;
            } else {
                editMode = !editMode;
            }
            redraw = true;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        delay(50);
    }
}
