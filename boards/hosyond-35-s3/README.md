# Hosyond 3.5" ESP32-S3 (ST77922, QSPI)

ESP32-S3-N16R8, 320x480 ST77922 over quad SPI, ST77922 I2C touch @ 0x55.
Build env: `hosyond-35-s3`. First hardware bring-up: 2026-10-03.

## Status
- Display: working (Bruce menu renders, landscape, correct colors without the invert menu).
- Touch: register protocol follows the vendor driver, not yet verified on hardware.
- Not yet checked: SD (4-bit MMC failed to mount at boot), RGB LED, audio, battery reading, 80 MHz bus clock (currently 40 MHz).

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
- `patch.py` swaps in a patched `libnet80211.a` guarded by `.patched`. If another PlatformIO project refreshes the
  libs package, Bruce can fail to link with `cannot find -lnet80211`; re-run the patch (objcopy
  `--weaken-symbol=ieee80211_raw_frame_sanity_check libnet80211.a.old libnet80211.a`, then create `.patched`).
