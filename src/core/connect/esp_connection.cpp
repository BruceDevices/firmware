#include <sdkconfig.h> // CONFIG_IDF_TARGET_ESP32P4 lives here; must precede the P4 guard below
#if !defined(LITE_VERSION) && !defined(CONFIG_IDF_TARGET_ESP32P4)
#include "core/display.h"
#include "esp_connection.h"
#include <WiFi.h>
#include <algorithm>
#include <cstring>

// Initialize the static instance pointer
EspConnection *EspConnection::instance = nullptr;
std::vector<Option> peerOptions;

EspConnection::EspConnection() { setInstance(this); }

EspConnection::~EspConnection() {
    esp_now_unregister_send_cb();
    esp_now_unregister_recv_cb();

    esp_now_deinit();
}

bool EspConnection::beginSend() {
    sendStatus = CONNECTING;

    if (!beginEspnow()) return false;

    sendPing();

    loopOptions(peerOptions);

    peerOptions.clear();

    if (!setupPeer(dstAddress)) {
        displayError("Failed to add peer");
        delay(1000);
        return false;
    }

    return true;
}

bool EspConnection::beginEspnow() {
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        displayError("Error initializing share");
        delay(1000);
        return false;
    }

    if (!setupPeer(broadcastAddress)) {
        displayError("Failed to add peer");
        delay(1000);
        return false;
    }

    esp_now_register_send_cb(onDataSentStatic);
    esp_now_register_recv_cb(onDataRecvStatic);

    return true;
}

EspConnection::Message EspConnection::createMessage(String text) {
    Message message;

    size_t maxLen = ESP_DATA_SIZE - 1;
    size_t copyLen = std::min<size_t>(text.length(), maxLen);
    message.dataSize = copyLen;
    message.totalBytes = copyLen;
    message.bytesSent = copyLen;
    message.done = true;

    memcpy(message.data, text.c_str(), copyLen);
    message.data[copyLen] = '\0';

    return message;
}

EspConnection::Message EspConnection::createFileMessage(File file) {
    Message message;
    String path = String(file.path());

    message.isFile = true;
    message.totalBytes = file.size();

    strncpy(message.filename, file.name(), ESP_FILENAME_SIZE - 1);
    message.filename[ESP_FILENAME_SIZE - 1] = '\0';
    strncpy(message.filepath, path.substring(0, path.lastIndexOf("/")).c_str(), ESP_FILEPATH_SIZE - 1);
    message.filepath[ESP_FILEPATH_SIZE - 1] = '\0';

    return message;
}

EspConnection::Message EspConnection::createPingMessage() {
    Message message;
    message.ping = true;

    return message;
}

EspConnection::Message EspConnection::createPongMessage() {
    Message message;
    message.pong = true;

    return message;
}

void EspConnection::sendPing() {
    peerOptions = {
        {"Broadcast", [this]() { setDstAddress(broadcastAddress); }},
    };

    Message message = createPingMessage();

    esp_err_t response = esp_now_send(broadcastAddress, (uint8_t *)&message, sizeof(message));
    if (response != ESP_OK) { Serial.printf("Send ping response: %s\n", esp_err_to_name(response)); }

    delay(500);
}

void EspConnection::sendPong(const uint8_t *mac) {
    Message message = createPongMessage();

    if (!setupPeer(mac)) return;

    esp_err_t response = esp_now_send(mac, (uint8_t *)&message, sizeof(message));
    if (response != ESP_OK) { Serial.printf("Send pong response: %s\n", esp_err_to_name(response)); }
}

