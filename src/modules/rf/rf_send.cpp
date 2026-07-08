#include "rf_send.h"
#include "core/led_control.h"
#include "core/type_convertion.h"
#include "protocols/rf_config.h"
#include "protocols/rf_encoder.h"
#include "protocols/rf_legacy_migrate.h"
#include "protocols/rf_presets.h"
#include "protocols/rf_registry.h"
#include "rf_utils.h"

#define CLOSE_MENU 3
#define MAIN_MENU 4

std::vector<int> bitList;
std::vector<int> bitRawList;
std::vector<uint64_t> keyList;
std::vector<String> rawDataList;

uint16_t num_steps_keeloq = 1;
uint8_t num_signal_repeat = 4;
String filepath = "";
FS *filesystem = NULL;

void sendCustomRF() {
    // interactive menu part only
    struct RfCodes selected_code;

    returnToMenu = true; // make sure menu is redrawn when quitting in any point

    options = {
        {"Recent",   [&]() { selected_code = selectRecentRfMenu(); }},
        {"LittleFS", [&]() { filesystem = &LittleFS; }              },
    };
    if (setupSdCard()) options.insert(options.begin(), {"SD Card", [&]() { filesystem = &SD; }});

    loopOptions(options);

    if (filesystem == NULL) {                                           // recent menu was selected
        if (selected_code.filepath != "") sendRfCommand(selected_code); // a code was selected
        return;
        // no need to proceed, go back
    }

    returnToMenu = false;
    filepath = "";

    while (!returnToMenu) {
        num_steps_keeloq = 1;
        num_signal_repeat = 4;
        delay(200);
        filepath = loopSD(*filesystem, true, "SUB", "/BruceRF");
        if (filepath == "" || check(EscPress)) return; //  cancelled

        RfCodes data{};

        if (!readSubFile(filesystem, filepath, data)) continue;

        if (data.protocol == "RcSwitch") {
            loopEmulate(data);
        } else {
            txSubFile(data);
            delay(200);
        }
    }
}

void set_option(int opt) {
    switch (opt) {
        case COUNTER_STEP: {
            options = {
                {"-50", [&] { num_steps_keeloq = -50; }},
                {"-10", [&] { num_steps_keeloq = -10; }},
                {"-1",  [&] { num_steps_keeloq = -1; } },
                {"1",   [&] { num_steps_keeloq = 1; }  },
                {"10",  [&] { num_steps_keeloq = 10; } },
                {"50",  [&] { num_steps_keeloq = 50; } },
            };

            loopOptions(options);

            break;
        }

        case REPEAT: {
            options = {};

            for (int i = 1; i <= 10; ++i) {
                options.emplace_back(String(i), [&, i] { num_signal_repeat = i; });
            }

            loopOptions(options);

            break;
        }

        case CLOSE_MENU: break;

        case MAIN_MENU: returnToMenu = true; break;
    }
}

void select_menu_option(bool keeloq) {
    options = {};

    if (keeloq) {
        options.emplace_back("Counter step", [] { set_option(COUNTER_STEP); });
    }

    options.emplace_back("Repeat", [] { set_option(REPEAT); });
    options.emplace_back("Close Menu", [] { set_option(CLOSE_MENU); });
    options.emplace_back("Main Menu", [] { set_option(MAIN_MENU); });

    loopOptions(options);
}

