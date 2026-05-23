#include "subghz_advanced_menu.h"

#include "core/display.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "modules/rf/rf_scan.h"
#include "modules/rf/rf_send.h"
#include "modules/rf/rf_utils.h"
#include "subghz_advanced_engine.h"

#include <globals.h>

static SubGhzAdvancedEngine& eng = SubGhzAdvancedEngine::instance();
static String lastTxPath = "";
static bool lastTxOnSd = false;
static bool hasLastCapturedFrame = false;
static bool hasLastCapturedText = false;
static SubGhzAdvancedFrame lastCapturedFrame;
static String lastCapturedText = "";

static uint64_t parseHexU64(String value) {
    value.trim();
    value.replace(" ", "");
    if(value.startsWith("0x") || value.startsWith("0X")) value = value.substring(2);
    if(value.length() == 0) return 0;
    return strtoull(value.c_str(), NULL, 16);
}

static uint32_t parseFlexibleU32(String value) {
    value.trim();
    if(value.length() == 0) return 0;

    bool asHex = value.startsWith("0x") || value.startsWith("0X");
    if(!asHex) {
        for(size_t i = 0; i < value.length(); i++) {
            char c = value[i];
            if((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
                asHex = true;
                break;
            }
        }
    }

    if(asHex && (value.startsWith("0x") || value.startsWith("0X"))) value = value.substring(2);
    return strtoul(value.c_str(), NULL, asHex ? 16 : 10);
}

static String extractNoteValue(const String& notes, const String& key) {
    int start = notes.indexOf(key);
    if(start < 0) return "";
    start += key.length();
    while(start < (int)notes.length() && notes[start] == ' ') start++;

    int end = notes.indexOf(';', start);
    if(end < 0) end = notes.length();
    String out = notes.substring(start, end);
    out.trim();
    return out;
}

static String extractManufacturerFromNotes(const String& notes) {
    String mf = extractNoteValue(notes, "MF:");
    if(mf.length()) return mf;
    return extractNoteValue(notes, "Manufacturer=");
}

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

static bool transmitCaptureText(const String& capture, bool hideDefaultUI = false) {
    if(capture.length() == 0) return false;

    const char* tmpPath = "/.subghz_adv_scan_copy_tmp.sub";
    File out = LittleFS.open(tmpPath, FILE_WRITE, true);
    if(!out) return false;
    out.print(capture);
    out.close();

    RfCodes code{};
    bool ok = readSubFile(&LittleFS, tmpPath, code) && txSubFile(code, hideDefaultUI);
    LittleFS.remove(tmpPath);
    return ok;
}

static bool captureAndDecodeAdvanced(int timeoutSec = 10) {
    String capture = RCSwitch_Read(bruceConfigPins.rfFreq, timeoutSec, true, true);
    if(capture.length() == 0) return false;

    SubGhzAdvancedFrame frame = eng.analyzeSubFileText(capture, "scan-copy");
    if(!frame.valid) return false;

    hasLastCapturedFrame = true;
    hasLastCapturedText = true;
    lastCapturedFrame = frame;
    lastCapturedText = capture;
    return true;
}

static bool frameToRollingCode(const SubGhzAdvancedFrame& frame, RfCodes& out) {
    out = {};
    out.protocol = "RcSwitch";
    out.preset = "1";
    out.frequency = frame.frequency_hz ? frame.frequency_hz : uint32_t(bruceConfigPins.rfFreq * 1000000.0f);
    out.Bit = frame.bit_count > 0 ? frame.bit_count : 64;
    out.key = parseHexU64(frame.key_hex);
    out.te = parseFlexibleU32(extractNoteValue(frame.notes, "TE="));
    if(out.te == 0) out.te = 350;

    String mf = extractManufacturerFromNotes(frame.notes);
    if(mf.length()) out.mf_name = mf;
    if(frame.button.length()) out.btn = uint8_t(parseFlexibleU32(frame.button));
    if(frame.serial.length()) out.serial = uint32_t(parseHexU64(frame.serial));
    if(frame.counter.length()) out.cnt = uint16_t(parseFlexibleU32(frame.counter));

    if(out.key) {
        uint64_t yek = reverse_bits(out.key, 64);
        out.fix = uint32_t(yek >> 32);
        out.encrypted = uint32_t(yek & 0xFFFFFFFFUL);
        if(out.btn == 0) out.btn = uint8_t(out.fix >> 28);
        if(out.serial == 0) out.serial = uint32_t((yek >> 32) & 0x0FFFFFFFUL);
        if(out.cnt == 0) out.cnt = uint16_t(out.encrypted & 0xFFFF);
    }

    return out.key != 0 || out.serial != 0;
}

static bool chooseRecentFrame(SubGhzAdvancedFrame& out, const char* title = "Rolling Source") {
    const auto& recent = eng.getRecent();
    if(recent.empty()) return false;

    size_t selected = SIZE_MAX;
    std::vector<Option> opts;
    for(size_t i = 0; i < recent.size(); i++) {
        String label = String(i + 1) + ") " + recent[i].protocol_name;
        opts.emplace_back(label, [i, &selected]() { selected = i; });
    }
    addOptionToMainMenu();
    loopOptions(opts, MENU_TYPE_SUBMENU, title);
    if(selected == SIZE_MAX || selected >= recent.size()) return false;
    out = recent[selected];
    return true;
}

static void showRollingPreview(const RfCodes& code, const SubGhzAdvancedFrame& frame) {
    drawMainBorderWithTitle("Rolling Preview");
    padprintln("Proto: " + frame.protocol_name);
    padprintln("Freq: " + String(code.frequency));
    if(code.mf_name.length()) padprintln("MF: " + code.mf_name);
    if(code.serial) padprintln("Serial: " + String(code.serial, HEX));
    padprintln("Btn: " + String(code.btn));
    padprintln("Counter: " + String(code.cnt));
    if(code.key) padprintln("Key: " + frame.key_hex);
    padprintln("");
    padprintln("Press any key to return");
    while(!check(AnyKeyPress)) delay(20);
    while(check(AnyKeyPress)) delay(20);
}

static void rollingToolsForFrame(const SubGhzAdvancedFrame& frame) {
    RfCodes rolling{};
    if(!frameToRollingCode(frame, rolling)) {
        displayError("Frame has no rolling/key data", true);
        return;
    }

    std::vector<Option> roll = {
        {"Preview", [rolling, frame]() { showRollingPreview(rolling, frame); }},
        {"Send Once", [rolling]() {
             sendRfCommand(rolling);
             addToRecentCodes(rolling);
         }},
    };

    if(rolling.serial != 0) {
        roll.push_back({"Emulate Rolling", [rolling]() mutable { loopEmulate(rolling); }});
    } else {
        roll.push_back({"Emulate Rolling (N/A)", []() { displayInfo("Needs serial/rolling fields", true); }});
    }

    addOptionToMainMenu();
    loopOptions(roll, MENU_TYPE_SUBMENU, "Rolling Interface");
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
    hasLastCapturedFrame = true;
    lastCapturedFrame = frame;
    showFrame(frame, "SubGHz Identify");
}

static void actionScanCopy() {
    if(!captureAndDecodeAdvanced(10)) {
        displayError("No signal captured", true);
        return;
    }

    std::vector<Option> copy = {
        {"View Decode", []() { showFrame(lastCapturedFrame, "Scan/Copy"); }},
        {"Replay Capture", []() {
             if(!hasLastCapturedText || !transmitCaptureText(lastCapturedText, false)) {
                 displayError("Replay failed", true);
             }
         }},
        {"Rolling Tools", []() { rollingToolsForFrame(lastCapturedFrame); }},
    };
    addOptionToMainMenu();
    loopOptions(copy, MENU_TYPE_SUBMENU, "Scan/Copy");
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

static void actionDecoderRegistry() {
    const auto& protocols = eng.getEnabledProtocolNames();
    if(protocols.empty()) {
        displayInfo("No registry loaded", true);
        return;
    }

    std::vector<Option> opts;
    for(const auto& p : protocols) {
        opts.emplace_back(p, [p]() { displayInfo(p, true); });
    }
    addOptionToMainMenu();
    loopOptions(opts, MENU_TYPE_SUBMENU, "Decoder Registry");
}

static void actionDecoderMenu() {
    String lastLabel = hasLastCapturedFrame ? "Show Last Decode" : "Show Last Decode (N/A)";
    std::vector<Option> decoder = {
        {"Capture & Decode", actionScanAndIdentify},
        {lastLabel, []() {
             if(!hasLastCapturedFrame) {
                 displayInfo("No last decode", true);
                 return;
             }
             showFrame(lastCapturedFrame, "Last Decode");
         }},
        {"Protocol Registry", actionDecoderRegistry},
    };
    addOptionToMainMenu();
    loopOptions(decoder, MENU_TYPE_SUBMENU, "Decoder Interface");
}

static void actionRollingMenu() {
    std::vector<Option> rolling = {
        {"Use Last Decode", []() {
             if(!hasLastCapturedFrame) {
                 displayInfo("No last decode", true);
                 return;
             }
             rollingToolsForFrame(lastCapturedFrame);
         }},
        {"Choose From Recent", []() {
             SubGhzAdvancedFrame frame;
             if(!chooseRecentFrame(frame, "Rolling Source")) {
                 displayInfo("No frame selected", true);
                 return;
             }
             rollingToolsForFrame(frame);
         }},
    };
    addOptionToMainMenu();
    loopOptions(rolling, MENU_TYPE_SUBMENU, "Rolling");
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
        {"Scan/Copy", actionScanCopy},
        {"Decoder UI", actionDecoderMenu},
        {"Rolling UI", actionRollingMenu},
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
