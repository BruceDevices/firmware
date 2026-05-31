/**
 * @file nrf_spectrum.cpp
 * @brief Enhanced 2.4 GHz spectrum analyzer for Bruce firmware.
 *
 * Features:
 *  - 126 channels (full 2.400-2.525 GHz ISM band)
 *  - Color gradient bars (green→yellow→red based on signal level)
 *  - Peak hold markers with slow decay
 *  - Smooth EMA (Exponential Moving Average) filtering
 *  - 6 simultaneous receive pipes for maximum sensitivity
 *  - Adaptive layout for all screen resolutions
 *  - Grid lines every 10 channels for visual reference
 *  - PA+LNA module support (E01-ML01SP2: -90dBm effective threshold)
 *
 * RPD (Received Power Detector) is binary: 1 = signal above -64dBm
 * at chip input (-90dBm with PA+LNA module).
 */

#include "nrf_spectrum.h"
#include "core/display.h"
#include "core/mykeyboard.h"

// ── Spectrum data ───────────────────────────────────────────────
static uint8_t channel[NRF_SPECTRUM_CHANNELS];
static uint8_t peakHold[NRF_SPECTRUM_CHANNELS];
static uint8_t peakTimer[NRF_SPECTRUM_CHANNELS];

#define PEAK_HOLD_SWEEPS 25 // Number of sweeps before peak decays

// ── Device label tracking ────────────────────────────────────────
#define LABEL_DECAY_SWEEPS 10                           // Sweeps until label fades after signal gone
static uint8_t deviceLabelTimer[NRF_SPECTRUM_CHANNELS]; // Decay timer per channel

// Display mode (0=bars+peaks, 1=bars only, 2=bars+device labels)
static uint8_t specDisplayMode = 0;

// Device type detection for channels
enum DeviceType { DEV_NONE, DEV_WIFI, DEV_BLE, DEV_BT, DEV_ZIGBEE };

struct DeviceInfo {
    const char *label;
    uint16_t labelColor;
};

static const struct DeviceInfo deviceInfo[] = {
    {nullptr,  TFT_BLACK  }, // DEV_NONE
    {"WiFi",   TFT_WHITE  }, // DEV_WIFI
    {"BLE",    TFT_CYAN   }, // DEV_BLE
    {"BT",     TFT_MAGENTA}, // DEV_BT
    {"Zigbee", TFT_GREEN  }, // DEV_ZIGBEE
};

// Detect device type from channel number
static inline DeviceType getDeviceType(int channel) {
    // WiFi: ch 1-14 (2.412-2.484 GHz) → NRF ch 12-84
    if (channel >= 12 && channel <= 84) return DEV_WIFI;

    // BLE Advertising: ch 37-39 (2.402, 2.426, 2.480 GHz) → NRF ch 2, 26, 80
    if (channel == 2 || channel == 26 || channel == 80) return DEV_BLE;

    // BT Classic: ch ~50-79 (2.450-2.480 GHz) → NRF ch 50-79
    if (channel >= 50 && channel <= 79) return DEV_BT;

    // Zigbee/Thread: ch 11-26 + 5-80 with 5MHz spacing (2.405-2.480 GHz) → NRF ch 5,10,15...80
    if (channel >= 5 && channel <= 80 && (channel - 5) % 5 == 0) return DEV_ZIGBEE;

    return DEV_NONE;
}

// ── Color gradient based on signal intensity (0-100) ────────────
static uint16_t getSpectrumColor(uint8_t level) {
    if (level > 85) return TFT_RED;
    if (level > 65) return TFT_ORANGE;
    if (level > 45) return TFT_YELLOW;
    if (level > 25) return TFT_GREEN;
    return TFT_DARKGREEN;
}

// ── Layout calculations ─────────────────────────────────────────
static int spec_headerH;  // Header area height
static int spec_footerH;  // Footer area height (freq labels)
static int spec_barAreaY; // Top of bar area
static int spec_barAreaH; // Height of bar area
static int spec_mirrorH;  // Height of top mirror area
static int spec_marginL;  // Left margin
static int spec_drawW;    // Available drawing width (after margins)

