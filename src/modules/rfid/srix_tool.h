/**
 * @file srix_tool.cpp
 * @brief SRIX4K/SRIX512 Reader/Writer Tool v1.3
 * @author Senape3000
 * @info https://github.com/Senape3000/firmware/blob/main/docs_custom/SRIX/SRIX_Tool_README.md
 * @date 2026-01-01
 */

#ifndef __SRIX_TOOL_H__
#define __SRIX_TOOL_H__

// DEBUG MODE: uncomment for read the write process output from serial
// #define SRIX_DEBUG_WRITE_SIMULATION

// Uncomment to enable verbose debug output on Serial
// #define SRIX_DEBUG

// Helper macro for debug
#ifdef SRIX_DEBUG
#define SRIX_LOG(...)                                                                                        \
    Serial.printf(__VA_ARGS__);                                                                              \
    Serial.println()
#define SRIX_PRINT(...) Serial.print(__VA_ARGS__)
#else
#define SRIX_LOG(...)
#define SRIX_PRINT(...)
#endif

#include "pn532_srix.h"
#include <Arduino.h>
#include <FS.h>

class SRIXTool {
public:
    enum SRIX_State {
        IDLE_MODE,
        READ_TAG_MODE,
        WRITE_TAG_MODE,
        READ_UID_MODE,
        PN_INFO_MODE,
        SAVE_MODE,
        LOAD_MODE
    };

    SRIXTool();
    ~SRIXTool();

    void setup();
    void loop();
    // ========== JS INTERPRETER SUPPORT ==========
    // Headless constructor (no UI, no loop)
    SRIXTool(bool headless_mode);

    // Headless method for JS
    String read_tag_headless(int timeout_seconds);
    int write_tag_headless(int timeout_seconds);
    String save_file_headless(const String &filename);
    int load_file_headless(const String &filename);
    int write_single_block_headless(uint8_t block_num, const uint8_t *block_data);
    bool waitForTagHeadless(uint32_t timeout_ms);

    // Getter for data access
    uint8_t *getDump() { return _dump; }
    uint8_t *getUID() { return _uid; }
    bool isDumpValid() { return _dump_valid_from_read || _dump_valid_from_load; }
    uint8_t getBlockCount() { return _block_count; }

    // SRIX/ST25TB parts differ in size: ST25TB512/SRIX512 have 16 blocks,
    // ST25TB02K has 64, ST25TB04K/SRIX4K have 128. Reading stops at the first
    // block the tag does not answer, instead of assuming 128.
    static const uint8_t BLOCKS_MAX = 128;
    // Blocks 0-6 are the resettable OTP area (0-4) and the 32-bit counters
    // (5-6): they are not EEPROM user area and must not be written blindly.
    static const uint8_t FIRST_USER_BLOCK = 7;

private:
// PN532 for SRIX - uses IRQ/RST if board has them defined
// Devices such as T-Embed CC1101 have embedded PN532 that needs IRQ and RST pins
// If using other device, set -DPN532_IRQ=pin_num and -DPN532_RF_REST=pin_num in platformio.ini
#if defined(PN532_IRQ) && defined(PN532_RF_REST)
    Arduino_PN532_SRIX *nfc;
    bool _has_hardware_pins = true;
#else
    Arduino_PN532_SRIX *nfc;
    bool _has_hardware_pins = false;
#endif

    SRIX_State current_state;
    bool _tag_read = false;
    bool _screen_drawn = false;
    uint32_t _lastReadTime = 0;

    // RAM storage for 128 blocks (512 bytes)
    uint8_t _dump[128 * 4];
    uint8_t _block_count = 128;
    uint8_t _uid[8];
    bool _dump_valid_from_read = false;
    bool _dump_valid_from_load = false;

    // Dump editor bookkeeping
    bool _block_edited[BLOCKS_MAX] = {false};
    uint8_t _edited_count = 0;

    struct WriteStats {
        uint8_t tagBlocks = 0;
        uint16_t written = 0;
        uint16_t unchanged = 0;
        uint16_t mismatches = 0;
        uint8_t mismatchBlocks[10] = {0};
        uint8_t mismatchCount = 0;
    };

    bool readBlockRetry(uint8_t b, uint8_t *out, uint8_t attempts = 3);
    // Reads the tag until the first block that does not answer. When diffMask
    // is not null it is filled with 1 for every user block that differs from
    // _dump, and diffCount receives the number of differences.
    uint8_t scanTag(uint8_t *diffMask, uint16_t *diffCount, bool showProgress);
    // Writes every differing user block, then re-reads the tag to verify.
    // Returns 0 = verified, -1 = no tag, -2 = size mismatch, -3 = no dump,
    // -5 = write not accepted, -6 = verification failed.
    int writeDumpToTag(bool ui, int timeoutSeconds, WriteStats *st);

    void display_banner();
    void dump_tag_details();
    void dump_pn_info();

    void select_state();
    void set_state(SRIX_State state);
    bool waitForTag();
    // Waits indefinitely for a tag, refreshing the RF field periodically.
    // Returns false when the user cancels with the Back/Esc button.
    bool waitForTagCancelable(const char *title);
    void delayWithReturn(uint32_t ms);

    void read_tag();
    void write_tag();
    void read_uid();
    void show_pn_info();
    void show_main_menu();
    void save_file();
    void load_file();
    void load_file_data(FS *fs, const String &filepath);
    void verify_tag();
    void edit_dump();
    void edit_block(uint8_t b);
};

void PN532_SRIX();

#endif
