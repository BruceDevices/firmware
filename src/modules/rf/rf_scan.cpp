#include "rf_scan.h"
#include "core/led_control.h"
#include "core/sd_functions.h"
#include "core/type_convertion.h"
#include "protocols/rf_config.h"   // RF_DBG
#include "protocols/rf_registry.h" // rf_flipper_protocol_name
#include "rf_send.h"
#include <globals.h>
#include <sstream>

#define RF_M5_SCAN_COOLDOWN_MS 500
#define RF_M5_SCAN_DUPLICATE_MS 1500
#define RF_M5_SCAN_RESYNC_MS 20
#define RF_M5_RAW_MIN_BITS 24
#define RF_M5_RAW_MIN_TE_US 300

static void rf_clear_nav_state() {
    NextPress = false;
    PrevPress = false;
    UpPress = false;
    DownPress = false;
    SelPress = false;
    EscPress = false;
    AnyKeyPress = false;
    SerialCmdPress = false;
    forceMenuOption = -1;
}

static bool rf_m5_raw_is_plausible(bool hasCrc, int rawBits, int rawTe) {
    return hasCrc && rawBits >= RF_M5_RAW_MIN_BITS && rawTe >= RF_M5_RAW_MIN_TE_US;
}

RFScan::RFScan() {
    if (bruceConfigPins.rfModule == M5_RF_MODULE) ReadRAW = false;
    setup();
}

RFScan::~RFScan() {
    _rx.end();
    deinitRfModule();
}

void RFScan::setup() {
    rf_clear_nav_state();
    returnToMenu = false;
    exitRequested = false;

    if (!initRfModule("rx", bruceConfigPins.rfFreq)) { return; }

    enable_receive();

    if (bruceConfigPins.rfScanRange < 0 || bruceConfigPins.rfScanRange > 3) {
        bruceConfigPins.setRfScanRange(3);
    }
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) { bruceConfigPins.setRfFxdFreq(1); }

    display_info(received, signals, ReadRAW, codesOnly, autoSave, title);

    if (bruceConfigPins.rfFxdFreq) frequency = bruceConfigPins.rfFreq;

    restartScan = false;

    return loop();
}

