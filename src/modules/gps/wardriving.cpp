/**
 * @file wardriving.cpp
 * @author IncursioHack - https://github.com/IncursioHack
 * @brief WiFi Wardriving
 * @version 0.2
 * @note Updated: 2024-08-28 by Rennan Cockles (https://github.com/rennancockles)
 */

#include "wardriving.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include "current_year.h"
#include "modules/ble/ble_common.h"
#include <cctype>

#define MAX_WAIT 5000

#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
#endif

static bool parseMacToU64(const String &mac, uint64_t &out) {
    uint64_t value = 0;
    int nibbles = 0;
    for (size_t i = 0; i < mac.length(); i++) {
        char c = mac[i];
        if (c == ':' || c == '-') continue;
        if (!isxdigit(static_cast<unsigned char>(c))) return false;
        value <<= 4;
        if (c >= '0' && c <= '9') value |= static_cast<uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') value |= static_cast<uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') value |= static_cast<uint64_t>(c - 'A' + 10);
        else return false;
        nibbles++;
        if (nibbles > 12) return false;
    }
    if (nibbles != 12) return false;
    out = value;
    return true;
}
Wardriving::Wardriving(bool scanWiFi, bool scanBLE) {
    this->scanWiFi = scanWiFi;
    this->scanBLE = scanBLE;
    setup();
}

Wardriving::~Wardriving() {
    if (gpsConnected) end();
    ioExpander.turnPinOnOff(IO_EXP_GPS, LOW);
#ifdef USE_BOOST /// ENABLE 5V OUTPUT
    PPM.disableOTG();
#endif
}

void Wardriving::setup() {
    wifiNetworkCount = 0;
    bluetoothDeviceCount = 0;
    ioExpander.turnPinOnOff(IO_EXP_GPS, HIGH);
#ifdef USE_BOOST /// ENABLE 5V OUTPUT
    PPM.enableOTG();
#endif
    display_banner();
    padprintln("Initializing...");

    loadAlertMACs();
    begin_wifi();
    if (!begin_gps()) return;

    vTaskDelay(500 / portTICK_PERIOD_MS);
    return loop();
}

void Wardriving::begin_wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
}

