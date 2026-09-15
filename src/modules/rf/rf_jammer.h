#ifndef __RF_JAMMER_H__
#define __RF_JAMMER_H__

#include <cstdint>

// ── RF Jammer modes ─────────────────────────────────────────────
enum RFJamMode : uint8_t {
    RF_JAM_FULL = 0,     // Continuous TX — maximum duty cycle
    RF_JAM_ITMT,         // Intermittent bursts — varied pulse patterns
    RF_JAM_NOISE,        // CC1101 PN9 random TX — hardware noise generator
    RF_JAM_SWEEP,        // Frequency sweep around target — CC1101 only
    RF_JAM_MODE_COUNT
};

class RFJammer {
public:
    RFJammer(bool full = false);
    ~RFJammer();

    void setup();

private:
    int nTransmitterPin;
    bool sendRF = true;
    bool isCC1101 = false;
    RFJamMode jamMode = RF_JAM_FULL;
    uint32_t pulseCount = 0;
    uint32_t sweepCount = 0;

    RFJamMode showModeMenu(bool defaultFull);
    void display_banner();
    void update_display(uint32_t elapsedMs);
    void run_full_jammer();
    void run_itmt_jammer();
    void run_noise_jammer();
    void run_sweep_jammer();
    void send_optimized_pulse(int width);
    void send_random_pattern(int numPulses);
};

#endif
