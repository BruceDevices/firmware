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
 * @brief Suspends the WiFi driver without losing init state.
 *
 * Calls esp_wifi_stop(), which frees the driver's DMA buffers (internal RAM).
 * The esp_wifi_init() state is preserved, so wifiResume() is cheap (no
 * re-allocation). This is the correct way for another module (e.g. BLE) to
 * reclaim DMA memory: it goes through wifi_common, which owns the lifecycle.
 *
 * Returns true if WiFi is suspended (or was already off), false on error.
 */
bool wifiSuspend();

/**
 * @brief Resumes the WiFi driver after wifiSuspend().
 *
 * Calls esp_wifi_start() from the preserved init state. Does NOT reconnect
 * to an AP; callers that need the connection back should call wifiConnectMenu()
 * or _connectToWifiNetwork() afterward.
 */
bool wifiResume();

/**
 * @brief True if WiFi is currently suspended via wifiSuspend().
 */
bool isWifiSuspended();

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
bool _wifiConnect(const String &ssid, int encryption, int32_t channel = 0, const uint8_t* bssid = nullptr);
bool _connectToWifiNetwork(const String &ssid, const String &pwd, int32_t channel = 0, const uint8_t* bssid = nullptr);

/**
 * @brief sets up wifi in AP mode
 * @note wifi.mode should be set before calling the method
 */
bool _setupAP();

void updateTimezoneTask(void *pvParameters);

#endif