void RFScan::loop() {
    while (1) {
        if (EscPress || exitRequested) {
            RF_DBG("RFScan exit: esc=%d exitRequested=%d", (int)EscPress, (int)exitRequested);
            check(EscPress);
            return;
        }
        if (check(NextPress)) {
            select_menu_option();
            if (exitRequested) {
                RF_DBG("RFScan exit after options");
                return;
            }
            return setup();
        }
        if (restartScan) {
            RF_DBG("RFScan restart");
            return setup();
        }

        if (bruceConfigPins.rfFxdFreq) frequency = bruceConfigPins.rfFreq;
        if (frequency <= 0) init_freqs();

        while (frequency <= 0) { // FastScan
            if (EscPress || exitRequested) {
                RF_DBG(
                    "RFScan fast-scan exit: esc=%d exitRequested=%d", (int)EscPress, (int)exitRequested
                );
                check(EscPress);
                return;
            }
            if (check(NextPress)) {
                select_menu_option();
                if (exitRequested) {
                    RF_DBG("RFScan fast-scan exit after options");
                    return;
                }
                return setup();
            }

            if (fast_scan()) return setup(); // frequency found, reset
        }

        std::vector<int> durations;
        if (_rx.poll(durations)) {
            bool captured = false;
            if (!ReadRAW) {
                captured = decode_signal(durations);
                if (captured && autoSave && (lastSavedKey != received.key || received.key == 0)) save_signal();
            } else {
                captured = read_raw(durations);
                if (captured && autoSave && (lastSavedKey != received.key || received.key == 0)) save_signal();
            }
            if (captured && bruceConfigPins.rfModule == M5_RF_MODULE) {
                _rx.end();
                rf_clear_nav_state();
                returnToMenu = false;
                vTaskDelay(RF_M5_SCAN_COOLDOWN_MS / portTICK_PERIOD_MS);
                enable_receive();
                continue;
            }
            if (!captured && bruceConfigPins.rfModule == M5_RF_MODULE) {
                _rx.end();
                vTaskDelay(RF_M5_SCAN_RESYNC_MS / portTICK_PERIOD_MS);
                enable_receive();
                continue;
            }
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void RFScan::enable_receive() {
    // (Re)start the native RMT RX session used to capture signals.
    _rx.end();
    _rx.begin();
}

void RFScan::init_freqs() {
    for (int i = 0; i < _MAX_TRIES; i++) {
        _freqs[i].freq = 433.92;
        _freqs[i].rssi = -75;
    }
    _try = 0;
}

bool RFScan::fast_scan() {

    if (idx < range_limits[bruceConfigPins.rfScanRange][0] ||
        idx > range_limits[bruceConfigPins.rfScanRange][1]) {
        idx = range_limits[bruceConfigPins.rfScanRange][0];
    }
    float checkFrequency = subghz_frequency_list[idx];
    setMHZ(checkFrequency);
    tft.drawPixel(0, 0, 0); // To make sure CC1101 shared with TFT works properly
    vTaskDelay(5 / portTICK_PERIOD_MS);
    rssi = ELECHOUSE_cc1101.getRssi();
    if (rssi > rssiThreshold) {
        _freqs[_try].freq = checkFrequency;
        _freqs[_try].rssi = rssi;
        _try++;
        if (_try >= _MAX_TRIES) {
            int max_index = 0;
            for (int i = 1; i < _MAX_TRIES; ++i) {
                if (_freqs[i].rssi > _freqs[max_index].rssi) { max_index = i; }
            }

            bruceConfigPins.setRfFreq(_freqs[max_index].freq, 1); // change to fixed frequency
            frequency = _freqs[max_index].freq;
            setMHZ(frequency);
            Serial.println("Frequency Found: " + String(frequency));
            // When changing to fixed frequency, need to restart the module to reset the registers
            // so we get good signal reception at this frequency
            deinitRfModule();

            return true;
        }
    }
    ++idx;
    return false;
}

void keeloq_identify(RfCodes &instance) {
    // A null fs is fine: the keystore falls back to the encrypted built-in keys.
    KeeloqKeystore keystore{keeloq_mfcodes_fs()};

    // Secure/Erreka need a seed; mirror the reference fallback to the
    // serial-derived seed when none was captured from the frame.
    uint32_t seed_eff = instance.seed ? instance.seed : (instance.fix & 0x0FFFFFFF);

    for (const auto &key : keystore.get_keys()) {
        // Unified derivation: every learning type turns (fix, seed, key) into the
        // manufacturer key, then we decrypt the captured `encrypted` with it.
        uint64_t man = keeloq_derive_man(key.type, instance.fix, seed_eff, key.key);
        uint64_t decrypt = keeloq_decrypt(instance.encrypted, man);

        if (key.mf_name == "Centurion") {
            if (instance.keeloq_check_decrypt_centurion(decrypt)) {
                instance.mf_name = key.mf_name;
                instance.hop = decrypt;
                return;
            }
        }

        if (instance.keeloq_check_decrypt(decrypt)) {
            instance.mf_name = key.mf_name;
            instance.hop = decrypt;
            return;
        }
    }
}

// Try to decode a KeeLoq frame and resolve its manufacturer. On success fills
// the keeloq fields (key/fix/encrypted/serial/btn + mf_name/cnt via the
// keystore) and returns true. Shared by the scan and CLI receive paths.
bool rf_try_keeloq(const std::vector<int> &durations, RfCodes &received) {
    if (!rf_decode_keeloq(durations, received)) return false;

    uint64_t yek = reverse_bits(received.key, 64);
    received.fix = yek >> 32;
    received.btn = received.fix >> 28;
    received.encrypted = yek & 0xFFFFFFFF;
    received.serial = (yek >> 32) & 0xFFFFFFF;
    received.seed = 0;

    keeloq_identify(received); // sets mf_name + cnt when a keystore entry matches
    RF_DBG(
        "decode keeloq MATCH mf=%s btn=%u cnt=%04X key=%llX",
        received.mf_name.c_str(),
        (unsigned)received.btn,
        (unsigned)received.cnt,
        (unsigned long long)received.key
    );
    return true;
}

bool RFScan::decode_signal(const std::vector<int> &durations) {
    received.fix = 0;
    received.hop = 0;
    received.btn = 0;
    received.cnt = 0;
    received.mf_name = "Unknown";
    received.encrypted = 0;

    // Decode-only mode: show the signal only if a registry protocol (or KeeLoq)
    // matched.
    if (rf_try_keeloq(durations, received) || rf_decode_ook(durations, received)) {
        if (is_m5_duplicate_capture(received)) return false;

        Serial.println("Decoded signal captured: " + received.protocol);
        blinkLed();
        ++signals;
        found_freq = frequency;
        received.frequency = long(frequency * 1000000);
        received.filepath = "signal_" + String(signals);
        received.data = "";

        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
        return true;
    }

    if (bruceConfigPins.rfModule == M5_RF_MODULE) {
        String _data;
        bool hasCrc = false;
        uint64_t crc = 0;
        std::vector<int> indexed_durations;
        int rawBits = 0;
        int rawTe = 0;
        rf_build_raw(durations, _data, hasCrc, crc, indexed_durations, rawBits, rawTe);
        RF_DBG("m5 scan discard: crc=%d bits=%d te=%d", (int)hasCrc, rawBits, rawTe);
    }
    return false;
}

bool RFScan::read_raw(const std::vector<int> &durations) {
    found_freq = frequency;

    received.fix = 0;
    received.hop = 0;
    received.btn = 0;
    received.cnt = 0;
    received.mf_name = "Unknown";
    received.encrypted = 0;

    // Build the RAW representation (durations string + optional CRC).
    String _data;
    bool hasCrc = false;
    uint64_t crc = 0;
    std::vector<int> indexed_durations;
    int rawBits = 0;
    int rawTe = 0;
    rf_build_raw(durations, _data, hasCrc, crc, indexed_durations, rawBits, rawTe);

    received.data = _data;
    received.te = rawTe;
    received.filepath = "signal_" + String(signals);
    received.frequency = long(frequency * 1000000);

    // if a registry protocol (or KeeLoq) decoded the signal, show it
    if (rf_try_keeloq(durations, received) || rf_decode_ook(durations, received)) {
        if (is_m5_duplicate_capture(received)) return false;

        Serial.println("Decoded signal captured: " + received.protocol);
        blinkLed();
        ++signals;
        received.indexed_durations = {};

        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
        return true;
    }
    // no decode, but a repeated pattern gave us a CRC
    else if (hasCrc) {
        if (bruceConfigPins.rfModule == M5_RF_MODULE &&
            !rf_m5_raw_is_plausible(hasCrc, rawBits, rawTe)) {
            RF_DBG(
                "m5 raw discard: crc=%d bits=%d te=%d minBits=%d minTe=%d",
                (int)hasCrc,
                rawBits,
                rawTe,
                RF_M5_RAW_MIN_BITS,
                RF_M5_RAW_MIN_TE_US
            );
            return false;
        }
        received.preset = "Ook270Async";
        received.protocol = "RAW";
        received.key = crc;
        if (is_m5_duplicate_capture(received)) return false;

        Serial.println("Raw signal captured");
        blinkLed();
        ++signals;
        received.indexed_durations = indexed_durations;
        received.Bit = rawBits;
        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
        return true;
    }
    // no decode and no CRC: only show the raw data when not filtering for codes
    else if (!codesOnly) {
        if (bruceConfigPins.rfModule == M5_RF_MODULE) {
            RF_DBG("m5 raw discard: crc=0 bits=%d te=%d", rawBits, rawTe);
            return false;
        }
        Serial.println("Raw data captured");
        blinkLed();
        ++signals;
        received.preset = "Ook270Async";
        received.protocol = "RAW";
        received.key = 0;
        received.indexed_durations = {};
        received.Bit = 0;
        frequency = 0;
        display_info(received, signals, ReadRAW, codesOnly, autoSave, title);
        return true;
    }
    return false;
}

bool RFScan::is_m5_duplicate_capture(const RfCodes &data) {
    if (bruceConfigPins.rfModule != M5_RF_MODULE) return false;

    unsigned long now = millis();
    bool duplicate = data.key == lastM5CaptureKey && data.protocol == lastM5CaptureProtocol &&
                     now - lastM5CaptureMs < RF_M5_SCAN_DUPLICATE_MS;
    if (duplicate) {
        RF_DBG(
            "m5 duplicate ignored: proto=%s key=%llX age=%lums",
            data.protocol.c_str(),
            (unsigned long long)data.key,
            now - lastM5CaptureMs
        );
        return true;
    }

    lastM5CaptureKey = data.key;
    lastM5CaptureProtocol = data.protocol;
    lastM5CaptureMs = now;
    return false;
}

void RFScan::select_menu_option() {
    _rx.end(); // stop the RMT receiver while the menu is open

    options = {};

    if (received.protocol != "") options.emplace_back("Replay", [this]() { set_option(REPLAY); });
    if (received.data != "" && received.protocol != "RAW")
        options.emplace_back("Replay as RAW", [this]() { set_option(REPLAY_RAW); });

    if (received.protocol != "") options.emplace_back("Save Signal", [this]() { set_option(SAVE); });
    if (received.data != "" && received.protocol != "RAW")
        options.emplace_back("Save as RAW", [this]() { set_option(SAVE_RAW); });

    if (received.protocol != "") options.emplace_back("Reset Signal", [this]() { set_option(RESET); });

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE)
        options.emplace_back("Range", [this]() { set_option(RANGE); });
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE && !bruceConfigPins.rfFxdFreq)
        options.emplace_back("Threshold", [this]() { set_option(THRESHOLD); });

    if (ReadRAW)
        options.emplace_back("Mode = RAW", [&]() {
            ReadRAW = false;
            return select_menu_option();
        });
    else
        options.emplace_back("Mode = Decode", [&]() {
            ReadRAW = true;
            return select_menu_option();
        });

    if (ReadRAW && codesOnly)
        options.emplace_back("Filter = Code", [&]() {
            codesOnly = false;
            return select_menu_option();
        });
    else if (ReadRAW)
        options.emplace_back("Filter = All", [&]() {
            codesOnly = true;
            return select_menu_option();
        });

    if (autoSave)
        options.emplace_back("Save = Auto", [&]() {
            autoSave = false;
            return select_menu_option();
        });
    else
        options.emplace_back("Save = Manual", [&]() {
            autoSave = true;
            return select_menu_option();
        });

    options.emplace_back("Close Menu", [this]() { set_option(CLOSE_MENU); });
    options.emplace_back("Main Menu", [this]() { set_option(MAIN_MENU); });

    loopOptions(options);
}

void RFScan::set_option(RFMenuOption option) {
    switch (option) {
        case REPLAY:
        case REPLAY_RAW: replay_signal(option == REPLAY_RAW); break;

        case SAVE:
        case SAVE_RAW: save_signal(option == SAVE_RAW); break;

        case RANGE: rf_range_selection(); break; // using a common function to other features
        case RESET: reset_signals(); break;
        case THRESHOLD: set_threshold(); break;

        case CLOSE_MENU: break;

        case MAIN_MENU:
            exitRequested = true;
            returnToMenu = true;
            return;
    }

    restartScan = true;
    deinitRfModule();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void RFScan::replay_signal(bool asRaw) {
    String actualProtocol = received.protocol;
    if (asRaw) { received.protocol = "RAW"; }
    displayTextLine("Sending..");
    sendRfCommand(received);
    addToRecentCodes(received);
    received.protocol = actualProtocol;

    if (received.fix != 0 && !asRaw) { received.keeloq_step(1); }
}

void RFScan::save_signal(bool asRaw) {
    asRaw = asRaw || received.protocol == "RAW";
    Serial.println(asRaw ? "rfSaveSignal RAW true" : "rfSaveSignal RAW false");
    decimalToHexString(received.key, hexString);
    rfSaveSignal(found_freq, received, asRaw, hexString, autoSave);
    lastSavedKey = received.key;
}

void RFScan::reset_signals() {
    received.Bit = 0;
    received.data = "";
    received.key = 0;
    received.preset = "";
    received.protocol = "";
    signals = 0;
    received.fix = 0;
    received.hop = 0;
    received.btn = 0;
    received.cnt = 0;
    received.mf_name = "Unknown";
    received.encrypted = 0;
}

void RFScan::set_threshold() {
    options = {
        {"(-55) More Accurate", [&]() { rssiThreshold = -55; }},
        {"(-60)",               [&]() { rssiThreshold = -60; }},
        {"(-65) Default ",      [&]() { rssiThreshold = -65; }},
        {"(-70)",               [&]() { rssiThreshold = -70; }},
        {"(-75)",               [&]() { rssiThreshold = -75; }},
        {"(-80) Less Accurate", [&]() { rssiThreshold = -80; }},
    };
    loopOptions(options);
}
/*
// Using similar function from rf_utils.h
void RFScan::set_range() {
    bool chooseFixedOpt = false;

    options = {
        {String("Fxd [" + String(bruceConfigPins.rfFreq) + "]").c_str(),
         [=]() { bruceConfigPins.setRfScanRange(bruceConfigPins.rfScanRange, 1); } },
        {"Choose Fxd",                                                   [&]() { chooseFixedOpt = true; } },
        {subghz_frequency_ranges[0],                                     [=]() {
bruceConfigPins.setRfScanRange(0); }}, {subghz_frequency_ranges[1],                                     [=]()
{ bruceConfigPins.setRfScanRange(1); }}, {subghz_frequency_ranges[2], [=]() {
bruceConfigPins.setRfScanRange(2); }}, {subghz_frequency_ranges[3],                                     [=]()
{ bruceConfigPins.setRfScanRange(3); }},
    };

    loopOptions(options);

    if (chooseFixedOpt) { // Range
        options.clear();
        int ind = 0;
        int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
        for (int i = 0; i < arraySize; i++) {
            String tmp = String(subghz_frequency_list[i], 2) + "Mhz";
            options.emplace_back(tmp.c_str(), [=]() { bruceConfigPins.rfFreq = subghz_frequency_list[i]; });
            if (int(frequency * 100) == int(subghz_frequency_list[i] * 100)) ind = i;
        }
        loopOptions(options, ind);
        options.clear();
        bruceConfigPins.setRfScanRange(bruceConfigPins.rfScanRange, 1);
    }

    if (bruceConfigPins.rfFxdFreq) displayTextLine("Scan freq set to " + String(bruceConfigPins.rfFreq));
    else displayTextLine("Range set to " + String(subghz_frequency_ranges[bruceConfigPins.rfScanRange]));
}
*/
// Routes one info line to the right sink: Serial when running headless (CLI),
// otherwise the on-screen padded print used by the interactive scanner.
static void rf_info_line(bool headless, const String &s) {
    if (headless) Serial.println(s);
    else padprintln(s);
}

void display_info(
    RfCodes received, int signals, bool ReadRAW, bool codesOnly, bool autoSave, const String &title,
    bool headless
) {
    if (!headless) {
        if (title != "") drawMainBorderWithTitle(title);
        else drawMainBorder();
    }

    if (received.protocol != "") display_signal_data(received, headless);

    if (!headless) tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);

    if (!ReadRAW) rf_info_line(headless, "Recording: Only decoded codes.");
    else if (codesOnly) rf_info_line(headless, "Recording: RAW with CRC or decoded codes.");
    else rf_info_line(headless, "Recording: Any RAW signal.");

    if (autoSave) rf_info_line(headless, "Auto save: Enabled");

    if (bruceConfigPins.rfFxdFreq)
        rf_info_line(headless, "Scanning: " + String(bruceConfigPins.rfFreq) + " MHz");
    else rf_info_line(headless, "Scanning: " + String(subghz_frequency_ranges[bruceConfigPins.rfScanRange]));

    rf_info_line(headless, "Total signals found: " + String(signals));

    if (!headless) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        padprintln("");
        padprintln("Press [NEXT] for options.");
    }
}

