#include "rf_bruteforce.h"
#include "rf_utils.h"

static float brute_frequency = 433.92;
static int brute_protocol_idx = 1; // Default: Nice
static int brute_repeats = 1;

static void rf_brute_frequency() {
    options = {};
    int ind = 0;
    int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
    for (int i = 0; i < arraySize; i++) {
        String tmp = String(subghz_frequency_list[i], 2) + "MHz";
        options.push_back({tmp.c_str(), [=]() { brute_frequency = subghz_frequency_list[i]; }});
        if (int(brute_frequency * 100) == int(subghz_frequency_list[i] * 100)) ind = i;
    }
    loopOptions(options, ind);
}

static void rf_brute_protocol() {
    options = {};
    for (int i = 0; i < BRUTE_PROTOCOL_COUNT; i++) {
        options.push_back({brute_protocols[i].name, [=]() { brute_protocol_idx = i; }});
    }
    loopOptions(options, brute_protocol_idx);
}

static void rf_brute_repeats() {
    options = {};
    for (int i = 1; i <= 5; i++) {
        options.push_back({String(i).c_str(), [=]() { brute_repeats = i; }});
    }
    loopOptions(options, brute_repeats - 1);
}

static inline void sendPulse(int txpin, int duration) {
    if (duration > 0) {
        digitalWrite(txpin, HIGH);
        delayMicroseconds(duration);
    } else if (duration < 0) {
        digitalWrite(txpin, LOW);
        delayMicroseconds(-duration);
    }
}

static bool rf_brute_start() {
    int txpin;

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        txpin = bruceConfigPins.CC1101_bus.io0;
        if (!initRfModule("tx", brute_frequency)) return false;
    } else {
        txpin = bruceConfigPins.rfTx;
        if (!initRfModule("tx")) return false;
    }

    const BruteProtocol &proto = brute_protocols[brute_protocol_idx];
    const int total = 1 << proto.bits;

    pinMode(txpin, OUTPUT);
    setMHZ(brute_frequency);

    for (int code = 0; code < total; ++code) {
        for (int r = 0; r < brute_repeats; ++r) {
            // Pilot/sync period
            if (proto.pilot[0] || proto.pilot[1]) {
                sendPulse(txpin, proto.pilot[0]);
                sendPulse(txpin, proto.pilot[1]);
            }

            // Data bits (MSB first)
            for (int j = proto.bits - 1; j >= 0; --j) {
                const int *timings = ((code >> j) & 1) ? proto.one : proto.zero;
                sendPulse(txpin, timings[0]);
                sendPulse(txpin, timings[1]);
            }

            // Stop bit
            if (proto.stop[0] || proto.stop[1]) {
                sendPulse(txpin, proto.stop[0]);
                sendPulse(txpin, proto.stop[1]);
            }
        }

        if (check(EscPress)) break;

        if (code % 10 == 0) {
            displayRedStripe(
                String(code) + "/" + String(total) + " " + proto.name,
                getComplementaryColor2(bruceConfig.priColor),
                bruceConfig.priColor
            );
        }
    }

    digitalWrite(txpin, LOW);
    deinitRfModule();
    return true;
}

void rf_bruteforce() {
    while (true) {
        const BruteProtocol &proto = brute_protocols[brute_protocol_idx];
        int option = 0;
        options = {
            {"Frequency: " + String(brute_frequency, 2), [&]() { option = 1; }},
            {String("Protocol: ") + proto.name, [&]() { option = 2; }},
            {"Repeats: " + String(brute_repeats), [&]() { option = 3; }},
            {"Start", [&]() { option = 4; }},
            {"Main Menu", [&]() { option = 5; }},
        };
        loopOptions(options);

        switch (option) {
            case 1: rf_brute_frequency(); break;
            case 2: rf_brute_protocol(); break;
            case 3: rf_brute_repeats(); break;
            case 4: rf_brute_start(); break;
            case 5: return;
            default: return; // EscPress
        }
    }
}
