#if !defined(LITE_VERSION)

#include "gatt_server.h"
#include "BLE_Suite.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <NimBLEDevice.h>
#include <globals.h>
#include <esp_mac.h>
#include <algorithm>
#include <deque>
#include <functional>
#include <vector>

//=============================================================================
// Constants & UUIDs
//=============================================================================

// Standard Services
#define UUID_SVC_DIS              "180A"
#define UUID_CHR_MFG_NAME         "2A29"
#define UUID_CHR_MODEL_NUM        "2A24"
#define UUID_CHR_SERIAL_NUM       "2A25"
#define UUID_CHR_FW_REV           "2A26"
#define UUID_CHR_HW_REV           "2A27"
#define UUID_CHR_SW_REV           "2A28"

#define UUID_SVC_BATTERY          "180F"
#define UUID_CHR_BATTERY_LEVEL    "2A19"

#define UUID_SVC_ENVIRONMENTAL    "181A"
#define UUID_CHR_TEMPERATURE      "2A6E"
#define UUID_CHR_HUMIDITY         "2A6F"

// Custom Echo & Stream Service
#define UUID_SVC_ECHO             "FFE0"
#define UUID_CHR_ECHO_RW          "FFE1"
#define UUID_CHR_STREAM_COUNT     "FFE2"

// Nordic UART Service (NUS)
#define UUID_SVC_NUS              "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UUID_CHR_NUS_RX           "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UUID_CHR_NUS_TX           "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

//=============================================================================
// State
//=============================================================================

struct GattServerState {
    volatile bool isRunning = false;
    volatile bool isConnected = false;
    String peerAddress = "None";
    uint16_t peerMtu = 23;
    uint32_t readCount = 0;
    uint32_t writeCount = 0;
    uint32_t notifyCount = 0;
    String lastWriteVal = "";
    std::deque<String> logLines;
    SemaphoreHandle_t logMutex = nullptr;

    void initMutex() {
        if (!logMutex) {
            logMutex = xSemaphoreCreateMutex();
        }
    }

