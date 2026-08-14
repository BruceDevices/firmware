#include "custom_ir.h"
#include "TV-B-Gone.h" // for checkIrTxPin()
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "core/type_convertion.h"
#include "ir_utils.h"
#include <IRutils.h>

uint32_t swap32(uint32_t value) {
    return ((value & 0x000000FF) << 24) | ((value & 0x0000FF00) << 8) | ((value & 0x00FF0000) >> 8) |
           ((value & 0xFF000000) >> 24);
}

std::vector<uint16_t> parseRawDataString(const String &rawData) {
    std::vector<uint16_t> buffer;
    int idx = 0;
    while (idx < (int)rawData.length()) {
        while (idx < (int)rawData.length() && rawData[idx] == ' ') idx++;
        int start = idx;
        while (idx < (int)rawData.length() && rawData[idx] != ' ') idx++;
        if (idx > start) { buffer.push_back((uint16_t)rawData.substring(start, idx).toInt()); }
    }
    return buffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Custom IR

static std::vector<IRCode *> codes;

void resetCodesArray() {
    for (auto code : codes) { delete code; }
    codes.clear();
}

// Parse an .ir file into IRCode objects held in RAM (no repeated LittleFS reads)
// Returns false if the file can't be opened. Codes without a "name:" get skipped.
bool parseIrFile(FS *fs, const String &filepath, std::vector<IRCode *> &out, int maxCodes) {
    File databaseFile = fs->open(filepath, FILE_READ);
    if (!databaseFile) return false;
    if (maxCodes < 1) maxCodes = 1;

    String line;
    String txt;
    int total_codes = 0;

    out.push_back(new IRCode());

    while (databaseFile.available() && total_codes < maxCodes) {
        line = databaseFile.readStringUntil('\n');
        txt = line.substring(line.indexOf(":") + 1);
        txt.trim();

        if (line.startsWith("name:")) {
            if (out[total_codes]->name != "") {
                total_codes++;
                out.push_back(new IRCode());
            }
            if (total_codes >= maxCodes) break; // cap reached, stop parsing
            out[total_codes]->name = txt;
            out[total_codes]->filepath = txt + " " + filepath.substring(1 + filepath.lastIndexOf("/"));
        }

        if (line.startsWith("type:")) out[total_codes]->type = txt;
        if (line.startsWith("protocol:")) out[total_codes]->protocol = txt;
        if (line.startsWith("address:")) out[total_codes]->address = txt;
        if (line.startsWith("frequency:")) out[total_codes]->frequency = txt.toInt();
        if (line.startsWith("bits:")) out[total_codes]->bits = txt.toInt();
        if (line.startsWith("command:")) out[total_codes]->command = txt;
        if (line.startsWith("data:") || line.startsWith("value:") || line.startsWith("state:")) {
            out[total_codes]->data = txt;
        }

        if (line.startsWith("#") && out[total_codes]->name != "") {
            total_codes++;
            if (total_codes < maxCodes) out.push_back(new IRCode());
        }
    }
    databaseFile.close();

    // Pre-parse raw data once so spamming never re-parses the string or reads the FS
    for (auto code : out) {
        if (code->type.equalsIgnoreCase("raw") && code->data.length() > 0) {
            code->rawData = parseRawDataString(code->data);
        }
    }
    return !out.empty();
}

static std::vector<IRCode *> recent_ircodes;

void addToRecentCodes(IRCode *ircode) {
    // copy ircode -> recent_ircodes
    // if code exist in recent codes do not save it
    for (auto recent_ircode : recent_ircodes) {
        if (recent_ircode->filepath == ircode->filepath) { return; }
    }

    IRCode *ircode_copy = new IRCode(ircode);
    recent_ircodes.insert(recent_ircodes.begin(), ircode_copy);

    if (recent_ircodes.size() > 16) { // cycle
        delete recent_ircodes.back();
        recent_ircodes.pop_back();
    }
}

void selectRecentIrMenu() {
    // show menu with filenames
    checkIrTxPin();
    options = {};
    bool exit = false;
    IRCode *selected_code = NULL;
    for (auto recent_ircode : recent_ircodes) {
        if (recent_ircode->filepath == "") continue; // not inited
        // else
        options.push_back({recent_ircode->filepath.c_str(), [recent_ircode, &selected_code]() {
                               selected_code = recent_ircode;
                           }});
    }
    options.push_back({"Main Menu", [&]() { exit = true; }});

    int idx = 0;
    while (1) {
        idx = loopOptions(options, idx);
        if (selected_code != NULL) {
            sendIRCommand(selected_code);
            selected_code = NULL;
        }
        if (check(EscPress) || exit) break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    options.clear();

    return;
}

bool txIrFile(FS *fs, const String &filepath, bool hideDefaultUI) {
    // Load the whole file into RAM once, then spam all codes forever until ESC

    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);

    std::vector<IRCode *> fileCodes;
    if (!parseIrFile(fs, filepath, fileCodes, 100)) {
        if (!hideDefaultUI) {
            displayError("Fail to open file");
            delay(2000);
        }
        return false;
    }

    int total_codes = 0;
    for (auto code : fileCodes) {
        if (code->name != "") total_codes++;
    }

    if (total_codes == 0) {
        for (auto code : fileCodes) { delete code; }
        fileCodes.clear();
        digitalWrite(bruceConfigPins.irTx, LED_OFF);
        return false;
    }

    Serial.printf("Loaded %d codes into RAM, spamming until ESC\n", total_codes);
    if (!hideDefaultUI) { displayTextLine("Spamming " + String(total_codes) + " codes - ESC to stop"); }

    bool exit = false;
    uint32_t sent = 0;
    while (!check(EscPress) && !exit) {
        for (auto code : fileCodes) {
            if (code->name == "") continue;
            if (check(EscPress)) {
                exit = true;
                break;
            }
            sendIRCommand(code, true);
            sent++;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    for (auto code : fileCodes) { delete code; }
    fileCodes.clear();
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
    return true;
}

void otherIRcodes() {
    checkIrTxPin();
    resetCodesArray();
    String filepath;
    FS *fs = NULL;

    returnToMenu = true; // make sure menu is redrawn when quitting in any point

    options = {
        {"Recent",   selectRecentIrMenu       },
        {"LittleFS", [&]() { fs = &LittleFS; }},
        {"Menu",     yield                    },
    };
    if (setupSdCard()) options.insert(options.begin(), {"SD Card", [&]() { fs = &SD; }});

    loopOptions(options);

    if (fs == NULL) { // recent or menu was selected
        return;
    }

    // select a file to tx
    if (!(*fs).exists("/BruceIR")) (*fs).mkdir("/BruceIR");

    // startPath: remember the last visited folder so the user lands back there
    // after pressing back in the command list
    String startPath = "/BruceIR";

    while (true) {
        filepath = loopSD(*fs, true, "IR", startPath);
        if (filepath == "") return; // user cancelled / pressed back at root

        // Remember the folder of the selected file for next loop iteration
        startPath = filepath.substring(0, filepath.lastIndexOf('/'));
        if (startPath == "") startPath = "/";

        // select mode
        bool exit = false;
        bool mode_cmd = true;
        options = {
            {"Choose cmd", [&]() { mode_cmd = true; } },
            {"Spam all",   [&]() { mode_cmd = false; }},
            {"Menu",       [&]() { exit = true; }     },
        };

        loopOptions(options);

        if (exit) return;

        if (!mode_cmd) {
            // Spam all selected
            txIrFile(fs, filepath);
            // After spam, loop back to file picker in the same folder
            continue;
        }

        // Choose cmd:
        // chooseCmdIrFile returns false = short back → loop back to file browser
        //                          true  = long press / Main Menu → exit
        bool goToMain = chooseCmdIrFile(fs, filepath);
        if (goToMain) return;
        // else: loop back to loopSD, starting in the same folder (startPath)
    }
} // end of otherIRcodes

// Recursively collect all .ir file paths under dir (littlefs read once, paths only)
static void collectIrFiles(FS &fs, const String &dir, std::vector<String> &files, uint8_t depth) {
    if (depth > 8) return; // safety against cyclic/deep trees
    File root = fs.open(dir);
    if (!root || !root.isDirectory()) return;

    std::vector<String> subdirs;
    File entry = root.openNextFile();
    while (entry) {
        if (entry.isDirectory()) subdirs.push_back(entry.path());
        else if (String(entry.name()).endsWith(".ir")) files.push_back(entry.path());
        entry = root.openNextFile();
    }
    root.close();

    for (auto &sub : subdirs) collectIrFiles(fs, sub, files, depth + 1);
}

void spamAllIR() {
    checkIrTxPin();

    if (!setupLittleFS()) {
        displayError("LittleFS not mounted");
        delay(1000);
        return;
    }

    // 1) collect every .ir file on LittleFS (paths only, no parsing yet)
    std::vector<String> irFiles;
    collectIrFiles(LittleFS, "/BruceIR", irFiles, 0);
    if (irFiles.empty()) collectIrFiles(LittleFS, "/", irFiles, 0);
    if (irFiles.empty()) {
        displayError("No .ir files found");
        delay(1000);
        return;
    }

    // 2) load ALL codes from ALL files into RAM (single LittleFS pass)
    std::vector<IRCode *> allCodes;
    const int MAX_CODES = 200; // RAM safety cap (parsed codes are ~0.5KB each)
    for (size_t i = 0; i < irFiles.size() && (int)allCodes.size() < MAX_CODES; i++) {
        progressHandler(i, irFiles.size(), "Loading IR files...");
        std::vector<IRCode *> fileCodes;
        if (parseIrFile(&LittleFS, irFiles[i], fileCodes, MAX_CODES - (int)allCodes.size())) {
            for (auto code : fileCodes) {
                if (code->name == "") {
                    delete code;
                    continue;
                }
                allCodes.push_back(code);
            }
        }
        fileCodes.clear();
    }
    irFiles.clear();

    bool capped = (int)allCodes.size() >= MAX_CODES;
    if (allCodes.empty()) {
        displayError("No valid IR codes found");
        delay(1000);
        return;
    }

    String msg = "Loaded " + String((int)allCodes.size()) + " codes";
    if (capped) msg += " (RAM cap)";
    displayTextLine(msg);
    delay(500);

    // 3) spam everything from RAM, forever, until ESC
    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);
    bool exit = false;
    uint32_t sent = 0;
    while (!check(EscPress) && !exit) {
        for (auto code : allCodes) {
            if (check(EscPress)) {
                exit = true;
                break;
            }
            sendIRCommand(code, true);
            sent++;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    for (auto code : allCodes) { delete code; }
    allCodes.clear();
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

// IR commands

void sendIRCommand(IRCode *code, bool hideDefaultUI) {
    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);
    // https://developer.flipper.net/flipperzero/doxygen/infrared_file_format.html
    if (code->type.equalsIgnoreCase("raw")) {
        if (!code->rawData.empty()) sendRawBuffer(code->rawData, code->frequency, hideDefaultUI);
        else sendRawCommand(code->frequency, code->data, hideDefaultUI);
    } else if (code->protocol.equalsIgnoreCase("NEC"))
        sendNECCommand(code->address, code->command, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("NECext"))
        sendNECextCommand(code->address, code->command, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("RC5") || code->protocol.equalsIgnoreCase("RC5X"))
        sendRC5Command(code->address, code->command, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("RC6"))
        sendRC6Command(code->address, code->command, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("Samsung32"))
        sendSamsungCommand(code->address, code->command, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("SIRC"))
        sendSonyCommand(code->address, code->command, 12, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("SIRC15"))
        sendSonyCommand(code->address, code->command, 15, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("SIRC20"))
        sendSonyCommand(code->address, code->command, 20, hideDefaultUI);
    else if (code->protocol.equalsIgnoreCase("Kaseikyo"))
        sendKaseikyoCommand(code->address, code->command, hideDefaultUI);
    // Others protocols of IRRemoteESP8266, not related to Flipper Zero IR File Format
    else if (
        code->protocol != "" && code->data != "" &&
        strToDecodeType(code->protocol.c_str()) != decode_type_t::UNKNOWN
    )
        sendDecodedCommand(code->protocol, code->data, code->bits, hideDefaultUI);
}

void sendNECCommand(String address, String command, bool hideDefaultUI) {
    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }
    uint16_t addressValue = strtoul(address.substring(0, 2).c_str(), nullptr, 16);
    uint16_t commandValue = strtoul(command.substring(0, 2).c_str(), nullptr, 16);
    uint64_t data = irsend.encodeNEC(addressValue, commandValue);
    irsend.sendNEC(data, 32);

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.sendNEC(data, 32); }
    }

    Serial.println(
        "Sent NEC Command" + (bruceConfigPins.irTxRepeats > 0
                                  ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                  : "")
    );

    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

void sendNECextCommand(String address, String command, bool hideDefaultUI) {
    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }

    int first_zero_byte_pos = address.indexOf("00", 2);
    if (first_zero_byte_pos != -1) address = address.substring(0, first_zero_byte_pos);
    first_zero_byte_pos = command.indexOf("00", 2);
    if (first_zero_byte_pos != -1) command = command.substring(0, first_zero_byte_pos);

    address.replace(" ", "");
    command.replace(" ", "");

    uint16_t addressValue = strtoul(address.c_str(), nullptr, 16);
    uint16_t commandValue = strtoul(command.c_str(), nullptr, 16);

    // Invert Endianness
    uint16_t newAddress = (addressValue >> 8) | (addressValue << 8);
    uint16_t newCommand = (commandValue >> 8) | (commandValue << 8);

    // NEC protocol bit order is LSB first
    uint16_t lsbAddress = reverseBits(newAddress, 16);
    uint16_t lsbCommand = reverseBits(newCommand, 16);

    uint32_t data = ((uint32_t)lsbAddress << 16) | lsbCommand;
    irsend.sendNEC(data, 32); // Sends MSB first

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.sendNEC(data, 32); }
    }

    Serial.println(
        "Sent NECext Command" + (bruceConfigPins.irTxRepeats > 0
                                     ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                     : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

void sendRC5Command(String address, String command, bool hideDefaultUI) {
    IRsend irsend(bruceConfigPins.irTx, true); // Set the GPIO to be used to sending the message
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }
    uint8_t addressValue = strtoul(address.substring(0, 2).c_str(), nullptr, 16);
    uint8_t commandValue = strtoul(command.substring(0, 2).c_str(), nullptr, 16);
    uint16_t data = irsend.encodeRC5(addressValue, commandValue);
    irsend.sendRC5(data, 13);

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.sendRC5(data, 13); }
    }
    Serial.println(
        "Sent RC5 Command" + (bruceConfigPins.irTxRepeats > 0
                                  ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                  : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

void sendRC6Command(String address, String command, bool hideDefaultUI) {
    IRsend irsend(bruceConfigPins.irTx, true); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }
    address.replace(" ", "");
    command.replace(" ", "");
    uint32_t addressValue = strtoul(address.substring(0, 2).c_str(), nullptr, 16);
    uint32_t commandValue = strtoul(command.substring(0, 2).c_str(), nullptr, 16);
    uint64_t data = irsend.encodeRC6(addressValue, commandValue);

    irsend.sendRC6(data, 20);

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.sendRC6(data, 20); }
    }

    Serial.println(
        "Sent RC6 Command" + (bruceConfigPins.irTxRepeats > 0
                                  ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                  : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

void sendSamsungCommand(String address, String command, bool hideDefaultUI) {
    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }
    uint8_t addressValue = strtoul(address.substring(0, 2).c_str(), nullptr, 16);
    uint8_t commandValue = strtoul(command.substring(0, 2).c_str(), nullptr, 16);
    uint64_t data = irsend.encodeSAMSUNG(addressValue, commandValue);

    irsend.sendSAMSUNG(data, 32);

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.sendSAMSUNG(data, 32); }
    }

    Serial.println(
        "Sent Samsung Command" + (bruceConfigPins.irTxRepeats > 0
                                      ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                      : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

void sendSonyCommand(String address, String command, uint8_t nbits, bool hideDefaultUI) {
    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }

    address.replace(" ", "");
    command.replace(" ", "");

    uint32_t addressValue = strtoul(address.c_str(), nullptr, 16);
    uint32_t commandValue = strtoul(command.c_str(), nullptr, 16);

    uint16_t swappedAddr = static_cast<uint16_t>(swap32(addressValue));
    uint8_t swappedCmd = static_cast<uint8_t>(swap32(commandValue));

    uint32_t data;

    if (nbits == 12) {
        // SIRC (12 bits)
        data = ((swappedAddr & 0x1F) << 7) | (swappedCmd & 0x7F);
    } else if (nbits == 15) {
        // SIRC15 (15 bits)
        data = ((swappedAddr & 0xFF) << 7) | (swappedCmd & 0x7F);
    } else if (nbits == 20) {
        // SIRC20 (20 bits)
        data = ((swappedAddr & 0x1FFF) << 7) | (swappedCmd & 0x7F);
    } else {
        Serial.println("Invalid Sony (SIRC) protocol bit size.");
        return;
    }

    // SIRC protocol bit order is LSB First
    data = reverseBits(data, nbits);

    // 1 initial + 2 repeat
    irsend.sendSony(data, nbits, 2); // Sends MSB First

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.sendSony(data, nbits, 2); }
    }

    Serial.println(
        "Sent Sony Command" + (bruceConfigPins.irTxRepeats > 0
                                   ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                   : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

void sendKaseikyoCommand(String address, String command, bool hideDefaultUI) {
    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }

    address.replace(" ", "");
    command.replace(" ", "");

    uint32_t addressValue = strtoul(address.c_str(), nullptr, 16);
    uint32_t commandValue = strtoul(command.c_str(), nullptr, 16);

    uint32_t newAddress = swap32(addressValue);
    uint16_t newCommand = static_cast<uint16_t>(swap32(commandValue));

    uint8_t id = (newAddress >> 24) & 0xFF;
    uint16_t vendor_id = (newAddress >> 8) & 0xFFFF;
    uint8_t genre1 = (newAddress >> 4) & 0x0F;
    uint8_t genre2 = newAddress & 0x0F;

    uint16_t data = newCommand & 0x3FF;

    byte bytes[6];
    bytes[0] = vendor_id & 0xFF;
    bytes[1] = (vendor_id >> 8) & 0xFF;

    uint8_t vendor_parity = bytes[0] ^ bytes[1];
    vendor_parity = (vendor_parity & 0xF) ^ (vendor_parity >> 4);

    bytes[2] = (genre1 << 4) | (vendor_parity & 0x0F);
    bytes[3] = ((data & 0x0F) << 4) | genre2;
    bytes[4] = ((id & 0x03) << 6) | ((data >> 4) & 0x3F);

    bytes[5] = bytes[2] ^ bytes[3] ^ bytes[4];

    uint64_t lsb_data = 0;
    for (int i = 0; i < 6; i++) { lsb_data |= (uint64_t)bytes[i] << (8 * i); }

    // LSB First --> MSB First
    uint64_t msb_data = reverseBits(lsb_data, 48);

    irsend.sendPanasonic64(msb_data, 48); // Sends MSB First

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.sendPanasonic64(msb_data, 48); }
    }

    Serial.println(
        "Sent Kaseikyo Command" + (bruceConfigPins.irTxRepeats > 0
                                       ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                       : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

bool sendDecodedCommand(String protocol, String value, uint8_t bits, bool hideDefaultUI) {
    // https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/SmartIRRepeater/SmartIRRepeater.ino
#if !defined(LITE_VERSION)
    decode_type_t type = strToDecodeType(protocol.c_str());
    if (type == decode_type_t::UNKNOWN) return false;

    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    bool success = false;
    if (!hideDefaultUI) { displayTextLine("Sending.."); }

    if (hasACState(type)) {
        // need to send the state (still passed from value)
        uint8_t state[bits / 8] = {0};
        uint16_t state_pos = 0;
        for (uint16_t i = 0; i < value.length(); i += 3) {
            // parse  value -> state
            uint8_t highNibble = hexCharToDecimal(value[i]);
            uint8_t lowNibble = hexCharToDecimal(value[i + 1]);
            state[state_pos] = (highNibble << 4) | lowNibble;
            state_pos++;
        }
        // success = irsend.send(type, state, bits / 8);
        success = irsend.send(type, state, state_pos); // safer

        if (bruceConfigPins.irTxRepeats > 0) {
            for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) {
                irsend.send(type, state, state_pos);
            }
        }

    } else {
        value.replace(" ", "");
        uint64_t value_int = strtoull(value.c_str(), nullptr, 16);

        success =
            irsend.send(type, value_int, bits); // bool send(const decode_type_t type, const uint64_t data,
                                                // const uint16_t nbits, const uint16_t repeat = kNoRepeat);

        if (bruceConfigPins.irTxRepeats > 0) {
            for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) { irsend.send(type, value_int, bits); }
        }
    }

    delay(20);
    Serial.println(
        "Sent Decoded Command" + (bruceConfigPins.irTxRepeats > 0
                                      ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                      : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
    return success;
#else
    if (!hideDefaultUI) { displayTextLine("Unavailable on this Version"); }
    delay(1000);
    return false;
#endif
}

void sendRawBuffer(const std::vector<uint16_t> &buffer, uint16_t frequency, bool hideDefaultUI) {
#ifdef USE_BOOST /// ENABLE 5V OUTPUT
    PPM.enableOTG();
#endif

    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }

    irsend.sendRaw(buffer.data(), buffer.size(), frequency);

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) {
            irsend.sendRaw(buffer.data(), buffer.size(), frequency);
        }
    }

    Serial.println(
        "Sent Raw Command" + (bruceConfigPins.irTxRepeats > 0
                                  ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                  : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

void sendRawCommand(uint16_t frequency, String rawData, bool hideDefaultUI) {
#ifdef USE_BOOST /// ENABLE 5V OUTPUT
    PPM.enableOTG();
#endif

    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    if (!hideDefaultUI) { displayTextLine("Sending.."); }

    uint16_t dataBufferSize = 1;
    for (int i = 0; i < rawData.length(); i++) {
        if (rawData[i] == ' ') dataBufferSize += 1;
    }
    uint16_t *dataBuffer = (uint16_t *)malloc((dataBufferSize) * sizeof(uint16_t));

    uint16_t count = 0;
    // Parse raw data string
    while (rawData.length() > 0 && count < dataBufferSize) {
        int delimiterIndex = rawData.indexOf(' ');
        if (delimiterIndex == -1) { delimiterIndex = rawData.length(); }
        String dataChunk = rawData.substring(0, delimiterIndex);
        rawData.remove(0, delimiterIndex + 1);
        dataBuffer[count++] = (dataChunk.toInt());
    }

    Serial.println("Parsing raw data complete.");
    // Serial.println(count);
    // Serial.println(dataBuffer[count-1]);
    // Serial.println(dataBuffer[0]);

    // Send raw command
    irsend.sendRaw(dataBuffer, count, frequency);

    if (bruceConfigPins.irTxRepeats > 0) {
        for (uint8_t i = 1; i <= bruceConfigPins.irTxRepeats; i++) {
            irsend.sendRaw(dataBuffer, count, frequency);
        }
    }

    free(dataBuffer);

    Serial.println(
        "Sent Raw Command" + (bruceConfigPins.irTxRepeats > 0
                                  ? " (1 initial + " + String(bruceConfigPins.irTxRepeats) + " repeats)"
                                  : "")
    );
    digitalWrite(bruceConfigPins.irTx, LED_OFF);
}

bool chooseCmdIrFile(FS *fs, const String &filepath) {
    checkIrTxPin();
    resetCodesArray();
    int total_codes = 0;
    File databaseFile;

    returnToMenu = true;

    databaseFile = fs->open(filepath, FILE_READ);
    drawMainBorder();

    if (!databaseFile) {
        Serial.println("Failed to open IR file.");
        return false;
    }
    Serial.println("Opened IR file.");

    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);

    // Mode to choose and send command by command (limitted to 100 commands)
    if (!parseIrFile(fs, filepath, codes, 100)) {
        databaseFile.close();
        return false;
    }
    databaseFile.close();

    int validCodes = 0;
    for (auto code : codes) {
        if (code->name != "") validCodes++;
    }
    total_codes = validCodes;

    options = {};
    bool exit = false;
    bool goToMainMenu = false;
    bool actionTaken = false;

    for (auto code : codes) {
        if (code->name != "") {
            options.push_back({code->name.c_str(), [code, &actionTaken]() {
                                   actionTaken = true;
                                   sendIRCommand(code);
                                   addToRecentCodes(code);
                               }});
        }
    }
    options.push_back({"Main Menu", [&]() {
                           actionTaken = true;
                           exit = true;
                           goToMainMenu = true;
                       }});
    databaseFile.close();

#ifdef USE_BOOST /// DISABLE 5V OUTPUT
    PPM.disableOTG();
#endif

    digitalWrite(bruceConfigPins.irTx, LED_OFF);
    int idx = 0;
    while (1) {
        actionTaken = false;
        idx = loopOptions(options, idx);

        if (exit) break;

        // loopOptions returned without any lambda running → EscPress was consumed internally
        // Treat it like a back button press
        if (!actionTaken) {
            // Distinguish short vs long press by checking if button is still held
            unsigned long pressStart = millis();
            bool longPress = false;
            while (check(EscPress)) { // button still physically held
                if (millis() - pressStart >= 2000) {
                    longPress = true;
                    break;
                }
                delay(10);
            }
            while (check(EscPress)) delay(10); // wait for release

            if (longPress) goToMainMenu = true;
            // Short (or already released): goToMainMenu stays false → back to file browser
            break;
        }
    }
    options.clear();
    resetCodesArray();
    // Flush any residual EscPress
    delay(100);
    while (check(EscPress)) delay(10);

    if (!goToMainMenu) {
        // Short press: going back to file browser, NOT to main menu
        // Reset returnToMenu so loopOptions chain doesn't cascade-exit everything
        returnToMenu = false;
    }
    // true  = go to main menu (long press or "Main Menu" item selected)
    // false = go back to file browser (short Esc press)
    return goToMainMenu;
}
