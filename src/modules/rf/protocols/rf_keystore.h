// SPDX-License-Identifier: GPL-3.0-or-later
//
// Key material for the shared RF protocol engine (ProtoPirate vehicle codecs).
//
// The Flipper app keeps its keystore device-encrypted behind the secure
// enclave (furi_hal_crypto, key slot 1); ESP32 boards have no enclave, so key
// material is provided in plaintext:
//
//   1. A built-in community keyfile compiled into the firmware
//      (rf_keystore_data.h; one "KEY,TYPE,NAME" entry per key — the Kia
//      V3/V4/V5/V6 keys are the entries with type 10..13, the rest are
//      KeeLoq manufacturer codes feeding Star Line and the keeloq families).
//   2. Optionally a plaintext "Flipper SubGhz Keystore File" from storage,
//      one "HEXKEY:TYPE:NAME" line per key:
//        <storage>/BruceRF/ProtoPirate/keystore   (also /BruceRF/keystore, /keystore)
//   3. Optionally the VAG aut64 key blob ("Flipper SubGhz Keystore RAW File",
//      bare-hex or RAW layout):
//        <storage>/BruceRF/ProtoPirate/keystore/vag
//
// Bruce's KeeLoq /mfcodes manufacturer keystore (or its embedded copy) is
// always merged in as well, which restores Star Line manufacturer
// identification and the KeeLoq-family encoding without any extra file.
#pragma once
#if !defined(LITE_VERSION)
void rf_keystore_load(); // idempotent; once per session, before first decode
#endif
