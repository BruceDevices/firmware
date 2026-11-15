#ifndef BRUCE_SDR_H
#define BRUCE_SDR_H

// SDR — raw I/Q takeover of the ESP32-S3 WiFi radio.
// Ported from PBJ3000-ESP32-Jammer (lib/esp_sdr, GPL-3.0-or-later), which is
// itself based on ESPARGOS/esp-sdr. S3 only: the dump engine's DAC side (raw
// I/Q TX) exists on the S3, and the Arduino/IDF C5 memory layout puts .bss
// inside the only C5 dump-engine SRAM block (see the .cpp header for the full
// analysis), so the C5 is out even for RX.
// Call sites must gate on !defined(LITE_VERSION) && !defined(BRUCE_NO_SDR_JAMMER)
// && defined(CONFIG_IDF_TARGET_ESP32S3) AFTER including the Arduino headers.
void sdr_menu();

#endif
