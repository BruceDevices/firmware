# Heltec V4 Expansion V2 policy tests

Run from the repository root:

```sh
python tests/heltec_v4_expansion_v2/run.py
```

The runner uses a native `g++` or `clang++`, or PlatformIO's installed Windows
MinGW compiler. An explicit compiler path can be supplied with `--compiler`.
Executables are built in a temporary directory and removed after the run.
No PlatformIO firmware build, USB connection, Arduino headers, or external test
framework is required.

The C++ tests include the production `board_input.h` directly. They cover CHSC
report validation, all native pixels, rotated corner and navigation-strip
coordinates, bounded one-to-one mapping at every rotation, button debounce,
quick taps, single hold events, suppression of release clicks after a hold,
release near the hold deadline, boot/wake with a button held, independent
buttons, and `millis()` wraparound. Input-queue tests cover its eight-event
capacity, dropping the newest event on overflow, FIFO order through repeated
wraparound, combined button actions, and multiple touch events retained while
the UI finishes handling an earlier event.

The runner also compiles `lora_band_tests.cpp` twice against the production
`lora_board.h`: with the Heltec's 863–928 MHz frontend limits, and without board
limits to check generic behavior. Coverage includes the complete occupied
channel at both band edges, exact and immediately adjacent boundary values,
invalid/nonfinite inputs, imported 434.5 MHz settings, and both default
frequencies. This checks band policy; RadioLib remains responsible for validating
whether a chosen modulation bandwidth is supported by the transceiver.

`file_hash_tests.cpp` exercises the production streaming helper used by
`md5File()`. A recording hash sink verifies that all bytes reach the hash,
including embedded/trailing NULs and a 3.8 MB fixture, with reads bounded to
512 bytes and a yield after each chunk. Empty files, partial reads, read errors,
invalid lengths, and changing file sizes are covered. The host tests check the
streaming path; the cryptographic algorithm remains Arduino's MD5Builder.

The rotation expectations match the ST7789 driver coordinate convention:
rotation 0 is native portrait; rotation 1 is portrait plus 90 degrees. Physical
display orientation, actual touch sampling latency, and event consumption by
Bruce's input task still need device testing.
