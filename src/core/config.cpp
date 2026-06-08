#include "config.h"
#include "mifare_keys_manager.h"
#include "sd_functions.h"
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include "esp_mac.h"

// --- Encrypted config field helpers ---
// Derives a 32-byte AES key from the device MAC address
static bool derive_config_key(uint8_t key[32]) {
    uint8_t mac[6];
    if (esp_efuse_mac_get_default(mac) != ESP_OK) {
        // Fallback: use a fixed key (less secure but functional)
        const char *fallback = "BruceConfigEncKey2024";
        memcpy(key, fallback, strlen(fallback));
        memset(key + strlen(fallback), 0, 32 - strlen(fallback));
        return false;
    }
    // Derive key from MAC + salt using SHA-256
    uint8_t material[16];
    memcpy(material, mac, 6);
    const char *salt = "BruceSecureConfig";
    memcpy(material + 6, salt, 10);
    mbedtls_sha256(material, 16, key, 0);
    return true;
}

// Encrypts a plaintext string using AES-256-CBC with MAC-derived key
// Returns base64-encoded "IV:ciphertext" format
static String encrypt_field(const String &plaintext) {
    if (plaintext.length() == 0) return "";
    
    uint8_t key[32];
    derive_config_key(key);
    
    // Generate random IV
    uint8_t iv[16];
    for (int i = 0; i < 16; i++) iv[i] = esp_random() & 0xFF;
    
    // Pad plaintext using PKCS#7
    size_t pad_len = 16 - (plaintext.length() % 16);
    size_t buf_len = plaintext.length() + pad_len;
    uint8_t *buf = (uint8_t *)malloc(buf_len);
    if (!buf) return "";
    memcpy(buf, plaintext.c_str(), plaintext.length());
    for (size_t i = plaintext.length(); i < buf_len; i++) buf[i] = pad_len;
    
    // Encrypt
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 256);
    uint8_t ciphertext[buf_len];
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, buf_len, iv, buf, ciphertext);
    mbedtls_aes_free(&aes);
    free(buf);
    
    // Encode IV:ciphertext as hex
    String result;
    for (int i = 0; i < 16; i++) result += String(iv[i], HEX);
    result += ":";
    for (size_t i = 0; i < buf_len; i++) {
        if (ciphertext[i] < 16) result += "0";
        result += String(ciphertext[i], HEX);
    }
    return result;
}

// Decrypts a field encrypted with encrypt_field
static String decrypt_field(const String &encrypted) {
    if (encrypted.length() == 0 || !encrypted.contains(':')) return encrypted;
    
    uint8_t key[32];
    derive_config_key(key);
    
    int colon = encrypted.indexOf(':');
    String iv_hex = encrypted.substring(0, colon);
    String ct_hex = encrypted.substring(colon + 1);
    
    if (iv_hex.length() != 32 || ct_hex.length() % 32 != 0) return encrypted;
    
    // Parse IV
    uint8_t iv[16];
    for (int i = 0; i < 16; i++) {
        char hex[3] = {iv_hex[i*2], iv_hex[i*2+1], 0};
        iv[i] = strtol(hex, NULL, 16);
    }
    
    // Parse ciphertext
    size_t ct_len = ct_hex.length() / 2;
    uint8_t *ct = (uint8_t *)malloc(ct_len);
    if (!ct) return encrypted;
    for (size_t i = 0; i < ct_len; i++) {
        char hex[3] = {ct_hex[i*2], ct_hex[i*2+1], 0};
        ct[i] = strtol(hex, NULL, 16);
    }
    
    // Decrypt
    uint8_t *plaintext = (uint8_t *)malloc(ct_len);
    if (!plaintext) { free(ct); return encrypted; }
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ct_len, iv, ct, plaintext);
    mbedtls_aes_free(&aes);
    free(ct);
    
    // Remove PKCS#7 padding
    size_t pad = plaintext[ct_len - 1];
    if (pad > 16 || pad == 0) { free(plaintext); return String((const char *)plaintext, ct_len); }
    size_t text_len = ct_len - pad;
    
    String result = String((const char *)plaintext, text_len);
    free(plaintext);
    return result;
}

