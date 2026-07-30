# Prebuilt firmware images

Convenience binaries so you can flash from another machine without rebuilding.
This folder is **not** part of the build — PlatformIO only compiles `src/`, so
nothing here affects firmware operation. Safe to delete or regenerate anytime.

| File | Board / env | Notes |
| --- | --- | --- |
| `Bruce-CYD-2432S028.bin` | `CYD-2432S028` (ESP32, ILI9341, resistive touch) | Includes the Cam Detector features (Camera Radar catch-all: AP+BLE+LAN+P2P / Deauth All + Target / TUTK Watch / persistent Flock / multi-vendor Bodycam / RayBan). Merged image → flash at offset `0x0`. |

## Verify then flash

```sh
sha256sum Bruce-CYD-2432S028.bin
# eb7b68847f86e54d9889ef73831ab6821f6878e21142bd630679f5144e3ec768

esptool.py --chip esp32 --port /dev/ttyACM0 write_flash 0x0 Bruce-CYD-2432S028.bin
```

Or drag the `.bin` into the [Web Flasher](https://bruce.computer/flasher) (offset 0x0).

If the display shows inverted/negative colours, your panel is the 2-USB variant —
rebuild/flash the `CYD-2USB` env instead.

## Regenerate

```sh
pio run -e CYD-2432S028
cp Bruce-CYD-2432S028.bin prebuilt/
```
