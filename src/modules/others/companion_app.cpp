#include "companion_app.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/serialcmds.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include <globals.h>
#include <SerialDevice.h>
#include <esp_mac.h>
#include <nvs.h>
#include <Preferences.h>
#include <FS.h>
#include <SD.h>
#include <LittleFS.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

#define SERVICE_UUID         "B1234567-89AB-CDEF-0123-456789ABCDEF"
#define AUTH_CHAR_UUID       "A1234567-89AB-CDEF-0123-456789ABCDEF"
#define HW_INFO_CHAR_UUID    "C1234567-89AB-CDEF-0123-456789ABCDEF"
#define BATT_CHAR_UUID       "D1234567-89AB-CDEF-0123-456789ABCDEF"
#define TERM_TX_CHAR_UUID    "E1234567-89AB-CDEF-0123-456789ABCDEF"
#define TERM_RX_CHAR_UUID    "F1234567-89AB-CDEF-0123-456789ABCDEF"
#define SCREEN_CHAR_UUID     "99999999-89AB-CDEF-0123-456789ABCDEF"

#define NVS_NS        "companion"
#define NVS_KEY_TOKEN "app_token"

#define BATT_TASK_INTERVAL_MS 10000

bool companionActive = false;

// ===== BLE State =====
static BLEServer          *pSrv       = NULL;
static BLEService         *pSvc       = NULL;
static BLECharacteristic  *pAuth      = NULL;
static BLECharacteristic  *pHwInfo    = NULL;
static BLECharacteristic  *pBatt      = NULL;
static BLECharacteristic  *pTermTx    = NULL;
static BLECharacteristic  *pTermRx    = NULL;
static BLECharacteristic  *pScreenChar = NULL;

static bool    bleInited       = false;
static bool    isAdv           = false;
static bool    clientConnected = false;
static uint16_t clientConnHandle = 0;
static String  deviceName      = "";
static TaskHandle_t battTaskHandle = NULL;
static TaskHandle_t screenTaskHandle = NULL;
static volatile bool screenStreamActive = false;

static bool    isBleAuthenticated = false;
static String  currentBlePin      = "";

static TaskHandle_t pinOverlayTaskHandle = NULL;
static void pinOverlayTaskFunc(void *param);

static File uploadFile;

// ===== NVS Direct API =====

static String readToken() {
    nvs_handle_t h;
    String result;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t len = 0;
        if (nvs_get_str(h, NVS_KEY_TOKEN, NULL, &len) == ESP_OK && len > 0) {
            char *buf = (char *)malloc(len);
            if (buf) {
                nvs_get_str(h, NVS_KEY_TOKEN, buf, &len);
                result = String(buf);
                free(buf);
            }
        }
        nvs_close(h);
    }
    return result;
}

static void writeToken(const String &token) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_TOKEN, token.c_str());
        nvs_commit(h);
        nvs_close(h);
    }
}

static void eraseToken() {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_key(h, NVS_KEY_TOKEN);
        nvs_commit(h);
        nvs_close(h);
    }
}

static bool hasToken() { return readToken().length() > 0; }

// ===== Toast =====

static char    toastMsg[64]    = "";
static uint32_t toastTimeout   = 0;

static void setToast(const char *msg) {
    strncpy(toastMsg, msg, sizeof(toastMsg) - 1);
    toastMsg[sizeof(toastMsg) - 1] = '\0';
    toastTimeout = millis() + 1500;
}

void checkCompanionToast() {
    if (toastTimeout && millis() < toastTimeout) {
        tft.fillScreen(TFT_DARKGREEN);
        tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        tft.setTextSize(FM);
        tft.drawCentreString(toastMsg, tftWidth / 2, tftHeight / 2 - 8, SMOOTH_FONT);
        delay(1500);
        toastTimeout = 0;
        toastMsg[0] = '\0';
    }
}

// ===== Serial Bridge =====

class CompanionSerialBridge : public SerialDevice {
public:
    SerialDevice *orig;
    BLECharacteristic *rxChar;

    CompanionSerialBridge(SerialDevice *o, BLECharacteristic *r) : orig(o), rxChar(r) {}

    size_t println(const String &s) override {
        size_t r = orig->println(s);
        if (clientConnected && rxChar) {
            rxChar->notify(String(s + "\r\n"));
        }
        return r;
    }

