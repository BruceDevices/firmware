/**
 * @file ir_read.h
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Read Infrared signals
 * @version 0.1
 * @date 2024-07-17
 */

#include <IRrecv.h>
#include <globals.h>

class IrRead {
public:
    // IRrecv irrecv = IRrecv(bruceConfigPins.irRx);
    IRrecv irrecv = IRrecv(bruceConfigPins.irRx, SAFE_STACK_BUFFER_SIZE / 2, 50);

    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    IrRead(bool headless_mode = false, bool raw_mode = false);

    ///////////////////////////////////////////////////////////////////////////////////
    // Arduino Life Cycle
    ///////////////////////////////////////////////////////////////////////////////////
    void setup();
    void loop();
    void quickLoop();

    String loop_headless(int max_loops);

private:
    bool _read_signal = false;
    decode_results results;
    uint16_t *rawcode;
    uint16_t raw_data_len;
    int signals_read = 0;
    int button_pos = 0;
    String strDeviceContent = "";
    bool headless = false;
    bool raw = false;

    /////////////////////////////////////////////////////////////////////////////////////
    // Display functions
    /////////////////////////////////////////////////////////////////////////////////////
    void cls();
    void display_banner();
    void display_btn_options();

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    void begin();
    void read_signal();
    void save_device();
    void save_signal();
    void discard_signal();
    void append_to_file_str(String btn_name);
    bool write_file(String filename, FS *fs);
    String parse_raw_signal();
    String parse_state_signal();
    /////////////////////////////////////////////////////////////////////////////////////
    // Quick Remotes
    /////////////////////////////////////////////////////////////////////////////////////
    std::vector<String> quickButtonsTV = {"POWER", "UP",   "DOWN", "LEFT",  "RIGHT", "OK",       "SOURCES",
                                          "VOL+",  "VOL-", "CHA+", "CHA-",  "MUTE",  "SETTINGS", "NETFLIX",
                                          "HOME",  "BACK", "EXIT", "SMART", "1",     "2",        "3",
                                          "4",     "5",    "6",    "7",     "8",     "9",        "0"};
    std::vector<String> quickButtonsAC = {
        "POWER", "TEMP+", "TEMP-", "SPEED", "SWING", "SWING+", "SWING-", "JET", "UP", "DOWN", "MODE"
    };
    std::vector<String> quickButtonsSOUND = {"POWER",    "UP",      "DOWN", "LEFT",    "RIGHT",
                                             "OK",       "SOURCES", "VOL+", "VOL-",    "MUTE",
                                             "SETTINGS", "BACK",    "EQ",   "REC",     "PLAY/PAUSE",
                                             "STOP",     "NEXT",    "PREV", "SHUFFLE", "REPEAT"};
    std::vector<String> &quickButtons = quickButtonsTV;
};
