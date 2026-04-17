#ifndef __BW16_TOOL_H__
#define __BW16_TOOL_H__

#include <Arduino.h>
#include <algorithm>
#include <globals.h>
#include <vector>

struct BW16Network {
    int index;
    String ssid;
    String bssid;
    int channel;
    int rssi;
};

struct BW16Group {
    String ssid;
    std::vector<BW16Network> aps;
    int maxRssi = -127;
    bool has24 = false;
    bool has5 = false;
};

class BW16Tool {
public:
    BW16Tool();
    ~BW16Tool();

    void setup();
    void scanWifi();
    void selectWifi();
    void attackWifiMenu();

    void attackSelected();
    void attackAll();
    void beaconSpam();
    void beaconList();
    void beaconDeauth();
    void evilPortal();
    void attckbleMenu();
    void send_at_ble(String type);
    String readSerialLine();

private:
    bool isBW16Active = false;
    bool rxPinReleased = false;
    std::vector<BW16Network> all_networks;
    std::vector<BW16Group> grouped_networks;
    std::vector<String> selected_bssids;

    bool begin_bw16();
    void end();
    void releasePins();
    void restorePins();
    void display_banner();
    void send_attack_command(String type);
    void printCustomLog(String msg);
    void centerString(String text);
    void save_captured_creds(const std::vector<String> &creds);
};

#endif
