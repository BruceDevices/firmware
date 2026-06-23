# Third-party code and attribution

Bruce is licensed under the **GNU Affero General Public License v3.0 or later**
(AGPL-3.0-or-later); see [`LICENSE`](LICENSE).

Parts of Bruce's RF / sub-GHz module (`src/modules/rf/`) are **derived from**
other free-software projects. Those portions were modified to fit Bruce's native
RMT engine, data model and UI, but remain the work of their original authors and
are used under their respective licenses. Each affected source file carries a
header pointing back to this document.

> This file documents attribution for copyleft compliance. It is not legal
> advice; if you redistribute Bruce, review these obligations yourself.

## Sub-GHz protocols / KeeLoq

### Flipper Zero firmware — GPL-3.0-or-later
- Upstream: <https://github.com/flipperdevices/flipperzero-firmware>
- License: GNU General Public License v3.0
- Copyright (C) Flipper Devices Inc. and the flipperzero-firmware contributors.
- Used by: the KeeLoq block cipher, the manufacturer "learning" key-derivation
  schemes, the KeeLoq frame decoder/encoder framing, the `.sub` keystore format
  and protocol identities.
  Files: `src/modules/rf/protocols/rf_keeloq.{h,cpp}`,
  `src/modules/rf/protocols/rf_decoder.{h,cpp}`,
  `src/modules/rf/protocols/rf_encoder.{h,cpp}`.

### Momentum firmware — GPL-3.0-or-later
- Upstream: <https://github.com/Next-Flip/Momentum-Firmware>
- License: GNU General Public License v3.0
- Copyright (C) the Momentum Firmware contributors.
- Used by: the extended KeeLoq manufacturer list and the model for the
  encrypted built-in keystore (`keeloq_mfcodes`), which Bruce mirrors by
  shipping an AES-encrypted keystore decrypted at runtime as a fallback.
  Files: `src/modules/rf/protocols/rf_keeloq.cpp`,
  `src/modules/rf/protocols/rf_keeloq_mfcodes_data.h` (generated),
  `tools/gen_mfcodes.py`.

> Note: the KeeLoq manufacturer keys themselves originate from the respective
> device manufacturers. They are proprietary key material; their redistribution
> is a separate concern from the GPL attribution above and is the redistributor's
> responsibility to evaluate.

### rc-switch (RCSwitch) — LGPL-2.1-or-later
- Upstream: <https://github.com/sui77/rc-switch>
- License: GNU Lesser General Public License v2.1
- Copyright (C) 2011 Suat Özgür and the rc-switch contributors.
- Used by: the classic OOK protocol timing table (the numbered `RcSwitch_N`
  protocols and the factor-based `{high,low}×te` model) and the
  capture/decode and send state machines that Bruce re-implemented over its
  native RMT engine.
  Files: `src/modules/rf/protocols/rf_registry.{h,cpp}`,
  `src/modules/rf/protocols/rf_decoder.{h,cpp}`,
  `src/modules/rf/protocols/rf_encoder.{h,cpp}`.

## Bundled libraries (linked dependencies)

Fetched via PlatformIO `lib_deps` (see `platformio.ini`); each keeps its own
license. Notably the CC1101 radio driver:

- **SmartRC-CC1101-Driver-Lib** (ELECHOUSE driver fork) —
  <https://github.com/bmorcelli/SmartRC-CC1101-Driver-Lib>
