#include "rf_audio.h"

#include "rf_utils.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"

#if defined(BUZZ_PIN) || defined(HAS_NS4168_SPKR)
#define RF_AUDIO_HAS_SPEAKER 1
#include "../others/audio.h"
#endif

static void rf_audio_beep(unsigned int hz, unsigned long ms) {
#if defined(BUZZ_PIN)
    tone(BUZZ_PIN, hz, ms);
#elif defined(HAS_NS4168_SPKR)
    serialCli.parse("tone " + String(hz) + " " + String(ms));
#endif
}

void rf_audio_rx() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Music RX needs CC1101!", true);
        return;
    }

    float freq = bruceConfigPins.rfFreq;
    bool audioEnabled = true;
    bool redraw = true;
    const int HEADER = 46;

    // RSSI history for waveform
    const int graphW = tftWidth - 40;
    int rssiHistory[graphW];
    int histIdx = 0;
    for (int i = 0; i < graphW; i++) rssiHistory[i] = -100;

    if (!initRfModule("rx", freq)) {
        displayError("CC1101 not found!", true);
        return;
    }

    // Short beep to confirm start
#if defined(RF_AUDIO_HAS_SPEAKER)
    rf_audio_beep(800, 200);
#endif

    while (!check(EscPress)) {
        // Adjust frequency
        if (check(PrevPress)) { freq -= 0.1f; redraw = true; }
        if (check(NextPress)) { freq += 0.01f; redraw = true; }
        freq = constrain(freq, 300.0f, 928.0f);

        if (check(SelPress)) {
            audioEnabled = !audioEnabled;
            redraw = true;
            delay(200);
        }

        if (redraw) {
            setMHZ(freq);
            redraw = false;
        }

        // Sample RSSI
        int rssi = ELECHOUSE_cc1101.getRssi();
        tft.drawPixel(0, 0, 0); // CC1101 shared SPI workaround

        // Store in history
        rssiHistory[histIdx] = rssi;
        histIdx = (histIdx + 1) % graphW;

        // --- Draw display ---
        // Clear waveform area
        tft.fillRect(0, HEADER, tftWidth, tftHeight - HEADER - 28, bruceConfig.bgColor);

        // Title bar
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
        tft.setTextFont(FP);
        tft.setTextSize(1);
        tft.drawString("SubGHz Music RX", tftWidth / 2, HEADER + 2);

        // Frequency
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextFont(FM);
        char freqBuf[16];
        snprintf(freqBuf, sizeof(freqBuf), "%.3f MHz", freq);
        tft.drawString(freqBuf, tftWidth / 2, HEADER + 16);

        // RSSI value
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setTextFont(FP);
        char rssiBuf[16];
        snprintf(rssiBuf, sizeof(rssiBuf), "RSSI: %d dBm", rssi);
        tft.drawString(rssiBuf, tftWidth / 2, HEADER + 32);

        // RSSI bar
        int barW = tftWidth - 20;
        int barH = 8;
        int barX = 10;
        int barY = HEADER + 44;
        int fill = map(constrain(rssi, -100, -10), -100, -10, 0, barW);
        tft.drawRect(barX, barY, barW, barH, bruceConfig.priColor);
        tft.fillRect(barX, barY, fill, barH, bruceConfig.priColor);

        // Waveform
        const int waveTop = barY + barH + 8;
        const int waveH = tftHeight - waveTop - 26;
        const int waveMid = waveTop + waveH / 2;

        // Draw axis
        tft.drawFastHLine(20, waveMid, graphW, bruceConfig.secColor);

        // Draw waveform
        for (int i = 1; i < graphW; i++) {
            int idx0 = (histIdx + i - 1) % graphW;
            int idx1 = (histIdx + i) % graphW;
            int r0 = rssiHistory[idx0];
            int r1 = rssiHistory[idx1];
            int y0 = waveMid - map(constrain(r0, -100, -10), -100, -10, -waveH / 2, waveH / 2);
            int y1 = waveMid - map(constrain(r1, -100, -10), -100, -10, -waveH / 2, waveH / 2);
            int x0 = 20 + i - 1;
            int x1 = 20 + i;
            tft.drawLine(x0, y0, x1, y1, bruceConfig.priColor);
        }

        // Audio output
#if defined(RF_AUDIO_HAS_SPEAKER)
        if (audioEnabled && rssi > -70) {
            // Map RSSI to audible frequency: -70 dBm → 200 Hz, -10 dBm → 2000 Hz
            unsigned int toneHz = map(constrain(rssi, -70, -10), -70, -10, 200, 2000);
            rf_audio_beep(toneHz, 50);
        }
#endif

        // Footer
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
        tft.setTextFont(FP);
#if defined(RF_AUDIO_HAS_SPEAKER)
        tft.drawString(
            audioEnabled ? "SEL=mute  Prev/Next=freq" : "SEL=unmute  Prev/Next=freq",
            tftWidth / 2, tftHeight - 12
        );
#else
        tft.drawString("Prev/Next=freq  (no speaker)", tftWidth / 2, tftHeight - 12);
#endif

        vTaskDelay(pdMS_TO_TICKS(50));
    }

    deinitRfModule();
}
