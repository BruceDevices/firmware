#include "rf_send.h"
#include "core/led_control.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/type_convertion.h"
#include "rf_utils.h"
#include "rolling_code_db.h"
#include "rolling_code_proto.h"
#include <RCSwitch.h>

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

        // Reverse rolling-code check: warn if a rolling signal lands in the
        // static Send Saved path. Still allows the user to send anyway.
        if (is_rolling_code_file(filesystem, filepath)) {
            int r = displayConfirm(
                "This is a rolling\ncode file. It may\nnot send here.", "Send anyway", "Cancel"
            );
            if (r != 0) continue; // Cancel or ESC
        }

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

    options.emplace_back("Repeat",     [] { set_option(REPEAT); });
    options.emplace_back("Close Menu", [] { set_option(CLOSE_MENU); });
    options.emplace_back("Main Menu",  [] { set_option(MAIN_MENU); });

    loopOptions(options);
}

void keeloq_save(RfCodes data) {
    String subfile_out = "Filetype: Bruce SubGhz File\nVersion 1\n";
    subfile_out += "Frequency: " + String(data.frequency) + "\n";
    subfile_out += "Preset: " + String(data.preset) + "\n";
    subfile_out += "Protocol: RcSwitch\n";
    subfile_out += "Bit: " + String(data.Bit) + "\n";

    subfile_out += "Manufacturer: " + String(data.mf_name) + "\n";
    char hexString[64] = {0};

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
        padprintln("Manufacturer: " + data.mf_name);

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

bool readSubFile(FS *fs, String filepath, RfCodes &data) {
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

        if (line.startsWith("Manufacturer:")) selected_code.mf_name = txt;
        if (line.startsWith("Serial:")) selected_code.serial = hexStringToDecimal(txt.c_str());
        if (line.startsWith("Button:")) selected_code.btn = txt.toInt();
        if (line.startsWith("Counter:")) selected_code.cnt = txt.toInt();

        // Rolling code extensions.
        if (line.startsWith("Seed:")) selected_code.seed = hexStringToDecimal(txt.c_str());
        if (line.startsWith("CRC:")) selected_code.crc_field = hexStringToDecimal(txt.c_str());
        if (line.startsWith("SomfyKey:")) selected_code.somfy_key = hexStringToDecimal(txt.c_str());
        if (line.startsWith("Protocol:") && rolling_protocol_by_name(txt.c_str()))
            selected_code.rolling_protocol = txt;

        if (line.startsWith("Bit_RAW:"))
            bitRawList.push_back(txt.toInt()); // selected_code.BitRAW = txt.toInt();
        if (line.startsWith("Key:"))
            keyList.push_back(
                hexStringToDecimal(txt.c_str())
            ); // selected_code.key = hexStringToDecimal(txt.c_str());
        if (line.startsWith("RAW_Data:") || line.startsWith("Data_RAW:"))
            rawDataList.push_back(txt); // selected_code.data = txt;

        if (check(EscPress)) break;
    }

    databaseFile.close();

    data = selected_code;

    return true;
}

