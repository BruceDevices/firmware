# NimBLE Stream Server Example

This example demonstrates how to use the `NimBLEStreamServer` class to create a BLE GATT server that behaves like a serial port using the familiar Arduino Stream interface.

## Features

- Uses Arduino Stream interface (print, println, read, available, etc.)
- Automatic connection management
- Bidirectional communication
- Buffered TX/RX using ring buffers
- Similar usage to Serial communication

## How it Works

1. Creates a BLE GATT server with a custom service and characteristic
2. Initializes `NimBLEStreamServer` with the characteristic configured for notifications and writes
3. Uses familiar Stream methods like `print()`, `println()`, `read()`, and `available()`
4. Automatically handles connection state and MTU negotiation

## Usage

1. Upload this sketch to your ESP32
2. The device will advertise as "NimBLE-Stream"
3. Connect with a BLE client (such as the NimBLE_Stream_Client example or a mobile app)
4. Once connected, the server will:
   - Send periodic messages to the client
   - Echo back any data received from the client
   - Display all communication on the Serial monitor

## Service UUIDs

- Service: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- Characteristic: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`

These are based on the Nordic UART Service (NUS) UUIDs for compatibility with many BLE terminal apps.

## Compatible With

- NimBLE_Stream_Client example
- nRF Connect mobile app
- Serial Bluetooth Terminal apps
- Any BLE client that supports characteristic notifications and writes
