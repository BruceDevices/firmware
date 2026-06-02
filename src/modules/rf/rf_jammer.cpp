#include "rf_jammer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "rf_utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <globals.h>

static const uint32_t MAX_SEQUENCE = 50;
static const uint32_t DURATION_CYCLES = 3;

static const char *JAM_MODE_NAMES[] = {"FULL POWER", "INTERMITTENT", "NOISE STORM", "FREQ SWEEP"};
static const uint16_t JAM_MODE_COLORS[] = {TFT_RED, TFT_ORANGE, TFT_MAGENTA, TFT_CYAN};

RFJammer::RFJammer(bool full) {
    jamMode = full ? RF_JAM_FULL : RF_JAM_ITMT;
    setup();
}

RFJammer::~RFJammer() {
    deinitRfModule();
}

// ── Mode selection menu ─────────────────────────────────────────
RFJamMode RFJammer::showModeMenu(bool defaultFull) {
    isCC1101 = (bruceConfigPins.rfModule == CC1101_SPI_MODULE);

    int menuIdx = defaultFull ? 0 : 1;
    bool redraw = true;
    int modeCount = isCC1101 ? RF_JAM_MODE_COUNT : 2; // Only Full+Itmt for raw GPIO

    while (true) {
        if (check(EscPress)) return (RFJamMode)255; // Cancel

        if (redraw) {
            drawMainBorderWithTitle("RF JAMMER MODE");
            tft.setTextSize(FP);

            int y = BORDER_PAD_Y + FM * LH + 4;
            int lineH = max(14, tftHeight / (modeCount + 4));

            for (int i = 0; i < modeCount; i++) {
                int itemY = y + i * lineH;
                uint16_t fg = (i == menuIdx) ? bruceConfig.bgColor : bruceConfig.priColor;
                uint16_t bg = (i == menuIdx) ? JAM_MODE_COLORS[i] : bruceConfig.bgColor;

                tft.fillRect(7, itemY, tftWidth - 14, lineH - 2, bg);
                tft.setTextColor(fg, bg);
                tft.drawCentreString(JAM_MODE_NAMES[i], tftWidth / 2, itemY + 2, 1);
            }

            // Description of selected mode
            int descY = y + modeCount * lineH + 4;
            tft.fillRect(7, descY, tftWidth - 14, lineH * 2, bruceConfig.bgColor);
            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            const char *descs[] = {
                "Max duty cycle continuous TX",
                "Varied pulse patterns + bursts",
                "CC1101 HW random noise (PN9)",
                "Sweep +/-5MHz around target"
            };
            tft.drawCentreString(descs[menuIdx], tftWidth / 2, descY + 2, 1);

            // Footer
            int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
            tft.fillRect(7, footerY, tftWidth - 14, FP * LH, bruceConfig.bgColor);
            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawCentreString("[OK]Select [ESC]Back", tftWidth / 2, footerY, 1);

            redraw = false;
        }

        if (check(NextPress)) { menuIdx = (menuIdx + 1) % modeCount; redraw = true; }
        if (check(PrevPress)) { menuIdx = (menuIdx + modeCount - 1) % modeCount; redraw = true; }
        if (check(SelPress)) return (RFJamMode)menuIdx;

        delay(50);
    }
}

void RFJammer::setup() {
    // Show mode selection first
    RFJamMode selected = showModeMenu(jamMode == RF_JAM_FULL);
    if ((uint8_t)selected == 255) { sendRF = false; return; }
    jamMode = selected;

    nTransmitterPin = bruceConfigPins.rfTx;
    if (!initRfModule("tx")) { sendRF = false; return; }

    isCC1101 = (bruceConfigPins.rfModule == CC1101_SPI_MODULE);
    if (isCC1101) {
        nTransmitterPin = bruceConfigPins.CC1101_bus.io0;
    }

    sendRF = true;
    pulseCount = 0;
    sweepCount = 0;
    display_banner();

    switch (jamMode) {
        case RF_JAM_FULL:  run_full_jammer();  break;
        case RF_JAM_ITMT:  run_itmt_jammer();  break;
        case RF_JAM_NOISE: run_noise_jammer(); break;
        case RF_JAM_SWEEP: run_sweep_jammer(); break;
        default: break;
    }
}

