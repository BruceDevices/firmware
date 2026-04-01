/*
Thanks to thoses developers for their projects:
* @7h30th3r0n3 : https://github.com/7h30th3r0n3/Evil-M5Core2 and https://github.com/7h30th3r0n3/PwnGridSpam
* @viniciusbo : https://github.com/viniciusbo/m5-palnagotchi
* @sduenasg : https://github.com/sduenasg/pio_palnagotchi

Thanks to @bmorcelli for his help doing a better code.
*/
#if !defined(LITE_VERSION)
#include "ArduinoJson.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <Arduino.h>
#include <vector>

typedef struct {
    int epoch;
    String face;
    String grid_version;
    String identity;
    String name;
    int pwnd_run;
    int pwnd_tot;
    String session_id;
    int timestamp;
    int uptime;
    String version;
    signed int rssi;
    unsigned long last_ping;
    bool gone;
} pwngrid_peer;

void initPwngrid();
esp_err_t pwngridAdvertise(uint8_t channel, String face);
std::vector<pwngrid_peer> getPwngridPeers();
uint8_t getPwngridRunTotalPeers();
uint8_t getPwngridTotalPeers();
String getPwngridLastFriendName();
signed int getPwngridClosestRssi();
void checkPwngridGoneFriends();
#endif
