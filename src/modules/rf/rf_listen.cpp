#include "rf_listen.h"

#include "../others/audio.h"

volatile unsigned long lastEdgeMicros = 0;
volatile unsigned long pulseDuration = 0;
volatile bool newPulse = false;

// MUST be fully IRAM-safe: no digitalRead() (lives in flash) and no access to C++ objects
// like bruceConfigPins from here. On GDO0 the noise can fire this thousands of times/s; if it
// runs while the flash cache is busy (e.g. display/font reads) the chip crashes/reboots.
void IRAM_ATTR onPulse() {
    unsigned long now = micros();
    unsigned long period = now - lastEdgeMicros;
    lastEdgeMicros = now;

    // Measure the period between rising edges. Drop sub-20us glitches and >1s gaps so a noise
    // burst doesn't keep newPulse pinned and the displayed value stays meaningful.
    if (period >= 20 && period <= 1000000UL) {
        pulseDuration = period;
        newPulse = true;
    }
}

// Beep through the EXACT same path as the working DTMF Tones.js script:
// audio.tone -> (HAS_NS4168_SPKR) serialCli.parse("tone f d") -> toneCallback -> playTone.
// The key detail learned from that script: the I2S speaker needs a long tone (~500ms) to be
// audible - short beeps get swallowed by the DMA buffer before it drains.
static void rf_listen_beep(unsigned int hz, unsigned long ms) {
#if defined(BUZZ_PIN)
    tone(BUZZ_PIN, hz, ms); // non-blocking buzzer
#else
    serialCli.parse("tone " + String(hz) + " " + String(ms));
#endif
}

void rf_listen() {
    float freq = 433.92;
    float last_freq = -1;
    bool redraw = false;
    while (!check(SelPress) && !check(EscPress)) {
        if (check(PrevPress)) { freq -= 0.1f; }
        if (check(NextPress)) { freq += 0.1f; }

        freq = constrain(freq, 300.0f, 928.0f);
        if (freq != last_freq) {
            redraw = true;
            last_freq = freq;
        } else {
            redraw = false;
        }

        if (redraw) {
            String text = String("Frequency: ") + String(freq, 2) + String("MHz");
            displayRedStripe(text, getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);
        }

        if (check(EscPress)) break;
        if (check(SelPress)) break;
    }

    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Listener needs a CC1101!", true);
        return;
    }
    if (!initRfModule("rx", freq)) {
        displayError("CC1101 not found!", true);
        return;
    }

    ELECHOUSE_cc1101.setRxBW(58);
    ELECHOUSE_cc1101.setModulation(2);
    ELECHOUSE_cc1101.setDcFilterOff(true);
    // Short confirmation beep so you know listening has started.
    rf_listen_beep(1000, 500);

    // Seed the timestamp so the first measured period isn't a bogus "time since boot" value
    // (that was the 0.03 Hz reading).
    lastEdgeMicros = micros();
    newPulse = false;
    attachInterrupt(digitalPinToInterrupt(bruceConfigPins.CC1101_bus.io0), onPulse, RISING);
    displayRedStripe("Listening...", getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);

    unsigned long lastPulseTime = millis();
    unsigned long lastTone = 0;
    bool pulseActive = false;

    // Only repaint the stripe when the text actually changes. Redrawing every loop iteration
    // is a full SPI screen draw that (together with the GDO0 interrupt storm) freezes the UI.
    String lastStripe = "";
    auto showStripe = [&](const String &text) {
        if (text == lastStripe) return;
        lastStripe = text;
        displayRedStripe(text, getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);
    };

    while (check(EscPress)) { delay(10); }

    showStripe("Waiting for a pulse");

    while (!check(EscPress)) {
        if (newPulse) {
            newPulse = false;
            lastPulseTime = millis();
            pulseActive = true;
            // Compute frequency here (out of the ISR) to keep the interrupt fast.
            unsigned long dur = pulseDuration;
            float freqHz = dur ? (1000000.0f / dur) : 0.0f;
            showStripe(String("Freq: ") + String(freqHz, 2) + String(" Hz"));
            // Audible feedback at the detected pulse frequency, via the same proven path as
            // dtmf.js. The I2S speaker needs ~500ms to be audible, and playTone BLOCKS for that
            // time, so throttle to 700ms so a continuous signal doesn't make the listener feel
            // stuck.
            if (millis() - lastTone > 700) {
                lastTone = millis();
                rf_listen_beep((unsigned int)constrain(freqHz, 100.0f, 8000.0f), 500);
            }
        }

        if (pulseActive && millis() - lastPulseTime > 3000) {
            pulseActive = false;
            showStripe("Waiting for a pulse");
        }

        if (check(SelPress)) break;
        delay(5); // feed the watchdog and let button/UI handling breathe
    }

    detachInterrupt(digitalPinToInterrupt(bruceConfigPins.CC1101_bus.io0));
}
