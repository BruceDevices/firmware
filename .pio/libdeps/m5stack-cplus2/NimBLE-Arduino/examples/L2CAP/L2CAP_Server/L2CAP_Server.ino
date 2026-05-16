//
// (C) Dr. Michael 'Mickey' Lauer <mickey@vanille-media.de>
//

#include <Arduino.h>
#include <NimBLEDevice.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "dcbc7255-1e9e-49a0-a360-b0430b6c6905"
#define CHARACTERISTIC_UUID "371a55c8-f251-4ad2-90b3-c7c195b049be"
#define L2CAP_CHANNEL       150
#define L2CAP_MTU           5000

class GATTCallbacks : public NimBLEServerCallbacks {
  public:
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& info) {
        /// Booster #1
        pServer->setDataLen(info.getConnHandle(), 251);
        /// Booster #2 (especially for Apple devices)
        NimBLEDevice::getServer()->updateConnParams(info.getConnHandle(), 12, 12, 0, 200);
    }
} gattCallbacks;

class L2CAPChannelCallbacks : public NimBLEL2CAPChannelCallbacks {
  public:
    bool    connected = false;
    size_t  numberOfReceivedBytes;
    uint8_t nextSequenceNumber;
    int     numberOfSeconds;

  public:
    void onConnect(NimBLEL2CAPChannel* channel) {
        Serial.println("L2CAP connection established");
        connected       = true;
        numberOfSeconds = numberOfReceivedBytes = nextSequenceNumber = 0;
    }

    void onRead(NimBLEL2CAPChannel* channel, std::vector<uint8_t>& data) {
        numberOfReceivedBytes += data.size();
        size_t sequenceNumber  = data[0];
        Serial.printf("L2CAP read %d bytes w/ sequence number %d", data.size(), sequenceNumber);
        if (sequenceNumber != nextSequenceNumber) {
            Serial.printf("(wrong sequence number %d, expected %d)\n", sequenceNumber, nextSequenceNumber);
        } else {
            nextSequenceNumber++;
        }
    }

    void onDisconnect(NimBLEL2CAPChannel* channel) {
        Serial.println("L2CAP disconnected");
        connected             = false;
    }
} l2capCallbacks;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting L2CAP server example");

    NimBLEDevice::init("L2CAP-Server");
    NimBLEDevice::setMTU(BLE_ATT_MTU_MAX);

    auto cocServer = NimBLEDevice::createL2CAPServer();
    auto channel   = cocServer->createService(L2CAP_CHANNEL, L2CAP_MTU, &l2capCallbacks);

    auto server = NimBLEDevice::createServer();
    server->setCallbacks(&gattCallbacks);

    auto service        = server->createService(SERVICE_UUID);
    auto characteristic = service->createCharacteristic(CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ);
    characteristic->setValue(L2CAP_CHANNEL);

    auto advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->enableScanResponse(true);

    NimBLEDevice::startAdvertising();
    Serial.println("Server waiting for connection requests");
}

void loop() {
    // Wait until transfer actually starts...
    if (!l2capCallbacks.numberOfReceivedBytes) {
        delay(10);
        return;
    }

    delay(1000);
    if (!l2capCallbacks.connected) {
        return;
    }

    int bps = l2capCallbacks.numberOfReceivedBytes / ++l2capCallbacks.numberOfSeconds;
    Serial.printf("Bandwidth: %d b/sec = %d KB/sec\n", bps, bps / 1024);
}
