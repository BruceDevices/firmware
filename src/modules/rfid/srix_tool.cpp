/**
 * @file srix_tool.cpp
 * @brief SRIX4K/SRIX512 Reader/Writer Tool v1.3
 * @author Senape3000
 * @info https://github.com/Senape3000/firmware/blob/main/docs_custom/SRIX/SRIX_Tool_README.md
 * @date 2026-01-01
 */
#include "srix_tool.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/settings.h"
#include <vector>

#define TAG_TIMEOUT_MS 100
#define TAG_MAX_ATTEMPTS 5
static constexpr uint32_t SRIX_EEPROM_WRITE_DELAY_MS = 15; // Delay for write operation

SRIXTool::SRIXTool() {
    current_state = IDLE_MODE;
    _dump_valid_from_read = false;
    _dump_valid_from_load = false;
    _tag_read = false;
    setup();
}

SRIXTool::~SRIXTool() { delete nfc; }

void SRIXTool::setup() {
    // Use I2C pins from global config
    int sda_pin = bruceConfigPins.i2c_bus.sda;
    int scl_pin = bruceConfigPins.i2c_bus.scl;

    drawMainBorderWithTitle("SRIX TOOL");
    padprintln("");
    padprintln("Initializing I2C...");

    // Init I2C
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(100000);

    padprintln("Initializing PN532...");

// Create PN532 object based on board
#if defined(PN532_IRQ) && defined(PN532_RF_REST)
    // Board with integrated PN532 (e.g. T-Embed
    nfc = new Arduino_PN532_SRIX(PN532_IRQ, PN532_RF_REST);
    padprintln("Hardware mode (IRQ + RST)");
#else
    // Board with external PN532 I2C (e.g. CYD)
    nfc = new Arduino_PN532_SRIX(-1, -1);
    padprintln("I2C-only mode");
#endif

    if (!nfc->init()) {
        displayError("PN532 not found!", true);
        return;
    }

    padprintln("Init OK, testing retries...");

    // Configure for SRIX
    if (!nfc->setPassiveActivationRetries(0xFF)) {
        displayError("Retry config failed!", true);
        delay(500);
        return;
    }

    padprintln("Testing SRIX init...");
    if (!nfc->SRIX_init()) {
        displayError("SRIX init failed!", true);
        return;
    }
    uint32_t ver = nfc->getFirmwareVersion();
    if (ver) {
        uint8_t chip = (ver >> 24) & 0xFF;
        uint8_t fw_major = (ver >> 16) & 0xFF;
        uint8_t fw_minor = (ver >> 8) & 0xFF;

        padprintln("Chip: PN5" + String(chip, HEX));
        padprintln("FW: " + String(fw_major) + "." + String(fw_minor));
    }
    delay(2000);
    displaySuccess("PN532-SRIX ready!");
    delay(2500);

    set_state(IDLE_MODE);
    return loop();
}

void SRIXTool::loop() {
    while (1) {
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }

        if (check(SelPress)) { select_state(); }

        switch (current_state) {
            case IDLE_MODE: show_main_menu(); break;
            case READ_TAG_MODE: read_tag(); break;
            case WRITE_TAG_MODE: write_tag(); break;
            case READ_UID_MODE: read_uid(); break;
            case PN_INFO_MODE: show_pn_info(); break;
            case SAVE_MODE: save_file(); break;
            case LOAD_MODE:

                if (_screen_drawn) {
                    delay(50);
                } else {
                    load_file();
                }
                break;
        }
    }
}

void SRIXTool::select_state() {
    options = {};

    options.emplace_back("Main Menu", [this]() { set_state(IDLE_MODE); });
    options.emplace_back("Read tag", [this]() { set_state(READ_TAG_MODE); });

    if (_dump_valid_from_read) {
        options.emplace_back(" -Save dump", [this]() { set_state(SAVE_MODE); });
        options.emplace_back(" -Clone tag", [this]() { set_state(WRITE_TAG_MODE); });
    }
    options.emplace_back("Read UID", [this]() { set_state(READ_UID_MODE); });
    options.emplace_back("Load dump", [this]() { set_state(LOAD_MODE); });
    if (_dump_valid_from_load) {
        options.emplace_back(" -Write to tag", [this]() { set_state(WRITE_TAG_MODE); });
    }
    options.emplace_back("PN532 Info", [this]() { set_state(PN_INFO_MODE); });

    loopOptions(options);
}

void SRIXTool::set_state(SRIX_State state) {
    current_state = state;
    _tag_read = false;
    _screen_drawn = false;
    if (state == READ_TAG_MODE) { _dump_valid_from_load = false; }
    display_banner();
    delay(300);
}

