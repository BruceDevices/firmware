#include "BW16.h"
#include <globals.h>

BW16::BW16() {
    // Constructor
}

BW16::~BW16() {
    end();
}

void BW16::begin() {
    releasePins();
    // Use HardwareSerial(2) as defined in header
    _serial.begin(115200, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);

    // Check if pins are valid
    if (bruceConfigPins.uart_bus.rx == GPIO_NUM_NC || bruceConfigPins.uart_bus.tx == GPIO_NUM_NC) {
        log_e("BW16 UART pins not configured!");
        _connected = false;
        return;
    }

    _connected = true;
    _scanning = false;
    _scanResults.clear();
    _lastMessage = "";
    getStatus(); // Check connection
}

void BW16::end() {
    if (_connected) {
        _serial.end();
        restorePins();
        _connected = false;
    }
}

void BW16::loop() {
    if (!_connected) return;

    while (_serial.available()) {
        char c = _serial.read();
        if (c == '\n') {
            parseLine(_buffer);
            _buffer = "";
        } else {
            if (c != '\r') _buffer += c;
        }
    }
}

void BW16::sendCommand(String cmd) {
    if (!_connected) return;
    String fullCmd = "BRUCE:" + cmd;
    _serial.println(fullCmd);
    log_d("Sent: %s", fullCmd.c_str());
}

void BW16::scan(int timeMs) {
    _scanning = true;
    _scanResults.clear();
    sendCommand("SCAN," + String(timeMs));
}

void BW16::deauthStart(int index) {
    sendCommand("DEAUTH_START," + String(index));
}

void BW16::deauthStop(int index) {
    sendCommand("DEAUTH_STOP," + String(index));
}

void BW16::deauthStopAll() {
    sendCommand("DEAUTH_STOP_ALL");
}

void BW16::deauthAll() {
    sendCommand("DEAUTH_ALL");
}

void BW16::selectSSID(int index) {
    sendCommand("SELECT_SSID," + String(index));
}

void BW16::getAPList() {
    _scanResults.clear();
    sendCommand("GET_AP_LIST");
}

void BW16::getStatus() {
    sendCommand("GET_STATUS");
}

void BW16::parseLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    // Log for debug
    // Serial.println("[BW16] " + line);

    if (!line.startsWith("BRUCE_RESP:")) return;

    String resp = line.substring(11); // Remove prefix

    if (resp.startsWith("SCAN_SUCCESS")) {
        _scanning = false;
    } else if (resp.startsWith("SCAN_FAILED")) {
        _scanning = false;
    } else if (resp.startsWith("DEAUTH_ALL_STARTED:COUNT=")) {
        _lastMessage = "Deauth All Started: " + resp.substring(25);
    } else if (resp.startsWith("ERROR:")) {
        _lastMessage = resp.substring(6);
    } else if (resp.startsWith("AP:")) {
        // Format: AP:index|SSID|BSSID|Channel|Security|RSSI
        // Example: AP:0|MyWifi|00:11:22:33:44:55|6|3|-60

        int idx = resp.indexOf(':') + 1;
        String data = resp.substring(idx);

        int pipe1 = data.indexOf('|');
        int pipe2 = data.indexOf('|', pipe1 + 1);
        int pipe3 = data.indexOf('|', pipe2 + 1);
        int pipe4 = data.indexOf('|', pipe3 + 1);
        int pipe5 = data.indexOf('|', pipe4 + 1);

        if (pipe1 > 0 && pipe2 > pipe1 && pipe3 > pipe2 && pipe4 > pipe3 && pipe5 > pipe4) {
            BW16ScanResult res;
            res.index = data.substring(0, pipe1).toInt();
            res.ssid = data.substring(pipe1 + 1, pipe2);
            res.bssid = data.substring(pipe2 + 1, pipe3);
            res.channel = data.substring(pipe3 + 1, pipe4).toInt();
            res.security = data.substring(pipe4 + 1, pipe5).toInt();
            res.rssi = data.substring(pipe5 + 1).toInt();

            _scanResults.push_back(res);
        }
    } else if (resp.startsWith("SCAN_COMPLETE")) {
        _scanning = false;
    } else if (resp.startsWith("SCAN_RESULT:COUNT=")) {
        int count = resp.substring(18).toInt();
        if (count == 0) _scanning = false;
        // Pre-allocate vector? Not strictly necessary.
    }
}

// Logic copied/adapted from GPSTracker to avoid conflicts
void BW16::releasePins() {
    _pinsReleased = false;
    if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
#if !defined(LITE_VERSION)
        bruceConfigPins.W5500_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.LoRa_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
#endif
        bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.rx)) {

        pinMode(bruceConfigPins.uart_bus.rx, INPUT);
        _pinsReleased = true;
    }
}

void BW16::restorePins() {
    if (_pinsReleased) {
        if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
            bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
#if !defined(LITE_VERSION)
            bruceConfigPins.W5500_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
            bruceConfigPins.LoRa_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
#endif
            bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.rx)) {

            pinMode(bruceConfigPins.uart_bus.rx, OUTPUT);

            // Check specific conflicts to set HIGH/LOW
            if (bruceConfigPins.uart_bus.rx == bruceConfigPins.CC1101_bus.cs ||
                bruceConfigPins.uart_bus.rx == bruceConfigPins.NRF24_bus.cs ||
#if !defined(LITE_VERSION)
                bruceConfigPins.uart_bus.rx == bruceConfigPins.W5500_bus.cs ||
#endif
                bruceConfigPins.uart_bus.rx == bruceConfigPins.SDCARD_bus.cs) {
                // If it is conflicting to an SPI CS pin, keep it HIGH
                digitalWrite(bruceConfigPins.uart_bus.rx, HIGH);
            } else {
                // Keep it LOW otherwise
                digitalWrite(bruceConfigPins.uart_bus.rx, LOW);
            }
        }
        _pinsReleased = false;
    }
}