void RFJammer::display_banner() {
    drawMainBorderWithTitle("RF JAMMER");

    int y = BORDER_PAD_Y + FM * LH + 4;
    int lineH = max(14, tftHeight / 10);
    tft.setTextSize(FP);
    char buf[40];

    // Line 1: Mode badge (centered, colored rounded rect)
    const char *modeName = JAM_MODE_NAMES[jamMode];
    uint16_t modeClr = JAM_MODE_COLORS[jamMode];
    int mbW = strlen(modeName) * 6 + 10;
    int mbX = (tftWidth - mbW) / 2;
    tft.fillRoundRect(mbX, y, mbW, lineH - 2, 3, modeClr);
    tft.setTextColor(TFT_BLACK, modeClr);
    tft.drawCentreString(modeName, tftWidth / 2, y + 2, 1);
    y += lineH;

    // Line 2: Frequency + Module badge
    tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
    snprintf(buf, sizeof(buf), "%.2f MHz", bruceConfigPins.rfFreq);
    tft.drawString(buf, 12, y + 2, 1);
    const char *modStr = isCC1101 ? "CC1101" : "RAW TX";
    int mdW = strlen(modStr) * 6 + 8;
    int mdX = tftWidth - 12 - mdW;
    tft.fillRoundRect(mdX, y + 1, mdW, lineH - 3, 3, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawCentreString(modStr, mdX + mdW / 2, y + 2, 1);
    y += lineH;

    // Line 3: Timer + Stats
    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    tft.drawString("00:00", 12, y + 2, 1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    if (jamMode == RF_JAM_SWEEP) {
        tft.drawRightString("Sweeps: 0", tftWidth - 12, y + 2, 1);
    } else {
        tft.drawRightString("Pulses: 0", tftWidth - 12, y + 2, 1);
    }
    y += lineH;

    // Line 4: TX Power bar (gradient fill)
    {
        int bX = 12, bW = tftWidth - 24;
        int bH = max(6, lineH - 6);
        int bY = y + (lineH - bH) / 2;
        tft.drawRect(bX, bY, bW, bH, bruceConfig.priColor);
        int segW = (bW - 2) / 4;
        tft.fillRect(bX + 1, bY + 1, segW, bH - 2, TFT_GREEN);
        tft.fillRect(bX + 1 + segW, bY + 1, segW, bH - 2, TFT_YELLOW);
        tft.fillRect(bX + 1 + segW * 2, bY + 1, segW, bH - 2, TFT_ORANGE);
        tft.fillRect(bX + 1 + segW * 3, bY + 1, bW - 2 - segW * 3, bH - 2, TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString("TX POWER", tftWidth / 2, bY + 1, 1);
    }
    y += lineH;

    // Line 5: ACTIVE indicator with dot
    tft.fillCircle(tftWidth / 2 - 50, y + lineH / 2, 4, modeClr);
    tft.setTextColor(modeClr, bruceConfig.bgColor);
    tft.drawString("JAMMING ACTIVE", tftWidth / 2 - 38, y + 2, 1);

    // Footer
    int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString("[ESC] Stop", tftWidth / 2, footerY, 1);
}

void RFJammer::update_display(uint32_t elapsedMs) {
    unsigned long secs = elapsedMs / 1000;
    unsigned long mins = secs / 60;
    secs %= 60;

    int y = BORDER_PAD_Y + FM * LH + 4;
    int lineH = max(14, tftHeight / 10);
    y += lineH * 2; // Skip to timer line

    // Update timer + stats
    tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    char buf[30];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", mins, secs);
    tft.drawString(buf, 12, y + 2, 1);
    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
    if (jamMode == RF_JAM_SWEEP) {
        snprintf(buf, sizeof(buf), "Sweeps: %lu", (unsigned long)sweepCount);
    } else {
        snprintf(buf, sizeof(buf), "P: %lu", (unsigned long)pulseCount);
    }
    tft.drawRightString(buf, tftWidth - 12, y + 2, 1);

    // Blink activity dot
    y += lineH * 2; // Skip to ACTIVE line
    uint16_t modeClr = JAM_MODE_COLORS[jamMode];
    bool dotOn = (secs % 2 == 0);
    tft.fillCircle(tftWidth / 2 - 50, y + lineH / 2, 4, dotOn ? modeClr : bruceConfig.bgColor);
}

// ── FULL POWER: Maximum duty cycle continuous TX ────────────────
// Aggressive multi-pattern approach: alternates between sustained carrier,
// rapid micro-glitches, and burst disruption patterns for maximum spectral
// pollution across the widest bandwidth possible.
void RFJammer::run_full_jammer() {
    digitalWrite(nTransmitterPin, HIGH);
    uint32_t startTime = millis();
    uint32_t lastCheckTime = startTime;
    uint32_t lastDisplayTime = startTime;
    uint8_t phase = 0; // Cycle through 3 attack phases

    while (sendRF) {
        uint32_t currentTime = millis();
        uint32_t elapsed = currentTime - startTime;

        // Phase rotation every 100ms for maximum spectral diversity
        phase = (elapsed / 100) % 3;

        switch (phase) {
        case 0:
            // Phase A: Ultra-rapid micro-glitches (1µs LOW every 4µs)
            for (int i = 0; i < 100 && sendRF; i++) {
                digitalWrite(nTransmitterPin, HIGH);
                delayMicroseconds(3);
                digitalWrite(nTransmitterPin, LOW);
                delayMicroseconds(1);
                pulseCount++;
            }
            break;
        case 1:
            // Phase B: Variable-width burst disruption (2-20µs pulses)
            for (int i = 0; i < 50 && sendRF; i++) {
                uint32_t w = 2 + (micros() % 18);
                digitalWrite(nTransmitterPin, HIGH);
                delayMicroseconds(w);
                digitalWrite(nTransmitterPin, LOW);
                delayMicroseconds(1);
                pulseCount++;
            }
            break;
        case 2:
            // Phase C: Sustained carrier with periodic hard cuts
            digitalWrite(nTransmitterPin, HIGH);
            delayMicroseconds(80);
            digitalWrite(nTransmitterPin, LOW);
            delayMicroseconds(2);
            digitalWrite(nTransmitterPin, HIGH);
            delayMicroseconds(80);
            pulseCount += 2;
            break;
        }

        if (currentTime - lastCheckTime > 100) {
            lastCheckTime = currentTime;
            if (check(EscPress)) {
                sendRF = false;
                returnToMenu = true;
                break;
            }
        }

        if (currentTime - lastDisplayTime >= 1000) {
            lastDisplayTime = currentTime;
            update_display(currentTime - startTime);
        }
    }
    digitalWrite(nTransmitterPin, LOW);
}

// ── INTERMITTENT: Varied pulse patterns + bursts ────────────────
void RFJammer::run_itmt_jammer() {
    uint32_t startTime = millis();
    uint32_t lastCheckTime = startTime;
    uint32_t lastDisplayTime = startTime;

    uint32_t sequenceValues[MAX_SEQUENCE];
    for (uint32_t i = 0; i < MAX_SEQUENCE; i++) {
        sequenceValues[i] = 10 * (i + 1);
    }

    while (sendRF) {
        // Forward sweep: 10µs → 500µs
        for (uint32_t sequence = 0; sequence < MAX_SEQUENCE && sendRF; sequence++) {
            uint32_t pulseWidth = sequenceValues[sequence];

            for (uint32_t duration = 0; duration < DURATION_CYCLES && sendRF; duration++) {
                send_optimized_pulse(pulseWidth);
                pulseCount++;

                uint32_t currentTime = millis();
                if (currentTime - lastCheckTime > 50) {
                    lastCheckTime = currentTime;
                    if (check(EscPress)) {
                        sendRF = false;
                        returnToMenu = true;
                        break;
                    }
                }

                if (currentTime - lastDisplayTime >= 1000) {
                    lastDisplayTime = currentTime;
                    update_display(currentTime - startTime);
                }
            }
        }

        // Reverse sweep: 500µs → 10µs for chirp effect
        for (int sequence = MAX_SEQUENCE - 1; sequence >= 0 && sendRF; sequence--) {
            send_optimized_pulse(sequenceValues[sequence]);
            pulseCount++;

            uint32_t currentTime = millis();
            if (currentTime - lastCheckTime > 50) {
                lastCheckTime = currentTime;
                if (check(EscPress)) { sendRF = false; returnToMenu = true; break; }
            }
            if (currentTime - lastDisplayTime >= 1000) {
                lastDisplayTime = currentTime;
                update_display(currentTime - startTime);
            }
        }

        // Random noise burst
        if (sendRF) {
            send_random_pattern(200);
            pulseCount += 200;
        }
    }
    digitalWrite(nTransmitterPin, LOW);
}

// ── NOISE STORM: CC1101 PN9 hardware random TX ─────────────────
// Uses CC1101's internal PN9 pseudo-random generator to transmit
// continuous modulated noise at maximum bandwidth. Cycles between
// modulation schemes (ASK→2FSK→MSK) for wideband spectral coverage.
void RFJammer::run_noise_jammer() {
    if (!isCC1101) return;

    // Switch CC1101 to PN9 random TX mode — maximum aggression
    ELECHOUSE_cc1101.setSidle();
    ELECHOUSE_cc1101.setPktFormat(2);   // PN9 random TX mode
    ELECHOUSE_cc1101.setDRate(800);     // Push data rate higher for wider occupied BW
    ELECHOUSE_cc1101.setModulation(2);  // Start with ASK/OOK
    ELECHOUSE_cc1101.setDeviation(47.6);// Max deviation for FSK modes
    ELECHOUSE_cc1101.setRxBW(812);      // Widest RX BW setting (not critical for TX but sets filter)
    ELECHOUSE_cc1101.setPA(12);         // Maximum TX power
    ELECHOUSE_cc1101.SetTx();

    uint32_t startTime = millis();
    uint32_t lastDisplayTime = startTime;

    // Update mode badge text for noise mode
    int y = BORDER_PAD_Y + FM * LH + 4;
    int lineH = max(14, tftHeight / 10);
    y += lineH * 4; // ACTIVE line
    tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
    tft.fillCircle(tftWidth / 2 - 50, y + lineH / 2, 4, TFT_MAGENTA);
    tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawString("PN9 HW NOISE TX", tftWidth / 2 - 38, y + 2, 1);

    uint8_t modCycle = 0;
    static const uint8_t modSchemes[] = {2, 0, 1}; // ASK, 2FSK, MSK
    static const char *modNames[] = {"ASK/OOK", "2-FSK", "MSK"};

    while (sendRF) {
        uint32_t currentTime = millis();

        // Cycle modulation scheme every 3 seconds for spectral diversity
        uint8_t newMod = ((currentTime - startTime) / 3000) % 3;
        if (newMod != modCycle) {
            modCycle = newMod;
            ELECHOUSE_cc1101.setSidle();
            ELECHOUSE_cc1101.setModulation(modSchemes[modCycle]);
            ELECHOUSE_cc1101.SetTx();

            // Show current modulation on display
            int modY = BORDER_PAD_Y + FM * LH + 4 + max(14, tftHeight / 10) * 4;
            tft.fillRect(7, modY, tftWidth - 14, max(14, tftHeight / 10), bruceConfig.bgColor);
            tft.fillCircle(tftWidth / 2 - 50, modY + max(14, tftHeight / 10) / 2, 4, TFT_MAGENTA);
            tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
            tft.setTextSize(FP);
            char modBuf[30];
            snprintf(modBuf, sizeof(modBuf), "PN9 %s TX", modNames[modCycle]);
            tft.drawString(modBuf, tftWidth / 2 - 38, modY + 2, 1);
        }

        if (currentTime - lastDisplayTime >= 1000) {
            lastDisplayTime = currentTime;
            pulseCount = (currentTime - startTime) / 1000;
            update_display(currentTime - startTime);
        }

        if (check(EscPress)) {
            sendRF = false;
            returnToMenu = true;
            break;
        }

        delay(50); // Low CPU usage since hardware handles TX
    }

    // Restore CC1101 to idle
    ELECHOUSE_cc1101.setSidle();
    ELECHOUSE_cc1101.setPktFormat(3); // Restore async serial mode
}

// ── FREQ SWEEP: Hop around target frequency ─────────────────────
// Sweeps ±5MHz around the configured frequency in 50kHz steps.
// Alternates sweep direction each pass for maximum coverage.
// TX burst at each step creates wideband interference.
void RFJammer::run_sweep_jammer() {
    if (!isCC1101) return;

    float baseFreq = bruceConfigPins.rfFreq;
    float sweepMin = baseFreq - 5.0f;
    float sweepMax = baseFreq + 5.0f;
    float sweepStep = 0.05f; // 50kHz steps for finer coverage
    float currentFreq = sweepMin;
    bool sweepForward = true;

    // Maximum CC1101 TX power
    ELECHOUSE_cc1101.setPA(12);

    uint32_t startTime = millis();
    uint32_t lastCheckTime = startTime;
    uint32_t lastDisplayTime = startTime;

    // Start TX on first frequency
    ELECHOUSE_cc1101.setMHZ(currentFreq);
    digitalWrite(nTransmitterPin, HIGH);

    while (sendRF) {
        // Sweep frequency
        ELECHOUSE_cc1101.setMHZ(currentFreq);

        // TX burst at each frequency — 40 pulses with tight timing
        for (int burst = 0; burst < 40 && sendRF; burst++) {
            digitalWrite(nTransmitterPin, HIGH);
            delayMicroseconds(30);
            digitalWrite(nTransmitterPin, LOW);
            delayMicroseconds(5);
            pulseCount++;
        }

        // Bidirectional sweep
        if (sweepForward) {
            currentFreq += sweepStep;
            if (currentFreq > sweepMax) {
                currentFreq = sweepMax;
                sweepForward = false;
                sweepCount++;
            }
        } else {
            currentFreq -= sweepStep;
            if (currentFreq < sweepMin) {
                currentFreq = sweepMin;
                sweepForward = true;
                sweepCount++;
            }
        }

        uint32_t currentTime = millis();
        if (currentTime - lastCheckTime > 100) {
            lastCheckTime = currentTime;
            if (check(EscPress)) {
                sendRF = false;
                returnToMenu = true;
                break;
            }
        }

        if (currentTime - lastDisplayTime >= 500) {
            lastDisplayTime = currentTime;

            // Update frequency display on sweep
            int y = BORDER_PAD_Y + FM * LH + 4;
            int lineH = max(14, tftHeight / 10);
            y += lineH; // Freq line
            tft.fillRect(7, y, tftWidth - 14, lineH, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
            char buf[30];
            snprintf(buf, sizeof(buf), "%.2f MHz", currentFreq);
            tft.drawString(buf, 12, y + 2, 1);

            update_display(currentTime - startTime);
        }
    }

    digitalWrite(nTransmitterPin, LOW);
    // Restore original frequency
    ELECHOUSE_cc1101.setMHZ(baseFreq);
}

void RFJammer::send_optimized_pulse(int width) {
    // Aggressive HIGH phase: rapid micro-interruptions every 6µs
    // creates wideband harmonic content around center frequency
    for (uint32_t i = 0; i < (uint32_t)width; i += 6) {
        digitalWrite(nTransmitterPin, HIGH);
        delayMicroseconds(5);
        // Sub-microsecond glitch every cycle for spectral spread
        digitalWrite(nTransmitterPin, LOW);
        delayMicroseconds(1);
    }
    digitalWrite(nTransmitterPin, HIGH);
    delayMicroseconds(width / 3); // Extra sustained tail
    digitalWrite(nTransmitterPin, LOW);

    // Minimal dead time — maximize duty cycle
    uint32_t lowPeriod = width / 4;
    if (lowPeriod > 0) delayMicroseconds(lowPeriod);
}

void RFJammer::send_random_pattern(int numPulses) {
    uint32_t startTime = millis();

    for (int i = 0; i < numPulses && sendRF; i++) {
        // Wider range of pulse widths for maximum spectral diversity
        uint32_t pulseWidth = 1 + (micros() % 60);

        digitalWrite(nTransmitterPin, HIGH);
        delayMicroseconds(pulseWidth);

        digitalWrite(nTransmitterPin, LOW);

        // Minimal dead time: 1-8µs for near-max duty cycle
        uint32_t spaceWidth = 1 + (micros() % 8);
        delayMicroseconds(spaceWidth);

        if (millis() - startTime > 250) {
            break; // Longer burst window
        }
    }
}
