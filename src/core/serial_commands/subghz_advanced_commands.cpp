#include "subghz_advanced_commands.h"

#include "modules/subghz_advanced/subghz_advanced_engine.h"

#include <globals.h>

static bool parseBoolArg(const Argument& arg, bool defaultValue) {
    if(!arg.isSet()) return defaultValue;
    String s = arg.getValue();
    s.trim();
    s.toLowerCase();
    if(s.length() == 0) return defaultValue;
    if(s == "1" || s == "true" || s == "yes" || s == "on") return true;
    if(s == "0" || s == "false" || s == "no" || s == "off") return false;
    return defaultValue;
}

static uint32_t subghzAdvRxCallback(cmd* c) {
    Command cmd(c);

    float freqMhz = bruceConfigPins.rfFreq;
    int timeout = 10;

    Argument freqArg = cmd.getArgument("frequency");
    if(freqArg.isSet()) {
        String s = freqArg.getValue();
        s.trim();
        if(s.length()) {
            double v = strtod(s.c_str(), NULL);
            // Accept both "433.92" (MHz) and "433920000" (Hz)
            if(v > 1000000.0) freqMhz = (float)(v / 1000000.0);
            else if(v > 0.0) freqMhz = (float)v;
        }
    }

    Argument timeoutArg = cmd.getArgument("timeout");
    if(timeoutArg.isSet()) {
        String s = timeoutArg.getValue();
        s.trim();
        if(s.length()) {
            int parsed = s.toInt();
            if(parsed > 0) timeout = parsed;
        }
    }

    SubGhzAdvancedFrame frame;
    bool ok = SubGhzAdvancedEngine::instance().readAndDecode(freqMhz, timeout, frame);
    if(!ok) {
        serialDevice->println("{}");
        return false;
    }

    serialDevice->println(frame.toJson());
    return true;
}

static uint32_t subghzAdvTxFileCallback(cmd* c) {
    Command cmd(c);
    Argument pathArg = cmd.getArgument("filepath");
    Argument hideArg = cmd.getArgument("hideDefaultUI");

    String path = pathArg.getValue();
    path.trim();
    if(path.length() == 0) return false;

    bool hideDefaultUI = parseBoolArg(hideArg, true);
    return SubGhzAdvancedEngine::instance().transmitPathAuto(path, hideDefaultUI);
}

static uint32_t subghzAdvAnalyzeFileCallback(cmd* c) {
    Command cmd(c);
    Argument pathArg = cmd.getArgument("filepath");

    String path = pathArg.getValue();
    path.trim();

    if(path.length() == 0) {
        serialDevice->println("{}");
        return false;
    }

    SubGhzAdvancedFrame frame;
    bool ok = SubGhzAdvancedEngine::instance().analyzePathAuto(path, frame);
    if(!ok) {
        serialDevice->println("{}");
        return false;
    }

    serialDevice->println(frame.toJson());
    return true;
}

static uint32_t subghzAdvProtocolsCallback(cmd* c) {
    Command cmd(c);
    Argument profileArg = cmd.getArgument("profile");

    SubGhzAdvancedEngine& engine = SubGhzAdvancedEngine::instance();
    engine.begin();

    if(profileArg.isSet()) {
        String profile = profileArg.getValue();
        profile.trim();
        profile.toUpperCase();
        if(profile == "CORE") engine.setProfile(SubGhzAdvancedProfile::CORE);
        else if(profile == "FULL")
            engine.setProfile(SubGhzAdvancedProfile::FULL);
    }

    const auto& protocols = engine.getEnabledProtocolNames();
    serialDevice->println(
        "{\"profile\":\"" + engine.getProfileName() + "\",\"count\":" + String((int)protocols.size()) +
        "}");
    for(const auto& protocol : protocols) {
        serialDevice->println(protocol);
    }

    return true;
}

static uint32_t subghzAdvProfileCallback(cmd* c) {
    Command cmd(c);
    Argument profileArg = cmd.getArgument("profile");
    if(!profileArg.isSet()) return false;

    String profile = profileArg.getValue();
    profile.trim();
    profile.toUpperCase();

    SubGhzAdvancedEngine& engine = SubGhzAdvancedEngine::instance();
    engine.begin();

    if(profile == "CORE")
        engine.setProfile(SubGhzAdvancedProfile::CORE);
    else if(profile == "FULL")
        engine.setProfile(SubGhzAdvancedProfile::FULL);
    else
        return false;

    serialDevice->println(engine.getProfileName());
    return true;
}

static uint32_t subghzAdvFilterCallback(cmd* c) {
    Command cmd(c);
    Argument filterArg = cmd.getArgument("protocol");
    if(!filterArg.isSet()) return false;

    String value = filterArg.getValue();
    value.trim();

    SubGhzAdvancedEngine& engine = SubGhzAdvancedEngine::instance();
    engine.begin();

    String lower = value;
    lower.toLowerCase();
    if(lower.length() == 0 || lower == "any" || lower == "none" || lower == "off" || lower == "*")
        engine.clearProtocolFilter();
    else
        engine.setProtocolFilter(value);

    String active = engine.getProtocolFilter();
    if(active.length() == 0) active = "Any";
    serialDevice->println(active);
    return true;
}

void createSubGhzAdvancedCommands(SimpleCLI* cli) {
    Command root = cli->addCompositeCmd("subghz_adv");

    Command rx = root.addCommand("rx", subghzAdvRxCallback);
    rx.addPosArg("frequency", String((uint32_t)(bruceConfigPins.rfFreq * 1000000.0f)).c_str());
    rx.addPosArg("timeout", "10");

    Command txFile = root.addCommand("tx_file", subghzAdvTxFileCallback);
    txFile.addPosArg("filepath");
    txFile.addPosArg("hideDefaultUI", "true");

    Command analyze = root.addCommand("analyze_file", subghzAdvAnalyzeFileCallback);
    analyze.addPosArg("filepath");

    Command protocols = root.addCommand("protocols", subghzAdvProtocolsCallback);
    protocols.addPosArg("profile", "current");

    Command profile = root.addCommand("profile", subghzAdvProfileCallback);
    profile.addPosArg("profile");

    Command filter = root.addCommand("filter", subghzAdvFilterCallback);
    filter.addPosArg("protocol", "any");
}