void keeloq_save(RfCodes data) {
    String subfile_out = "Filetype: Bruce SubGhz File\nVersion 1\n";
    subfile_out += "Frequency: " + String(data.frequency) + "\n";
    subfile_out += "Preset: " + String(data.preset == "" ? String("Ook650Async") : data.preset) + "\n";
    subfile_out += "Protocol: KeeLoq\n";
    subfile_out += "Bit: " + String(data.Bit) + "\n";

    char hexString[64] = {0};
    // The 64-bit Key is what gets replayed (sendRfCommand routes KeeLoq to the
    // dedicated encoder); the rolling-code fields below are informative.
    decimalToHexString(data.key, hexString);
    subfile_out += "Key: " + String(hexString) + "\n";

    if (data.seed != 0) {
        char seedHex[32] = {0};
        decimalToHexString(data.seed, seedHex);
        subfile_out += "Seed: " + String(seedHex) + "\n";
    }
    subfile_out += "Manufacture: " + String(data.mf_name) + "\n";

    decimalToHexString(data.serial, hexString);
    subfile_out += "Serial: " + String(hexString) + "\n";
    subfile_out += "Button: " + String(data.btn) + "\n";
    subfile_out += "Counter: " + String(data.cnt) + "\n";

    subfile_out += "TE: " + String(data.te) + "\n";

    File file = filesystem->open(filepath, "w", true);

    if (file) { file.println(subfile_out); }

    file.close();
}

void loopEmulate(RfCodes &data) {
    if (data.serial != 0) {
        data.fix = data.btn << 28 | data.serial;
        data.Bit = 64;
        data.keeloq_step(0);
    }

    display_info(data);

    while (1) {
        if (check(EscPress)) {
            keyList.clear();
            bitList.clear();

            return;
        }

        if (check(NextPress)) {
            select_menu_option(data.serial != 0);

            if (returnToMenu) {
                keyList.clear();
                bitList.clear();

                return;
            }

            display_info(data);
        }

        if (check(SelPress)) {
            blinkLed();

            if (data.serial == 0) {
                for (int i = 0; uint64_t key : keyList) {
                    data.Bit = bitList[i++];
                    data.key = key;
                    sendRfCommand(data);
                }
            } else {
                sendRfCommand(data);
                data.keeloq_step(num_steps_keeloq);
                keeloq_save(data);
                display_info(data);
            }
        }
    }
}

void display_info(RfCodes &data) {
    char hexString[64] = {0};

    drawMainBorderWithTitle("RF Emulate");

    padprintln("Frequency: " + String(data.frequency / 1000000.0) + "MHz");

    if (data.serial != 0) {
        padprintln("Protocol: KeeLoq");
        padprintln("Manufacture: " + data.mf_name);

        decimalToHexString(data.serial, hexString);
        padprintln("Serial: " + String(hexString));

        padprintln("Btn: " + String(data.btn));
        padprintln("Counter: " + String(data.cnt));
        padprintln("\n");

        decimalToHexString(data.key, hexString);
        padprintln("Payload: " + String(hexString));
    } else {
        padprintln("Protocol: " + String(data.protocol) + "(" + data.preset + ")");

        for (uint64_t key : keyList) {
            decimalToHexString(key, hexString);
            padprintln("Key: " + String(hexString));
        }
    }

    padprintln("");
    padprintln("");

    padprintln("Press [Mid] to send or [Next] for options");
}

