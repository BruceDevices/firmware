#ifndef __SUBGHZ_ADVANCED_ENGINE_H__
#define __SUBGHZ_ADVANCED_ENGINE_H__

#include "subghz_advanced_types.h"

#include <FS.h>
#include <vector>

enum class SubGhzAdvancedProfile {
    CORE = 0,
    FULL = 1,
};

class SubGhzAdvancedEngine {
public:
    static SubGhzAdvancedEngine& instance();

    void begin();

    void setProfile(SubGhzAdvancedProfile profile);
    SubGhzAdvancedProfile getProfile() const;
    String getProfileName() const;
    void setProtocolFilter(const String& protocol_name);
    void clearProtocolFilter();
    String getProtocolFilter() const;

    SubGhzAdvancedFrame analyzeSubFileText(const String& text, const String& source = "");
    bool analyzeSubFile(FS* fs, const String& path, SubGhzAdvancedFrame& frame);
    bool analyzePathAuto(const String& path, SubGhzAdvancedFrame& frame);
    bool transmitSubFile(FS* fs, const String& path, bool hideDefaultUI = false);
    bool transmitPathAuto(const String& path, bool hideDefaultUI = false);

    bool readAndDecode(
        float freqMHz,
        int timeoutSec,
        SubGhzAdvancedFrame& frame,
        String* capture_text = nullptr);

    void pushRecent(const SubGhzAdvancedFrame& frame);
    const std::vector<SubGhzAdvancedFrame>& getRecent() const;

    const std::vector<String>& getEnabledProtocolNames() const;

private:
    SubGhzAdvancedEngine() = default;

    bool m_initialized = false;
    SubGhzAdvancedProfile m_profile = SubGhzAdvancedProfile::CORE;
    String m_protocolFilter = "";
    std::vector<SubGhzAdvancedFrame> m_recent;

    std::vector<String> m_coreProtocols;
    std::vector<String> m_fullProtocols;

    void initProtocolTables();
};

#endif
