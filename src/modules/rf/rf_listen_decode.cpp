/*
 * rf_listen_decode.cpp
 *
 * CC1101 RF pulse capture + rtl_433 decode for Bruce firmware.
 *
 * Architecture:
 *   - onPulse() ISR accumulates alternating pulse/gap durations into a
 *     double-buffer of RawTrain structs (no heap allocation in ISR).
 *   - When a gap longer than PULSE_TRAIN_TIMEOUT_US is detected the active
 *     buffer is marked ready and the ISR swaps to the other buffer.
 *   - The main loop detects the ready buffer, heap-allocates a pulse_data_t,
 *     copies data in, and calls processSignal() which queues it to the
 *     rtl_433_DecoderTask running on Core 1.
 *   - Decoded results arrive via onDecoderResult() callback as JSON and are
 *     printed to Serial.
 *
 * CC1101 register notes (raw overrides after ELECHOUSE high-level calls):
 *   AGCCTRL2 = 0xC7  disable AGC / max gain    — critical for OOK sensitivity
 *   PKTLEN   = 0x00  infinite / raw packet mode — prevent early truncation
 *   MDMCFG2  = 0x30  OOK, no sync word, no Manchester — allows raw GDO0 output
 *   MDMCFG4  = 0x07  RX bandwidth (guards against ELECHOUSE rounding)
 *   MDMCFG3  = 0x93  data rate    (guards against ELECHOUSE rounding)
 */

#include "rf_listen_decode.h"

#include "../others/audio.h"
#include "signalDecoder.h" // pulse_data_t, processSignal(), rtlSetup(), _setCallback()

// ─── Pulse_data_t sizing guard ───────────────────────────────────────────────
// rtl_433 defines PD_MAX_PULSES (typically 1200). Our buffer must not exceed it.
#ifndef PD_MAX_PULSES
#define PD_MAX_PULSES 1200
#endif
static_assert(PULSE_BUF_SIZE <= PD_MAX_PULSES, "PULSE_BUF_SIZE must be <= PD_MAX_PULSES (1200)");

// ─── ISR double-buffer ───────────────────────────────────────────────────────
// Plain POD struct — no heap, no constructors, safe in IRAM context.
struct RawTrain {
    int pulse[PULSE_BUF_SIZE]; // mark durations  (µs)
    int gap[PULSE_BUF_SIZE];   // space durations (µs)
    int num_pulses;            // pulse/gap pairs stored
    bool ready;                // main loop may read this buffer
};

static RawTrain rawBuf[2];
static volatile int writeBuf = 0; // ISR writes here
static volatile bool trainReady = false;
static volatile float captureFreqMhz = 433.92f;

// ISR edge-tracking state
static volatile unsigned long isrLastEdge = 0;
static volatile bool isrInPulse = false;
static File file;
static char line[100];

// ─── ISR ─────────────────────────────────────────────────────────────────────
/**
 * Called on every CHANGE of CC1101 GDO0.
 *
 * GDO0 is HIGH during a carrier burst (mark/pulse) and LOW during silence (gap).
 *
 * Pulse/gap storage follows the rtl_433 convention:
 *   pulse[i] = duration of the i-th HIGH period
 *   gap[i]   = duration of the LOW period that follows pulse[i]
 *
 * End-of-train detection:
 *   A falling edge after a suspiciously long HIGH (>= PULSE_TRAIN_TIMEOUT_US)
 *   is treated as noise/stray and resets the buffer.
 *   A LOW period >= PULSE_TRAIN_TIMEOUT_US (detected on the next rising edge)
 *   marks the inter-message gap and triggers a buffer swap.
 */
void IRAM_ATTR onPulse_decode() {
    unsigned long now = micros();
    unsigned long dur = now - isrLastEdge;
    isrLastEdge = now;

    bool lineHigh = digitalRead(bruceConfigPins.CC1101_bus.io0);
    RawTrain &t = rawBuf[writeBuf];

    if (lineHigh) {
        // ── Rising edge ──────────────────────────────────────────────────────
        // 'dur' is the gap that just ended.

        if (dur >= PULSE_TRAIN_TIMEOUT_US) {
            // Inter-message gap: ship the completed train if it has content.
            if (t.num_pulses > 1 && !t.ready) {
                t.ready = true;
                trainReady = true;
                // Swap to the other buffer; reset it for the next train.
                writeBuf ^= 1;
                rawBuf[writeBuf].num_pulses = 0;
                rawBuf[writeBuf].ready = false;
            } else {
                // Nothing useful accumulated — just reset.
                t.num_pulses = 0;
            }
        } else {
            // Normal gap: store it against the preceding pulse slot.
            int idx = t.num_pulses - 1;
            if (t.num_pulses > 0 && idx < PULSE_BUF_SIZE) { t.gap[idx] = (int)dur; }
        }
        isrInPulse = true;

    } else if (isrInPulse) {
        // ── Falling edge ─────────────────────────────────────────────────────
        // 'dur' is the pulse that just ended.
        isrInPulse = false;

        if (dur >= PULSE_TRAIN_TIMEOUT_US) {
            // Absurdly long HIGH — almost certainly noise; reset buffer.
            t.num_pulses = 0;
            return;
        }

        int idx = t.num_pulses;
        if (idx < PULSE_BUF_SIZE) {
            t.pulse[idx] = (int)dur;
            t.gap[idx] = 0; // gap will be filled on next rising edge
            t.num_pulses++;
        }
        // Buffer full — silently drop further pulses; train will still decode
        // with however many pulses fit (most protocols repeat anyway).
    }
}