static void calcLayout() {
    spec_headerH = 0;
    spec_footerH = 14;
    spec_mirrorH = 0; // Mirror removed — eliminates top artifacts
    spec_barAreaY = 0;
    spec_barAreaH = tftHeight - spec_footerH - 2;
    spec_marginL = max(2, tftWidth / 80); // Small left margin
    int marginR = spec_marginL;           // Symmetric right margin
    spec_drawW = tftWidth - spec_marginL - marginR;
}

/// Get x position and width for channel i, distributed proportionally
static inline void getBarGeom(int i, int &x, int &w) {
    x = spec_marginL + (i * spec_drawW) / NRF_SPECTRUM_CHANNELS;
    int nextX = spec_marginL + ((i + 1) * spec_drawW) / NRF_SPECTRUM_CHANNELS;
    w = max(1, nextX - x);
}

// ── Scanning and drawing ────────────────────────────────────────
String scanChannels(bool web) {
    String result = web ? "{" : "";

    // Toggle CE low during channel switch
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);

    for (int i = 0; i < NRF_SPECTRUM_CHANNELS; i++) {
        NRFradio.setChannel(i);
        NRFradio.startListening();
        delayMicroseconds(170); // 130µs PLL settle + 40µs RPD sample window
        NRFradio.stopListening();

        int rpd = NRFradio.testRPD() ? 1 : 0;

        // EMA smoothing: fast attack, medium decay
        // Attack: signal instantly jumps to ~50 on first hit
        // Decay: drops ~25% per sweep when signal gone
        if (rpd) {
            channel[i] = (uint8_t)min(100, (int)((channel[i] + 100) / 2));
            deviceLabelTimer[i] = LABEL_DECAY_SWEEPS; // Reset label timer on active signal
        } else {
            channel[i] = (uint8_t)((channel[i] * 3) / 4);
            // Decay label timer when no signal
            if (deviceLabelTimer[i] > 0) { deviceLabelTimer[i]--; }
        }

        // Peak hold tracking
        if (channel[i] >= peakHold[i]) {
            peakHold[i] = channel[i];
            peakTimer[i] = PEAK_HOLD_SWEEPS;
        } else if (peakTimer[i] > 0) {
            peakTimer[i]--;
        } else {
            if (peakHold[i] > 2) peakHold[i] -= 2;
            else peakHold[i] = 0;
        }
    }

    digitalWrite(bruceConfigPins.NRF24_bus.io0, HIGH);

    // ── Draw spectrum bars ──────────────────────────────────────
    uint8_t maxLevel = 0;
    uint8_t maxCh = 0;

    for (int i = 0; i < NRF_SPECTRUM_CHANNELS; i++) {
        int x, w;
        getBarGeom(i, x, w);

        int level = channel[i];
        if (level > maxLevel) {
            maxLevel = level;
            maxCh = i;
        }

        int barH = (level * spec_barAreaH) / 100;
        int peakH = (peakHold[i] * spec_barAreaH) / 100;

        // Grid line color (every 10 channels)
        uint16_t gridColor = (i % 10 == 0) ? TFT_DARKGREY : bruceConfig.bgColor;

        // Main bar area: clear above, draw bar from bottom
        if (barH < spec_barAreaH) { tft.fillRect(x, spec_barAreaY, w, spec_barAreaH - barH, gridColor); }
        if (barH > 0) {
            uint16_t barColor = getSpectrumColor(level);
            tft.fillRect(x, spec_barAreaY + spec_barAreaH - barH, w, barH, barColor);
        }

        // Peak hold marker (Mode 0 only): white line segment
        if (specDisplayMode == 0 && peakH > 0 && peakH >= barH) {
            int peakY = spec_barAreaY + spec_barAreaH - peakH;
            if (peakY >= spec_barAreaY && peakY < spec_barAreaY + spec_barAreaH) {
                tft.fillRect(x, peakY, w, 1, TFT_WHITE);
            }
        }

        if (web) {
            if (i > 0) result += ",";
            result += String(level);
        }
    }

    // Show peak channel indicator at top-right
    if (maxLevel > 10) {
        tft.setTextSize(FP);
        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        char peakBuf[12];
        snprintf(peakBuf, sizeof(peakBuf), "pk:%d", (int)maxCh);
        int pkW = 42;
        int pkY = 1;
        tft.fillRect(tftWidth - pkW - spec_marginL, pkY, pkW, 10, bruceConfig.bgColor);
        tft.drawRightString(peakBuf, tftWidth - spec_marginL - 2, pkY, 1);
    }

    // ── Draw device labels (Mode 2 only) ──────────────────────────
    if (specDisplayMode == 2) {
        // Group labels by channel and stack vertically
        int labelY = 2;
        for (int i = 0; i < NRF_SPECTRUM_CHANNELS; i++) {
            // Show label if signal present or timer still active
            if ((channel[i] > 10) || (deviceLabelTimer[i] > 0)) {
                DeviceType dev = getDeviceType(i);

                if (dev != DEV_NONE) {
                    // Display known device label
                    int x, w;
                    getBarGeom(i, x, w);
                    int labelX = x + w / 2; // Center on channel

                    tft.setTextSize(FP);
                    tft.setTextColor(deviceInfo[dev].labelColor, bruceConfig.bgColor);
                    tft.drawCentreString(deviceInfo[dev].label, labelX, labelY, 1);

                    labelY += 8;                       // Stack labels vertically
                    if (labelY > tftHeight / 4) break; // Prevent overflow
                } else if (channel[i] > 10) {
                    // Unknown device: show small "?" in gray
                    int x, w;
                    getBarGeom(i, x, w);
                    int labelX = x + w / 2;

                    tft.setTextSize(1); // Tiny font
                    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
                    tft.drawCentreString("?", labelX, labelY, 1);

                    labelY += 6;
                    if (labelY > tftHeight / 5) break;
                }
            }
        }
    }

    if (web) result += "}";
    return result;
}

