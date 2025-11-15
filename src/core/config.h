#ifndef __BRUCE_CONFIG_H__
#define __BRUCE_CONFIG_H__

#include "theme.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <set>
#include <vector>

enum RFIDModules {
    M5_RFID2_MODULE = 0,
    PN532_I2C_MODULE = 1,
    PN532_SPI_MODULE = 2,
    RC522_SPI_MODULE = 3,
    ST25R3916_SPI_MODULE = 4,
    PN532_I2C_SPI_MODULE = 5
};

enum RFModules {
    M5_RF_MODULE = 0,
    CC1101_SPI_MODULE = 1,
};

class BruceConfig : public BruceTheme {
public:
    struct WiFiCredential {
        String ssid;
        String pwd;
    };
    struct Credential {
        String user;
        String pwd;
    };
    struct QrCodeEntry {
        String menuName;
        String content;
    };

    const char *filepath = "/bruce.conf";

    // Settings
    int rotation = ROTATION > 1 ? 3 : 1;
    int dimmerSet = 10;
    int screenOffTimeout = 15;
    int autoSleepTimeout = 0;
    int autoDeepSleepTimeout = 0;
    int bright = 100;
    int tmz = 0;
    int soundEnabled = 1;
    int soundVolume = 100;
    int wifiAtStartup = 0;
    int instantBoot = 0;

    // Led
    int ledBright = 75;
    uint32_t ledColor = 0;
    int ledBlinkEnabled = 1;
    int ledEffect = 0;
    int ledEffectSpeed = 5;
    int ledEffectDirection = 1;

    // Wifi
    Credential webUI = {"admin", "bruce"};
    WiFiCredential wifiAp = {"BruceNet", "brucenet"};
    std::map<String, String> wifi = {};
    std::set<String> evilWifiNames = {};
    String wifiMAC = ""; //@IncursioHack

    void setWifiMAC(const String &mac) {
        wifiMAC = mac;
        saveFile(); // opcional, para salvar imediatamente
    }

    // BLE
    String bleName = String("Keyboard_" + String((uint8_t)(ESP.getEfuseMac() >> 32), HEX));

    // IR
    int irTx = LED;
    uint8_t irTxRepeats = 0;
    int irRx = GROVE_SCL;

    // RF
    int rfTx = GROVE_SDA;
    int rfRx = GROVE_SCL;
    int rfModule = M5_RF_MODULE;
    float rfFreq = 433.92;
    int rfFxdFreq = 1;
    int rfScanRange = 3;

    // iButton Pin
    int iButton = 0;

    // RFID
    int rfidModule = M5_RFID2_MODULE;
    std::set<String> mifareKeys = {};

    // GPS
    int gpsBaudrate = 9600;

    // Misc
    String startupApp = "";
    String wigleBasicToken = "";
    int devMode = 0;
    int colorInverted = 1;

    std::vector<String> disabledMenus = {};

    std::vector<QrCodeEntry> qrCodes = {
        {"Bruce AP",   "WIFI:T:WPA;S:BruceNet;P:brucenet;;"},
        {"Bruce Wiki", "https://github.com/pr3y/Bruce/wiki"},
        {"Bruce Site", "https://bruce.computer"            },
        {"Rickroll",   "https://youtu.be/dQw4w9WgXcQ"      }
    };

    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    BruceConfig() {};
    // ~BruceConfig();

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    void saveFile();
    void fromFile(bool checkFS = true);
    void factoryReset();
    void validateConfig();
    JsonDocument toJson() const;

    // UI Color
    void setUiColor(uint16_t primary, uint16_t *secondary = nullptr, uint16_t *background = nullptr);

    // Settings
    void setRotation(int value);
    void validateRotationValue();
    void setDimmer(int value);
    void validateDimmerValue();
    void setScreenOffTimeout(int value);
    void validateScreenOffTimeoutValue();
    void setAutoSleepTimeout(int value);
    void validateAutoSleepTimeoutValue();
    void setAutoDeepSleepTimeout(int value);
    void validateAutoDeepSleepTimeoutValue();
    void setBright(uint8_t value);
    void validateBrightValue();
    void setTmz(int value);
    void validateTmzValue();
    void setSoundEnabled(int value);
    void setSoundVolume(int value);
    void validateSoundEnabledValue();
    void validateSoundVolumeValue();
    void setWifiAtStartup(int value);
    void validateWifiAtStartupValue();

    // Led
    void setLedBright(int value);
    void validateLedBrightValue();
    void setLedColor(uint32_t value);
    void validateLedColorValue();
    void setLedBlinkEnabled(int value);
    void validateLedBlinkEnabledValue();
    void setLedEffect(int value);
    void validateLedEffectValue();
    void setLedEffectSpeed(int value);
    void validateLedEffectSpeedValue();
    void setLedEffectDirection(int value);
    void validateLedEffectDirectionValue();

    // Wifi
    void setWebUICreds(const String &usr, const String &pwd);
    void setWifiApCreds(const String &ssid, const String &pwd);
    void addWifiCredential(const String &ssid, const String &pwd);
    void addQrCodeEntry(const String &menuName, const String &content);
    void removeQrCodeEntry(const String &menuName);
    String getWifiPassword(const String &ssid) const;
    void addEvilWifiName(String value);
    void removeEvilWifiName(String value);

    // BLE
    void setBleName(const String name);

    // IR
    void setIrTxPin(int value);
    void setIrTxRepeats(uint8_t value);
    void setIrRxPin(int value);

    // RF
    void setRfTxPin(int value);
    void setRfRxPin(int value);
    void setRfModule(RFModules value);
    void validateRfModuleValue();
    void setRfFreq(float value, int fxdFreq = 2);
    void setRfFxdFreq(float value);
    void setRfScanRange(int value, int fxdFreq = 0);
    void validateRfScanRangeValue();

    // iButton
    void setiButtonPin(int value);

    // RFID
    void setRfidModule(RFIDModules value);
    void validateRfidModuleValue();
    void addMifareKey(String value);
    void validateMifareKeysItems();

    // GPS
    void setGpsBaudrate(int value);
    void validateGpsBaudrateValue();

    // Misc
    void setStartupApp(String value);
    void setWigleBasicToken(String value);
    void setDevMode(int value);
    void validateDevModeValue();
    void setColorInverted(int value);
    void validateColorInverted();
    void addDisabledMenu(String value);
    // TODO: removeDisabledMenu(String value);
};

#endif
