/**
 *  NimBLE_Stream_Server Example:
 *
 *  Demonstrates using NimBLEStreamServer to create a BLE GATT server
 *  that behaves like a serial port using the Arduino Stream interface.
 *
 *  This allows you to use familiar methods like print(), println(),
 *  read(), and available() over BLE, similar to how you would use Serial.
 *
 *  Created: November 2025
 *      Author: NimBLE-Arduino Contributors
 */

#include <Arduino.h>
#include <NimBLEDevice.h>

// Create the stream server instance
NimBLEStreamServer bleStream;

struct RxOverflowStats {
    uint32_t droppedOld{0};
    uint32_t droppedNew{0};
};

RxOverflowStats g_rxOverflowStats;

NimBLEStream::RxOverflowAction onRxOverflow(const uint8_t* data, size_t len, void* userArg) {
    auto* stats = static_cast<RxOverflowStats*>(userArg);
    if (stats) {
        stats->droppedOld++;
    }

    // Keep the newest bytes for command/stream style traffic.
    (void)data;
    (void)len;
    return NimBLEStream::DROP_OLDER_DATA;
}

// Service and Characteristic UUIDs for the stream
// Using custom UUIDs - you can change these as needed
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

/** Server callbacks to handle connection/disconnection events */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("Client connected: %s\n", connInfo.getAddress().toString().c_str());
        // Optionally update connection parameters for better throughput
        pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, 200);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("Client disconnected - reason: %d, restarting advertising\n", reason);
        NimBLEDevice::startAdvertising();
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
    }
} serverCallbacks;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting NimBLE Stream Server");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("NimBLE-Stream");

    /**
     * Create the BLE server and set callbacks
     * Note: The stream will create its own service and characteristic
     */
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    /**
     * Initialize the stream server with:
     * - Service UUID
     * - Characteristic UUID
     * - txBufSize: 1024 bytes for outgoing data (notifications)
     * - rxBufSize: 1024 bytes for incoming data (writes)
     * - secure: false (no encryption required - set to true for secure connections)
     */
    if (!bleStream.begin(NimBLEUUID(SERVICE_UUID),
                         NimBLEUUID(CHARACTERISTIC_UUID),
                         1024,   // txBufSize
                         1024,   // rxBufSize
                         false)) // secure
    {
        Serial.println("Failed to initialize BLE stream!");
        return;
    }

    bleStream.setRxOverflowCallback(onRxOverflow, &g_rxOverflowStats);

    /**
     * Create advertising instance and add service UUID
     * This makes the stream service discoverable
     */
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName("NimBLE-Stream");
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    Serial.println("BLE Stream Server ready!");
    Serial.println("Waiting for client connection...");
    Serial.println("Once connected, you can send data from the client.");
}

void loop() {
    static uint32_t lastDroppedOld = 0;
    static uint32_t lastDroppedNew = 0;
    if (g_rxOverflowStats.droppedOld != lastDroppedOld || g_rxOverflowStats.droppedNew != lastDroppedNew) {
        lastDroppedOld = g_rxOverflowStats.droppedOld;
        lastDroppedNew = g_rxOverflowStats.droppedNew;
        Serial.printf("RX overflow handled (drop-old=%lu, drop-new=%lu)\n", lastDroppedOld, lastDroppedNew);
    }

    // Check if a client is subscribed (connected and listening)
    if (bleStream.ready()) {
        // Send a message every 2 seconds using Stream methods
        static unsigned long lastSend = 0;
        if (millis() - lastSend > 2000) {
            lastSend = millis();

            // Using familiar Serial-like methods!
            bleStream.print("Hello from BLE Server! Time: ");
            bleStream.println(millis());

            // You can also use printf
            bleStream.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

            Serial.println("Sent data to client via BLE stream");
        }

        // Check if we received any data from the client
        if (bleStream.available()) {
            Serial.print("Received from client: ");

            // Read all available data (just like Serial.read())
            while (bleStream.available()) {
                char c = bleStream.read();
                Serial.write(c);    // Echo to Serial
                bleStream.write(c); // Echo back to BLE client
            }
            Serial.println();
        }
    } else {
        // No subscriber, just wait
        delay(100);
    }
}
