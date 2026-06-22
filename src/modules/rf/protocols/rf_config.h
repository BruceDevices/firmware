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