bool txSubFile(RfCodes &selected_code, bool hideDefaultUI) {
    int sent = 0;

    int total = bitList.size() + bitRawList.size() + keyList.size() + rawDataList.size() > 0 ? 1 : 0;
    Serial.printf("Total signals found: %d\n", total);
    // If the signal is complete, send all of the code(s) that were found in it.
    // TODO: try to minimize the overhead between codes.
    if (selected_code.protocol != "" && selected_code.preset != "" && selected_code.frequency > 0) {
        for (int bit : bitList) {
            selected_code.Bit = bit;
            sendRfCommand(selected_code, hideDefaultUI);
            sent++;
            if (!hideDefaultUI) {
                if (check(EscPress)) break;
                displayTextLine("Sent " + String(sent) + "/" + String(total));
            }
        }
        for (int bitRaw : bitRawList) {
            selected_code.Bit = bitRaw;
            sendRfCommand(selected_code, hideDefaultUI);
            sent++;
            if (!hideDefaultUI) {
                if (check(EscPress)) break;
                displayTextLine("Sent " + String(sent) + "/" + String(total));
            }
        }
        for (uint64_t key : keyList) {
            selected_code.key = key;
            sendRfCommand(selected_code, hideDefaultUI);
            sent++;
            if (!hideDefaultUI) {
                if (check(EscPress)) break;
                displayTextLine("Sent " + String(sent) + "/" + String(total));
            }
        }

        // RAS_Data is considered one long signal, doesn't matter the number of lines it has
        if (rawDataList.size() > 0) sent++;
        for (String rawData : rawDataList) {
            selected_code.data = rawData;
            sendRfCommand(selected_code, hideDefaultUI);
            // sent++;
            if (check(EscPress)) break;
            // displayTextLine("Sent " + String(sent) + "/" + String(total));
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
    // struct Protocol rcswitch_protocol;
    int rcswitch_protocol_no = 1;
    if (preset == "FuriHalSubGhzPresetOok270Async") {
        rcswitch_protocol_no = 1;
        //  pulseLength , syncFactor , zero , one, invertedSignal
        // rcswitch_protocol = { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false };
        modulation = 2;
        rxBW = 270;
    } else if (preset == "FuriHalSubGhzPresetOok650Async") {
        rcswitch_protocol_no = 2;
        // rcswitch_protocol = { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false };
        modulation = 2;
        rxBW = 650;
    } else if (preset == "FuriHalSubGhzPreset2FSKDev238Async") {
        modulation = 0;
        deviation = 2.380371;
        rxBW = 238;
    } else if (preset == "FuriHalSubGhzPreset2FSKDev476Async") {
        modulation = 0;
        deviation = 47.60742;
        rxBW = 476;
    } else if (preset == "FuriHalSubGhzPresetMSK99_97KbAsync") {
        modulation = 4;
        deviation = 47.60742;
        dataRate = 99.97;
    } else if (preset == "FuriHalSubGhzPresetGFSK9_99KbAsync") {
        modulation = 1;
        deviation = 19.042969;
        dataRate = 9.996;
    } else {
        bool found = false;
        for (int p = 0; p < 30; p++) {
            if (preset == String(p)) {
                rcswitch_protocol_no = preset.toInt();
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
        RCSwitch_RAW_send(transmittimings);
        free(transmittimings);
    } else if (protocol == "BinRAW") {
        // transform from "00 01 02 ... FF" into "00000000 00000001 00000010 .... 11111111"
        rfcode.data = hexStrToBinStr(rfcode.data);
        // Serial.println(rfcode.data);
        rfcode.data.trim();
        RCSwitch_RAW_Bit_send(rfcode);
    }

    else if (protocol == "RcSwitch") {
        data.replace(" ", ""); // remove spaces
        // uint64_t data_val = strtoul(data.c_str(), nullptr, 16);
        uint64_t data_val = rfcode.key;
        int bits = rfcode.Bit;
        int pulse = rfcode.te; // not sure about this...
        int repeat = num_signal_repeat;
        /*
        Serial.print("RcSwitch: ");
        Serial.println(data_val,16);
        Serial.println(bits);
        Serial.println(pulse);
        Serial.println(rcswitch_protocol_no);
        */
        // if (!hideDefaultUI) { displayTextLine("Sending.."); }
        RCSwitch_send(data_val, bits, pulse, rcswitch_protocol_no, repeat);
    } else if (protocol.startsWith("Princeton")) {
        RCSwitch_send(rfcode.key, rfcode.Bit, 350, 1, 10);
    } else {
        Serial.print("unsupported protocol: ");
        Serial.println(protocol);
        Serial.println("Sending RcSwitch 11 protocol");
        // if(protocol.startsWith("CAME") || protocol.startsWith("HOLTEC" || NICE)) {
        RCSwitch_send(rfcode.key, rfcode.Bit, 270, 11, 10);
        //}

        return;
    }

    // digitalWrite(bruceConfigPins.rfTx, LED_OFF);
    deinitRfModule();
}

void RCSwitch_send(uint64_t data, unsigned int bits, int pulse, int protocol, int repeat) {
    // derived from
    // https://github.com/LSatan/SmartRC-CC1101-Driver-Lib/blob/master/examples/Rc-Switch%20examples%20cc1101/SendDemo_cc1101/SendDemo_cc1101.ino

    RCSwitch mySwitch = RCSwitch();

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        mySwitch.enableTransmit(bruceConfigPins.CC1101_bus.io0);
    } else {
        mySwitch.enableTransmit(bruceConfigPins.rfTx);
    }

    mySwitch.setProtocol(protocol); // override
    if (pulse) { mySwitch.setPulseLength(pulse); }
    mySwitch.setRepeatTransmit(repeat);
    mySwitch.send(data, bits);

    /*
    Serial.println(data,HEX);
    Serial.println(bits);
    Serial.println(pulse);
    Serial.println(protocol);
    Serial.println(repeat);
    */

    mySwitch.disableTransmit();

    deinitRfModule();
}

// ported from https://github.com/sui77/rc-switch/blob/3a536a172ab752f3c7a58d831c5075ca24fd920b/RCSwitch.cpp
void RCSwitch_RAW_Bit_send(RfCodes data) {
    int nTransmitterPin = bruceConfigPins.rfTx;
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) { nTransmitterPin = bruceConfigPins.CC1101_bus.io0; }

    if (data.data == "") return;
    bool currentlogiclevel = false;
    int nRepeatTransmit = 1;
    for (int nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++) {
        int currentBit = data.data.length();
        while (currentBit >= 0) { // Starts from the end of the string until the max number of bits to send
            char c = data.data[currentBit];
            if (c == '1') {
                currentlogiclevel = true;
            } else if (c == '0') {
                currentlogiclevel = false;
            } else {
                Serial.println("Invalid data");
                currentBit--;
                continue;
                // return;
            }

            digitalWrite(nTransmitterPin, currentlogiclevel ? HIGH : LOW);
            delayMicroseconds(data.te);

            // Serial.print(currentBit);
            // Serial.print("=");
            // Serial.println(currentlogiclevel);

            currentBit--;
        }
        digitalWrite(nTransmitterPin, LOW);
    }
}

void RCSwitch_RAW_send(int *ptrtransmittimings) {
    int nTransmitterPin = bruceConfigPins.rfTx;
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) { nTransmitterPin = bruceConfigPins.CC1101_bus.io0; }

    if (!ptrtransmittimings) return;

    bool currentlogiclevel = true;
    int nRepeatTransmit = 1; // repeats RAW signal twice!
    // HighLow pulses ;

    for (int nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++) {
        unsigned int currenttiming = 0;
        while (ptrtransmittimings[currenttiming]) { // && currenttiming < RCSWITCH_MAX_CHANGES
            if (ptrtransmittimings[currenttiming] >= 0) {
                currentlogiclevel = true;
            } else {
                // negative value
                currentlogiclevel = false;
                ptrtransmittimings[currenttiming] = (-1) * ptrtransmittimings[currenttiming]; // invert sign
            }

            digitalWrite(nTransmitterPin, currentlogiclevel ? HIGH : LOW);
            delayMicroseconds(ptrtransmittimings[currenttiming]);

            /*
            Serial.print(ptrtransmittimings[currenttiming]);
            Serial.print("=");
            Serial.println(currentlogiclevel);
            */

            currenttiming++;
        }
        digitalWrite(nTransmitterPin, LOW);
    } // end for
}

// ===========================================================================
// Rolling Code RF — TX engine, persistence, file-type check, and screens.
// ===========================================================================

void rolling_code_tx(const std::vector<int> &timings, float freq_mhz, uint8_t bw_preset, int repeat) {
    if (timings.empty()) return;

    if (!initRfModule("", freq_mhz)) return;

    int nTransmitterPin = bruceConfigPins.rfTx;
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        ELECHOUSE_cc1101.setModulation(2); // OOK/ASK
        ELECHOUSE_cc1101.setRxBW(bw_preset == BW_OOK_650 ? 650 : 270);
        pinMode(bruceConfigPins.CC1101_bus.io0, OUTPUT);
        ELECHOUSE_cc1101.setPA(12);
        ioExpander.turnPinOnOff(IO_EXP_CC_RX, LOW);
        ioExpander.turnPinOnOff(IO_EXP_CC_TX, HIGH);
        ELECHOUSE_cc1101.SetTx();
        nTransmitterPin = bruceConfigPins.CC1101_bus.io0;
    } else {
        if (!initRfModule("tx", freq_mhz)) return;
    }

    for (int r = 0; r < repeat; r++) {
        for (int dur : timings) {
            digitalWrite(nTransmitterPin, dur >= 0 ? HIGH : LOW);
            delayMicroseconds(dur >= 0 ? dur : -dur);
        }
        digitalWrite(nTransmitterPin, LOW);
        delay(20);
    }
    deinitRfModule();
}

