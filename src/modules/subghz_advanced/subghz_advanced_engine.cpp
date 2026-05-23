#include "subghz_advanced_engine.h"

#include "core/sd_functions.h"
#include "modules/rf/rf_scan.h"
#include "modules/rf/rf_send.h"
#include "subghz_advanced_decoder_adapter.h"
#include "subghz_advanced_subfile_codec.h"

static void normalizePath(String& p) {
    p.trim();
    if(!p.startsWith("/")) p = "/" + p;
}

SubGhzAdvancedEngine& SubGhzAdvancedEngine::instance() {
    static SubGhzAdvancedEngine s;
    return s;
}

void SubGhzAdvancedEngine::begin() {
    initProtocolTables();
#ifdef SUBGHZ_ADV_PROFILE_FULL
    m_profile = SubGhzAdvancedProfile::FULL;
#else
    m_profile = SubGhzAdvancedProfile::CORE;
#endif
}

void SubGhzAdvancedEngine::initProtocolTables() {
    if(!m_coreProtocols.empty()) return;

    m_coreProtocols = {
        "Keeloq", "CAME", "CAME Twee", "CAME Atomo", "Nice FLO", "Nice FloR-S", "FAAC SLH",
        "Princeton", "Linear", "LinearDelta3", "Security+ 1.0", "Security+ 2.0", "Holtek",
        "Holtek_HT12X", "Doitrand", "Megacode", "Power Smart", "RAW", "BinRAW"};

    // FULL profile placeholder for all protocols from upstream registry.
    m_fullProtocols = m_coreProtocols;
    m_fullProtocols.push_back("Nice One");
    m_fullProtocols.push_back("Nero Sketch");
    m_fullProtocols.push_back("Nero Radio");
    m_fullProtocols.push_back("Mastercode");
    m_fullProtocols.push_back("Marantec");
    m_fullProtocols.push_back("Marantec24");
    m_fullProtocols.push_back("Dooya");
    m_fullProtocols.push_back("Bett");
    m_fullProtocols.push_back("Intertechno_V3");
}

void SubGhzAdvancedEngine::setProfile(SubGhzAdvancedProfile profile) { m_profile = profile; }

SubGhzAdvancedProfile SubGhzAdvancedEngine::getProfile() const { return m_profile; }

String SubGhzAdvancedEngine::getProfileName() const {
    return m_profile == SubGhzAdvancedProfile::FULL ? "FULL" : "CORE";
}

SubGhzAdvancedFrame
SubGhzAdvancedEngine::analyzeSubFileText(const String& text, const String& source) {
    begin();
    const bool full_profile = (m_profile == SubGhzAdvancedProfile::FULL);
    SubGhzAdvancedFrame frame =
        SubGhzAdvancedDecoderAdapter::decodeBruceCapture(text, source, full_profile);
    if(frame.valid) pushRecent(frame);
    return frame;
}

bool SubGhzAdvancedEngine::analyzeSubFile(FS* fs, const String& path, SubGhzAdvancedFrame& frame) {
    begin();
    if(fs == nullptr) return false;
    if(!fs->exists(path)) return false;

    File f = fs->open(path, FILE_READ);
    if(!f) return false;

    String content = "";
    while(f.available()) {
        content += f.readStringUntil('\n');
        content += "\n";
    }
    f.close();

    frame = analyzeSubFileText(content, path);
    return frame.valid;
}

bool SubGhzAdvancedEngine::analyzePathAuto(const String& inputPath, SubGhzAdvancedFrame& frame) {
    begin();
    String path = inputPath;
    normalizePath(path);

    if(setupSdCard() && SD.exists(path)) {
        return analyzeSubFile(&SD, path, frame);
    }
    if(LittleFS.exists(path)) {
        return analyzeSubFile(&LittleFS, path, frame);
    }
    return false;
}

bool SubGhzAdvancedEngine::transmitSubFile(FS* fs, const String& path, bool hideDefaultUI) {
    begin();
    if(fs == nullptr) return false;
    if(!fs->exists(path)) return false;

    // Keep the frame history aligned with TX actions whenever the file is parseable.
    SubGhzAdvancedFrame frame;
    analyzeSubFile(fs, path, frame);

    RfCodes data{};
    return readSubFile(fs, path, data) && txSubFile(data, hideDefaultUI);
}

bool SubGhzAdvancedEngine::transmitPathAuto(const String& inputPath, bool hideDefaultUI) {
    begin();
    String path = inputPath;
    normalizePath(path);

    if(setupSdCard() && SD.exists(path)) {
        return transmitSubFile(&SD, path, hideDefaultUI);
    }
    if(LittleFS.exists(path)) {
        return transmitSubFile(&LittleFS, path, hideDefaultUI);
    }
    return false;
}

bool SubGhzAdvancedEngine::readAndDecode(float freqMHz, int timeoutSec, SubGhzAdvancedFrame& frame) {
    begin();
    if(freqMHz <= 0.0f) freqMHz = bruceConfigPins.rfFreq;
    if(timeoutSec <= 0) timeoutSec = 10;

    String capture = RCSwitch_Read(freqMHz, timeoutSec, true, true);
    if(capture.length() == 0) return false;

    const bool full_profile = (m_profile == SubGhzAdvancedProfile::FULL);
    frame = SubGhzAdvancedDecoderAdapter::decodeBruceCapture(capture, "live-rx", full_profile);
    if(frame.valid) pushRecent(frame);
    return frame.valid;
}

void SubGhzAdvancedEngine::pushRecent(const SubGhzAdvancedFrame& frame) {
    if(!frame.valid) return;
    m_recent.insert(m_recent.begin(), frame);
    if(m_recent.size() > 16) m_recent.pop_back();
}

const std::vector<SubGhzAdvancedFrame>& SubGhzAdvancedEngine::getRecent() const { return m_recent; }

const std::vector<String>& SubGhzAdvancedEngine::getEnabledProtocolNames() const {
    return m_profile == SubGhzAdvancedProfile::FULL ? m_fullProtocols : m_coreProtocols;
}
