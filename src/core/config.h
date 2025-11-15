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

enum EvilPortalPasswordMode { FULL_PASSWORD = 0, FIRST_LAST_CHAR = 1, HIDE_PASSWORD = 2, SAVE_LENGTH = 3 };

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
    struct EvilPortalEndpoints {
        String getCredsEndpoint;
        String setSsidEndpoint;
        bool showEndpoints;
        bool allowSetSsid;
        bool allowGetCreds;
    };

    const char *filepath = "/bruce.conf";

    // Settings
    int rotation = ROTATION > 1 ? 3 : 1;
    int dimmerSet = 10;
    int bright = 100;
    float tmz = 0;
    int soundEnabled = 1;
    int soundVolume = 100;
    int wifiAtStartup = 0;
    int instantBoot = 0;

#ifdef HAS_RGB_LED
    // Led
    int ledBright = 50;
    uint32_t ledColor = 0x960064;
    int ledBlinkEnabled = 1;
    int ledEffect = 0;
    int ledEffectSpeed = 5;
    int ledEffectDirection = 1;
#endif

    // Wifi
    Credential webUI = {"admin", "bruce"};
    std::vector<String> webUISessions = {}; // FIFO queue of session tokens
    WiFiCredential wifiAp = {"BruceNet", "brucenet"};
    std::map<String, String> wifi = {};
    std::set<String> evilWifiNames = {};
    String wifiMAC = ""; //@IncursioHack

    // EvilPortal
    EvilPortalEndpoints evilPortalEndpoints = {"/creds", "/ssid", true, true, true};
    EvilPortalPasswordMode evilPortalPasswordMode = FULL_PASSWORD;

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
    int badUSBBLEKeyboardLayout = 0;
    int badUSBBLEKeyDelay = 50;
    bool badUSBBLEShowOutput = true;

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
    void setBright(uint8_t value);
    void validateBrightValue();
    void setTmz(float value);
    void validateTmzValue();
    void setSoundEnabled(int value);
    void setSoundVolume(int value);
    void validateSoundEnabledValue();
    void validateSoundVolumeValue();
    void setWifiAtStartup(int value);
    void validateWifiAtStartupValue();

#ifdef HAS_RGB_LED
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
#endif

    // Wifi
    void setWebUICreds(const String &usr, const String &pwd);
    void setWifiApCreds(const String &ssid, const String &pwd);
    void addWifiCredential(const String &ssid, const String &pwd);
    void addQrCodeEntry(const String &menuName, const String &content);
    void removeQrCodeEntry(const String &menuName);
    String getWifiPassword(const String &ssid) const;
    void addEvilWifiName(String value);
    void removeEvilWifiName(String value);
    void setEvilEndpointCreds(String value);
    void setEvilEndpointSsid(String value);
    void setEvilAllowEndpointDisplay(bool value);
    void setEvilAllowGetCreds(bool value);
    void setEvilAllowSetSsid(bool value);
    void setEvilPasswordMode(EvilPortalPasswordMode value);
    void validateEvilEndpointCreds();
    void validateEvilEndpointSsid();
    void validateEvilPasswordMode();

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
    void setBadUSBBLEKeyboardLayout(int value);
    void validateBadUSBBLEKeyboardLayout();
    void setBadUSBBLEKeyDelay(int value);
    void validateBadUSBBLEKeyDelay();
    void setBadUSBBLEShowOutput(bool value);
    void addDisabledMenu(String value);
    // TODO: removeDisabledMenu(String value);

    void addWebUISession(const String &token);
    void removeWebUISession(const String &token);
    bool isValidWebUISession(const String &token);
};

#endif