void display_signal_data(RfCodes received, bool headless) {
    std::string txt = received.data.c_str();
    std::stringstream ss(txt);
    std::string palavra;
    int transitions = 0;
    char hexString[64];

    while (ss >> palavra) transitions++;

    if (received.preset != "") {
        if (received.fix != 0) {
            rf_info_line(headless, "Protocol: KeeLoq");
        } else rf_info_line(headless, "Protocol: " + String(received.protocol) + "(" + received.preset + ")");
    } else rf_info_line(headless, "Protocol: " + String(received.protocol));

    if (received.key > 0) {
        decimalToHexString(received.key, hexString);
        if (received.protocol == "RAW") {
            rf_info_line(headless, "Length: " + String(received.Bit) + " transitions");
            rf_info_line(headless, "Record length: " + String(transitions) + " transitions");
        } else {
            if (received.fix == 0) {
                rf_info_line(headless, "Length: " + String(received.Bit) + " bits");
                const char *b = dec2binWzerofill(received.key, min(received.Bit, 40));
                rf_info_line(headless, "Binary: " + String(b));
            }
        }
    } else {
        strlcpy(hexString, "No code identified", sizeof(hexString));
        rf_info_line(headless, "Length: No code identified");
        rf_info_line(headless, "Record length: " + String(transitions) + " transitions");
    }

    if (received.protocol == "RAW") rf_info_line(headless, "CRC: " + String(hexString));
    else {
        if (received.fix != 0) {
            rf_info_line(headless, "Manufacturer: " + received.mf_name);

            decimalToHexString(received.serial, hexString);
            rf_info_line(headless, "Serial: " + String(hexString));

            rf_info_line(headless, "Btn: " + String(received.btn));

            decimalToHexString(received.fix, hexString);
            rf_info_line(headless, "Fix: " + String(hexString));

            if (received.mf_name != "Unknown") {
                decimalToHexString(received.hop, hexString);
                rf_info_line(headless, "Hop: " + String(hexString));

                rf_info_line(headless, "Counter: " + String(received.cnt));
            } else {
                decimalToHexString(received.encrypted, hexString);
                rf_info_line(headless, "Encrypted: " + String(hexString));
            }
        } else {
            rf_info_line(headless, "Key: " + String(hexString));
        }
    }

    // if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
    //     int rssi = ELECHOUSE_cc1101.getRssi();
    //     tft.drawPixel(0, 0, 0);
    //     padprintln("Rssi: " + String(rssi));
    // }

    // if (!received.indexed_durations.empty()) {
    //     padprint("PulseLenghts: ");
    //     for (int i = 0; i < received.indexed_durations.size(); i++) {
    //         if (i < received.indexed_durations.size() - 1)
    //             tft.print(String(received.indexed_durations[i]) + "us, ");
    //         else tft.println(String(received.indexed_durations[i]) + "us");
    //     }
    // } else if (received.te) padprintln("PulseLenght: " + String(received.te) + "us");
    // else padprintln("PulseLenght: unknown");

    // padprintln("Frequency: " + String(received.frequency) + " Hz");
    rf_info_line(headless, "");
}

