# NimBLE Stream Echo Example

This is the simplest example demonstrating `NimBLEStreamServer`. It echoes back any data received from BLE clients.

## Features

- Minimal code showing essential NimBLE Stream usage
- Echoes all received data back to the client
- Uses default service and characteristic UUIDs
- Perfect starting point for learning the Stream interface

## How it Works

1. Initializes BLE with minimal configuration
2. Creates a stream server with default UUIDs
3. Waits for client connection and data
4. Echoes received data back to the client
5. Displays received data on Serial monitor

## Default UUIDs

- Service: `0xc0de`
- Characteristic: `0xfeed`

## Usage

1. Upload this sketch to your ESP32
2. Connect with a BLE client app (nRF Connect, Serial Bluetooth Terminal, etc.)
3. Find the service `0xc0de` and characteristic `0xfeed`
4. Subscribe to notifications
5. Write data to the characteristic
6. The data will be echoed back and displayed in Serial monitor

## Good For

- Learning the basic NimBLE Stream API
- Testing BLE connectivity
- Starting point for custom applications
- Understanding Stream read/write operations