bool readSubFile(FS *fs, const String &filepath, RfCodes &data) {
    struct RfCodes selected_code;
    File databaseFile;
    String line;
    String txt;

    if (!fs) return false;

    databaseFile = fs->open(filepath, FILE_READ);

    if (!databaseFile) {
        Serial.println("Failed to open database file.");
        displayError("Fail to open file", true);
        return false;
    }
    Serial.println("Opened sub file.");
    selected_code.filepath = filepath.substring(1 + filepath.lastIndexOf("/"));

    if (!databaseFile) Serial.println("Fail opening file");
    // Store the code(s) in the signal
    while (databaseFile.available()) {
        line = databaseFile.readStringUntil('\n');
        txt = line.substring(line.indexOf(":") + 1);
        if (txt.endsWith("\r")) txt.remove(txt.length() - 1);
        txt.trim();
        if (line.startsWith("Protocol:")) selected_code.protocol = txt;
        if (line.startsWith("Preset:")) selected_code.preset = txt;
        if (line.startsWith("Frequency:")) selected_code.frequency = txt.toInt();
        if (line.startsWith("TE:")) selected_code.te = txt.toInt();
        if (line.startsWith("Bit:")) bitList.push_back(txt.toInt()); // selected_code.Bit = txt.toInt();

        // KeeLoq rolling-code fields. Flipper writes "Manufacture" (no trailing
        // r); accept both spellings. "Seed" is the secure-learning seed.
        if (line.startsWith("Manufacturer:") || line.startsWith("Manufacture:")) selected_code.mf_name = txt;
        if (line.startsWith("Serial:")) selected_code.serial = hexStringToDecimal(txt.c_str());
        if (line.startsWith("Button:")) selected_code.btn = txt.toInt();
        if (line.startsWith("Counter:")) selected_code.cnt = txt.toInt();
        if (line.startsWith("Seed:")) selected_code.seed = (uint32_t)hexStringToU64(txt.c_str());

        if (line.startsWith("Bit_RAW:"))
            bitRawList.push_back(txt.toInt()); // selected_code.BitRAW = txt.toInt();
        // Keys can exceed 32 bits (Holtek 40, Mastercode 36, PhoenixV2 52,
        // KeeLoq 64), so parse the full 64-bit value, not a truncated uint32.
        if (line.startsWith("Key:")) keyList.push_back(hexStringToU64(txt.c_str()));
        if (line.startsWith("RAW_Data:") || line.startsWith("Data_RAW:"))
            rawDataList.push_back(txt); // selected_code.data = txt;

        if (check(EscPress)) break;
    }

    databaseFile.close();

    data = selected_code;

#if RF_SUB_LEGACY_MIGRATION
    // One-shot migration of old-format `.sub` files (Protocol: RcSwitch +
    // numeric Preset) into the registry-based format: backs the original up to
    // `<file>.bak` and rewrites the `.sub` once. `data` is migrated in memory
    // regardless, so replay always uses the resolved protocol name.
    if (rf_sub_is_legacy(data)) rf_sub_migrate(fs, filepath, data);
#endif

    return true;
}

bool txSubFile(RfCodes &selected_code, bool hideDefaultUI) {
    int sent = 0;

    int total = keyList.size() + rawDataList.size();
    Serial.printf("Total signals found: %d\n", total);
    // If the signal is complete, send all of the code(s) that were found in it.
    // TODO: try to minimize the overhead between codes.
    if (selected_code.protocol != "" && selected_code.preset != "" && selected_code.frequency > 0) {
        // A `.sub` carries a code as a Key (paired with its Bit length) or as a
        // RAW/BinRAW data stream (paired with Bit_RAW). Each code is sent ONCE:
        // earlier this looped Bit and Key separately, transmitting every code
        // twice — the first time with key=0 (a bogus frame). Pair them instead.
        for (size_t i = 0; i < keyList.size(); i++) {
            selected_code.key = keyList[i];
            if (i < bitList.size()) selected_code.Bit = bitList[i];
            sendRfCommand(selected_code, hideDefaultUI);
            sent++;
            if (!hideDefaultUI) {
                if (check(EscPress)) break;
                displayTextLine("Sent " + String(sent) + "/" + String(total));
            }
        }

        // RAW_Data / Data_RAW: one (long) signal per data line, BinRAW pairs it
        // with Bit_RAW. RAW protocol ignores Bit.
        for (size_t i = 0; i < rawDataList.size(); i++) {
            selected_code.data = rawDataList[i];
            if (i < bitRawList.size()) selected_code.Bit = bitRawList[i];
            sendRfCommand(selected_code, hideDefaultUI);
            sent++;
            if (check(EscPress)) break;
        }
        addToRecentCodes(selected_code);
    }

    Serial.printf("\nSent %d of %d signals\n", sent, total);
    if (!hideDefaultUI) { displayTextLine("Sent " + String(sent) + "/" + String(total), true); }

    // Reset vectors
    bitList.clear();
    bitRawList.clear();
    keyList.clear();
    rawDataList.clear();

    delay(1000);
    deinitRfModule();
    return true;
}

