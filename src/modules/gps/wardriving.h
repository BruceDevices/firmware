/**
 * @file wardriving.h
 * @author IncursioHack - https://github.com/IncursioHack
 * @brief WiFi Wardriving
 * @version 0.2
 * @note Updated: 2024-08-28 by Rennan Cockles (https://github.com/rennancockles)
 */

#ifndef __WAR_DRIVING_H__
#define __WAR_DRIVING_H__

#include "modules/ble/ble_common.h"
#include <TinyGPS++.h>
#include <esp_wifi_types.h>
#include <globals.h>
#include <set>
#include <vector>

class Wardriving {
public:
    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    Wardriving(bool scanWiFi = false, bool scanBLE = false);
    ~Wardriving();

    /////////////////////////////////////////////////////////////////////////////////////
    // Life Cycle
    /////////////////////////////////////////////////////////////////////////////////////
    void setup();
    void loop();

private:
    bool date_time_updated = false;
    bool initial_position_set = false;
    double cur_lat;
    double cur_lng;
    double distance = 0;
    String filename = "";
    TinyGPSPlus gps;
    HardwareSerial GPSserial = HardwareSerial(2); // Uses UART2 for GPS
    std::set<String> registeredMACs;              // Store and track registered MAC
    std::set<String> alertMACs;                   // Store alert MAC addresses from file
    bool scanWiFi = false;                        // Flag to scan WiFi networks
    bool scanBLE = false;                         // Flag to scan Bluetooth devices
    int wifiNetworkCount = 0;                     // Counter fo wifi networks
    int bluetoothDeviceCount = 0;                 // Counter for bluetooth devices
    int foundMACAddressCount = 0;                 // Counter for found MAC addresses

    // Structure to safely store BLE device data
    struct BLEDeviceData {
        String address;
        String name;
        int rssi;
        uint16_t manufacturerId;
    };
    std::vector<BLEDeviceData> bleDevices; // Safe storage for BLE device data
    bool rxPinReleased = false;

    /////////////////////////////////////////////////////////////////////////////////////
    // Setup
    /////////////////////////////////////////////////////////////////////////////////////
    void begin_wifi(void);
    bool begin_gps(void);
    void end(void);
    void releasePins(void);
    void restorePins(void);

    /////////////////////////////////////////////////////////////////////////////////////
    // Display functions
    /////////////////////////////////////////////////////////////////////////////////////
    void display_banner(void);
    void dump_gps_data(void);

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    void set_position(void);
    void scanWiFiBLE(void);
    int scanWiFiNetworks(void);
    int scanBLEDevices(void);
    void loadAlertMACs(void);
    void checkForAlert(const String &macAddress, const String &deviceType, const String &deviceName = "");
    String auth_mode_to_string(wifi_auth_mode_t authMode);
    void append_to_file(int network_amount = 0, int bluetooth_amount = 0);
    void create_filename(void);
};

#endif // WAR_DRIVING_H
