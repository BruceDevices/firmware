#ifndef BRUCE_SDR_H
#define BRUCE_SDR_H

// IQ Record — raw I/Q capture from the ESP32-S3 WiFi radio to the SD card.
// Ported from PBJ3000-ESP32-Jammer (lib/esp_sdr, GPL-3.0-or-later), which is
// itself based on ESPARGOS/esp-sdr (RX half only; the TX/jammer side was
// removed).
// Availability is decided by core/iq_record_gate.h: full build (no LITE),
// env flag not set (BRUCE_NO_IQ_RECORD), ESP32-S3, and a memory_type variant
// with SPIRAM enabled (CONFIG_SPIRAM) — the capture bank costs 64 KiB of
// internal RAM, which PSRAM-less boards cannot spare.
// Include "core/iq_record_gate.h" (after Arduino.h) and gate call sites on
// !defined(BRUCE_IQ_RECORD_UNAVAILABLE).
void sdr_menu();

#endif
