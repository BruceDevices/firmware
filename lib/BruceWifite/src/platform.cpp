// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bruce_wifite_platform.h"
#include <WiFi.h>
#include "../../HAL/SD.h"
#include <atomic>
#include <cstring>
namespace {
std::atomic<bool> leased{false};
bool prepared=false,wasConnected=false,autoReconnect=false,haveConfig=false;
wifi_mode_t previousMode=WIFI_MODE_NULL;
wifi_config_t previousConfig{};
wifi_ps_type_t previousPs=WIFI_PS_NONE;
const char *sdpath(const char *path) { return strncmp(path,"/ext/",5)==0?path+4:path; }
}
extern "C" bool wf_platform_prepare() {
    if(prepared || leased) return false;
    previousMode=WiFi.getMode(); wasConnected=WiFi.isConnected();
    autoReconnect=WiFi.getAutoReconnect();
    haveConfig=(previousMode & WIFI_MODE_STA) && esp_wifi_get_config(WIFI_IF_STA,&previousConfig)==ESP_OK;
    esp_wifi_get_ps(&previousPs);
    WiFi.setAutoReconnect(false);
    if(!WiFi.mode(WIFI_STA)) { WiFi.setAutoReconnect(autoReconnect); return false; }
    WiFi.disconnect(false,false);
    delay(100);
    prepared=true;
    return true;
}
extern "C" void wf_platform_restore() {
    if(!prepared || leased) return;
    esp_wifi_set_promiscuous(false); esp_wifi_set_promiscuous_rx_cb(nullptr);
    esp_wifi_scan_stop(); esp_wifi_disconnect();
    if(haveConfig) esp_wifi_set_config(WIFI_IF_STA,&previousConfig);
    WiFi.mode(previousMode);
    if(previousMode!=WIFI_MODE_NULL) esp_wifi_set_ps(previousPs);
    WiFi.setAutoReconnect(autoReconnect);
    if(wasConnected && haveConfig) esp_wifi_connect();
    memset(&previousConfig,0,sizeof(previousConfig)); prepared=false;
}
extern "C" bool wlan_hal_survey_begin(bool exclusive) {
    (void)exclusive;
    bool expected=false; return prepared && leased.compare_exchange_strong(expected,true);
}
extern "C" bool wlan_hal_survey_run(void (*fn)(void*),void *ctx) {
    if(!leased || !fn) return false; fn(ctx); return true;
}
extern "C" void wlan_hal_survey_end() { leased=false; }
extern "C" const char *wlan_hal_survey_status() { return leased?"Wi-Fi reserved":"Wi-Fi released"; }
extern "C" size_t wf_checkpoint_read(const char *path,char *text,size_t cap) {
    File f=SD.open(sdpath(path),FILE_READ); if(!f || f.size()>cap) return 0;
    size_t n=f.readBytes(text,cap); f.close(); return n;
}
extern "C" bool wf_checkpoint_write(const char *path,const char *text) {
    SD.mkdir("/apps_data"); SD.mkdir("/apps_data/wifite");
    String target=sdpath(path),tmp=target+".tmp";
    File f=SD.open(tmp,FILE_WRITE); if(!f) return false;
    bool ok=f.print(text)==strlen(text); f.flush(); f.close();
    if(ok) { SD.remove(target); ok=SD.rename(tmp,target); }
    if(!ok) SD.remove(tmp); return ok;
}
extern "C" void wf_checkpoint_remove(const char *path) { SD.remove(sdpath(path)); }