    size_t println(size_t n) override { return println(String(n)); }
    size_t println(uint32_t n) override { return println(String(n)); }

    size_t println() override {
        size_t r = orig->println();
        if (clientConnected && rxChar) rxChar->notify(String("\r\n"));
        return r;
    }

    size_t println(int n, int format) override { return println(String(n, format)); }
    size_t print(const String &s) override {
        size_t r = orig->print(s);
        if (clientConnected && rxChar) rxChar->notify(s);
        return r;
    }

    size_t print(int n, int format) override { return print(String(n, format)); }

    void vprintf(const char *fmt, va_list args) override {
        va_list copy;
        va_copy(copy, args);
        orig->vprintf(fmt, args);
        if (clientConnected && rxChar) {
            char buf[256];
            vsnprintf(buf, sizeof(buf), fmt, copy);
            rxChar->notify(String(buf));
        }
        va_end(copy);
    }

    size_t write(uint8_t *str, size_t size) override {
        size_t r = orig->write(str, size);
        if (clientConnected && rxChar) rxChar->notify(str, size);
        return r;
    }

    void flush() override { orig->flush(); }
    int available() override { return orig->available(); }
    String readStringUntil(char terminator) override { return orig->readStringUntil(terminator); }
};

static CompanionSerialBridge *serialBridge = NULL;

// ===== BLE Callbacks =====

class CompanionServerCB : public BLEServerCallbacks {
    void onConnect(BLEServer *server, NimBLEConnInfo &connInfo) override {
        clientConnected = true;
        clientConnHandle = connInfo.getConnHandle();
        isAdv = false;
        server->updateConnParams(clientConnHandle, 24, 40, 0, 400);

        isBleAuthenticated = false;

        {
            Preferences prefs;
            prefs.begin("bruce_auth", false);
            currentBlePin = prefs.getString("ble_pin", "");
            if (currentBlePin.length() == 0) {
                currentBlePin = String((esp_random() % 9000) + 1000);
                prefs.putString("ble_pin", currentBlePin);
            }
            prefs.end();
        }

        if (pAuth) {
            pAuth->setValue("LOCKED");
            pAuth->notify();
        }

        if (pinOverlayTaskHandle) {
            vTaskDelete(pinOverlayTaskHandle);
            pinOverlayTaskHandle = NULL;
        }
        xTaskCreatePinnedToCore(pinOverlayTaskFunc, "pinOverlay", 2048, NULL, 1, &pinOverlayTaskHandle, 1);
    }

    void onDisconnect(BLEServer *server, NimBLEConnInfo &connInfo, int reason) override {
        if (uploadFile) uploadFile.close();
        clientConnected = false;
        clientConnHandle = 0;
        server->startAdvertising();
        isAdv = true;
        isBleAuthenticated = false;
        currentBlePin = "";
        if (pinOverlayTaskHandle) {
            vTaskDelete(pinOverlayTaskHandle);
            pinOverlayTaskHandle = NULL;
        }
    }
};

class CompanionAuthCB : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *ch, NimBLEConnInfo &connInfo) override {
        if (isBleAuthenticated) ch->setValue("UNLOCKED");
        else if (currentBlePin.length() > 0) ch->setValue("LOCKED");
        else ch->setValue("NO_PIN");
    }

    void onWrite(BLECharacteristic *ch, NimBLEConnInfo &connInfo) override {
        NimBLEAttValue att = ch->getValue();
        std::string val = (std::string)att;
        if (val.empty()) return;
        String pin = String(val.c_str());

        if (pin == currentBlePin) {
            isBleAuthenticated = true;
            ch->setValue("UNLOCKED");
            ch->notify();
            currentBlePin = "";
        } else {
            ch->setValue("WRONG_PIN");
            ch->notify();
        }
    }
};

class CompanionHwInfoCB : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *ch, NimBLEConnInfo &connInfo) override {
#if defined(DEVICE_NAME)
        ch->setValue(DEVICE_NAME);
#else
        ch->setValue("Unknown Device");
#endif
    }
};