void nrf_spectrum() {
    tft.fillScreen(bruceConfig.bgColor);

    // Initialize data
    memset(channel, 0, sizeof(channel));
    memset(peakHold, 0, sizeof(peakHold));
    memset(peakTimer, 0, sizeof(peakTimer));
    memset(deviceLabelTimer, 0, sizeof(deviceLabelTimer));
    specDisplayMode = 0; // Start in mode 0

    // Calculate layout
    calcLayout();

    // Draw frequency labels at bottom
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    int labelY = tftHeight - spec_footerH + 2;
    tft.drawString("2.400", spec_marginL, labelY, 1);
    tft.drawCentreString("2.462", tftWidth / 2, labelY, 1);
    tft.drawRightString("2.525", tftWidth - spec_marginL, labelY, 1);

    // Draw separator line
    tft.drawFastHLine(0, spec_barAreaY + spec_barAreaH + 1, tftWidth, TFT_DARKGREY);

    // Draw mode indicator
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    const char *modeStr[] = {"Mode:Peak", "Mode:Bar", "Mode:Dev"};
    tft.drawString(modeStr[specDisplayMode], spec_marginL, 2, 1);

    if (nrf_start(NRF_MODE_SPI)) {
        // Configure for wideband spectrum sensing
        NRFradio.setAutoAck(false);
        NRFradio.disableCRC();
        NRFradio.setAddressWidth(2);

        // Open 6 reading pipes at noise-detection addresses
        // More pipes = higher sensitivity (radio checks all in parallel)
        const uint8_t noiseAddress[][2] = {
            {0x55, 0x55},
            {0xAA, 0xAA},
            {0xA0, 0xAA},
            {0xAB, 0xAA},
            {0xAC, 0xAA},
            {0xAD, 0xAA}
        };
        for (uint8_t i = 0; i < 6; ++i) { NRFradio.openReadingPipe(i, noiseAddress[i]); }

        NRFradio.setDataRate(RF24_1MBPS);

        while (!check(EscPress)) {
            scanChannels();

            // SEL to cycle through modes
            if (check(SelPress)) {
                specDisplayMode = (specDisplayMode + 1) % 3;
                // Clear only spectrum bar area, preserve frequency labels at bottom
                tft.fillRect(0, spec_barAreaY, tftWidth, spec_barAreaH, bruceConfig.bgColor);
                delay(200);
            }
        }

        NRFradio.stopListening();
        NRFradio.powerDown();
        delay(250);
    } else {
        Serial.println("Fail Starting radio");
        displayError("NRF24 not found");
        delay(500);
    }
}
