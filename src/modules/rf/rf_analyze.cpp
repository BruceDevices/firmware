// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_analyze.h"

#include "core/display.h"
#include "core/type_convertion.h"
#include "protocols/rf_decoder.h"
#include "protocols/rf_fsk_receive_profile.h"
#include "protocols/rf_protocol_engine.h"
#include "rf_classify.h"
#include "rf_utils.h"
#include "structs.h"

namespace {

// Top of the report area, just below the title bar (matches rf_spectrum.cpp).
int report_top() { return BORDER_PAD_Y + 8 * FM + 2; }
uint16_t dim_color() { return blendColors(bruceConfig.bgColor, bruceConfig.priColor, 170); }

void draw_report(
    const RfClassifyResult &cls, const RfCodes &code, bool decoded, bool fskMode, float freq, bool have
) {
    const uint16_t fg = bruceConfig.priColor;
    const uint16_t bg = bruceConfig.bgColor;
    const int x = BORDER_PAD_X;
    const int lh = 8 * FP;
    int y = report_top();

    tft.setTextSize(FP);
    tft.fillRect(0, y - 2, tftWidth, tftHeight - (y - 2), bg);

    auto line = [&](const String &s, uint16_t c) {
        if (y + lh > tftHeight - 4) return;
        tft.setTextColor(c, bg);
        tft.drawString(s, x, y, 1);
        y += lh;
    };

    if (!have) {
        line("Waiting for a signal...", fg);
        line(String(freq, 3) + " MHz", dim_color());
        line("[Sel] frequency   [Esc] exit", dim_color());
        return;
    }

    const char *mod = cls.modulation == RfClassifyMod::OOK    ? "ASK/OOK"
                      : cls.modulation == RfClassifyMod::FSK ? "2-FSK (continuous)"
                                                             : "Unknown";
    line("RX: " + String(fskMode ? "2-FSK" : "ASK/OOK") + " @ " + String(freq, 3) + " MHz", dim_color());
    line("Modulation: " + String(mod) + "  [" + String((int)(cls.confidence * 100)) + "%]", fg);

    if (cls.modulation == RfClassifyMod::OOK)
        line("TE: " + String(cls.teUs) + "us   bits: " + String(cls.bitCount), dim_color());
    else if (cls.modulation == RfClassifyMod::FSK)
        line("Baud: " + String(cls.baud) + " sym/s", dim_color());

    line("Pulses: " + String(cls.pulseCount) + "  duty: " + String((int)(cls.dutyCycle * 100)) + "%",
         dim_color());

    if (decoded && code.protocol != "") {
        String info = "Decoded: " + String(code.protocol);
        if (code.fix != 0 && code.mf_name != "" && code.mf_name != "Unknown")
            info += " (" + code.mf_name + ")";
        else if (code.key != 0) {
            char hex[24];
            decimalToHexString(code.key, hex);
            info += " key:" + String(hex);
        }
        line(info, fg);
    } else {
        line("Decoded: none", dim_color());
    }

    line("[Sel] frequency   [Esc] exit", dim_color());
}

} // namespace

void rf_analyze() {
#if !defined(LITE_VERSION)
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Analyze needs a CC1101!", true);
        return;
    }

    float freq = bruceConfigPins.rfFreq;
    if (!initRfModule("rx", freq)) {
        displayError("CC1101 not found!", true);
        return;
    }

    // Configure the CC1101 async RX for OOK (false) or 2-FSK (true).
    auto setMode = [&](bool fsk) {
        ELECHOUSE_cc1101.setSidle();
        if (fsk) {
            rf_apply_fsk_receive_profile(
                "2FSKDev238Async",
                [](uint8_t a, uint8_t v) { return ELECHOUSE_cc1101.SpiWriteReg(a, v); },
                [](uint8_t a) { return ELECHOUSE_cc1101.SpiReadReg(a); });
        } else {
            ELECHOUSE_cc1101.setModulation(2); // ASK/OOK
            ELECHOUSE_cc1101.setDeviation(1.58f);
            ELECHOUSE_cc1101.setRxBW(270.83f);
            ELECHOUSE_cc1101.setDRate(10.f);
            ELECHOUSE_cc1101.setPktFormat(3);
        }
        ELECHOUSE_cc1101.SetRx();
    };

    RfRxSession rx;
    bool fskMode = false;
    setMode(false);
    if (!rx.begin(true)) {
        deinitRfModule();
        return;
    }

    drawMainBorderWithTitle("RF Auto Classify");

    RfClassifyResult cls;
    RfCodes code;
    bool have = false, decoded = false;
    unsigned long modeSince = millis();
    const unsigned long MODE_DWELL_MS = 600; // flip OOK<->FSK when idle this long

    draw_report(cls, code, false, fskMode, freq, false);

    while (1) {
        if (check(EscPress)) break;

        std::vector<int> durations;
        if (rx.poll(durations)) {
            cls = rf_classify(durations);
            code = RfCodes{};
            decoded = cls.pulseCount >= 6 &&
                      rf_protocol_decode_capture(
                          durations, (uint32_t)(freq * 1000000.0f),
                          fskMode ? "2FSKDev238Async" : "Ook270Async", code);
            have = true;
            modeSince = millis(); // keep this mode while a signal is present
            draw_report(cls, code, decoded, fskMode, freq, have);
        } else if (millis() - modeSince > MODE_DWELL_MS) {
            // Quiet: flip the RX mode so both modulations get a listen window.
            fskMode = !fskMode;
            setMode(fskMode);
            rx.end();
            rx.begin(true);
            modeSince = millis();
        }

        if (setMHZMenu()) {
            freq = bruceConfigPins.rfFreq;
            setMode(fskMode);
            rx.end();
            rx.begin(true);
            modeSince = millis();
            drawMainBorderWithTitle("RF Auto Classify");
            draw_report(cls, code, decoded, fskMode, freq, have);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    rx.end();
    deinitRfModule();
    returnToMenu = true;
#else
    displayError("Not available on Launcher version");
#endif
}