bool EspConnection::setupPeer(const uint8_t *mac) {
    if (esp_now_is_peer_exist(mac)) return true;

    esp_now_peer_info_t peerInfo = {};

    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void EspConnection::printMessage(Message message) {
    delay(100);

    Serial.println("Message Details:");
    if (message.ping) {
        Serial.println("Ping: " + String(message.ping));
        Serial.println("");
        return;
    }
    if (message.pong) {
        Serial.println("Pong: " + String(message.pong));
        Serial.println("");
        return;
    }

    if (message.isFile) {
        Serial.println("Filename: " + String(message.filename));
        Serial.println("Filepath: " + String(message.filepath));
    }
    Serial.println("Data Size: " + String(message.dataSize));
    Serial.println("Total Bytes: " + String(message.totalBytes));
    Serial.println("Bytes Sent: " + String(message.bytesSent));
    Serial.println("Done: " + String(message.done));
    Serial.print("Data: ");

    // Append data to the result if dataSize is greater than 0
    size_t safeSize = std::min<size_t>(message.dataSize, ESP_DATA_SIZE);
    if (safeSize > 0) {
        for (size_t i = 0; i < safeSize; ++i) {
            Serial.print((char)message.data[i]); // Assuming data contains valid characters
        }
    } else {
        Serial.println("No data");
    }

    Serial.println("");
}

String EspConnection::macToString(const uint8_t *mac) {
    char macStr[18];
    snprintf(
        macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
    return macStr;
}

void EspConnection::appendPeerToList(const uint8_t *mac) {
    if (!mac || peerOptions.size() >= ESP_MAX_PEERS) return;

    String macLabel = macToString(mac);
    for (const auto &opt : peerOptions) {
        if (opt.label == macLabel) return;
    }

    uint8_t peerMac[6];
    memcpy(peerMac, mac, 6);
    peerOptions.push_back({macLabel.c_str(), [this, peerMac]() { setDstAddress(peerMac); }});
}

void EspConnection::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        sendStatus = SUCCESS;
        Serial.println("ESPNOW send success");
    } else {
        sendStatus = FAILED;
        Serial.println("ESPNOW send fail");
    }
}

void EspConnection::onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (!mac || !incomingData || len != static_cast<int>(sizeof(Message))) {
        Serial.printf(
            "ESPNOW: Dropping malformed frame (len=%d, expected=%u)\n", len, (unsigned)sizeof(Message)
        );
        return;
    }

    Message recvMessage;
    memcpy(&recvMessage, incomingData, sizeof(Message));

    if (recvMessage.dataSize >= ESP_DATA_SIZE) { recvMessage.dataSize = ESP_DATA_SIZE - 1; }
    recvMessage.filename[sizeof(recvMessage.filename) - 1] = '\0';
    recvMessage.filepath[sizeof(recvMessage.filepath) - 1] = '\0';
    recvMessage.data[recvMessage.dataSize] = '\0';
    recvMessage.data[ESP_DATA_SIZE - 1] = '\0';

    printMessage(recvMessage);

    if (recvMessage.ping) return sendPong(mac);
    if (recvMessage.pong) return appendPeerToList(mac);

    if (hasAuthorizedPeer && !isAuthorizedPeer(mac)) {
        Serial.printf("ESPNOW: Dropping frame from unauthorized peer %s\n", macToString(mac).c_str());
        return;
    }

    memcpy(lastSenderMac, mac, sizeof(lastSenderMac));
    recvQueue.push_back(recvMessage);
}

void EspConnection::onDataSentStatic(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    if (instance) instance->onDataSent(info->src_addr, status);
}

void EspConnection::onDataRecvStatic(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    if (instance) instance->onDataRecv(info->src_addr, incomingData, len);
}
#elif !defined(LITE_VERSION)
// ESP32-P4 has no ESP-NOW: its Wi-Fi is provided by the ESP-Hosted co-processor,
// which does not expose the esp_now_* API. Only the base ctor/dtor are defined so the
// derived stub classes (FileSharing / EspSerialCmd) still link.
#include "esp_connection.h"

EspConnection *EspConnection::instance = nullptr;

EspConnection::EspConnection() { setInstance(this); }
EspConnection::~EspConnection() {}
#endif
