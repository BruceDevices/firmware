// Original: https://github.com/mathcampbell/ANCS
#include <Arduino.h>
#include "NimBLEDevice.h"

static NimBLEUUID ancsServiceUUID("7905F431-B5CE-4E99-A40F-4B1E122D00D0");
static NimBLEUUID notificationSourceCharacteristicUUID("9FBF120D-6301-42D9-8C58-25E699A21DBD");
static NimBLEUUID controlPointCharacteristicUUID("69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9");
static NimBLEUUID dataSourceCharacteristicUUID("22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB");

static NimBLEClient* pClient;

uint8_t latestMessageID[4];
boolean pendingNotification = false;
boolean incomingCall        = false;
uint8_t acceptCall          = 0;

static void dataSourceNotifyCallback(NimBLERemoteCharacteristic* pDataSourceCharacteristic,
                                     uint8_t*                    pData,
                                     size_t                      length,
                                     bool                        isNotify) {
    // Serial.print("Notify callback for characteristic ");
    // Serial.print(pDataSourceCharacteristic->getUUID().toString().c_str());
    // Serial.print(" of data length ");
    // Serial.println(length);
    for (int i = 0; i < length; i++) {
        if (i > 7) {
            Serial.write(pData[i]);
        } else {
            Serial.print(pData[i], HEX);
            Serial.print(" ");
        }
    }
    Serial.println();
}

static void NotificationSourceNotifyCallback(NimBLERemoteCharacteristic* pNotificationSourceCharacteristic,
                                             uint8_t*                    pData,
                                             size_t                      length,
                                             bool                        isNotify) {
    if (pData[0] == 0) {
        Serial.println("New notification!");
        latestMessageID[0] = pData[4];
        latestMessageID[1] = pData[5];
        latestMessageID[2] = pData[6];
        latestMessageID[3] = pData[7];

        switch (pData[2]) {
            case 0:
                Serial.println("Category: Other");
                break;
            case 1:
                incomingCall = true;
                Serial.println("Category: Incoming call");
                break;
            case 2:
                Serial.println("Category: Missed call");
                break;
            case 3:
                Serial.println("Category: Voicemail");
                break;
            case 4:
                Serial.println("Category: Social");
                break;
            case 5:
                Serial.println("Category: Schedule");
                break;
            case 6:
                Serial.println("Category: Email");
                break;
            case 7:
                Serial.println("Category: News");
                break;
            case 8:
                Serial.println("Category: Health");
                break;
            case 9:
                Serial.println("Category: Business");
                break;
            case 10:
                Serial.println("Category: Location");
                break;
            case 11:
                Serial.println("Category: Entertainment");
                break;
            default:
                break;
        }
    } else if (pData[0] == 1) {
        Serial.println("Notification Modified!");
        if (pData[2] == 1) {
            Serial.println("Call Changed!");
        }
    } else if (pData[0] == 2) {
        Serial.println("Notification Removed!");
        if (pData[2] == 1) {
            Serial.println("Call Gone!");
        }
    }
    pendingNotification = true;
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
        Serial.printf("Client connected: %s\n", connInfo.getAddress().toString().c_str());
        pClient = pServer->getClient(connInfo);
        Serial.println("Client connected!");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
        Serial.printf("Client disconnected: %s, reason: %d\n", connInfo.getAddress().toString().c_str(), reason);
    }
} serverCallbacks;

void setup() {
    Serial.begin(115200);

    NimBLEDevice::init("ANCS");
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO);
    NimBLEDevice::setPower(9);

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
    pServer->advertiseOnDisconnect(true);

    NimBLEAdvertising*      pAdvertising = pServer->getAdvertising();
    NimBLEAdvertisementData advData{};
    advData.setFlags(0x06);
    advData.addServiceUUID(ancsServiceUUID);
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();

    Serial.println("Advertising started!");
}

void loop() {
    if (pClient != nullptr && pClient->isConnected()) {
        auto pAncsService = pClient->getService(ancsServiceUUID);
        if (pAncsService == nullptr) {
            Serial.printf("Failed to find our service UUID: %s\n", ancsServiceUUID.toString().c_str());
            return;
        }
        // Obtain a reference to the characteristic in the service of the remote BLE server.
        auto pNotificationSourceCharacteristic = pAncsService->getCharacteristic(notificationSourceCharacteristicUUID);
        if (pNotificationSourceCharacteristic == nullptr) {
            Serial.printf("Failed to find our characteristic UUID: %s\n",
                          notificationSourceCharacteristicUUID.toString().c_str());
            return;
        }
        // Obtain a reference to the characteristic in the service of the remote BLE server.
        auto pControlPointCharacteristic = pAncsService->getCharacteristic(controlPointCharacteristicUUID);
        if (pControlPointCharacteristic == nullptr) {
            Serial.printf("Failed to find our characteristic UUID: %s\n",
                          controlPointCharacteristicUUID.toString().c_str());
            return;
        }
        // Obtain a reference to the characteristic in the service of the remote BLE server.
        auto pDataSourceCharacteristic = pAncsService->getCharacteristic(dataSourceCharacteristicUUID);
        if (pDataSourceCharacteristic == nullptr) {
            Serial.printf("Failed to find our characteristic UUID: %s\n", dataSourceCharacteristicUUID.toString().c_str());
            return;
        }
        pDataSourceCharacteristic->subscribe(true, dataSourceNotifyCallback);
        pNotificationSourceCharacteristic->subscribe(true, NotificationSourceNotifyCallback);

        while (1) {
            if (pendingNotification || incomingCall) {
                // CommandID: CommandIDGetNotificationAttributes
                // 32bit uid
                // AttributeID
                Serial.println("Requesting details...");
                uint8_t val[8] =
                    {0x0, latestMessageID[0], latestMessageID[1], latestMessageID[2], latestMessageID[3], 0x0, 0x0, 0x10};
                pControlPointCharacteristic->writeValue(val, 6, true); // Identifier
                val[5] = 0x1;
                pControlPointCharacteristic->writeValue(val, 8, true); // Title
                val[5] = 0x3;
                pControlPointCharacteristic->writeValue(val, 8, true); // Message
                val[5] = 0x5;
                pControlPointCharacteristic->writeValue(val, 6, true); // Date

                while (incomingCall) {
                    if (Serial.available() > 0) {
                        acceptCall = Serial.read();
                        Serial.println((char)acceptCall);
                    }

                    if (acceptCall == 49) { // call accepted , get number 1 from serial
                        const uint8_t vResponse[] =
                            {0x02, latestMessageID[0], latestMessageID[1], latestMessageID[2], latestMessageID[3], 0x00};
                        pControlPointCharacteristic->writeValue((uint8_t*)vResponse, 6, true);

                        acceptCall = 0;
                        // incomingCall = false;
                    } else if (acceptCall == 48) { // call rejected , get number 0 from serial
                        const uint8_t vResponse[] =
                            {0x02, latestMessageID[0], latestMessageID[1], latestMessageID[2], latestMessageID[3], 0x01};
                        pControlPointCharacteristic->writeValue((uint8_t*)vResponse, 6, true);

                        acceptCall   = 0;
                        incomingCall = false;
                    }
                }

                pendingNotification = false;
            }
            delay(1);
        }
    }
    delay(1);
}