void rolling_code_save(RfCodes &data, const String &path, FS *fs) {
    if (!fs) return;
    const RollingProtocol *proto = rolling_protocol_by_name(data.rolling_protocol.c_str());
    char hexString[64] = {0};

    String out = "Filetype: Bruce SubGhz File\nVersion 1\n";
    out += "Frequency: " + String(data.frequency) + "\n";
    out += "Preset: " +
           String(data.preset.length() ? data.preset : String("FuriHalSubGhzPresetOok270Async")) + "\n";
    out += "Protocol: " + data.rolling_protocol + "\n";
    out += "Bit: " + String(data.Bit) + "\n";
    out += "Manufacturer: " + data.mf_name + "\n";

    decimalToHexString(data.serial, hexString);
    out += "Serial: " + String(hexString) + "\n";
    out += "Button: " + String(data.btn) + "\n";
    out += "Counter: " + String(data.cnt) + "\n";

    if (proto && proto->has_seed) {
        decimalToHexString(data.seed, hexString);
        out += "Seed: " + String(hexString) + "\n";
    }

    decimalToHexString(data.key, hexString);
    out += "Key: " + String(hexString) + "\n";
    out += "TE: " + String(data.te) + "\n";

    if (proto && proto->has_crc) out += "CRC: " + String(data.crc_field) + "\n";
    if (proto && (proto->family == RF_FAMILY_SOMFY_TELIS || proto->family == RF_FAMILY_SOMFY_KEYTIS)) {
        decimalToHexString(data.somfy_key, hexString);
        out += "SomfyKey: " + String(hexString) + "\n";
    }

    File file = fs->open(path, "w", true);
    if (file) file.println(out);
    file.close();
}

