//
// (C) Dr. Michael 'Mickey' Lauer <mickey@vanille-media.de>
//

#include <Arduino.h>
#include <NimBLEDevice.h>

#if CONFIG_BT_NIMBLE_L2CAP_COC_MAX_NUM <= 0
# error "CONFIG_BT_NIMBLE_L2CAP_COC_MAX_NUM must be set to 1 or greater"
#endif

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// The remote service we wish to connect to.
static NimBLEUUID serviceUUID("dcbc7255-1e9e-49a0-a360-b0430b6c6905");
// The characteristic of the remote service we are interested in.
static NimBLEUUID charUUID("371a55c8-f251-4ad2-90b3-c7c195b049be");

#define L2CAP_CHANNEL 150
#define L2CAP_MTU     5000

const NimBLEAdvertisedDevice* theDevice  = NULL;
NimBLEClient*                 theClient  = NULL;
NimBLEL2CAPChannel*           theChannel = NULL;

size_t bytesSent       = 0;
size_t bytesReceived   = 0;
size_t numberOfSeconds = 0;

class L2CAPChannelCallbacks : public NimBLEL2CAPChannelCallbacks {
  public:
    void onConnect(NimBLEL2CAPChannel* channel) { Serial.println("L2CAP connection established"); }

    void onMTUChange(NimBLEL2CAPChannel* channel, uint16_t mtu) { Serial.printf("L2CAP MTU changed to %d\n", mtu); }

    void onRead(NimBLEL2CAPChannel* channel, std::vector<uint8_t>& data) {
        Serial.printf("L2CAP read %d bytes\n", data.size());
    }
    void onDisconnect(NimBLEL2CAPChannel* channel) { Serial.println("L2CAP disconnected"); }
} L2Callbacks;

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("GAP connected");
        pClient->setDataLen(251);

        theChannel = NimBLEL2CAPChannel::connect(pClient, L2CAP_CHANNEL, L2CAP_MTU, &L2Callbacks);
    }

    void onDisconnect(NimBLEClient* pClient, int reason) {
        printf("GAP disconnected (reason: %d)\n", reason);
        theDevice  = NULL;
        theChannel = NULL;
        NimBLEDevice::getScan()->start(5 * 1000, true);
    }
} clientCallbacks;

class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
        if (theDevice) {
            return;
        }
        Serial.printf("BLE Advertised Device found: %s\n", advertisedDevice->toString().c_str());

        if (!advertisedDevice->haveServiceUUID()) {
            return;
        }
        if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
            return;
        }

        Serial.println("Found the device we're interested in!");
        NimBLEDevice::getScan()->stop();

        // Hand over the device to the other task
        theDevice = advertisedDevice;
    }
} scanCallbacks;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting L2CAP client example");

    NimBLEDevice::init("L2CAP-Client");
    NimBLEDevice::setMTU(BLE_ATT_MTU_MAX);

    auto scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&scanCallbacks);
    scan->setInterval(1349);
    scan->setWindow(449);
    scan->setActiveScan(true);
    scan->start(25 * 1000, false);
}

void loop() {
    static uint8_t       sequenceNumber = 0;
    static unsigned long firstBytesTime = 0;
    auto                 now            = millis();

    if (!theDevice) {
        delay(1000);
        return;
    }

    if (!theClient) {
        theClient = NimBLEDevice::createClient();
        theClient->setConnectionParams(6, 6, 0, 42);
        theClient->setClientCallbacks(&clientCallbacks);
        if (!theClient->connect(theDevice)) {
            Serial.println("Error: Could not connect to device");
            return;
        }
        delay(2000);
    }

    if (!theChannel) {
        Serial.println("l2cap channel not initialized");
        delay(2000);
        return;
    }

    if (!theChannel->isConnected()) {
        Serial.println("l2cap channel not connected\n");
        delay(2000);
        return;
    }

    std::vector<uint8_t> data(5000, sequenceNumber++);
    if (theChannel->write(data)) {
        if (bytesSent == 0) {
            firstBytesTime = now;
        }
        bytesSent += data.size();
        if (now - firstBytesTime > 1000) {
            int bytesSentPerSeconds = bytesSent / ((now - firstBytesTime) / 1000);
            printf("Bandwidth: %d b/sec = %d KB/sec\n", bytesSentPerSeconds, bytesSentPerSeconds / 1024);
        }
    } else {
        Serial.println("failed to send!");
        abort();
    }
}
