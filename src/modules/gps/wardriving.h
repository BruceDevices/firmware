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
#include <cstdint>
#include <esp_wifi_types.h>
#include <globals.h>
#include <set>

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
    uint32_t sessionStartMs = 0;
    String filename = "";
    TinyGPSPlus gps;
    HardwareSerial GPSserial = HardwareSerial(2); // Uses UART2 for GPS
    std::set<uint64_t> registeredMACs;            // Store and track registered MAC (packed 48-bit)
    std::set<String> alertMACs;                   // Store alert MAC addresses from file
    bool scanWiFi = false;                        // Flag to scan WiFi networks
    bool scanBLE = false;                         // Flag to scan Bluetooth devices
    bool bleInitialized = false;                  // BLE init guard for wardriving
    int wifiNetworkCount = 0;                     // Counter fo wifi networks
    int bluetoothDeviceCount = 0;                 // Counter for bluetooth devices
    int foundMACAddressCount = 0;                 // Counter for found MAC addresses
    uint32_t macCacheClears = 0;                  // Number of times MAC cache was cleared
    static constexpr size_t MAX_REGISTERED_MACS = 250;

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
    void enforceRegisteredMACLimit(void);
    void loadAlertMACs(void);
    void checkForAlert(const String &macAddress, const String &deviceType, const String &deviceName = "");
    String auth_mode_to_string(wifi_auth_mode_t authMode);
    void create_filename(void);
};

#endif // WAR_DRIVING_H
