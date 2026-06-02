#include "rf_send.h"
#include "core/led_control.h"
#include "core/type_convertion.h"
#include "rf_utils.h"
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
