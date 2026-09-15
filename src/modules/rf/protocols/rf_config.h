#pragma once

// Central compile-time configuration for the RF module refactor.
//
// RF_SUB_LEGACY_MIGRATION: when enabled, the one-shot migration of old
// `.sub` files (Protocol: RcSwitch + numeric Preset) into the new
// registry-based format is compiled in. All legacy code lives in a single
// removable module (rf_legacy_migrate.{h,cpp}) guarded by this macro, so a
// future release can drop legacy support by deleting that file and the
// single call site, and flipping this flag to 0.
#ifndef RF_SUB_LEGACY_MIGRATION
#define RF_SUB_LEGACY_MIGRATION 1
#endif

// RF_DEBUG: when set to 1, the RF decode/capture path prints diagnostic
// traces to Serial (capture sizes, separation gaps, decode matches, RAW
// classification). Intended for bring-up / hardware validation only — leave
// at 0 for normal builds. Toggle here (or with -DRF_DEBUG=1) to enable.
#ifndef RF_DEBUG
#define RF_DEBUG 0
#endif

#if RF_DEBUG
#include <Arduino.h>
#define RF_DBG(fmt, ...) Serial.printf("[RFDBG] " fmt "\n", ##__VA_ARGS__)
#else
#define RF_DBG(fmt, ...)                                                                                     \
    do {                                                                                                     \
    } while (0)
#endif
