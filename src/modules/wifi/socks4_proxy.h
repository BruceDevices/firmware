#ifndef SOCKS4_PROXY_H
#define SOCKS4_PROXY_H

#include <cstdint>

/**
 * SOCKS4 proxy **server** for Bruce firmware (ESP32).
 *
 * Listens on port 1080 (or given port). Use from your PC e.g.:
 *   ssh -o ProxyCommand='nc -X 4 -x <esp_ip>:1080 %h %p' user@target
 * Or set system/browser SOCKS proxy to <esp_ip>:1080.
 *
 * Asio on ESP32 (upstream chriskohlhoff/asio):
 * - espressif/asio repo is **archived**; migration is to ESP-Protocols.
 * - Current Asio component: espressif/esp-protocols (components/asio), uses upstream Asio.
 * - Docs: https://docs.espressif.com/projects/esp-protocols/asio/docs/latest/
 * - Official socks4 **client** example: https://components.espressif.com/components/espressif/asio/versions/1.28.0/examples/socks4
 *
 * This module implements the **server** (ESP32 as proxy). Bruce uses the Arduino
 * framework; the Asio component is ESP-IDF–oriented, so we use WiFiServer/WiFiClient
 * here. An Asio-based server would follow the same protocol and the esp-protocols
 * socks4/async_request patterns (with acceptor + relay instead of client connect).
 */
void socks4Proxy(uint16_t port = 1080);

#endif