bool Wardriving::begin_gps() {
    releasePins();
    GPSserial.begin(
        bruceConfigPins.gpsBaudrate, SERIAL_8N1, bruceConfigPins.gps_bus.rx, bruceConfigPins.gps_bus.tx
    );

    int count = 0;
    padprintln("Waiting for GPS data");
    while (GPSserial.available() <= 0) {
        if (check(EscPress)) {
            end();
            return false;
        }
        displayTextLine("Waiting GPS: " + String(count) + "s");
        count++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    gpsConnected = true;
    return true;
}

void Wardriving::end() {
    if (scanWiFi) wifiDisconnect();
    if (scanBLE) {
#if defined(CONFIG_IDF_TARGET_ESP32C5)
        esp_bt_controller_deinit();
#else
        BLEDevice::deinit();
#endif
    }

    GPSserial.end();
    restorePins();
    returnToMenu = true;
    gpsConnected = false;
}

void Wardriving::loop() {
    int count = 0;
    returnToMenu = false;
    while (1) {
        display_banner();

        if (check(EscPress) || returnToMenu) return end();

        if (GPSserial.available() > 0) {
            count = 0;
            while (GPSserial.available() > 0) gps.encode(GPSserial.read());
            String txt = "GPS Read: ";
            // Debuging GPS messages
            // while (GPSserial.available() > 0) {
            //     char read = GPSserial.read();
            //     txt += read;
            //     gps.encode(read);
            // }
            // Serial.println(txt);
            if (gps.location.isUpdated()) {
                padprintln("GPS location updated");
                set_position();
                scanWiFiBLE();
            } else {
                padprintln("GPS location not updated");
                dump_gps_data();

                if (filename == "" && gps.date.year() >= CURRENT_YEAR && gps.date.year() < CURRENT_YEAR + 5)
                    create_filename();
            }
        } else {
            if (count > 5) {
                displayError("GPS not Found!");
                return end();
            }
            padprintln("No GPS data available");
            count++;
        }

        int tmp = millis();
        while (millis() - tmp < MAX_WAIT && !gps.location.isUpdated()) {
            if (check(EscPress) || returnToMenu) return end();
        }
    }
}

void Wardriving::set_position() {
    double lat = gps.location.lat();
    double lng = gps.location.lng();

    if (initial_position_set) distance += gps.distanceBetween(cur_lat, cur_lng, lat, lng);
    else initial_position_set = true;

    cur_lat = lat;
    cur_lng = lng;
}

void Wardriving::display_banner() {
    drawMainBorderWithTitle("Wardriving");

    padprintln("");
    if (filename != "") padprintln("File: " + filename.substring(0, filename.length() - 4));
    String txt = "Found";
    if (scanWiFi) txt += " WiFi: " + String(wifiNetworkCount);
    if (scanBLE) txt += " BLE: " + String(bluetoothDeviceCount);
    padprint(txt);
    if (foundMACAddressCount) padprint(" Alert: " + String(foundMACAddressCount));

    padprintln("");
    padprintf("Distance: %.2fkm\n", distance / 1000);
}

void Wardriving::dump_gps_data() {
    if (!date_time_updated && (!gps.date.isUpdated() || !gps.time.isUpdated())) {
        padprintln("Waiting for valid GPS data");
        return;
    }
    date_time_updated = true;
    padprintf(2, "Date: %02d-%02d-%02d\n", gps.date.year(), gps.date.month(), gps.date.day());
    padprintf(2, "Time: %02d:%02d:%02d\n", gps.time.hour(), gps.time.minute(), gps.time.second());
    padprintf(2, "Sat:  %d\n", gps.satellites.value());
    padprintf(2, "HDOP: %.2f\n", gps.hdop.hdop());
}

String Wardriving::auth_mode_to_string(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
        case WIFI_AUTH_WAPI_PSK: return "WAPI_PSK";
        default: return "UNKNOWN";
    }
}

void Wardriving::scanWiFiBLE() {
    padprintf("Coord: %.6f, %.6f\n", gps.location.lat(), gps.location.lng());
    int networksFound = scanWiFi ? scanWiFiNetworks() : 0;
    int bleDevicesFound = scanBLE ? scanBLEDevices() : 0;
    append_to_file(networksFound, bleDevicesFound);
}

void Wardriving::enforceRegisteredMACLimit() {
    if (registeredMACs.size() < MAX_REGISTERED_MACS) return;

    registeredMACs.clear();
    macCacheClears++;
    padprintln("MAC cache cleared to prevent heap overflow");
}

int Wardriving::scanWiFiNetworks() {
    padprint("Scanning Wi-Fi...");
    wifiConnected = true;
    int network_amount = WiFi.scanNetworks();
    if (network_amount == 0) {
        padprint(" Found: None");
        return 0;
    }
    padprint(" Found: " + String(network_amount) + " Networks");
    padprintln("");

    return network_amount;
}

