/**
 * @file srix_tool.cpp
 * @brief SRIX4K/SRIX512 Reader/Writer Tool v1.2
 * @author Senape3000
 * @info https://github.com/Senape3000/firmware/blob/main/docs_custom/SRIX/SRIX_Tool_README.md
 * @date 2026-01-01
 */

#ifndef __SRIX_TOOL_H__
#define __SRIX_TOOL_H__

// DEBUG MODE: uncomment for read the write process output from serial
// #define SRIX_DEBUG_WRITE_SIMULATION

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
    uint8_t _uid[8];
    bool _dump_valid_from_read = false;
    bool _dump_valid_from_load = false;

    void display_banner();
    void dump_tag_details();
    void dump_pn_info();

    void select_state();
    void set_state(SRIX_State state);
    bool waitForTag();
    void delayWithReturn(uint32_t ms);

    void read_tag();
    void write_tag();
    void read_uid();
    void show_pn_info();
    void show_main_menu();
    void save_file();
    void load_file();
    void load_file_data(FS *fs, String filepath);
};

void PN532_SRIX();

#endif
