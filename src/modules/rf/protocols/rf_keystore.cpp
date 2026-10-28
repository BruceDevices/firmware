// SPDX-License-Identifier: GPL-3.0-or-later
//
// Plaintext keystore loading for the shared RF protocol engine. See
// rf_keystore.h for the file formats.
#if !defined(LITE_VERSION)
#include "rf_keystore.h"
#include "rf_keystore_data.h" // built-in plaintext community keyfile
#include "core/sd_functions.h"
#include "rf_keeloq.h"
#include "rf_stream.h"
#include <globals.h>
#include <vector>

namespace {
bool keystoreLoaded = false;

bool hexValue(char c, uint8_t &out) {
    if (c >= '0' && c <= '9') out = c - '0';
    else if (c >= 'a' && c <= 'f') out = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') out = c - 'A' + 10;
    else return false;
    return true;
}

uint64_t parseHex64(const String &text, bool &ok) {
    uint64_t key = 0;
    ok = text.length() > 0 && text.length() <= 16;
    for (unsigned i = 0; ok && i < text.length(); i++) {
        uint8_t nibble = 0;
        if (!hexValue(text[i], nibble)) ok = false;
        else key = (key << 4) | nibble;
    }
    return key;
}

// Parse "HEXKEY:TYPE:NAME" lines from a plaintext keyfile. Only lines with
// a valid 64-bit hex key and a name are accepted; comments and header lines
// (Filetype/Version) are skipped naturally.
size_t parseKeystoreLines(const String &content) {
    size_t added = 0;
    int start = 0;
    const int len = content.length();
    while (start < len) {
        int nl = content.indexOf('\n', start);
        String line = (nl < 0) ? content.substring(start) : content.substring(start, nl);
        start = (nl < 0) ? len : nl + 1;
        line.trim();
        if (line.isEmpty() || line.startsWith("#")) continue;
        const int c1 = line.indexOf(':');
        const int c2 = (c1 < 0) ? -1 : line.indexOf(':', c1 + 1);
        if (c1 <= 0 || c2 < 0) continue;
        String keyhex = line.substring(0, c1);
        keyhex.trim();
        String name = line.substring(c2 + 1);
        name.trim();
        String type = line.substring(c1 + 1, c2);
        type.trim();
        bool ok = false;
        const uint64_t key = parseHex64(keyhex, ok);
        if (!ok || !key || name.isEmpty()) continue;
        if (rf_stream_keystore_add_key(name.c_str(), key, (uint16_t)type.toInt())) added++;
    }
    return added;
}

void loadKeystoreFile(FS *fs, const char *path) {
    File file = fs->open(path, FILE_READ);
    if (!file) return;
    String content = file.readString();
    file.close();
    const size_t added = parseKeystoreLines(content);
    log_d("Keystore %s: %u keys", path, (unsigned)added);
}

// Collect hex bytes for the VAG aut64 key blob. Handles both the Flipper RAW
// layout (header lines then "Encrypt_data: RAW" hex data) and a bare hex dump.
void loadRawBlob(FS *fs, const char *path) {
    File file = fs->open(path, FILE_READ);
    if (!file) return;
    const String content = file.readString();
    file.close();

    std::vector<uint8_t> blob;
    int start = 0;
    const int len = content.length();
    while (start < len) {
        int nl = content.indexOf('\n', start);
        String line = (nl < 0) ? content.substring(start) : content.substring(start, nl);
        start = (nl < 0) ? len : nl + 1;
        line.trim();
        if (line.isEmpty()) continue;
        if (line.indexOf(':') >= 0) continue; // Filetype/Version/Encrypt_data headers
        for (unsigned i = 0; i + 1 < line.length(); i += 2) {
            uint8_t hi = 0, lo = 0;
            if (!hexValue(line[i], hi) || !hexValue(line[i + 1], lo)) { blob.clear(); break; }
            blob.push_back((hi << 4) | lo);
        }
    }
    if (blob.empty()) {
        log_d("RAW keystore %s: no data", path);
        return;
    }
    rf_stream_keystore_raw_blob_set(blob.data(), blob.size());
    log_d("RAW keystore %s: %u bytes", path, (unsigned)blob.size());
}

void loadVagBlob(FS *fs) {
    if (fs->exists("/BruceRF/ProtoPirate/keystore/vag")) loadRawBlob(fs, "/BruceRF/ProtoPirate/keystore/vag");
    else if (fs->exists("/BruceRF/ProtoPirate/vag")) loadRawBlob(fs, "/BruceRF/ProtoPirate/vag");
}

void loadFlipperKeystore(FS *fs) {
    static const char *paths[] = {"/BruceRF/ProtoPirate/keystore", "/BruceRF/keystore", "/keystore"};
    for (const char *path : paths) {
        if (fs->exists(path)) {
            loadKeystoreFile(fs, path);
            return;
        }
    }
}
} // namespace

void rf_keystore_load() {
    if (keystoreLoaded) return;
    keystoreLoaded = true;

    rf_stream_keystore_clear_keys();

    // Built-in plaintext community keyfile (Kia V3/V4/V5/V6, KeeLoq
    // manufacturer codes, ...). Storage files below override/extend it.
    for (const RfKeystoreBuiltinKey &entry : rf_keystore_builtin)
        rf_stream_keystore_add_key(entry.name, entry.key, entry.type);

    FS *fs = nullptr;
    if (getFsStorage(fs) && fs) {
        loadFlipperKeystore(fs);
        loadVagBlob(fs);
    }

    // Bruce's KeeLoq manufacturer keystore (/mfcodes or the embedded copy).
    // Entries whose type is 10..13 double as the Kia V3/V4/V5/V6 keys, so a
    // keystore carrying those works without a separate Flipper file.
    KeeloqKeystore keeloq{keeloq_mfcodes_fs()};
    for (const KeeloqKey &key : keeloq.get_keys())
        rf_stream_keystore_add_key(key.mf_name.c_str(), key.key, (uint16_t)key.type);
}
#endif
