# SubGHz Advanced v1 - Release Notes

Date: 2026-05-23
Scope: Bruce SubGHz migration from Unleashed library via adapter/compat (no vendor source edits)

## Highlights

- Added advanced `SubGHz` main menu while keeping legacy `RF` menu unchanged.
- Added advanced RX decode pipeline with protocol-native identification.
- Added advanced TX path for `.sub` files using protocol-native transmitters with safe fallback to legacy RcSwitch TX.
- Added offline `.sub` analysis for Bruce and Flipper formats.
- Added protocol profiles:
- `CORE`: default compact set for general boards.
- `FULL`: full runtime registry for CC1101-oriented targets (except intentional `RAW/BinRAW` exclusion).

## New Surfaces

- CLI group: `subghz_adv`
- `subghz_adv rx [freq] [timeout]`
- `subghz_adv tx_file <path>`
- `subghz_adv analyze_file <path>`
- `subghz_adv protocols`
- `subghz_adv profile <core|full>`
- `subghz_adv filter <protocol|off>`

- JS namespace: `subghzAdvanced`
- `subghzAdvanced.read(timeoutSec?)`
- `subghzAdvanced.analyzeFile(path)`
- `subghzAdvanced.transmitFile(path, hideDefaultUI?)`

## Legacy Compatibility

- Existing legacy routes remain additive and backward-compatible.
- Legacy RX save flow now enriches captures with detected fields when available:
- `Detected_Protocol`
- `Detected_Key`
- `Detected_Serial`
- `Detected_Button`
- `Detected_Counter`
- Legacy TX commands (`tx_from_file`, `tx_from_buffer`, `sendCustomRF`) attempt advanced TX first and fallback to RcSwitch behavior.

## Validation Artifacts

- Protocol registry coverage check:
- `tools/subghz_protocol_coverage_check.py`
- Current result: `FULL` in sync with vendor registry excluding intentional `RAW/BinRAW`.

- Upstream sample matrix:
- `SUBGHZ_V1_SAMPLE_MATRIX.md`
- Decoder fixtures parsed: `47`
- CORE fixture coverage: `17/17`
- FULL fixture coverage: `46/56`

## Known Limits (v1)

- `RAW/BinRAW` remains intentionally disabled in FULL runtime profile in v1.
- Full parity for all Unleashed UI scenes is still in progress.
- 10 FULL protocols currently have no upstream decoder fixture in the imported unit-test battery:
- `Allstar Firefly`
- `Beninca ARC`
- `Cham_Code`
- `Ditec GOL4`
- `Elplast`
- `Honeywell Sec`
- `Jarolift`
- `KeyFinder`
- `Nord ICE`
- `Treadmill37`
- Hardware validation with live captures is still pending for formal v1 closure.

## Recommended Next Steps

1. Capture live RF samples for the 10 FULL-only protocols without fixture coverage.
2. Run hardware validation on at least one CC1101 target and append evidence to the sample matrix.
3. Decide whether v1.1 should include optional `RAW/BinRAW` enablement via additional compat shims.
