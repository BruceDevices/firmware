#ifndef __BW16_H__
#define __BW16_H__

#include <globals.h>
#include <vector>

struct BW16ScanResult {
    int index;
    String ssid;
    String bssid;
    int channel;
    int security;
    int rssi;
};

class BW16 {
public:
    BW16();
    ~BW16();

    void begin();
    void end();
    void loop();

    void scan(int timeMs = 5000);
    void deauthStart(int index);
    void deauthStop(int index);
    void deauthStopAll();
    void deauthAll();
    void selectSSID(int index);
    void getAPList();
    void getStatus();

    std::vector<BW16ScanResult> getResults() { return _scanResults; }
    String getLastMessage() { return _lastMessage; }
    bool isScanning() { return _scanning; }
    bool isConnected() { return _connected; }

private:
    HardwareSerial _serial = HardwareSerial(2);
    std::vector<BW16ScanResult> _scanResults;
    bool _scanning = false;
    bool _connected = false;
    String _buffer;
    String _lastMessage;
    bool _pinsReleased = false;

    void parseLine(String line);
    void releasePins();
    void restorePins();
    void sendCommand(String cmd);
};

#endif
