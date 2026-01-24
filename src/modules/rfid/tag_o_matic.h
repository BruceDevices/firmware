/**
 * @file tag_o_matic.h
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Read and Write RFID tags
 * @version 0.2
 * @date 2024-08-19
 */

#ifndef __TAG_O_MATIC_H__
#define __TAG_O_MATIC_H__

#include "RFIDInterface.h"
#include <set>

class TagOMatic {
public:
    enum RFID_State {
        READ_MODE,
        SCAN_MODE,
        CHECK_MODE,
        CLONE_MODE,
        CUSTOM_UID_MODE,
        WRITE_MODE,
        WRITE_NDEF_MODE,
        ERASE_MODE,
        LOAD_MODE,
        SAVE_MODE
    };

    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    TagOMatic();
    TagOMatic(RFID_State initial_state);
    ~TagOMatic();
// Headless constructor
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    TagOMatic(bool headless_mode);
#endif
    /////////////////////////////////////////////////////////////////////////////////////
    // Life Cycle and Setup
    /////////////////////////////////////////////////////////////////////////////////////
    void setup();
    void loop();
    void set_rfid_module();
// JS Support
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

    String read_tag_headless(int timeout_seconds);
    String read_uid_headless(int timeout_seconds);
    int write_tag_headless(int timeout_seconds);
    String save_file_headless(String filename);
    int load_file_headless(String filename);
    RFIDInterface *getRFIDInterface() { return _rfid; } // Controlled Access Getter
#endif

private:
    RFIDInterface *_rfid;
    RFID_State _initial_state;
    bool _read_uid = false;
    bool _ndef_created = false;
    uint32_t _lastReadTime = 0;
    RFID_State current_state;
    std::set<String> _scanned_set;
    std::vector<String> _scanned_tags;
    String _sourceUID;
    String _sourcePages;

    /////////////////////////////////////////////////////////////////////////////////////
    // Display functions
    /////////////////////////////////////////////////////////////////////////////////////
    void display_banner();
    void dump_card_details();
    void dump_check_details();
    void dump_ndef_details();
    void dump_scan_results();

    /////////////////////////////////////////////////////////////////////////////////////
    // State management
    /////////////////////////////////////////////////////////////////////////////////////
    void select_state();
    void set_state(RFID_State state);
    void delayWithReturn(uint32_t ms);

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    void read_card();
    void scan_cards();
    void check_card();
    void write_custom_uid();
    void clone_card();
    void erase_card();
    void write_data();
    void write_ndef_data();
    void save_file();
    void save_scan_result();
    void load_file();

    /////////////////////////////////////////////////////////////////////////////////////
    // NDEF
    /////////////////////////////////////////////////////////////////////////////////////
    void create_ndef_message();
    void create_ndef_url();
    void create_ndef_text();
};

#endif