bool rfSaveSignal(float frequency, RfCodes codes, bool raw, char *key, bool autoSave) {
    FS *fs;
    String filename = "";

    if (!getFsStorage(fs)) {
        displayError("No space left on device", true);
        return false;
    }

    if (!codes.key && codes.data == "") {
        Serial.println("Empty signal, it was not saved.");
        return false;
    }

    String subfile_out = rf_subghz_header(frequency);
    if (!raw) {
        subfile_out += "Preset: " + String(codes.preset) + "\n";
        // Write the identified protocol under its Flipper-standard name (so the
        // `.sub` is portable), falling back to RcSwitch only if unidentified.
        subfile_out +=
            "Protocol: " +
            (codes.protocol == "" ? String("RcSwitch") : rf_flipper_protocol_name(codes.protocol)) + "\n";
        subfile_out += "Bit: " + String(codes.Bit) + "\n";
        // The Key (64-bit) is always written so the signal can be replayed; for
        // KeeLoq the rolling-code fields are added as informative extras.
        subfile_out += "Key: " + String(key) + "\n";
        if (codes.hop != 0) {
            char hexString[64] = {0};
            decimalToHexString(codes.serial, hexString);
            if (codes.seed != 0) {
                char seedHex[32] = {0};
                decimalToHexString(codes.seed, seedHex);
                subfile_out += "Seed: " + String(seedHex) + "\n";
            }
            subfile_out += "Manufacture: " + String(codes.mf_name) + "\n";
            subfile_out += "Serial: " + String(hexString) + "\n";
            subfile_out += "Button: " + String(codes.btn) + "\n";
            subfile_out += "Counter: " + String(codes.cnt) + "\n";
        }
        subfile_out += "TE: " + String(codes.te) + "\n";
        filename = "rcs.sub";
        // subfile_out += "RAW_Data: " + codes.data;
    } else {
        // save as raw
        if (codes.preset == "1") {
            codes.preset = "FuriHalSubGhzPresetOok270Async";
        } else if (codes.preset == "2") {
            codes.preset = "FuriHalSubGhzPresetOok650Async";
        }

        subfile_out += "Preset: " + String(codes.preset) + "\n";
        subfile_out += "Protocol: RAW\n";
        subfile_out += "RAW_Data: " + codes.data;
        filename = "raw.sub";
    }

    String filepath = "/BruceRF";
    if (autoSave) filepath += "/autoSaved";
    File file = createNewFile(fs, filepath, filename);

    if (file) {
        file.println(subfile_out);
        if (!autoSave) displaySuccess(file.path());
    } else {
        displayError("Error saving file", true);
    }

    file.close();
    return true;
}

