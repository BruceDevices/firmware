#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>

#include <subghz/subghz_keystore.h>
#include <subghz/subghz_keystore_i.h>

extern bool setupSdCard();

static void subghz_keystore_clear(SubGhzKeystore* instance) {
    if(!instance) return;
    for M_EACH(manufacture_code, instance->data, SubGhzKeyArray_t) {
            furi_string_free(manufacture_code->name);
            manufacture_code->key = 0;
        }
    SubGhzKeyArray_clear(instance->data);
    SubGhzKeyArray_init(instance->data);
}

static bool subghz_keystore_open_read(const String& path, File& out) {
    if(path.length() == 0) return false;
    if(setupSdCard() && SD.exists(path)) {
        out = SD.open(path, FILE_READ);
        if(out) return true;
    }
    if(LittleFS.exists(path)) {
        out = LittleFS.open(path, FILE_READ);
        if(out) return true;
    }
    return false;
}

static bool subghz_keystore_parse_line(SubGhzKeystore* instance, const String& lineIn) {
    String line = lineIn;
    line.trim();
    if(line.length() == 0 || line.startsWith("#")) return true;

    String name;
    String keyStr;
    uint16_t type = 0;

    if(line.indexOf(';') > 0) {
        // Bruce format: name;key;type
        const int p1 = line.indexOf(';');
        const int p2 = line.indexOf(';', p1 + 1);
        if(p1 <= 0 || p2 <= p1) return false;
        name = line.substring(0, p1);
        keyStr = line.substring(p1 + 1, p2);
        type = (uint16_t)line.substring(p2 + 1).toInt();
    } else if(line.indexOf(':') > 0) {
        // Flipper format: key:type:name
        const int p1 = line.indexOf(':');
        const int p2 = line.indexOf(':', p1 + 1);
        if(p1 <= 0 || p2 <= p1) return false;
        keyStr = line.substring(0, p1);
        type = (uint16_t)line.substring(p1 + 1, p2).toInt();
        name = line.substring(p2 + 1);
    } else {
        return false;
    }

    name.trim();
    keyStr.trim();
    if(name.length() == 0 || keyStr.length() == 0) return false;

    if(keyStr.startsWith("0x") || keyStr.startsWith("0X")) keyStr = keyStr.substring(2);
    uint64_t key = strtoull(keyStr.c_str(), NULL, 16);

    SubGhzKey* manufacture_code = SubGhzKeyArray_push_new(instance->data);
    manufacture_code->name = furi_string_alloc_set(name.c_str());
    manufacture_code->key = key;
    manufacture_code->type = type;
    return true;
}

extern "C" SubGhzKeystore* subghz_keystore_alloc(void) {
    SubGhzKeystore* instance = (SubGhzKeystore*)malloc(sizeof(SubGhzKeystore));
    if(!instance) return NULL;
    SubGhzKeyArray_init(instance->data);
    subghz_keystore_reset_kl(instance);
    return instance;
}

extern "C" void subghz_keystore_reset_kl(SubGhzKeystore* instance) {
    if(!instance) return;
    instance->mfname = "";
    instance->kl_type = 0;
}

extern "C" void subghz_keystore_free(SubGhzKeystore* instance) {
    if(!instance) return;
    subghz_keystore_clear(instance);
    free(instance);
}

extern "C" bool subghz_keystore_load(SubGhzKeystore* instance, const char* file_name) {
    if(!instance || !file_name) return false;

    subghz_keystore_clear(instance);

    File file;
    if(!subghz_keystore_open_read(String(file_name), file)) return false;

    bool ok = false;
    while(file.available()) {
        String line = file.readStringUntil('\n');
        line.replace("\r", "");
        if(subghz_keystore_parse_line(instance, line)) ok = true;
    }
    file.close();
    return ok;
}

extern "C"
bool subghz_keystore_save(SubGhzKeystore* instance, const char* file_name, uint8_t* iv) {
    UNUSED(instance);
    UNUSED(file_name);
    UNUSED(iv);
    return false;
}

extern "C" SubGhzKeyArray_t* subghz_keystore_get_data(SubGhzKeystore* instance) {
    if(!instance) return NULL;
    return &instance->data;
}

extern "C" bool subghz_keystore_raw_encrypted_save(
    const char* input_file_name,
    const char* output_file_name,
    uint8_t* iv) {
    UNUSED(input_file_name);
    UNUSED(output_file_name);
    UNUSED(iv);
    return false;
}

extern "C"
bool subghz_keystore_raw_get_data(const char* file_name, size_t offset, uint8_t* data, size_t len) {
    if(!file_name || !data || len == 0) return false;

    File file;
    if(!subghz_keystore_open_read(String(file_name), file)) return false;
    if(offset > file.size()) {
        file.close();
        return false;
    }
    if(!file.seek(offset)) {
        file.close();
        return false;
    }

    const size_t read_sz = file.read(data, len);
    file.close();
    return read_sz == len;
}
