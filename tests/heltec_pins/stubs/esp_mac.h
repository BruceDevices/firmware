#pragma once
#include <cstdint>
constexpr int ESP_MAC_WIFI_STA = 0;
inline void esp_read_mac(uint8_t *mac, int) {
    const uint8_t fixed[6] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};
    for (int i = 0; i < 6; ++i) mac[i] = fixed[i];
}
