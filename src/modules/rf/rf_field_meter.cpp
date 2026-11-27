#include "rf_field_meter.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "rf_ui_shim.h"
#include "rf_utils.h"
#include "structs.h"
#include <globals.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Task 2.1 - RF field-strength meter
//
// The Frequency Analyzer answers "what frequency is that?" and then forgets it.
// This page answers the other question: "which way is it?" It sweeps once,
// locks onto the strongest source, and then samples that one frequency as fast
// as the CC1101 allows, so the number follows the walk instead of the sweep.
//
// Relative readings only. A CC1101 RSSI moves with temperature, antenna and the
// body holding the board, so the screen shows the raw dBm next to a 0-100 level
// and never claims a distance or a power.
// ---------------------------------------------------------------------------

#define FM_SAMPLE_MS 40  // 25 Hz: fast enough that a step registers, slower than the radio's settle
#define FM_LOST_MS 1500  // quiet this long = the transmitter stopped, go looking again
#define FM_FLOOR_DBM (-100)
#define FM_CEIL_DBM (-35)
#define FM_PEAK_DECAY 0.94f // peak-hold fall per drawn frame
#define FM_MAX_FREQS 64

// dBm -> 0..100. Linear in dB, which is what a signal-strength bar should be.
static int fmLevel(int rssi) {
    if (rssi <= FM_FLOOR_DBM) return 0;
    if (rssi >= FM_CEIL_DBM) return 100;
    return (rssi - FM_FLOOR_DBM) * 100 / (FM_CEIL_DBM - FM_FLOOR_DBM);
}

// One sample, settled: the CC1101 needs a moment after a frequency change before
// its RSSI means anything, so read twice and keep the second value. Same reason
// the Frequency Analyzer does it.
static int fmRssiAt(float mhz) {
    setMHZ(mhz);
    delayMicroseconds(1200);
    rfGetRssi();
    delayMicroseconds(300);
    return rfGetRssi();
}

