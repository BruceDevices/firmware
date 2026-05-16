# NimBLE Stream Client Example

This example demonstrates how to use the `NimBLEStreamClient` class to connect to a BLE GATT server and communicate using the familiar Arduino Stream interface.

## Features

- Uses Arduino Stream interface (print, println, read, available, etc.)
- Automatic server discovery and connection
- Bidirectional communication
- Buffered TX/RX using ring buffers
- Automatic reconnection on disconnect
- Similar usage to Serial communication

## How it Works

1. Scans for BLE devices advertising the target service UUID
2. Connects to the server and discovers the stream characteristic
3. Initializes `NimBLEStreamClient` with the remote characteristic
4. Subscribes to notifications to receive data in the RX buffer
5. Uses familiar Stream methods like `print()`, `println()`, `read()`, and `available()`

## Usage

1. First, upload the NimBLE_Stream_Server example to one ESP32
2. Upload this client sketch to another ESP32
3. The client will automatically:
   - Scan for the server
   - Connect when found
   - Set up the stream interface
   - Begin bidirectional communication
4. You can also type in the Serial monitor to send data to the server

## Service UUIDs

Must match the server:
- Service: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- Characteristic: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`

## Serial Monitor Output

The example displays:
- Server discovery progress
- Connection status
- All data received from the server
- Confirmation of data sent to the server

## Testing

Run with NimBLE_Stream_Server to see bidirectional communication:
- Server sends periodic status messages
- Client sends periodic uptime messages
- Both echo data received from each other
- You can send data from either Serial monitor