void sendRfCommand(struct RfCodes rfcode, bool hideDefaultUI) {
    uint32_t frequency = rfcode.frequency;
    String protocol = rfcode.protocol;
    String preset = rfcode.preset;
    String data = rfcode.data;
    uint64_t key = rfcode.key;
    byte modulation = 2; // possible values for CC1101: 0 = 2-FSK, 1 =GFSK, 2=ASK, 3 = 4-FSK, 4 = MSK
    float deviation = 1.58;
    float rxBW = 270.83; // Receive bandwidth
    float dataRate = 10; // Data Rate
                         /*
                             Serial.println("sendRawRfCommand");
                             Serial.println(data);
                             Serial.println(frequency);
                             Serial.println(preset);
                             Serial.println(protocol);
                           */

    // Radio preset name (configures modulation, bandwidth, filters, etc.).
    /*  supported flipper presets:
        FuriHalSubGhzPresetIDLE, // < default configuration
        FuriHalSubGhzPresetOok270Async, ///< OOK, bandwidth 270kHz, asynchronous
        FuriHalSubGhzPresetOok650Async, ///< OOK, bandwidth 650kHz, asynchronous
        FuriHalSubGhzPreset2FSKDev238Async, //< FM, deviation 2.380371 kHz, asynchronous
        FuriHalSubGhzPreset2FSKDev476Async, //< FM, deviation 47.60742 kHz, asynchronous
        FuriHalSubGhzPresetMSK99_97KbAsync, //< MSK, deviation 47.60742 kHz, 99.97Kb/s, asynchronous
        FuriHalSubGhzPresetGFSK9_99KbAsync, //< GFSK, deviation 19.042969 kHz, 9.996Kb/s, asynchronous
        FuriHalSubGhzPresetCustom, //Custom Preset
    */
    // Radio preset parameters come from the central registry (protocols/).
    // A preset field at 0 means "keep the module default" (set above).
    int legacy_protocol_no = 1;
    const RfPreset *rp = rf_find_preset(preset);
    if (rp) {
        modulation = rp->modulation;
        if (rp->deviation) deviation = rp->deviation;
        if (rp->rxBW) rxBW = rp->rxBW;
        if (rp->dataRate) dataRate = rp->dataRate;
        legacy_protocol_no = rp->legacyProto;
    } else {
        bool found = false;
        for (int p = 0; p < 30; p++) {
            if (preset == String(p)) {
                legacy_protocol_no = preset.toInt();
                found = true;
            }
        }
        if (!found) {
            Serial.print("unsupported preset: ");
            Serial.println(preset);
            return;
        }
    }

    // init transmitter
    if (!initRfModule("", frequency / 1000000.0)) return;
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) { // CC1101 in use
        // derived from
        // https://github.com/LSatan/SmartRC-CC1101-Driver-Lib/blob/master/examples/Rc-Switch%20examples%20cc1101/SendDemo_cc1101/SendDemo_cc1101.ino
        ELECHOUSE_cc1101.setModulation(modulation);
        if (deviation) ELECHOUSE_cc1101.setDeviation(deviation);
        if (rxBW)
            ELECHOUSE_cc1101.setRxBW(
                rxBW
            ); // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
        if (dataRate) ELECHOUSE_cc1101.setDRate(dataRate);
        pinMode(bruceConfigPins.CC1101_bus.io0, OUTPUT);
        ELECHOUSE_cc1101.setPA(
            12
        ); // set TxPower. The following settings are possible depending on the frequency band.  (-30  -20 -15
        // -10  -6    0    5    7    10   11   12)   Default is max!
        ioExpander.turnPinOnOff(IO_EXP_CC_RX, LOW);
        ioExpander.turnPinOnOff(IO_EXP_CC_TX, HIGH);
        ELECHOUSE_cc1101.SetTx();
    } else {
        // other single-pinned modules in use
        if (modulation != 2) {
            Serial.print("unsupported modulation: ");
            Serial.println(modulation);
            return;
        }
        initRfModule("tx", frequency / 1000000.0);
    }

    if (protocol == "RAW") {
        // count the number of elements of RAW_Data
        int buff_size = 0;
        int index = 0;
        while (index >= 0) {
            index = data.indexOf(' ', index + 1);
            buff_size++;
        }
        // alloc buffer for transmittimings
        int *transmittimings =
            (int *)calloc(sizeof(int), buff_size + 1); // should be smaller the data.length()
        size_t transmittimings_idx = 0;

        // split data into words, convert to int, and store them in transmittimings
        int startIndex = 0;
        index = 0;
        for (transmittimings_idx = 0; transmittimings_idx < buff_size; transmittimings_idx++) {
            index = data.indexOf(' ', startIndex);
            if (index == -1) {
                transmittimings[transmittimings_idx] = data.substring(startIndex).toInt();
            } else {
                transmittimings[transmittimings_idx] = data.substring(startIndex, index).toInt();
            }
            startIndex = index + 1;
        }
        transmittimings[transmittimings_idx] = 0; // termination

        // send rf command
        if (!hideDefaultUI) { displayTextLine("Sending.."); }
        rfTransmitRawTimings(transmittimings);
        free(transmittimings);
    } else if (protocol == "BinRAW") {
        // transform from "00 01 02 ... FF" into "00000000 00000001 00000010 .... 11111111"
        rfcode.data = hexStrToBinStr(rfcode.data);
        // Serial.println(rfcode.data);
        rfcode.data.trim();
        rfTransmitRawBits(rfcode);
    }

    else if (protocol == "KeeLoq") {
        // KeeLoq has dedicated framing (see rf_keeloq_durations). `rfcode.key` is
        // the 64-bit rolling code already assembled by keeloq_step.
        if (!hideDefaultUI) { displayTextLine("Sending.."); }
        rf_tx_keeloq(rfcode.key, num_signal_repeat);
    }

    else {
        // Dispatch by protocol NAME (the decoder + migration now write
        // `Protocol: <name>`). The named definition comes straight from the
        // central registry.
        const RfProtocolDef *def = rf_find_protocol(protocol);

#if RF_SUB_LEGACY_MIGRATION
        // Safety net for non-migrated legacy sources (PSRamFS / read-only FS):
        // a `.sub` still carrying `Protocol: RcSwitch` + numeric `Preset`.
        if (def == nullptr && protocol == "RcSwitch") def = rf_find_legacy(legacy_protocol_no);
#endif

        if (def != nullptr) {
            rf_tx_protocol(rfcode.key, rfcode.Bit, rfcode.te, def, num_signal_repeat);
        } else {
            Serial.print("unsupported protocol: ");
            Serial.println(protocol);
            Serial.println("Falling back to generic RcSwitch_11");
            rfTransmitCode(rfcode.key, rfcode.Bit, 270, 11, 10);
        }
    }

    // digitalWrite(bruceConfigPins.rfTx, LED_OFF);
    deinitRfModule();
}

// Transmit `data` (`bits` long) using a classic OOK protocol NUMBER. Thin
// wrapper over the RMT encoder; the number is resolved to a registry definition
// (keeps the external CLI/JSON numeric contract). Signature unchanged.
void rfTransmitCode(uint64_t data, unsigned int bits, int pulse, int protocol, int repeat) {
    const RfProtocolDef *def = rf_protocol_for_number(protocol);
    rf_tx_protocol(data, bits, pulse, def, repeat);
    deinitRfModule();
}

// Transmit a bit string (each bit held for `te` µs) via RMT. Signature unchanged.
void rfTransmitRawBits(RfCodes data) {
    if (data.data == "") return;
    rf_tx_raw_bits(data.data, data.te);
}

// Transmit a 0-terminated RAW timings array (signed µs) via RMT. Signature
// unchanged (1 repetition, as before).
void rfTransmitRawTimings(int *ptrtransmittimings) {
    if (!ptrtransmittimings) return;
    rf_tx_raw_timings(ptrtransmittimings);
}
