#pragma once

#include "rf_config.h"

#if RF_SUB_LEGACY_MIGRATION

#include "../structs.h"
#include "rf_protocol.h"
#include <FS.h>

// ===========================================================================
// REMOVABLE legacy-compatibility module.
//
// Everything related to the OLD `.sub` format (Protocol: RcSwitch + numeric
// Preset) lives here and only here, guarded by RF_SUB_LEGACY_MIGRATION.
// To drop legacy support in a future release: delete this file + its .cpp,
// remove the single call site in readSubFile(), and set the macro to 0.
//
// Do NOT spread RcSwitch / legacy handling into the registry files.
// ===========================================================================

// Map an old legacy protocol number to a registry protocol definition.
// Returns nullptr when the number has no known mapping.
const RfProtocolDef *rf_find_legacy(int preset_no);

// True if `code` is in the old format (Protocol == "RcSwitch", or a bare
// numeric Preset) and is therefore a migration candidate.
bool rf_sub_is_legacy(const RfCodes &code);

// One-shot migration: rewrite `path` from the old format to the new
// registry-based format, after backing the original up to `<path>.bak`
// (idempotent — never overwrites an existing `.bak`). Read-only filesystems
// and temporary paths (/tmpramfile, PSRamFS) are skipped. Updates `code` to
// the migrated values regardless of whether the file could be rewritten.
// Returns true when the in-memory `code` was migrated.
bool rf_sub_migrate(FS *fs, const String &path, RfCodes &code);

#endif // RF_SUB_LEGACY_MIGRATION