String rf_scan(float start_freq, float stop_freq, int max_loops) {
    // derived from https://github.com/mcore1976/cc1101-tool/blob/main/cc1101-tool-esp32.ino#L480

    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("rf scanning is available with CC1101 only", true);
        return ""; // only CC1101 is supported for this
    }
    if (!initRfModule("rx", start_freq)) return "";

    ELECHOUSE_cc1101.setRxBW(256);

    float settingf1 = start_freq;
    float settingf2 = stop_freq;
    float freq = 0;
    long compare_freq = 0;
    float mark_freq;
    int rssi;
    int mark_rssi = -100;
    String out = "";

    while (max_loops || !check(EscPress)) {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        max_loops -= 1;

        setMHZ(freq);

        rssi = ELECHOUSE_cc1101.getRssi();
        if (rssi > -75) {
            if (rssi > mark_rssi) {
                mark_rssi = rssi;
                mark_freq = freq;
            };
        };

        freq += 0.01;

        if (freq > settingf2) {
            freq = settingf1;

            if (mark_rssi > -75) {
                long fr = mark_freq * 100;
                if (fr == compare_freq) {
                    Serial.print(F("\r\nSignal found at  "));
                    Serial.print(F("Freq: "));
                    Serial.print(mark_freq);
                    Serial.print(F(" Rssi: "));
                    Serial.println(mark_rssi);
                    out += String(mark_freq) + ",";
                    mark_rssi = -100;
                    compare_freq = 0;
                    mark_freq = 0;
                } else {
                    compare_freq = mark_freq * 100;
                    freq = mark_freq - 0.10;
                    mark_freq = 0;
                    mark_rssi = -100;
                };
            };
        }; // end of IF freq>stop frequency
    }; // End of While

    deinitRfModule();
    return out;
}

