#include "subghz_advanced_menu.h"

#include "core/display.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "modules/rf/rf_utils.h"
#include "subghz_advanced_engine.h"

#include <globals.h>

static SubGhzAdvancedEngine& eng = SubGhzAdvancedEngine::instance();
static String lastTxPath = "";
static bool lastTxOnSd = false;

static void showFrame(const SubGhzAdvancedFrame& f, const String& title) {
    drawMainBorderWithTitle(title);
    padprintln("Protocol: " + f.protocol_name);
    if(f.frequency_hz) padprintln("Frequency: " + String(f.frequency_hz));
    if(f.bit_count) padprintln("Bit: " + String(f.bit_count));
    if(f.key_hex.length()) padprintln("Key: " + f.key_hex);
    if(f.counter.length()) padprintln("Counter: " + f.counter);
    if(f.button.length()) padprintln("Button: " + f.button);
    if(f.serial.length()) padprintln("Serial: " + f.serial);
    if(f.raw_summary.length()) padprintln("RAW: " + f.raw_summary);
    if(f.filetype.length()) padprintln("Filetype: " + f.filetype);
    if(f.notes.length()) padprintln("Notes: " + f.notes);
    padprintln("");
    padprintln("Press any key to return");
    while(!check(AnyKeyPress)) delay(20);
    while(check(AnyKeyPress)) delay(20);
}

static bool chooseSubFile(FS*& fs, String& path) {
    fs = &LittleFS;
    options = {
        {"LittleFS", [&]() { fs = &LittleFS; }},
    };
    if(setupSdCard()) options.insert(options.begin(), {"SD Card", [&]() { fs = &SD; }});
    loopOptions(options);

    path = loopSD(*fs, true, "SUB", "/BruceRF");
    return path.length() > 0;
}

static bool getLastTxFs(FS*& fs) {
    if(lastTxOnSd) {
        if(!setupSdCard()) return false;
        fs = &SD;
        return true;
    }
    fs = &LittleFS;
    return true;
}

static void actionScanAndIdentify() {
    SubGhzAdvancedFrame frame;
    bool ok = eng.readAndDecode(bruceConfigPins.rfFreq, 10, frame);
    if(!ok) {
        displayError("No signal decoded", true);
        return;
    }
    showFrame(frame, "SubGHz Identify");
}

static void actionAnalyzeSubFile() {
    FS* fs = nullptr;
    String path = "";
    if(!chooseSubFile(fs, path)) return;

    SubGhzAdvancedFrame frame;
    if(!eng.analyzeSubFile(fs, path, frame)) {
        displayError("Unable to parse .sub", true);
        return;
    }

    showFrame(frame, "Analyze .sub");
}

static void actionAnalyzeLastTxFile() {
    if(lastTxPath.length() == 0) {
        displayInfo("No last TX file", true);
        return;
    }

    FS* fs = nullptr;
    if(!getLastTxFs(fs) || !fs->exists(lastTxPath)) {
        displayError("Last TX file unavailable", true);
        return;
    }

    SubGhzAdvancedFrame frame;
    if(!eng.analyzeSubFile(fs, lastTxPath, frame)) {
        displayError("Unable to parse last TX file", true);
        return;
    }

    showFrame(frame, "Analyze Last TX");
}

static void actionTransmitSubFile() {
    FS* fs = nullptr;
    String path = "";
    if(!chooseSubFile(fs, path)) return;

    if(!eng.transmitSubFile(fs, path, false)) {
        displayError("Unable to transmit .sub", true);
        return;
    }

    lastTxPath = path;
    lastTxOnSd = (fs == &SD);
}

static void actionTransmitLastSubFile() {
    if(lastTxPath.length() == 0) {
        displayInfo("No last TX file", true);
        return;
    }

    FS* fs = nullptr;
    if(!getLastTxFs(fs) || !fs->exists(lastTxPath)) {
        displayError("Last TX file unavailable", true);
        return;
    }

    if(!eng.transmitSubFile(fs, lastTxPath, false)) {
        displayError("Unable to transmit last file", true);
    }
}

static void actionRecent() {
    const auto& recent = eng.getRecent();
    if(recent.empty()) {
        displayInfo("No recent frames", true);
        return;
    }

    std::vector<Option> opts;
    for(size_t i = 0; i < recent.size(); i++) {
        String label = String(i + 1) + ") " + recent[i].protocol_name;
        opts.emplace_back(label, [i, &recent]() { showFrame(recent[i], "Recent"); });
    }
    addOptionToMainMenu();
    loopOptions(opts, MENU_TYPE_SUBMENU, "SubGHz Recent");
}

static void actionSettings() {
    int idx = eng.getProfile() == SubGhzAdvancedProfile::FULL ? 1 : 0;

    std::vector<Option> settings = {
        {"Profile: CORE", [&]() { eng.setProfile(SubGhzAdvancedProfile::CORE); }},
        {"Profile: FULL", [&]() { eng.setProfile(SubGhzAdvancedProfile::FULL); }},
        {"Set Frequency", []() { setMHZMenu(); }},
        {"Set Range", []() { rf_range_selection(bruceConfigPins.rfFreq); }},
    };
    addOptionToMainMenu();
    loopOptions(settings, MENU_TYPE_SUBMENU, "SubGHz Settings", idx);
}

static void actionRxMenu() {
    std::vector<Option> rx = {
        {"Scan & Identify", actionScanAndIdentify},
    };
    addOptionToMainMenu();
    loopOptions(rx, MENU_TYPE_SUBMENU, "SubGHz RX");
}

static void actionTxMenu() {
    std::vector<Option> tx = {
        {"Transmit .sub", actionTransmitSubFile},
        {"Transmit Last", actionTransmitLastSubFile},
    };
    addOptionToMainMenu();
    loopOptions(tx, MENU_TYPE_SUBMENU, "SubGHz TX");
}

static void actionAnalyzeMenu() {
    std::vector<Option> analyze = {
        {"Analyze .sub", actionAnalyzeSubFile},
        {"Analyze Last TX", actionAnalyzeLastTxFile},
    };
    addOptionToMainMenu();
    loopOptions(analyze, MENU_TYPE_SUBMENU, "SubGHz Analyze");
}

void subghz_advanced_menu() {
    eng.begin();

    options = {
        {"RX", actionRxMenu},
        {"TX", actionTxMenu},
        {"Analyze", actionAnalyzeMenu},
        {"Recent", actionRecent},
        {"Settings", actionSettings},
    };

    addOptionToMainMenu();
    String title = "SubGHz Advanced [" + eng.getProfileName() + "]";
    loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
}
