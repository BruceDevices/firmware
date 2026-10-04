# Hosyond 3.5" ESP32-S3 (ST77922, QSPI)

ESP32-S3-N16R8, 320x480 ST77922 over quad SPI, ST77922 I2C touch @ 0x55.
Build env: `hosyond-35-s3`. First hardware bring-up: 2026-10-03.

## Status
- Display: working (Bruce menu renders, landscape, correct colors without the invert menu).
- Touch: working (verified 2026-10-04). Report slots: byte0 bit7=valid, bits[5:0]=x_h; coordinates are portrait-native panel pixels and the existing rotation mapping in `interface.cpp` is correct.
- SD: mounts in 4-bit SDMMC mode with a card inserted (verified 2026-10-04; the `0x107` boot error only means an empty slot). File browse/write not yet exercised.
- RGB LED: working (WS2812B, 3-channel GRB, GPIO40); Bruce's LED menu appears via `HAS_RGB_LED` + `LED_TYPE/ORDER/COUNT` in the variant header.
- Audio: working (verified 2026-10-04: `tone` and `say` serial commands audible on a connected speaker). Bruce gates sound on `HAS_NS4168_SPKR` with `BCLK/WCLK/DOUT/MCLK`; the board supplies `_setup_codec_speaker()` which drives the amp enable (GPIO1, active LOW, defaults off) and programs the ES8311 (I2C 0x18, shared bus) in I2S slave mode. Chip ID 0x8311 is printed to serial on use. Microphone path (ES8311 ADC) not implemented.
- Battery: calibrated ADC (`analogReadMilliVolts`, 16-sample average, x2 for the 100k/100k divider), smoothed, mapped through a Li-ion curve; floored at 1% so Bruce (which hides the indicator at 0%) always shows it. Verified 2026-10-04 with a direct 1000 mAh cell: it reads terminal voltage, so it is load sensitive (LED/backlight cost ~100 mV of sag on a small cell) and reads ~4.1 V (high %) while the TP4054 is charging. There is no VBUS sense or charge-status GPIO, so charging can't be detected.
- Display clock is 40 MHz (vendor uses 80 MHz); 40 MHz judged good enough.
- Not yet done: microphone, IR pin assignment (spare GPIOs are IO45/IO46 only), repointing Bruce's default SPI-radio pins away from the display bus.
- Free pins: only IO45/IO46, the shared I2C pair (SDA38/SCL39) and the UART header (43/44) are exposed. SPI radios (CC1101/NRF24/W5500, ~5 pins) do not fit; I2C and UART modules and IR do.
- Serial shell: `help` over USB serial lists Bruce's commands (`tone`, `say`, `gpio`, `settings`, `led`, ...), handy for exercising hardware without touching the UI.
- Touch debugging tip: a tiny Arduino sketch that scans I2C (touch is 0x55 on SDA38/SCL39) and prints reports from register 0x0014 finds protocol bugs much faster than debugging inside Bruce.

## Display notes (the hard-won part)
- The panel needs the draw window **x start and width aligned to 4 pixels**. Arbitrary windows (text glyphs,
  icons, 1px lines) corrupt the raster: interlaced/offset smear, tinted text. Aligned solid fills hide the problem.
- `Panel_ST77922` (`lgfx_st77922.h`) therefore draws into a PSRAM framebuffer (`Panel_FrameBufferBase`), and
  flushes the dirty rectangle widened to a multiple of 4 pixels in x. Rotation is done in software, the panel
  stays at its native portrait MADCTL.
- `Panel_ST77922HW` is a `Panel_ST77916` subclass that only provides the QSPI plumbing and the vendor init
  sequence (from the vendor Arduino `Simple_test`). It is never drawn to directly.
- Pixel format is plain RGB565 (COLMOD 0x01 or 0x55 both work). MADCTL bit 3 swaps R/B, hence `TFT_RGB_ORDER=1`.
- INVON is the panel's natural state, so `Panel_ST77922::setInvert` is flipped and the ini uses `TFT_INVERTION=0`.
- The `HOSYOND_TEST_PATTERN` define in `interface.cpp` draws a corner/cross/image test card at boot (8 s hold),
  useful for checking geometry after display changes.

## Build and flash (Windows)
- Build with a fresh build cache if results look stale: `PLATFORMIO_BUILD_CACHE_DIR=<new dir>`; the repo-wide
  `.pio/buildcache` once returned an old firmware.bin.
- Flash with esptool directly (`pio run -t upload` crashes on cp1252 consoles):
  `python -m esptool --chip esp32s3 --port COMx --baud 921600 write-flash 0x0 Bruce-hosyond-35-s3.bin`
- Bruce persists rotation and color inversion in flash; `erase-flash` when testing display defaults.
- Run builds with PlatformIO's `Scripts` dir on PATH (`export PATH=/c/Users/<you>/.platformio/penv/Scripts:$PATH`); otherwise `patch.py`'s nested `pio pkg exec` fails.
- `patch.py` swaps in a patched `libnet80211.a` guarded by `.patched`. If another PlatformIO project refreshes the
  libs package, Bruce can fail to link with `cannot find -lnet80211`; re-run the patch (objcopy
  `--weaken-symbol=ieee80211_raw_frame_sanity_check libnet80211.a.old libnet80211.a`, then create `.patched`).
