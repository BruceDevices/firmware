#ifndef __RF_SCAN_H__
#define __RF_SCAN_H__

#include "protocols/rf_decoder.h"
#include "rf_utils.h"
#include "structs.h"

#define _MAX_TRIES 5

class RFScan {
public:
    enum RFMenuOption {
        REPLAY,
        REPLAY_RAW,
        SAVE,
        SAVE_RAW,
        RESET,
        RANGE,
        THRESHOLD,
        CLOSE_MENU,
        MAIN_MENU,
    };

    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    RFScan();
    ~RFScan();

    /////////////////////////////////////////////////////////////////////////////////////
    // Life Cycle
    /////////////////////////////////////////////////////////////////////////////////////
    void setup();
    void loop();

private:
    RfRxSession _rx;
    RfCodes received;
    String title = "RF Scan Copy";
    bool restartScan = false;
    bool ReadRAW = true;
    bool codesOnly = false;
    bool autoSave = false;
    char hexString[64];
    int signals = 0;
    float frequency = 0.f;
    uint8_t _try = 0;
    FreqFound _freqs[_MAX_TRIES]; // get the best RSSI out of 5 tries
    int idx = range_limits[bruceConfigPins.rfScanRange][0];
    float found_freq = 0.f;
    int rssi = -80;
    int rssiThreshold = -65;
    uint64_t lastSavedKey = 0;

    /////////////////////////////////////////////////////////////////////////////////////
    // State management
    /////////////////////////////////////////////////////////////////////////////////////
    void select_menu_option();
    void set_option(RFMenuOption option);

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    void decode_signal(const std::vector<int> &durations);
    void read_raw(const std::vector<int> &durations);
    void replay_signal(bool asRaw = false);
    void save_signal(bool asRaw = false);
    void reset_signals();
    void set_threshold();
    // void set_range(); // Using similar function from rf_utils.h

    /////////////////////////////////////////////////////////////////////////////////////
    // Utils
    /////////////////////////////////////////////////////////////////////////////////////
    void enable_receive();
    void init_freqs();
    bool fast_scan();
};

void display_info(
    RfCodes received, int signals, bool ReadRAW = false, bool codesOnly = false, bool autoSave = false,
    String title = "", bool headless = false
);
void display_signal_data(RfCodes received, bool headless = false);

bool rfSaveSignal(float frequency, RfCodes codes, bool raw, char *key, bool autoSave = false);

String rf_scan(float start_freq, float stop_freq, int max_loops = -1);
String rfReceiveSignal(float frequency = 0, int max_loops = -1, bool raw = false, bool headless = false);

#endif
