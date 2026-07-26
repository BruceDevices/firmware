# Prebuilt firmware images

Convenience binaries so you can flash from another machine without rebuilding.
This folder is **not** part of the build — PlatformIO only compiles `src/`, so
nothing here affects firmware operation. Safe to delete or regenerate anytime.

| File | Board / env | Notes |
| --- | --- | --- |
| `Bruce-CYD-2432S028.bin` | `CYD-2432S028` (ESP32, ILI9341, resistive touch) | Includes the Cam Detector features (Camera Radar catch-all: AP+BLE+LAN+P2P / Deauth All + Target / TUTK Watch / Flock / Axon / RayBan). Merged image → flash at offset `0x0`. |

## Verify then flash

```sh
sha256sum Bruce-CYD-2432S028.bin
# 4da8668747eb347ac5bfe2954ef90a3332c4f8bb52abc11da836e0df70e107ac

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
