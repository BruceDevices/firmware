# SubGHz Legacy-to-Unified Parity Checklist

Last update: 2026-05-23

Goal: replace `RF` legacy module by `SubGHz` unified module without losing capabilities.

## Legacy RF features mapped into SubGHz unified

| Legacy RF feature | Unified location | Status |
| --- | --- | --- |
| Scan/copy | `SubGHz -> Legacy+ Tools -> Scan/copy (Legacy+)` | done |
| Record RAW | `SubGHz -> Legacy+ Tools -> Record RAW` | done (`!LITE_VERSION`) |
| Custom SubGhz | `SubGHz -> Legacy+ Tools -> Custom SubGhz` | done (`!LITE_VERSION`) |
| Spectrum | `SubGHz -> Legacy+ Tools -> Spectrum` | done |
| RSSI Spectrum | `SubGHz -> Legacy+ Tools -> RSSI Spectrum` | done (`!LITE_VERSION`) |
| SquareWave Spec | `SubGHz -> Legacy+ Tools -> SquareWave Spec` | done (`!LITE_VERSION`) |
| Spectogram | `SubGHz -> Legacy+ Tools -> Spectogram` | done (`!LITE_VERSION`) |
| Listen | `SubGHz -> Legacy+ Tools -> Listen` | done (board/audio dependent) |
| Bruteforce | `SubGHz -> Legacy+ Tools -> Bruteforce` | done (`!LITE_VERSION`) |
| Jammer | `SubGHz -> Legacy+ Tools -> Jammer` | done (`!LITE_VERSION`) |
| RF TX/RX pin + module/freq config | `SubGHz -> Legacy+ Tools -> RF Config` | done |

## New advanced features kept in Unified

| Advanced feature | Unified location | Status |
| --- | --- | --- |
| Protocol-native RX identify | `SubGHz -> RX (Advanced) -> Scan & Identify` | done |
| Scan/copy with decode + rolling actions | `SubGHz -> RX (Advanced) -> Scan/Copy` | done |
| Decoder registry and last decode UI | `SubGHz -> RX (Advanced) -> Decoder UI` | done |
| Rolling interface from decoded frames | `SubGHz -> RX (Advanced) -> Rolling UI` | done |
| Protocol-native TX from `.sub` with fallback | `SubGHz -> TX (Advanced)` | done |
| Analyze `.sub` Bruce + Flipper | `SubGHz -> Analyze (Advanced)` | done |
| Recent decoded frame history | `SubGHz -> Recent` | done |
| Runtime profile/filter/frequency/range | `SubGHz -> Settings` | done |

## Exit criteria to deprecate RF menu

1. Manual regression pass confirms each legacy RF feature above works from `SubGHz` unified menu.
2. CLI/JS legacy routes stay backward-compatible while unified routes remain stable.
3. Hardware validation completed on at least one CC1101 board for RX/TX/scan paths.
4. Team sign-off on removal plan for top-level `RF` menu entry.