// Wrapper to check if a string looks encrypted (has the "hex:hex" format)
static bool is_encrypted(const String &s) {
    return s.indexOf(':') > 0 && s.indexOf(':') < s.length() - 1;
}

// -------------------------------------------------------------------------

JsonDocument BruceConfig::toJson() const {
    JsonDocument jsonDoc;
    JsonObject setting = jsonDoc.to<JsonObject>();

    setting["priColor"] = String(priColor, HEX);
    setting["secColor"] = String(secColor, HEX);
    setting["bgColor"] = String(bgColor, HEX);
    setting["themeFile"] = themePath;
    setting["themeOnSd"] = theme.fs;

    setting["dimmerSet"] = dimmerSet;
    setting["bright"] = bright;
    setting["automaticTimeUpdateViaNTP"] = automaticTimeUpdateViaNTP;
    setting["tmz"] = tmz;
    setting["dst"] = dst;
    setting["clock24hr"] = clock24hr;
    setting["soundEnabled"] = soundEnabled;
    setting["soundVolume"] = soundVolume;
    setting["wifiAtStartup"] = wifiAtStartup;
    setting["instantBoot"] = instantBoot;
    setting["keyboardLang"] = keyboardLang;

#ifdef HAS_RGB_LED
    setting["ledBright"] = ledBright;
    setting["ledColor"] = String(ledColor, HEX);
    setting["ledBlinkEnabled"] = ledBlinkEnabled;
    setting["ledEffect"] = ledEffect;
    setting["ledEffectSpeed"] = ledEffectSpeed;
    setting["ledEffectDirection"] = ledEffectDirection;
#endif

    JsonObject _webUI = setting["webUI"].to<JsonObject>();
    _webUI["user"] = webUI.user;
    _webUI["pwd"] = webUI.pwd;
    JsonObject _webUISessions = setting["webUISessions"].to<JsonObject>();
    for (size_t i = 0; i < webUISessions.size(); i++) { _webUISessions[String(i + 1)] = webUISessions[i]; }

    JsonObject _wifiAp = setting["wifiAp"].to<JsonObject>();
    _wifiAp["ssid"] = wifiAp.ssid;
    _wifiAp["pwd"] = wifiAp.pwd;
    setting["wifiMAC"] = wifiMAC; //@IncursioHack
    setting["TerminalLog"] = TerminalLog;

    JsonArray _evilWifiNames = setting["evilWifiNames"].to<JsonArray>();
    for (auto key : evilWifiNames) _evilWifiNames.add(key);

    JsonObject _evilWifiEndpoints = setting["evilWifiEndpoints"].to<JsonObject>();
    _evilWifiEndpoints["getCredsEndpoint"] = evilPortalEndpoints.getCredsEndpoint;
    _evilWifiEndpoints["setSsidEndpoint"] = evilPortalEndpoints.setSsidEndpoint;
    _evilWifiEndpoints["showEndpoints"] = evilPortalEndpoints.showEndpoints;
    _evilWifiEndpoints["allowSetSsid"] = evilPortalEndpoints.allowSetSsid;
    _evilWifiEndpoints["allowGetCreds"] = evilPortalEndpoints.allowGetCreds;
    _evilWifiEndpoints["gatewayIp"] = evilPortalGatewayIp;

    setting["evilWifiPasswordMode"] = evilPortalPasswordMode;

    JsonObject _wifi = setting["wifi"].to<JsonObject>();
    for (const auto &pair : wifi) { _wifi[pair.first] = pair.second; }

    setting["startupApp"] = startupApp;
    setting["startupAppJSInterpreterFile"] = startupAppJSInterpreterFile;
    setting["wigleBasicToken"] = wigleBasicToken;
    setting["wdgwarsApiKey"] = wdgwarsApiKey;
    setting["devMode"] = devMode;
    setting["colorInverted"] = colorInverted;

    setting["badUSBBLEKeyboardLayout"] = badUSBBLEKeyboardLayout;
    setting["badUSBBLEKeyDelay"] = badUSBBLEKeyDelay;
    setting["badUSBBLEShowOutput"] = badUSBBLEShowOutput;

    JsonArray dm = setting["disabledMenus"].to<JsonArray>();
    for (int i = 0; i < disabledMenus.size(); i++) { dm.add(disabledMenus[i]); }

    JsonArray qrArray = setting["qrCodes"].to<JsonArray>();
    for (const auto &entry : qrCodes) {
        JsonObject qrEntry = qrArray.add<JsonObject>();
        qrEntry["menuName"] = entry.menuName;
        qrEntry["content"] = entry.content;
    }

    // Encrypt sensitive fields before saving
    if (!setting["webUI"].isNull()) {
        setting["webUI"]["user"] = encrypt_field(setting["webUI"]["user"].as<String>());
        setting["webUI"]["pwd"] = encrypt_field(setting["webUI"]["pwd"].as<String>());
    }
    if (!setting["wifiAp"].isNull()) {
        setting["wifiAp"]["ssid"] = encrypt_field(setting["wifiAp"]["ssid"].as<String>());
        setting["wifiAp"]["pwd"] = encrypt_field(setting["wifiAp"]["pwd"].as<String>());
    }
    if (!setting["wifi"].isNull()) {
        JsonObject _wifiObj = setting["wifi"].as<JsonObject>();
        for (JsonPair kv : _wifiObj) {
            _wifiObj[kv.key()] = encrypt_field(kv.value().as<String>());
        }
    }
    if (!setting["wigleBasicToken"].isNull()) {
        setting["wigleBasicToken"] = encrypt_field(setting["wigleBasicToken"].as<String>());
    }
    if (!setting["wdgwarsApiKey"].isNull()) {
        setting["wdgwarsApiKey"] = encrypt_field(setting["wdgwarsApiKey"].as<String>());
    }

    return jsonDoc;
}