int Wardriving::scanBLEDevices() {
    padprint("Scanning BLE...");
    if (!bleInitialized || pBLEScan == nullptr) {
        ble_scan_setup();
        bleInitialized = true;
    }
    BLEScanResults foundDevices;

#ifdef NIMBLE_V2_PLUS
    foundDevices = pBLEScan->getResults(scanTime * 1000, false);
#else
    foundDevices = pBLEScan->start(scanTime, false);
#endif

    int count = foundDevices.getCount();
    if (count == 0) {
        padprint(" Found None");
        pBLEScan->clearResults();
        return 0;
    }

    // Extract device data immediately while scan results are valid
    bleDevices.clear();
    for (int i = 0; i < count; i++) {
        const NimBLEAdvertisedDevice *device = foundDevices.getDevice(i);
        if (!device) continue;

        BLEDeviceData deviceData;

        // Extract data with error handling
        try {
            deviceData.address = device->getAddress().toString().c_str();
            deviceData.rssi = device->getRSSI();

            deviceData.name = device->getName().c_str();

            deviceData.manufacturerId = 0;

            try {
                // Check if device has manufacturer data before accessing it
                if (device->haveManufacturerData()) {
                    std::string mfgData = device->getManufacturerData();
                    if (!mfgData.empty() && mfgData.length() >= 2) {
                        // Extract manufacturer ID from first 2 bytes (little endian)
                        deviceData.manufacturerId = (uint16_t(mfgData[1]) << 8) | uint16_t(mfgData[0]);
                    }
                }
            } catch (const std::exception &e) {
                // Serial.printf(
                //     "Exception extracting manufacturer data for device %s: %s\n",
                //     deviceData.address.c_str(),
                //     e.what()
                // );
                deviceData.manufacturerId = 0;
            } catch (...) {
                // Serial.printf(
                //     "Unknown error extracting manufacturer data for device %s\n",
                //     deviceData.address.c_str()
                // );
                deviceData.manufacturerId = 0;
            }

            bleDevices.push_back(deviceData);
        } catch (...) {
            // Serial.printf("Error extracting data for BLE device %d, skipping\n", i);
            continue;
        }
    }

    pBLEScan->clearResults();
    padprint(" Found: " + String(bleDevices.size()) + " Devices   ");
    padprintln("");

    return bleDevices.size();
}

void Wardriving::loadAlertMACs() {
    FS *fs;
    if (!getFsStorage(fs)) return;

    if (!(*fs).exists("/BruceWardriving")) (*fs).mkdir("/BruceWardriving");

    if ((*fs).exists("/BruceWardriving/alert.txt")) {
        File alertFile = (*fs).open("/BruceWardriving/alert.txt", FILE_READ);
        if (alertFile) {
            while (alertFile.available()) {
                String line = alertFile.readStringUntil('\n');
                line.trim();
                if (line.length() > 0 && !line.startsWith("#")) {
                    // Convert to lowercase for consistent comparison
                    line.toLowerCase();
                    alertMACs.insert(line);
                }
            }
            alertFile.close();
            if (alertMACs.size() > 0) { padprintln("Loaded " + String(alertMACs.size()) + " alert MACs"); }
        }
    } else {
        // Create sample alert file
        File alertFile = (*fs).open("/BruceWardriving/alert.txt", FILE_WRITE);
        if (alertFile) {
            alertFile.println("# Alert MAC addresses - one per line");
            alertFile.println("# Lines starting with # are comments");
            alertFile.println("# Example:");
            alertFile.println("# aa:bb:cc:dd:ee:ff");
            alertFile.close();
        }
    }
}

void Wardriving::create_filename() {
    char timestamp[20];
    sprintf(
        timestamp,
        "%02d%02d%02d_%02d%02d%02d",
        gps.date.year() % 100,
        gps.date.month() % 100,
        gps.date.day() % 100,
        gps.time.hour() % 100,
        gps.time.minute() % 100,
        gps.time.second() % 100
    );
    filename = String(timestamp) + "_wardriving.csv";
}