// ─── pulse_data_t builder ────────────────────────────────────────────────────
/**
 * Heap-allocates a pulse_data_t, copies the completed RawTrain into it, and
 * fills in metadata fields expected by the rtl_433 decoder.
 *
 * Ownership passes to the caller; processSignal() will free() it after decoding.
 *
 * @param  t    Completed RawTrain (must have ready == true).
 * @return      Heap-allocated pulse_data_t*, or nullptr on alloc failure.
 */
static pulse_data_t *buildPulseData(const RawTrain &t) {
    if (t.num_pulses < 2) return nullptr;

    pulse_data_t *pd = (pulse_data_t *)calloc(1, sizeof(pulse_data_t));
    if (!pd) return nullptr;

    int count = min(t.num_pulses, PD_MAX_PULSES);
    unsigned long totalDuration = 0;

    for (int i = 0; i < count; i++) {
        pd->pulse[i] = t.pulse[i];
        pd->gap[i] = t.gap[i];
        totalDuration += (unsigned long)t.pulse[i] + (unsigned long)t.gap[i];
    }

    pd->num_pulses = count;
    pd->signalDuration = totalDuration;
    pd->signalRssi = 0;      // optionally: ELECHOUSE_cc1101.getRssi()
    pd->sample_rate = 1.0e6; // µs timestamps → 1 MHz effective rate
    pd->freq1_hz = (uint32_t)(captureFreqMhz * 1.0e6f);

    return pd;
}

// ─── Decoder result callback ─────────────────────────────────────────────────
static char decoderMsgBuf[RF_DECODE_MSG_BUF_SIZE];

/**
 * Called by rtl_433_DecoderTask (Core 1) with a JSON string on every decode
 * event, including "undecoded signal" reports when PUBLISH_UNPARSED is defined.
 */
static void onDecoderResult(char *message) { 
    Serial.println(message);
    file.write((const uint8_t *)message, strlen(message));
    file.write((const uint8_t *)"\n", 2);
}