void SRIXTool::display_banner() {
    drawMainBorderWithTitle("SRIX TOOL");

    switch (current_state) {
        case READ_TAG_MODE: printSubtitle("READ TAG MODE"); break;
        case WRITE_TAG_MODE: printSubtitle("WRITE TAG MODE"); break;
        case READ_UID_MODE: printSubtitle("READ UID MODE"); break;
        case PN_INFO_MODE: printSubtitle("PN532 INFO"); break;
        case IDLE_MODE: printSubtitle("MAIN MENU"); break;
        case SAVE_MODE: printSubtitle("SAVE MODE"); break;
        case LOAD_MODE: printSubtitle("LOAD MODE"); break;
    }

    tft.setTextSize(FP);
    padprintln("");
}

void SRIXTool::show_main_menu() {
    if (_screen_drawn) {
        delay(50);
        return;
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    padprintln("SRIX Tool for SRIX4K/512 v1.3");
    padprintln("");
    padprintln("Features:");
    padprintln("- Read/Clone complete tag (512B)");
    padprintln("- Write tag from memory");
    padprintln("- Save/Load .srix dumps");
    padprintln("- Read 8-byte UID");
    padprintln("- PN532 module info");
    padprintln("");

    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    padprintln("Press [OK] to open menu");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    _screen_drawn = true;
}

bool SRIXTool::waitForTag() {
    static uint8_t attempts = 0;
    static unsigned long attemptStart = 0;
    static bool message_shown = false;
    static unsigned long lastDotTime = 0; // For progress dots

    if (millis() - _lastReadTime < 2000) return false;

    if (attempts == 0 && !message_shown) {
        attemptStart = millis();
        padprint("Waiting for tag");
        message_shown = true;
        lastDotTime = millis();
    }

    // Show progress dots every second
    if (message_shown && millis() - lastDotTime > TAG_TIMEOUT_MS) {
        tft.print(".");
        lastDotTime = millis();
    }

    unsigned long elapsed = millis() - attemptStart;
    if (elapsed > TAG_TIMEOUT_MS) {
        attempts++;
        attemptStart = millis();

        if (attempts >= TAG_MAX_ATTEMPTS) {
            padprintln("");
            padprintln("");
            padprintln("Timeout! No tag found.");
            attempts = 0;
            message_shown = false;
            delay(1000);
            display_banner();
            return false;
        }
    }

    if (nfc->SRIX_initiate_select()) {
        attempts = 0;
        message_shown = false;
        padprintln("");
        return true;
    }

    return false;
}

void SRIXTool::read_tag() {

    if (_screen_drawn) {
        delay(50);
        return;
    }

    if (!waitForTag()) return;

    display_banner();

    _dump_valid_from_read = false;
    padprintln("Tag detected!");
    padprintln("");

    // Read UID
    if (!nfc->SRIX_get_uid(_uid)) {
        displayError("Failed to read UID!");
        delay(2000);
        set_state(READ_TAG_MODE);
        return;
    }

    // Show UID
    String uid_str = "";
    for (uint8_t i = 0; i < 8; i++) {
        if (_uid[i] < 0x10) uid_str += "0";
        uid_str += String(_uid[i], HEX);
        if (i < 7) uid_str += " ";
    }
    uid_str.toUpperCase();
    padprintln("UID: " + uid_str);
    padprintln("");

    // Read 128 blocks
    padprintln("Reading 128 blocks...");
    padprint("Please Wait");
    uint8_t block[4];

    for (uint8_t b = 0; b < 128; b++) {
        if (!nfc->SRIX_read_block(b, block)) {
            displayError("Read failed at block " + String(b));
            delay(2000);
            set_state(READ_TAG_MODE);
            return;
        }

        const uint16_t off = (uint16_t)b * 4;
        _dump[off + 0] = block[0];
        _dump[off + 1] = block[1];
        _dump[off + 2] = block[2];
        _dump[off + 3] = block[3];

        // Progress indicator
        progressHandler(b + 1, 128, "Reading data blocks");
    }

    _dump_valid_from_read = true;
    _dump_valid_from_load = false;
    padprintln("");
    padprintln("");
    displaySuccess("Tag read successfully!");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("");

    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    padprintln("Press [OK] for Main Menu");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    _tag_read = true;
    _lastReadTime = millis();
    _screen_drawn = true;
}

void SRIXTool::write_tag() {
    if (!_dump_valid_from_read && !_dump_valid_from_load) {
        displayError("No data in memory!");
        displayError("Read or load a dump first.");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }
#ifdef SRIX_DEBUG_WRITE_SIMULATION
    // ✅ DEBUG MODE
    display_banner();
    tft.setTextSize(FM);
    padprintln("DEBUG MODE ACTIVE");
    padprintln("Simulation via Serial...");
    padprintln("");

    Serial.println("\n========== SRIX WRITE SIMULATION ==========");
    Serial.print("Target UID: ");
    for (int i = 0; i < 8; i++) { Serial.printf("%02X", _uid[i]); }
    Serial.println("\n");

    uint8_t block[4];
    for (uint8_t b = 0; b < 128; b++) {
        const uint16_t off = (uint16_t)b * 4;
        block[0] = _dump[off + 0];
        block[1] = _dump[off + 1];
        block[2] = _dump[off + 2];
        block[3] = _dump[off + 3];

        // Output:[XX]: 4 byte raw hex
        Serial.printf("[%02X]: %02X%02X%02X%02X\n", b, block[0], block[1], block[2], block[3]);

        delay(70);
    }

    Serial.println("\n========== SIMULATION COMPLETE ==========");
    Serial.printf("Total: %d blocks (%d bytes)\n", 128, 512);
    Serial.println("===========================================\n");

    displaySuccess("Simulation complete!");
    displayInfo("Check Serial Monitor");
    delay(4000);
    set_state(IDLE_MODE);
    return;

#else

    if (!waitForTag()) return;

    display_banner();
    padprintln("Tag detected!");
    padprintln("");
    padprintln("Writing 128 blocks...");
    padprintln("");
    padprint("Please Wait");

    uint8_t block[4];
    uint8_t blocks_written = 0;
    uint8_t blocks_failed = 0;
    String failed_blocks = "";

    for (uint8_t b = 0; b < 128; b++) {
        const uint16_t off = (uint16_t)b * 4;
        block[0] = _dump[off + 0];
        block[1] = _dump[off + 1];
        block[2] = _dump[off + 2];
        block[3] = _dump[off + 3];

        if (!nfc->SRIX_write_block(b, block)) {
            SRIX_LOG("[WriteBlock_GUI] ❌ WRITE BLOCK FAILED");
            blocks_failed++;
            if (blocks_failed <= 10) { // Max 10 block for report
                if (failed_blocks.length() > 0) failed_blocks += ",";
                failed_blocks += String(b);
            }
        } else {
            SRIX_LOG("[WriteBlock_GUI] ✅ WRITE BLOCK: %u", blocks_written);
            blocks_written++;
            delay(SRIX_EEPROM_WRITE_DELAY_MS); // Delay for write
            waitForTagHeadless(600);
        }

        progressHandler(b + 1, 128, "Writing data blocks");
    }

    padprintln("");
    padprintln("");

    // Final report
    if (blocks_failed == 0) {
        displaySuccess("Write complete!", true);

    } else if (blocks_written > 0) {
        displayWarning("Partial write!", true);
        padprintln("");
        padprintln("Written: " + String(blocks_written) + "/128");
        padprintln("Failed: " + String(blocks_failed));
        padprintln("");
        tft.setTextSize(FP);

        // Over 10 failed block add "..."
        if (blocks_failed > 10) {
            padprintln("Failed blocks: " + failed_blocks + "...");
        } else {
            padprintln("Failed blocks: " + failed_blocks);
        }

    } else {
        displayError("Write failed!", true);
        padprintln("No blocks written");
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    _lastReadTime = millis();
    delayWithReturn(3000);
    set_state(IDLE_MODE);
#endif
}

void SRIXTool::read_uid() {

    if (_screen_drawn) {
        delay(50);
        return;
    }

    if (!waitForTag()) return;

    display_banner();

    padprintln("Tag detected!");
    padprintln("");
    padprintln("");

    if (!nfc->SRIX_get_uid(_uid)) {
        displayError("Failed to read UID!");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        delay(2000);
        set_state(READ_UID_MODE);
        return;
    }

    String uid_parts[4]; // Array for 4 groups

    // Build the 4 groups (2 bytes each = 4 hex characters)
    for (uint8_t group = 0; group < 4; group++) {
        uid_parts[group] = "";
        for (uint8_t byte_in_group = 0; byte_in_group < 2; byte_in_group++) {
            uint8_t idx = group * 2 + byte_in_group;
            if (_uid[idx] < 0x10) uid_parts[group] += "0";
            uid_parts[group] += String(_uid[idx], HEX);
        }
        uid_parts[group].toUpperCase();
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);
    // Build and print directly
    String uid_line = "UID: ";
    for (uint8_t i = 0; i < 8; i++) {
        if (_uid[i] < 0x10) uid_line += "0";
        uid_line += String(_uid[i], HEX);

        if (i == 1 || i == 3 || i == 5) { uid_line += " "; }
    }
    uid_line.toUpperCase();
    padprintln(uid_line); // Output: "0123 4567 89AB CDEF"
    padprintln("");
    tft.setTextSize(FP);
    padprintln("");

    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    padprintln("Press [OK] for Main Menu");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    _lastReadTime = millis();
    _screen_drawn = true;
}

void SRIXTool::show_pn_info() {

    if (_screen_drawn) {
        delay(50);
        return;
    }

    uint32_t ver = nfc->getFirmwareVersion();
    if (!ver) {
        displayError("Failed to read firmware!");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);
    padprintln("PN532 Info:");
    tft.setTextSize(FP);
    padprintln("");

    uint8_t chip = (ver >> 24) & 0xFF;
    uint8_t fw_major = (ver >> 16) & 0xFF;
    uint8_t fw_minor = (ver >> 8) & 0xFF;

    padprintln("Chip: PN5" + String(chip, HEX));
    padprintln("");
    padprintln("Firmware: " + String(fw_major) + "." + String(fw_minor));
    padprintln("");

    if (_has_hardware_pins) {
        padprintln("Mode: Hardware (IRQ+RST)");
    } else {
        padprintln("Mode: I2C-Only (Polling)");
    }

    _screen_drawn = true;
}

void SRIXTool::save_file() {
    if (!_dump_valid_from_read) {
        displayError("No data in memory!");
        displayError("Read a tag first.");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    // Set UID as default filename
    String uid_str = "";
    for (uint8_t i = 0; i < 8; i++) {
        if (_uid[i] < 0x10) uid_str += "0";
        uid_str += String(_uid[i], HEX);
    }
    uid_str.toUpperCase();

    // Ask the user for the file name
    String filename = keyboard(uid_str, 30, "File name:");
    filename.trim();

    if (filename == "\x1B") {
        // User cancelled the operation
        padprintln("Operation cancelled.");
        delay(2000);
        set_state(IDLE_MODE); // O torna allo stato precedente
        return;
    }

    if (filename.isEmpty()) {
        displayError("Invalid filename!");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    display_banner();

    // Get filesystem
    FS *fs;
    if (!getFsStorage(fs)) {
        displayError("Filesystem error!");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    // Create directory if it does not exist
    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    if (!(*fs).exists("/BruceRFID/SRIX")) (*fs).mkdir("/BruceRFID/SRIX");

    // Check if the file already exists and add number
    String filepath = "/BruceRFID/SRIX/" + filename;
    if ((*fs).exists(filepath + ".srix")) {
        int i = 1;
        while ((*fs).exists(filepath + "_" + String(i) + ".srix")) i++;
        filepath += "_" + String(i);
    }
    filepath += ".srix";

    // Open file for writing
    File file = (*fs).open(filepath, FILE_WRITE);
    if (!file) {
        displayError("Error creating file!");
        delay(1500);
        set_state(IDLE_MODE);
        return;
    }

    // Write header
    file.println("Filetype: Bruce SRIX Dump");
    file.println("UID: " + uid_str);
    file.println("Blocks: 128");
    file.println("Data size: 512");
    file.println("# Data:");

    // Write all blocks in [XX] format YYYYYYYY
    for (uint8_t block = 0; block < 128; block++) {
        uint16_t offset = block * 4;

        String line = "[";

        // Block number in hex (2 sign)
        if (block < 0x10) line += "0";
        line += String(block, HEX);
        line += "] ";

        for (uint8_t i = 0; i < 4; i++) {
            if (_dump[offset + i] < 0x10) line += "0";
            line += String(_dump[offset + i], HEX);
        }

        line.toUpperCase();
        file.println(line);
    }

    file.close();

    displaySuccess("File saved!");
    padprintln("");
    padprintln("Path: " + filepath);

    delayWithReturn(2500);
    set_state(IDLE_MODE);
}

void SRIXTool::load_file() {
    FS *fs;
    if (!getFsStorage(fs)) {
        displayError("Filesystem error!");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    // Verify that the directory exists
    if (!(*fs).exists("/BruceRFID/SRIX")) {
        displayError("No dumps found!");
        delay(1500);
        displayError("Folder /BruceRFID/SRIX missing");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    // List all .srix files in the directory
    File dir = (*fs).open("/BruceRFID/SRIX");
    if (!dir || !dir.isDirectory()) {
        displayError("Cannot open SRIX folder!");
        delay(1500);
        set_state(IDLE_MODE);
        return;
    }

    // Build file list
    std::vector<String> fileList;
    File file = dir.openNextFile();
    while (file) {
        String filename = String(file.name());
        if (filename.endsWith(".srix") && !file.isDirectory()) {
            // Remove the full path, keep only the name
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash >= 0) { filename = filename.substring(lastSlash + 1); }
            fileList.push_back(filename);
        }
        file = dir.openNextFile();
    }
    dir.close();

    if (fileList.empty()) {
        displayError("No .srix files found!");
        delay(2500);
        set_state(IDLE_MODE);
        return;
    }

    // Show Menu
    display_banner();
    padprintln("Select file to load:");
    padprintln("");

    options = {};
    for (const String &fname : fileList) {
        options.emplace_back(fname, [this, fname, fs]() { load_file_data(fs, "/BruceRFID/SRIX/" + fname); });
    }
    options.emplace_back("Cancel", [this]() { set_state(IDLE_MODE); });

    loopOptions(options);
}

void SRIXTool::load_file_data(FS *fs, String filepath) {

    if (_screen_drawn) {
        delay(50);
        return;
    }

    display_banner();
    padprintln("Loading: " + filepath);
    padprintln("");

    File file = (*fs).open(filepath, FILE_READ);
    if (!file) {
        displayError("Cannot open file!");
        delay(1500);
        set_state(IDLE_MODE);
        return;
    }

    // Reset buffer
    memset(_dump, 0, sizeof(_dump));
    memset(_uid, 0, sizeof(_uid));
    _dump_valid_from_read = false;

    bool header_passed = false;
    int blocks_loaded = 0;
    String uid_from_file = "";

    // Parse file line by line
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        // Skip empty
        if (line.isEmpty()) continue;

        // Extract UID from header
        if (line.startsWith("UID:")) {
            uid_from_file = line.substring(5);
            uid_from_file.trim();
            uid_from_file.replace(" ", "");

            // Convert UID to byte array
            for (uint8_t i = 0; i < 8 && i * 2 < uid_from_file.length(); i++) {
                String byteStr = uid_from_file.substring(i * 2, i * 2 + 2);
                _uid[i] = strtoul(byteStr.c_str(), NULL, 16);
            }
            continue;
        }

        // Skip header until "# Data:"
        if (!header_passed) {
            if (line.startsWith("# Data:")) { header_passed = true; }
            continue;
        }

        // Parse blocks in the format: [XX] YYYYYYYY
        if (line.startsWith("[") && line.indexOf("]") > 0) {
            int bracket_end = line.indexOf("]");
            String block_num_str = line.substring(1, bracket_end);
            String data_str = line.substring(bracket_end + 1);
            data_str.trim();
            data_str.replace(" ", "");

            // Convert block number
            uint8_t block_num = strtoul(block_num_str.c_str(), NULL, 16);

            if (block_num >= 128) continue; // Skip invalid blocks

            // Convert 8 hex characters to 4 bytes
            if (data_str.length() >= 8) {
                uint16_t offset = block_num * 4;
                for (uint8_t i = 0; i < 4; i++) {
                    String byteStr = data_str.substring(i * 2, i * 2 + 2);
                    _dump[offset + i] = strtoul(byteStr.c_str(), NULL, 16);
                }
                blocks_loaded++;
            }
        }
    }
    file.close();

    // Verify that all 128 blocks have been loaded
    if (blocks_loaded < 128) {
        displayError("Incomplete dump!");
        displayError("Loaded " + String(blocks_loaded) + "/128 blocks");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    // Successo!
    _dump_valid_from_load = true;
    _dump_valid_from_read = false;

    displaySuccess("Dump loaded successfully!");
    delay(1000);

    // Extract only the file name from the full path
    String filename = filepath;
    int lastSlash = filename.lastIndexOf('/');
    if (lastSlash >= 0) {
        filename = filename.substring(lastSlash + 1); // Remove "/BruceRFID/SRIX/"
    }

    current_state = LOAD_MODE;
    _screen_drawn = false;

    display_banner();
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);
    padprintln("File:");
    padprintln(filename); // Rewrite for long filename
    padprintln("");
    padprintln("UID: " + uid_from_file);
    padprintln("");
    padprintln("Blocks: " + String(blocks_loaded));
    padprintln("");
    tft.setTextSize(FP);

    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    padprintln("Press [OK] to open menu");
    padprintln("Select 'Write tag' to write");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    _lastReadTime = millis();
    _screen_drawn = true;
}

void SRIXTool::delayWithReturn(uint32_t ms) {
    auto tm = millis();
    while (millis() - tm < ms && !returnToMenu) { vTaskDelay(pdMS_TO_TICKS(50)); }
}

// ========== JS INTERPRETER SUPPORT ==========
SRIXTool::SRIXTool(bool headless_mode) {
    SRIX_LOG("[SRIX] Headless Constructor start");

    current_state = IDLE_MODE;
    _dump_valid_from_read = false;
    _dump_valid_from_load = false;
    _tag_read = false;
    _screen_drawn = false;
    _lastReadTime = 0;

    memset(_dump, 0, sizeof(_dump));
    memset(_uid, 0, sizeof(_uid));

    int sda_pin = bruceConfigPins.i2c_bus.sda;
    int scl_pin = bruceConfigPins.i2c_bus.scl;

    SRIX_LOG("[SRIX] I2C pins: SDA=%d, SCL=%d", sda_pin, scl_pin);

    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(100000);

#if defined(PN532_IRQ) && defined(PN532_RF_REST)
    SRIX_LOG("[SRIX] Using HW IRQ: %d, RST: %d", PN532_IRQ, PN532_RF_REST);
    nfc = new Arduino_PN532_SRIX(PN532_IRQ, PN532_RF_REST);
#else
    SRIX_LOG("[SRIX] Using POLLING mode (No IRQ pin defined)");
    nfc = new Arduino_PN532_SRIX(255, 255); // Use 255 pin
#endif

    if (nfc) {
        SRIX_LOG("[SRIX] NFC object created. Initializing...");
        if (nfc->init()) {
            SRIX_LOG("[SRIX] Init OK. Configuring...");
            nfc->setPassiveActivationRetries(0xFF);
            nfc->SRIX_init();
            SRIX_LOG("[SRIX] Ready.");
        } else {
            SRIX_LOG("[SRIX] ❌ Init FAILED! (Check wiring)");
        }
    } else {
        SRIX_LOG("[SRIX] ❌ Failed to create NFC object (Out of memory?)");
    }
}

bool SRIXTool::waitForTagHeadless(uint32_t timeout_ms) {
    if (!nfc) return false;

    uint32_t startTime = millis();
    bool rfResetDone = false;

    SRIX_LOG("[WaitHeadless] Waiting for tag... (Timeout: %lu ms)", timeout_ms);

    while ((millis() - startTime) < timeout_ms) {

        // Init_select
        if (nfc->SRIX_initiate_select()) {
            SRIX_LOG("[WaitHeadless] ✅ Tag detected!");
            return true;
        }

        // Delay for i2C Bus
        delay(100);
    }

    SRIX_LOG("[WaitHeadless] ❌ Timeout. No tag found.");
    return false;
}

String SRIXTool::read_tag_headless(int timeout_seconds) {
    if (!nfc) return "";

    uint32_t startTime = millis();

    while ((millis() - startTime) < (timeout_seconds * 1000)) {
        // Try to detect tag
        if (!nfc->SRIX_initiate_select()) {
            delay(100);
            continue;
        }

        // Tag detected! Read UID
        if (!nfc->SRIX_get_uid(_uid)) {
            delay(100);
            continue;
        }

        // Read all 128 blocks (512 bytes)
        uint8_t block[4];
        bool read_success = true;

        for (uint8_t b = 0; b < 128; b++) {
            if (!nfc->SRIX_read_block(b, block)) {
                read_success = false;
                break;
            }

            const uint16_t off = (uint16_t)b * 4;
            _dump[off + 0] = block[0];
            _dump[off + 1] = block[1];
            _dump[off + 2] = block[2];
            _dump[off + 3] = block[3];
        }

        if (!read_success) {
            delay(100);
            continue;
        }

        // Mark dump as valid
        _dump_valid_from_read = true;
        _dump_valid_from_load = false;

        // Build UID string (8 bytes with spaces)
        String uid_str = "";
        for (uint8_t i = 0; i < 8; i++) {
            if (_uid[i] < 0x10) uid_str += "0";
            uid_str += String(_uid[i], HEX);
            if (i < 7) uid_str += " ";
        }
        uid_str.toUpperCase();

        // Build data hex string (1024 hex chars = 512 bytes)
        String dump_str = "";
        for (uint16_t i = 0; i < 512; i++) {
            if (_dump[i] < 0x10) dump_str += "0";
            dump_str += String(_dump[i], HEX);
        }
        dump_str.toUpperCase();

        // Build JSON response
        String result = "{";
        result += "\"uid\":\"" + uid_str + "\",";
        result += "\"blocks\":128,";
        result += "\"size\":512,";
        result += "\"data\":\"" + dump_str + "\"";
        result += "}";

        return result;
    }

    return ""; // Timeout
}

int SRIXTool::write_tag_headless(int timeout_seconds) {

    if (!nfc) {
        SRIX_LOG("[WriteTag_Headless] ❌ NFC object is NULL");
        return -6;
    }

    if (!_dump_valid_from_read && !_dump_valid_from_load) {
        SRIX_LOG("[WriteTag_Headless] ❌ No dump loaded in memory");
        return -2;
    }

    // Wait tag headless mode
    if (!waitForTagHeadless((uint32_t)timeout_seconds * 1000UL)) {
        SRIX_LOG("[WriteTag_Headless] ❌ Timeout waiting for tag");
        return -1;
    }

    SRIX_LOG("[WriteTag_Headless] ✅ Tag detected, starting write");

    uint8_t block[4];
    uint8_t readCheck[4];

    uint8_t blocks_written = 0;
    uint8_t blocks_verified = 0;

    // Write
    for (uint8_t b = 0; b < 128; b++) {

        const uint16_t off = (uint16_t)b * 4;
        block[0] = _dump[off + 0];
        block[1] = _dump[off + 1];
        block[2] = _dump[off + 2];
        block[3] = _dump[off + 3];

        // WRITE (critical)
        if (!nfc->SRIX_write_block(b, block)) {
            SRIX_LOG("[WriteTag_Headless] ❌ WRITE FAILED at block %u", b);
            return -5; // Error
        }

        blocks_written++;
        delay(SRIX_EEPROM_WRITE_DELAY_MS);
        SRIX_LOG("[WriteTag_Headless] ✅ WRITE BLOCK: %u", blocks_written);

        // VERIFY (best-effort)
        if (nfc->SRIX_read_block(b, readCheck)) {
            if (memcmp(block, readCheck, 4) == 0) { blocks_verified++; }
        }
        if (!waitForTagHeadless(600)) {
            SRIX_LOG("[WriteTag_Headless] ❌ Tag lost at block %u", b);
            return blocks_written;
        }
    }

    SRIX_LOG(
        "[WriteTag_Headless] ✅ WRITE COMPLETE. Written=%u Verified=%u", blocks_written, blocks_verified
    );

    // Return if OK
    if (blocks_verified == 128) {
        return 0; // WRITE + VERIFY OK
    }

    return blocks_verified; // WRITE OK, VERIFY partial / skipped
}

String SRIXTool::save_file_headless(String filename) {
    if (!_dump_valid_from_read && !_dump_valid_from_load) return ""; // No data

    FS *fs;
    if (!getFsStorage(fs)) return "";

    // Create directories if needed
    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    if (!(*fs).exists("/BruceRFID/SRIX")) (*fs).mkdir("/BruceRFID/SRIX");

    // Build filepath
    String filepath = "/BruceRFID/SRIX/" + filename;
    if (!filename.endsWith(".srix")) filepath += ".srix";

    // Handle existing file
    if ((*fs).exists(filepath)) {
        int i = 1;
        String base = filepath.substring(0, filepath.length() - 5); // Remove .srix
        while ((*fs).exists(base + "_" + String(i) + ".srix")) i++;
        filepath = base + "_" + String(i) + ".srix";
    }

    // Open file for writing
    File file = (*fs).open(filepath, FILE_WRITE);
    if (!file) return "";

    // Build UID string
    String uid_str = "";
    for (uint8_t i = 0; i < 8; i++) {
        if (_uid[i] < 0x10) uid_str += "0";
        uid_str += String(_uid[i], HEX);
    }
    uid_str.toUpperCase();

    // Write header
    file.println("Filetype: Bruce SRIX Dump");
    file.println("UID: " + uid_str);
    file.println("Blocks: 128");
    file.println("Data size: 512");
    file.println("# Data:");

    // Write blocks in format: [XX] YYYYYYYY
    for (uint8_t block = 0; block < 128; block++) {
        uint16_t offset = block * 4;
        String line = "[";
        if (block < 0x10) line += "0";
        line += String(block, HEX);
        line += "] ";

        for (uint8_t i = 0; i < 4; i++) {
            if (_dump[offset + i] < 0x10) line += "0";
            line += String(_dump[offset + i], HEX);
        }
        line.toUpperCase();
        file.println(line);
    }

    file.close();
    return filepath;
}

int SRIXTool::load_file_headless(String filename) {
    if (!nfc) return -1;

    FS *fs;
    if (!getFsStorage(fs)) return -1;

    // Build filepath
    String filepath = "/BruceRFID/SRIX/" + filename;
    if (!filename.endsWith(".srix")) filepath += ".srix";

    if (!(*fs).exists(filepath)) return -2; // File not found

    // Open file
    File file = (*fs).open(filepath, FILE_READ);
    if (!file) return -1;

    // Reset buffers
    memset(_dump, 0, sizeof(_dump));
    memset(_uid, 0, sizeof(_uid));

    bool header_passed = false;
    int blocks_loaded = 0;

    // Parse file line by line
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) continue;

        // Parse UID from header
        if (line.startsWith("UID:")) {
            String uid_str = line.substring(5);
            uid_str.trim();
            uid_str.replace(" ", "");

            for (uint8_t i = 0; i < 8 && i * 2 < uid_str.length(); i++) {
                String byteStr = uid_str.substring(i * 2, i * 2 + 2);
                _uid[i] = strtoul(byteStr.c_str(), NULL, 16);
            }
            continue;
        }

        // Skip header until "# Data:"
        if (!header_passed) {
            if (line.startsWith("# Data:")) header_passed = true;
            continue;
        }

        // Parse blocks in format: [XX] YYYYYYYY
        if (line.startsWith("[") && line.indexOf("]") > 0) {
            int bracket_end = line.indexOf("]");
            String block_num_str = line.substring(1, bracket_end);
            String data_str = line.substring(bracket_end + 1);
            data_str.trim();
            data_str.replace(" ", "");

            uint8_t block_num = strtoul(block_num_str.c_str(), NULL, 16);
            if (block_num >= 128) continue; // Skip invalid blocks

            // Convert 8 hex chars to 4 bytes
            if (data_str.length() >= 8) {
                uint16_t offset = block_num * 4;
                for (uint8_t i = 0; i < 4; i++) {
                    String byteStr = data_str.substring(i * 2, i * 2 + 2);
                    _dump[offset + i] = strtoul(byteStr.c_str(), NULL, 16);
                }
                blocks_loaded++;
            }
        }
    }

    file.close();

    if (blocks_loaded < 128) return -3; // Incomplete dump

    // Mark dump as valid
    _dump_valid_from_load = true;
    _dump_valid_from_read = false;

    return 0; // Success
}

int SRIXTool::write_single_block_headless(uint8_t block_num, const uint8_t *block_data) {
    if (!nfc) {
        SRIX_LOG("[WriteBlock_Headless] ❌ NFC object is NULL");
        return -6;
    }
    if (block_num > 127) {
        SRIX_LOG("[WriteBlock_Headless] ❌ Invalid block num: %d", block_num);
        return -4;
    }
    if (!block_data) {
        SRIX_LOG("[WriteBlock_Headless] ❌ Invalid data pointer");
        return -3;
    }

    SRIX_LOG(
        "[WriteBlock_Headless] Start writing block %u. Data: %02X %02X %02X %02X",
        block_num,
        block_data[0],
        block_data[1],
        block_data[2],
        block_data[3]
    );

    // 1. Wait Tag
    // Timeout 2500ms
    if (!waitForTagHeadless(2500)) {
        SRIX_LOG("[WriteBlock_Headless] ❌ Timeout: Tag not found");
        return -1;
    }

    // 2. Tag found and selected
    SRIX_LOG("[WriteBlock_Headless] Tag ready. Sending write command...");

    if (nfc->SRIX_write_block(block_num, (uint8_t *)block_data)) {

        // 3. Wait HW write (Critical Wait)
        delay(SRIX_EEPROM_WRITE_DELAY_MS);
        SRIX_LOG("[WriteBlock_Headless] ✅ WRITE SENT OK");

        // ==============================
        // 4. VERIFY (NEED BIGGER UPGRADE)
        // ==============================
        uint8_t readBuffer[4];

        // Re-select: NEED TO IMPLEMNTAT VERIFY WRITTEN BLOCK - NOT POSSIBLE NOW
        if (!nfc->SRIX_initiate_select()) { SRIX_LOG("[WriteBlock_Headless] ⚠️ Re-select failed (ignored)"); }
        delay(10);

        if (nfc->SRIX_read_block(block_num, readBuffer)) {
            delay(5);
            if (memcmp(block_data, readBuffer, 4) == 0) {
                SRIX_LOG("[WriteBlock_Headless] ✅ VERIFY OK");
                return 0; // WRITE + VERIFY OK
            } else {
                SRIX_LOG("[WriteBlock_Headless] ⚠️ VERIFY MISMATCH (write assumed OK)");
                return 1; // WRITE OK, VERIFY FAIL
            }
        }

        SRIX_LOG("[WriteBlock_Headless] ⚠️ VERIFY SKIPPED (no RST / RF dirty)");
        return 2; // WRITE OK, VERIFY NOT POSSIBLE
    } else {
        SRIX_LOG("[WriteBlock_Headless] ❌ WRITE FAILED");
        return -5;
    }
}

// Entry point
void PN532_SRIX() { SRIXTool srix_tool; }
