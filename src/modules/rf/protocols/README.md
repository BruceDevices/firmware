# `rf/protocols/` — RF Protocol Definitions (sub-GHz)

This directory is the **only place** where RF protocol definitions and
sub-GHz radio presets used by Bruce's RF module should live.

## Purpose

Centralize, in a single versioned and documented location:

- The **timing parameters** of each static OOK protocol
  (pulse length/`TE`, sync factors, bit 0/1 encoding,
  inverted signal, typical bit count).
- The **radio presets** (modulation, bandwidth, deviation, data
  rate) associated with each mode of operation.
- The mapping table between preset names in `.sub` file format and
  the concrete parameters applied to the transceiver.

Before this directory existed, this data was scattered and
duplicated across `rf_send.cpp`, `rf_scan.cpp`, and utilities. The
refactor goal is that any protocol addition/adjustment happens
**here**, and consumers (send, scan, replay) read from these definitions.

## Rules

1. No protocol parameters should be redefined outside this
   directory. Consumers import, they don't recreate.
2. Each protocol is documented with the source/note that justifies
   its timing values.
3. The external contract (CLI commands, JS names, `.sub` format, Menu
   entries) does not change based on internal changes here — see
   `.claude/rf_contract.md`.
4. The UI palette remains restricted to `priColor` / `bgColor` /
   `getComplementaryColor2(priColor)`.

## Files (Milestone 1)

- `rf_config.h` — RF module compilation configuration.
  `RF_SUB_LEGACY_MIGRATION` (default 1) enables `.sub` backward compatibility.
- `rf_protocol.h` — `RfPreset` and `RfProtocolDef` structs (data model,
  no tables). `RfProtocolFlags` (`RF_PF_HAS_SYNC`, `RF_PF_FIXED_LEN`).
- `rf_presets.h/.cpp` — radio preset table + `rf_find_preset()`.
- `rf_registry.h/.cpp` — static OOK protocol table +
  `rf_find_protocol()` / `rf_protocol_at()` / `rf_protocol_count()`.
- `rf_legacy_migrate.h/.cpp` — **removable backward compatibility module**
  (guarded by `RF_SUB_LEGACY_MIGRATION`): `n→name` table,
  `rf_sub_is_legacy()`, `rf_sub_migrate()`. Wiring in `readSubFile`
  is deferred to M3.

## Preset Alias Table (`.sub`)

Old Furi names remain valid via alias → neutral canonical name:

| `.sub` old (alias)                       | Canonical        |
|------------------------------------------|------------------|
| `FuriHalSubGhzPresetOok270Async`         | `Ook270Async`    |
| `FuriHalSubGhzPresetOok650Async`         | `Ook650Async`    |
| `FuriHalSubGhzPreset2FSKDev238Async`     | `2FSKDev238Async`|
| `FuriHalSubGhzPreset2FSKDev476Async`     | `2FSKDev476Async`|
| `FuriHalSubGhzPresetMSK99_97KbAsync`     | `MSK99_97KbAsync`|
| `FuriHalSubGhzPresetGFSK9_99KbAsync`     | `GFSK9_99KbAsync`|

## Protocol coverage

Coverage is delivered in three layers:

