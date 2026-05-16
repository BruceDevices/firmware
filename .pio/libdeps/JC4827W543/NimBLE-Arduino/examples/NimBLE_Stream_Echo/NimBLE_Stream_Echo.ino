/**
 *  NimBLE_Stream_Echo Example:
 *
 *  A minimal example demonstrating NimBLEStreamServer.
 *  Echoes back any data received from BLE clients.
 *
 *  This is the simplest way to use the NimBLE Stream interface,
 *  showing just the essential setup and read/write operations.
 *
 *  Created: November 2025
 *      Author: NimBLE-Arduino Contributors
 */

#include <Arduino.h>
#include <NimBLEDevice.h>

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

    // Echo mode prefers the latest incoming bytes.
    (void)data;
    (void)len;
    return NimBLEStream::DROP_OLDER_DATA;
}

void setup() {
    Serial.begin(115200);
    Serial.println("NimBLE Stream Echo Server");

    // Initialize BLE
    NimBLEDevice::init("BLE-Echo");
    auto pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true); // Keep advertising after clients disconnect

    /**
     * Initialize the stream server with:
     * - Service UUID
     * - Characteristic UUID
     * - txBufSize: 1024 bytes for outgoing data (notifications)
     * - rxBufSize: 1024 bytes for incoming data (writes)
     * - secure: false (no encryption required - set to true for secure connections)
     */
    if (!bleStream.begin(NimBLEUUID(uint16_t(0xc0de)), // Service UUID
                         NimBLEUUID(uint16_t(0xfeed)), // Characteristic UUID
                         1024,                         // txBufSize
                         1024,                         // rxBufSize
                         false)) {                     // secure
        Serial.println("Failed to initialize BLE stream");
        return;
    }

    bleStream.setRxOverflowCallback(onRxOverflow, &g_rxOverflowStats);

    // Start advertising
    NimBLEDevice::getAdvertising()->start();
    Serial.println("Ready! Connect with a BLE client and send data.");
}

void loop() {
    static uint32_t lastDroppedOld = 0;
    static uint32_t lastDroppedNew = 0;
    if (g_rxOverflowStats.droppedOld != lastDroppedOld || g_rxOverflowStats.droppedNew != lastDroppedNew) {
        lastDroppedOld = g_rxOverflowStats.droppedOld;
        lastDroppedNew = g_rxOverflowStats.droppedNew;
        Serial.printf("RX overflow handled (drop-old=%lu, drop-new=%lu)\n", lastDroppedOld, lastDroppedNew);
    }

    // Echo any received data back to the client
    if (bleStream.ready() && bleStream.available()) {
        Serial.print("Echo: ");
        while (bleStream.available()) {
            char c = bleStream.read();
            Serial.write(c);
            bleStream.write(c); // Echo back
        }
        Serial.println();
    }
    delay(10);
}
