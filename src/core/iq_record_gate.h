#pragma once
// IQ Record availability gate (implementation: modules/wifi/sdr.cpp).
//
// The capture engine reserves a 64 KiB internal-SRAM bank out of the heap at
// boot. Boards WITH PSRAM afford that; PSRAM-less boards (cardputer-class)
// cannot — with the bank reserved their post-WiFi heap no longer hosts
// allocations like the Wifite worker stack, so the whole feature compiles out
// there and those boards keep the full heap for WiFi features.
//
// Requirements: full build (no LITE), feature not disabled via env flag
// (BRUCE_NO_IQ_RECORD), an ESP32-S3 base (the undocumented MAC dump engine
// only exists there) and a build variant with SPIRAM enabled (CONFIG_SPIRAM).
//
// INCLUDE THIS AFTER Arduino.h / sdkconfig.h: CONFIG_SPIRAM and
// CONFIG_IDF_TARGET_ESP32S3 come from the memory_type variant's sdkconfig.
#if defined(LITE_VERSION) || defined(BRUCE_NO_IQ_RECORD) || !defined(CONFIG_SPIRAM) || \
    !defined(CONFIG_IDF_TARGET_ESP32S3)
#define BRUCE_IQ_RECORD_UNAVAILABLE 1
#endif