void BruceConfig::fromFile(bool checkFS) {
    FS *fs;
    if (checkFS) {
        if (!getFsStorage(fs)) {
            log_i("Fail getting filesystem for config");
            return;
        }
    } else {
        if (checkLittleFsSize()) fs = &LittleFS;
        else return;
    }

    if (!fs->exists(filepath)) {
        log_i("Config file not found. Creating default config");
        return saveFile();
    }

    File file;
    file = fs->open(filepath, FILE_READ);
    if (!file) {
        log_i("Config file not found. Using default values");
        return;
    }

    // Deserialize the JSON document
    JsonDocument jsonDoc;
    if (deserializeJson(jsonDoc, file)) {
        Serial.println("Failed to read config file, using default configuration");
        return;
    }
    file.close();

    JsonObject setting = jsonDoc.as<JsonObject>();
    int count = 0;

    if (!setting["priColor"].isNull()) {
        priColor = strtoul(setting["priColor"], nullptr, 16);
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["secColor"].isNull()) {
        secColor = strtoul(setting["secColor"], nullptr, 16);
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["bgColor"].isNull()) {
        bgColor = strtoul(setting["bgColor"], nullptr, 16);
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["themeFile"].isNull()) {
        themePath = setting["themeFile"].as<String>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["themeOnSd"].isNull()) {
        theme.fs = setting["themeOnSd"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["dimmerSet"].isNull()) {
        dimmerSet = setting["dimmerSet"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["bright"].isNull()) {
        bright = setting["bright"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["automaticTimeUpdateViaNTP"].isNull()) {
        automaticTimeUpdateViaNTP = setting["automaticTimeUpdateViaNTP"].as<bool>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["tmz"].isNull()) {
        tmz = setting["tmz"].as<float>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["dst"].isNull()) {
        dst = setting["dst"].as<bool>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["clock24hr"].isNull()) {
        clock24hr = setting["clock24hr"].as<bool>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["soundEnabled"].isNull()) {
        soundEnabled = setting["soundEnabled"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["soundVolume"].isNull()) {
        soundVolume = setting["soundVolume"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["wifiAtStartup"].isNull()) {
        wifiAtStartup = setting["wifiAtStartup"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["instantBoot"].isNull()) {
        instantBoot = setting["instantBoot"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["keyboardLang"].isNull()) {
        keyboardLang = setting["keyboardLang"].as<String>();
    } else {
        keyboardLang = "QWERTY";
    }

#ifdef HAS_RGB_LED
    if (!setting["ledBright"].isNull()) {
        ledBright = setting["ledBright"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["ledColor"].isNull()) {
        ledColor = strtoul(setting["ledColor"], nullptr, 16);
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["ledBlinkEnabled"].isNull()) {
        ledBlinkEnabled = setting["ledBlinkEnabled"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["ledEffect"].isNull()) {
        ledEffect = setting["ledEffect"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["ledEffectSpeed"].isNull()) {
        ledEffectSpeed = setting["ledEffectSpeed"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["ledEffectDirection"].isNull()) {
        ledEffectDirection = setting["ledEffectDirection"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
#endif

    if (!setting["webUI"].isNull()) {
        JsonObject webUIObj = setting["webUI"].as<JsonObject>();
        {
            String _val = webUIObj["user"].as<String>();
            webUI.user = is_encrypted(_val) ? decrypt_field(_val) : _val;
        }
        {
            String _val = webUIObj["pwd"].as<String>();
            webUI.pwd = is_encrypted(_val) ? decrypt_field(_val) : _val;
        }
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["webUISessions"].isNull()) {
        webUISessions.clear();
        JsonObject webUISessionsObj = setting["webUISessions"].as<JsonObject>();
        for (JsonPair kv : webUISessionsObj) { webUISessions.push_back(kv.value().as<String>()); }
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["wifiAp"].isNull()) {
        JsonObject wifiApObj = setting["wifiAp"].as<JsonObject>();
        {
            String _val = wifiApObj["ssid"].as<String>();
            wifiAp.ssid = is_encrypted(_val) ? decrypt_field(_val) : _val;
        }
        {
            String _val = wifiApObj["pwd"].as<String>();
            wifiAp.pwd = is_encrypted(_val) ? decrypt_field(_val) : _val;
        }
    } else {
        count++;
        log_e("Fail");
    }

    //@IncursioHack
    if (!setting["wifiMAC"].isNull()) {
        wifiMAC = setting["wifiMAC"].as<String>();
    } else {
        wifiMAC = "";
        count++;
        log_e("wifiMAC not found, using default");
    }
    if (!setting["TerminalLog"].isNull()) {
        TerminalLog = setting["TerminalLog"].as<bool>();
    } else {
        count++;
        log_e("TerminalLog not found, using default");
    }

    // Wifi List
    if (!setting["wifi"].isNull()) {
        wifi.clear();
        JsonObject wifiObj = setting["wifi"].as<JsonObject>();
        for (JsonPair kv : wifiObj) {
            String _val = kv.value().as<String>();
            wifi[kv.key().c_str()] = is_encrypted(_val) ? decrypt_field(_val) : _val;
        }
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["evilWifiNames"].isNull()) {
        evilWifiNames.clear();
        JsonArray _evilWifiNames = setting["evilWifiNames"].as<JsonArray>();
        for (JsonVariant key : _evilWifiNames) evilWifiNames.insert(key.as<String>());
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["evilWifiEndpoints"].isNull()) {
        JsonObject evilPortalEndpointsObj = setting["evilWifiEndpoints"].as<JsonObject>();
        evilPortalEndpoints.getCredsEndpoint = evilPortalEndpointsObj["getCredsEndpoint"].as<String>();
        evilPortalEndpoints.setSsidEndpoint = evilPortalEndpointsObj["setSsidEndpoint"].as<String>();
        evilPortalEndpoints.showEndpoints = evilPortalEndpointsObj["showEndpoints"].as<bool>();
        evilPortalEndpoints.allowSetSsid = evilPortalEndpointsObj["allowSetSsid"].as<bool>();
        evilPortalEndpoints.allowGetCreds = evilPortalEndpointsObj["allowGetCreds"].as<bool>();
        if (!evilPortalEndpointsObj["gatewayIp"].isNull()) {
            evilPortalGatewayIp = evilPortalEndpointsObj["gatewayIp"].as<String>();
        } else {
            evilPortalGatewayIp = "172.0.0.1";
        }
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["evilWifiPasswordMode"].isNull()) {
        int mode = setting["evilWifiPasswordMode"].as<int>();
        if (mode >= 0 && mode <= 2) {
            evilPortalPasswordMode = static_cast<EvilPortalPasswordMode>(mode);
        } else {
            evilPortalPasswordMode = FULL_PASSWORD;
            log_w("Invalid evilWifiPasswordMode, using FULL_PASSWORD");
        }
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["startupApp"].isNull()) {
        startupApp = setting["startupApp"].as<String>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["startupAppJSInterpreterFile"].isNull()) {
        startupAppJSInterpreterFile = setting["startupAppJSInterpreterFile"].as<String>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["wigleBasicToken"].isNull()) {
        {
            String _val = setting["wigleBasicToken"].as<String>();
            wigleBasicToken = is_encrypted(_val) ? decrypt_field(_val) : _val;
        }
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["wdgwarsApiKey"].isNull()) {
        {
            String _val = setting["wdgwarsApiKey"].as<String>();
            wdgwarsApiKey = is_encrypted(_val) ? decrypt_field(_val) : _val;
        }
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["devMode"].isNull()) {
        devMode = setting["devMode"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["colorInverted"].isNull()) {
        colorInverted = setting["colorInverted"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["badUSBBLEKeyboardLayout"].isNull()) {
        badUSBBLEKeyboardLayout = setting["badUSBBLEKeyboardLayout"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["badUSBBLEKeyDelay"].isNull()) {
        badUSBBLEKeyDelay = setting["badUSBBLEKeyDelay"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["badUSBBLEShowOutput"].isNull()) {
        badUSBBLEShowOutput = setting["badUSBBLEShowOutput"].as<bool>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["disabledMenus"].isNull()) {
        disabledMenus.clear();
        JsonArray dm = setting["disabledMenus"].as<JsonArray>();
        for (JsonVariant e : dm) { disabledMenus.push_back(e.as<String>()); }
    } else {
        count++;
        log_e("Fail");
    }

    if (!setting["qrCodes"].isNull()) {
        qrCodes.clear();
        JsonArray qrArray = setting["qrCodes"].as<JsonArray>();
        for (JsonObject qrEntry : qrArray) {
            String menuName = qrEntry["menuName"].as<String>();
            String content = qrEntry["content"].as<String>();
            qrCodes.push_back({menuName, content});
        }
    } else {
        count++;
        log_e("Fail to load qrCodes");
    }

    validateConfig();
    if (count > 0) saveFile();

    // Load MIFARE keys (loading via manager)
    MifareKeysManager::ensureLoaded(mifareKeys);

    log_i("Using config from file");
}

void BruceConfig::saveFile() {
    FS *fs = &LittleFS;
    JsonDocument jsonDoc = toJson();

    // Open file for writing
    File file = fs->open(filepath, FILE_WRITE);
    if (!file) {
        log_e("Failed to open config file");
        file.close();
        return;
    };

    // Serialize JSON to file
    serializeJsonPretty(jsonDoc, Serial);
    if (serializeJsonPretty(jsonDoc, file) < 5) log_e("Failed to write config file");
    else log_i("config file written successfully");

    file.close();

    if (setupSdCard()) copyToFs(LittleFS, SD, filepath, false);
}

void BruceConfig::factoryReset() {
    FS *fs = &LittleFS;
    fs->rename(String(filepath), "/bak." + String(filepath).substring(1));
    if (setupSdCard()) SD.rename(String(filepath), "/bak." + String(filepath).substring(1));
    ESP.restart();
}

void BruceConfig::validateConfig() {
    validateDimmerValue();
    validateBrightValue();
    validateTmzValue();
    validateSoundEnabledValue();
    validateSoundVolumeValue();
    validateWifiAtStartupValue();
#ifdef HAS_RGB_LED
    validateLedBrightValue();
    validateLedColorValue();
    validateLedBlinkEnabledValue();
    validateLedEffectValue();
    validateLedEffectSpeedValue();
    validateLedEffectDirectionValue();
#endif
    validateMifareKeysItems();
    validateDevModeValue();
    validateColorInverted();
    validateBadUSBBLEKeyboardLayout();
    validateBadUSBBLEKeyDelay();
    validateEvilEndpointCreds();
    validateEvilEndpointSsid();
    validateEvilPasswordMode();
    validateEvilGatewayIp();
}

void BruceConfig::setUiColor(uint16_t primary, uint16_t *secondary, uint16_t *background) {
    BruceTheme::_setUiColor(primary, secondary, background);
    saveFile();
}

void BruceConfig::setDimmer(int value) {
    dimmerSet = value;
    validateDimmerValue();
    saveFile();
}

void BruceConfig::validateDimmerValue() {
    if (dimmerSet < 0) dimmerSet = 10;
    if (dimmerSet > 60) dimmerSet = 0;
}

void BruceConfig::setBright(uint8_t value) {
    bright = value;
    validateBrightValue();
    saveFile();
}

void BruceConfig::validateBrightValue() {
    if (bright > 100) bright = 100;
}

void BruceConfig::setAutomaticTimeUpdateViaNTP(bool value) {
    automaticTimeUpdateViaNTP = value;
    saveFile();
}

void BruceConfig::setTmz(float value) {
    tmz = value;
    validateTmzValue();
    saveFile();
}

void BruceConfig::validateTmzValue() {
    if (tmz < -12 || tmz > 14) tmz = 0;
}

void BruceConfig::setDST(bool value) {
    dst = value;
    saveFile();
}

void BruceConfig::setClock24Hr(bool value) {
    clock24hr = value;
    saveFile();
}

void BruceConfig::setSoundEnabled(int value) {
    soundEnabled = value;
    validateSoundEnabledValue();
    saveFile();
}

void BruceConfig::setSoundVolume(int value) {
    soundVolume = value;
    validateSoundVolumeValue();
    saveFile();
}

void BruceConfig::validateSoundEnabledValue() {
    if (soundEnabled > 1) soundEnabled = 1;
}

void BruceConfig::validateSoundVolumeValue() {
    if (soundVolume > 100) soundVolume = 100;
}

void BruceConfig::setWifiAtStartup(int value) {
    wifiAtStartup = value;
    validateWifiAtStartupValue();
    saveFile();
}

void BruceConfig::validateWifiAtStartupValue() {
    if (wifiAtStartup > 1) wifiAtStartup = 1;
}

#ifdef HAS_RGB_LED
void BruceConfig::setLedBright(int value) {
    ledBright = value;
    validateLedBrightValue();
    saveFile();
}

void BruceConfig::validateLedBrightValue() { ledBright = max(0, min(100, ledBright)); }

void BruceConfig::setLedColor(uint32_t value) {
    ledColor = value;
    validateLedColorValue();
    saveFile();
}

void BruceConfig::validateLedColorValue() {
    ledColor = max<uint32_t>(0, min<uint32_t>(0xFFFFFFFF, ledColor));
}

void BruceConfig::setLedBlinkEnabled(int value) {
    ledBlinkEnabled = value;
    validateLedBlinkEnabledValue();
    saveFile();
}

void BruceConfig::validateLedBlinkEnabledValue() {
    if (ledBlinkEnabled > 1) ledBlinkEnabled = 1;
}

void BruceConfig::setLedEffect(int value) {
    ledEffect = value;
    validateLedEffectValue();
    saveFile();
}

void BruceConfig::validateLedEffectValue() {
    if (ledEffect < 0 || ledEffect > 9) ledEffect = 0;
}

void BruceConfig::setLedEffectSpeed(int value) {
    ledEffectSpeed = value;
    validateLedEffectSpeedValue();
    saveFile();
}

void BruceConfig::validateLedEffectSpeedValue() {
#ifdef HAS_ENCODER_LED
    if (ledEffectSpeed > 11) ledEffectSpeed = 11;
#else
    if (ledEffectSpeed > 10) ledEffectSpeed = 10;
#endif
    if (ledEffectSpeed < 0) ledEffectSpeed = 1;
}

void BruceConfig::setLedEffectDirection(int value) {
    ledEffectDirection = value;
    validateLedEffectDirectionValue();
    saveFile();
}

void BruceConfig::validateLedEffectDirectionValue() {
    if (ledEffectDirection > 1 || ledEffectDirection == 0) ledEffectDirection = 1;
    if (ledEffectDirection < -1) ledEffectDirection = -1;
}
#endif

void BruceConfig::setWebUICreds(const String &usr, const String &pwd) {
    webUI.user = usr;
    webUI.pwd = pwd;
    saveFile();
}

void BruceConfig::setWifiApCreds(const String &ssid, const String &pwd) {
    wifiAp.ssid = ssid;
    wifiAp.pwd = pwd;
    saveFile();
}

void BruceConfig::setTerminalLog(bool value) {
    TerminalLog = value;
    saveFile();
}

void BruceConfig::addWifiCredential(const String &ssid, const String &pwd) {
    wifi[ssid] = pwd;
    saveFile();
}

String BruceConfig::getWifiPassword(const String &ssid) const {
    auto it = wifi.find(ssid);
    if (it != wifi.end()) return it->second;
    return "";
}

void BruceConfig::addEvilWifiName(String value) {
    evilWifiNames.insert(value);
    saveFile();
}

void BruceConfig::removeEvilWifiName(String value) {
    evilWifiNames.erase(value);
    saveFile();
}

void BruceConfig::setEvilEndpointCreds(String value) {
    evilPortalEndpoints.getCredsEndpoint = value;
    validateEvilEndpointCreds();
    saveFile();
}

void BruceConfig::validateEvilEndpointCreds() {
    if (evilPortalEndpoints.getCredsEndpoint == evilPortalEndpoints.setSsidEndpoint) {
        // on collision reset to defaults
        evilPortalEndpoints.getCredsEndpoint = "/creds";
    }
    if (evilPortalEndpoints.getCredsEndpoint[0] != '/') {
        evilPortalEndpoints.getCredsEndpoint = '/' + evilPortalEndpoints.getCredsEndpoint;
    }
}

void BruceConfig::setEvilEndpointSsid(String value) {
    evilPortalEndpoints.setSsidEndpoint = value;
    validateEvilEndpointCreds();
    saveFile();
}

void BruceConfig::validateEvilEndpointSsid() {
    if (evilPortalEndpoints.getCredsEndpoint == evilPortalEndpoints.setSsidEndpoint) {
        // on collision reset to defaults
        evilPortalEndpoints.setSsidEndpoint = "/ssid";
    }
    if (evilPortalEndpoints.setSsidEndpoint[0] != '/') {
        evilPortalEndpoints.setSsidEndpoint = '/' + evilPortalEndpoints.setSsidEndpoint;
    }
}

void BruceConfig::setEvilAllowEndpointDisplay(bool value) {
    evilPortalEndpoints.showEndpoints = value;
    saveFile();
}

void BruceConfig::setEvilAllowGetCreds(bool value) {
    evilPortalEndpoints.allowGetCreds = value;
    saveFile();
}

void BruceConfig::setEvilAllowSetSsid(bool value) {
    evilPortalEndpoints.allowSetSsid = value;
    saveFile();
}

void BruceConfig::setEvilPasswordMode(EvilPortalPasswordMode value) {
    evilPortalPasswordMode = value;
    saveFile();
}

void BruceConfig::validateEvilPasswordMode() {
    if (evilPortalPasswordMode < 0 || evilPortalPasswordMode > 2) evilPortalPasswordMode = FULL_PASSWORD;
}

void BruceConfig::setEvilGatewayIp(String value) {
    evilPortalGatewayIp = value;
    validateEvilGatewayIp();
    saveFile();
}

void BruceConfig::validateEvilGatewayIp() {
    IPAddress gatewayIp;
    if (!gatewayIp.fromString(evilPortalGatewayIp)) evilPortalGatewayIp = "172.0.0.1";
}

void BruceConfig::setStartupApp(String value) {
    startupApp = value;
    saveFile();
}

void BruceConfig::setStartupAppJSInterpreterFile(String value) {
    startupAppJSInterpreterFile = value;
    saveFile();
}

void BruceConfig::setWigleBasicToken(String value) {
    wigleBasicToken = value;
    saveFile();
}

void BruceConfig::setWdgwarsApiKey(String value) {
    wdgwarsApiKey = value;
    saveFile();
}

void BruceConfig::setDevMode(int value) {
    devMode = value;
    validateDevModeValue();
    saveFile();
}

void BruceConfig::validateDevModeValue() {
    if (devMode > 1) devMode = 1;
}

void BruceConfig::setColorInverted(int value) {
    colorInverted = value;
    validateColorInverted();
    saveFile();
}

void BruceConfig::validateColorInverted() {
    if (colorInverted > 1) colorInverted = 1;
}

void BruceConfig::setBadUSBBLEKeyboardLayout(int value) {
    badUSBBLEKeyboardLayout = value;
    validateBadUSBBLEKeyboardLayout();
    saveFile();
}

void BruceConfig::validateBadUSBBLEKeyboardLayout() {
    if (badUSBBLEKeyboardLayout < 0 || badUSBBLEKeyboardLayout > 13) badUSBBLEKeyboardLayout = 0;
}

void BruceConfig::setBadUSBBLEKeyDelay(uint16_t value) {
    badUSBBLEKeyDelay = value;
    validateBadUSBBLEKeyDelay();
    saveFile();
}

void BruceConfig::validateBadUSBBLEKeyDelay() {
    if (badUSBBLEKeyDelay < 0) badUSBBLEKeyDelay = 0;
    if (badUSBBLEKeyDelay > 500) badUSBBLEKeyDelay = 500;
}

void BruceConfig::setBadUSBBLEShowOutput(bool value) {
    badUSBBLEShowOutput = value;
    saveFile();
}
void BruceConfig::addMifareKey(String value) { MifareKeysManager::addKey(mifareKeys, value); }

void BruceConfig::validateMifareKeysItems() { MifareKeysManager::validateKeys(mifareKeys); }

void BruceConfig::addDisabledMenu(String value) {
    // TODO: check if duplicate
    disabledMenus.push_back(value);
    saveFile();
}

void BruceConfig::addQrCodeEntry(const String &menuName, const String &content) {
    qrCodes.push_back({menuName, content});
    saveFile();
}

void BruceConfig::removeQrCodeEntry(const String &menuName) {
    size_t writeIndex = 0;

    for (size_t readIndex = 0; readIndex < qrCodes.size(); ++readIndex) {
        const QrCodeEntry &entry = qrCodes[readIndex];

        if (entry.menuName != menuName) {
            if (writeIndex != readIndex) { qrCodes[writeIndex] = std::move(qrCodes[readIndex]); }
            ++writeIndex;
        }
    }

    if (writeIndex < qrCodes.size()) { qrCodes.erase(qrCodes.begin() + writeIndex, qrCodes.end()); }

    saveFile();
}

void BruceConfig::addWebUISession(const String &token) {
    webUISessions.push_back(token);
    // Limit to maximum 5 sessions - remove oldest (first element) if exceeded
    if (webUISessions.size() > 5) { webUISessions.erase(webUISessions.begin()); }
    saveFile();
}

void BruceConfig::removeWebUISession(const String &token) {
    for (auto it = webUISessions.begin(); it != webUISessions.end(); ++it) {
        if (*it == token) {
            webUISessions.erase(it);
            break;
        }
    }
    saveFile();
}

bool BruceConfig::isValidWebUISession(const String &token) {
    auto it = std::find(webUISessions.begin(), webUISessions.end(), token);

    if (it == webUISessions.end()) {
        return false; // Token not found
    }

    // Check if token is already at the end (most recent position)
    if (it == webUISessions.end() - 1) {
        return true; // Already most recent, no changes needed
    }

    // Move token to end and save
    webUISessions.erase(it);
    webUISessions.push_back(token);

    // Limit to maximum 10 sessions
    if (webUISessions.size() > 10) { webUISessions.erase(webUISessions.begin()); }

    saveFile();
    return true;
}