bool is_rolling_code_file(FS *fs, const String &path) {
    if (!fs) return false;
    File f = fs->open(path, FILE_READ);
    if (!f) return false;

    String chunk;
    int n = 0;
    while (f.available() && n < 512) {
        chunk += (char)f.read();
        n++;
    }
    f.close();

    if (chunk.indexOf("Counter:") >= 0) return true;

    int p = chunk.indexOf("Protocol:");
    if (p >= 0) {
        int e = chunk.indexOf('\n', p);
        String val = chunk.substring(p + 9, e < 0 ? chunk.length() : e);
        if (val.endsWith("\r")) val.remove(val.length() - 1);
        val.trim();
        if (rolling_protocol_by_name(val.c_str())) return true;
    }
    return false;
}

// Show the rolling-code mismatch warning. Returns true to proceed.
static bool rolling_warn(const String &message) {
    // displayConfirm shows the reason and the buttons together; ESC = cancel.
    return displayConfirm(message.c_str(), "Send anyway", "Cancel") == 0;
}

static bool rolling_key_warning(const RollingProtocol *proto, const String &mf);

static void rolling_clear_globals() {
    bitList.clear();
    bitRawList.clear();
    keyList.clear();
    rawDataList.clear();
}

// Choose SD or LittleFS, then pick a .sub from /BruceRF. Runs the rolling-code
// file-type warning (expectRolling controls warning direction). Returns true
// when a file is selected and the user opted to proceed.
static bool pickRollingFile(FS *&fs, String &path, bool expectRolling) {
    fs = NULL;
    options = {{"LittleFS", [&]() { fs = &LittleFS; }}};
    if (setupSdCard()) options.insert(options.begin(), {"SD Card", [&]() { fs = &SD; }});
    loopOptions(options);
    if (fs == NULL) return false;

    delay(150);
    path = loopSD(*fs, true, "SUB", "/BruceRF");
    if (path == "" || check(EscPress)) return false;

    bool isRolling = is_rolling_code_file(fs, path);
    if (isRolling != expectRolling) {
        String msg = expectRolling
                         ? "This file has no\ncounter. It may not\nsend correctly."
                         : "This is a rolling\ncode file. It may\nnot send here.";
        if (!rolling_warn(msg)) return false;
    }
    return true;
}

static void rollingDisplayInfo(RfCodes &data, const RollingProtocol *proto) {
    char hexString[64] = {0};
    drawMainBorderWithTitle("RF Emulate");
    padprintln(data.rolling_protocol + "   " + String(data.frequency / 1000000.0) + "MHz");
    padprintln("");
    decimalToHexString(data.serial, hexString);
    padprintln("Serial:  " + String(hexString));
    padprintln("Button:  " + String(data.btn));
    padprintln("Counter: " + String(data.cnt));
    if (proto && proto->has_seed) {
        decimalToHexString(data.seed, hexString);
        padprintln("Seed:    " + String(hexString));
    }
    decimalToHexString(data.key, hexString);
    padprintln("Payload: " + String(hexString));
    padprintln("");
    padprintln("[Mid]=Send  [Next]=Options");
}

