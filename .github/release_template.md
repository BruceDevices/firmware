## Bruce Scroller <version>

BLE HID phone remote for the **LILYGO T-Embed CC1101 (ESP32-S3)**.

### Changes

- <user-visible changes>

### Compatibility

- Target: LILYGO T-Embed CC1101 (ESP32-S3)
- Bruce base commit: `<full commit SHA>`
- PlatformIO environment: `lilygo-t-embed-cc1101`
- Build: PASS / FAIL (link the workflow run)
- Other boards and phone apps: not verified unless listed with exact test details

### Firmware asset

Attach the merged PlatformIO image named:

`Bruce-Scroller-<version>-LILYGO-T-Embed-CC1101-ESP32-S3.bin`

Include its SHA-256 in the asset notes.

### Install

1. Back up any data you need; flashing replaces the installed firmware.
2. Download the firmware asset above.
3. Connect the LILYGO T-Embed CC1101 over USB and flash the merged image at offset `0x0` with Espressif's [`esptool`](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/basic-commands.html):

   ```sh
   esptool --chip esp32s3 --port <PORT> write-flash 0x0 Bruce-Scroller-<version>-LILYGO-T-Embed-CC1101-ESP32-S3.bin
   ```

   Replace `<PORT>` with the serial port for the device. Verify the SHA-256 before flashing.
4. In Bruce, open **Bluetooth → Bruce Scroller**, then pair with **Bruce Scroller** on the phone.

No release is valid until its attached binary was built from the stated source commit and the hash matches the published asset.
