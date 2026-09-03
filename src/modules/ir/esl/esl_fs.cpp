#include "esl_fs.h"

#include "core/sd_functions.h"
#include <Arduino.h>
#include <FS.h>
#include <string.h>

#define ESL_FS_DIR "/tagtinker"
#define ESL_FS_SETTINGS "/tagtinker/settings.txt"
#define ESL_FS_TARGETS "/tagtinker/targets.txt"
#define ESL_FS_RECENTS "/tagtinker/recents.txt"

#define ESL_FS_SETTINGS_CAP 32
#define ESL_FS_TEXT_CAP 1024

static FS *esl_fs_storage(void) {
    setupSdCard();
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return nullptr;
    if (!fs->exists(ESL_FS_DIR)) fs->mkdir(ESL_FS_DIR);
    return fs;
}

static bool esl_fs_read_text(FS &fs, const char *path, char *out, size_t cap) {
    if (out == nullptr || cap == 0u) return false;
    if (!fs.exists(path)) return false;

    File f = fs.open(path, FILE_READ);
    if (!f) return false;

    size_t n = f.readBytes(out, cap - 1u);
    out[n] = '\0';
    f.close();
    return true;
}

static bool esl_fs_write_text(FS &fs, const char *path, const char *text) {
    if (text == nullptr) return false;

    File f = fs.open(path, FILE_WRITE);
    if (!f) return false;

    size_t len = strlen(text);
    size_t wrote = f.write(reinterpret_cast<const uint8_t *>(text), len);
    f.close();
    return wrote == len;
}

bool esl_fs_load_session(EslSession *s) {
    if (s == nullptr) return false;

    esl_store_session_init(s);

    FS *fs = esl_fs_storage();
    if (fs == nullptr) return true; /* defaults / empty lists */

    char buf[ESL_FS_TEXT_CAP];
    if (esl_fs_read_text(*fs, ESL_FS_SETTINGS, buf, sizeof(buf))) {
        if (!esl_store_settings_parse(buf, &s->settings)) {
            esl_store_settings_defaults(&s->settings);
        }
    }
    if (esl_fs_read_text(*fs, ESL_FS_TARGETS, buf, sizeof(buf))) {
        esl_store_targets_parse(buf, s);
    }
    if (esl_fs_read_text(*fs, ESL_FS_RECENTS, buf, sizeof(buf))) {
        esl_store_recents_parse(buf, s);
    }
    return true;
}

bool esl_fs_save_settings(const EslSession *s) {
    if (s == nullptr) return false;

    FS *fs = esl_fs_storage();
    if (fs == nullptr) return false;

    char buf[ESL_FS_SETTINGS_CAP];
    if (!esl_store_settings_format(&s->settings, buf, sizeof(buf))) return false;
    return esl_fs_write_text(*fs, ESL_FS_SETTINGS, buf);
}

bool esl_fs_save_targets(const EslSession *s) {
    if (s == nullptr) return false;

    FS *fs = esl_fs_storage();
    if (fs == nullptr) return false;

    char buf[ESL_FS_TEXT_CAP];
    if (!esl_store_targets_format(s, buf, sizeof(buf))) return false;
    return esl_fs_write_text(*fs, ESL_FS_TARGETS, buf);
}

bool esl_fs_save_recents(const EslSession *s) {
    if (s == nullptr) return false;

    FS *fs = esl_fs_storage();
    if (fs == nullptr) return false;

    char buf[ESL_FS_TEXT_CAP];
    if (!esl_store_recents_format(s, buf, sizeof(buf))) return false;
    return esl_fs_write_text(*fs, ESL_FS_RECENTS, buf);
}
