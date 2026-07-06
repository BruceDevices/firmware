#include "config.h"
#include "mifare_keys_manager.h"
#include "sd_functions.h"

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
    setting["wifiAtStartup"] = wifiAtStartup;
    setting["instantBoot"] = instantBoot;
    setting["keyboardLang"] = keyboardLang;

    JsonObject _wifiAp = setting["wifiAp"].to<JsonObject>();
    _wifiAp["ssid"] = wifiAp.ssid;
    _wifiAp["pwd"] = wifiAp.pwd;
    setting["wifiMAC"] = wifiMAC; //@IncursioHack
    setting["TerminalLog"] = TerminalLog;


    JsonObject _wifi = setting["wifi"].to<JsonObject>();
    for (const auto &pair : wifi) { _wifi[pair.first] = pair.second; }

    setting["startupApp"] = startupApp;
    setting["wigleBasicToken"] = wigleBasicToken;
    setting["wdgwarsApiKey"] = wdgwarsApiKey;
    setting["devMode"] = devMode;
    setting["colorInverted"] = colorInverted;

    setting["badUSBBLEKeyboardLayout"] = badUSBBLEKeyboardLayout;
    setting["badUSBBLEKeyDelay"] = badUSBBLEKeyDelay;
    setting["badUSBBLEShowOutput"] = badUSBBLEShowOutput;

    JsonArray dm = setting["disabledMenus"].to<JsonArray>();
    for (int i = 0; i < disabledMenus.size(); i++) { dm.add(disabledMenus[i]); }

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

    if (!setting["wifiAp"].isNull()) {
        JsonObject wifiApObj = setting["wifiAp"].as<JsonObject>();
        wifiAp.ssid = wifiApObj["ssid"].as<String>();
        wifiAp.pwd = wifiApObj["pwd"].as<String>();
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
        for (JsonPair kv : wifiObj) wifi[kv.key().c_str()] = kv.value().as<String>();
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

    if (!setting["wigleBasicToken"].isNull()) {
        wigleBasicToken = setting["wigleBasicToken"].as<String>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!setting["wdgwarsApiKey"].isNull()) {
        wdgwarsApiKey = setting["wdgwarsApiKey"].as<String>();
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

void BruceConfig::setStartupApp(String value) {
    startupApp = value;
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