static uint16_t rolling_step_picker() {
    uint16_t step = 1;
    options = {
        {"1",      [&]() { step = 1; }   },
        {"10",     [&]() { step = 10; }  },
        {"100",    [&]() { step = 100; } },
        {"1000",   [&]() { step = 1000; }},
        {"Custom", [&]() {
             String s = num_keyboard("", 10, "Step value:");
             step = (uint16_t)s.toInt();
         }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Step by");
    return step;
}

static void rolling_options_menu(int &step, int &repeat) {
    options = {
        {"Counter Step", [&]() {
             options = {
                 {"-50", [&]() { step = -50; }},
                 {"-10", [&]() { step = -10; }},
                 {"-1",  [&]() { step = -1; } },
                 {"1",   [&]() { step = 1; }  },
                 {"10",  [&]() { step = 10; } },
                 {"50",  [&]() { step = 50; } },
             };
             loopOptions(options);
         }},
        {"Repeat", [&]() {
             options = {};
             for (int i = 1; i <= 10; ++i) options.emplace_back(String(i), [&, i]() { repeat = i; });
             loopOptions(options);
         }},
        {"Main Menu", [&]() { returnToMenu = true; }},
        {"Back",      []() {}},
    };
    loopOptions(options);
}

void loopEmulateRolling(RfCodes &data, const String &path, FS *fs) {
    const RollingProtocol *proto = rolling_protocol_by_name(data.rolling_protocol.c_str());
    if (!proto) {
        displayError("Unknown rolling protocol", true);
        return;
    }
    // Backend key-presence check on send too.
    if (!rolling_key_warning(proto, data.mf_name)) return;
    int step = 1, repeat = 3;
    returnToMenu = false;

    // Pre-compute payload for display.
    rolling_encode(data, proto);
    rollingDisplayInfo(data, proto);

    while (true) {
        if (check(EscPress)) {
            rolling_clear_globals();
            return;
        }
        if (check(NextPress)) {
            rolling_options_menu(step, repeat);
            if (returnToMenu) {
                rolling_clear_globals();
                return;
            }
            rollingDisplayInfo(data, proto);
        }
        if (check(SelPress)) {
            blinkLed();
            std::vector<int> timings = rolling_encode(data, proto);
            rolling_code_tx(timings, proto->frequency_hz / 1000000.0, proto->bw_preset, repeat);
            // Auto-increment counter (skip the no-counter Security+ 1.0).
            if (proto->family != RF_FAMILY_SECURITY_PLUS_1) data.cnt += step;
            rolling_code_save(data, path, fs);
            rollingDisplayInfo(data, proto);
        }
    }
}

static void rollingKeyHelpText() {
    drawMainBorderWithTitle("Key Setup");
    tft.setTextSize(FP);
    padprintln("");
    padprintln("Needs key: KeeLoq, FAAC SLH,");
    padprintln("Jarolift, Beninca, Nice, Alutech.");
    padprintln("No key: Somfy, Security+.");
    padprintln("");
    padprintln("KeeLoq keys: SD root /mfcodes");
    padprintln("  name;key_hex;type");
    padprintln("  e.g. DoorHan;A1B2C3D4;1");
    padprintln("  type 1=simple 2=learn");
    padprintln("Name matches list entry.");
    padprintln("");
    padprintln("Nice/Alutech: /nice_flors.bin");
    padprintln("/alutech.bin (32 bytes each).");
    padprintln("Beninca ARC: /beninca.bin (16B).");
    padprintln("Place on the active key source.");
    padprintln("");
    padprintln("Press a key to go back.");
    while (!check(EscPress) && !check(SelPress) && !check(NextPress)) delay(20);
}

static void rollingReloadKeys() {
    FS *keyFs = (bruceConfig.rfKeyFs == 1) ? (FS *)&LittleFS : nullptr;
    if (bruceConfig.rfKeyFs == 0 && setupSdCard()) keyFs = (FS *)&SD;
    rolling_code_db_load_sd(keyFs);
    displayTextLine("Keys reloaded", true);
}

void rollingKeyHelp(std::function<void()> backFn) {
    bool back = false;
    while (!back) {
        bool sdAvail = setupSdCard();
        String srcLabel = String("Key Source: ") +
                          (bruceConfig.rfKeyFs == 1 ? "Flash" : "SD Card");
        options = {
            {"Instructions",  rollingKeyHelpText},
            {srcLabel,        [sdAvail]() {
                int next = (bruceConfig.rfKeyFs == 1) ? 0 : 1;
                if (next == 0 && !sdAvail) { displayTextLine("No SD card", true); return; }
                bruceConfig.rfKeyFs = next;
                bruceConfig.saveFile();
                rollingReloadKeys();
            }},
            {"Reload Keys",   rollingReloadKeys},
            {"Back",          [&back, &backFn]() { back = true; if (backFn) backFn(); }},
        };
        if (loopOptions(options, MENU_TYPE_SUBMENU, "Key Setup") < 0) break;
    }
}

// If the protocol needs key material that isn't loaded, warn. Returns true to
// proceed (key present, or user chose to continue), false to back out.
static bool rolling_key_warning(const RollingProtocol *proto, const String &mf) {
    if (rolling_key_present(proto, mf.length() ? mf.c_str() : nullptr)) return true;

    while (true) {
        int r = displayConfirm(
            "No key loaded.\nA receiver won't\naccept this until\na key is added.", "Continue", "Key help"
        );
        if (r == 0) return true;   // Continue anyway
        if (r == 1) { rollingKeyHelp(); continue; } // show help, then re-ask
        return false;              // ESC = back out
    }
}

// ---- Create Signal --------------------------------------------------------

static const RollingProtocol *rolling_pick_protocol() {
    // One flat, alphabetical list of equal protocols (KeeLoq is just one row).
    const RollingProtocol *chosen = nullptr;
    options = {};
    for (size_t i = 0; i < rolling_protocols_count; i++) {
        const RollingProtocol *p = &rolling_protocols[i];
        options.emplace_back(String(p->display_name), [&chosen, p]() { chosen = p; });
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "Create Signal");
    return chosen;
}

// Pick a KeeLoq manufacturer from the keystore (compiled-in + any SD merge).
// Returns the chosen manufacturer name, or "" if cancelled.
static String rolling_pick_keeloq_manufacturer(const String &current) {
    String chosen = "";
    options = {};
    for (size_t i = 0; i < rolling_mf_keys_count; i++) {
        if (!rolling_mf_is_keeloq_manufacturer(rolling_mf_keys[i].type)) continue;
        String name = rolling_mf_keys[i].name;
        options.emplace_back(name, [&chosen, name]() { chosen = name; });
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "KeeLoq Manufacturer");
    return chosen.length() ? chosen : current;
}

// Uppercase hex, fixed width to the field byte count (so values don't jump).
static String rolling_fmt_hex(uint32_t value, int bytes) {
    if (bytes < 1) bytes = 1;
    char buf[16];
    snprintf(buf, sizeof(buf), "%0*lX", bytes * 2, (unsigned long)value);
    return String(buf);
}

static uint32_t rolling_edit_hex(const String &label, uint32_t current, int bytes) {
    String s = hex_keyboard(rolling_fmt_hex(current, bytes), bytes * 2, "Edit " + label + ":");
    return hexStringToDecimal(s.c_str());
}

// Sensible starting values for a freshly created signal: a unique random serial
// (a real new remote has one), button 1, counter 1, random seed where needed,
// and a valid Somfy key byte. KeeLoq starts on the first available manufacturer.
static void rolling_set_defaults(RfCodes &data, const RollingProtocol *proto) {
    if (proto->serial_bytes) {
        uint32_t mask = (proto->serial_bytes >= 4) ? 0xFFFFFFFF
                                                   : ((1UL << (proto->serial_bytes * 8)) - 1);
        data.serial = esp_random() & mask;
    }
    if (proto->button_bytes) data.btn = 1;
    if (proto->counter_bytes) data.cnt = 1;
    if (proto->has_seed && proto->seed_bytes) {
        uint32_t mask = (proto->seed_bytes >= 4) ? 0xFFFFFFFF
                                                 : ((1UL << (proto->seed_bytes * 8)) - 1);
        data.seed = esp_random() & mask;
    }
    if (proto->family == RF_FAMILY_SOMFY_TELIS || proto->family == RF_FAMILY_SOMFY_KEYTIS) {
        data.somfy_key = 0xA0 | (esp_random() & 0x0F);
        data.cnt = 1; // Somfy counter must start at 1
    }
    if (proto->family == RF_FAMILY_KEELOQ && data.mf_name.length() == 0) {
        for (size_t i = 0; i < rolling_mf_keys_count; i++) {
            if (rolling_mf_is_keeloq_manufacturer(rolling_mf_keys[i].type)) {
                data.mf_name = rolling_mf_keys[i].name;
                break;
            }
        }
    }
}

// Parameter editor modelled on the scan post-capture options menu: a regular
// popup list (the selected row auto-scrolls if long, so wide hex values fit).
// Sel edits a field; there is no Next button here.
static bool rolling_param_form(RfCodes &data, const RollingProtocol *proto) {
    rolling_set_defaults(data, proto);

    bool generate = false, cancel = false;
    bool isKeeloq = (proto->family == RF_FAMILY_KEELOQ);
    int idx = 0;
    while (!generate && !cancel) {
        options = {};

        if (isKeeloq)
            options.emplace_back(String("Manufacturer: ") + data.mf_name, [&]() {
                data.mf_name = rolling_pick_keeloq_manufacturer(data.mf_name);
            });

        if (proto->serial_bytes)
            options.emplace_back(
                String("Serial: ") + rolling_fmt_hex(data.serial, proto->serial_bytes), [&]() {
                    data.serial = rolling_edit_hex("Serial", data.serial, proto->serial_bytes);
                }
            );
        if (proto->button_bytes)
            options.emplace_back(
                String("Button: ") + rolling_fmt_hex(data.btn, proto->button_bytes), [&]() {
                    data.btn = rolling_edit_hex("Button", data.btn, proto->button_bytes);
                }
            );
        if (proto->counter_bytes)
            options.emplace_back(
                String("Counter: ") + rolling_fmt_hex(data.cnt, proto->counter_bytes), [&]() {
                    data.cnt = rolling_edit_hex("Counter", data.cnt, proto->counter_bytes);
                }
            );
        if (proto->has_seed)
            options.emplace_back(
                String("Seed: ") + rolling_fmt_hex(data.seed, proto->seed_bytes), [&]() {
                    data.seed = rolling_edit_hex("Seed", data.seed, proto->seed_bytes);
                }
            );

        options.emplace_back("Randomize all", [&]() { rolling_randomize(data, proto); });
        options.emplace_back("Generate", [&]() { generate = true; });
        options.emplace_back("Back", [&]() { cancel = true; });

        idx = loopOptions(options, MENU_TYPE_REGULAR, proto->display_name, idx);
        if (idx < 0) cancel = true; // Esc
    }
    return generate;
}

void createSignal() {
    returnToMenu = true;
    rolling_clear_globals();

    const RollingProtocol *proto = rolling_pick_protocol();
    if (!proto) return;

    RfCodes data{};
    data.rolling_protocol = proto->display_name;
    data.frequency = proto->frequency_hz;
    data.preset = proto->bw_preset == BW_OOK_650 ? "FuriHalSubGhzPresetOok650Async"
                                                 : "FuriHalSubGhzPresetOok270Async";
    data.mf_name = proto->mf_key_name ? String(proto->mf_key_name) : String(proto->display_name);
    data.te = proto->te_short_us;

    if (!rolling_param_form(data, proto)) return;

    // Backend key-presence check — warn (but allow continue) if this protocol
    // can't produce a receiver-valid frame without key material.
    if (!rolling_key_warning(proto, data.mf_name)) return;

    std::vector<int> timings = rolling_encode(data, proto);

    // Default file name: <protocol>_<serial>, sanitised; editable below.
    String defName = String(proto->display_name);
    defName.replace(" ", "_");
    defName.replace(",", "");
    defName.replace("/", "-");
    defName += "_" + rolling_fmt_hex(data.serial, proto->serial_bytes ? proto->serial_bytes : 4);

    // One screen: file name (prefilled, editable) at the top, storage selection,
    // and a Continue button.
    FS *fs = NULL;
    String fileName = defName;
    bool sdAvail = setupSdCard();
    bool useSD = sdAvail;
    bool proceed = false, cancel = false;
    int sidx = 0;
    while (!proceed && !cancel) {
        options = {};
        options.emplace_back(String("Name: ") + fileName, [&]() {
            String s = keyboard(fileName, 30, "Signal file name:");
            s.trim();
            if (s.length()) fileName = s;
        });
        if (sdAvail)
            options.emplace_back(String("Storage: ") + (useSD ? "SD Card" : "LittleFS"), [&]() {
                useSD = !useSD;
            });
        else options.emplace_back("Storage: LittleFS", [&]() {});
        options.emplace_back("Continue", [&]() { proceed = true; });
        options.emplace_back("Cancel", [&]() { cancel = true; });
        sidx = loopOptions(options, MENU_TYPE_REGULAR, "Save Signal", sidx);
        if (sidx < 0) cancel = true;
    }
    if (!proceed) return;

    fs = useSD ? (FS *)&SD : (FS *)&LittleFS;
    if (!fileName.endsWith(".sub")) fileName += ".sub";
    String path = "/BruceRF/" + fileName;
    rolling_code_save(data, path, fs);

    char hx[16] = {0};

    // Success / pairing screen.
    drawMainBorderWithTitle("Signal Created");
    padprintln(String(proto->display_name));
    decimalToHexString(data.serial, hx);
    padprintln("Serial:  " + String(hx));
    padprintln("Button:  " + String(data.btn));
    padprintln("Counter: " + String(data.cnt));
    padprintln("");
    padprintln("Put receiver in learn mode,");
    padprintln("then send this signal once.");
    padprintln("");
    padprintln("[Mid]=Send now (pair)  [Esc]=Skip");

    while (true) {
        if (check(SelPress)) {
            blinkLed();
            rolling_code_tx(timings, proto->frequency_hz / 1000000.0, proto->bw_preset, 3);
            if (proto->family != RF_FAMILY_SECURITY_PLUS_1) data.cnt += 1;
            rolling_code_save(data, path, fs);
            break;
        }
        if (check(EscPress)) break;
    }

    loopEmulateRolling(data, path, fs);
}

void sendRollingCode() {
    returnToMenu = true;
    FS *fs = NULL;
    String path;
    if (!pickRollingFile(fs, path, true)) return;

    rolling_clear_globals();
    RfCodes data{};
    if (!readSubFile(fs, path, data)) return;

    if (data.rolling_protocol == "") {
        // No descriptor matched — fall back to the static emulate path.
        if (data.serial != 0 || keyList.size()) loopEmulate(data);
        return;
    }
    loopEmulateRolling(data, path, fs);
}

void counterManager(std::function<void()> backFn) {
    FS *fs = NULL;
    String path;
    if (!pickRollingFile(fs, path, true)) return;

    rolling_clear_globals();
    RfCodes data{};
    if (!readSubFile(fs, path, data)) return;

    // Build title: "Counter Manager - <filename>" truncated to fit the screen.
    String fname = path.substring(path.lastIndexOf('/') + 1);
    if (fname.endsWith(".sub")) fname.remove(fname.length() - 4);
    const String base = "Counter Manager - ";
    int maxTitleChars = (tftWidth - 12) / (FP * LW);
    int maxFname = maxTitleChars - (int)base.length();
    if (maxFname > 0 && (int)fname.length() > maxFname) fname = fname.substring(0, maxFname);
    String title = maxFname > 0 ? base + fname : "Counter Manager";

    const RollingProtocol *proto = rolling_protocol_by_name(data.rolling_protocol.c_str());
    bool back = false;
    while (!back) {
        char hx[16] = {0};
        decimalToHexString(data.cnt, hx);

        options = {
            {"Increment",   [&]() { data.cnt += rolling_step_picker(); }},
            {"Decrement",   [&]() {
                 uint16_t s = rolling_step_picker();
                 bool somfy = proto && (proto->family == RF_FAMILY_SOMFY_TELIS ||
                                        proto->family == RF_FAMILY_SOMFY_KEYTIS);
                 if (somfy && s > data.cnt) {
                     if (!rolling_warn("Somfy counter can't\ngo backwards.\nReceiver may reject."))
                         return;
                 }
                 data.cnt -= s;
             }},
            {"Set Manually",[&]() {
                 data.cnt = rolling_edit_hex("Counter", data.cnt, 2);
             }},
            {"Reset to 0",  [&]() { data.cnt = 0; }},
            {"Save",        [&]() {
                 if (data.rolling_protocol == "") data.rolling_protocol = data.protocol;
                 rolling_code_save(data, path, fs);
                 displayTextLine("Counter saved", true);
             }},
            {"Back",        [&back, &backFn]() { back = true; if (backFn) backFn(); }},
        };
        if (loopOptions(options, MENU_TYPE_SUBMENU, title.c_str()) < 0) break;
    }
    rolling_clear_globals();
}
