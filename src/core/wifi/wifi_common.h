#include "core/display.h"
#include <NTPClient.h>
#include <WiFi.h>

#ifndef __WIFI_COMMON_H__
#define __WIFI_COMMON_H__
// TODO wrap in a class

// public
/**
 * @brief disconnects and turns off wifi module
 */
void wifiDisconnect();

/**
 * @brief Opens a menu to connect to a wifi
 * @param mode connection mode(AP, STA, AP_STA)
 * @note This is the primary entry point for establishing connections
 * @note returns false if wifi is already connected
 */
bool wifiConnectMenu(wifi_mode_t = WIFI_MODE_STA);

/**
 * @brief Scans the networks and tries to connect to a known network
 * @param mode connection mode(void)
 * @note This is the primary entry point for establishing connections in the Headless environment
 * @note returns true if connected successfully
 */
bool wifiConnecttoKnownNet(void);

/**
 * @brief returns MAC adress
 */
String checkMAC();

/**
 * @brief Transmits a raw 802.11 frame while respecting TX-buffer backpressure.
 *
 * esp_wifi_80211_tx() returns ESP_ERR_NO_MEM when the static TX buffers are
 * temporarily full. Callers that ignore the return value (beacon spam, deauth
 * flood, karma, pwngrid) silently drop frames when the injection loop runs faster
 * than the driver, especially under -flto with CONFIG_ESP_WIFI_STATIC_TX_BUFFER_NUM
 * reduced. Here we retry briefly, yielding 1 tick for the driver to drain: injection
 * becomes self-regulated and independent of LTO and buffer count.
 *
 * @return ESP_OK if the frame was accepted; otherwise the last error.
 */
esp_err_t wifiRawTx(wifi_interface_t ifx, const void *frame, int len, uint8_t retries = 8);

/**
 * @brief tries to connect to min(found_networks, maxSearch) networks
 * using stored passwords
 * @TODO fix: rn it skips open networks due to password == "" check
 */
void wifiConnectTask(void *pvParameters);

/**
 * @brief Ensures esp_netif and the default event loop are initialized (idempotent)
 */
void ensureWifiPlatform();

// private
/**
 * @brief Connects to wifiNetwork
 */
bool _wifiConnect(const String &ssid, int encryption);
bool _connectToWifiNetwork(const String &ssid, const String &pwd);

/**
 * @brief sets up wifi in AP mode
 * @note wifi.mode should be set before calling the method
 */
bool _setupAP();

void updateTimezoneTask(void *pvParameters);

#endif