void Wardriving::append_to_file(int network_amount, int bluetooth_amount) {
    FS *fs;
    if (!getFsStorage(fs)) {
        padprintln("Storage setup error");
        returnToMenu = true;
        return;
    }

    if (filename == "") create_filename();

    if (!(*fs).exists("/BruceWardriving")) (*fs).mkdir("/BruceWardriving");

    bool is_new_file = false;
    if (!(*fs).exists("/BruceWardriving/" + filename)) is_new_file = true;
    File file = (*fs).open("/BruceWardriving/" + filename, is_new_file ? FILE_WRITE : FILE_APPEND);

    if (!file) {
        padprintln("Failed to open file for writing");
        returnToMenu = true;
        return;
    }

    if (is_new_file) {
        file.println(
            "WigleWifi-1.6,appRelease=v" + String(BRUCE_VERSION) + ",model=M5Stack GPS Unit,release=v" +
            String(BRUCE_VERSION) +
            ",device=ESP32 M5Stack,display=SPI TFT,board=ESP32 M5Stack,brand=Bruce,star=Sol,body=4,subBody=1"
        );
        file.println(
            "MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,CurrentLatitude,CurrentLongitude,"
            "AltitudeMeters,AccuracyMeters,RCOIs,MfgrId,Type"
        );
    }

    // WiFi Rows
    // [BSSID],[SSID],[Capabilities],[First timestamp seen],[Channel],[Frequency],
    // [RSSI],[Latitude],[Longitude],[Altitude],[Accuracy],[RCOIs],[MfgrId],[Type]
    // Example: 1a:9f:ee:5c:71:c6,Scampoodle,[WPA2-EAP-CCMP][ESS],2018-08-01 13:08:27,161,5805,
    // -43,37.76578028,-123.45919439,67,3.2160000801086426,5A03BA0000 BAA2D00000 BAA2D02000,,WIFI

    for (int i = 0; i < network_amount; i++) {
        String macAddress = WiFi.BSSIDstr(i);
        uint64_t macKey = 0;
        bool macKeyOk = parseMacToU64(macAddress, macKey);

        // Check if MAC was already found in this session
        enforceRegisteredMACLimit();
        if (!macKeyOk || registeredMACs.find(macKey) == registeredMACs.end()) {

            if (macKeyOk) registeredMACs.insert(macKey); // Adds MAC to cache
            int32_t channel = WiFi.channel(i);

            char buffer[512];
            snprintf(
                buffer,
                sizeof(buffer),
                "%s,\"%s\",[%s],%04d-%02d-%02d %02d:%02d:%02d,%ld,%ld,%ld,%f,%f,%f,%f,,,WIFI\n",
                macAddress.c_str(),
                WiFi.SSID(i).c_str(),
                auth_mode_to_string(WiFi.encryptionType(i)).c_str(),
                gps.date.year(),
                gps.date.month(),
                gps.date.day(),
                gps.time.hour(),
                gps.time.minute(),
                gps.time.second(),
                channel,
                channel != 14 ? 2407 + (channel * 5) : 2484,
                WiFi.RSSI(i),
                gps.location.lat(),
                gps.location.lng(),
                gps.altitude.meters(),
                gps.hdop.hdop() * 1.0
            );
            file.print(buffer);

            // Check for alert
            checkForAlert(macAddress, "WiFi", WiFi.SSID(i));

            wifiNetworkCount++;
        }

        if ((i & 0x1F) == 0) vTaskDelay(1);
    }
    // Free scan results from heap as soon as we finish consuming them
    WiFi.scanDelete();

    // Bluetooth Rows
    // [BD_ADDR],[Device Name],[Capabilities],[First timestamp seen],[Channel],[Frequency],
    // [RSSI],[Latitude],[Longitude],[Altitude],[Accuracy],[RCOIs],[MfgrId],[Type]
    // Example: 63:56:ac:c4:d4:30,,Misc [LE],2018-08-03 18:14:12,0,,
    // -67,37.76090571,-122.44877987,104,49.3120002746582,,72,BLE

    int deviceIndex = 0;
    for (const auto &device : bleDevices) {
        Serial.printf(
            "Processing BLE device: %s, Name: %s, RSSI: %d\n",
            device.address.c_str(),
            device.name.c_str(),
            device.rssi
        );

        // Check if MAC was already found in this session
        enforceRegisteredMACLimit();
        uint64_t macKey = 0;
        bool macKeyOk = parseMacToU64(device.address, macKey);
        if (!macKeyOk || registeredMACs.find(macKey) == registeredMACs.end()) {
            if (macKeyOk) registeredMACs.insert(macKey); // Adds MAC to cache

            char buffer[512];
            char manufacturerIdStr[8] = "";
            if (device.manufacturerId != 0) {
                snprintf(manufacturerIdStr, sizeof(manufacturerIdStr), "%04X", device.manufacturerId);
            }
            snprintf(
                buffer,
                sizeof(buffer),
                "%s,\"%s\",Misc [BLE],%04d-%02d-%02d %02d:%02d:%02d,0,,%d,%f,%f,%f,%f,,%s,BLE\n",
                device.address.c_str(),
                device.name.c_str(),
                gps.date.year(),
                gps.date.month(),
                gps.date.day(),
                gps.time.hour(),
                gps.time.minute(),
                gps.time.second(),
                device.rssi,
                gps.location.lat(),
                gps.location.lng(),
                gps.altitude.meters(),
                gps.hdop.hdop() * 1.0,
                manufacturerIdStr
            );
            file.print(buffer);

            // Check for alert
            checkForAlert(device.address, "BLE", device.name);

            bluetoothDeviceCount++;
        }

        if ((deviceIndex++ & 0x1F) == 0) vTaskDelay(1);
    }
    file.close();
}