// 1 FPS BLE screen stream task using getBinLog
static void bleScreenTaskFunc(void *param) {
    while (screenStreamActive) {
        if (clientConnected && isBleAuthenticated) {
            uint8_t binBuffer[1024];
            size_t binSize = 0;
            tft.getBinLog(binBuffer, binSize);
            if (binSize > 0) {
                pScreenChar->setValue(binBuffer, binSize);
                pScreenChar->notify();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

// ===== PIN Overlay Task =====
// Periodically draws the BLE pairing PIN as a notification bar
// at the bottom of the screen, on top of whatever the menu shows.
static void pinOverlayTaskFunc(void *param) {
    uint32_t startTime = millis();
    while (true) {
        if (clientConnected && !isBleAuthenticated && currentBlePin.length() > 0
            && millis() - startTime < 20000) {
            tft.native()->startWrite();
            tft.fillRect(0, tftHeight - 20, tftWidth, 20, TFT_DARKGREEN);
            tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
            tft.setTextSize(FM);
            tft.drawCentreString("BLE PIN: " + currentBlePin, tftWidth / 2, tftHeight - 16, SMOOTH_FONT);
            tft.native()->endWrite();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ===== File System Commands =====

static void handleFSCommand(const String &cmd) {
    // Send a response line over BLE terminal
    auto resp = [](const String &s) {
        if (pTermRx) pTermRx->notify(s + "\r\n");
    };

    if (cmd == "FS_UPLOAD_END") {
        if (uploadFile) {
            uploadFile.close();
            resp("FS_UPLOAD_SUCCESS");
        }
        return;
    }

    if (cmd.startsWith("FS_UPLOAD_CHUNK ")) {
        if (!uploadFile) return;
        String hexStr = cmd.substring(16);
        hexStr.trim();
        size_t byteLen = hexStr.length() / 2;
        if (byteLen == 0) return;
        uint8_t *buf = (uint8_t *)malloc(byteLen);
        if (!buf) return;
        for (size_t i = 0; i < byteLen; i++) {
            char byteStr[3] = { hexStr[i * 2], hexStr[i * 2 + 1], 0 };
            buf[i] = strtol(byteStr, NULL, 16);
        }
        uploadFile.write(buf, byteLen);
        free(buf);
        return;
    }

    if (cmd.startsWith("FS_UPLOAD_START ")) {
        String rest = cmd.substring(16);
        rest.trim();
        int spaceIdx = rest.lastIndexOf(' ');
        if (spaceIdx < 0) return;
        String path = rest.substring(0, spaceIdx);
        if (path.length() == 0) return;

        if (uploadFile) uploadFile.close();

        FS *fs;
        if (!getFsStorage(fs)) { resp("FS_UPLOAD_FAIL"); return; }

        String dirPath;
        int slash = path.lastIndexOf('/');
        if (slash > 0) dirPath = path.substring(0, slash);
        if (dirPath.length() > 0) fs->mkdir(dirPath);

        uploadFile = fs->open(path, FILE_WRITE);
        if (!uploadFile) resp("FS_UPLOAD_FAIL");
        return;
    }

    if (cmd.startsWith("FS_REMOVE ")) {
        String path = cmd.substring(10);
        path.trim();
        FS *fs;
        if (getFsStorage(fs)) deleteFromSd(*fs, path);
        return;
    }

    if (cmd.startsWith("FS_MKDIR ")) {
        String path = cmd.substring(9);
        path.trim();
        FS *fs;
        if (getFsStorage(fs)) fs->mkdir(path);
        return;
    }

    if (cmd.startsWith("FS_LIST ")) {
        String path = cmd.substring(8);
        path.trim();
        if (path.length() == 0) path = "/";

        FS *fs;
        if (!getFsStorage(fs)) { resp("FS_LIST_DONE"); return; }

        File dir = fs->open(path);
        if (!dir || !dir.isDirectory()) { resp("FS_LIST_DONE"); return; }

        File entry;
        while (entry = dir.openNextFile()) {
            String line = "FILE_ITEM:" + String(entry.name()) + "|"
                        + String(entry.isDirectory() ? 1 : 0) + "|"
                        + String(entry.size());
            resp(line);
            entry.close();
        }
        dir.close();
        resp("FS_LIST_DONE");
        return;
    }

    if (cmd.startsWith("FS_DOWNLOAD_START ")) {
        String path = cmd.substring(18);
        path.trim();
        if (path.length() == 0) return;

        FS *fs;
        if (!getFsStorage(fs)) { resp("FS_DOWNLOAD_END"); return; }

        File dlFile = fs->open(path, FILE_READ);
        if (!dlFile) { resp("FS_DOWNLOAD_END"); return; }

        String fname = path;
        int lastSlash = fname.lastIndexOf('/');
        if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);
        resp("FS_DOWNLOAD_INIT:" + fname);

        uint8_t buf[200];
        while (dlFile.available()) {
            size_t bytesRead = dlFile.read(buf, sizeof(buf));
            if (bytesRead == 0) break;

            String hexLine = "FS_DOWNLOAD_CHUNK:";
            for (size_t i = 0; i < bytesRead; i++) {
                if (buf[i] < 16) hexLine += '0';
                hexLine += String(buf[i], HEX);
            }
            resp(hexLine);
            delay(5);
        }
        dlFile.close();
        resp("FS_DOWNLOAD_END");
        return;
    }
}

// ===== App Command Normalizer =====
// Translates Swift QuickActions-style commands (e.g. "WIFI_SCAN", "IR_RX")
// to Bruce CLI format (e.g. "wifi scan", "ir rx") with specific remapping.

static void handleAppCommand(const String &cmd) {
    String c = cmd;
    c.toLowerCase();
    c.replace('_', ' ');
    c.trim();
    if (c.length() == 0) return;

    static const std::vector<std::pair<String, String>> remap = {
        {"wifi scan",                  "sniffer"},
        {"wifi deauth",                "sniffer"},
        {"blespam apple continuity",   "blespam apple"},
        {"blespam samsung",            "blespam samsung"},
        {"blespam android",            "blespam android"},
        {"blespam windows",            "blespam windows"},
        {"ir rec",                     "ir rx"},
        {"rf analyze",                 "rf scan"},
        {"rf record",                  "rf rx"},
    };

    for (auto &m : remap) {
        if (c == m.first) { parseSerialCommand(m.second); return; }
    }

    parseSerialCommand(c);
}

class CompanionTermTxCB : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *ch, NimBLEConnInfo &connInfo) override {
        NimBLEAttValue att = ch->getValue();
        std::string val = (std::string)att;
        if (val.empty()) return;
        String cmd = String(val.c_str());

        if (!isBleAuthenticated) return;

        if (cmd == "START_BLE_SCREEN") {
            if (!screenStreamActive && pScreenChar) {
                tft.setLogging(true);
                screenStreamActive = true;
                xTaskCreatePinnedToCore(bleScreenTaskFunc, "bleScreenTask", 4096, NULL, 1, &screenTaskHandle, 1);
            }
            return;
        }
        if (cmd == "STOP_BLE_SCREEN") {
            screenStreamActive = false;
            if (screenTaskHandle) {
                vTaskDelay(pdMS_TO_TICKS(100));
                screenTaskHandle = NULL;
            }
            tft.setLogging(false);
            return;
        }
        if (cmd == "SCREEN_KEEP_ALIVE") {
            AnyKeyPress = true;
            SerialCmdPress = true;
            return;
        }
        if (cmd == "BTN_UP") { UpPress = true; AnyKeyPress = true; return; }
        if (cmd == "BTN_DOWN") { DownPress = true; AnyKeyPress = true; return; }
        if (cmd == "BTN_LEFT") { PrevPress = true; AnyKeyPress = true; return; }
        if (cmd == "BTN_RIGHT") { NextPress = true; AnyKeyPress = true; return; }
        if (cmd == "BTN_ACTION") { SelPress = true; AnyKeyPress = true; return; }

        if (cmd.startsWith("FS_")) { handleFSCommand(cmd); return; }

        if (clientConnected && pTermRx) {
            pTermRx->notify(String("Selected: " + cmd + "\r\n"));
        }
        handleAppCommand(cmd);
    }
};

// ===== Battery Task =====

static void batteryTaskFunc(void *param) {
    BLECharacteristic *batChar = (BLECharacteristic *)param;
    while (true) {
        if (clientConnected) {
            uint8_t level = (uint8_t)getBattery();
            batChar->setValue(&level, 1);
            batChar->notify();
        }
        vTaskDelay(pdMS_TO_TICKS(BATT_TASK_INTERVAL_MS));
    }
}

// ===== BLE Lifecycle =====

static void initCompanionBLE() {
    if (bleInited) return;

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char suffix[8];
    snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
    deviceName = "Bruce_" + String(suffix);

    BLEDevice::init(deviceName.c_str());
    NimBLEDevice::setMTU(512);

    pSrv = BLEDevice::createServer();
    pSrv->setCallbacks(new CompanionServerCB());

    pSvc = pSrv->createService(SERVICE_UUID);

    pAuth = pSvc->createCharacteristic(
        AUTH_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY
    );
    pAuth->setCallbacks(new CompanionAuthCB());

    pHwInfo = pSvc->createCharacteristic(HW_INFO_CHAR_UUID, NIMBLE_PROPERTY::READ);
    pHwInfo->setCallbacks(new CompanionHwInfoCB());

    pBatt = pSvc->createCharacteristic(
        BATT_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    {
        uint8_t initLevel = (uint8_t)getBattery();
        pBatt->setValue(&initLevel, 1);
    }

    pTermTx = pSvc->createCharacteristic(
        TERM_TX_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pTermTx->setCallbacks(new CompanionTermTxCB());

    pTermRx = pSvc->createCharacteristic(TERM_RX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

    pScreenChar = pSvc->createCharacteristic(SCREEN_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

    xTaskCreatePinnedToCore(batteryTaskFunc, "companion_bat", 2048, pBatt, 1, &battTaskHandle, 1);

    if (!serialBridge) {
        serialBridge = new CompanionSerialBridge(serialDevice, pTermRx);
        serialDevice = serialBridge;
    }

    bleInited = true;
}

static void startAdv() {
    if (!pSrv) return;
    BLEAdvertising *a = pSrv->getAdvertising();
    a->stop();

    BLEAdvertisementData d;
    d.setCompleteServices(BLEUUID(SERVICE_UUID));
    d.setName(std::string(deviceName.c_str()));
    a->setAdvertisementData(d);
    a->start();
    isAdv = true;
    BLEConnected = true;
}

static void stopScreenStream() {
    screenStreamActive = false;
    if (screenTaskHandle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        screenTaskHandle = NULL;
    }
    tft.setLogging(false);
}

static void stopCompanionBLE() {
    if (uploadFile) uploadFile.close();
    stopScreenStream();
    if (pSrv) { pSrv->getAdvertising()->stop(); isAdv = false; }
    if (serialBridge) {
        serialDevice = serialBridge->orig;
        delete serialBridge;
        serialBridge = NULL;
    }
    if (battTaskHandle) { vTaskDelete(battTaskHandle); battTaskHandle = NULL; }
    if (pinOverlayTaskHandle) { vTaskDelete(pinOverlayTaskHandle); pinOverlayTaskHandle = NULL; }
    if (bleInited) { BLEDevice::deinit(); bleInited = false; }
    pSrv = NULL; pSvc = NULL;
    pAuth = NULL; pHwInfo = NULL; pBatt = NULL;
    pTermTx = NULL; pTermRx = NULL; pScreenChar = NULL;
    isBleAuthenticated = false; currentBlePin = "";
    clientConnected = false; isAdv = false;
    companionActive = false; BLEConnected = false;
}

// ===== Entry Point =====

void companion_app_setup() {
    if (!bleInited) {
        initCompanionBLE();
        startAdv();
        companionActive = true;
    }

    if (!isAdv) startAdv();
    tft.native()->startWrite();
    tft.native()->endWrite();

    while (true) {
        String bleStatus = "BLE: " + String(clientConnected ? "Connected" : "Waiting");
        if (clientConnected && !isBleAuthenticated && currentBlePin.length() > 0) {
            bleStatus = "BLE PIN: " + currentBlePin;
        }
        options = {
            {bleStatus, []() {}},
            {"Delete App", [=]() {
                eraseToken();
                if (clientConnected && pSrv) {
                    pSrv->disconnect(clientConnHandle);
                }
                stopScreenStream();
                displaySuccess("App Deleted");
                delay(800);
            }},
            {"Stop Service", [=]() {
                stopCompanionBLE();
                backToMenu();
            }},
        };
        addOptionToMainMenu();
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, "Companion App");
        if (sel < 0 || sel >= (int)options.size() - 1) break;
        if (sel == 2) break;
    }
}

void companion_service_setup() {
    if (!bleInited) {
        initCompanionBLE();
        startAdv();
        companionActive = true;
    }
    if (!isAdv) startAdv();
    backToMenu();
}