    void addLog(const String &msg) {
        initMutex();
        if (logMutex && xSemaphoreTake(logMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            logLines.push_back(msg);
            while (logLines.size() > 25) {
                logLines.pop_front();
            }
            xSemaphoreGive(logMutex);
        }
    }

    void reset() {
        isRunning = false;
        isConnected = false;
        peerAddress = "None";
        peerMtu = 23;
        readCount = 0;
        writeCount = 0;
        notifyCount = 0;
        lastWriteVal = "";
        initMutex();
        if (logMutex && xSemaphoreTake(logMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            logLines.clear();
            xSemaphoreGive(logMutex);
        }
    }
};

static GattServerState g_srvState;

// Active characteristic pointers for dynamic updates
static NimBLECharacteristic *g_pBatChar = nullptr;
static NimBLECharacteristic *g_pTempChar = nullptr;
static NimBLECharacteristic *g_pHumChar = nullptr;
static NimBLECharacteristic *g_pEchoChar = nullptr;
static NimBLECharacteristic *g_pStreamChar = nullptr;
static NimBLECharacteristic *g_pNusTxChar = nullptr;

//=============================================================================
// UI Layout Helpers
//=============================================================================

struct SrvUiGeom {
    int listL, listW;
    int top;
    int rowH;
    int rows;
    int footY;
};

static SrvUiGeom srvUiGeom() {
    SrvUiGeom g;
    g.listL = 8;
    g.listW = tftWidth - 16;
    g.top = BORDER_PAD_Y + 8 * FM + 3;
    g.footY = tftHeight - 8 * FP - 6;
    g.rowH = 8 * FP + 5;
    int avail = g.footY - g.top - 2;
    if (avail < g.rowH) avail = g.rowH;
    g.rows = avail / g.rowH;
    if (g.rows < 1) g.rows = 1;
    return g;
}

static uint16_t srvDimColor() {
    return getColorVariation(bruceConfig.priColor, 8, -1);
}

static String srvFitText(const String &text, int maxPx) {
    if (maxPx <= 0) return "";
    tft.setTextSize(FP);
    if (tft.textWidth(text.c_str()) <= maxPx) return text;
    String s = text;
    while (s.length() > 1 && tft.textWidth((s + "..").c_str()) > maxPx) {
        s.remove(s.length() - 1);
    }
    return s + "..";
}

struct SrvMenuItem {
    String label;
    std::function<void()> action;
};

static int srvListLoop(
    const char *title, int count, const String &hint,
    std::function<void(int idx, int x, int y, int w, bool sel)> drawRow, int *cursor = nullptr
) {
    if (count <= 0) return -1;
    SrvUiGeom g = srvUiGeom();
    int sel = (cursor && *cursor >= 0 && *cursor < count) ? *cursor : 0;
    int off = 0, lastSel = -1, lastOff = -1;

    if (sel >= g.rows) off = sel - g.rows + 1;
    drawMainBorderWithTitle(title);

    for (;;) {
        if (sel != lastSel || off != lastOff) {
            tft.setTextSize(FP);
            for (int i = 0; i < g.rows; i++) {
                int y = g.top + i * g.rowH;
                int idx = off + i;
                bool selected = (idx == sel);
                tft.fillRect(
                    g.listL,
                    y - 2,
                    g.listW,
                    g.rowH,
                    selected ? bruceConfig.priColor : bruceConfig.bgColor
                );
                if (idx < count) drawRow(idx, g.listL + 3, y, g.listW - 6, selected);
            }

            // Position & hint footer
            tft.fillRect(g.listL, g.footY, g.listW, 8 * FP, bruceConfig.bgColor);
            tft.setTextSize(FP);
            String pos = String(sel + 1) + "/" + String(count);
            int posW = tft.textWidth(pos.c_str());
            tft.setTextColor(srvDimColor(), bruceConfig.bgColor);
            tft.drawString(srvFitText(hint, g.listW - posW - 8), g.listL, g.footY, 1);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawRightString(pos, g.listL + g.listW, g.footY, 1);

            lastSel = sel;
            lastOff = off;
            if (cursor) *cursor = sel;
        }

        if (check(EscPress)) return -1;
        else if (check(PrevPress) || check(UpPress)) sel = (sel > 0) ? sel - 1 : count - 1;
        else if (check(NextPress) || check(DownPress)) sel = (sel < count - 1) ? sel + 1 : 0;
        else if (check(SelPress)) {
            if (cursor) *cursor = sel;
            return sel;
        }

        if (sel < off) off = sel;
        if (sel >= off + g.rows) off = sel - g.rows + 1;
        if (off > count - g.rows) off = std::max(0, count - g.rows);
        if (off < 0) off = 0;
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

static int srvMenu(
    const char *title,
    const std::vector<SrvMenuItem> &items,
    const String &hint = "SEL choose  ESC back",
    int *cursor = nullptr
) {
    if (items.empty()) return -1;
    int count = items.size();
    auto drawer = [&items](int idx, int x, int y, int w, bool sel) {
        uint16_t fg = sel ? bruceConfig.bgColor : bruceConfig.priColor;
        uint16_t bg = sel ? bruceConfig.priColor : bruceConfig.bgColor;
        tft.setTextColor(fg, bg);
        tft.setTextSize(FP);
        tft.drawString(srvFitText(items[idx].label, w), x, y, 1);
    };

    int chosen = srvListLoop(title, count, hint, drawer, cursor);
    if (chosen >= 0 && chosen < count) {
        if (items[chosen].action) {
            items[chosen].action();
        }
    }
    return chosen;
}

//=============================================================================
// Server & Characteristic Callbacks
//=============================================================================

class BruceGattServerCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
        g_srvState.isConnected = true;
        g_srvState.peerAddress = String(connInfo.getAddress().toString().c_str());
        g_srvState.peerMtu = connInfo.getMTU();
        g_srvState.addLog("[CONN] " + g_srvState.peerAddress);
        Serial.printf("[GATT-SRV] Connected by: %s (MTU: %d)\n", g_srvState.peerAddress.c_str(), g_srvState.peerMtu);
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
        g_srvState.isConnected = false;
        String peer = String(connInfo.getAddress().toString().c_str());
        g_srvState.addLog("[DISC] " + peer + " (0x" + String(reason, HEX) + ")");
        Serial.printf("[GATT-SRV] Disconnected by %s (Reason: 0x%02X)\n", peer.c_str(), reason);

        // Automatically resume advertising
        if (g_srvState.isRunning && pServer && pServer->getAdvertising()) {
            pServer->getAdvertising()->start();
            g_srvState.addLog("[ADV] Advertising resumed");
        }
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override {
        g_srvState.peerMtu = MTU;
        g_srvState.addLog("[MTU] Updated: " + String(MTU) + " B");
        Serial.printf("[GATT-SRV] MTU changed to: %d\n", MTU);
    }
};

class GenericCharCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onRead(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override {
        g_srvState.readCount++;
        String uuidStr = pChar->getUUID().toString().c_str();
        g_srvState.addLog("[READ] " + uuidStr);
        Serial.printf("[GATT-SRV] Read Char: %s\n", uuidStr.c_str());
    }

    void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override {
        g_srvState.writeCount++;
        String uuidStr = pChar->getUUID().toString().c_str();
        std::string raw = pChar->getValue();
        String strVal = String(raw.c_str());
        g_srvState.lastWriteVal = strVal;

        String displayVal = strVal;
        if (displayVal.length() > 16) {
            displayVal = displayVal.substring(0, 14) + "..";
        }
        g_srvState.addLog("[WRITE] " + uuidStr + ": \"" + displayVal + "\"");
        Serial.printf("[GATT-SRV] Write Char %s -> '%s'\n", uuidStr.c_str(), strVal.c_str());

        // If this is Echo characteristic, update value and echo back via notification
        if (uuidStr.equalsIgnoreCase(UUID_CHR_ECHO_RW) || pChar == g_pEchoChar) {
            pChar->setValue("Echo: " + raw);
            if (g_srvState.isConnected) {
                pChar->notify();
                g_srvState.notifyCount++;
            }
        }
    }
};

class NusRxCharCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override {
        g_srvState.writeCount++;
        std::string raw = pChar->getValue();
        String strVal = String(raw.c_str());
        g_srvState.lastWriteVal = strVal;

        String displayVal = strVal;
        if (displayVal.length() > 18) {
            displayVal = displayVal.substring(0, 16) + "..";
        }
        g_srvState.addLog("[NUS RX] \"" + displayVal + "\"");
        Serial.printf("[GATT-SRV] NUS RX: '%s'\n", strVal.c_str());

        // Send ACK over NUS TX if available
        if (g_pNusTxChar && g_srvState.isConnected) {
            String ack = "ACK: " + strVal;
            g_pNusTxChar->setValue(ack.c_str());
            g_pNusTxChar->notify();
            g_srvState.notifyCount++;
            g_srvState.addLog("[NUS TX] Sent ACK");
        }
    }
};

static BruceGattServerCallbacks g_serverCallbacks;
static GenericCharCallbacks g_genericCharCallbacks;
static NusRxCharCallbacks g_nusRxCallbacks;

//=============================================================================
// Server Profiles & Service Initialization
//=============================================================================

static String getOwnMacString() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

static void buildServerServices(NimBLEServer *pServer, int profileMode) {
    g_pBatChar = nullptr;
    g_pTempChar = nullptr;
    g_pHumChar = nullptr;
    g_pEchoChar = nullptr;
    g_pStreamChar = nullptr;
    g_pNusTxChar = nullptr;

    bool addDis = (profileMode == 0 || profileMode == 1);
    bool addBat = (profileMode == 0 || profileMode == 1);
    bool addEnv = (profileMode == 0);
    bool addEcho = (profileMode == 0 || profileMode == 2);
    bool addNus = (profileMode == 0 || profileMode == 3);

    // 1. Device Information Service (0x180A)
    if (addDis) {
        NimBLEService *pDis = pServer->createService(UUID_SVC_DIS);
        if (pDis) {
            NimBLECharacteristic *pMfg = pDis->createCharacteristic(UUID_CHR_MFG_NAME, NIMBLE_PROPERTY::READ);
            if (pMfg) { pMfg->setValue("Bruce Project"); pMfg->setCallbacks(&g_genericCharCallbacks); }

            NimBLECharacteristic *pModel = pDis->createCharacteristic(UUID_CHR_MODEL_NUM, NIMBLE_PROPERTY::READ);
            if (pModel) { pModel->setValue("ESP32-Bruce"); pModel->setCallbacks(&g_genericCharCallbacks); }

            NimBLECharacteristic *pSerial = pDis->createCharacteristic(UUID_CHR_SERIAL_NUM, NIMBLE_PROPERTY::READ);
            if (pSerial) { pSerial->setValue(getOwnMacString().c_str()); pSerial->setCallbacks(&g_genericCharCallbacks); }

            NimBLECharacteristic *pFw = pDis->createCharacteristic(UUID_CHR_FW_REV, NIMBLE_PROPERTY::READ);
            if (pFw) { pFw->setValue("Bruce v2.0"); pFw->setCallbacks(&g_genericCharCallbacks); }

            NimBLECharacteristic *pHw = pDis->createCharacteristic(UUID_CHR_HW_REV, NIMBLE_PROPERTY::READ);
            if (pHw) { pHw->setValue("ESP32-D0WD/S3"); pHw->setCallbacks(&g_genericCharCallbacks); }
        }
    }

    // 2. Battery Service (0x180F)
    if (addBat) {
        NimBLEService *pBat = pServer->createService(UUID_SVC_BATTERY);
        if (pBat) {
            g_pBatChar = pBat->createCharacteristic(
                UUID_CHR_BATTERY_LEVEL,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
            );
            if (g_pBatChar) {
                int bat = getBattery();
                uint8_t batVal = (bat > 0 && bat <= 100) ? (uint8_t)bat : 98;
                g_pBatChar->setValue(&batVal, 1);
                g_pBatChar->setCallbacks(&g_genericCharCallbacks);
            }
        }
    }

    // 3. Environmental Sensing Service (0x181A)
    if (addEnv) {
        NimBLEService *pEnv = pServer->createService(UUID_SVC_ENVIRONMENTAL);
        if (pEnv) {
            g_pTempChar = pEnv->createCharacteristic(
                UUID_CHR_TEMPERATURE,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
            );
            if (g_pTempChar) {
                int16_t tempC = 2450; // 24.50 °C
                g_pTempChar->setValue((uint8_t *)&tempC, 2);
                g_pTempChar->setCallbacks(&g_genericCharCallbacks);
            }

            g_pHumChar = pEnv->createCharacteristic(
                UUID_CHR_HUMIDITY,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
            );
            if (g_pHumChar) {
                uint16_t humVal = 4800; // 48.00 %
                g_pHumChar->setValue((uint8_t *)&humVal, 2);
                g_pHumChar->setCallbacks(&g_genericCharCallbacks);
            }
        }
    }

    // 4. Custom Echo & Read/Write/Stream Service (0xFFE0)
    if (addEcho) {
        NimBLEService *pEcho = pServer->createService(UUID_SVC_ECHO);
        if (pEcho) {
            g_pEchoChar = pEcho->createCharacteristic(
                UUID_CHR_ECHO_RW,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY
            );
            if (g_pEchoChar) {
                g_pEchoChar->setValue("Bruce GATT Echo Char");
                g_pEchoChar->setCallbacks(&g_genericCharCallbacks);
            }

            g_pStreamChar = pEcho->createCharacteristic(
                UUID_CHR_STREAM_COUNT,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE
            );
            if (g_pStreamChar) {
                g_pStreamChar->setValue("Stream Tick #0");
                g_pStreamChar->setCallbacks(&g_genericCharCallbacks);
            }
        }
    }

    // 5. Nordic UART Service (NUS)
    if (addNus) {
        NimBLEService *pNus = pServer->createService(UUID_SVC_NUS);
        if (pNus) {
            NimBLECharacteristic *pNusRx = pNus->createCharacteristic(
                UUID_CHR_NUS_RX,
                NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
            );
            if (pNusRx) {
                pNusRx->setCallbacks(&g_nusRxCallbacks);
            }

            g_pNusTxChar = pNus->createCharacteristic(
                UUID_CHR_NUS_TX,
                NIMBLE_PROPERTY::NOTIFY
            );
            if (g_pNusTxChar) {
                g_pNusTxChar->setCallbacks(&g_genericCharCallbacks);
            }
        }
    }
}

//=============================================================================
// Live Server Service Control & Interactive Monitor
//=============================================================================

bool startGattServerService(int profileMode) {
    if (g_srvState.isRunning) {
        return true;
    }
    g_srvState.reset();
    g_srvState.isRunning = true;

    uint64_t chipid = ESP.getEfuseMac();
    String serverName = "Bruce-GATT-" + String((uint16_t)(chipid), HEX);
    serverName.toUpperCase();

    // 1. Initialize BLE with maximum power and zero authentication hurdles
    BLEStateManager::initBLE(serverName, ESP_PWR_LVL_P9);
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
    NimBLEDevice::setSecurityAuth(false, false, false);

    // 2. Create and configure Server
    NimBLEServer *pServer = NimBLEDevice::createServer();
    if (!pServer) {
        g_srvState.isRunning = false;
        return false;
    }

    pServer->setCallbacks(&g_serverCallbacks, false);

    // 3. Build active GATT database
    buildServerServices(pServer, profileMode);
    pServer->start();

    // 4. Configure Advertising (Connectable, Legacy 1M PHY for universal compatibility)
    NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
    pAdv->reset();
    pAdv->setName(serverName.c_str());

    // Advertise primary standard services in the 31-byte packet
    if (profileMode == 0 || profileMode == 1) {
        pAdv->addServiceUUID(UUID_SVC_BATTERY);
        pAdv->addServiceUUID(UUID_SVC_DIS);
    } else if (profileMode == 2) {
        pAdv->addServiceUUID(UUID_SVC_ECHO);
    } else if (profileMode == 3) {
        pAdv->addServiceUUID(UUID_SVC_NUS);
    }

    pAdv->enableScanResponse(true);
    pAdv->start();

    g_srvState.addLog("[INIT] Server name: " + serverName);
    g_srvState.addLog("[ADV] Connectable 1M PHY started");
    Serial.printf("[GATT-SRV] Server Started: %s (MAC: %s)\n", serverName.c_str(), getOwnMacString().c_str());
    return true;
}

void stopGattServerService() {
    if (!g_srvState.isRunning) return;
    g_srvState.isRunning = false;
    g_srvState.addLog("[STOP] Stopping server...");

    g_pBatChar = nullptr;
    g_pTempChar = nullptr;
    g_pHumChar = nullptr;
    g_pEchoChar = nullptr;
    g_pStreamChar = nullptr;
    g_pNusTxChar = nullptr;

    NimBLEServer *pServer = NimBLEDevice::getServer();
    if (pServer) {
        if (pServer->getAdvertising()) {
            pServer->getAdvertising()->stop();
        }
        std::vector<uint16_t> peers = pServer->getPeerDevices();
        for (uint16_t handle : peers) {
            pServer->disconnect(handle);
        }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
    BLEStateManager::deinitBLE(true);
    Serial.println(F("[GATT-SRV] Server Stopped."));
}

bool isGattServerActive() {
    return g_srvState.isRunning;
}

void runGattServer(int profileMode) {
    if (!startGattServerService(profileMode)) {
        displayError("Failed to create BLE Server");
        return;
    }

    String profileName = "All-in-One";
    if (profileMode == 1) profileName = "DIS & Battery";
    else if (profileMode == 2) profileName = "Echo & Stream";
    else if (profileMode == 3) profileName = "Nordic UART";

    uint64_t chipid = ESP.getEfuseMac();
    String serverName = "Bruce-GATT-" + String((uint16_t)(chipid), HEX);
    serverName.toUpperCase();
    String ownMac = getOwnMacString();

    // UI Initial Frame
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorderWithTitle("GATT SERVER (ONLINE)");

    uint32_t lastTickMs = millis();
    uint32_t tickCount = 0;
    bool lastConn = false;
    uint32_t lastReadCount = 0;
    uint32_t lastWriteCount = 0;
    uint32_t lastNotifyCount = 0;
    size_t lastLogSize = 0;

    int boxX = BORDER_PAD_X + 2;
    int boxW = tftWidth - 2 * BORDER_PAD_X - 4;
    int topY = STATUS_BAR_HEIGHT + 2;
    int headerH = 50;
    int logTopY = topY + headerH + 6;
    int logBottomY = tftHeight - 8 * FP - 8;
    int maxLogRows = (logBottomY - logTopY) / (8 * FP + 3);
    if (maxLogRows < 2) maxLogRows = 2;

    while (g_srvState.isRunning) {
        // Handle Periodic Data Updates & Live Streams (every 1000ms)
        uint32_t now = millis();
        if (now - lastTickMs >= 1000) {
            lastTickMs = now;
            tickCount++;

            // Stream Characteristic Tick
            if (g_pStreamChar) {
                String tickMsg = "Tick #" + String(tickCount);
                g_pStreamChar->setValue(tickMsg.c_str());
                if (g_srvState.isConnected) {
                    g_pStreamChar->notify();
                    g_srvState.notifyCount++;
                }
            }

            // Simulated temperature variation (e.g. 24.50 C .. 25.20 C)
            if (g_pTempChar) {
                int16_t tempC = 2450 + (int16_t)((tickCount % 8) * 10);
                g_pTempChar->setValue((uint8_t *)&tempC, 2);
            }

            // Update Battery Level
            if (g_pBatChar) {
                int bat = getBattery();
                uint8_t batVal = (bat > 0 && bat <= 100) ? (uint8_t)bat : (uint8_t)(95 - (tickCount / 60) % 10);
                g_pBatChar->setValue(&batVal, 1);
            }
        }

        // Draw / Refresh Status Header
        bool conn = g_srvState.isConnected;
        bool needHeaderRedraw = (conn != lastConn) || (g_srvState.readCount != lastReadCount) ||
                                (g_srvState.writeCount != lastWriteCount) || (g_srvState.notifyCount != lastNotifyCount);

        if (needHeaderRedraw) {
            lastConn = conn;
            lastReadCount = g_srvState.readCount;
            lastWriteCount = g_srvState.writeCount;
            lastNotifyCount = g_srvState.notifyCount;

            tft.fillRect(boxX, topY, boxW, headerH, bruceConfig.bgColor);
            tft.setTextSize(FP);

            // Row 1: Name & Profile
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawString(srvFitText("Name: " + serverName, boxW / 2 + 10), boxX, topY, 1);
            tft.setTextColor(srvDimColor(), bruceConfig.bgColor);
            tft.drawRightString(srvFitText("[" + profileName + "]", boxW / 2 - 10), boxX + boxW, topY, 1);

            // Row 2: MAC & Status Banner
            int r2Y = topY + 8 * FP + 3;
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawString("MAC: " + ownMac, boxX, r2Y, 1);

            if (conn) {
                tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                tft.drawRightString("[CONNECTED]", boxX + boxW, r2Y, 1);
            } else {
                tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                tft.drawRightString("[ADVERTISING]", boxX + boxW, r2Y, 1);
            }

            // Row 3: Peer / Statistics
            int r3Y = r2Y + 8 * FP + 3;
            if (conn) {
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.drawString(srvFitText("Peer: " + g_srvState.peerAddress + " (MTU " + String(g_srvState.peerMtu) + ")", boxW), boxX, r3Y, 1);
            } else {
                tft.setTextColor(srvDimColor(), bruceConfig.bgColor);
                tft.drawString("Ready for connections (1M PHY)", boxX, r3Y, 1);
            }

            // Row 4: Counters
            int r4Y = r3Y + 8 * FP + 3;
            String statStr = "Reads: " + String(g_srvState.readCount) + "  Writes: " + String(g_srvState.writeCount) + "  Notifs: " + String(g_srvState.notifyCount);
            tft.setTextColor(srvDimColor(), bruceConfig.bgColor);
            tft.drawString(statStr, boxX, r4Y, 1);

            // Divider
            tft.drawFastHLine(boxX, topY + headerH + 2, boxW, srvDimColor());
        }

        // Draw / Refresh Event Log Window
        size_t curLogSize = 0;
        if (g_srvState.logMutex && xSemaphoreTake(g_srvState.logMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            curLogSize = g_srvState.logLines.size();
            xSemaphoreGive(g_srvState.logMutex);
        }

        if (curLogSize != lastLogSize || needHeaderRedraw) {
            lastLogSize = curLogSize;
            tft.fillRect(boxX, logTopY, boxW, logBottomY - logTopY, bruceConfig.bgColor);
            tft.setTextSize(FP);

            std::vector<String> linesToDraw;
            if (g_srvState.logMutex && xSemaphoreTake(g_srvState.logMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                int startIdx = std::max(0, (int)g_srvState.logLines.size() - maxLogRows);
                for (size_t i = startIdx; i < g_srvState.logLines.size(); i++) {
                    linesToDraw.push_back(g_srvState.logLines[i]);
                }
                xSemaphoreGive(g_srvState.logMutex);
            }

            int row = 0;
            for (const auto &line : linesToDraw) {
                int lineY = logTopY + row * (8 * FP + 3);

                if (line.startsWith("[CONN]")) tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                else if (line.startsWith("[DISC]")) tft.setTextColor(TFT_RED, bruceConfig.bgColor);
                else if (line.startsWith("[WRITE]")) tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
                else if (line.startsWith("[READ]")) tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                else if (line.startsWith("[ADV]")) tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                else tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

                tft.drawString(srvFitText(line, boxW), boxX, lineY, 1);
                row++;
            }

            // Footer
            tft.fillRect(boxX, logBottomY + 2, boxW, 8 * FP, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setTextColor(srvDimColor(), bruceConfig.bgColor);
            tft.drawString("ESC: Stop Server", boxX, logBottomY + 2, 1);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawRightString("SEL: Manual Notify", boxX + boxW, logBottomY + 2, 1);
        }

        // Handle User Input
        if (check(EscPress)) {
            g_srvState.isRunning = false;
            break;
        }

        if (check(SelPress)) {
            // Send Manual Test Notification
            if (g_pEchoChar && g_srvState.isConnected) {
                String manualMsg = "Manual Test Event #" + String(tickCount);
                g_pEchoChar->setValue(manualMsg.c_str());
                g_pEchoChar->notify();
                g_srvState.notifyCount++;
                g_srvState.addLog("[MANUAL NOTIFY] Sent");
            } else if (!g_srvState.isConnected) {
                g_srvState.addLog("[WARN] No client connected");
            }
            vTaskDelay(150 / portTICK_PERIOD_MS);
        }

        vTaskDelay(40 / portTICK_PERIOD_MS);
    }

    // Cleanup and Stop Server
    stopGattServerService();
    displayInfo("GATT Server Stopped", false);
    vTaskDelay(400 / portTICK_PERIOD_MS);
}

//=============================================================================
// Main GATT Server Menu
//=============================================================================

void gattServerMenu() {
    int cursor = 0;
    while (true) {
        std::vector<SrvMenuItem> menuOps;

        menuOps.push_back({"1. Multi-Service Server (All-in-One)", [=]() {
            runGattServer(0);
        }});

        menuOps.push_back({"2. Standard DIS & Battery Server", [=]() {
            runGattServer(1);
        }});

        menuOps.push_back({"3. Custom Echo & Stream Server", [=]() {
            runGattServer(2);
        }});

        menuOps.push_back({"4. Nordic UART Serial (NUS)", [=]() {
            runGattServer(3);
        }});

        menuOps.push_back({"< Back to Bluetooth", []() {}});

        int sel = srvMenu("GATT SERVER", menuOps, "SEL start  ESC back", &cursor);
        if (sel == -1 || sel == (int)menuOps.size() - 1) {
            break;
        }
    }
}

#endif // !LITE_VERSION