// ─── Main function ───────────────────────────────────────────────────────────
void rf_listen_decode() {
    float freq = 433.92f;
    float last_freq = -1.0f;

    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) {
        displayError("No space left on device", true);

    }

    char filename[32];
    int index = 0;

    if (!fs->exists("/BruceRF")) {
        if (!fs->mkdir("/BruceRF")) {
            displayError("Error creating directory", true);
        }
    }

    do { snprintf(filename, sizeof(filename), "/BruceRF/rtl433_%d.sub", index++); } while (fs->exists(filename));

    file = fs->open(filename, FILE_WRITE, true);
    if (!file) {
        displayError("Error creating file", true);
    }

    file.write((const uint8_t *)"Filetype: RTL433 decoded signal File\n", 38);

    int len = snprintf(line, sizeof(line), "Frequency: %d\n", (int)(freq));
    file.write((const uint8_t *)line, len);

    // ── 1. Frequency selection UI ────────────────────────────────────────────
    while (!check(SelPress) && !check(EscPress)) {
        if (check(PrevPress)) freq -= 0.1f;
        if (check(NextPress)) freq += 0.1f;
        freq = constrain(freq, 300.0f, 928.0f);

        if (freq != last_freq) {
            last_freq = freq;
            String text = String("Frequency: ") + String(freq, 2) + String(" MHz");
            displayRedStripe(text, getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);
        }
        if (check(EscPress) || check(SelPress)) break;
    }
    if (check(EscPress)) return;

    // ── 2. Hardware validation ───────────────────────────────────────────────
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Listener needs a CC1101!", true);
        return;
    }
    if (!initRfModule("rx", freq)) {
        displayError("CC1101 not found!", true);
        return;
    }

    // ── 3. CC1101 configuration ──────────────────────────────────────────────
    // High-level ELECHOUSE calls first, then raw register overrides that
    // match the settings used by the rtl_433_ESP library (receiver.cpp).

    ELECHOUSE_cc1101.setMHZ(freq);
    ELECHOUSE_cc1101.setRxBW(270);         // 270 kHz RX bandwidth
    ELECHOUSE_cc1101.setModulation(2);     // 2 = OOK/ASK
    ELECHOUSE_cc1101.setDcFilterOff(true); // keep DC component for OOK
    ELECHOUSE_cc1101.setPA(10);            // 10 dBm TX power (idle during RX)
    ELECHOUSE_cc1101.setPRE(32);           // 32-bit preamble
    ELECHOUSE_cc1101.setDRate(17.24);      // 17.24 kbps symbol rate

    // Raw register overrides — must come AFTER the high-level calls above
    // because some ELECHOUSE setters touch these same registers.

    // Disable AGC, force maximum LNA + VGA gain.
    // rtl_433_ESP sets 0xC7 here; without it the AGC actively fights OOK
    // carrier bursts and causes missed or clipped pulses.
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL2, 0xC7);

    // Infinite / raw packet length.
    // Prevents the CC1101 packet engine from asserting end-of-packet
    // mid-train and suppressing GDO0 output.
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTLEN, 0x00);

    // OOK modulation, no sync-word filter, no Manchester encoding.
    // Bits: [6:4]=011 (OOK), [3:2]=00 (no Manchester), [1:0]=00 (no sync).
    // Critically: without clearing the sync-word bits, GDO0 stays LOW until
    // the CC1101 sees a matching sync word — raw pulses are never output.
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG2, 0x30);

    // Pin RX bandwidth register directly (guards against ELECHOUSE rounding).
    // 0x07 = CHANBW_E=0, CHANBW_M=1 → ~270 kHz at 26 MHz crystal.
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG4, 0x07);

    // Pin data rate register directly (guards against ELECHOUSE rounding).
    // 0x93 → 17.24 kbps at 26 MHz crystal, matching rtl_433_ESP.
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG3, 0x93);

    // Start reception — must be the last CC1101 call.
    ELECHOUSE_cc1101.SetRx();

    // ── 4. rtl_433 decoder stack initialisation ──────────────────────────────
    // rtlSetup() is idempotent (guards on cfg->demod == nullptr) so re-entering
    // rf_listen() won't spawn duplicate FreeRTOS tasks or queues.
    captureFreqMhz = freq;
    _setCallback(onDecoderResult, decoderMsgBuf, sizeof(decoderMsgBuf));
    rtlSetup();

    // ── 5. Capture buffer reset ──────────────────────────────────────────────
    memset(&rawBuf[0], 0, sizeof(RawTrain));
    memset(&rawBuf[1], 0, sizeof(RawTrain));
    writeBuf = 0;
    trainReady = false;
    isrLastEdge = micros();
    isrInPulse = false;

    // ── 6. Attach interrupt ──────────────────────────────────────────────────
    attachInterrupt(digitalPinToInterrupt(bruceConfigPins.CC1101_bus.io0), onPulse_decode, CHANGE);

    Serial.printf("# RF listen+decode started — %.2f MHz\n", freq);
    displayRedStripe("Listening...", getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);

    // Drain any queued Esc event that may have triggered the Sel confirm above.
    while (check(EscPress)) delay(10);

    unsigned long lastActivity = millis();
    unsigned long lastIdleDisp = 0;

    // ── 7. Capture / decode loop ─────────────────────────────────────────────
    while (!check(EscPress)) {

        if (trainReady) {
            // readBuf is the buffer the ISR just finished writing;
            // writeBuf has already been swapped to the other slot.
            int readBuf = writeBuf ^ 1;

            if (rawBuf[readBuf].ready) {
                int count = rawBuf[readBuf].num_pulses;

                // Display pulse count while decoder runs asynchronously.
                displayRedStripe(
                    String("Decoding: ") + String(count) + String(" pulses"),
                    getComplementaryColor2(bruceConfig.priColor),
                    bruceConfig.priColor
                );

                // Short audio feedback.
#if defined(BUZZ_PIN)
                tone(BUZZ_PIN, 1000, 60);
#elif defined(HAS_NS4168_SPKR)
                playTone(1000, 60, 0);
#endif

                // Build heap-allocated pulse_data_t and hand ownership to the
                // decoder queue. processSignal() free()s it after decoding.
                pulse_data_t *pd = buildPulseData(rawBuf[readBuf]);
                if (pd) {
                    processSignal(pd);
                } else {
                    Serial.println("# buildPulseData: alloc failed or too few pulses");
                }

                // Release buffer slot back to the ISR.
                rawBuf[readBuf].ready = false;
                trainReady = false;
                lastActivity = millis();
            }
        }

        // Idle indicator — update at most once per 3 s to avoid display spam.
        if (millis() - lastActivity > 3000 && millis() - lastIdleDisp > 3000) {
            displayRedStripe(
                "Waiting for signal", getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor
            );
            lastIdleDisp = millis();
        }

        delay(5);
    }

    // ── 8. Cleanup ───────────────────────────────────────────────────────────
    detachInterrupt(digitalPinToInterrupt(bruceConfigPins.CC1101_bus.io0));
    Serial.println("# RF listen+decode stopped.");
    file.flush();
    file.close();
}