// Coarse pass over the standard channels. Median of the sweep is the noise
// floor: a single unsettled or anomalous reading then cannot make noise look
// like a transmitter.
static bool fmAcquire(float &bestFreq, int &noiseFloor, int &bestRssi) {
    const size_t n = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
    if (n == 0 || n > FM_MAX_FREQS) return false;

    int samples[FM_MAX_FREQS];
    bestRssi = -127;
    bestFreq = 0;
    for (size_t i = 0; i < n; i++) {
        // A press during a sweep must not be swallowed by the remaining channels.
        if (check(EscPress)) return false;
        const int r = fmRssiAt(subghz_frequency_list[i]);
        samples[i] = r;
        if (r > bestRssi) {
            bestRssi = r;
            bestFreq = subghz_frequency_list[i];
        }
    }

    int sorted[FM_MAX_FREQS];
    memcpy(sorted, samples, n * sizeof(int));
    for (size_t i = 1; i < n; i++) {
        const int key = sorted[i];
        size_t j = i;
        while (j > 0 && sorted[j - 1] > key) {
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = key;
    }
    noiseFloor = sorted[n / 2];
    return bestFreq > 0;
}

static void fmDraw(
    float freq, int rssi, int level, int peak, int mn, int avg, int mx, int noise, bool quiet
) {
    drawMainBorder(false);
    tft.setTextSize(1);

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(10, 26);
    tft.print("FIELD METER");

    char fbuf[20];
    snprintf(fbuf, sizeof(fbuf), "%.3f MHz", freq);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.drawRightString(fbuf, tftWidth - 10, 26, 1);

    // Big readout: the number you can read with the board held away from you.
    char big[8];
    snprintf(big, sizeof(big), "%d", rssi);
    tft.setTextSize(3);
    tft.setTextColor(quiet ? bruceConfig.secColor : bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(12, 46);
    tft.print(big);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setCursor(12 + (int)strlen(big) * 18 + 4, 58);
    tft.print("dBm");

    // Level percentage, the same number the bar and the beep use.
    tft.setTextSize(3);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawRightString(String(level) + "%", tftWidth - 12, 46, 1);
    tft.setTextSize(1);

    // Bar: length is the live level, the thin marker is the peak hold.
    const int barX = 12, barY = 82, barW = tftWidth - 24, barH = 16;
    tft.drawRect(barX - 1, barY - 1, barW + 2, barH + 2, bruceConfig.secColor);
    tft.fillRect(barX, barY, barW, barH, bruceConfig.bgColor);
    const int fill = barW * level / 100;
    if (fill > 0) uiGradientFillRect(barX, barY, fill, barH);
    const int peakX = barX + barW * peak / 100;
    if (peak > 0) tft.drawFastVLine(peakX > barX ? peakX : barX, barY, barH, bruceConfig.priColor);

    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setCursor(12, 104);
    tft.printf("min %d  avg %d  max %d", mn, avg, mx);
    tft.setCursor(12, 118);
    tft.printf("peak %d%%   noise %d dBm%s", peak, noise, quiet ? "   quiet" : "");

    tft.drawFastHLine(10, tftHeight - 24, tftWidth - 20, bruceConfig.priColor);
    tft.setCursor(12, tftHeight - 10);
    tft.print("SEL reset   BACK exit");
}

void rf_field_meter() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Field Meter needs a CC1101!", true);
        return;
    }
    if (!initRfModule("rx", 433.92f)) {
        displayError("CC1101 not found!", true);
        return;
    }
    rfSetRxBW(200);

    float freq = 433.92f;
    int noise = -95;
    int best = -127;

    displayTextLine("sweeping...");
    if (!fmAcquire(freq, noise, best)) {
        returnToMenu = true;
        deinitRfModule();
        return;
    }

    // Own page, not an overlay: clear the menu that launched this before the
    // first frame is drawn. Until now the page only called drawMainBorder(false)
    // and painted its own numbers on top of whatever was on screen, so the
    // main menu's icon and rows showed through everywhere the meter did not
    // write.
    drawMainBorder(true);

    // Statistics since the last reset, on the locked frequency only.
    int mn = 0, mx = 0;
    int avg = 0;
    int32_t sum = 0;
    uint32_t count = 0;
    float peakF = 0;
    uint32_t lastGood = millis();
    uint32_t lastBeep = 0;
    uint32_t lastDraw = 0;

    while (true) {
        if (check(EscPress)) break;

        if (check(SelPress)) {
            mn = mx = avg = 0;
            sum = 0;
            count = 0;
            peakF = 0;
            lastGood = millis();
        }

        setMHZ(freq);
        delayMicroseconds(1200);
        const int rssi = rfGetRssi();

        // A signal that stays below the noise floor for a while means the
        // transmitter stopped (or was never there): sweep again instead of
        // watching an empty channel forever.
        if (rssi >= noise + 4) lastGood = millis();
        if (millis() - lastGood > FM_LOST_MS) {
            fmAcquire(freq, noise, best);
            lastGood = millis();
            // New frequency, new statistics: keeping them would blend two
            // different transmitters into one min/avg/max and read as a lie.
            mn = mx = avg = 0;
            sum = 0;
            count = 0;
            peakF = 0;
        }

        const int level = fmLevel(rssi);
        if (count == 0 || rssi < mn) mn = rssi;
        if (count == 0 || rssi > mx) mx = rssi;
        sum += rssi;
        count++;
        if (count >= 250) { // keep the average meaningful on a long walk
            sum /= 2;
            count /= 2;
        }
        avg = (int)(sum / (int32_t)count);
        if ((float)level > peakF) peakF = (float)level;
        else peakF *= FM_PEAK_DECAY;
        const int peak = (int)(peakF + 0.5f);

        // Audible proximity cue: the closer, the faster. Below the noise floor
        // the meter stays silent, so an empty room does not tick all the way.
        const uint32_t interval = (uint32_t)(1000 - level * 8); // 1000 ms quiet .. 200 ms loud
        if (level >= 5 && rssi >= noise + 6 && millis() - lastBeep >= interval) {
            lastBeep = millis();
        }

        if (millis() - lastDraw >= 120) {
            fmDraw(freq, rssi, level, peak, mn, avg, mx, noise, level < 5);
            lastDraw = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(FM_SAMPLE_MS));
    }

    returnToMenu = true;
    deinitRfModule();
}