void Wardriving::releasePins() {
    rxPinReleased = false;
    if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
        bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
#if !defined(LITE_VERSION)
        bruceConfigPins.W5500_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
        bruceConfigPins.LoRa_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
#endif
        bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.gps_bus.rx)) {
        // T-Embed CC1101 and T-Display S3 Touch ties this pin to the NRF24 CS;
        // switch it to input so the GPS UART can drive it.
        pinMode(bruceConfigPins.gps_bus.rx, INPUT);
        rxPinReleased = true;
    }
}

void Wardriving::checkForAlert(const String &macAddress, const String &deviceType, const String &deviceName) {
    String macLower = macAddress;
    macLower.toLowerCase();

    if (alertMACs.find(macLower) != alertMACs.end()) {
        String alertMsg = "ALERT: " + deviceType + " found!";
        if (deviceName.length() > 0) { alertMsg += " Name: " + deviceName; }
        alertMsg += " MAC: " + macAddress;

        foundMACAddressCount++;

        displayError(alertMsg.c_str());

        // Brief delay to make alert visible
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void Wardriving::restorePins() {
    if (rxPinReleased) {
        if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
            bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
#if !defined(LITE_VERSION)
            bruceConfigPins.W5500_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
            bruceConfigPins.LoRa_bus.checkConflict(bruceConfigPins.gps_bus.rx) ||
#endif
            bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.gps_bus.rx)) {
            // Restore the original board state after leaving the GPS app s
            // o the radio/other peripherals behave as expected
            pinMode(bruceConfigPins.gps_bus.rx, OUTPUT);
            if (bruceConfigPins.gps_bus.rx == bruceConfigPins.CC1101_bus.cs ||
                bruceConfigPins.gps_bus.rx == bruceConfigPins.NRF24_bus.cs ||
#if !defined(LITE_VERSION)
                bruceConfigPins.gps_bus.rx == bruceConfigPins.W5500_bus.cs ||
                bruceConfigPins.gps_bus.rx == bruceConfigPins.W5500_bus.cs ||
#endif
                bruceConfigPins.gps_bus.rx == bruceConfigPins.SDCARD_bus.cs) {
                // If it is conflicting to an SPI CS pin, keep it HIGH
                digitalWrite(bruceConfigPins.gps_bus.rx, HIGH);
            } else {
                // If it is conflicting with any other SPI pin, keep it LOW
                // Avoids CC1101 Jamming and nRF24 radio to keep enabled
                digitalWrite(bruceConfigPins.gps_bus.rx, LOW);
            }
        }
        rxPinReleased = false;
    }
}
