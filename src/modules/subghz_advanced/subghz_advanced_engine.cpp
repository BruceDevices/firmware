#include "subghz_advanced_engine.h"

#include "core/sd_functions.h"
#include "modules/rf/rf_scan.h"
#include "modules/rf/rf_send.h"
#include "subghz_advanced_decoder_adapter.h"
#include "subghz_advanced_subfile_codec.h"
#include "subghz_advanced_transmitter_adapter.h"

static void normalizePath(String& p) {
    p.trim();
    if(!p.startsWith("/")) p = "/" + p;
}

static bool readFileText(FS* fs, const String& path, String& out_text) {
    out_text = "";
    if(fs == nullptr || !fs->exists(path)) return false;
    File f = fs->open(path, FILE_READ);
    if(!f) return false;
    while(f.available()) out_text += f.readStringUntil('\n') + "\n";
    f.close();
    return out_text.length() > 0;
}

SubGhzAdvancedEngine& SubGhzAdvancedEngine::instance() {
    static SubGhzAdvancedEngine s;
    return s;
}

void SubGhzAdvancedEngine::begin() {
    if(m_initialized) return;
    initProtocolTables();
#ifdef SUBGHZ_ADV_PROFILE_FULL
    m_profile = SubGhzAdvancedProfile::FULL;
#else
    m_profile = SubGhzAdvancedProfile::CORE;
#endif
    m_initialized = true;
}

void SubGhzAdvancedEngine::initProtocolTables() {
    if(!m_coreProtocols.empty() || !m_fullProtocols.empty()) return;

    SubGhzAdvancedDecoderAdapter::getEnabledProtocolNames(false, m_coreProtocols);
    SubGhzAdvancedDecoderAdapter::getEnabledProtocolNames(true, m_fullProtocols);

    // Safety fallback: never expose an empty list to UI.
    if(m_coreProtocols.empty()) m_coreProtocols = {"Keeloq", "CAME", "Princeton", "RAW"};
    if(m_fullProtocols.empty()) m_fullProtocols = m_coreProtocols;
}

void SubGhzAdvancedEngine::setProfile(SubGhzAdvancedProfile profile) { m_profile = profile; }

SubGhzAdvancedProfile SubGhzAdvancedEngine::getProfile() const { return m_profile; }

String SubGhzAdvancedEngine::getProfileName() const {
    return m_profile == SubGhzAdvancedProfile::FULL ? "FULL" : "CORE";
}

void SubGhzAdvancedEngine::setProtocolFilter(const String& protocol_name) {
    m_protocolFilter = protocol_name;
    m_protocolFilter.trim();
}

void SubGhzAdvancedEngine::clearProtocolFilter() { m_protocolFilter = ""; }

String SubGhzAdvancedEngine::getProtocolFilter() const { return m_protocolFilter; }

SubGhzAdvancedFrame
SubGhzAdvancedEngine::analyzeSubFileText(const String& text, const String& source) {
    begin();
    const bool full_profile = (m_profile == SubGhzAdvancedProfile::FULL);
    const String* protocol_filter = m_protocolFilter.length() ? &m_protocolFilter : nullptr;
    SubGhzAdvancedFrame frame =
        SubGhzAdvancedDecoderAdapter::decodeBruceCapture(text, source, full_profile, protocol_filter);
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

    // Advanced TX path: use protocol-native encoders from vendored Unleashed when applicable.
    String content = "";
    if(readFileText(fs, path, content)) {
        const bool full_profile = (m_profile == SubGhzAdvancedProfile::FULL);
        if(SubGhzAdvancedTransmitterAdapter::canHandleCapture(content, full_profile)) {
            return SubGhzAdvancedTransmitterAdapter::transmitCapture(
                content, full_profile, hideDefaultUI);
        }
    }

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

bool SubGhzAdvancedEngine::readAndDecode(
    float freqMHz,
    int timeoutSec,
    SubGhzAdvancedFrame& frame,
    String* capture_text) {
    begin();
    if(freqMHz <= 0.0f) freqMHz = bruceConfigPins.rfFreq;
    if(timeoutSec <= 0) timeoutSec = 10;

    String capture = RCSwitch_Read(freqMHz, timeoutSec, true, true);
    if(capture.length() == 0) return false;
    if(capture_text) *capture_text = capture;

    const bool full_profile = (m_profile == SubGhzAdvancedProfile::FULL);
    const String* protocol_filter = m_protocolFilter.length() ? &m_protocolFilter : nullptr;
    frame =
        SubGhzAdvancedDecoderAdapter::decodeBruceCapture(capture, "live-rx", full_profile, protocol_filter);
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