String rfReceiveSignal(float frequency, int max_loops, bool raw, bool headless) {
    RfCodes received;

    if (!frequency) frequency = bruceConfigPins.rfFreq; // default from config

    char hexString[64] = {0};

    if (!headless) {
        drawMainBorder();
        tft.setCursor(10, 28);
        tft.setTextSize(FP);
        tft.println("Waiting for a " + String(frequency) + " MHz " + "signal.");
    }

    // init native RMT receive
    if (!initRfModule("rx", frequency)) return "";
    RfRxSession rx;
    if (!rx.begin()) {
        deinitRfModule();
        return "";
    }

    while (!check(EscPress)) {
        std::vector<int> durations;
        if (rx.poll(durations)) {
            // In decode mode try KeeLoq, then the registry; raw mode skips decoding.
            bool decoded =
                (!raw) && (rf_try_keeloq(durations, received) || rf_decode_ook(durations, received));

            // Build the RAW representation (also the data string when decoded).
            String _data;
            bool hasCrc = false;
            uint64_t crc = 0;
            std::vector<int> indexed;
            int rawBits = 0, rawTe = 0;
            int transitions = rf_build_raw(durations, _data, hasCrc, crc, indexed, rawBits, rawTe);

            if (decoded) {
                received.frequency = long(frequency * 1000000);
                received.filepath = "unsaved";
                received.data = _data;
                decimalToHexString(received.key, hexString);
                // Interactive: draw on screen. Headless (CLI): emit the same
                // formatted info on Serial instead of the display.
                display_info(received, 1, raw, false, false, "", headless);
            } else if (raw && transitions > 20) {
                // Raw mode requested: keep undecoded captures as RAW.
                received.frequency = long(frequency * 1000000);
                received.protocol = "RAW";
                received.preset = "Ook270Async";
                received.te = rawTe;
                received.data = _data;
                received.filepath = "unsaved";
                display_info(received, 1, raw, false, false, "", headless);
            } else {
                received.data = ""; // too few transitions - discard
                received.key = 0;
            }
        }

        if (received.key > 0 ||
            received.data.length() > 20) { // RAW data does not have "key", 20 is more than 5 transitions
            bool outRaw = raw || received.protocol == "RAW";

            String subfile_out = rf_subghz_header(frequency);
            if (!outRaw) {
                subfile_out += "Preset: " + String(received.preset) + "\n";
                subfile_out +=
                    "Protocol: " + String(received.protocol == "" ? "RcSwitch" : received.protocol) + "\n";
                subfile_out += "Bit: " + String(received.Bit) + "\n";
                subfile_out += "Key: " + String(hexString) + "\n";
                if (received.fix != 0) { // KeeLoq: include the resolved rolling-code fields
                    char tmp[32] = {0};
                    subfile_out += "Manufacture: " + received.mf_name + "\n";
                    decimalToHexString(received.serial, tmp);
                    subfile_out += "Serial: " + String(tmp) + "\n";
                    subfile_out += "Button: " + String(received.btn) + "\n";
                    subfile_out += "Counter: " + String(received.cnt) + "\n";
                }
                subfile_out += "TE: " + String(received.te) + "\n";
            } else {
                subfile_out +=
                    "Preset: " + String(received.preset == "" ? String("Ook270Async") : received.preset) +
                    "\n";
                subfile_out += "Protocol: RAW\n";
                subfile_out += "RAW_Data: " + received.data;
            }
            rx.end();
            deinitRfModule();
            return subfile_out;
        }
        if (max_loops > 0) {
            // headless mode, quit if nothing received after max_loops
            vTaskDelay(1000 / portTICK_PERIOD_MS); // wait first, THEN check
            max_loops -= 1;
            if (max_loops == 0) {
                // Use sentinel -1: loop runs one more iteration to catch signals
                // that arrived during vTaskDelay before giving up
                max_loops = -1;
            }
        } else if (max_loops == -1) {
            // Final check already done in this iteration - truly timed out
            Serial.println("timeout");
            rx.end();
            deinitRfModule();
            return "";
        }
    }

    rx.end();
    deinitRfModule();

    return "";
}
