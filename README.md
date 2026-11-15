# Bruce Scroller

**A Bluetooth phone remote built into Bruce Firmware for the LILYGO T-Embed CC1101 (ESP32-S3).** Turn the encoder to send mouse-wheel scrolling; use clicks for page and media controls.

[English](README.md) · [Русский](README.ru.md) · [简体中文](README.zh-CN.md)

Bruce Scroller is a small, board-specific modification of [Bruce Firmware](https://github.com/BruceDevices/firmware). It keeps Bruce's existing menus and features and adds a BLE HID scrolling mode for the **LILYGO T-Embed CC1101 (ESP32-S3)**.

## What it does

- Sends Bluetooth LE HID mouse-wheel reports from the rotary encoder.
- Sends Volume Down on a single encoder click and Volume Up on a double click. MangaLib may map these media keys to next and previous page; behavior depends on the Android app and its settings.
- Cycles through five scroll sensitivity levels when the encoder is held.
- Attempts to move the mouse pointer away from the center after connection. Android controls pointer visibility, so the pointer may remain visible.
- Draws the screen once and updates changing status rows, avoiding the old periodic full-screen redraw.
- Leaves Bruce's existing `Media Cmds` and other Bruce modes in place.

In the maintainer's tests, vertical scrolling was comfortable in scrollable Android screens, including the standard TikTok feed, and the remote was used with MangaLib. TikTok photo posts/carousels did not advance with the wheel. The wheel sends vertical scroll events, not touchscreen swipe gestures. Phone model and software versions were not recorded; Instagram Reels, YouTube Shorts, and other apps have not been verified.

## Compatibility

| Hardware / software | Status |
| --- | --- |
| [LILYGO T-Embed CC1101 (ESP32-S3)](https://github.com/Xinyuan-LilyGO/documentation/blob/master/en/products/t-embed-series/t-embed-cc1101/index.md) | Supported target; PlatformIO build verified for this environment |
| Other Bruce-supported boards | Not tested by this project; no compatibility claim |
| Android BLE HID host | Designed to pair as `Bruce Scroller`; app behavior varies |
| iOS, desktop, or other hosts | Not verified |

This project is based on Bruce Firmware commit [`2b6ba4d490bfcf94ce2574c6154537dbd5838c3d`](https://github.com/BruceDevices/firmware/commit/2b6ba4d490bfcf94ce2574c6154537dbd5838c3d). See the upstream [Bruce README](README.bruce.md), [wiki](https://github.com/BruceDevices/firmware/wiki), and [third-party notices](THIRD_PARTY.md) for Bruce features and component attributions.

## Controls

| Control | Action |
| --- | --- |
| Rotate right | Scroll down |
| Rotate left | Scroll up |
| Single encoder click | Volume Down; next page where the app maps it that way |
| Double encoder click | Volume Up; previous page where the app maps it that way |
| Hold encoder | Cycle scroll sensitivity from 1 to 5 |
| Side Back button | Exit Bruce Scroller to Bruce |

Wheel events are sent in small steps at a 16 ms interval. A bounded queue smooths short encoder bursts; very fast turns can exceed the queue and some steps can be dropped. The remote sends wheel events rather than touch swipes, so it cannot advance TikTok photo carousels in the maintainer's test. On connection the device sends a short relative mouse movement to try to park the pointer near the edge; it does not guarantee a hidden pointer.

## Build

The build uses the Bruce PlatformIO project and its pinned platform and library configuration. Install Git, Python 3, and [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), then:

```sh
git clone --recurse-submodules https://github.com/andreychatgptac-sketch/bruce-scroller.git
cd bruce-scroller
git submodule update --init --recursive
python -m pip install platformio
pio run -e lilygo-t-embed-cc1101
```

The verified environment is `lilygo-t-embed-cc1101`; a successful build for it does not imply support for other boards. The first build downloads toolchains and libraries and can take a while.

GitHub Actions checks this same board on relevant pushes and pull requests. It does not build other boards or publish release assets.

## Install and use

1. Back up anything you need from the device before flashing. Installing firmware replaces the running firmware.
2. Connect the **LILYGO T-Embed CC1101 (ESP32-S3)** over USB and identify its serial port (`pio device list`).
3. Upload the build for the exact PlatformIO environment:

   ```sh
   pio run -e lilygo-t-embed-cc1101 -t upload --upload-port <PORT>
   ```

   Replace `<PORT>` with the detected port, such as `COM5` on Windows or `/dev/ttyACM0` on Linux. The uploader uses this environment's partition and flash settings; do not flash this application image at an arbitrary offset.
4. On Bruce, open **Bluetooth → Bruce Scroller**.
5. On the phone, pair with **Bruce Scroller**, then test scrolling and buttons in the target app.
6. Press the side **Back** button to leave the mode and return to Bruce.

If Android has a stale pairing, remove the old Bruce HID entry from Bluetooth settings and pair again. No special Android app is required for standard BLE HID, but application behavior is app-specific.

## Release files

When a release is published, its merged firmware asset should be named `Bruce-Scroller-<version>-LILYGO-T-Embed-CC1101-ESP32-S3.bin` and include the exact source commit, SHA-256, build environment, and flashing instructions. No release binary is provided by this source checkout; build it locally using the steps above.

## License and attribution

Bruce Firmware is licensed under **GNU AGPL v3.0 or later**; this repository retains Bruce's license and attribution files. Bruce includes components under additional licenses; see [`THIRD_PARTY.md`](THIRD_PARTY.md). Bruce Scroller is an independent community modification and is not an official BruceDevices or LILYGO product.

## Repository metadata

Suggested GitHub description: **Bruce Firmware BLE HID phone remote for LILYGO T-Embed CC1101 (ESP32-S3): scroll and control media.**

Suggested topics: `bruce-firmware`, `lilygo-t-embed-cc1101`, `esp32-s3`, `ble-hid`, `bluetooth-le`, `android`, `phone-remote`, `media-remote`, `scroll-controller`, `manga-reader`, `social-media`, `firmware`.
