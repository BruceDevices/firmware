# RF / EMV host regression checks

Requires Python 3 and g++. Run from the repository root:

```sh
python3 tests/test_tembed_rf_antenna.py
ASAN_OPTIONS=detect_leaks=0 python3 tests/test_emv_protocol.py
ASAN_OPTIONS=detect_leaks=0 python3 tests/test_rf_raw_capture.py
```

The latter two compile with AddressSanitizer and UndefinedBehaviorSanitizer.
LeakSanitizer is disabled in the example because it is unavailable in some
containers; memory bounds and undefined-behavior checks remain enabled.

- Antenna: compiles the production switch block for the current CC1101 board,
  legacy board/module combinations and an unrelated board, then checks the
  315/433/868/915 MHz switch states and unnecessary writes.
- EMV: compiles the portable production parser using synthetic APDU transcripts.
  Covers variable AIDs, multiple applications, dynamic PDOL, both GPO formats,
  all AFL records, PAN/track-2 padding, optional fields, SW61/SW6C, malformed
  TLV/PDOL/AFL, full PN532 framing/checksums, extended responses and I/O failure.
  Also exercises 5,000 deterministic malformed inputs under the sanitizers.
- RAW capture: runs the production receive function with a fake radio. Covers
  named/alias presets, preserving initialized frequency/modulation, unsupported
  requests, signed timing round trips, 512-value lines, full-buffer
  metadata, prompt polling, clock wraparound and preserving the JS Back event.

These tests do not exercise physical RF range, RMT interrupts, PN532 bus timing
or real payment cards. They make no radio transmissions and contain no real
card data. Firmware builds and device checks remain separate validation steps.

## JavaScript RAW receive API

`require('subghz').readRaw(timeoutSeconds = 10, rxPreset = '')` accepts an optional
CC1101 preset name. The timeout is bounded to 1–60 seconds. An empty preset keeps
the existing default RX profile; unknown names and preset requests on external
non-CC1101 receivers return an empty string. Supported names/aliases are listed
in `src/modules/rf/protocols/rf_presets.cpp`.

Successful named-preset captures include `Bruce_RX_Preset` (canonical name),
`Bruce_RX_Registers` (configuration registers 00–2E), and `Bruce_Buffer_Full`.
The latter is 1 when the capture buffer filled, so a frame may be incomplete.
RAW mode preserves captured durations instead of normalizing repeats. The
existing 256-symbol RMT buffer is retained, so a long capture can be truncated;
full-buffer metadata makes this explicit. The M5 GPIO receiver keeps its existing capture buffer and
noise filter. These metadata fields do not configure arbitrary custom presets.
