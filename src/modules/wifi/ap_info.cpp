#include "ap_info.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/net_utils.h"
#include "core/scrollableTextArea.h"
#include "esp_wifi.h"
#include "lwip/etharp.h"
#include <WiFi.h>
#include <globals.h>

String autoMode2String(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_ENTERPRISE: return "WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
        case WIFI_AUTH_WAPI_PSK: return "WAPI_PSK";
        case WIFI_AUTH_WPA3_ENT_192: return "WPA3_ENT_192";
        default: return "UNKNOWN";
    }
}

String cypherType2String(wifi_cipher_type_t cipherType) {
    switch (cipherType) {
        case WIFI_CIPHER_TYPE_NONE: return "NONE";
        case WIFI_CIPHER_TYPE_WEP40: return "WEP40";
        case WIFI_CIPHER_TYPE_WEP104: return "WEP104";
        case WIFI_CIPHER_TYPE_TKIP: return "TKIP";
        case WIFI_CIPHER_TYPE_CCMP: return "CCMP";
        case WIFI_CIPHER_TYPE_TKIP_CCMP: return "TKIP_CCMP";
        case WIFI_CIPHER_TYPE_AES_CMAC128: return "AES_CMAC128";
        case WIFI_CIPHER_TYPE_SMS4: return "SMS4";
        case WIFI_CIPHER_TYPE_GCMP: return "GCMP";
        case WIFI_CIPHER_TYPE_GCMP256: return "GCMP256";
        case WIFI_CIPHER_TYPE_AES_GMAC128: return "AES_GMAC128";
        case WIFI_CIPHER_TYPE_AES_GMAC256: return "AES_GMAC256";
        case WIFI_CIPHER_TYPE_UNKNOWN:
        default: return "UNKNOWN";
    }
}

String phyModes2String(wifi_ap_record_t record) {
    String modes;
    if (record.phy_11b || record.phy_11g || record.phy_11g || record.phy_11n) modes = "11";
    if (record.phy_11b) modes += "b/";
    if (record.phy_11g) modes += "g/";
    if (record.phy_11n) modes += "n/";
    if (record.phy_lr) modes += "low/";
    if (!modes.isEmpty()) modes[modes.length() - 1] = ' ';

    if (record.ftm_responder || record.ftm_initiator) {
        modes += "FTM: ";
        if (record.ftm_responder) modes += "RESP ";
        if (record.ftm_initiator) modes += "INIT ";
    }

    return modes.isEmpty() ? "None" : modes;
}

String getChannelWidth(wifi_second_chan_t secondChannel) {
    switch (secondChannel) {
        case WIFI_SECOND_CHAN_NONE: return "HT20";   // 20 MHz channel width (no secondary channel)
        case WIFI_SECOND_CHAN_ABOVE: return "HT40+"; // 40 MHz channel width with secondary channel above
        case WIFI_SECOND_CHAN_BELOW: return "HT40-"; // 40 MHz channel width with secondary channel below
        default: return "Unknown";
    }
}

void fillInfo(ScrollableTextArea &area) {
    wifi_ap_record_t ap_info;
    esp_err_t res;
    if ((res = esp_wifi_sta_get_ap_info(&ap_info)) != ESP_OK) {
        String err;
        switch (res) {
            case ESP_ERR_WIFI_CONN: err = "iface is not initialized"; break;
            case ESP_ERR_WIFI_NOT_CONNECT: err = "station disconnected"; break;
            default: err = "failed with" + String(res); break;
        }

        area.addLine(err);
        area.show();

        while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
        while (!check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
    }

    const auto mac = MAC(ap_info.bssid);

    displayTextLine("Gathering...");

    // in promiscius mode also Rx/Tx can be gathered
    // organized in the most to least usable
    area.addLine("SSID: " + String((char *)ap_info.ssid));
    area.addLine("PSK: " + bruceConfig.getWifiPassword((char *)ap_info.ssid));
    area.addLine("Internet: " + String(internetConnection() ? "avail" : "unavail"));
    area.addLine("Modes: " + phyModes2String(ap_info));
    area.addLine("Signal strength: " + String(ap_info.rssi) + "db");
    // AP might not have assigned IP and gateway ip might differ from an ap ip
    area.addLine("Gateway: " + WiFi.gatewayIP().toString());
    area.addLine("Channel: " + String(ap_info.primary) + " " + getChannelWidth(ap_info.second));
    area.addLine("BSSID: " + mac); // sometimes MAC != BSSID (but we ignore that case)
    area.addLine("Manufacturer: " + getManufacturer(mac));
    area.addLine(
        "Auth mode: " + autoMode2String(ap_info.authmode) + " WPA: " + String(ap_info.wps ? "enbld" : "dsbld")
    );
    area.addLine(
        "Cypher uni: " + cypherType2String(ap_info.pairwise_cipher) +
        " mulit: " + cypherType2String(ap_info.group_cipher)
    );
    area.addLine("Antenna: " + String(ap_info.ant));
}

void displayAPInfo() {
    ScrollableTextArea area = ScrollableTextArea("AP INFO");
    fillInfo(area);
    area.show();
}
