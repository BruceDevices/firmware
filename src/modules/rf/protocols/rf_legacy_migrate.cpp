#include "rf_legacy_migrate.h"

#if RF_SUB_LEGACY_MIGRATION

#include "rf_registry.h"

// Old legacy protocol number -> registry protocol name. Only the numbers
// the firmware actually emitted are mapped; everything else falls through to
// a generic "RcSwitch_N" entry when one exists.
struct LegacyMap {
    int proto_no;
    const char *name;
};
static const LegacyMap legacy_map[] = {
    {1,  "Princeton"  },
    {6,  "Holtek_HT12"},
    {11, "RcSwitch_11"},
    {20, "CAME"       },
    {22, "NICE_FLO"   },
};

const RfProtocolDef *rf_find_legacy(int preset_no) {
    for (const auto &m : legacy_map) {
        if (m.proto_no == preset_no) return rf_find_protocol(m.name);
    }
    // generic numbered fallback (RcSwitch_1..12)
    const RfProtocolDef *p = rf_find_protocol(String("RcSwitch_") + String(preset_no));
    return p;
}

bool rf_sub_is_legacy(const RfCodes &code) {
    if (code.protocol == "RcSwitch") return true;
    // A bare numeric preset (e.g. "11") is the old way of carrying a protocol.
    if (code.preset.length() > 0) {
        bool allDigits = true;
        for (size_t i = 0; i < code.preset.length(); i++) {
            if (!isDigit(code.preset[i])) {
                allDigits = false;
                break;
            }
        }
        if (allDigits) return true;
    }
    return false;
}

// Paths we must never try to rewrite (volatile / RAM-backed filesystems).
static bool is_volatile_path(const String &path) {
    return path.startsWith("/tmpramfile") || path.indexOf("tmpramfile") >= 0;
}

bool rf_sub_migrate(FS *fs, const String &path, RfCodes &code) {
    if (!rf_sub_is_legacy(code)) return false;

    // Resolve the legacy protocol number -> registry name and migrate the
    // in-memory representation first (this is what replay uses).
    int proto_no = 0;
    if (code.protocol == "RcSwitch") {
        proto_no = code.preset.toInt(); // old: Preset carried the number
    } else {
        proto_no = code.preset.toInt();
    }
    const RfProtocolDef *def = rf_find_legacy(proto_no);
    if (!def) return false;

    code.protocol = def->name;
    if (code.te == 0) code.te = def->te;

    // Best-effort on-disk rewrite. Skip volatile / unavailable filesystems;
    // the in-memory migration above is enough for replay to work.
    if (!fs || is_volatile_path(path)) return true;

    String bakPath = path + ".bak";
    if (fs->exists(bakPath)) return true; // already migrated once (idempotent)

    File in = fs->open(path, FILE_READ);
    if (!in) return true; // read-only / unavailable: keep in-memory migration

    String original;
    while (in.available()) original += (char)in.read();
    in.close();

    // Back up the original verbatim, then rewrite Protocol:/Preset: lines.
    File bak = fs->open(bakPath, FILE_WRITE);
    if (!bak) return true; // can't back up -> don't risk rewriting
    bak.print(original);
    bak.close();

    String out;
    int start = 0;
    while (start < (int)original.length()) {
        int nl = original.indexOf('\n', start);
        String line = (nl < 0) ? original.substring(start) : original.substring(start, nl + 1);
        if (line.startsWith("Protocol:")) {
            out += "Protocol: " + code.protocol + "\n";
        } else if (line.startsWith("Preset:")) {
            // keep a sane OOK preset name so the new path resolves it
            out += "Preset: Ook270Async\n";
        } else {
            out += line;
        }
        if (nl < 0) break;
        start = nl + 1;
    }

    File rewrite = fs->open(path, FILE_WRITE);
    if (rewrite) {
        rewrite.print(out);
        rewrite.close();
    }
    return true;
}

#endif // RF_SUB_LEGACY_MIGRATION