1. **Named/parameterized (registry).** Static OOK families whose pulses are
   integer multiples of a base `TE` are described declaratively in
   `rf_registry.cpp` and run through the shared decode/encode engines. These
   carry a stable `name` written to `Protocol:` in the `.sub`, so a captured
   code identifies and replays as the same protocol (round-trip).
   The table distinguishes two structural shapes handled by one model
   (`RfProtocolDef`): *high-first* (`inv=false`) and *space-coded* (`inv=true`,
   `sync` = `{gap, preamble}`). Fixed-length protocols (`FIXED`) only match a
   frame of exactly their bit length, which disambiguates codes that share
   timings.

   | Protocol      | bits | shape       | decode | encode | round-trip tested      |
   |---------------|-----:|-------------|:------:|:------:|:-----------------------|
   | Princeton     |  24  | high-first  |   ✓    |   ✓    | ✓ loopback + golden    |
   | NICE_FLO      |  12  | high-first  |   ✓    |   ✓    | ✓ loopback             |
   | Linear        |  10  | high-first  |   ✓    |   ✓    | ✓ loopback + golden    |
   | Clemsa        |  18  | high-first  |   ✓    |   ✓    | ✓ loopback + golden    |
   | Mastercode    |  36  | high-first  |   ✓    |   ✓    | ✓ loopback + golden    |
   | CAME          |  12  | space-coded |   ✓    |   ✓    | ✓ loopback + golden    |
   | Ansonic       |  12  | space-coded |   ✓    |   ✓    | ✓ TX; RX aliases CAME¹ |
   | GateTX        |  24  | space-coded |   ✓    |   ✓    | ✓ loopback + golden    |
   | Holtek        |  40  | space-coded |   ✓    |   ✓    | ✓ loopback + golden    |
   | Holtek_HT12   |  12  | space-coded |   ✓    |   ✓    | ✓ loopback             |
   | PhoenixV2     |  52  | space-coded |   ✓    |   ✓    | ✓ loopback + golden    |
   | RcSwitch_1..12|  var | both        |   ✓    |   ✓    | ✓ loopback             |

   - "loopback" = transmitted by one CC1101 (`subghz txp <name> ...`) and decoded
     by another (`subghz rx`); key/bits/te round-trip.
   - "golden" = encoder output checked against absolute reference timings by
     `subghz selftest` (`rf_encoder_selftest`).
   - ¹ Ansonic and CAME are both 12-bit space-coded with a 1:2 ratio, so they are
     timing-ambiguous (the classic OOK alias class): a captured Ansonic
     frame decodes as `CAME` (key still correct). TX by name keeps Ansonic's own
     `te`. Distinguishing them would need exact-`te` matching, which the
     tolerance-based decoder intentionally does not do.

2. **KeeLoq rolling code.** Manufacturer keystore (`/mfcodes`) + block cipher
   live in `rf_keeloq.{h,cpp}`, together with `keeloq_build_hop()` — the pure
   per-manufacturer hop framing (button/serial mask/counter), shared by the
   counter step. The remaining per-frame state (counter step, key assembly)
   stays on `RfCodes::keeloq_step` in `rf_utils`, since it operates on a captured
   code; emulation transmits through the OOK engine. Supported manufacturers are
   those handled by `keeloq_build_hop`.
   The cipher is byte-identical to the reference algorithm (NLF, 528 rounds).
   Learning schemes are centralized in one dispatch, `keeloq_derive_man(type,
   fix, seed, key)`, shared by encode (`keeloq_step`) and decode
   (`keeloq_identify`): simple(1), normal(2), secure(3), magic_xor_type1(4),
   magic_serial_type1/2/3(6/7/8), erreka(12), pujol(13), aerf(14),
   simple_jcm(15). FAAC SLH(5), KingGates(10) and Jarolift(11) are separate
   protocols and are not handled here. `type` ids match the Flipper keystore, so
   a `/mfcodes` exported from there works verbatim. The keystore is read via
   `keeloq_mfcodes_fs()` (SD first, LittleFS fallback) and can be managed over
   the CLI with `subghz mfcodes add/list/clear`.
   `subghz keeloqtest` (`rf_keeloq_selftest`) round-trips every manufacturer
   family — including Pecinin and Rossi — and every supported learning type;
   `subghz keeloqfiletest` does the same driven by the real `/mfcodes`. Both are
   under `RF_DEBUG`.

   KeeLoq's over-the-air framing (11-pulse header, sync gap, 64 PWM bits at
   te 400/800) does not fit the factor-based registry model, so it has a dedicated
   encoder (`rf_keeloq_durations`/`rf_tx_keeloq`) and decoder (`rf_decode_keeloq`,
   wired into the RX paths via `rf_try_keeloq`). `sendRfCommand` routes
   `Protocol: KeeLoq` to the encoder; `subghz rx` decodes it and `subghz keeloqtx`
   emits it. Verified OTA between two CC1101 boards (simple/normal/secure types,
   manufacturer + button + counter recovered).

3. **RAW / BinRAW replay.** Any captured signal replays bit-exactly through the
   RAW path, independent of the registry. Families that require a bespoke
   per-protocol decoder — rolling-code derivatives and weather / TPMS sensors —
   are **not** named-decoded today; they are captured and replayed via RAW.
   Adding a named decoder for one of these means registering a decode/encode
   callback on `RfProtocolDef` (planned extension point), not a timing row.
