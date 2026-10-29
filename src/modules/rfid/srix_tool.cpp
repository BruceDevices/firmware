/**
 * @file srix_tool.cpp
 * @brief SRIX4K/SRIX512 Reader/Writer Tool v1.3
 * @author Senape3000
 * @info https://github.com/Senape3000/firmware/blob/main/docs_custom/SRIX/SRIX_Tool_README.md
 * @date 2026-01-01
 */
#include "srix_tool.h"
#include "core/bus_HAL.h"
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

SRIXTool::~SRIXTool() {
    delete nfc;
    releaseI2CBus();
}

void SRIXTool::setup() {
    drawMainBorderWithTitle("SRIX TOOL");
    padprintln("");
    padprintln("Initializing I2C...");

    // Init I2C
    TwoWire *Wire = acquireI2CBus();
    Wire->setClock(100000);

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
    nfc->setWire(Wire);

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

    padprintln("Configuring ISO14443-B...");
    if (!nfc->SRIX_prepareRead()) {
        displayError("RF config failed!", true);
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
    delay(1000);
#ifdef T_EMBED_1101
    displayError("T-Embed detected!", false);
    delay(1000);
    displayError("Read Menu INFO!", true);
#endif
    displaySuccess("PN532-SRIX ready!");
    delay(1000);

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

    if (isDumpValid()) {
        options.emplace_back(" -Save dump", [this]() { set_state(SAVE_MODE); });
        options.emplace_back(" -Write to tag", [this]() { set_state(WRITE_TAG_MODE); });
        options.emplace_back(" -Verify tag", [this]() {
            verify_tag();
            _screen_drawn = false;
        });
        options.emplace_back(" -View/Edit dump", [this]() {
            edit_dump();
            _screen_drawn = false;
        });
    }
    options.emplace_back("Read UID", [this]() { set_state(READ_UID_MODE); });
    options.emplace_back("Load dump", [this]() { set_state(LOAD_MODE); });
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
#ifdef T_EMBED_1101
    padprintln("- !!! - T-Embed CC1101 detected - !!!");
    padprintln("- !!! - Antenna too Weak for SRIX - !!!");
    padprintln("");
#endif
    padprintln("Features:");
    padprintln("- Read/Write tag (16/64/128 blocks)");
    padprintln("- Verify tag against dump");
    padprintln("- View/Edit dump blocks");
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

    // Same RF preparation the kpn532/ST25TB tools do before touching a tag:
    // field cycle + ISO14443-B registers + longer timeouts. Without it the
    // PN532 reads weakly coupled ST25TB parts much worse.
    _lastReadTime = 0; // bypass the 2s throttle in waitForTag()
    nfc->SRIX_prepareRead();

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

    // Read blocks until the tag stops answering. ST25TB512/SRIX512 have 16
    // blocks, ST25TB02K has 64 and ST25TB04K/SRIX4K have 128, so reaching the
    // end of memory is a normal stop condition, not an error.
    //
    // Do not probe past the end here to guess the size: a failed block read
    // leaves the PN532/tag exchange out of sync and the following block 0 read
    // fails ("Read failed!"). The bar is scaled to 128 while reading and then
    // filled to 100% when the end of memory is reached.
    padprintln("Reading blocks...");
    padprint("Please Wait");
    uint8_t block[4];
    uint8_t blocks_read = 0;
    const uint8_t total = BLOCKS_MAX;

    for (uint8_t b = 0; b < BLOCKS_MAX; b++) {
        if (!readBlockRetry(b, block)) break;

        const uint16_t off = (uint16_t)b * 4;
        _dump[off + 0] = block[0];
        _dump[off + 1] = block[1];
        _dump[off + 2] = block[2];
        _dump[off + 3] = block[3];

        blocks_read = b + 1;

        // Progress indicator
        progressHandler(blocks_read, total, "Reading data blocks");
    }

    if (blocks_read == 0) {
        // Recovery: a failed exchange can leave the tag out of sync. Retry once
        // with the tuned RF config (field restart), then once with the PN532
        // defaults in case the CIU register writes are not accepted by this
        // module.
        for (uint8_t attempt = 0; attempt < 2 && blocks_read == 0; attempt++) {
            delay(100);
            nfc->SRIX_prepareRead(attempt == 0);
            if (!waitForTag()) break;

            for (uint8_t b = 0; b < BLOCKS_MAX; b++) {
                if (!readBlockRetry(b, block)) break;

                const uint16_t off = (uint16_t)b * 4;
                _dump[off + 0] = block[0];
                _dump[off + 1] = block[1];
                _dump[off + 2] = block[2];
                _dump[off + 3] = block[3];

                blocks_read = b + 1;
                progressHandler(blocks_read, total, "Reading data blocks");
            }
        }
    }

    if (blocks_read > 0) {
        // The read is complete at the end of memory: show a full bar.
        progressHandler(blocks_read, blocks_read, "Read complete");
    }

    if (blocks_read == 0) {
        displayError("Read failed!");
        delay(2000);
        set_state(READ_TAG_MODE);
        return;
    }

    _block_count = blocks_read;
    _dump_valid_from_read = true;
    _dump_valid_from_load = false;
    memset(_block_edited, 0, sizeof(_block_edited));
    _edited_count = 0;
    padprintln("");
    padprintln("");
    displaySuccess("Read " + String(_block_count) + " blocks!");
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
    if (!isDumpValid()) {
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
    for (uint8_t b = 0; b < _block_count; b++) {
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
    Serial.printf("Total: %d blocks (%d bytes)\n", _block_count, _block_count * 4);
    Serial.println("===========================================\n");

    displaySuccess("Simulation complete!");
    displayInfo("Check Serial Monitor");
    delay(4000);
    set_state(IDLE_MODE);
    return;

#else

    display_banner();
    padprintln("Place the tag on the reader");
    padprintln("");
    padprint("Please Wait");

    WriteStats st;
    int result = writeDumpToTag(true, 0, &st);

    padprintln("");
    padprintln("");

    switch (result) {
        case 0:
            padprintln("Written: " + String(st.written));
            padprintln("Unchanged: " + String(st.unchanged));
            padprintln("OTP/counter blocks skipped: " + String((int)FIRST_USER_BLOCK));
            displaySuccess("Write verified!");
            break;
        case -1:
            displayError("No tag detected!");
            break;
        case -2:
            padprintln("Dump: " + String(_block_count) + " blocks");
            padprintln("Tag: " + String(st.tagBlocks) + " blocks");
            displayError("Size mismatch, nothing written");
            break;
        case -6:
            padprintln("Written: " + String(st.written));
            padprintln("Mismatched blocks: " + String(st.mismatches));
            if (st.mismatchCount > 0) {
                String list = "";
                for (uint8_t i = 0; i < st.mismatchCount; i++) {
                    if (i) list += ",";
                    list += String(st.mismatchBlocks[i]);
                }
                if (st.mismatches > st.mismatchCount) list += ",...";
                padprintln("Blocks: " + list);
            }
            displayError("Verification failed");
            break;
        default:
            displayError("Write failed!");
            break;
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
    if (!isDumpValid()) {
        displayError("No data in memory!");
        displayError("Read or load a dump first.");
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
        set_state(IDLE_MODE); // Back to IDLE
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
    file.println("Blocks: " + String(_block_count));
    file.println("Data size: " + String(_block_count * 4));
    file.println("# Data:");

    // Write all blocks in [XX] format YYYYYYYY
    for (uint8_t block = 0; block < _block_count; block++) {
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

void SRIXTool::load_file_data(FS *fs, const String &filepath) {

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
    int max_block = -1;
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
                if ((int)block_num > max_block) max_block = block_num;
            }
        }
    }
    file.close();

    // Verify that the dump is complete and contiguous. Any tag size is valid
    // (16, 64 or 128 blocks), only gaps in the block numbers are rejected.
    if (blocks_loaded < 1 || (max_block + 1) != blocks_loaded) {
        displayError("Incomplete dump!");
        displayError("Loaded " + String(blocks_loaded) + " blocks");
        delay(2000);
        set_state(IDLE_MODE);
        return;
    }

    _block_count = (uint8_t)(max_block + 1);

    // Successo!
    _dump_valid_from_load = true;
    _dump_valid_from_read = false;
    memset(_block_edited, 0, sizeof(_block_edited));
    _edited_count = 0;

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

    SRIX_LOG(
        "[SRIX] I2C pins: SDA=%d, SCL=%d", bruceConfigPins.i2c_bus.sda, bruceConfigPins.i2c_bus.scl
    );

    TwoWire *Wire = acquireI2CBus();
    Wire->setClock(100000);

#if defined(PN532_IRQ) && defined(PN532_RF_REST)
    SRIX_LOG("[SRIX] Using HW IRQ: %d, RST: %d", PN532_IRQ, PN532_RF_REST);
    nfc = new Arduino_PN532_SRIX(PN532_IRQ, PN532_RF_REST);
#else
    SRIX_LOG("[SRIX] Using POLLING mode (No IRQ pin defined)");
    nfc = new Arduino_PN532_SRIX(255, 255); // Use 255 pin
#endif

    if (nfc) {
        nfc->setWire(Wire);
        SRIX_LOG("[SRIX] NFC object created. Initializing...");
        if (nfc->init()) {
            SRIX_LOG("[SRIX] Init OK. Configuring...");
            nfc->setPassiveActivationRetries(0xFF);
            nfc->SRIX_prepareRead();
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

        // Read blocks until the tag stops answering (16/64/128 depending on
        // the part). End of memory is a valid result, not a failure.
        uint8_t block[4];
        uint8_t blocks_read = 0;

        for (uint8_t b = 0; b < BLOCKS_MAX; b++) {
            bool ok = false;
            for (uint8_t attempt = 0; attempt < 3 && !ok; attempt++) {
                ok = nfc->SRIX_read_block(b, block);
                if (!ok && attempt < 2) {
                    nfc->SRIX_initiate_select();
                    delay(20);
                }
            }
            if (!ok) break;

            const uint16_t off = (uint16_t)b * 4;
            _dump[off + 0] = block[0];
            _dump[off + 1] = block[1];
            _dump[off + 2] = block[2];
            _dump[off + 3] = block[3];
            blocks_read = b + 1;
        }

        if (blocks_read == 0) {
            delay(100);
            continue;
        }

        _block_count = blocks_read;

        // Mark dump as valid
        _dump_valid_from_read = true;
        _dump_valid_from_load = false;
        memset(_block_edited, 0, sizeof(_block_edited));
        _edited_count = 0;

        // Build UID string (8 bytes with spaces)
        String uid_str = "";
        for (uint8_t i = 0; i < 8; i++) {
            if (_uid[i] < 0x10) uid_str += "0";
            uid_str += String(_uid[i], HEX);
            if (i < 7) uid_str += " ";
        }
        uid_str.toUpperCase();

        // Build data hex string (4 hex chars per byte)
        String dump_str = "";
        for (uint16_t i = 0; i < (uint16_t)_block_count * 4; i++) {
            if (_dump[i] < 0x10) dump_str += "0";
            dump_str += String(_dump[i], HEX);
        }
        dump_str.toUpperCase();

        // Build JSON response
        String result = "{";
        result += "\"uid\":\"" + uid_str + "\",";
        result += "\"blocks\":" + String(_block_count) + ",";
        result += "\"size\":" + String(_block_count * 4) + ",";
        result += "\"data\":\"" + dump_str + "\"";
        result += "}";

        return result;
    }

    return ""; // Timeout
}

int SRIXTool::write_tag_headless(int timeout_seconds) {

    if (!nfc) {
        SRIX_LOG("[WriteTag_Headless] ❌ NFC object is NULL");
        return -5;
    }

    if (!isDumpValid()) {
        SRIX_LOG("[WriteTag_Headless] ❌ No dump loaded in memory");
        return -3;
    }

    WriteStats st;
    int result = writeDumpToTag(false, timeout_seconds, &st);

    SRIX_LOG(
        "[WriteTag_Headless] result=%d tagBlocks=%u written=%u unchanged=%u mismatches=%u",
        result,
        st.tagBlocks,
        st.written,
        st.unchanged,
        st.mismatches
    );

    return result;
}

String SRIXTool::save_file_headless(const String &filename) {
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
    file.println("Blocks: " + String(_block_count));
    file.println("Data size: " + String(_block_count * 4));
    file.println("# Data:");

    // Write blocks in format: [XX] YYYYYYYY
    for (uint8_t block = 0; block < _block_count; block++) {
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

int SRIXTool::load_file_headless(const String &filename) {
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
    int max_block = -1;

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
                if ((int)block_num > max_block) max_block = block_num;
            }
        }
    }

    file.close();

    // Any tag size is valid (16/64/128 blocks), only gaps are rejected
    if (blocks_loaded < 1 || (max_block + 1) != blocks_loaded) return -3; // Incomplete dump

    _block_count = (uint8_t)(max_block + 1);

    // Mark dump as valid
    _dump_valid_from_load = true;
    _dump_valid_from_read = false;
    memset(_block_edited, 0, sizeof(_block_edited));
    _edited_count = 0;

    return 0; // Success
}

int SRIXTool::write_single_block_headless(uint8_t block_num, const uint8_t *block_data) {
    if (!nfc) {
        SRIX_LOG("[WriteBlock_Headless] ❌ NFC object is NULL");
        return -6;
    }
    if (block_num >= BLOCKS_MAX) {
        SRIX_LOG("[WriteBlock_Headless] ❌ Invalid block num: %d", block_num);
        return -4;
    }
    // Blocks 0-6 are the OTP area and the 32-bit counters: writing them either
    // fails or permanently clears bits, so refuse instead of corrupting them.
    if (block_num < FIRST_USER_BLOCK) {
        SRIX_LOG("[WriteBlock_Headless] ❌ Block %u is OTP/counter, not writable", block_num);
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

// ========== SHARED READ / WRITE / VERIFY HELPERS ==========

bool SRIXTool::readBlockRetry(uint8_t b, uint8_t *out, uint8_t attempts) {
    for (uint8_t attempt = 0; attempt < attempts; attempt++) {
        if (nfc->SRIX_read_block(b, out)) return true;
        // A single failed read can be a coupling dropout with the weak internal
        // antenna: re-select and retry before accepting that the block is gone.
        if (attempt + 1 < attempts) {
            nfc->SRIX_initiate_select();
            delay(20);
        }
    }
    return false;
}

uint8_t SRIXTool::scanTag(uint8_t *diffMask, uint16_t *diffCount, bool showProgress) {
    uint8_t block[4];
    uint8_t blocks = 0;
    uint8_t total = (_block_count > 0) ? _block_count : BLOCKS_MAX;

    if (diffCount) *diffCount = 0;
    if (diffMask) memset(diffMask, 0, BLOCKS_MAX);

    for (uint8_t b = 0; b < BLOCKS_MAX; b++) {
        if (!readBlockRetry(b, block)) break;

        blocks = b + 1;
        if (blocks > total) total = blocks;

        if (diffMask && b >= FIRST_USER_BLOCK && b < _block_count) {
            if (memcmp(block, &_dump[b * 4], 4) != 0) {
                diffMask[b] = 1;
                if (diffCount) (*diffCount)++;
            }
        }

        if (showProgress) progressHandler(blocks, total, "Reading tag");
    }

    if (showProgress && blocks > 0) progressHandler(blocks, blocks, "Read complete");

    return blocks;
}

int SRIXTool::writeDumpToTag(bool ui, int timeoutSeconds, WriteStats *st) {
    if (st) *st = WriteStats();
    if (!nfc) return -5;
    if (!isDumpValid()) return -3;

    // waitForTag() ignores calls for 2s after a read; bypass that so a write
    // right after reading does not report a false "no tag".
    if (ui) _lastReadTime = 0;
    nfc->SRIX_prepareRead();
    bool tagOk = ui ? waitForTag() : waitForTagHeadless((uint32_t)timeoutSeconds * 1000UL);
    if (!tagOk) return -1;

    // 1. Size + difference scan: also proves which blocks actually need writing.
    uint8_t diffMask[BLOCKS_MAX];
    uint16_t diffCount = 0;
    uint8_t tagBlocks = scanTag(diffMask, &diffCount, ui);
    if (st) st->tagBlocks = tagBlocks;
    if (tagBlocks == 0) return -1;
    if (tagBlocks != _block_count) return -2;

    if (diffCount == 0) {
        if (st) st->unchanged = (_block_count > FIRST_USER_BLOCK) ? (_block_count - FIRST_USER_BLOCK) : 0;
        SRIX_LOG("[Write] Tag already matches the dump");
        return 0;
    }

    // The scan above ends on a failed read (the first block past the end of
    // memory). Restart the field and select the tag again so the write
    // sequence does not inherit a desynchronised exchange.
    nfc->SRIX_prepareRead();
    if (ui) _lastReadTime = 0;
    bool reselectOk = ui ? waitForTag() : waitForTagHeadless((uint32_t)timeoutSeconds * 1000UL);
    if (!reselectOk) return -1;

    // 2. Write only the blocks that differ; never touch OTP/counters (0-6).
    const uint8_t writable = (_block_count > FIRST_USER_BLOCK) ? (_block_count - FIRST_USER_BLOCK) : 1;
    for (uint8_t b = FIRST_USER_BLOCK; b < _block_count; b++) {
        if (!diffMask[b]) {
            if (st) st->unchanged++;
            if (ui) progressHandler(b - FIRST_USER_BLOCK + 1, writable, "Writing data blocks");
            continue;
        }

        bool accepted = false;
        for (uint8_t attempt = 0; attempt < 3 && !accepted; attempt++) {
            int status = nfc->SRIX_write_block_status(b, &_dump[b * 4]);
            delay(SRIX_EEPROM_WRITE_DELAY_MS);
            if (status >= 0) {
                accepted = true;
            } else {
                nfc->SRIX_initiate_select();
                delay(20);
            }
        }

        if (accepted) {
            if (st) st->written++;
        }
        SRIX_LOG("[Write] block %u accepted=%d", b, (int)accepted);

        if (ui) progressHandler(b - FIRST_USER_BLOCK + 1, writable, "Writing data blocks");
    }

    // 3. Verify by re-reading the whole tag: this is the authoritative result.
    uint8_t afterMask[BLOCKS_MAX];
    uint16_t mismatches = 0;
    uint8_t afterBlocks = scanTag(afterMask, &mismatches, ui);
    if (st) st->mismatches = mismatches;

    if (afterBlocks != _block_count) {
        // Tag lost or unreadable during verification: report every block we wrote.
        if (st) {
            st->mismatchCount = 0;
            for (uint8_t b = FIRST_USER_BLOCK; b < _block_count && st->mismatchCount < 10; b++) {
                if (diffMask[b]) st->mismatchBlocks[st->mismatchCount++] = b;
            }
        }
        return -6;
    }

    if (st && mismatches > 0) {
        for (uint8_t b = FIRST_USER_BLOCK; b < _block_count && st->mismatchCount < 10; b++) {
            if (afterMask[b]) st->mismatchBlocks[st->mismatchCount++] = b;
        }
    }

    SRIX_LOG(
        "[Write] done written=%u unchanged=%u mismatches=%u",
        st ? st->written : 0,
        st ? st->unchanged : 0,
        mismatches
    );

    return (mismatches == 0) ? 0 : -6;
}

bool SRIXTool::waitForTagCancelable(const char *title) {
    drawMainBorderWithTitle(title);
    tft.setTextSize(FP);
    padprintln("");
    padprintln("Place the tag on the reader");
    padprintln("");
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    padprintln("Press [BACK] to cancel");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    unsigned long start = millis();
    unsigned long lastDraw = 0;
    uint8_t misses = 0;

    while (true) {
        if (check(EscPress)) return false; // user cancelled
        if (nfc->SRIX_initiate_select()) return true;

        // Every ~5 s without a tag, restart the RF field so a stale exchange
        // does not keep the tag from being detected.
        if (++misses >= 50) {
            misses = 0;
            nfc->SRIX_prepareRead();
        }

        if (millis() - lastDraw > 500) {
            lastDraw = millis();
            int y = tftHeight - 40;
            tft.fillRect(6, y, tftWidth - 12, 24, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
            tft.setCursor(8, y + 4);
            tft.print("Waiting " + String((millis() - start) / 1000) + "s");
        }

        delay(50);
    }
}

void SRIXTool::verify_tag() {
    if (!isDumpValid()) {
        displayError("No dump in memory!");
        delay(1500);
        return;
    }

    _lastReadTime = 0; // bypass the 2s throttle in waitForTag()
    nfc->SRIX_prepareRead();

    // Wait for the tag without timing out: verify is a deliberate action, so
    // the user decides when to give up (Back button).
    if (!waitForTagCancelable("SRIX VERIFY")) return;

    uint8_t mask[BLOCKS_MAX];
    uint16_t mismatches = 0;
    uint8_t tagBlocks = scanTag(mask, &mismatches, true);

    drawMainBorderWithTitle("SRIX VERIFY");
    tft.setTextSize(FP);
    padprintln("");

    if (tagBlocks == 0) {
        displayError("No tag detected!");
        delayWithReturn(2500);
        return;
    }

    padprintln("Tag:  " + String((int)tagBlocks) + " blocks");
    padprintln("Dump: " + String((int)_block_count) + " blocks");
    padprintln("");

    if (tagBlocks != _block_count) {
        displayError("Size mismatch!");
        padprintln("");
        padprintln("Nothing to compare.");
    } else if (mismatches == 0) {
        displaySuccess("All user blocks match!");
        padprintln("");
        padprintln("System blocks 0-6 not compared.");
    } else {
        String list = "";
        uint8_t shown = 0;
        for (uint8_t b = FIRST_USER_BLOCK; b < _block_count && shown < 10; b++) {
            if (!mask[b]) continue;
            if (shown) list += ",";
            list += String((int)b);
            shown++;
        }
        if (mismatches > shown) list += ",...";

        displayError("Mismatch: " + String((int)mismatches) + " blocks");
        padprintln("");
        padprintln("Blocks: " + list);
        padprintln("");
        padprintln("System blocks 0-6 not compared.");
    }

    delayWithReturn(4000);
}

// ========== DUMP EDITOR ==========

void SRIXTool::edit_block(uint8_t b) {
    String current = "";
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t v = _dump[b * 4 + i];
        if (v < 0x10) current += "0";
        current += String(v, HEX);
    }
    current.toUpperCase();

    // System area: resettable OTP (blocks 0-4) and 32-bit counters (5-6).
    // Still viewable so the whole dump can be scrolled, but not editable.
    if (b < FIRST_USER_BLOCK) {
        drawMainBorderWithTitle("SRIX DUMP");
        tft.setTextSize(FP);
        padprintln("");
        padprintln("Block " + String((int)b) + ": " + current);
        padprintln("");
        padprintln(b < 5 ? "Resettable OTP bit area." : "32-bit count-down counter.");
        padprintln("Read only, never written.");
        delayWithReturn(2500);
        return;
    }

    String input = hex_keyboard(current, 8, "Block " + String((int)b) + " (8 hex)");
    if (input == "\x1B") return; // cancelled

    input.trim();
    input.replace(" ", "");
    input.toUpperCase();

    if (input.length() != 8) {
        displayError("Need 8 hex chars");
        delay(1500);
        return;
    }
    for (uint8_t i = 0; i < 8; i++) {
        if (!isxdigit((unsigned char)input[i])) {
            displayError("Invalid hex value");
            delay(1500);
            return;
        }
    }

    for (uint8_t i = 0; i < 4; i++) {
        String pair = input.substring(i * 2, i * 2 + 2);
        _dump[b * 4 + i] = (uint8_t)strtoul(pair.c_str(), NULL, 16);
    }

    if (!_block_edited[b]) {
        _block_edited[b] = true;
        _edited_count++;
    }

    SRIX_LOG("[Edit] block %u = %s", b, input.c_str());
}

void SRIXTool::edit_dump() {
    if (!isDumpValid()) {
        displayError("No dump in memory!");
        delay(1500);
        return;
    }

    bool saveRequested = false;
    bool exitEditor = false;

    while (!exitEditor && !saveRequested) {
        std::vector<Option> opts;
        opts.reserve(_block_count + 2);

        for (uint8_t b = 0; b < _block_count; b++) {
            String label = "B";
            if (b < 100) label += "0";
            if (b < 10) label += "0";
            label += String((int)b) + " ";
            for (uint8_t i = 0; i < 4; i++) {
                if (_dump[b * 4 + i] < 0x10) label += "0";
                label += String(_dump[b * 4 + i], HEX);
            }
            label.toUpperCase();
            if (b < FIRST_USER_BLOCK) {
                label += " sys";
            } else if (_block_edited[b]) {
                label += " *";
            }

            Option opt(label, [this, b]() { edit_block(b); });
            opts.push_back(opt);
        }

        opts.emplace_back("Save dump", [&saveRequested]() { saveRequested = true; });
        opts.emplace_back("Back", [&exitEditor]() { exitEditor = true; });

        int sel = loopOptions(opts, MENU_TYPE_SUBMENU, "SRIX DUMP");
        if (sel < 0) break; // long-press Back exits the editor
    }

    if (saveRequested) {
        save_file();
        _screen_drawn = false;
        return;
    }

    drawMainBorderWithTitle("SRIX DUMP");
    tft.setTextSize(FP);
    padprintln("");
    if (_edited_count > 0) {
        padprintln("Edited blocks: " + String((int)_edited_count));
        padprintln("Save or write to apply.");
    } else {
        padprintln("No changes made.");
    }
    delayWithReturn(1500);
    _screen_drawn = false;
}

// Entry point
void PN532_SRIX() { SRIXTool srix_tool; }
